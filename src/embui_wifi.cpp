/*
This file is a part of EmbUI project https://github.com/vortigont/EmbUI - a forked
version of EmbUI project https://github.com/DmytroKorniienko/EmbUI

(c) Emil Muratov, 2022
*/

// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "embui_wifi.hpp"
#include "embui_log.h"

#define WIFI_STA_CONNECT_TIMEOUT    10                      // timer for WiFi STA connection attempt 
#define WIFI_STA_COOLDOWN_TIMOUT    90                      // timer for STA connect retry
#define WIFI_AP_GRACE_PERIOD        15                      // time to delay AP enable/disable, sec
#define WIFI_BEGIN_DELAY            3                       // a timeout before initiating WiFi-Client connection
#define WIFI_PSK_MIN_LENGTH         8

// c-tor
WiFiController::WiFiController(EmbUI *ui, bool aponly) : emb(ui) {
    if (aponly) wconn = wifi_recon_t::ap_only;
    _tWiFi.set( TASK_SECOND, TASK_FOREVER, [this](){ _state_switcher(); } );
    ts.addTask(_tWiFi);

    // Set WiFi event handlers
    eid = WiFi.onEvent( [this](WiFiEvent_t event, WiFiEventInfo_t info){ _onWiFiEvent(event, info); } );
    if (!eid){
        LOGE(P_EmbUI_WiFi, println, "Err registering evt handler!");
    }
}

// d-tor
WiFiController::~WiFiController(){
    WiFi.removeEvent(eid);
    ts.deleteTask(_tWiFi);
};


void WiFiController::connect(const char *ssid, const char *pwd)
{
    String _ssid(ssid); String _pwd(pwd);   // I need objects to pass it to lambda
    Task *t = new Task(WIFI_BEGIN_DELAY * TASK_SECOND, TASK_ONCE,
        [_ssid, _pwd](){
            LOGI(P_EmbUI_WiFi, printf, "client connecting to SSID:'%s', pwd:'%s'\n", _ssid.c_str(), _pwd.isEmpty() ? P_empty_quotes : _pwd.c_str());
            WiFi.disconnect();
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);

            _ssid.length() ? WiFi.begin(_ssid.c_str(), _pwd.c_str()) : WiFi.begin();
        },
        &ts, false, nullptr, nullptr, true
    );
    t->enableDelayed();
    wconn = wifi_recon_t::ap_grace_enable;
    ap_ctr = WIFI_AP_GRACE_PERIOD;
    // drop "AP-only flag from config"
    if (emb->getConfig()[V_APonly]){
        emb->getConfig().remove(V_APonly);
        emb->autosave();
    }
}


void WiFiController::setmode(WiFiMode_t mode){
    LOGI(P_EmbUI_WiFi, printf, "set mode: %d\n", mode);
    WiFi.mode(mode);
}

/*use mdns for host name resolution*/
void WiFiController::setup_mDns(){
        //MDNS.end();       // TODO - this often leads to crash, needs triage

    if (!MDNS.begin(emb->hostname())){
        LOGE(P_EmbUI_WiFi, println, "Error setting up responder!");
        MDNS.end();
        return;
    }
    LOGI(P_EmbUI_WiFi, printf, "mDNS responder: %s.local\n", emb->hostname());

    if (!MDNS.addService("http", P_tcp, 80)) { LOGE(P_EmbUI_WiFi, println, "mDNS failed to add tcp:80 service"); };

    if (emb->getConfig()[P_ftp])
        MDNS.addService(P_ftp, P_tcp, 21);

    //MDNS.addService(F("txt"), F("udp"), 4243);

    // run callback
    if (mdns_cb)
        mdns_cb();
}

/**
 * Configure esp's internal AP
 * default is to configure credentials from the config
 * bool force - reapply credentials even if AP is already started, exit otherwise
 */
void WiFiController::setupAP(bool force){
    // check if AP is already started
    if ((bool)(WiFi.getMode() & WIFI_AP) && !force)
        return;

    // clear password if invalid
    String pwd;
    if (emb->getConfig()[V_APpwd].is<const char*>()){
        pwd = emb->getConfig()[V_APpwd].as<const char*>();
        if (pwd.length() < WIFI_PSK_MIN_LENGTH)
            emb->getConfig().remove(V_APpwd);
    }

    LOGD(P_EmbUI_WiFi, printf, "set AP params to SSID:'%s', pwd:'%s'\n", emb->hostname(), pwd.c_str());

    WiFi.softAP(emb->hostname(), pwd.c_str());
    if (!emb->getConfig()[V_NOCaptP])          // start DNS server in "captive portal mode"
        dnssrv.start();
}

