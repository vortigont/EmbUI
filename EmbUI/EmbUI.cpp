// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include <string_view>
#include "EmbUI.h"
#include "ui.h"
#include "basicui.h"
#include "ftpsrv.h"

#define POST_ACTION_DELAY   50      // delay for large posts processing in ms
#define POST_LARGE_SIZE     1024    // large post threshold

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
    // pass data to handler fuction
    if(type == WS_EVT_DATA)
        return wsDataHandler(server, client, type, arg, data, len);

    if(type == WS_EVT_CONNECT){
        LOG(printf_P, PSTR("CONNECT ws:%s id:%u\n"), server->url(), client->id());
        {
            Interface interf(client);
            if (!embui.action.exec(&interf, nullptr, A_get_ui_page_main))    // call user defined mainpage callback
                basicui::page_main(&interf, nullptr, NULL);             // if no callback was registered, then show default stub page
        }
        embui.send_pub();
        return;
    }

    if(type == WS_EVT_DISCONNECT){
        LOG(printf_P, PSTR("ws[%s][%u] disconnect\n"), server->url(), client->id());
        return;
    }

    if(type == WS_EVT_ERROR){
        LOG(printf_P, PSTR("ws[%s][%u] error(%u): %s\n"), server->url(), client->id(), *((uint16_t*)arg), (char*)data);
        httpCallback(F("sys_WS_EVT_ERROR"), "", false); // сообщим об ошибке сокета
        return;
    }

    if(type == WS_EVT_PONG){
        LOG(printf_P, PSTR("ws[%s][%u] pong[%u]: %s\n"), server->url(), client->id(), len, (len)?(char*)data:"");
        return;
    }
}


void wsDataHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    AwsFrameInfo *info = (AwsFrameInfo*)arg;

    // fragmented messages reassembly is not (yet) supported
    if(!info->final || info->index != 0 || info->len != len)
        return;

    LOG(printf_P, PSTR("UI: =WS MSG= data len: %u\n"), len);

    // ignore packets without "pkg":"post" marker
    std::string_view payload((const char *)data, len);
    if (payload.substr(1, 12) != "\"pkg\":\"post\"")
        return;

    // deserializing data with deep copy to pass to post() for action lookup
    uint16_t objCnt = 0;
    for(uint16_t i=0; i<len; ++i)
        if(data[i]==0x3a || data[i]==0x7b)    // считаем ':' и '{' это учитывает и пары k:v и вложенные массивы
            ++objCnt;

    DynamicJsonDocument *res = new DynamicJsonDocument(len + JSON_OBJECT_SIZE(objCnt)); // https://arduinojson.org/v6/assistant/
    if(!res->capacity())
        return;

    DeserializationError error = deserializeJson((*res), (const char*)data, len); // deserialize via copy to prevent dangling pointers in action()'s
    if (error){
        LOG(printf_P, PSTR("UI: WS_EVT_DATA deserialization err: %d\n"), error.code());
        delete res;
        return;
    }

    // switch context for processing data
    Task *t = new Task(POST_ACTION_DELAY, TASK_ONCE,
        [res](){
            JsonObject o = res->as<JsonObject>();
            // if there is nested data in the object
/*
            if (o[P_data].size()){
                // echo back data to all feeders
                Interface interf(&embui.ws, MEDIUM_JSON_SIZE);
                interf.json_frame_value(o[P_data], true);
                interf.json_frame_flush();
            }
*/
            // call action handler for post'ed data
            embui.post(o);
            delete res; },
        &ts, false, nullptr, nullptr, true
    );
    if (t)
        t->enableDelayed();
}

// EmbUI constructor
EmbUI::EmbUI() : cfg(EMBUI_CFGSIZE), server(80), ws(EMBUI_WEBSOCK_URI){
        _getmacid();

        tAutoSave.set(EMBUI_AUTOSAVE_TIMEOUT * TASK_SECOND, TASK_ONCE, [this](){LOG(println, F("UI: AutoSave")); save();} );    // config autosave timer
        ts.addTask(tAutoSave);
}

