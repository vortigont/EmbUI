// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include <list>
#include "globals.h"
#include "embuifs.hpp"
#include "ts.h"
#include "timeProcessor.h"
#include "embui_wifi.hpp"
#include "ui.h"

#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
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

// maximum number of websocket client connections
#ifndef EMBUI_MAX_WS_CLIENTS
#define EMBUI_MAX_WS_CLIENTS          4
#endif

#define EMBUI_WEBSOCK_URI             "/ws"



//---------------------- Callbak functions
using asyncsrv_callback_t = std::function< bool (AsyncWebServerRequest *req)>;
using actionCallback_t = std::function< void (Interface *interf, JsonObjectConst data, const char* action)>;
// embui's language setting callback
using embui_lang_cb_t = std::function< void (const char* lang)>;


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

    section_handler_t(const char* id, const actionCallback_t& callback) : action(id), cb(callback){};
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
    size_t exec(Interface *interf, JsonObjectConst data, const char* action);

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
    JsonDocument cfg;                        // system config

  public:
    EmbUI();
    ~EmbUI();
    // Copy semantics not implemented
    EmbUI(const EmbUI&) = delete;
    EmbUI& operator=(const EmbUI&) = delete;

    AsyncWebServer server;
    AsyncWebSocket ws;
    std::unique_ptr<WiFiController> wifi;

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
     * @brief Get EmbUI's config object
     * a raw access to cfg memeber
     * @todo config functionality needs a reimplementation from scratch, this method is bare plain stub for now
     * 
     * @return JsonObject that maps to EmbUI's /config.json
     */
    JsonObject getConfig(){ return cfg.as<JsonObject>(); }

    /***  config operations ***/
    void save(const char *_cfg = nullptr);
    void load(const char *cfgfile = nullptr);   // if null, than default cfg file is used
    void cfgclear();                            // clear current config, both in RAM and file

    /**
     * @brief - initialize/restart config autosave timer
     * each call postpones cfg write to flash
     */
    void autosave(bool force = false);

    /**
     * @brief - process posted data for the registered action
     * echoes back posted data to all registed feeders
     * looks for registered action for the section name and calls the action with post data if found
     */
    void post(JsonObjectConst data);

    /**
     * @brief Set EmbUI's language
     * 
     * @param lang - two-letter language code (string will be deep-copied to internal EmbUI's var) 
     */
    void setLang(const char* lang);

    // return current ui's language string
    const char* getLang() const;


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
    void on_notfound(const asyncsrv_callback_t& cb){ cb_not_found = cb; };

    /**
     * @brief set language change callback
     * user-defined callback is called when UI's language is changes via system menu
     * 
     * @param cb 
     */
    void onLangChange(embui_lang_cb_t cb);

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
    void publish(const char* topic, const JsonVariantConst data, bool retained = false);


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

    // EmbUI's language
    String _lang;
    // language change user-callback
    embui_lang_cb_t _lang_cb{nullptr};

    // Scheduler tasks
    Task *tValPublisher = nullptr;    // Status data publisher
    Task tHouseKeeper;      // Maintenance task, runs every second
    Task tAutoSave;         // config autosave timer

    // external handler for 404 not found 
    asyncsrv_callback_t cb_not_found = nullptr;

    // AsyncJson handler (for HTTP REST API)
    std::unique_ptr<AsyncCallbackJsonWebHandler> _ajs_handler;



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
    
    /**
     * @brief HTTP rest api req handler
     * executes action for posted json data and hooks to the feeder chain to get the reply
     * ONLY value type packets are responded back to http request since http can't handle section packets as ws/mqtt feedrs 
     * @param request - async http request
     * @param json - json request body
     */
    void _http_api_hndlr(AsyncWebServerRequest *request, JsonVariant &json);

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

    void _mqttSubscribe();

    /**
     * @brief makes an EmbUI-compatible topic from a provided suffix
     *  - prepends EmbUI's configured prefix
     *  - replaces all '_' with '/'
     * 
     * @param topic 
     * @return std::string 
     */
    std::string _mqttMakeTopic(const char* topic);

    //void _onMqttSubscribe(uint16_t packetId, uint8_t qos);
    //void _onMqttUnsubscribe(uint16_t packetId);
    //void _onMqttPublish(uint16_t packetId);

};


class FrameSendMQTT: public FrameSend {
protected:
    EmbUI *_eu;
public:
    explicit FrameSendMQTT(EmbUI *emb) : _eu(emb){}
    virtual ~FrameSendMQTT() { _eu = nullptr; }
    bool available() const override { return _eu->mqttAvailable(); }
    virtual void send(const char* data) override {};     // a do-nothig overload

    /**
     * @brief method will publish to MQTT json-serialized data for EmbUI packets
     * packets with keys "interface" and "xload" are published to EmbUI's topic '~/pub/interface'
     * packets with key "value" is published to EmbUI's topic '~/pub/value'
     * @param data object to publish
     */
    virtual void send(const JsonVariantConst& data) override;
};


// Global EmbUI instance
extern EmbUI embui;


// ----------------------- Templated methods implementation

template <typename P>
    typename std::enable_if< std::is_fundamental_v<P>, void >::type
EmbUI::publish(const char* topic, P payload, bool retained){
    publish(topic, String(payload).c_str(), retained);
}
