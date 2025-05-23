// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include <string_view>
#include "EmbUI.h"
#include "ui.h"
#include "basicui.h"
#include "ftpsrv.h"
#include "nvs_handle.hpp"

#define POST_ACTION_DELAY   10      // delay for large posts processing in ms
//#define POST_LARGE_SIZE     1024    // large post threshold

// instance of embui object
EmbUI embui;


union MacID
{
    uint64_t u64;
    uint8_t mc[8];
};

// forward declaration
void wsDataHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

/**
 * WebSocket events handler
 *
 */
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    LOGV(P_EmbUI, printf, "=WS MSG= len: %u\n", len);
    // pass data to handler fuction
    if(type == WS_EVT_DATA)
        return wsDataHandler(server, client, type, arg, data, len);

    if(type == WS_EVT_CONNECT){
        LOGD(P_EmbUI, printf, "WS_EVT_CONNECT:%s id:%u\n", server->url(), client->id());
        {
            Interface interf(client);
            embui.publish_language(&interf);

            if (!embui.action.exec(&interf, {}, A_ui_page_main))    // call user defined mainpage callback
                basicui::page_main(&interf, {}, NULL);             // if no callback was registered, then show default stub page
        }
        embui.send_pub();
        return;
    }

    if(type == WS_EVT_DISCONNECT){
        LOGD(P_EmbUI, printf, "WS_EVT_DISCONNECT:%s id:%u\n", server->url(), client->id());
        return;
    }

    if(type == WS_EVT_ERROR){
        LOGD(P_EmbUI, printf, "ws[%s][%u] WS_EVT_ERROR(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
        //httpCallback("sys_WS_EVT_ERROR", "", false); // сообщим об ошибке сокета
        return;
    }

    if(type == WS_EVT_PONG){
        LOGD(P_EmbUI, printf, "ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
        return;
    }
}


void wsDataHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    AwsFrameInfo *info = (AwsFrameInfo*)arg;

    // fragmented messages reassembly is not (yet) supported
    if(!info->final || info->index != 0 || info->len != len){
        LOGV(P_EmbUI, printf, "WS fragment fin:%u, idx:%u, len:%u\n", info->final, info->index, len);
        return;
    }

    // ignore packets without "pkg":"post" marker
    std::string_view payload((const char *)data, len);
    if (payload.substr(1, 12) != "\"pkg\":\"post\""){
        LOGW(P_EmbUI, println, "bad post pkt");
        return;
    }

    JsonDocument *res = new JsonDocument();

    DeserializationError error = deserializeJson((*res), (const char*)data, len); // deserialize via copy to prevent dangling pointers in action()'s
    if (error){
        LOGE(P_EmbUI, printf, "WS_EVT_DATA deserialization err: %d\n", error.code());
        delete res;
        return;
    }

    // switch context to the main loop() for processing data
    Task *t = new Task(POST_ACTION_DELAY, TASK_ONCE,
        [res](){
            JsonObject o = res->as<JsonObject>();
            // if there is nested data in the object
            // call action handler for post'ed data
            embui.post(o);
            delete res; },
        &ts, false, nullptr, nullptr, true
    );
    if (t)
        t->enableDelayed();
}

// EmbUI constructor
EmbUI::EmbUI() : server(80), ws(EMBUI_WEBSOCK_URI){
        _getmacid();

        tAutoSave.set(EMBUI_AUTOSAVE_TIMEOUT * TASK_SECOND, TASK_ONCE, [this](){LOGD(P_EmbUI, println, "AutoSave"); save();} );    // config autosave timer
        ts.addTask(tAutoSave);
}

EmbUI::~EmbUI(){
    ts.deleteTask(tAutoSave);
    ts.deleteTask(tHouseKeeper);
    delete tValPublisher;
    delete tMqttReconnector;
#ifndef EMBUI_NOFTP
    ftp_stop();
#endif
}



