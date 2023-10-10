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
            _connectToMqtt();
        }, &ts, true );
        tMqttReconnector->enable();
    }
}

void EmbUI::_connectToMqtt() {
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

    if (mqttClient.connected())
        mqttClient.disconnect();

    IPAddress ip; 

    if(ip.fromString(mqtt_host))
        mqttClient.setServer(ip, mqtt_port);
    else
        mqttClient.setServer(mqtt_host.c_str(), mqtt_port);

    mqttClient.setKeepAlive(mqtt_ka);
    mqttClient.setCredentials(mqtt_user.c_str(), mqtt_pass.c_str());
    //setWill(mqtt_lwt.c_str(), 0, true, "0").
    mqttClient.connect();
}

void EmbUI::mqtt_start(){
    if (cfg[P_mqtt_on] != true || !cfg.containsKey(P_mqtt_host)){
        LOG(println, "UI: MQTT disabled or no host set");
        return;   // выходим если host не задан
    }

    LOG(println, "Starting MQTT Client");
    mqttClient.onConnect([this](bool sessionPresent){ _onMqttConnect(sessionPresent); });
    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason){_onMqttDisconnect(reason);});
    mqttClient.onMessage( [this](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total){_onMqttMessage(topic, payload, properties, len, index, total);} );
    //mqttClient.onSubscribe(onMqttSubscribe);
    //mqttClient.onUnsubscribe(onMqttUnsubscribe);
    //mqttClient.onPublish(onMqttPublish);

    mqttReconnect();
}

void EmbUI::mqttReconnect(){ // принудительный реконнект, при смене чего-либо в UI
    _mqttConnTask(true);
}

void EmbUI::_onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  LOG(printf, "UI: Disconnected from MQTT:%u\n", static_cast<uint8_t>(reason));
  //mqttReconnect();
}

void EmbUI::_onMqttConnect(bool sessionPresent){
    LOG(println,F("UI: Connected to MQTT."));
/*
    if(sysData.mqtt_remotecontrol){
        subscribeAll();
        // mqttClient.publish(mqtt_lwt.c_str(), 0, true, "1");  // publish Last Will testament
        //httpCallback(F("sys_AUTODISCOVERY"), "", false); // реализация AUTODISCOVERY
    }
*/
    _mqttConnTask(false);
}

void EmbUI::_onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    //LOG(printf, "UI: Got MQTT msg len:%u/%u\n", len, total);

    if (index || len != total) return;     // this is chunked message, reassembly is not supported (yet)
/*
    std::string_view t_view(topic, len);
    const auto pos = t_view.find(mqttPrefix().c_str());
    if (pos == t_view.npos)
    tpc.remove_prefix(mqttPrefix().length());
*/
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
            //mqt(tpc, String(buffer));                           // отправим во внешний пользовательский обработчик
        }
    } else if (/* embui.sysData.mqtt_remotecontrol && */ tpc.startsWith(F("embui/set/"))) {
       String cmd = tpc.substring(10); // sizeof embui/set/
       httpCallback(cmd, String(buffer), true); // нельзя напрямую передавать payload, это не ASCIIZ
    } else if (/* embui.sysData.mqtt_remotecontrol && */ tpc.startsWith(F("embui/jsset/"))) {
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload, len);
        JsonObject obj = doc.as<JsonObject>();
        embui.post(obj);
    } else {
        //mqt(tpc, String(buffer));
    }
}
/*
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
*/
void EmbUI::publish(const char* topic, const char* payload, bool retained){
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
void EmbUI::subscribe(const char* topic, uint8_t qos){
    mqttClient.subscribe(topic, 0);
}
*/
#endif  // EMBUI_MQTT
