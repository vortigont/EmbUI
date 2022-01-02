// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#ifndef EmbUI_h
#define EmbUI_h

#include "globals.h"

#define EMBUI_VERSION_MAJOR     2
#define EMBUI_VERSION_MINOR     6
#define EMBUI_VERSION_REVISION  999  // '999' here is current dev version

#define EMBUI_VERSION_VALUE     (MAJ, MIN, REV) ((MAJ) << 16 | (MIN) << 8 | (REV))

/* make version as integer for comparison */
#define EMBUI_VERSION           EMBUI_VERSION_VALUE(EMBUI_VERSION_MAJOR, EMBUI_VERSION_MINOR, EMBUI_VERSION_REVISION)

/* make version as string, i.e. "2.6.1" */
#define EMBUI_VERSION_STRING    TOSTRING(EMBUI_VERSION_MAJOR) "." TOSTRING(EMBUI_VERSION_MINOR) "." TOSTRING(EMBUI_VERSION_REVISION)
// compat definiton
#define EMBUIVER                EMBUI_VERSION_STRING

#include <FS.h>

#ifdef ESP8266
// #include <ESPAsyncTCP.h>
 #include <LittleFS.h>
 #define FORMAT_LITTLEFS_IF_FAILED
 #include <Updater.h>
#endif

#ifdef ESP32
// #include <AsyncTCP.h>
 #ifdef ESP_ARDUINO_VERSION
  #include <LittleFS.h>
 #else
  #include <LITTLEFS.h>
  #define LittleFS LITTLEFS
 #endif

 #ifndef FORMAT_LITTLEFS_IF_FAILED
  #define FORMAT_LITTLEFS_IF_FAILED true
 #endif
 #define U_FS   U_SPIFFS
 #include <Update.h>
#endif


#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include <AsyncMqttClient.h>
#include "LList.h"
#include "ts.h"
#include "timeProcessor.h"

#define UDP_PORT                4243    // UDP server port

#ifndef PUB_PERIOD
#define PUB_PERIOD              10      // Values Publication period, s
#endif

#define MQTT_PUB_PERIOD         30

#ifndef DELAY_AFTER_FS_WRITING
#define DELAY_AFTER_FS_WRITING  (50U)   // 50мс, меньшие значения могут повлиять на стабильность
#endif

#define AUTOSAVE_TIMEOUT        2       // configuration autosave timer, sec    (4 bit value, multiplied by AUTOSAVE_MULTIPLIER)

#ifndef AUTOSAVE_MULTIPLIER
#define AUTOSAVE_MULTIPLIER     (10U)   // множитель таймера автосохранения конфиг файла
#endif

#ifndef __DISABLE_BUTTON0
#define __BUTTON 0 // Кнопка "FLASH" на NODE_MCU
#endif

// Default Hostname/AP prefix
#ifndef __IDPREFIX
#define __IDPREFIX "EmbUI"
#endif

// size of a JsonDocument to hold EmbUI config 
#ifndef __CFGSIZE
#define __CFGSIZE (2048)
#endif

#ifndef MAX_WS_CLIENTS
#define MAX_WS_CLIENTS          4
#endif

#define WEBSOCK_URI             "/ws"

// TaskScheduler - Let the runner object be a global, single instance shared between object files.
extern Scheduler ts;

class Interface;

//-----------------------
#define TOGLE_STATE(val, curr) (val == "1")? true : (val == "0")? false : !curr;

#define SETPARAM(key, call...) { \
    embui.var(key, (*data)[key]); \
    call; \
}

// Saves non-null keys, otherwise - removes key
#define SETPARAM_NONULL(key, call...) { \
    embui.var_dropnulls(key, (JsonVariant)(*data)[key]); \
    call; \
}

#define CALL_SETTER(key, val, call) { \
    obj[key] = val; \
    call(nullptr, &obj); \
    obj.clear(); \
}

#define CALL_INTF(key, val, call) { \
    obj[key] = val; \
    Interface *interf = embui.ws.count()? new Interface(&embui, &embui.ws, SMALL_JSON_SIZE) : nullptr; \
    call(interf, &obj); \
    if (interf) { \
        interf->json_frame_value(); \
        interf->value(key, val, false); \
        interf->json_frame_flush(); \
        delete interf; \
    } \
}

#define CALL_INTF_OBJ(call) { \
    Interface *interf = embui.ws.count()? new Interface(&embui, &embui.ws, SMALL_JSON_SIZE*1.5) : nullptr; \
    call(interf, &obj); \
    if (interf) { \
        interf->json_frame_value(); \
        for (JsonPair kv : obj) { \
            interf->value(kv.key().c_str(), kv.value(), false); \
        } \
        interf->json_frame_flush(); \
        delete interf; \
    } \
}