void EmbUI::begin(){
    int retry_cnt = 3;

    // монтируем ФС только один раз при старте
    while(!LittleFS.begin(true)){   // format FS if corrupted
        LOGE(P_EmbUI, println, "LittleFS initialization error, retrying...");
        --retry_cnt;
        delay(100);
        if (!retry_cnt){
            LOGE(P_EmbUI, println, "FS dirty, I Give up!");
            return;
        }
    }

    load();                 // load embui's config from json file

    LOGD(P_EmbUI, print, "UI CONFIG: ");
    LOG_CALL(serializeJson(_cfg, EMBUI_DEBUG_PORT));

    // restore language value
    _lang = _cfg[V_LANGUAGE] | "en";

    // restore Time settings
    if (_cfg[V_timezone]){
        std::string_view tzrule(_cfg[V_timezone].as<const char*>());
        TimeProcessor::getInstance().tzsetup(tzrule.substr(4).data());   // cutoff '000_' prefix
    } else {
        // instantiate NTP to set required hooks
        TimeProcessor::getInstance();
    }

    // start-up WiFi
    wifi = std::make_unique<WiFiController>(this, _cfg[V_APonly]);

    if (_cfg[V_noNTPoDHCP])
        wifi->ntpodhcp(false);

    wifi->init();
    
    // set WebSocket event handler
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // install WebSocker feeder
    feeders.add(std::make_unique<FrameSendWSServer> (&ws));

    // install EmbUI http handlers
    http_set_handlers();
    server.begin();

    // register system menu handlers
    basicui::register_handlers();

    setPubInterval(EMBUI_PUB_PERIOD);

    tHouseKeeper.set(TASK_SECOND, TASK_FOREVER, [this](){
            ws.cleanupClients(EMBUI_MAX_WS_CLIENTS);
        } );
    ts.addTask(tHouseKeeper);
    tHouseKeeper.enableDelayed();

    // create and start MQTT client if properly configured
    mqttStart();

// FTP server
#ifndef EMBUI_NOFTP
    if (_cfg[P_ftp]) ftp_start();
#endif
}

/**
 * @brief - process posted data for the registered action
 * looks for registered action for the section name and calls the action with post data if found
 */
void EmbUI::post(JsonObjectConst data){
    LOGD(P_EmbUI, print, "post() "); //LOG_CALL(serializeJson(data, EMBUI_DEBUG_PORT)); LOG(println);
    JsonObjectConst odata = data[P_data].as<JsonObjectConst>();
    Interface interf(&feeders);

    // echo back injected data to all available feeders IF request 'data' object is not empty
    if (odata.size() && feeders.available()){
        interf.json_frame_value(odata);
        interf.json_frame_flush();
    }

    // reflect post'ed data to MQTT (todo: do this on-demand)
    JsonVariantConst jvc(data);
    publish(C_pub_post, jvc);

    // execute callback actions
    action.exec(&interf, odata, data[P_action].as<const char*>());
}

void EmbUI::send_pub(){
    if (mqttAvailable()) _mqtt_pub_sys_status();
    if (!ws.count()) return;
    Interface interf(&ws);     // only websocket publish!
    basicui::embuistatus(&interf);
    action.exec(&interf, {}, A_publish);   // call user-callback for publishing task
}

void EmbUI::handle(){
    ts.execute();           // run task scheduler
// FTP server
#ifndef EMBUI_NOFTP
    ftp_loop();
#endif
}

/**
 * @brief - set interval period for send_pub() task in seconds
 * 0 - will disable periodic task
 */
void EmbUI::setPubInterval(uint16_t _t){
    if (!_t && tValPublisher){
        delete tValPublisher;
        tValPublisher = nullptr;
        return;
    }

    if(tValPublisher)
        tValPublisher->setInterval(_t * TASK_SECOND);
    else
        tValPublisher = new Task(_t * TASK_SECOND, TASK_FOREVER, [this](){ send_pub(); }, &ts, true );
}

/**
 * @brief - restart config autosave timer
 * each call postpones cfg write to flash
 */
void EmbUI::autosave(bool force){
    if (force){
        tAutoSave.disable();
        save();
    } else {
        tAutoSave.restartDelayed();
    }
};

/**
 * @brief get/set device hosname
 * if hostname has not been set or empty returns autogenerated __IDPREFIX-[mac_id] hostname
 * autogenerated hostname is NOT saved into persistent config
 * 
 * @return const char* current hostname
 */
const char* EmbUI::hostname(){

    JsonVariant h = _cfg[V_hostname];
    if (h.is<const char*>())
        return h.as<const char*>();

    if (autohostname.get())
        return autohostname.get();

    autohostname.reset(new char[sizeof(EMBUI_IDPREFIX) + sizeof(mc) * 2]);
    sprintf(autohostname.get(), PSTR(EMBUI_IDPREFIX "-%s"), mc);
    LOGD(P_EmbUI, printf, "generate autohostname: %s\n", autohostname.get());

    return autohostname.get();
}

const char* EmbUI::hostname(const char* name){
    if (name)
        _cfg[V_hostname] = name;
    else
        _cfg.remove(V_hostname);

    return hostname();
};

