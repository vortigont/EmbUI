/*
This file is a part of EmbUI project https://github.com/vortigont/EmbUI - a forked
version of EmbUI project https://github.com/DmytroKorniienko/EmbUI

(c) Emil Muratov, 2022
*/

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

#include <DNSServerAsync.h>
#include "ts.h"

using mdns_callback_t = std::function< void (void)>;

class EmbUI;

class WiFiController {

    enum class wifi_recon_t:uint8_t {
        none,
        sta_noip,           // station connected, but no IP obtained
        sta_good,           // station connected and got IP
        sta_reconnecting,   // station is attempting to reconnect
        sta_cooldown,       // station intentionally disabled for a grace period to allow proper AP operation
        ap_grace_disable,   // Access Point is waiting to be disabled
        ap_grace_enable,     // Access Point is waiting to be enabled
        ap_only
    };


    EmbUI *emb;
    Task _tWiFi;      // WiFi connection event handler task
    wifi_event_id_t eid;
    wifi_recon_t  wconn = {wifi_recon_t::none};    // WiFi (re)connection state

    // timer counters
    uint8_t ap_ctr={0};   // AccessPoint status counter
    uint8_t sta_ctr={0};  // Station status counter

    // WiFi events callback handler
    void _onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

    /**
     * @brief bring up mDNS service
     * 
     */
    void setup_mDns();


    /**
     * @brief periodicaly running task to handle and switch WiFi state
     * 
     */
    void _state_switcher();

    /**
      * update WiFi AP params and state
      */
    //void wifi_updateAP();

public:
    WiFiController(EmbUI *ui, bool aponly = false);
    ~WiFiController();

    /**
     * @brief mDNS init callback
     * callback is called on mDNS initialization to povide
     * proper order for service registrations
     * 
     */
    mdns_callback_t mdns_cb = nullptr;

    /**
     * @brief DNSServer provides Captive-Portal capability in AP mode
     * 
     */
    DNSServer dnssrv;

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

    /**
     * @brief set AP only mode
     * 
     */
    void aponly(bool ap);

    /**
     * @brief get ap-only status
     * 
     */
    inline bool aponly(){ return (wconn == wifi_recon_t::ap_only); };
};

#include "EmbUI.h"