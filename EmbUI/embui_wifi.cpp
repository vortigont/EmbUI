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

#define WIFI_STA_CONNECT_TIMEOUT    10                      // timer for WiFi STA connection attempt 
//#define WIFI_SET_AP_AFTER_DISCONNECT_TIMEOUT    15      // time after WiFi client disconnects and before internal AP is brought up
#define WIFI_STA_COOLDOWN_TIMOUT    90                      // timer for STA connect retry
#define WIFI_AP_GRACE_PERIOD        15                      // time to delay AP enable/disable, sec
#define WIFI_BEGIN_DELAY            3                       // a timeout before initiating WiFi-Client connection
#define WIFI_PSK_MIN_LENGTH         8

// c-tor
WiFiController::WiFiController(EmbUI *ui, bool aponly) : emb(ui) {
    if (aponly) wconn = wifi_recon_t::ap_only;
    _tWiFi.set(TASK_SECOND, TASK_FOREVER, std::bind(&WiFiController::_state_switcher, this));
    ts.addTask(_tWiFi);

    // Set WiFi event handlers
    eid = WiFi.onEvent(std::bind(&WiFiController::_onWiFiEvent, this, std::placeholders::_1, std::placeholders::_2));
}

// d-tor
WiFiController::~WiFiController(){ WiFi.removeEvent(eid); };


void WiFiController::connect(const char *ssid, const char *pwd)
{
    String _ssid(ssid); String _pwd(pwd);   // I need objects to pass it to lambda
    Task *t = new Task(WIFI_BEGIN_DELAY * TASK_SECOND, TASK_ONCE,
        [_ssid, _pwd](){
            LOG(printf_P, PSTR("UI WiFi: client connecting to SSID:'%s', pwd:'%s'\n"), _ssid.c_str(), _pwd.isEmpty() ? P_empty_quotes : _pwd.c_str());
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
    if (emb->paramVariant(V_APonly)){
        emb->var_remove(V_APonly);
        emb->autosave();
    }
}


void WiFiController::setmode(WiFiMode_t mode){
    LOG(printf_P, PSTR("UI WiFi: set mode: %d\n"), mode);
    WiFi.mode(mode);
}

/*use mdns for host name resolution*/
void WiFiController::setup_mDns(){
        //MDNS.end();       // TODO - this often leads to crash, needs triage

    if (!MDNS.begin(emb->hostname())){
        LOG(println, F("UI mDNS: Error setting up responder!"));
        MDNS.end();
        return;
    }
    LOG(printf_P, PSTR("UI mDNS: responder started: %s.local\n"), emb->hostname());

    if (!MDNS.addService("http", P_tcp, 80)) { LOG(println, "mDNS failed to add tcp:80 service"); };

    if (emb->paramVariant(P_ftp))
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
    if (emb->param(V_APpwd) && strlen(emb->param(V_APpwd)) < WIFI_PSK_MIN_LENGTH)
        emb->var_remove(V_APpwd);

    LOG(printf_P, PSTR("UI WiFi: set AP params to SSID:'%s', pwd:'%s'\n"), emb->hostname(), emb->paramVariant(V_APpwd) ? emb->paramVariant(V_APpwd).as<const char*>() : P_empty_quotes);

    WiFi.softAP(emb->hostname(), emb->paramVariant(V_APpwd).as<const char*>());
    if (!emb->paramVariant(V_NOCaptP))          // start DNS server in "captive portal mode"
        dnssrv.start();
}

void WiFiController::init(){
    WiFi.setHostname(emb->hostname());
    LOG(print, F("UI WiFi: start in "));
    if (wconn == wifi_recon_t::ap_only){
        LOG(println, F("AP-only mode"));
        setupAP(true);
        WiFi.enableSTA(false);
        return;
    }

    LOG(println, F("STA mode"));

    wconn = wifi_recon_t::ap_grace_enable;      // start in gracefull AP mode in case if MCU does not have any stored creds
    ap_ctr = WIFI_AP_GRACE_PERIOD;
    WiFi.begin();
    _tWiFi.enableDelayed();
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
        wconn = wifi_recon_t::sta_noip;
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

        LOG(printf, "SSID:'%s', IP: ", WiFi.SSID().c_str());  // IPAddress(info.got_ip.ip_info.ip.addr)
        LOG(println, WiFi.localIP().toString().c_str());

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

    case SYSTEM_EVENT_STA_DISCONNECTED:
        LOG(printf_P, PSTR("UI WiFi: Disconnected, reason: %d\n"), info.wifi_sta_disconnected.reason);  // PIO's ARDUINO=10812    Core >=2.0.0

        // WiFi STA has just lost the conection => enable internal AP after grace period
        if(wconn == wifi_recon_t::sta_good){
            wconn = wifi_recon_t::ap_grace_enable;
            ap_ctr = WIFI_AP_GRACE_PERIOD;
            break;
        }

    default:
        LOG(printf_P, PSTR("UI WiFi event: %d\n"), event);
        break;
    }
}


void WiFiController::_state_switcher(){

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
                LOG(println, F("UI WiFi: AP disabled"));
                wconn = wifi_recon_t::sta_good;
                ap_ctr = WIFI_AP_GRACE_PERIOD;
            }
        }
        break;

    case wifi_recon_t::ap_grace_enable:
        if (ap_ctr){
            if(!--ap_ctr && !(WiFi.getMode() & WIFI_MODE_AP)){
                setupAP();
                LOG(println, F("UI WiFi: AP enabled"));
                wconn = wifi_recon_t::sta_reconnecting;
                sta_ctr = WIFI_STA_CONNECT_TIMEOUT;
            }
        }
        break;

    case wifi_recon_t::sta_reconnecting:
        if(sta_ctr){
            if(!--sta_ctr){                             // disable STA mode for cooldown period
                WiFi.enableSTA(false);
                LOG(println, F("UI WiFi: STA disabled"));
                wconn = wifi_recon_t::sta_cooldown;
                sta_ctr = WIFI_STA_COOLDOWN_TIMOUT;
            }
        }
        break;

    case wifi_recon_t::sta_cooldown:
        if(sta_ctr){
            if(!--sta_ctr){                             // try to reconnect STA
                LOG(println, F("UI WiFi: STA reconnecting"));
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
