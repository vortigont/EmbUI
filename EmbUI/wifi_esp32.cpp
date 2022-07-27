// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#ifdef ESP32

#include "EmbUI.h"
#include "wi-fi.h"

union MacID
{
    uint64_t u64;
    uint8_t mc[8];
};

void EmbUI::wifi_init(){
    LOG(print, F("UI WiFi: start in "));
    if (paramVariant(FPSTR(P_APonly))){
        LOG(println, F("AP-only mode"));
        wifi_setAP(true);
        return;
    }

    LOG(println, F("STA mode"));

#ifdef ESP_ARDUINO_VERSION
    WiFi.setHostname(hostname());
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
                        wifi_setAP();
                        WiFi.enableSTA(true);
                        LOG(println, F("UI WiFi: Switch to AP/STA mode"));}
        );
        tWiFi.restartDelayed();
    }

#ifdef ESP_ARDUINO_VERSION
    // 2.x core does not have this issue
#else
    if (!WiFi.setHostname(hostname())){ LOG(println, F("UI WiFi: Failed to set hostname :(")); }
#endif
    LOG(println, F("UI WiFi: STA reconecting..."));
}

void EmbUI::WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
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

        if(_cb_STAConnected)
            _cb_STAConnected();        // execule callback
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

        sysData.wifi_sta = true;
        setup_mDns();
        if(_cb_STAGotIP)
            _cb_STAGotIP();        // execule callback
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        #ifdef ESP_ARDUINO_VERSION
                LOG(printf_P, PSTR("UI WiFi: Disconnected, reason: %d\n"), info.wifi_sta_disconnected.reason);  // PIO's ARDUINO=10812    Core >=2.0.0
        #else
                LOG(printf_P, PSTR("UI WiFi: Disconnected, reason: %d\n"), info.disconnected.reason);           // PIO's ARDUINO=10805    Core <=1.0.6
        #endif
        
        // https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/esp32/esp_wifi_types.h
        if(WiFi.getMode() != WIFI_MODE_APSTA && !tWiFi.isEnabled()){
            LOG(println, PSTR("UI WiFi: Reconnect attempt"));
            tWiFi.set(WIFI_CONNECT_TIMEOUT * TASK_SECOND, TASK_ONCE, [this](){
                        LOG(println, F("UI WiFi: Switch to AP/STA mode"));
                        WiFi.enableAP(true);
                        WiFi.begin();
                        } );
            tWiFi.restartDelayed();
        }

        sysData.wifi_sta = false;
        if(_cb_STADisconnected)
            _cb_STADisconnected();        // execule callback
        break;

    default:
        break;
    }
    timeProcessor.WiFiEvent(event, info);    // handle network events for timelib
}

/**
 * формирует chipid из MAC-адреса вида 'ddeeff'
 */
void EmbUI::_getmacid(){
    MacID _mac;
    _mac.u64 = ESP.getEfuseMac();

    //LOG(printf_P,PSTR("UI MAC ID:%06llX\n"), _mac.id);
    // EfuseMac LSB comes first, so need to transpose bytes
    sprintf_P(mc, PSTR("%02X%02X%02X"), _mac.mc[3], _mac.mc[4], _mac.mc[5]);
    LOG(printf_P,PSTR("UI ID:%02X%02X%02X\n"), _mac.mc[3], _mac.mc[4], _mac.mc[5]);
}

#endif  // ESP32