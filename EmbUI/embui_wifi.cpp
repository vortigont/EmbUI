// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "embui_wifi.hpp"

void WiFiController::connect(const char *ssid, const char *pwd)
{
    String _ssid(ssid); String _pwd(pwd);   // I need objects to pass it to lambda
    tWiFi.set(WIFI_BEGIN_DELAY * TASK_SECOND, TASK_ONCE,
        [_ssid, _pwd](){
                LOG(printf_P, PSTR("UI WiFi: client connecting to SSID:%s, pwd:%s\n"), _ssid.c_str(), _pwd.c_str());
                #ifdef ESP32
                    WiFi.disconnect();
                    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
                #endif

                _ssid.length() ? WiFi.begin(_ssid.c_str(), _pwd.c_str()) : WiFi.begin();
                ts.getCurrentTask()->disable();
        }
    );

    tWiFi.restartDelayed();
}


void WiFiController::setmode(WiFiMode_t mode){
    LOG(printf_P, PSTR("UI WiFi: set mode: %d\n"), mode);
    WiFi.mode(mode);
}

/*use mdns for host name resolution*/
void WiFiController::setup_mDns(){
        MDNS.end();

    if (!MDNS.begin(emb->hostname())){
        LOG(println, F("UI mDNS: Error setting up responder!"));
        MDNS.end();
        return;
    }

    MDNS.addService(F("http"), F("tcp"), 80);
    //MDNS.addService(F("ftp"), F("tcp"), 21);
    //MDNS.addService(F("txt"), F("udp"), 4243);
    LOG(printf_P, PSTR("UI mDNS: responder started: %s.local\n"), emb->hostname());
}

/**
 * Configure esp's internal AP
 * default is to configure credentials from the config
 * bool force - reapply credentials even if AP is already started, exit otherwise
 */
void WiFiController::setupAP(bool force){
    // check if AP is already started
    if ((bool)(WiFi.getMode() & WIFI_AP) & !force)
        return;

    // clear password if invalid 
    if (emb->param(P_APpwd) && strlen(emb->param(P_APpwd)) < WIFI_PSK_MIN_LENGTH)
        emb->var_remove(P_APpwd);

    LOG(printf_P, PSTR("UI WiFi: set AP params to SSID:%s, pwd:%s\n"), emb->hostname(), emb->paramVariant(P_APpwd).as<const char*>());

    WiFi.softAP(emb->hostname(), emb->paramVariant(P_APpwd).as<const char*>());

    // run mDNS in WiFi-AP mode
    setup_mDns();
}

/*
void WiFiController::wifi_updateAP() {
    setupAP(true);

    if (paramVariant(FPSTR(P_APonly))){
        LOG(println, F("UI WiFi: Force AP mode"));
        WiFi.enableAP(true);
        WiFi.enableSTA(false);
    }
}
*/

void WiFiController::init(){
    LOG(print, F("UI WiFi: start in "));
    if (emb->paramVariant(FPSTR(P_APonly))){
        LOG(println, F("AP-only mode"));
        setupAP(true);
        return;
    }

    LOG(println, F("STA mode"));

#ifdef ESP_ARDUINO_VERSION
    WiFi.setHostname(emb->hostname());
    WiFi.enableSTA(true);
#else
    /* this is a weird hack to mitigate DHCP hostname issue
     * order of initialization does matter, pls keep it like this till fixed in upstream
     * https://github.com/espressif/arduino-esp32/issues/2537
     */
    WiFi.enableSTA(true);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
#endif
    // use internaly stored last known credentials for connection
    if ( WiFi.begin() == WL_CONNECT_FAILED ){
        tWiFi.set(WIFI_BEGIN_DELAY * TASK_SECOND, TASK_ONCE, [this](){
                        setupAP();
                        WiFi.enableSTA(true);
                        LOG(println, F("UI WiFi: Switch to AP/STA mode"));}
        );
        tWiFi.restartDelayed();
    }

// 2.x core does not have this issue
#ifndef ESP_ARDUINO_VERSION
    if (!WiFi.setHostname(hostname())){ LOG(println, F("UI WiFi: Failed to set hostname :(")); }
#endif
    LOG(println, F("UI WiFi: STA reconecting..."));
}

void WiFiController::_onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event){
/*
    case SYSTEM_EVENT_AP_START:
        LOG(println, F("UI WiFi: Access-point started"));
        setup_mDns();
        break;
*/
    case SYSTEM_EVENT_STA_CONNECTED:
        LOG(println, F("UI WiFi: STA connected"));
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
    	/* this is a weird hack to mitigate DHCP-client hostname issue
	     * https://github.com/espressif/arduino-esp32/issues/2537
         * we use some IDF functions to restart dhcp-client, that has been disabled before STA connect
	    */
	    tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
	    tcpip_adapter_ip_info_t iface;
	    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &iface);
        if(!iface.ip.addr){
            LOG(print, F("UI WiFi: DHCP discover... "));
	        return;
    	}

        LOG(printf_P, PSTR("SSID:'%s', IP: "), WiFi.SSID().c_str());  // IPAddress(info.got_ip.ip_info.ip.addr)
        LOG(println, IPAddress(iface.ip.addr));

        tWiFi.disable();
        tWiFi.set(WIFI_CONNECT_TIMEOUT * TASK_SECOND, TASK_ONCE, [](){
            if(WiFi.getMode() == WIFI_MODE_STA)
                return;

            WiFi.enableAP(false);
            LOG(println, F("UI WiFi: AP mode disabled"));
        });
        tWiFi.restartDelayed();

        setup_mDns();
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        #ifdef ESP_ARDUINO_VERSION
                LOG(printf_P, PSTR("UI WiFi: Disconnected, reason: %d\n"), info.wifi_sta_disconnected.reason);  // PIO's ARDUINO=10812    Core >=2.0.0
        #else
                LOG(printf_P, PSTR("UI WiFi: Disconnected, reason: %d\n"), info.disconnected.reason);           // PIO's ARDUINO=10805    Core <=1.0.6
        #endif

        if (tWiFi.isEnabled())      // task activity is pending
            return;

        if(emb->paramVariant(FPSTR(P_APonly)))       // ignore STA events in AP-only mode
            return;

        // https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/esp32/esp_wifi_types.h
        if(WiFi.getMode() != WIFI_MODE_APSTA && !tWiFi.isEnabled()){
            LOG(println, PSTR("UI WiFi: Reconnect attempt"));
            tWiFi.set(WIFI_CONNECT_TIMEOUT * TASK_SECOND, TASK_ONCE,
                [this](){
                    LOG(println, F("UI WiFi: Switch to STA mode"));
                    WiFi.enableSTA(true);
                    WiFi.enableAP(true);
                    //WiFi.begin();

                    // create postponed task to reenable client connection attempt
                    Task *t = new Task(WIFI_RECONNECT_TIMER * TASK_SECOND, TASK_ONCE,
                            [this](){
                                tWiFi.disable();
                                WiFi.enableSTA(true);
                                WiFi.begin();
                            },
                            &ts, false, nullptr,
                            task_delete
                        );
                    t->enableDelayed();
                } );

            tWiFi.restartDelayed();
        }
        break;

    default:
        break;
    }
}
