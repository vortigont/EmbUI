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

/**
 * WebSocket events handler
 *
 */
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){
        LOG(printf_P, PSTR("CONNECT ws:%s id:%u\n"), server->url(), client->id());
        {
            Interface interf(&embui, client);
            if (!embui.action.exec(&interf, nullptr, A_mainpage))   // call user defined mainpage callback
                basicui::page_main(&interf, nullptr, NULL);         // if no callback was registered, then show default stub page
        }
        embui.send_pub();
        return;
    }

    if(type == WS_EVT_DATA){
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if(info->final && info->index == 0 && info->len == len){
            LOG(printf_P, PSTR("UI: =POST= LEN: %u\n"), len);
            // ignore packets without 'post' marker
            if (strncmp_P((const char *)data+1, PSTR("\"pkg\":\"post\""), 12))
                return;

            // deserializing data with deep copy to pass to post-action
            uint16_t objCnt = 0;
            for(uint16_t i=0; i<len; ++i)
                if(data[i]==0x3a || data[i]==0x7b)    // считаем ':' и '{' это учитывает и пары k:v и вложенные массивы
                    ++objCnt;

            DynamicJsonDocument *res = new DynamicJsonDocument(len + JSON_OBJECT_SIZE(objCnt)); // https://arduinojson.org/v6/assistant/
            if(!res->capacity())
                return;

            DeserializationError error = deserializeJson((*res), (const char*)data, len); // deserialize via copy to prevent dangling pointers in action()'s
            if (error){
                LOG(printf_P, PSTR("UI: Post deserialization err: %d\n"), error.code());
                delete res;
                return;
            }
            //res->shrinkToFit();      // this doc should not grow anyway

            if (embui.ws.count()>1 && data){   // if there are multiple ws cliens connected, we must echo back data section, to reflect any changes in UI
                Interface interf(&embui, &embui.ws, TINY_JSON_SIZE);
                JsonVariant d = (*res)[P_data];
                interf.json_frame_value(d, true);
                interf.json_frame_flush();

                if (len > POST_LARGE_SIZE){     // если прилетел большой пост, то откладываем обработку и даем возможность освободить часть памяти
                    Task *t = new Task(POST_ACTION_DELAY, TASK_ONCE,
                        [res](){
                            //JsonObject data = (*res)[F("data")];
                            JsonObject o = res->as<JsonObject>();
                            embui.post(o);
                            delete res; },
                        &ts, false, nullptr, nullptr, true
                    );
                    if (t){
                        t->enableDelayed();
                        return;
                    }
                }
            }

            JsonObject o = res->as<JsonObject>();
            embui.post(o);
            delete res;
        }
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
        LOG(println, F("UI: LittleFS initialization error, retrying..."));
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
    TimeProcessor::getInstance().setcustomntp(paramVariant(P_userntp).as<const char*>());
    TimeProcessor::getInstance().tzsetup(paramVariant(P_TZSET).as<String>().substring(4).c_str());  // cut off 4 chars of html selector index
    if (paramVariant(P_noNTPoDHCP))
        TimeProcessor::getInstance().ntpodhcp(false);

    // start-up WiFi
    wifi = new WiFiController(this, paramVariant(P_APonly));
    wifi->init();
    
    // set WebSocket event handler
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

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
    Interface interf(this, &ws);

    if (inject && ws.count()){            // echo back injected data to WebUI
        interf.json_frame_value();
        interf.value(odata);              // copy values array as-is
        interf.json_frame_flush();
    }

    action.exec(&interf, &odata, data[P_action].as<const char*>());
}

void EmbUI::send_pub(){
    if (mqttAvailable()) _mqtt_pub_sys_status();
    if (!ws.count()) return;
    Interface interf(this, &ws, SMALL_JSON_SIZE);
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

void EmbUI::var_dropnulls(const String &key, const char* value){
    (value && *value) ? var(key, (char*)value, true ) : var_remove(key);    // deep copy
};

void EmbUI::var_dropnulls(const String &key, JsonVariant value){
    if (value.is<const char*>()){
        var_dropnulls(key, value.as<const char*>());
        return;
    }
    value ? var(key, value, true ) : var_remove(key);
};

/**
 * @brief get/set device hosname
 * if hostname has not been set or empty returns autogenerated __IDPREFIX-[mac_id] hostname
 * autogenerated hostname is NOT saved into persistent config
 * 
 * @return const char* current hostname
 */
const char* EmbUI::hostname(){

    JsonVariantConst h = paramVariant(P_hostname);
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
    var_dropnulls(P_hostname, (char*)name);
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

void EmbUI::var_remove(const String &key){
    if (cfg.containsKey(key)){
        LOG(printf_P, PSTR("UI cfg REMOVE key:'%s'\n"), key.c_str());
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

        //.ends_with (since C++20)
        if (std::char_traits<char>::eq(item.back(), 0x2a))       // 0x2a  == '*'
            if (a.compare(0, item.length()-1, item) != 0)  continue;
 
        if (a.compare(item) != 0) continue;

        // execute action callback
        LOG(print, "UI: exec handler: "); LOG(println, item.data());
        i.cb(interf, data, action);
        ++cnt;
    }

    return cnt;
}

void ActionHandler::set_mainpage_cb(actionCallback_t callback){
    remove(A_mainpage);
    add(A_mainpage, callback);
}

void ActionHandler::set_settings_cb(actionCallback_t callback){
    remove(A_block_usr_settings);
    add(A_block_usr_settings, callback);
}