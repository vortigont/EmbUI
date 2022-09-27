// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#ifdef ESP32
 #include <WiFi.h>
 #include <ESPmDNS.h>
#ifdef USE_SSDP
 #include <ESP32SSDP.h>
#endif
#endif

#include "ts.h"

#define WIFI_CONNECT_TIMEOUT    10                      // timer for WiFi STA connection attempt 
#define WIFI_SET_AP_AFTER_DISCONNECT_TIMEOUT    15      // time after WiFi client disconnects and before internal AP is brought up
#define WIFI_RECONNECT_TIMER    30                      // timer for STA connect retry
#define WIFI_BEGIN_DELAY        3                       // scheduled delay for STA begin() connection

#define WIFI_PSK_MIN_LENGTH     8

class EmbUI;

class WiFiController {
    EmbUI *emb;
    Task tWiFi;       // WiFi reconnection helper
    wifi_event_id_t eid;

    // WiFi events callback handler
    void _onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

    /**
     * @brief bring up mDNS service
     * 
     */
    void setup_mDns();

    /**
      * update WiFi AP params and state
      */
    //void wifi_updateAP();

public:
    WiFiController(EmbUI *ui) : emb(ui){
      ts.addTask(tWiFi);
          // Set WiFi event handlers
      eid = WiFi.onEvent(std::bind(&WiFiController::_onWiFiEvent, this, std::placeholders::_1, std::placeholders::_2));
    };

    ~WiFiController(){ WiFi.removeEvent(eid); };

    /**
     * Initialize WiFi using stored configuration
     */
    void init();

    /**
     * Подключение к WiFi AP в клиентском режиме
     */
    void connect(const char *ssid=nullptr, const char *pwd=nullptr);

    /**
     * Configure and bring up esp's internal AP
     * defualt is to configure credentials from the config
     * bool force - reapply credentials even if AP is already started, exit otherwise
     */
    void setupAP(bool force=false);

    /**
      * switch WiFi modes
      */
    void setmode(WiFiMode_t mode);
};

#include "EmbUI.h"