EmbUI::~EmbUI(){
    ts.deleteTask(tAutoSave);
    ts.deleteTask(tHouseKeeper);
    delete tValPublisher;
    delete tMqttReconnector;
    delete wifi;
#ifndef EMBUI_NOFTP
    ftp_stop();
#endif
}



void EmbUI::begin(){
    uint8_t retry_cnt = 3;

    // монтируем ФС только один раз при старте
    while(!LittleFS.begin(true)){   // format FS if corrupted
        LOG(println, "UI: LittleFS initialization error, retrying...");
        --retry_cnt;
        delay(100);
        if (!retry_cnt){
            LOG(println, F("FS dirty, I Give up!"));
            return;
        }
    }

    load();                 // load embui's config from json file

    LOG(print, F("UI CONFIG: "));
    LOG_CALL(serializeJson(cfg, EMBUI_DEBUG_PORT));

    // restore Time settings
    if (cfg[V_userntp])
        TimeProcessor::getInstance().setcustomntp(paramVariant(V_userntp).as<const char*>());

    if (cfg[V_timezone]){
        std::string_view tzrule(cfg[V_timezone].as<const char*>());
        TimeProcessor::getInstance().tzsetup(tzrule.substr(4).data());   // cutoff '000_' prefix
    }

    if (paramVariant(V_noNTPoDHCP))
        TimeProcessor::getInstance().ntpodhcp(false);

    // start-up WiFi
    wifi = new WiFiController(this, paramVariant(V_APonly));
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
#ifdef USE_SSDP
    ssdp_begin(); LOG(println, F("Start SSDP"));
#endif
// FTP server
#ifndef EMBUI_NOFTP
    if (paramVariant(P_ftp)) ftp_start();
#endif
}

/**
 * @brief - process posted data for the registered action
 * looks for registered action for the section name and calls the action with post data if found
 */
void EmbUI::post(JsonObject &data, bool inject){
    JsonObject odata = data[P_data].as<JsonObject>();
    Interface interf(&feeders);

    //if (inject && feeders.available()){             // echo back injected data to all availbale feeders
    if (odata.size() && feeders.available()){         // echo back injected data to all availbale feeders
        interf.json_frame_value(odata, true);
        interf.json_frame_flush();
    }

    // reflect post'ed data to MQTT (todo: do this on-demand)
    JsonVariantConst jvc(data);
    publish(C_pub_post, jvc);

    // execute callback actions
    action.exec(&interf, &odata, data[P_action].as<const char*>());
}

void EmbUI::send_pub(){
    if (mqttAvailable()) _mqtt_pub_sys_status();
    if (!ws.count()) return;
    Interface interf(&ws, SMALL_JSON_SIZE);     // only websocket publish!
    basicui::embuistatus(&interf);
    action.exec(&interf, nullptr, A_publish);   // call user-callback for publishing task
}

/**
 * Возвращает указатель на строку со значением параметра из конфига
 * В случае отсутствующего параметра возвращает пустой указатель
 * (метод оставлен для совместимости)
 */
const char* EmbUI::param(const char* key)
{
    LOG(printf_P, PSTR("UI READ KEY: '%s'"), key);

    const char* value = cfg[key] | "";
    if (value){
        LOG(printf_P, PSTR(" value (%s)\n"), value);
    } else {
        LOG(println, F(" key is missing or not a *char\n"));
    }
    return value;
}

/**
 * @brief obtain cfg parameter as String
 * Method tries to cast arbitrary JasonVariant types to string or return "" otherwise
 * @param key - required cfg key
 * @return String
 * TODO: эти методы толком не работают с объектами типа "не строка", нужна нормальная реализация с шаблонами и ДжейсонВариант
 */
