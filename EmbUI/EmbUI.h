// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include "globals.h"

#include <FS.h>
#include "embuifs.hpp"
#define U_FS   U_SPIFFS

#include "embuifs.hpp"
#include <ESPAsyncWebServer.h>
#include "LList.h"
#include "ts.h"
#include "timeProcessor.h"
#include <functional>

#ifdef EMBUI_MQTT
#include <AsyncMqttClient.h>
#endif

#include "embui_wifi.hpp"

#ifndef EMBUI_PUB_PERIOD
#define EMBUI_PUB_PERIOD              10      // Values Publication period, s
#endif

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

// forward declarations
class Interface;


// Weak Callback functions (user code might override it)
void    __attribute__((weak)) section_main_frame(Interface *interf, JsonObject *data);
void    __attribute__((weak)) pubCallback(Interface *interf);
String  __attribute__((weak)) httpCallback(const String &param, const String &value, bool isset);
uint8_t __attribute__((weak)) uploadProgress(size_t len, size_t total);
void    __attribute__((weak)) create_parameters();

//---------------------- Callbak functions
using asyncsrv_callback_t = std::function< bool (AsyncWebServerRequest *req)>;
using actionCallback_t = std::function< void (Interface *interf, JsonObject *data)>;


/**
 * @brief a struct that keeps action handlers and their name+id
 * used to run callbacks on actions from UI
 * 
 */
struct section_handle_t{
    String name;
    actionCallback_t callback;
    section_handle_t(const String& n, actionCallback_t& cb) : name(n), callback(cb){};
};


class EmbUI
{
    typedef union _BITFIELDS {
        struct {
            bool cfgCorrupt:1;      // todo: no use? remove it!
            bool fsDirty:1;         // FS is dirty/unmountable
            uint8_t asave:4;        // 4-bit autosave timer (with AUTOSAVE_MULTIPLIER applied)
        #ifdef EMBUI_MQTT
            bool disabled_0:1;          // ex. mqtt_connected
            bool disabled_1:1;          // ex. mqtt_connect
            bool disabled_3:1;          // ex. mqtt_remotecontrol
            bool disabled_2:1;
        #endif  // #ifdef EMBUI_MQTT
        };

        _BITFIELDS() {
            cfgCorrupt = false;
            fsDirty = false;
            asave = EMBUI_AUTOSAVE_TIMEOUT; // defaul timeout 2*10 sec
        #ifdef EMBUI_MQTT
            //disabled_0 = false;
            //disabled_1 = false;
            //disabled_3 = false;
            //disabled_2 = false;
        #endif  // #ifdef EMBUI_MQTT
        }
    } BITFIELDS;

    DynamicJsonDocument cfg;                        // system config
    LList<std::shared_ptr<section_handle_t>> section_handle;        // action handlers


  public:
    EmbUI();
    ~EmbUI();
    // Copy semantics not implemented
    EmbUI(const EmbUI&) = delete;
    EmbUI& operator=(const EmbUI&) = delete;

    BITFIELDS sysData;
    AsyncWebServer server;
    AsyncWebSocket ws;
    WiFiController *wifi;

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
    void section_handle_add(const String &name, actionCallback_t response);

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
    template <typename T>
    JsonVariantConst paramVariant(const T &key) const { return cfg[key]; }


    bool isparamexists(const char* key){ return cfg.containsKey(key);}
    bool isparamexists(const String &key){ return cfg.containsKey(key);}

    /***  config operations ***/
    void save(const char *_cfg = nullptr, bool force = false);
    void load(const char *cfgfile = nullptr);   // if null, than default cfg file is used
    void cfgclear();                            // clear current config, both in RAM and file

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
    void post(JsonObject &data, bool inject = false);



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
    template <typename T>
    void var(const String &key, const T& value, bool force = false);

    /**
     * @brief - create config varialbe if it does not exist yet
     * it accepts types suitable to be added to the ArduinoJson cfg document used as a dictionary
     * only non-existing variables are created/assigned a value. If var is already present in cfg
     * it's value won't be replaced
     */
    template <typename T>
    inline void var_create(const String &key, const T& value){ if(!cfg.containsKey(key)){var(key, value, true );} }

    /**
     * @brief - remove key from config
     */
    void var_remove(const String &key);

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
    void var_dropnulls(const String &key, const T& value);

    void var_dropnulls(const String &key, const char* value);
    void var_dropnulls(const String &key, JsonVariant value);

    // call-backs

    /**
     * @brief set callback for 404 not found operation
     * if set, than any 404 request will be passed to callback first before being processed by EmbUI
     * if callback returns 'true', than EmbUI consders it has already server the request and just quits
     * if callback returns 'flase', than usual EmbUI 404 handle will be executed
     * NOTE: EMBUI uses 404 callback for implementing Captive Portal in AP mode
     * do NOT override it for all requests without knowing how it works internally
     * @param cb call back function
     */
    void on_notfound(asyncsrv_callback_t cb){ cb_not_found = cb; };

#ifdef EMBUI_UDP
    void udp(const String &message);
    void udp();
#endif // EMBUI_UDP


#ifdef EMBUI_MQTT
    // MQTT
    /**
     * @brief MQTT client object
     * make it public, so user app could interact it easily, set own callback, publish messages
     * I only provide helper methods to get EmbUI topic prefixes, etc...
     * 
     */
    AsyncMqttClient mqttClient;


    void pub_mqtt(const String &key, const String &value);
    void subscribeAll(bool setonly=true);

    /**
     * @brief start MQTT client
     * 
     */
    void mqtt_start();

