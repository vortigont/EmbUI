// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "EmbUI.h"
#include "ui.h"
#include "ftpsrv.h"

#define POST_ACTION_DELAY   50      // delay for large posts processing in ms
#define POST_LARGE_SIZE     1024    // large post threshold

union MacID
{
    uint64_t u64;
    uint8_t mc[8];
};

#ifdef EMBUI_MQTT
void mqtt_emptyFunction(const String &, const String &);
#endif // EMBUI_MQTT

EmbUI embui;

/**
 * Those functions are weak, and by default do nothing
 * it is up to user to redefine it for proper WS event handling
 */
void section_main_frame(Interface *interf, JsonObject *data) {}
void pubCallback(Interface *interf){}

/**
 * WebSocket events handler
 *
 */
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){
        LOG(printf_P, PSTR("UI: ws[%s][%u] connect MEM: %u\n"), server->url(), client->id(), ESP.getFreeHeap());

        { Interface interf(&embui, client);
        section_main_frame(&interf, nullptr); }
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
EmbUI::EmbUI() : cfg(EMBUI_CFGSIZE), server(80), ws(F(EMBUI_WEBSOCK_URI)){
        _getmacid();

        tAutoSave.set(sysData.asave * EMBUI_AUTOSAVE_MULTIPLIER * TASK_SECOND, TASK_ONCE, [this](){LOG(println, F("UI: AutoSave")); save();} );    // config autosave timer
        ts.addTask(tAutoSave);
}

EmbUI::~EmbUI(){
    ts.deleteTask(tAutoSave);
    ts.deleteTask(tHouseKeeper);
    delete tValPublisher;
    delete wifi;
#ifndef EMBUI_NOFTP
    ftp_stop();
#endif
}



void EmbUI::begin(){
    uint8_t retry_cnt = 3;

    // монтируем ФС только один раз при старте
    while(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        LOG(println, F("UI: LittleFS initialization error, retrying..."));
        --retry_cnt;
        delay(100);
        if (!retry_cnt){
            LOG(println, F("FS dirty, I Give up!"));
            sysData.fsDirty = true;
            return;
        }
    }

    load();                 // try to load config from file
    //create_sysvars();       // create system variables, if missing (this one is empty now)
    create_parameters();    // weak function, creates user-defined variables
    LOG(print, F("UI CONFIG: "));
    LOG_CALL(serializeJson(cfg, EMBUI_DEBUG_PORT));

    // restore Time settings
    TimeProcessor::getInstance().setcustomntp(paramVariant(FPSTR(P_userntp)).as<const char*>());
    TimeProcessor::getInstance().tzsetup(paramVariant(FPSTR(P_TZSET)).as<String>().substring(4).c_str());  // cut off 4 chars of html selector index
    if (paramVariant(FPSTR(P_noNTPoDHCP)))
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

    setPubInterval(EMBUI_PUB_PERIOD);

    tHouseKeeper.set(TASK_SECOND, TASK_FOREVER, [this](){
            ws.cleanupClients(EMBUI_MAX_WS_CLIENTS);
        } );
    ts.addTask(tHouseKeeper);
    tHouseKeeper.enableDelayed();

#ifdef EMBUI_MQTT
    // try to connect to mqtt if mqtt hostname is defined
    if (param(FPSTR(P_m_host)).length())
        mqtt(param(FPSTR(P_m_pref)), param(FPSTR(P_m_host)), paramVariant(FPSTR(P_m_port)), param(FPSTR(P_m_user)), param(FPSTR(P_m_pass)), mqtt_emptyFunction, false); // init mqtt
#endif
#ifdef USE_SSDP
    ssdp_begin(); LOG(println, F("Start SSDP"));
#endif
// FTP server
#ifndef EMBUI_NOFTP
    if (paramVariant(FPSTR(P_ftp))) ftp_start();
#endif
}

/**
 * @brief - process posted data for the registered action
 * looks for registered action for the section name and calls the action with post data if found
 */
void EmbUI::post(JsonObject &data, bool inject){
    section_handle_t *section = nullptr;

    const char *submit = data[P_action];

    if ( submit ){  // if it was a form post, than only 'action' key is checked for matching section, not all data keys
        section = sectionlookup(submit);
    } else {        // otherwise scan all possible keys in data object (deprecated, kept for compatibility only)
        JsonObject odata = data[P_data].as<JsonObject>();
        for (JsonPair kv : odata){
            section = sectionlookup(kv.key().c_str());
            if (section) break;
        }
    }

    if (section || inject) {
        Interface interf(this, &ws);
        JsonObject odata = data[P_data].as<JsonObject>();
        if (inject && ws.count()){            // echo back injected data to WebUI
            interf.json_frame_value();
            interf.value(odata);              // copy values array as-is
            interf.json_frame_flush();
        }

        if(section){                           // execute an action on registered data
            LOG(printf_P, PSTR("UI: POST SECTION: %s\n"), section->name.c_str());
            section->callback(&interf, &odata);
        }
    }
}

void EmbUI::send_pub(){
    if (!ws.count()) return;
    Interface interf(this, &ws, SMALL_JSON_SIZE);
    pubCallback(&interf);
}

void EmbUI::section_handle_remove(const String &name)
{
    for(unsigned i=0; i<section_handle.size(); i++){
        if(section_handle.get(i)->name==name){
            section_handle.unlink(i);
            LOG(printf_P, PSTR("UI UNREGISTER: %s\n"), name.c_str());
            break;
        }
    }
}

void EmbUI::section_handle_add(const String &name, actionCallback response)
{
    std::shared_ptr<section_handle_t> section(new section_handle_t(name, response));
    section_handle.add(section);

    LOG(printf_P, PSTR("UI REGISTER: %s\n"), name.c_str());
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
#ifdef EMBUI_MQTT
    mqtt_handle();
#endif // EMBUI_MQTT
// FTP server
#ifndef EMBUI_NOFTP
    ftp_loop();
#endif
}

/**
 * call to create system-dependent variables,
 * both run-time and persistent
 */
void EmbUI::create_sysvars(){
    LOG(println, F("UI: Creating EmbUI system vars"));
    // this one is empty since all EmbUI vars could use defaults
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

    if(tValPublisher){
        tValPublisher->setInterval(_t * TASK_SECOND);
    } else {
        tValPublisher = new Task(_t * TASK_SECOND, TASK_FOREVER, [this](){ send_pub(); }, &ts, true );
    }
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

// find callback section matching specified name
section_handle_t*  EmbUI::sectionlookup(const char *id){
    unsigned _l = strlen(id);
    for (const auto& i : section_handle){
        if (_l < i->name.length())  // skip sections with longer names, obviously a mismatch
            continue;

        const char *sname = i->name.c_str();
        const char *mall = strchr(sname, '*');      // look for 'id*' template sections
        unsigned len = mall? mall - sname - 1 : _l;
        if (strncmp(sname, id, len) == 0) {
            return i.get();
        }
    };
    return nullptr;
}

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

    JsonVariantConst h = paramVariant(FPSTR(P_hostname));
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
    var_dropnulls(FPSTR(P_hostname), (char*)name);
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
