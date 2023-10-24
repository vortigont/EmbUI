/*
  Jusr another EMBUI fork of a fork :) https://github.com/vortigont/EmbUI/

  This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
  then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
  also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
  and others people
*/

#include "EmbUI.h"
#include "flashz-http.hpp"

static const char* UPDATE_URI = "/update";
FlashZhttp fz;

/**
 * @brief OTA update handler
 * 
 */
void ota_handler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

/*
 * OTA update progress calculator
 */
//uint8_t uploadProgress(size_t len, size_t total);

/**
 * @brief Default HTTP callback function
 * a catch-all stub for uknown http /cmd commands
 */
String httpCallback(const String &param, const String &value, bool isSet) { return String(); }

// default 404 handler
void EmbUI::_notFound(AsyncWebServerRequest *request) {

    if (cb_not_found && cb_not_found(request)) return;      // process redirect via external call-back if set

    // if external cb is not defined or returned false, than handle it via captive-portal or return 404
    if (!embui.paramVariant(V_NOCaptP) && WiFi.getMode() & WIFI_AP){         // return redirect to root page in Captive-Portal mode
        request->redirect("/");
        return;
    }
    request->send(404);
}

/**
 * @brief Set HTTP-handlers for EmbUI related URL's
 */
void EmbUI::http_set_handlers(){

    // HTTP commands handler
    server.on(PSTR("/cmd"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        String result; 
        int params = request->params();
        for(int i=0;i<params;i++){
            AsyncWebParameter* p = request->getParam(i);
            if(p->isFile()){ //p->isPost() is also true
                //Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
            } else if(p->isPost()){
                //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            } else {
                //Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
                result = httpCallback(p->name(), p->value(), !p->value().isEmpty());
            }
        }
        request->send(200, PGmimetxt, result);
    });


    // returns run-time system config serialized in JSON
    server.on(PSTR("/config"), HTTP_ANY, [this](AsyncWebServerRequest *request) {

        AsyncResponseStream *response = request->beginResponseStream(PGmimejson);
        response->addHeader(PGhdrcachec, PGnocache);

        serializeJson(cfg, *response);

        request->send(response);
    });


    server.on(PSTR("/version"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        request->send(200, PGmimetxt, F("EmbUI ver: " TOSTRING(EMBUIVER)));
    });

    // postponed reboot (TODO: convert to CMD)
    server.on(PSTR("/restart"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        Task *t = new Task(TASK_SECOND*5, TASK_ONCE, nullptr, &ts, false, nullptr, [](){ LOG(println, F("Rebooting...")); delay(100); ESP.restart(); });
        t->enableDelayed();
        request->redirect(F("/"));
    });

    // esp32 handles updates via external lib
    fz.provide_ota_form(&server, UPDATE_URI);
    fz.handle_ota_form(&server, UPDATE_URI);

    // some ugly stats
    server.on(PSTR("/heap"), HTTP_GET, [this](AsyncWebServerRequest *request){
        String out = F("Heap: ");
        out += ESP.getFreeHeap();
        out += F("\nWS Client: ");
        out += ws.count();
        request->send(200, PGmimetxt, out);
    });

    // serve all static files from LittleFS root /
    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile(PSTR("index.html"))
        .setCacheControl(PSTR("max-age=10, must-revalidate"));  // 10 second for caching, then revalidate based on etag/IMS headers


    // 404 handler - disabled to allow override in user code
    server.onNotFound([this](AsyncWebServerRequest *r){_notFound(r);});


/*
    // WiFi AP scanner (pretty useless)
    //First request will return 0 results unless you start scan from somewhere else (loop/setup)
    //Do not request more often than 3-5 seconds
    server.on(PSTR("/scan"), HTTP_GET, [](AsyncWebServerRequest *request){
        String json = F("[");
        int n = WiFi.scanComplete();
        if(n == -2){
            WiFi.scanNetworks(true);
        } else if(n){
            for (int i = 0; i < n; ++i){
            if(i) json += F(",");
            json += F("{");
            json += String(F("\"rssi\":"))+String(WiFi.RSSI(i));
            json += String(F(",\"ssid\":\""))+WiFi.SSID(i)+F("\"");
            json += String(F(",\"bssid\":\""))+WiFi.BSSIDstr(i)+F("\"");
            json += String(F(",\"channel\":"))+String(WiFi.channel(i));
            json += String(F(",\"secure\":"))+String(WiFi.encryptionType(i));
            json += F("}");
            }
            WiFi.scanDelete();
            if(WiFi.scanComplete() == -2){
            WiFi.scanNetworks(true);
            }
        }
        json += F("]");
        request->send(200, PGmimejson, json);
    });
*/

/*
    // может пригодится позже, файлы отдаются как статика

    server.on(PSTR("/config.json"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, P_cfgfile, String(), true);
    });

    server.on(PSTR("/events_config.json"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, F("/events_config.json"), String(), true);
    });
*/

}   //  end of EmbUI::http_set_handlers

/*
 * OTA update progress calculator

uint8_t uploadProgress(size_t len, size_t total){
    static int prev = 0;
    int parts = total / 25;  // logger chunks (each 4%)
    int curr = len / parts;
    uint8_t progress = 100*len/total;
    if (curr != prev) {
        prev = curr;
        LOG(printf_P, PSTR("%u%%.."), progress );
    }
    return progress;
}
 */
