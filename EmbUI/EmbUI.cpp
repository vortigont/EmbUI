// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "EmbUI.h"
#include "ui.h"
#include "ftpsrv.h"

#define POST_ACTION_DELAY   50      // delay for large posts processing
#define POST_LARGE_SIZE   256       // large post threshold


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

        Interface *interf = new Interface(&embui, client);
        section_main_frame(interf, nullptr);
        embui.send_pub();
        delete interf;
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
            res->shrinkToFit();      // this doc should not grow anyway

            if (embui.ws.count()>1 && data){   // if there are multiple ws cliens connected, we must echo back data section, to reflect any changes in UI
                JsonObject _d = (*res)[F("data")];
                Interface *interf = new Interface(&embui, &embui.ws, _d.memoryUsage()+128);      // about 128 bytes overhead requred for section structs
                interf->json_frame_value();
                interf->value(_d);              // copy values array as-is
                interf->json_frame_flush();
                delete interf;

                if (len > POST_LARGE_SIZE){     // если прилетел большой пост, то откладываем обработку и даем возможность освободить часть памяти
                    Task *t = new Task(POST_ACTION_DELAY, TASK_ONCE,
                        [res](){
                            JsonObject data = (*res)[F("data")];
                            embui.post(data);
                            delete res; },
                        &ts, false, nullptr, nullptr, true
                    );
                    if (t){
                        t->enableDelayed();
                        return;
                    }
                }
            }

            JsonObject data = (*res)[F("data")];
            embui.post(data);
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
EmbUI::EmbUI() : cfg(__CFGSIZE), server(80), ws(F(WEBSOCK_URI)){

    // Enable persistent storage for ESP8266 Core >3.0.0 (https://github.com/esp8266/Arduino/pull/7902)
    #ifdef WIFI_IS_OFF_AT_BOOT
        enableWiFiAtBootTime(); // can be called from anywhere with the same effect
    #endif

        memset(mc,0,sizeof(mc));
        getmacid();

        ts.addTask(embuischedw);    // WiFi helper
        tAutoSave.set(sysData.asave * AUTOSAVE_MULTIPLIER * TASK_SECOND, TASK_ONCE, [this](){LOG(println, F("UI: AutoSave")); save();} );    // config autosave timer
        ts.addTask(tAutoSave);
}

EmbUI::~EmbUI(){
    ts.deleteTask(tAutoSave);
    ts.deleteTask(tHouseKeeper);
    delete tValPublisher;
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
            fsDirty = true;
            return;
        }
    }

    load();                 // try to load config from file
    create_sysvars();       // create system variables (if missing)
    create_parameters();    // weak function, creates user-defined variables
#ifdef EMBUI_MQTT
    mqtt(param(FPSTR(P_m_pref)), param(FPSTR(P_m_host)), param(FPSTR(P_m_port)).toInt(), param(FPSTR(P_m_user)), param(FPSTR(P_m_pass)), mqtt_emptyFunction, false); // init mqtt
#endif

    LOG(print, F("UI CONFIG: "));
    LOG_CALL(serializeJson(cfg, EMBUI_DEBUG_PORT));

    // Set WiFi event handlers
    #ifdef ESP8266
        e1 = WiFi.onStationModeGotIP(std::bind(&EmbUI::onSTAGotIP, this, std::placeholders::_1));
        e2 = WiFi.onStationModeDisconnected(std::bind(&EmbUI::onSTADisconnected, this, std::placeholders::_1));
        e3 = WiFi.onStationModeConnected(std::bind(&EmbUI::onSTAConnected, this, std::placeholders::_1));
        e4 = WiFi.onWiFiModeChange(std::bind(&EmbUI::onWiFiMode, this, std::placeholders::_1));
    #elif defined ESP32
        WiFi.onEvent(std::bind(&EmbUI::WiFiEvent, this, std::placeholders::_1, std::placeholders::_2));
    #endif

    // восстанавливаем настройки времени
    timeProcessor.setcustomntp(paramVariant(FPSTR(P_userntp)).as<const char*>());
    timeProcessor.tzsetup(param(FPSTR(P_TZSET)).substring(4).c_str());  // cut off 4 chars of html selector index
    if (paramVariant(FPSTR(P_noNTPoDHCP)))
        timeProcessor.ntpodhcp(false);

    // запускаем WiFi
    wifi_init();
    
    ws.onEvent(onWsEvent);      // WebSocket event handler
    server.addHandler(&ws);

    http_set_handlers();        // install various http handlers
    server.begin();

