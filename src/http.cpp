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

// default 404 handler
void EmbUI::_notFound(AsyncWebServerRequest *request) {

    if (cb_not_found && cb_not_found(request)) return;      // process redirect via external call-back if set

    // if external cb is not defined or returned false, than handle it via captive-portal or return 404
    if (!_cfg[V_NOCaptP] && WiFi.getMode() & WIFI_AP){         // return redirect to root page in Captive-Portal mode
        request->redirect("/");
        return;
    }
    request->send(404);
}

/**
 * @brief Set HTTP-handlers for EmbUI related URL's
 */
void EmbUI::http_set_handlers(){

    // returns run-time system config serialized in JSON
    server.on("/config", HTTP_ANY, [this](AsyncWebServerRequest *request) {

        AsyncResponseStream *response = request->beginResponseStream(asyncsrv::T_application_json);
        response->addHeader(asyncsrv::T_Cache_Control, asyncsrv::T_no_cache);

        serializeJson(_cfg, *response);

        request->send(response);
    });


    server.on("/version", HTTP_ANY, [this](AsyncWebServerRequest *request) {
        request->send(200, PGmimetxt, F("EmbUI ver: " TOSTRING(EMBUIVER)));
    });

    // postponed reboot (TODO: convert to CMD)
    server.on("/restart", HTTP_ANY, [this](AsyncWebServerRequest *request) {
        Task *t = new Task(TASK_SECOND*5, TASK_ONCE, nullptr, &ts, false, nullptr, [](){ ESP.restart(); });
        t->enableDelayed();
        request->redirect("/");
    });

    // HTTP REST API handler
    _ajs_handler = std::make_unique<AsyncCallbackJsonWebHandler>("/api", [this](AsyncWebServerRequest *r, JsonVariant &json) { _http_api_hndlr(r, json); });
    if (_ajs_handler)
        server.addHandler(_ajs_handler.get());

    // esp32 handles updates via external lib
    fz.provide_ota_form(&server, UPDATE_URI);
    fz.handle_ota_form(&server, UPDATE_URI);

    // serve all static files from LittleFS root /
    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("index.html")
        .setCacheControl("max-age=10, must-revalidate");  // 10 second for caching, then revalidate based on etag/IMS headers


    // 404 handler - disabled to allow override in user code
    server.onNotFound([this](AsyncWebServerRequest *r){_notFound(r);});

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
        LOG(printf_P, "%u%%..", progress );
    }
    return progress;
}
 */

void EmbUI::_http_api_hndlr(AsyncWebServerRequest *request, JsonVariant &json){
    // TODO:
    // the specific for this handler is that it won't inject action responces to regostered feeders
    // it's a design gap, I can't handle WS multimessaging and HTTP call in the same manner
    Interface interf(request);
    action.exec(&interf, json[P_data], json[P_action].as<const char*>());
}