String EmbUI::param(const String &key)
{
    LOG(printf_P, PSTR("UI READ KEY: '%s'"), key.c_str());
    String v;
    if (cfg[key].is<int>()){ v += cfg[key].as<int>(); }
    else if (cfg[key].is<float>()) { v += cfg[key].as<float>(); }
    else if (cfg[key].is<bool>())  { v += cfg[key] ? 1 : 0; }
    else { v = cfg[key] | ""; } // откат, все что не специальный тип, то строка (пустая если null)

    LOG(printf_P, PSTR(" VAL: '%s'\n"), v.c_str());
    return v;
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
        save(nullptr, force);
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

    JsonVariantConst h = paramVariant(V_hostname);
    if (h && strlen(h.as<const char*>()))
        return h.as<const char*>();

    if (autohostname.get())
        return autohostname.get();

    autohostname.reset(new char[sizeof(EMBUI_IDPREFIX) + sizeof(mc) * 2]);
    sprintf_P(autohostname.get(), PSTR(EMBUI_IDPREFIX "-%s"), mc);
    LOG(printf_P, PSTR("generate autohostname: %s\n"), autohostname.get());

    return autohostname.get();
}

const char* EmbUI::hostname(const char* name){
    var_dropnulls(V_hostname, (char*)name);
    return hostname();
};

/**
 * формирует chipid из MAC-адреса вида 'ddeeff'
 */
void EmbUI::_getmacid(){
    MacID _mac;
    _mac.u64 = ESP.getEfuseMac();

    sprintf_P(mc, PSTR("%02X%02X%02X%02X%02X%02X"), _mac.mc[0],_mac.mc[1],_mac.mc[2], _mac.mc[3], _mac.mc[4], _mac.mc[5]);
    LOG(printf_P,PSTR("UI ID:%s\n"), mc);
}

void EmbUI::var_remove(const char* key){
    if (cfg.containsKey(key)){
        LOG(printf, "UI cfg REMOVE key:'%s'\n", key);
        cfg.remove(key);
        autosave();
    }
}


void ActionHandler::add(const char* id, actionCallback_t callback){
    actions.emplace_back(id, callback);
    LOG(print, "Action REGISTER: "); LOG(println, id);
}

void ActionHandler::replace(const char* id, actionCallback_t callback){
    auto i = std::find_if(actions.begin(), actions.end(), [&id](const section_handler_t &arg) { return std::string_view(arg.action).compare(id) == 0; } );

    if ( i == actions.end() )
        return add(id, callback);
    
    i->cb = callback;
}

void ActionHandler::remove(const char* id){
    actions.remove_if([&id](const section_handler_t &arg) { return std::string_view(arg.action).compare(id) == 0; });
}

size_t ActionHandler::exec(Interface *interf, JsonObject *data, const char* action){
    size_t cnt{0};
    if (!action) return cnt;      // return if action is empty string
    std::string_view a(action);

    for (const auto& i : actions){
        std::string_view item(i.action);
        if (a.length() < item.length()) continue;  // skip handlers with longer names, obviously a mismatch

        // check if action has a wildcard suffix "*"
        if (std::char_traits<char>::eq(item.back(), 0x2a)){       // 0x2a  == '*'
            if (a.compare(0, item.length()-1, item, 0, item.length()-1) != 0)  continue;
        } else {
            if (a.compare(item) != 0) continue;     // full string compare
        }

        // execute action callback
        LOG(printf, "UI: exec act:%s hndlr:%s\n", action, item.data());
        i.cb(interf, data, action);
        ++cnt;
    }

    return cnt;
}

void ActionHandler::set_mainpage_cb(actionCallback_t callback){
    remove(A_get_ui_page_main);
    add(A_get_ui_page_main, callback);
}

void ActionHandler::set_settings_cb(actionCallback_t callback){
    remove(A_get_ui_blk_usersettings);
    add(A_get_ui_blk_usersettings, callback);
}