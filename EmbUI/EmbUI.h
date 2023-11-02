// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include <list>
#include "globals.h"
#include "embuifs.hpp"
#include "LList.h"
#include "ts.h"
#include "timeProcessor.h"
#include "embui_wifi.hpp"
#include "ui.h"

#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>

#define U_FS   U_SPIFFS

#ifndef EMBUI_PUB_PERIOD
#define EMBUI_PUB_PERIOD              10      // Values Publication period, s
#endif

#ifndef EMBUI_AUTOSAVE_TIMEOUT
#define EMBUI_AUTOSAVE_TIMEOUT        30        // configuration autosave timer, sec
#endif

// Default Hostname/AP prefix
#ifndef EMBUI_IDPREFIX
#define EMBUI_IDPREFIX                "EmbUI"
#endif

// size of a JsonDocument to hold EmbUI config 
#ifndef EMBUI_CFGSIZE
#define EMBUI_CFGSIZE (2048)
#endif

#define EMBUI_CFGSIZE_MIN_FREE        50        // capacity threshold before compaction (bytes)

// maximum number of websocket client connections
#ifndef EMBUI_MAX_WS_CLIENTS
#define EMBUI_MAX_WS_CLIENTS          4
#endif

#define EMBUI_WEBSOCK_URI             "/ws"


// Weak Callback functions (user code might override it)
//void    __attribute__((weak)) section_main_frame(Interface *interf, JsonObject *data, const char* action);
//void    __attribute__((weak)) pubCallback(Interface *interf);
String  __attribute__((weak)) httpCallback(const String &param, const String &value, bool isset);
//uint8_t __attribute__((weak)) uploadProgress(size_t len, size_t total);
//void    __attribute__((weak)) create_parameters();

//---------------------- Callbak functions
using asyncsrv_callback_t = std::function< bool (AsyncWebServerRequest *req)>;
using actionCallback_t = std::function< void (Interface *interf, JsonObject *data, const char* action)>;


/**
 * @brief a struct that keeps action callback handlers
 * used to keep callbacks on post'ed actions with data from UI
 * 
 */
struct section_handler_t {
    // action id
    const char* action;
    // callback function
    actionCallback_t cb;

    section_handler_t(const char* id, actionCallback_t callback) : action(id), cb(callback){};
};

/**
 * @brief a class that manages action handlers
 * add/remove/search, etc...
 * 
 */
class ActionHandler {
    // a list of action handlers
    std::list<section_handler_t> actions;

public:
    /**
     * @brief add ui action handler
     * 
     * @param id action name (note: pointer MUST be valid for the whole lifetime of ActionHandler instance,
     *              the string it points to won't be deep-copied )
     * @param response callback function
     * 
     */
    void add(const char* id, actionCallback_t callback);

    /**
     * @brief replace callback for specified id
     * if action with specified id does not exist in the list, a new action callback will be added ( like via add() )
     * 
     * @param id 
     * @param callback 
     */
    void replace(const char* id, actionCallback_t callback);

    /**
     * @brief remove all handlers matching id
     * 
     * @param id handlers to remove
     */
    void remove(const char* id);

    /**
     * @brief remove all registered actions
     * 
     */
    void clear(){ actions.clear(); };

    /**
     * @brief lookup and execute registered callbacks for the specified action
     * 
     * @return number of callbacks executed, 0 - if no callback were registered for such action
     */
    size_t exec(Interface *interf, JsonObject *data, const char* action);

    /**
     * @brief Set mainpage callback with predefined id - 'mainpage' 
     * defines callback for function that will build main index page for WebUI,
     * will be called on connecting a new websocket client.
     * If not set by user, then a default page will be displayed with system settings
     * 
     * @param callback function to call
     */
    void set_mainpage_cb(actionCallback_t callback);

    /**
     * @brief Set the settings object
     * 
     * @param callback 
     */
    void set_settings_cb(actionCallback_t callback);

    void set_publish_cb(actionCallback_t callback);

};

class EmbUI
{
    DynamicJsonDocument cfg;                        // system config

  public:
    EmbUI();
    ~EmbUI();
    // Copy semantics not implemented
    EmbUI(const EmbUI&) = delete;
    EmbUI& operator=(const EmbUI&) = delete;

    AsyncWebServer server;
    AsyncWebSocket ws;
    WiFiController *wifi;

    // action handler manager object
    ActionHandler action;

    /**
     * @brief  data feeders, that sends EmbUI objects to different proto/subscribers
     * i.e. WebSocket, MQTT, etc...
     */
    FrameSendChain feeders;

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
    template <typename V>
    void var(const char* key, const V& value, bool force = false);

    /**
     * @brief - create config varialbe if it does not exist yet
     * it accepts types suitable to be added to the ArduinoJson cfg document used as a dictionary
     * only non-existing variables are created/assigned a value. If var is already present in cfg
     * it's value won't be replaced
     */
    template <typename T>
    inline void var_create(const char* key, const T& value){ if(!cfg.containsKey(key)){var(key, value, true );} }

    /**
     * @brief - remove key from config
     */
    void var_remove(const char* key);

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
    void var_dropnulls(const char *key, const T& value);


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


    // *** MQTT ***