#ifdef USE_SSDP
    ssdp_begin(); LOG(println, F("Start SSDP"));
#endif

    setPubInterval(PUB_PERIOD);

    tHouseKeeper.set(TASK_SECOND, TASK_FOREVER, [this](){
            ws.cleanupClients(MAX_WS_CLIENTS);
            #ifdef ESP8266
                MDNS.update();
            #endif
        } );
    ts.addTask(tHouseKeeper);
    tHouseKeeper.enableDelayed();
// FTP server
#ifndef EMBUI_NOFTP
    if (paramVariant(FPSTR(P_ftp))) ftp_start();
#endif
}

/**
 * @brief - process posted data for the registered action
 * looks for registered action for the section name and calls the action with post data if found
 */
void EmbUI::post(JsonObject &data){
    section_handle_t *section = nullptr;

    const char *submit = data[FPSTR(P_submit)];

    if ( submit ){  // if it was a form post, than only 'submit' key is checked for matching section, not all data keys
        section = sectionlookup(submit);
    } else {        // otherwise scan all possible keys
        for (JsonPair kv : data)
            section = sectionlookup(kv.key().c_str());
    }

    if (section) {
        LOG(printf_P, PSTR("UI: POST SECTION: %s\n"), section->name.c_str());
        Interface *interf = new Interface(this, &ws);
        section->callback(interf, &data);
        delete interf;
    }
}

void EmbUI::send_pub(){
    if (!ws.count()) return;
    Interface *interf = new Interface(this, &ws, SMALL_JSON_SIZE);
    pubCallback(interf);
    delete interf;
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


void EmbUI::led(uint8_t pin, bool invert){
    if (pin == 31) return;
    sysData.LED_PIN = pin;
    sysData.LED_INVERT = invert;
    pinMode(sysData.LED_PIN, OUTPUT);
}

void EmbUI::handle(){
    ts.execute();           // run task scheduler
#ifdef EMBUI_MQTT
    mqtt_handle();
#endif // EMBUI_MQTT
    //btn();
    //led_handle();

#ifdef EMBUI_UDP
    void udpLoop();
#endif // EMBUI_UDP
// FTP server
#ifndef EMBUI_NOFTP
    ftp_loop();
#endif
}

/**
 * метод для установки коллбеков на системные события, типа:
 * - WiFi подключиля/отключился
 * - получено время от NTP
 */
void EmbUI::set_callback(CallBack set, CallBack action, callback_function_t callback){

    switch (action){
        case CallBack::STAConnected :
            set ? _cb_STAConnected = std::move(callback) : _cb_STAConnected = nullptr;
            break;
        case CallBack::STADisconnected :
            set ? _cb_STADisconnected = std::move(callback) : _cb_STADisconnected = nullptr;
            break;
        case CallBack::STAGotIP :
            set ? _cb_STAGotIP = std::move(callback) : _cb_STAGotIP = nullptr;
            break;
        case CallBack::TimeSet :
            set ? timeProcessor.attach_callback(callback) : timeProcessor.dettach_callback();
            break;
        default:
            return;
    }
};

/**
 * call to create system-dependent variables,
 * both run-time and persistent
 */
void EmbUI::create_sysvars(){
    LOG(println, F("UI: Creating system vars"));
    // параметры подключения к MQTT
    var_create(FPSTR(P_m_host), "");                   // MQTT server hostname
    var_create(FPSTR(P_m_port), "");                   // MQTT port
    var_create(FPSTR(P_m_user), "");                   // MQTT login
    var_create(FPSTR(P_m_pass), "");                   // MQTT pass
    var_create(FPSTR(P_m_pref), embui.mc);             // MQTT topic == use ESP MAC address
    var_create(FPSTR(P_m_tupd), TOSTRING(MQTT_PUB_PERIOD));              // интервал отправки данных по MQTT в секундах
    // date/time related vars
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