void WiFiController::init(){
    WiFi.setHostname(emb->hostname());
    if (wconn == wifi_recon_t::ap_only){
        LOGI(P_EmbUI_WiFi, println, "AP-only mode");
        setupAP(true);
        WiFi.enableSTA(false);
        return;
    }

    LOGI(P_EmbUI_WiFi, println, "STA mode");

    wconn = wifi_recon_t::ap_grace_enable;      // start in gracefull AP mode in case if MCU does not have any stored creds
    ap_ctr = WIFI_AP_GRACE_PERIOD;
    WiFi.begin();
    _tWiFi.enableDelayed();
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void WiFiController::_onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info)
#else
void WiFiController::_onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
#endif
{
    switch (event){
/*
    case SYSTEM_EVENT_AP_START:
        LOG(println, F("Access-point started"));
        setup_mDns();
        break;
*/

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
#else
    case SYSTEM_EVENT_STA_CONNECTED:
#endif
        LOGI(P_EmbUI_WiFi, println, "STA connected");
        wconn = wifi_recon_t::sta_noip;
        break;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
#else
    case SYSTEM_EVENT_STA_GOT_IP:
#endif

// do I still need it for IDF 4.x ???
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    	/* this is a weird hack to mitigate DHCP-client hostname issue
	     * https://github.com/espressif/arduino-esp32/issues/2537
         * we use some IDF functions to restart dhcp-client, that has been disabled before STA connect
	    */
	    tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
	    tcpip_adapter_ip_info_t iface;
	    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &iface);
        if(!iface.ip.addr){
            LOGD(P_EmbUI_WiFi, print, "DHCP discover... ");
	        return;
    	}
#endif
        LOGI(P_EmbUI_WiFi, printf, "SSID:'%s', IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

        // if we are in ap_grace_enable state (i.e. reconnecting) - cancell it
        if (wconn == wifi_recon_t::ap_grace_enable){
            wconn = wifi_recon_t::sta_good;
            ap_ctr = 0;
        }

        if(WiFi.getMode() & WIFI_MODE_AP){
            // need to disable AP after grace period
            wconn = wifi_recon_t::ap_grace_disable;
            ap_ctr = WIFI_AP_GRACE_PERIOD;
        } else {
            wconn = wifi_recon_t::sta_good;
        }
        setup_mDns();
        break;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
#else
    case SYSTEM_EVENT_STA_DISCONNECTED:
#endif
        LOGI(P_EmbUI_WiFi, printf, "Disconnected, reason: %d\n", info.wifi_sta_disconnected.reason);  // PIO's ARDUINO=10812    Core >=2.0.0

        // WiFi STA has just lost the conection => enable internal AP after grace period
        if(wconn == wifi_recon_t::sta_good){
            wconn = wifi_recon_t::ap_grace_enable;
            ap_ctr = WIFI_AP_GRACE_PERIOD;
            break;
        }
        _tWiFi.enableIfNot();
        break;
    default:
        LOGD(P_EmbUI_WiFi, printf, "event: %d\n", event);
        break;
    }
}


void WiFiController::_state_switcher(){
    //LOGV(P_EmbUI_WiFi, printf, "_state_switcher() %u\n", wconn);

    switch (wconn){
    case wifi_recon_t::ap_only:     // switch to AP-only mode
        setupAP();
        WiFi.enableSTA(false);
        _tWiFi.disable();           // no need to check for state changes in ap-only mode
        break;

    case wifi_recon_t::ap_grace_disable:
        if (ap_ctr){
            if(!--ap_ctr && (WiFi.getMode() & WIFI_MODE_STA)){
                dnssrv.stop();
                WiFi.enableAP(false);
                LOGD(P_EmbUI_WiFi, println, "AP disabled");
                wconn = wifi_recon_t::sta_good;
                ap_ctr = WIFI_AP_GRACE_PERIOD;
            }
        }
        break;

    case wifi_recon_t::ap_grace_enable:
        //LOGV(P_EmbUI_WiFi, printf, "AP grace time: %u\n", ap_ctr);
        if (ap_ctr){
            if(!--ap_ctr && !(WiFi.getMode() & WIFI_MODE_AP)){
                setupAP();
                LOGD(P_EmbUI_WiFi, println, "AP enabled");
                wconn = wifi_recon_t::sta_reconnecting;
                sta_ctr = WIFI_STA_CONNECT_TIMEOUT;
            }
        }
        break;

    case wifi_recon_t::sta_reconnecting:
        if(sta_ctr){
            if(!--sta_ctr){                             // disable STA mode for cooldown period
                WiFi.enableSTA(false);
                LOGD(P_EmbUI_WiFi, println, "STA disabled");
                wconn = wifi_recon_t::sta_cooldown;
                sta_ctr = WIFI_STA_COOLDOWN_TIMOUT;
            }
        }
        break;

    case wifi_recon_t::sta_cooldown:
        if(sta_ctr){
            if(!--sta_ctr){                             // try to reconnect STA
                LOGD(P_EmbUI_WiFi, println, "STA reconnecting");
                wconn = wifi_recon_t::sta_reconnecting;
                sta_ctr = WIFI_STA_CONNECT_TIMEOUT;
                WiFi.begin();
            }
        }
        break;

    default:
        break;
    }
}


void WiFiController::aponly(bool ap){
    if (ap){
        wconn = wifi_recon_t::ap_only;
        _tWiFi.enableDelayed(WIFI_BEGIN_DELAY * TASK_SECOND);
    } else if (wconn != wifi_recon_t::sta_good) {
        wconn = wifi_recon_t::ap_grace_disable;
        ap_ctr = WIFI_AP_GRACE_PERIOD;
        _tWiFi.enableDelayed();
        WiFi.begin();
    }
}