/**
 * формирует chipid из MAC-адреса вида 'ddeeff'
 */
void EmbUI::_getmacid(){
    MacID _mac;
    _mac.u64 = ESP.getEfuseMac();

    sprintf(mc, "%02X%02X%02X%02X%02X%02X", _mac.mc[0],_mac.mc[1],_mac.mc[2], _mac.mc[3], _mac.mc[4], _mac.mc[5]);
    LOGD(P_EmbUI, printf,"UI ID:%s\n", mc);
}

void EmbUI::setLang(const char* lang){
    if (!lang) return;
    _lang = lang;
    _cfg[V_LANGUAGE] = _lang;
    if (_lang_cb)
        _lang_cb(_lang.c_str());
    autosave();
}

const char* EmbUI::getLang() const {
    return _lang.c_str();
}

void EmbUI::publish_language(Interface *interf){
    interf->json_frame_interface();
    // load translation data for new lang
    String path(getLang());
    path += (char)0x2e; // '.'
    path += P_data;
    interf->json_section_uidata();
        interf->uidata_xload(P_sys, EMBUI_JSON_i18N, path.c_str(), true);
    interf->json_section_end();
    interf->json_section_begin(P_manifest);
        JsonObject o( interf->json_object_create() );
        o[P_lang] = getLang();

    interf->json_frame_flush();
}

void EmbUI::save(const char *cfg){
    embuifs::serialize2file(_cfg, cfg ? cfg : EMBUI_cfgfile);
    LOGD(P_EmbUI, println, "Save config file");
}

void EmbUI::load(const char *cfgfile){
    LOGD(P_EmbUI, println, "Load config file");
    auto err = embuifs::deserializeFile(_cfg, cfgfile ? cfgfile : EMBUI_cfgfile);
    if (err.code() != DeserializationError::Code::Ok || _cfg.isNull()){
        _cfg.to<JsonObject>();
    }
}

void EmbUI::cfgclear(){
    LOGI(P_EmbUI, println, "!CLEAR SYSTEM CONFIG!");
    _cfg.to<JsonObject>();
    LittleFS.remove(EMBUI_cfgfile);
    // wipe NVS entries
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(P_EmbUI, NVS_READWRITE, &err);
    if (err == ESP_OK)
        handle->erase_all();
}



void ActionHandler::add(const char* id, const actionCallback_t& callback){
    if (!id) return;
    actions.emplace_back(id, callback);
    LOGD(P_EmbUI, printf, "action register: %s\n", id);
}

void ActionHandler::replace(const char* id, const actionCallback_t& callback){
    auto i = std::find_if(actions.begin(), actions.end(), [&id](const section_handler_t &arg) { return std::string_view(arg.action).compare(id) == 0; } );

    if ( i == actions.end() )
        return add(id, callback);
    
    i->cb = callback;
}

void ActionHandler::remove(const char* id){
    actions.remove_if([&id](const section_handler_t &arg) { return std::string_view(arg.action).compare(id) == 0; });
}

size_t ActionHandler::exec(Interface *interf, JsonObjectConst data, const char* action){
    size_t cnt{0};
    if (!action) return cnt;      // return if action is empty string
    std::string_view a(action);

    for (const auto& i : actions){
        std::string_view item(i.action);
        if (a.length() < item.length()) continue;  // skip handlers with longer names, obviously a mismatch

        // check if action has a wildcard suffix "*" and it does not match
        if (std::char_traits<char>::eq(item.back(), 0x2a) && !starts_with(a, item.substr(0, item.size()-1) ))       // 0x2a  == '*'
            continue;

        // check if action has a wildcard prefix "*" and it does not match
        if (std::char_traits<char>::eq(item.front(), 0x2a) && !ends_with(a, item.substr(1) ))
            continue;

        if (!std::char_traits<char>::eq(item.back(), 0x2a) &&
            !std::char_traits<char>::eq(item.front(), 0x2a) &&
            a.compare(item) != 0
        ) continue;     // full string compare

        // execute action callback
        LOGI(P_EmbUI, printf, "exec act:%s hndlr:%s\n", action, item.data());
        i.cb(interf, data, action);
        ++cnt;
    }

    return cnt;
}

void ActionHandler::set_mainpage_cb(const actionCallback_t& callback){
    replace(A_ui_page_main, callback);
}

void ActionHandler::set_settings_cb(const actionCallback_t& callback){
    replace(A_ui_blk_usersettings, callback);
}

