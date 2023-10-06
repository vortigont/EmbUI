// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#ifdef EMBUI_MQTT

#include "EmbUI.h"
extern EmbUI embui;

void EmbUI::_mqttConnTask(bool state){
    if (!state){
        delete tMqttReconnector;
        tMqttReconnector = nullptr;
        return;
    }

    if(tMqttReconnector){
        tValPublisher->restart();
    } else {
        tMqttReconnector = new Task(15 * TASK_SECOND, TASK_FOREVER, [this](){
            if (!(WiFi.getMode() & WIFI_MODE_STA)) return;
            connectToMqtt();
        }, &ts, true );
        tMqttReconnector->enable();
    }
}

void EmbUI::connectToMqtt() {
    LOG(println, PSTR("Connecting to MQTT..."));

    if (cfg[P_mqtt_topic]){
        mqtt_topic = cfg[P_mqtt_topic].as<const char*>();
        mqtt_topic.replace("$id", mc);
    } else {
        mqtt_topic = "embui/";
        mqtt_topic += mc;
        mqtt_topic += (char)0x2f; // "/"
    }

    mqtt_host = paramVariant(P_mqtt_host).as<const char*>();
    mqtt_port = cfg[P_mqtt_port];
    mqtt_user = paramVariant(P_mqtt_user).as<const char*>();
    mqtt_pass = paramVariant(P_mqtt_pass).as<const char*>();
    //mqtt_lwt=id(F("embui/pub/online"));

    mqttClient.disconnect(true);

    IPAddress ip; 
    bool isIP = ip.fromString(mqtt_host);

    if(isIP)
        mqttClient.setServer(ip, mqtt_port);
    else
        mqttClient.setServer(mqtt_host.c_str(), mqtt_port);

    mqttClient.setKeepAlive(mqtt_ka);
    mqttClient.setCredentials(mqtt_user.c_str(), mqtt_pass.c_str());
    //mqttClient.setClientId(mqtt_topic.isEmpty() ? mc : mqtt_topic.c_str());
    //setWill(mqtt_lwt.c_str(), 0, true, "0").
    mqttClient.connect();
}
/*
String EmbUI::id(const String &topic){
    String ret = mqtt_topic;
    if (ret.isEmpty()) return topic;

    ret += '/'; ret += topic;
    return ret;
}
*/
void EmbUI::onMqttSubscribe(uint16_t packetId, uint8_t qos) {}

void EmbUI::onMqttUnsubscribe(uint16_t packetId) {}

void EmbUI::onMqttPublish(uint16_t packetId) {}

typedef void (*mqttCallback) (const String &topic, const String &payload);
mqttCallback mqt;

void mqtt_dummy_connect(){ embui.onMqttConnect(); }
void mqtt_emptyFunction(const String &, const String &){}

void EmbUI::mqtt_start(){
    if (cfg[P_mqtt_on] != true || !cfg.containsKey(P_mqtt_host)){
        LOG(println, "UI: MQTT disabled or no host set");
        return;   // выходим если host не задан
    }

    LOG(println, "Starting MQTT Client");
    mqttClient.onConnect([this](bool sessionPresent){ onMqttConnect(); });
    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason){onMqttDisconnect(reason);});
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);

    mqt = mqtt_emptyFunction;       // пользователький обработчик сообщений (заглушка)
    mqttReconnect();
}
/*
void EmbUI::mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), bool remotecontrol){
    if (host.isEmpty()){
        LOG(println, PSTR("UI: MQTT host is empty - disabled!"));
        return;   // выходим если host не задан
    }

    if(mqtt_topic == P_null) var(P_mqtt_topic, pref);
    if(mqtt_host == P_null) var(P_mqtt_host, host);
    if(mqtt_port == P_null) var(P_mqtt_port, String(port));
    if(mqtt_user == P_null) var(P_mqtt_user, user);
    if(mqtt_pass == P_null) var(P_mqtt_pass, pass);

    LOG(println, PSTR("UI: MQTT Init completed"));

    if (remotecontrol) embui.sysData.mqtt_remotecontrol = true;
    mqt = mqttFunction;

    mqttClient.onConnect([this](){Serial.println("sss");});
    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason){onMqttDisconnect(reason);});
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);
    
    sysData.mqtt_enable = true;
    mqttReconnect();
}

void EmbUI::mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload)){
    mqtt(pref, host, port, user, pass, mqttFunction, false);
    onConnectCallBack = mqtt_dummy_connect;
}

void EmbUI::mqtt(const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload)){
    mqtt(mc, host, port, user, pass, mqttFunction, false);
    onConnectCallBack = mqtt_dummy_connect;
}

void EmbUI::mqtt(const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), bool remotecontrol){
    mqtt(mc, host, port, user, pass, mqttFunction, remotecontrol);
    onConnectCallBack = mqtt_dummy_connect;
}

void EmbUI::mqtt(const String &host, int port, const String &user, const String &pass, bool remotecontrol){
    mqtt(mc, host, port, user, pass, mqtt_emptyFunction, remotecontrol);
    onConnectCallBack = mqtt_dummy_connect;
}

void EmbUI::mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, bool remotecontrol){
    mqtt(pref, host, port, user, pass, mqtt_emptyFunction, remotecontrol);
    onConnectCallBack = mqtt_dummy_connect;
}

void EmbUI::mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) (), bool remotecontrol){
    mqtt(pref, host, port, user, pass, mqtt_emptyFunction, remotecontrol);
    onConnectCallBack = mqttConnect;

}
void EmbUI::mqtt(const String &pref, const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) ()){
    mqtt(pref, host, port, user, pass, mqttFunction, false);
    onConnectCallBack = mqttConnect;
}
void EmbUI::mqtt(const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) ()){
    mqtt(mc, host, port, user, pass, mqttFunction, false);
    onConnectCallBack = mqttConnect;
}
void EmbUI::mqtt(const String &host, int port, const String &user, const String &pass, void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) (), bool remotecontrol){
    mqtt(mc, host, port, user, pass, mqttFunction, remotecontrol);
    onConnectCallBack = mqttConnect;
}
void EmbUI::mqtt(void (*mqttFunction) (const String &topic, const String &payload), bool remotecontrol){
    mqt = mqttFunction;
    if (remotecontrol) embui.sysData.mqtt_remotecontrol = true;
}
*/