// Weak Callback functions (user code might override it)
void    __attribute__((weak)) section_main_frame(Interface *interf, JsonObject *data);
void    __attribute__((weak)) pubCallback(Interface *interf);
String  __attribute__((weak)) httpCallback(const String &param, const String &value, bool isset);
uint8_t __attribute__((weak)) uploadProgress(size_t len, size_t total);
void    __attribute__((weak)) create_parameters();

//----------------------

#ifdef USE_SSDP
  #ifndef EXTERNAL_SSDP
    #define __SSDPNAME      ("EmbUI")
    #define __SSDPURLMODEL  ("https://github.com/vortigont/")
    #define __SSDPMODEL     EMBUI_VERSION_STRING
    #define __SSDPURLMANUF  ("https://github.com/anton-zolotarev")
    #define __SSDPMANUF     ("obliterator")
  #endif

  static const char PGnameModel[] PROGMEM = TOSTRING(__SSDPNAME);
  static const char PGurlModel[] PROGMEM = TOSTRING(__SSDPURLMODEL);
  static const char PGversion[] PROGMEM = EMBUI_VERSION_STRING;
  static const char PGurlManuf[] PROGMEM = TOSTRING(__SSDPURLMANUF);
  static const char PGnameManuf[] PROGMEM = TOSTRING(__SSDPMANUF);
#endif

// Callback enums
enum CallBack : uint8_t {
    detach = (0U),
    attach = (1U),
    STAConnected,
    STADisconnected,
    STAGotIP,
    TimeSet
};

typedef void (*actionCallback) (Interface *interf, JsonObject *data);

typedef struct section_handle_t{
    String name;
    actionCallback callback;
} section_handle_t;


class EmbUI
{
    friend void mqtt_dummy_connect();

    typedef void (*mqttCallback) ();

    // оптимизация расхода памяти, все битовые флаги и другие потенциально "сжимаемые" переменные скидываем сюда
    //#pragma pack(push,1)
    typedef union _BITFIELDS {
    struct {
        bool wifi_sta:1;    // флаг успешного подключения к внешней WiFi-AP, (TODO: переделать на события с коллбеками)
        bool mqtt_connected:1;
        bool mqtt_connect:1;
        bool mqtt_remotecontrol:1;

        bool mqtt_enable:1;
        bool cfgCorrupt:1;
        bool LED_INVERT:1;
        uint8_t LED_PIN:5;   // [0...30]
        uint8_t asave:4;     // 4 бита значения таймера автосохранения конфига (домножается на AUTOSAVE_MULTIPLIER)
    };
    uint32_t flags; // набор битов для конфига
    _BITFIELDS() {
        wifi_sta = false;    // флаг успешного подключения к внешней WiFi-AP, (TODO: переделать на события с коллбеками)
        mqtt_connected = false;
        mqtt_connect = false;
        mqtt_remotecontrol = false;
        mqtt_enable = false;
        LED_INVERT = false;
        cfgCorrupt = false;     // todo: убрать из конфига
        LED_PIN = 31; // [0...30]
        asave = AUTOSAVE_TIMEOUT; // defaul timeout 2*10 sec
    }
    } BITFIELDS;
    //#pragma pack(pop)

    bool fsDirty = false;       // флаг поврежденной FS (ошибка монтирования)

    DynamicJsonDocument cfg;    // system config
    LList<section_handle_t*> section_handle;        // action handlers
    AsyncMqttClient mqttClient;

  public:
    EmbUI();

    ~EmbUI(){
        ts.deleteTask(tAutoSave);
        ts.deleteTask(*tValPublisher);
        ts.deleteTask(tHouseKeeper);
    }

    BITFIELDS sysData;
    AsyncWebServer server;
    AsyncWebSocket ws;
    mqttCallback onConnect;
    TimeProcessor& timeProcessor = TimeProcessor::getInstance();

    std::unique_ptr<char[]> autohostname;   // pointer for autogenerated hostname
    char mc[4]; // last 3 LSB's of mac addr used as an ID

    void section_handle_add(const String &btn, actionCallback response);
    void section_handle_remove(const String &name);


    /**
     * @brief obtain cfg parameter as String
     * Method tries to cast arbitrary JasonVariant types to string or return "" otherwise
     * @param key - required cfg key
     * @return String
     */
    String param(const String &key);
    const char* param(const char* key);

