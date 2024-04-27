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
    LOGV(P_EmbUI, printf, "=WS MSG= len: %u\n", len);
    // pass data to handler fuction
    if(type == WS_EVT_DATA)
        return wsDataHandler(server, client, type, arg, data, len);

    if(type == WS_EVT_CONNECT){
        LOGD(P_EmbUI, printf, "WS_EVT_CONNECT:%s id:%u\n", server->url(), client->id());
        {
            Interface interf(client);
            if (!embui.action.exec(&interf, nullptr, A_ui_page_main))    // call user defined mainpage callback
                basicui::page_main(&interf, nullptr, NULL);             // if no callback was registered, then show default stub page
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
        LOGE(P_EmbUI, printf, "WS_EVT_DATA deserialization err: %d\n", error.code());
        delete res;
        return;
    }

    // switch context for processing data
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
EmbUI::EmbUI() : cfg(EMBUI_CFGSIZE), server(80), ws(EMBUI_WEBSOCK_URI){
        _getmacid();

        tAutoSave.set(EMBUI_AUTOSAVE_TIMEOUT * TASK_SECOND, TASK_ONCE, [this](){LOGD(P_EmbUI, println, "AutoSave"); save();} );    // config autosave timer
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
    ssdp_begin(); LOG(println, "Start SSDP");
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
void EmbUI::post(const JsonObject &data, bool inject){
    LOGD(P_EmbUI, print, "post() "); //LOG_CALL(serializeJson(data, EMBUI_DEBUG_PORT)); LOG(println);
    JsonObject odata = data[P_data].as<JsonObject>();
    Interface interf(&feeders);

    // echo back injected data to all available feeders IF request 'data' object is not empty
    if (odata.size() && feeders.available()){
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
    LOGV(P_EmbUI, printf, "UI READ KEY: '%s'", key);

    const char* value = cfg[key] | "";
    if (value){
        LOGV(P_EmbUI, printf, " value (%s)\n", value);
    } else {
        LOGV(P_EmbUI, println, " key is missing or not a *char\n");
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
    LOGV(P_EmbUI, printf, "READ key: '%s'", key.c_str());
    String v;
    if (cfg[key].is<int>()){ v += cfg[key].as<int>(); }
    else if (cfg[key].is<float>()) { v += cfg[key].as<float>(); }
    else if (cfg[key].is<bool>())  { v += cfg[key] ? 1 : 0; }
    else { v = cfg[key] | ""; } // откат, все что не специальный тип, то строка (пустая если null)

    LOGV(P_EmbUI, printf, " VAL: '%s'\n", v.c_str());
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
    sprintf(autohostname.get(), PSTR(EMBUI_IDPREFIX "-%s"), mc);
    LOGD(P_EmbUI, printf, "generate autohostname: %s\n", autohostname.get());

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

    sprintf(mc, "%02X%02X%02X%02X%02X%02X", _mac.mc[0],_mac.mc[1],_mac.mc[2], _mac.mc[3], _mac.mc[4], _mac.mc[5]);
    LOGD(P_EmbUI, printf,"UI ID:%s\n", mc);
}

void EmbUI::var_remove(const char* key){
    if (cfg.containsKey(key)){
        LOGD(P_EmbUI, printf, "cfg remove key:'%s'\n", key);
        cfg.remove(key);
        autosave();
    }
}


void ActionHandler::add(const char* id, actionCallback_t callback){
    actions.emplace_back(id, callback);
    LOGD(P_EmbUI, printf, "action register: %s\n", id);
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

void ActionHandler::set_mainpage_cb(actionCallback_t callback){
    remove(A_ui_page_main);
    add(A_ui_page_main, callback);
}

void ActionHandler::set_settings_cb(actionCallback_t callback){
    remove(A_ui_blk_usersettings);
    add(A_ui_blk_usersettings, callback);
}