    /**
     * @brief stop MQTT client
     * 
     */
    void mqtt_stop(){ _mqttConnTask(false); }

    /*
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
    */
    //void mqttSetCallbacks(AsyncMqttClientInternals::OnMessageUserCallback onMessage, AsyncMqttClientInternals::OnConnectUserCallback onConnect);

    void mqttReconnect();

    /**
     * @brief get configured MQTT topic prefix
     * it could be used to craft publish messages
     * @return const String& 
     */
    const String& mqttPrefix() const { return mqtt_topic; }

    /**
     * @brief subsribe to topic
     * 
     * @param topic 
     * @param qos 
     */
    void subscribe(const char* topic, uint8_t qos=0);

    void publish(const char* topic, const char* payload, bool retained = false);
    void publish(const String &topic, const String &payload, bool retained = false);

    //void publish(const String &topic, const String &payload, bool retained);
    //void publishto(const String &topic, const String &payload, bool retained);
#endif

/* ********** PRIVATE members *********** */
  private:
    char mc[13];                            // chip's mac addr used as an ID
    std::unique_ptr<char[]> autohostname;   // pointer for autogenerated hostname

    // Scheduler tasks
    Task *tValPublisher = nullptr;    // Status data publisher
    Task tHouseKeeper;      // Maintenance task, runs every second
    Task tAutoSave;         // config autosave timer

    // external handler for 404 not found 
    asyncsrv_callback_t cb_not_found = nullptr;

    /**
     * call to create system-dependent variables,
     * both run-time and persistent
     */ 
    void create_sysvars();

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

    // default callback for http 404 responces
    void _notFound(AsyncWebServerRequest *request);

#ifdef EMBUI_MQTT
    // MQTT Private Methods and members
    // need to keep literal params in obj and pass value by reference
    String mqtt_topic;
    String mqtt_host;               // server host or IP
    uint16_t mqtt_port = 1883;      // server port
    uint16_t mqtt_ka = 30;          // keep-alive time
    String mqtt_user;
    String mqtt_pass;
    //String mqtt_lwt;              // last will testament

    // Task that will make reconnect attempts to mqtt
    Task *tMqttReconnector = nullptr;

    // User definable MQTT callbacks
    //AsyncMqttClientInternals::OnConnectUserCallback _mqtt_OnConnectUserCallback = nullptr;
    //AsyncMqttClientInternals::OnMessageUserCallback _mqtt_OnMessageUserCallback = nullptr;
    //AsyncMqttClientInternals::OnDisconnectUserCallback _mqtt_OnDisconnectUserCallback = nullptr;

    /**
     * @brief enable/disable reconnecting task
     * 
     * @param state 
     */
    void _mqttConnTask(bool state);

    /**
     * @brief gets mqtt parameters from config and tries to connect to mqtt server
     * if mqtt is already connected, the old connection will be disconnected
     * 
     */
    void _connectToMqtt();

    /**
     * @brief callback function that triggers when MQTT connects to broker
     * 
     */
    void _onMqttConnect(bool sessionPresent);

    /**
     * @brief callback function that triggers when MQTT disconnects from broker
     * 
     * @param reason 
     */
    void _onMqttDisconnect(AsyncMqttClientDisconnectReason reason);

    /**
     * @brief EmbUI's callback for incoming MQTT messages
     * it will pick any interesting topics related to EmbUI internals and skip all others
     * user app code should subscribe on messages on his own, this callbach won't call user's code
     * 
     */
    void _onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

    //void _onMqttSubscribe(uint16_t packetId, uint8_t qos);
    //void _onMqttUnsubscribe(uint16_t packetId);
    //void _onMqttPublish(uint16_t packetId);
#endif

#ifdef USE_SSDP
    void ssdp_begin() {
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
          SSDP.setModelName(PGnameModel);
          SSDP.setModelNumber(PGversion);
          SSDP.setModelURL(String(F("http://"))+(WiFi.status() != WL_CONNECTED ? WiFi.softAPIP().toString() : WiFi.localIP().toString()));
          SSDP.setManufacturer(PGnameManuf);
          SSDP.setManufacturerURL(PGurlManuf);
          SSDP.begin();

          (&server)->on(PSTR("/description.xml"), HTTP_GET, [&](AsyncWebServerRequest *request){
            request->send(200, PGmimexml, getSSDPSchema());
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
        s += PGnameModel;
        s +=F("</modelName>\r\n");
        s +=F("\t<modelNumber>");
        s += PGversion;
        s +=F("</modelNumber>\r\n");
        s +=F("\t<modelURL>");
        s += PGurlModel;
        s +=F("</modelURL>\r\n");
        s +=F("\t<manufacturer>");
        s += PGnameManuf;
        s +=F("</manufacturer>\r\n");
        s +=F("\t<manufacturerURL>");
        s += PGurlManuf;
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

// Global EmbUI instance
extern EmbUI embui;
#include "ui.h"


// ----------------------- UI/VAR MACRO's

/**
 * @brief save key from the (*data) in sys config and call function
 * 
 */
#define SETPARAM(key, call...) { \
    embui.var(key, (*data)[key]); \
    call; \
}

/**
 * @brief save key from the (*data) in sys config but only if non empty/non null
 * and call function
 * if empty or null, then drop matching key from sys config
 */
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

/* ======================================== */
/* Templated methods implementation follows */
/* ======================================== */

template <typename T>
void EmbUI::var(const String &key, const T& value, bool force){
    if (!force && !cfg.containsKey(key)) {
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


template <typename T>
void EmbUI::var_dropnulls(const String &key, const T& value){
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