    /**
     * @brief - return JsonVariant of a config param
     * this method allows accessing cfg param JsonVariantConst and use member functions, like .as<T>
     * unlike param(), this method does not Stringify config values
     */
    JsonVariantConst paramVariant(const String &key){ return cfg[key]; }


    bool isparamexists(const char* key){ return cfg.containsKey(key);}
    bool isparamexists(const String &key){ return cfg.containsKey(key);}

    void led(uint8_t pin, bool invert);

    /**
     * @brief EmbUI initialization
     * load configuration from FS, setup WiFi, obtain system date/time, etc...
     * 
     */
    void begin();

    /**
     * @brief EmbUI process handler
     * must be placed into loop()
     * 
     */
    void handle();

    // config operations
    void save(const char *_cfg = nullptr, bool force = false);
    void load(const char *cfgfile = nullptr);   // if null, than default cfg file is used
    void cfgclear();                            // clear current config, both in RAM and file

    //  * tries to load json file from FS and deserialize it into provided DynamicJsonDocument, returns false on error
    bool loadjson(const char *filepath, DynamicJsonDocument &obj);

#ifdef EMBUI_UDP
    void udp(const String &message);
    void udp();
#endif // EMBUI_UDP

    // MQTT
    bool isMQTTconected() { return sysData.mqtt_connected; }
    void pub_mqtt(const String &key, const String &value);
    void mqtt_handle();
    void subscribeAll(bool isOnlyGetSet=true);
    void mqtt_reconnect();
    void mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), bool remotecontrol);
    void mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload));
    void mqtt(const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload));
    void mqtt(const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), bool remotecontrol);
    void mqtt(const String &host, int port, const String &user, const String &pass, bool remotecontrol);
    void mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, bool remotecontrol);
    void mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) (), bool remotecontrol);
    void mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) ());
    void mqtt(const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) ());
    void mqtt(const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) (), bool remotecontrol);
    void mqtt(void (*mqttFunction) (const String &topic, const String &payload), bool remotecontrol);
    void mqtt(void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) (), bool remotecontrol);
    void mqttReconnect();
    void subscribe(const String &topic);
    void publish(const String &topic, const String &payload);
    void publish(const String &topic, const String &payload, bool retained);
    void publishto(const String &topic, const String &payload, bool retained);
    void remControl();
    String id(const String &tpoic);

    /**
     * @brief - process posted data for the registered action
     * if post came from the WebUI echoes received data back to the WebUI,
     * if post came from some other place - sends data to the WebUI
     * looks for registered action for the section name and calls the action with post data if found
     */
    void post(JsonObject &data);

    // WiFi-related
    /**
     * Initialize WiFi using stored configuration
     */
    void wifi_init();

    /**
     * Подключение к WiFi AP в клиентском режиме
     */
    void wifi_connect(const char *ssid=nullptr, const char *pwd=nullptr);

    /**
      * update WiFi AP params and state
      */
    void wifi_updateAP();

    /**
     * @brief get/set device hosname
     * if hostname has not been set or empty returns autogenerated __IDPREFIX-[mac_id] hostname
     * autogenerated hostname is NOT saved into persistent config
     *
     * @return const char* current hostname
     */
    const char* hostname();
    const char* hostname(const char* name);

    /**
     * метод для установки коллбеков на системные события, типа:
     * - WiFi подключиля/отключился
     * - получено время от NTP
     */
    void set_callback(CallBack set, CallBack action, callback_function_t callback=nullptr);

    /**
     * @brief - set interval period for send_pub() task in seconds
     * 0 - will disable periodic task
     */
    void setPubInterval(uint16_t _t);

    /**
     * Publish status data to the WebUI
     */
    void send_pub();

    /**
     * @brief - set variable's value in the system config object
     * @param key - variable's key
     * @param value - value to set
     * @param force - register new key in config if it does not exist
     * Note: by default if key has not been registerred on init it won't be created
     * beware of dangling pointers here passing non-static char*, use JsonVariant or String instead 
     */
    template <typename T> void var(const String &key, const T& value, bool force = false){
        if (!force && cfg[key].isNull()) {
            LOG(printf_P, PSTR("UI ERR: KEY (%s) is NOT initialized!\n"), key.c_str());
            return;
        }

        // do not update key if new value is the same as existing one
        if (cfg[key] == value){
            LOG(printf_P, PSTR("UI: skip same value for KEY:'%s'\n"), key.c_str());
            return;
        }

        if ((cfg.capacity() - cfg.memoryUsage()) < 42){ // you know that 42 is THE answer, right?
            // cfg is out of mem, try to compact it
            cfg.garbageCollect();
            LOG(printf_P, PSTR("UI: cfg garbage cleanup: %u free out of %u\n"), cfg.capacity() - cfg.memoryUsage(), cfg.capacity());
        }

        if (cfg[key].set(value)){
            LOG(printf_P, PSTR("UI cfg WRITE key:'%s' val:'%s...', cfg mem free: %d\n"), key.c_str(), cfg[key].as<String>().substring(0, 5).c_str(), cfg.capacity() - cfg.memoryUsage());
            autosave();
            return;
        }

        LOG(printf_P, PSTR("UI ERR: KEY (%s), cfg out of mem!\n"), key.c_str());
    }

    /**
     * @brief - create varialbe template
     * it accepts types suitable to be added to the ArduinoJson cfg document used as a dictionary
     */
    template <typename T>
    inline void var_create(const String &key, const T& value){ if(cfg[key].isNull()){var(key, value, true );} }

    /**
     * @brief - remove key from config
     */
    void var_remove(const String &key){
        if (!cfg[key].isNull()){
            LOG(printf_P, PSTR("UI cfg REMOVE key:'%s'\n"), key.c_str());
            cfg.remove(key);
            autosave();
        }
    }

    /**
     * @brief create/update cfg key only with non-null value
     If value casted to <bool> returns 'true' than provided config key is created/updated,
     otherwise key is to be removed from config (if exist). This could be used to reduce doc size and
     does not keep any keys with default "null-ish" values, like:
        bools == false
        int == 0
        *char == ""
     Note: removing existing key leaks dict memory, should NOT be used for frequently modified keys

     *
     * @tparam T ArduinoJson acceptable types
     * @param key - config key
     * @param value - keys' value
     */
    template <typename T>
    void var_dropnulls(const String &key, const T& value){
/*
    // C++17, cant't build for esp32
        if constexpr (std::is_same_v<T, const char*>){  // check if pointed str is not empty ""
            (value && *value) ? var(key, (char*)value, true ) : var_remove(key);    // deep copy
            return;
        }
        if constexpr (std::is_same_v<T, JsonVariant>){  // JVars that points to strings must be treaded differently
            JsonVariant _v = value;
            _v.is<const char*>() ? var_dropnulls(key, _v.as<const char*>()) : var_remove(key);
            return;
        }
*/
        value ? var(key, value, true ) : var_remove(key);
    }

    void var_dropnulls(const String &key, const char* value);
    void var_dropnulls(const String &key, JsonVariant value);

    /**
     * @brief - initialize/restart config autosave timer
     * each call postpones cfg write to flash
     */
    void autosave(bool force = false);

    /**
     * Recycler for dynamic tasks
     */
    void taskRecycle(Task *t);

  private:
    /**
     * call to create system-dependent variables,
     * both run-time and persistent
     */ 
    void create_sysvars();
    void led_on();
    void led_off();
    void led_inv();

    void btn();
    void getmacid();

    // Scheduler tasks
    Task embuischedw;       // WiFi reconnection helper
    Task *tValPublisher;     // Status data publisher
    Task tHouseKeeper;     // Maintenance task, runs every second
    Task tAutoSave;          // config autosave timer
    std::vector<Task*> *taskTrash = nullptr;    // ptr to a vector container with obsolete tasks

    // WiFi-related
    /**
      * устанавлием режим WiFi
      */
    void wifi_setmode(WiFiMode_t mode);

    /**
     * Configure and bring up esp's internal AP
     * defualt is to configure credentials from the config
     * bool force - reapply credentials even if AP is already started, exit otherwise
     */
    void wifi_setAP(bool force=false);