void EmbUI::mqtt(void (*mqttFunction) (const String &topic, const String &payload), void (*mqttConnect) (), bool remotecontrol){
    onConnectCallBack = mqttConnect;
    mqt = mqttFunction;
    if (remotecontrol) embui.sysData.mqtt_remotecontrol = true;
}

void EmbUI::mqttReconnect(){ // принудительный реконнект, при смене чего-либо в UI
    _mqttConnTask(true);
}

void EmbUI::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  LOG(println,F("UI: Disconnected from MQTT."));
  //mqttReconnect();
}

void EmbUI::onMqttConnect(){
    LOG(println,F("UI: Connected to MQTT."));
    if(sysData.mqtt_remotecontrol){
        subscribeAll();
        // mqttClient.publish(mqtt_lwt.c_str(), 0, true, "1");  // publish Last Will testament
        //httpCallback(F("sys_AUTODISCOVERY"), "", false); // реализация AUTODISCOVERY
    }
    _mqttConnTask(false);
}

void EmbUI::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    //LOG(printf, "UI: Got MQTT msg len:%u/%u\n", len, total);

    char buffer[len + 2];
    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, payload, len);

    String tpc(topic);
    String mqtt_topic = embui.param(P_mqtt_topic); 
    if (!mqtt_topic.isEmpty()) tpc = tpc.substring(mqtt_topic.length() + 1, tpc.length());

    if (tpc.equals(F("embui/get/config"))) {
        String jcfg; serializeJson(embui.cfg, jcfg);
        embui.publish(F("embui/pub/config"), jcfg, false);    
    } else if (tpc.startsWith(F("embui/get/"))) {
        String param = tpc.substring(10); // sizeof embui/set/
        if(embui.isparamexists(param))
            embui.publish(String(F("embui/pub/")) + param, embui.param(param), false);
        else {
            httpCallback(param, String(buffer), false); // нельзя напрямую передавать payload, это не ASCIIZ
            mqt(tpc, String(buffer));                           // отправим во внешний пользовательский обработчик
        }
    } else if (embui.sysData.mqtt_remotecontrol && tpc.startsWith(F("embui/set/"))) {
       String cmd = tpc.substring(10); // sizeof embui/set/
       httpCallback(cmd, String(buffer), true); // нельзя напрямую передавать payload, это не ASCIIZ
    } else if (embui.sysData.mqtt_remotecontrol && tpc.startsWith(F("embui/jsset/"))) {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload, len);
        JsonObject obj = doc.as<JsonObject>();
        embui.post(obj);
    } else {
        mqt(tpc, String(buffer));
    }
}

void EmbUI::subscribeAll(bool setonly){
    String t(mqttPrefix());
    LOG(print, "MQTT: Subscribe {prefix}/");
    if(setonly){
        t += "set/#";
        LOG(println, "set/#");
    } else {
        t += (char)0x23;  //"#"
        LOG(println, "#");
    }
    mqttClient.subscribe(t.c_str(), 0);
}

void EmbUI::publish(const char* topic, const char* payload, bool retained){
    if (!mqttClient.connected()) return;
    /*
    LOG(print, "MQTT pub: topic:");
    LOG(print, topic);
    LOG(print, " payload:");
    LOG(println, payload);
    */
    mqttClient.publish(topic, 0, retained, payload);
}

void EmbUI::publish(const String& topic, const String& payload, bool retained){
    publish(topic.c_str(), payload.c_str(), retained);
}
/*
void EmbUI::publish(const String &topic, const String &payload){
    if (!(WiFi.getMode() & WIFI_MODE_STA) || !cfg[P_mqtt_on]) return;
    mqttClient.publish(id(topic).c_str(), 0, false, payload.c_str());
}

void EmbUI::publishto(const String &topic, const String &payload, bool retained){
    if (!(WiFi.getMode() & WIFI_MODE_STA) || !cfg[P_mqtt_on]) return;
    mqttClient.publish(topic.c_str(), 0, retained, payload.c_str());
}

void EmbUI::pub_mqtt(const String &key, const String &value){
    if (!mqttClient.connected()) return;
    publish(key, value, true);
}
*/
void EmbUI::subscribe(const char* topic, uint8_t qos){
    mqttClient.subscribe(topic, 0);
}

#endif  // EMBUI_MQTT
