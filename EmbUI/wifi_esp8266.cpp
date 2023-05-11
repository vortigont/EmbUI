// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#ifdef ESP8266

#include "EmbUI.h"
#include "wi-fi.h"
#include "user_interface.h"

void EmbUI::onSTAConnected(WiFiEventStationModeConnected ipInfo)
{
    LOG(printf_P, PSTR("UI WiFi: STA connected - SSID:'%s'"), ipInfo.ssid.c_str());
    if(_cb_STAConnected)
        _cb_STAConnected();        // execule callback
}

void EmbUI::onSTAGotIP(WiFiEventStationModeGotIP ipInfo)
{
    sysData.wifi_sta = true;
    embuischedw.disable();
    LOG(printf_P, PSTR(", IP: %s\n"), ipInfo.ip.toString().c_str());
    timeProcessor.onSTAGotIP(ipInfo);
    if(_cb_STAGotIP)
        _cb_STAGotIP();        // execule callback

    setup_mDns();

    // shutdown AP after timeout
    embuischedw.set(WIFI_CONNECT_TIMEOUT * TASK_SECOND, TASK_ONCE, [](){
        if(WiFi.getMode() == WIFI_STA)
            return;

        WiFi.enableAP(false);
        LOG(println, F("UI WiFi: AP mode disabled"));
    });
    embuischedw.restartDelayed();
}

void EmbUI::onSTADisconnected(WiFiEventStationModeDisconnected event_info)
{
    //https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiType.h
    LOG(printf_P, PSTR("UI WiFi: Disconnected from SSID: %s, reason: %d\n"), event_info.ssid.c_str(), event_info.reason);
    sysData.wifi_sta = false;       // to be removed and replaced with API-method

    if (embuischedw.isEnabled())
        return;

    if(paramVariant(FPSTR(P_APonly)))
        return;

    /*
      esp8266 сильно тормозит в комбинированном режиме AP-STA при постоянных попытках реконнекта, WEB-интерфейс становится
      неотзывчивым, сложно изменять настройки.
      В качестве решения переключаем контроллер в режим AP-only после WIFI_CONNECT_TIMEOUT таймаута на попытку переподключения.
      Далее делаем периодические попытки переподключений каждые WIFI_RECONNECT_TIMER секунд
    */
    embuischedw.set(WIFI_SET_AP_AFTER_DISCONNECT_TIMEOUT * TASK_SECOND, TASK_ONCE, [this](){
        wifi_setAP();
        WiFi.enableSTA(false);
        Task *t = new Task(WIFI_RECONNECT_TIMER * TASK_SECOND, TASK_ONCE,
                [this](){ embuischedw.disable();
                WiFi.enableSTA(true);
                WiFi.begin(); },
                &ts, false, nullptr, nullptr, true
            );
        t->enableDelayed();
    } );

    embuischedw.restartDelayed();

    timeProcessor.onSTADisconnected(event_info);
    if(_cb_STADisconnected)
        _cb_STADisconnected();        // execute callback
}

void EmbUI::onWiFiMode(WiFiEventModeChange event_info){
/*
    if(WiFi.getMode() == WIFI_AP){
        setup_mDns();
    }
*/
}

void EmbUI::wifi_init(){
    LOG(print, F("UI WiFi: start in "));
    if (paramVariant(FPSTR(P_APonly))){
        LOG(println, F("AP-only mode"));
        wifi_setAP(true);
        return;
    }

    LOG(println, F("STA mode"));
    WiFi.enableSTA(true);

    WiFi.hostname(hostname());
    WiFi.begin();   // use internaly stored last known credentials for connection
    LOG(println, F("UI WiFi: STA reconecting..."));
}

/**
 * формирует chipid из MAC-адреса вида 'ddeeff'
 */
void EmbUI::getmacid(){
    uint8_t _mac[6];
    WiFi.softAPmacAddress(_mac);

    sprintf_P(mc, PSTR("%02X%02X%02X"), _mac[3], _mac[4], _mac[5]);
    LOG(printf_P,PSTR("UI ID:%02X%02X%02X\n"), _mac[3], _mac[4], _mac[5]);
}


#endif  // ESP8266