#ifdef ESP8266
    WiFiEventHandler e1, e2, e3, e4;
    WiFiMode wifi_mode;           // используется в gpio led_handle (to be removed)
    void onSTAConnected(WiFiEventStationModeConnected ipInfo);
    void onSTAGotIP(WiFiEventStationModeGotIP ipInfo);
    void onSTADisconnected(WiFiEventStationModeDisconnected event_info);
    void onWiFiMode(WiFiEventModeChange event_info);
#endif

#ifdef ESP32
    void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
#endif

    // HTTP handlers

    /**
     * @brief Set HTTP-handlers for EmbUI related URL's
     * called at framework initialization
     */
    void http_set_handlers();


    // MQTT Private Methods and vars
    String m_pref; // к сожалению они нужны, т.к. в клиент передаются указатели на уже имеющийся объект, значит на конфиг ссылку отдавать нельзя!!!
    String m_host;
    String m_port;
    String m_user;
    String m_pass;
    String m_will;
    void connectToMqtt();
    void onMqttConnect();
    static void _onMqttConnect(bool sessionPresent);
    static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    static void onMqttSubscribe(uint16_t packetId, uint8_t qos);
    static void onMqttUnsubscribe(uint16_t packetId);
    static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    static void onMqttPublish(uint16_t packetId);

