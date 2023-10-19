#include "basicui.h"
#include "ftpsrv.h"
#include "EmbUI.h"

uint8_t lang = 0;            // default language for text resources (english)

namespace basicui {

/**
 * register handlers for system actions and setup pages
 * 
 */
void register_handlers(){
    // variable for UI language (specific to basic UI translations)
    lang = embui.paramVariant(P_LANGUAGE).as<uint8_t>();

    /**
     * UI action handlers
     */ 
    // вывод BasicUI секций
    embui.action.add(T_SETTINGS, page_system_settings);    // generate "settings" UI section
    embui.action.add(T_SH_SECT,  show_section);              // display UI section template

    // обработка базовых настроек
    embui.action.add(T_SET_HOSTNAME, set_hostname);          // hostname setup
#ifndef EMBUI_NOFTP
    embui.action.add(T_SET_FTP, set_settings_ftp);           // обработка настроек FTP Client
#endif  // #ifdef EMBUI_NOFTP
    embui.action.add(T_SET_WIFI, set_settings_wifi);         // обработка настроек WiFi Client
    embui.action.add(T_SET_WIFIAP, set_settings_wifiAP);     // обработка настроек WiFi AP
    embui.action.add(T_SET_MQTT, set_settings_mqtt);         // обработка настроек MQTT
    embui.action.add(T_SET_TIME, set_settings_time);         // установки даты/времени
    embui.action.add(P_LANGUAGE, set_language);              // смена языка интерфейса
    embui.action.add(T_REBOOT, set_reboot);                  // ESP reboot action
    embui.action.add(P_time, set_datetime);                  // set system date/time from a ISO string value
    embui.action.add(T_SET_CFGCLEAR, set_cfgclear);          // clear sysconfig
}

void page_main(Interface *interf, JsonObject *data, const char* action){

    interf->json_frame_interface();
    interf->json_section_manifest("BasicUI", 0, "v1");       // app name/version manifest
    interf->json_section_end();

    // create menu
    interf->json_section_menu();
        menuitem_settings(interf, data, action);
    interf->json_section_end();

    interf->json_frame_flush();

    page_system_settings(interf, data, action);
}

/**
 * This code adds "Settings" section to the MENU
 * it is up to you to properly open/close Interface menu json_section
 */
void menuitem_settings(Interface *interf, JsonObject *data, const char* action){
    interf->option(T_SETTINGS, T_DICT[lang][TD::D_SETTINGS]);     // пункт меню "настройки"
}

/**
 * формирование секции "настроек",
 * вызывается либо по выбору из "меню" либо при вызове из
 * других блоков/обработчиков
 * 
 */
void page_system_settings(Interface *interf, JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    interf->json_section_main(T_SETTINGS, T_DICT[lang][TD::D_SETTINGS]);

    interf->select(P_LANGUAGE, String(lang), String(T_DICT[lang][TD::D_LANG]), true);
        interf->option(0, "Eng");
        interf->option(1, "Rus");
    interf->json_section_end();

    interf->spacer();

    //interf->button_value(button_t::generic, T_SH_SECT, 0, T_GNRL_SETUP);                  // кнопка перехода в общие настройки
    interf->button_value(button_t::generic, T_SH_SECT, 1, T_DICT[lang][TD::D_WiFi]);        // кнопка перехода в настройки сети
    interf->button_value(button_t::generic, T_SH_SECT, 2, T_DICT[lang][TD::D_DATETIME]);    // кнопка перехода в настройки времени
    interf->button_value(button_t::generic, T_SH_SECT, 3, P_MQTT);                          // кнопка перехода в настройки MQTT

#ifndef EMBUI_NOFTP
    interf->button_value(button_t::generic, T_SH_SECT, 4, "FTP Server");           // кнопка перехода в настройки FTP
#endif
    interf->button_value(button_t::generic, T_SH_SECT, 0, T_DICT[lang][TD::D_SYSSET]);      // кнопка перехода в настройки System

    interf->spacer();

    // call for user_defined function that may add more elements to the "settings page"
    embui.action.exec(interf, nullptr, A_block_usr_settings);

    interf->json_frame_flush();
}

/**
 * @brief choose UI section to display based on supplied index
 * 
 */
void show_section(Interface *interf, JsonObject *data, const char* action){
    if (!interf || !data || (*data)[T_SH_SECT].isNull()) return;  // bail out if no section specifier

    // find section index "sh_sec"
    int idx = (*data)[T_SH_SECT];

    switch (idx){
//        case 0 :    // general setup section
//            block_settings_gnrl(interf, data, NULL);
//            break;
        case 1 :    // WiFi network setup section
            block_settings_netw(interf, nullptr, NULL);
            break;
        case 2 :    // time setup section
            block_settings_time(interf, nullptr, NULL);
            break;
        case 3 :    // MQTT setup section
            block_settings_mqtt(interf, nullptr, NULL);
            break;
        case 4 :    // FTP server setup section
            block_settings_ftp(interf, nullptr, NULL);
            break;
        default:
            block_settings_sys(interf, nullptr, NULL);
    }
}

/**
 *  BasicUI блок интерфейса настроек WiFi
 */
void block_settings_netw(Interface *interf, JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(T_OPT_NETW, T_EN_WiFi);

    // Hostname setup
    interf->json_section_hidden(T_SET_HOSTNAME, "Device name");
    interf->json_section_line();
    interf->comment(T_DICT[lang][TD::D_Hostname]);
    interf->constant(embui.hostname());
    interf->json_section_end(); // Line
    interf->text(P_hostname, P_EMPTY, "Redefine hostname, or clear to reset to default");
    interf->button(button_t::submit, T_SET_HOSTNAME, T_DICT[lang][TD::D_SAVE], P_GREEN);
    interf->json_section_end(); // Hostname setup

    // Wi-Fi Client setup block
    interf->json_section_hidden(T_SET_WIFI, T_DICT[lang][TD::D_WiFiClient]);
    interf->spacer(T_DICT[lang][TD::D_WiFiClientOpts]);
    interf->text(P_WCSSID, WiFi.SSID().c_str(), T_DICT[lang][TD::D_WiFiSSID]);
    interf->password(P_WCPASS, P_EMPTY, T_DICT[lang][TD::D_Password]);
    interf->button(button_t::submit, T_SET_WIFI, T_DICT[lang][TD::D_CONNECT], P_GRAY);
    interf->json_section_end();

    // Wi-Fi AP setup block
    interf->json_section_hidden(T_SET_WIFIAP, T_DICT[lang][TD::D_WiFiAP]);
    interf->spacer(T_DICT[lang][TD::D_WiFiAPOpts]);

    interf->password(P_APpwd, embui.paramVariant(P_APpwd).as<const char*>(),  T_DICT[lang][TD::D_MSG_APProtect]);          // AP password

    interf->json_section_line();
    interf->comment("Access Point SSID (hostname)");
    interf->constant(embui.hostname());
    interf->json_section_end(); // Line

    interf->json_section_line();
    interf->checkbox_cfg(P_APonly, T_DICT[lang][TD::D_APOnlyMode]);         // checkbox "AP-only mode"
    interf->comment(T_DICT[lang][TD::D_MSG_APOnly]);
    interf->json_section_end(); // Line

    interf->json_section_line();
    interf->checkbox_cfg(P_NOCaptP, "Disable WiFi Captive-Portal");         // checkbox "Disable Captive-portal"
    interf->comment("Do not run catch-all DNS in AP mode");
    interf->json_section_end(); // Line

    interf->button(button_t::submit, T_SET_WIFIAP, T_DICT[lang][TD::D_SAVE], P_GRAY);

    interf->json_section_end(); // Wi-Fi AP

    interf->spacer();
    interf->button(button_t::submit, T_SETTINGS, T_DICT[lang][TD::D_EXIT]);

    interf->json_frame_flush();
}

/**
 *  BasicUI блок настройки даты/времени
 */
void block_settings_time(Interface *interf, JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(T_SET_TIME, T_DICT[lang][TD::D_DATETIME]);

    // Simple Clock display
    interf->json_section_line();
    String clk("Device date/time: "); TimeProcessor::getDateTimeString(clk);
    interf->constant(P_date, clk);
    interf->button(button_t::js, P_DTIME, "Set local time");     // run js function that post browser's date/time to device
    interf->json_section_end(); // line

    interf->comment(T_DICT[lang][TD::D_MSG_TZSet01]);     // комментарий-описание секции

    // Current TIME Zone string from config
    interf->text(P_TZSET, embui.paramVariant(P_TZSET), T_DICT[lang][TD::D_MSG_TZONE]);

    // NTP servers section
    interf->json_section_line();
    interf->comment("NTP Servers");

#if ARDUINO <= 10805
    // ESP32's Arduino Core <=1.0.6 miss NTPoDHCP feature, see https://github.com/espressif/esp-idf/pull/7336
#else
    interf->checkbox_cfg(P_noNTPoDHCP, "Disable NTP over DHCP");
#endif
    interf->json_section_end(); // line

    // a list of ntp servers
    interf->json_section_line();
        for (uint8_t i = 0; i <= CUSTOM_NTP_INDEX; ++i)
            interf->constant(String(i), TimeProcessor::getInstance().getserver(i));
    interf->json_section_end(); // line

    // user-defined NTP server field
    interf->text(P_userntp, embui.paramVariant(P_userntp), T_DICT[lang][TD::D_NTP_Secondary]);

    // manual date and time setup
    interf->comment(T_DICT[lang][TD::D_MSG_DATETIME]);
    interf->json_section_line();
        interf->datetime(P_time, P_EMPTY, P_EMPTY);   // placeholder for ISO date/time string
        interf->button(button_t::js, P_DTIME, "Paste local time");  // js function that paste browser's date into P_time field
    interf->json_section_end(); // line

    // send form button
    interf->button(button_t::submit, T_SET_TIME, T_DICT[lang][TD::D_SAVE], P_GRAY);

    interf->spacer();

    // exit button
    interf->button(button_t::submit, T_SETTINGS, T_DICT[lang][TD::D_EXIT]);

    // close and send frame
    interf->json_frame_flush(); // main

    // формируем и отправляем кадр с запросом подгрузки внешнего ресурса со списком правил временных зон
    // полученные данные заместят предыдущее поле выпадающим списком с данными о всех временных зонах
    interf->json_frame("xload");
    interf->json_section_content();
                    //id           val                                 label    direct  URL for external data
    interf->select(P_TZSET, embui.paramVariant(P_TZSET), P_EMPTY, false,  "/js/tz.json");
    interf->json_section_end(); // select
    interf->json_frame_flush(); // xload

}

/**
 *  BasicUI блок интерфейса настроек MQTT
 */
void block_settings_mqtt(Interface *interf, JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(T_SET_MQTT, P_MQTT);

    // форма настроек MQTT
    interf->checkbox_cfg(P_mqtt_enable, "Enable MQTT Client");
    interf->json_section_line();
        interf->text(P_mqtt_host, embui.paramVariant(P_mqtt_host).as<const char*>(), T_DICT[lang][TD::D_MQTT_Host]);
        interf->number(P_mqtt_port, embui.paramVariant(P_mqtt_port).as<int>(), T_DICT[lang][TD::D_MQTT_Port]);
    interf->json_section_end();

    interf->json_section_line();
        interf->text(P_mqtt_user, embui.paramVariant(P_mqtt_user).as<const char*>(), T_DICT[lang][TD::D_User]);
        interf->text(P_mqtt_pass, embui.paramVariant(P_mqtt_pass).as<const char*>(), T_DICT[lang][TD::D_Password]);
    interf->json_section_end(); // select

    interf->json_section_line();
        // comment about mqtt prefix 
        interf->comment(T_DICT[lang][TD::D_MQTT_Cmt]);
        // current MQTT prefix
        interf->constant(P_EMPTY, embui.mqttPrefix().c_str());
    interf->json_section_end();

    int t = embui.paramVariant(P_mqtt_ka);
    if (!t) t = 30;     // default mqtt interval

    interf->json_section_line();
        // mqtt prefix
        interf->text(P_mqtt_topic, embui.paramVariant(P_mqtt_topic).as<const char*>(), T_DICT[lang][TD::D_MQTT_Topic]);
        interf->number(P_mqtt_ka, t, T_DICT[lang][TD::D_MQTT_Interval]);
    interf->json_section_end();

    interf->button(button_t::submit, T_SET_MQTT, T_DICT[lang][TD::D_SAVE]);

    interf->spacer();
    interf->button(button_t::generic, T_SETTINGS, T_DICT[lang][TD::D_EXIT]);

    interf->json_frame_flush();
}

/**
 *  BasicUI блок настройки system
 */
void block_settings_sys(Interface *interf, JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main("sys", T_DICT[lang][TD::D_SYSSET]);

    // FW update
    interf->json_section_hidden(T_DO_OTAUPD, T_DICT[lang][TD::D_UPDATEFW]);
    interf->spacer(T_DICT[lang][TD::D_FWLOAD]);
    interf->file_form(T_DO_OTAUPD, T_DO_OTAUPD, T_DICT[lang][TD::D_UPLOADFW], "fw");
    interf->file_form(T_DO_OTAUPD, T_DO_OTAUPD, T_DICT[lang][TD::D_UPLOADFS], "fs");
    interf->json_section_end();

    interf->button(button_t::generic, T_SET_CFGCLEAR, "Clear sys config", P_RED);

    interf->button(button_t::generic, T_REBOOT, T_DICT[lang][TD::D_REBOOT], P_RED);

    interf->spacer();

    // exit button
    interf->button(button_t::generic, T_SETTINGS, T_DICT[lang][TD::D_EXIT]);

    interf->json_frame_flush(); // main
}

/**
 * WiFi Client settings handler
 */
void set_settings_wifi(Interface *interf, JsonObject *data, const char* action){
    if (!data) return;

    embui.var_remove(P_APonly);              // remove "force AP mode" parameter when attempting connection to external AP
    embui.wifi->connect((*data)[P_WCSSID].as<const char*>(), (*data)[P_WCPASS].as<const char*>());

    page_system_settings(interf, nullptr, NULL);           // display "settings" page
}

/**
 * Обработчик настроек WiFi в режиме AP
 */
void set_settings_wifiAP(Interface *interf, JsonObject *data, const char* action){
    if (!data) return;

    embui.var_dropnulls(P_APonly, (*data)[P_APonly]);     // AP-Only chkbx
    embui.var_dropnulls(P_APpwd, (*data)[P_APpwd]);       // AP password
    embui.var_dropnulls(P_NOCaptP, (*data)[P_NOCaptP]);                 // captive portal chkbx

    embui.wifi->aponly((*data)[P_APonly]);
    //embui.wifi->setupAP(true);        // no need to apply settings now?

    if (interf) page_system_settings(interf, nullptr, NULL);                // go to "Options" page
}

/**
 * Обработчик настроек MQTT
 */
void set_settings_mqtt(Interface *interf, JsonObject *data, const char* action){
    if (!data) return;
    // сохраняем настройки в конфиг
    embui.var_dropnulls(P_mqtt_enable, (*data)[P_mqtt_enable]);
    embui.var_dropnulls(P_mqtt_host, (*data)[P_mqtt_host]);
    embui.var_dropnulls(P_mqtt_port, (*data)[P_mqtt_port]);
    embui.var_dropnulls(P_mqtt_user, (*data)[P_mqtt_user]);
    embui.var_dropnulls(P_mqtt_pass, (*data)[P_mqtt_pass]);
    embui.var_dropnulls(P_mqtt_topic, (*data)[P_mqtt_topic]);
    embui.var_dropnulls(P_mqtt_ka, (*data)[P_mqtt_ka]);
    embui.save();

    // reconnect/disconnect MQTT
    if ((*data)[P_mqtt_enable])
        embui.mqttStart();
    else
        embui.mqttStop();

    page_system_settings(interf, data, NULL);
}

/**
 * Обработчик настроек даты/времени
 */
void set_settings_time(Interface *interf, JsonObject *data, const char* action){
    if (!data) return;

    // Save and apply timezone rules
    SETPARAM_NONULL(P_TZSET)

    String tzrule = (*data)[P_TZSET];
    if (!tzrule.isEmpty()){
        TimeProcessor::getInstance().tzsetup(tzrule.substring(4).c_str());   // cutoff '000_' prefix key
    }

    SETPARAM_NONULL(P_userntp, TimeProcessor::getInstance().setcustomntp((*data)[P_userntp]));

#if ARDUINO <= 10805
    // ESP32's Arduino Core <=1.0.6 miss NTPoDHCP feature
#else
    SETPARAM_NONULL( P_noNTPoDHCP, TimeProcessor::getInstance().ntpodhcp(!(*data)[P_noNTPoDHCP]) )
#endif

    // if there is a field with custom ISO date/time, call time setter
    if ((*data)[P_time])
        set_datetime(nullptr, data, NULL);

    page_system_settings(interf, data, NULL);   // redirect to 'settings' page
}

/**
 * @brief set system date/time from ISO string
 * data obtained through "time":"YYYY-MM-DDThh:mm:ss" param
 */
void set_datetime(Interface *interf, JsonObject *data, const char* action){
    if (!data) return;
    TimeProcessor::getInstance().setTime((*data)[P_time].as<String>());
    if (interf)
        block_settings_time(interf, nullptr, NULL);
}

void set_language(Interface *interf, JsonObject *data, const char* action){
    if (!data) return;

    embui.var_dropnulls(P_LANGUAGE, (*data)[P_LANGUAGE]);
    lang = (*data)[P_LANGUAGE];
    page_system_settings(interf, data, NULL);
}

void embuistatus(Interface *interf){
    interf->json_frame_value();

    // system time
    interf->value(P_pTime, TimeProcessor::getInstance().getFormattedShortTime(), true);

    // memory
    char buff[20];
    if(psramFound())
        std::snprintf(buff, 20, "%uk/%uk", ESP.getFreeHeap()/1024, ESP.getFreePsram()/1024);
    else
        std::snprintf(buff, 20, "%ok", ESP.getFreeHeap()/1024);

    interf->value(P_pMem, buff, true);

    // uptime
    uint32_t seconds = esp_timer_get_time() / 1000000;
    std::snprintf(buff, 20, "%ud%02u:%02u:%02u", seconds/86400, (seconds/3600)%24, (seconds/60)%60, seconds%60);
    interf->value(P_pUptime, buff, true);

    // RSSI
    auto rssi = WiFi.RSSI();
    std::snprintf(buff, 20, "%u%% (%ddBm)", constrain(map(rssi, -85, -40, 0, 100),0,100), rssi);
    interf->value(P_pRSSI, buff, true);

    interf->json_frame_flush();
}

void set_reboot(Interface *interf, JsonObject *data, const char* action){
    Task *t = new Task(TASK_SECOND*5, TASK_ONCE, nullptr, &ts, false, nullptr, [](){ LOG(println, "Rebooting..."); ESP.restart(); });
    t->enableDelayed();
    if(interf){
        interf->json_frame_interface();
        block_settings_sys(interf, nullptr, NULL);
        interf->json_frame_flush();
    }
}

/**
 * @brief set system hostname
 * if empty/missing param provided, than use autogenerated hostname
 */
void set_hostname(Interface *interf, JsonObject *data, const char* action){
    if (!data) return;

    embui.hostname((*data)[P_hostname].as<const char*>());
    page_system_settings(interf, data, NULL);           // переходим в раздел "настройки"
}

/**
 * @brief clears EmbUI config
 */
void set_cfgclear(Interface *interf, JsonObject *data, const char* action){
    embui.cfgclear();
    if (interf) page_system_settings(interf, nullptr, NULL);
}

}   // end of "namespace basicui"