    /**
     * @brief MQTT client object pointer
     * make it public, so user app could interact it easily, set own callback, publish messages
     * I only provide helper methods to get EmbUI topic prefixes, etc...
     * NOTE: pointer created/destructed on demand, i.e. only if mqtt is enabled in embui configuration and contains valid
     * hostname/port settings.
     * A care should be taken to check if object is not null when calling it's methods
     */
    std::unique_ptr<AsyncMqttClient> mqttClient;

    //void subscribeAll(bool setonly=true);

    /**
     * @brief returns true is MQTT object exist and have a valid connection
     * 
     * @return true 
     * @return false 
     */
    bool mqttAvailable(){ return mqttClient && mqttClient->connected(); }

    /**
     * @brief reset and reestablish mqtt connection
     * 
     */
    void mqttReconnect();

    /**
     * @brief get configured MQTT topic prefix
     * it could be used to craft publish messages
     * @return const String& 
     */
    const String& mqttPrefix() const { return mqtt_topic; }

    /**
     * @brief start MQTT client
     * 
     */
    void mqttStart();

    /**
     * @brief stop MQTT client
     * and release unique pointer
     */
    void mqttStop();

    /**
     * @brief subsribe to topic
     * 
     * @param topic 
     * @param qos 
    void subscribe(const char* topic, uint8_t qos=0);
     */

    /**
     * @brief publish data to MQTT ~ topic
     * a data will be published to a topic with prefix that is set in MQTT properties for EMbUI
     * it's just a shortcut function. For more flexible topic control and message options
     * a user code could use native mqttClient member and mqttPrefix() call to get home prefix
     * 
     * @param topic - suffix to append to home topic (embui/$id)
     * @param payload - string payload data
     * @param retained - keep the message flag
     */
    void publish(const char* topic, const char* payload, bool retained = false);

    /**
     * @brief publish data to MQTT ~ topic
     * a data will be published to a topic with prefix that is set in MQTT properties for EMbUI
     * it's just a shortcut function. For more flexible topic control and message options
     * a user code could use native mqttClient member and mqttPrefix() call to get home prefix
     * 
     * @param topic 
     * @param data  - JsonObject that will be serialized and send to MQTT broker 
     * @param retained - flag
     */
    void publish(const char* topic, const JsonVariantConst& data, bool retained = false);


    /**
     * @brief publish data to MQTT ~ topic
     * templated method that accepts fundamental data types from which String is constructible
     * 
     * @tparam T - topic suffix
     * @tparam P - payload
     * @param topic 
     * @param payload 
     * @param retained 
     */
    template <typename P>
        typename std::enable_if< std::is_fundamental_v<P>, void >::type
    publish(const char* topic, P payload, bool retained = false);


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

    // *** MQTT Private Methods and members ***

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
    int _mqtt_feed_id{0};           // FrameSendMQTT id

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

    /**
     * @brief publish system metrics to mqtt 
     * will publish live values for mem, wifi signal, etc
     * must be called periodicaly
     */
    void _mqtt_pub_sys_status();

    //void _onMqttSubscribe(uint16_t packetId, uint8_t qos);
    //void _onMqttUnsubscribe(uint16_t packetId);
    //void _onMqttPublish(uint16_t packetId);

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


// ----------------------- UI/VAR MACRO's

/**
 * @brief save registered key from the (*data) object into sys config
 * 
 */
#define SETPARAM(key) { embui.var(key, (*data)[key].as<JsonVariant>()); }

/**
 * @brief save key from the (*data) in sys config but only if non empty/non null
 * and call function
 * if empty or null, then drop matching key from sys config
 */
#define SETPARAM_NONULL(key) { embui.var_dropnulls(key, (*data)[key].as<JsonVariant>()); }

/* ======================================== */
/* Templated methods implementation follows */
/* ======================================== */
template <typename V>
void EmbUI::var(const char* key, const V& value, bool force){
    LOG(print, "UI Key:"); LOG(print, key);
    if (!force && !cfg.containsKey(key)) {
        LOG(println, " is NOT initialized!\n");
        return;
    }

    // do not update key if new value is the same as existing one
    if (cfg[key] == value){
        LOG(println, " skip same value");
        return;
    }

    if ((cfg.capacity() - cfg.memoryUsage()) < EMBUI_CFGSIZE_MIN_FREE){
        // cfg is out of mem, try to compact it
        cfg.garbageCollect();
    }

    if (cfg[key].set(value)){
        LOG(printf, " WRITE val:'%s...', mem free: %d\n", cfg[key].template as<String>().substring(0, 10).c_str(), cfg.capacity() - cfg.memoryUsage());
        autosave();
        return;
    }

    LOG(println, " cfg out of mem!");
}


template <typename T>
void EmbUI::var_dropnulls(const char* key, const T& value){
    if constexpr (embui_traits::is_string_v<decltype(value)>)
      if (embui_traits::is_empty_string(value)) return var_remove(key);

    if constexpr (std::is_arithmetic_v<T>)
      if (value == 0) return var_remove(key);

    if constexpr (std::is_same_v<T, JsonVariant>){  // JVars that points to strings must be treaded differently
        if (value.template is<const char*>()) return var_dropnulls(key, value.template as<const char*>());
    }

    // deduce further???
    var(key, value, true ); // save value as-is
}

template <typename P>
    typename std::enable_if< std::is_fundamental_v<P>, void >::type
EmbUI::publish(const char* topic, P payload, bool retained){
    publish(topic, String(payload).c_str(), retained);
}