#ifdef EMBUI_UDP
    unsigned int localUdpPort = UDP_PORT;
    String udpMessage; // буфер для сообщений Обмена по UDP
    void udpBegin();
    void udpLoop();
#endif // EMBUI_UDP

    // callback pointers
    callback_function_t _cb_STAConnected = nullptr;
    callback_function_t _cb_STADisconnected = nullptr;
    callback_function_t _cb_STAGotIP = nullptr;

    void setup_mDns();

    // Dyn tasks garbage collector
    void taskGC();

    // find callback section matching specified name
    section_handle_t* sectionlookup(const char *id);

#ifdef USE_SSDP
    void ssdp_begin() {
          String hn = hostname();

          uint32_t chipId;
          #ifdef ESP32
              chipId = ESP.getEfuseMac();
          #else
              chipId = ESP.getChipId();    
          #endif  
          SSDP.setDeviceType(F("upnp:rootdevice"));
          SSDP.setSchemaURL(F("description.xml"));
          SSDP.setHTTPPort(80);
          SSDP.setName(hostname());
          SSDP.setSerialNumber(String(chipId));
          SSDP.setURL(F("/"));
          SSDP.setModelName(FPSTR(PGnameModel));
          SSDP.setModelNumber(FPSTR(PGversion));
          SSDP.setModelURL(String(F("http://"))+(WiFi.status() != WL_CONNECTED ? WiFi.softAPIP().toString() : WiFi.localIP().toString()));
          SSDP.setManufacturer(FPSTR(PGnameManuf));
          SSDP.setManufacturerURL(FPSTR(PGurlManuf));
          SSDP.begin();

          (&server)->on(PSTR("/description.xml"), HTTP_GET, [&](AsyncWebServerRequest *request){
            request->send(200, FPSTR(PGmimexml), getSSDPSchema());
          });
    }
    
    String getSSDPSchema() {
        uint32_t chipId;
        #ifdef ESP32
            chipId = ESP.getEfuseMac();
        #else
            chipId = ESP.getChipId();    
        #endif  
      String s = "";
        s +=F("<?xml version=\"1.0\"?>\n");
        s +=F("<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\n");
        s +=F("\t<specVersion>\n");
        s +=F("\t\t<major>1</major>\n");
        s +=F("\t\t<minor>0</minor>\n");
        s +=F("\t</specVersion>\n");
        s +=F("<URLBase>");
        s +=String(F("http://"))+(WiFi.status() != WL_CONNECTED ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
        s +=F("</URLBase>");
        s +=F("<device>\n");
        s +=F("\t<deviceType>upnp:rootdevice</deviceType>\n");
        s +=F("\t<friendlyName>");
        s += param(F("hostname"));
        s +=F("</friendlyName>\r\n");
        s +=F("\t<presentationURL>index.html</presentationURL>\r\n");
        s +=F("\t<serialNumber>");
        s += String(chipId);
        s +=F("</serialNumber>\r\n");
        s +=F("\t<modelName>");
        s += FPSTR(PGnameModel);
        s +=F("</modelName>\r\n");
        s +=F("\t<modelNumber>");
        s += FPSTR(PGversion);
        s +=F("</modelNumber>\r\n");
        s +=F("\t<modelURL>");
        s += FPSTR(PGurlModel);
        s +=F("</modelURL>\r\n");
        s +=F("\t<manufacturer>");
        s += FPSTR(PGnameManuf);
        s +=F("</manufacturer>\r\n");
        s +=F("\t<manufacturerURL>");
        s += FPSTR(PGurlManuf);
        s +=F("</manufacturerURL>\r\n");
        //s +=F("\t<UDN>0543bd4e-53c2-4f33-8a25-1f75583a19a2");
        s +=F("\t<UDN>0543bd4e-53c2-4f33-8a25-1f7558");
        char cn[7];
        sprintf_P(cn, PSTR("%02x%02x%02x"), ((chipId >> 16) & 0xff), ((chipId >>  8) & 0xff), chipId & 0xff);
        s += cn;
        s +=F("</UDN>\r\n");
        s +=F("\t</device>\n");
        s +=F("</root>\r\n\r\n");
      return s;
    }
#endif
};

// Глобальный объект фреймворка
extern EmbUI embui;
#include "ui.h"
#endif