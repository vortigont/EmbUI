/*
  Jusr another EMBUI fork of a fork :) https://github.com/vortigont/EmbUI/

  This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
  then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
  also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
  and others people
*/

#include "EmbUI.h"
#if defined ESP32
#include "flashz-http.hpp"
#endif

// Update defs
#ifndef ESP_IMAGE_HEADER_MAGIC
#define ESP_IMAGE_HEADER_MAGIC 0xE9
#endif
#ifndef GZ_HEADER
#define GZ_HEADER   0x1F
#endif
#ifndef ZLIB_HEADER
#define ZLIB_HEADER 0x78
#endif

#ifdef ESP8266
#define COMPRESSED_FW_HEADER GZ_HEADER
#elif defined ESP32
#define COMPRESSED_FW_HEADER ZLIB_HEADER
#endif

// fwd declaration
#ifdef ESP8266
void ota_register(AsyncWebServer &server, const char* url);
#endif

/**
 * @brief OTA update handler
 * 
 */
void ota_handler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

/*
 * OTA update progress calculator
 */
uint8_t uploadProgress(size_t len, size_t total);

/**
 * @brief Default HTTP callback function
 * a catch-all stub for uknown http handlers
 */
String httpCallback(const String &param, const String &value, bool isSet) { return String(); }

// default 404 handler
void notFound(AsyncWebServerRequest *request) {
    request->send(404, FPSTR(PGmimetxt), FPSTR(PG404));
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
        request->send(200, FPSTR(PGmimetxt), result);
    });


    // returns run-time system config serialized in JSON
    server.on(PSTR("/config"), HTTP_ANY, [this](AsyncWebServerRequest *request) {

        AsyncResponseStream *response = request->beginResponseStream(FPSTR(PGmimejson));
        response->addHeader(FPSTR(PGhdrcachec), FPSTR(PGnocache));

        serializeJson(cfg, *response);

        request->send(response);
    });


    server.on(PSTR("/version"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        request->send(200, FPSTR(PGmimetxt), F("EmbUI ver: " TOSTRING(EMBUIVER)));
    });

    // postponed reboot (TODO: convert to CMD)
    server.on(PSTR("/restart"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        Task *t = new Task(TASK_SECOND*5, TASK_ONCE, nullptr, &ts, false, nullptr, [](){ LOG(println, F("Rebooting...")); delay(100); ESP.restart(); });
        t->enableDelayed();
        request->redirect(F("/"));
    });

// esp32 handles updates via external lib
#ifdef ESP32
    fz_async_register_ota(server, "/update");
#endif

#ifdef ESP8266
    static const char* url PROGMEM = "/update";
    ota_register(server, url);
#endif


    // some ugly stats
    server.on(PSTR("/heap"), HTTP_GET, [this](AsyncWebServerRequest *request){
        String out = F("Heap: ");
        out += ESP.getFreeHeap();
    #ifdef ESP8266
        out += F("\nFrag: ");
        out += ESP.getHeapFragmentation();
    #endif
        out += F("\nWS Client: ");
        out += ws.count();
        request->send(200, FPSTR(PGmimetxt), out);
    });

    // serve all static files from LittleFS root /
    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile(PSTR("index.html"))
        .setCacheControl(PSTR("max-age=14400"));


    // 404 handler
    server.onNotFound(notFound);


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
#ifdef ESP8266
            json += String(F(",\"hidden\":"))+String(WiFi.isHidden(i)?FPSTR(P_true):FPSTR(P_false)); // что-то сломали и в esp32 больше не работает...
#endif
            json += F("}");
            }
            WiFi.scanDelete();
            if(WiFi.scanComplete() == -2){
            WiFi.scanNetworks(true);
            }
        }
        json += F("]");
        request->send(200, FPSTR(PGmimejson), json);
    });
*/

/*
    // может пригодится позже, файлы отдаются как статика

    server.on(PSTR("/config.json"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, FPSTR(P_cfgfile), String(), true);
    });

    server.on(PSTR("/events_config.json"), HTTP_ANY, [this](AsyncWebServerRequest *request) {
        request->send(LittleFS, F("/events_config.json"), String(), true);
    });
*/

}   //  end of EmbUI::http_set_handlers

#ifdef ESP8266
void ota_register(AsyncWebServer &server, const char* url){
    // Simple OTA-Update Form
    server.on(url, HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, FPSTR(PGmimehtml), F("<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' accept='.bin, .gz' name='update'><input type='submit' value='Update'></form>"));
    });

    server.on(url, HTTP_POST, [](AsyncWebServerRequest *request){
        if (Update.hasError()) {
            AsyncWebServerResponse *response = request->beginResponse(500, FPSTR(PGmimetxt), F("UPDATE FAILED"));
            response->addHeader(F("Connection"), F("close"));
            request->send(response);
        } else {
            request->redirect(F("/restart"));   // todo: set some nice UI page for this
        }
    },  ota_handler );
}

void ota_handler(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    //LOG(printf, "OTA - file: %s, idx: %u, len: %u, final:%u, byte: %02X\n", filename.c_str(), index, len, final, data[0]);

    if (!index) {
        int type;
        if (request->hasParam(FPSTR(PGimg), true)){
            // image type is specified in the form data
            type = request->getParam(FPSTR(PGimg), true)->value() == F("fs") ? U_FS : U_FLASH;
        } else {
            // no image type specified, try to autodetect
            if (data[0] == ESP_IMAGE_HEADER_MAGIC || data[0] == COMPRESSED_FW_HEADER)        // can't detect what is insize zlib, so assume it's a fw image (won't owerwrite chip's FS)
                type = U_FLASH;
            else
                type = U_FS;
        }

        // accept only uncompressed fw images with magic or properly compressed images
        if (not (data[0] == ESP_IMAGE_HEADER_MAGIC || data[0] == COMPRESSED_FW_HEADER) )
            return request->send(400, FPSTR(PGmimetxt), F("Not an FW image or img type is unknown"));

        Update.runAsync(true);
        size_t size = (type == U_FLASH)? request->contentLength() : (uintptr_t)&_FS_end - (uintptr_t)&_FS_start;

        LOG(printf_P, PSTR("Updating %s, file size:%u\n"), (type == U_FLASH)? "Firmware" : "Filesystem", request->contentLength());

        if (!Update.begin(size, type)) {
        #ifdef EMBUI_DEBUG
            Update.printError(EMBUI_DEBUG_PORT);
        #endif
            return request->send(503, FPSTR(PGmimetxt), F("Can't start OTA"));
        }
    }
    if (len) {
        if(Update.write(data, len) != len){
        #ifdef EMBUI_DEBUG
            Update.printError(EMBUI_DEBUG_PORT);
            LOG(println, "OTA failed in progress");
        #endif
            return request->send(503, FPSTR(PGmimetxt), F("OTA failed in progress"));
        }
    }
    if (final) {
        if(Update.end(true)){
            LOG(printf_P, PSTR("Update Success: %u bytes\n"), index+len);
        } else {
        #ifdef EMBUI_DEBUG
            Update.printError(EMBUI_DEBUG_PORT);
            LOG(println, "OTA failed to complete");
        #endif
            return request->send(503, FPSTR(PGmimetxt), F("OTA failed to complete"));
        }
    }
    #ifdef EMBUI_DEBUG
    uploadProgress(index + len, request->contentLength());
    #endif
}
#endif  // #ifdef ESP8266

/*
 * OTA update progress calculator
 */
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

