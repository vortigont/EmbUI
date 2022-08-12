// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#ifdef ESP8266
#error "Sorry, esp8266 is no longer supported"
#error "use v2.6 branch for 8266 https://github.com/vortigont/EmbUI/tree/v2.6"
#include "no_esp8266"
#endif

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

#ifdef ESP_ARDUINO_VERSION
  #include <LittleFS.h>
#else       // for older Arduino core <2.0
  #include <LITTLEFS.h>
  #define LittleFS LITTLEFS
#endif

#ifndef FORMAT_LITTLEFS_IF_FAILED
  #define FORMAT_LITTLEFS_IF_FAILED true
#endif
#define U_FS   U_SPIFFS

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "LList.h"
#include "ts.h"
#include "timeProcessor.h"
#include <functional>

#ifdef EMBUI_MQTT
#include <AsyncMqttClient.h>
#endif

#ifndef EMBUI_PUB_PERIOD
#define EMBUI_PUB_PERIOD              10      // Values Publication period, s
#endif

#define EMBUI_MQTT_PUB_PERIOD         30

#define EMBUI_AUTOSAVE_TIMEOUT        2       // configuration autosave timer, sec    (4 bit value, multiplied by AUTOSAVE_MULTIPLIER)

#ifndef EMBUI_AUTOSAVE_MULTIPLIER
#define EMBUI_AUTOSAVE_MULTIPLIER     (10U)   // множитель таймера автосохранения конфиг файла
#endif

// Default Hostname/AP prefix
#ifndef EMBUI_IDPREFIX
#define EMBUI_IDPREFIX "EmbUI"
#endif

// size of a JsonDocument to hold EmbUI config 
#ifndef EMBUI_CFGSIZE
#define EMBUI_CFGSIZE (2048)
#endif

#define EMBUI_CFGSIZE_MIN_FREE        50        // capacity threshold before compaction

// maximum number of websocket client connections
#ifndef EMBUI_MAX_WS_CLIENTS
#define EMBUI_MAX_WS_CLIENTS          4
#endif

#define EMBUI_WEBSOCK_URI             "/ws"

// TaskScheduler - Let the runner object be a global, single instance shared between object files.
extern Scheduler ts;

// forward declarations
class Interface;
class WiFiController;

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

typedef std::function<void(Interface *interf, JsonObject *data)> actionCallback;

typedef struct section_handle_t{
    String name;
    actionCallback callback;
} section_handle_t;


class EmbUI
{
    typedef union _BITFIELDS {
        struct {
            bool cfgCorrupt:1;      // todo: no use? remove it!
            bool fsDirty:1;         // FS is dirty/unmountable
            uint8_t asave:4;        // 4-bit autosave timer (with AUTOSAVE_MULTIPLIER applied)
        #ifdef EMBUI_MQTT
            bool mqtt_connected:1;
            bool mqtt_connect:1;
            bool mqtt_remotecontrol:1;
            bool mqtt_enable:1;
        #endif  // #ifdef EMBUI_MQTT
        };
        uint32_t flags; // набор битов для конфига
        _BITFIELDS() {
            cfgCorrupt = false;
            fsDirty = false;
            asave = EMBUI_AUTOSAVE_TIMEOUT; // defaul timeout 2*10 sec
        #ifdef EMBUI_MQTT
            mqtt_connected = false;
            mqtt_connect = false;
            mqtt_remotecontrol = false;
            mqtt_enable = false;
        #endif  // #ifdef EMBUI_MQTT
        }
    } BITFIELDS;

    DynamicJsonDocument cfg;                        // system config
    LList<section_handle_t*> section_handle;        // action handlers

  public:
    EmbUI();

    ~EmbUI(){
        ts.deleteTask(tAutoSave);
        ts.deleteTask(*tValPublisher);
        ts.deleteTask(tHouseKeeper);
        delete wifi;
    }

    BITFIELDS sysData;
    AsyncWebServer server;
    AsyncWebSocket ws;
    WiFiController *wifi;
#ifdef EMBUI_MQTT
    typedef void (*mqttCallback) ();
    mqttCallback onConnect;
#endif

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

    /**
     * @brief add ui section handler
     * 
     * @param name section name
     * @param response callback function
     */
    void section_handle_add(const String &name, actionCallback response);

    /**
     * @brief remove section handler
     * 
     * @param name section name
     */
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

    /***  config operations ***/
    void save(const char *_cfg = nullptr, bool force = false);
    void load(const char *cfgfile = nullptr);   // if null, than default cfg file is used
    void cfgclear();                            // clear current config, both in RAM and file

    // tries to load json file from FS and deserialize it into provided DynamicJsonDocument, returns false on error
    bool loadjson(const char *filepath, DynamicJsonDocument &obj);

    /**
     * @brief - initialize/restart config autosave timer
     * each call postpones cfg write to flash
     */
    void autosave(bool force = false);

    /**
     * @brief - process posted data for the registered action
     * if post came from the WebUI echoes received data back to the WebUI,
     * if post came from some other place - sends data to the WebUI
     * looks for registered action for the section name and calls the action with post data if found
     */
    void post(JsonObject &data);



    /*** WiFi/Network related methods***/



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
     * @brief get system MAC ID
     * returns a string with last bytes of MAC address (null terminated)
     */
    const char* macid() const { return mc; };

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

        if ((cfg.capacity() - cfg.memoryUsage()) < EMBUI_CFGSIZE_MIN_FREE){
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
     * Recycler for dynamic tasks
     */
    void taskRecycle(Task *t);



#ifdef EMBUI_MQTT
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
#endif


  private:
    char mc[7]; // last 3 LSB's of mac addr used as an ID
    std::unique_ptr<char[]> autohostname;   // pointer for autogenerated hostname

    // Scheduler tasks
    Task *tValPublisher;     // Status data publisher
    Task tHouseKeeper;     // Maintenance task, runs every second
    Task tAutoSave;          // config autosave timer
    std::vector<Task*> *taskTrash = nullptr;    // ptr to a vector container with obsolete tasks

    /**
     * call to create system-dependent variables,
     * both run-time and persistent
     */ 
    void create_sysvars();

    // Dyn tasks garbage collector
    void taskGC();

    // find callback section matching specified name
    section_handle_t* sectionlookup(const char *id);


    /*** WiFi-related methods ***/

    void _getmacid();


    // HTTP handlers

    /**
     * @brief Set HTTP-handlers for EmbUI related URL's
     * called at framework initialization
     */
    void http_set_handlers();


#ifdef EMBUI_MQTT
    // MQTT Private Methods and vars
    friend void mqtt_dummy_connect();
    AsyncMqttClient mqttClient;

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
#endif

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
#include "embui_wifi.hpp"
