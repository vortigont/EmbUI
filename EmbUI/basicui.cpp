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
    lang = embui.paramVariant(V_LANGUAGE).as<uint8_t>();

    /**
     * UI action handlers
     */ 
    // вывод BasicUI секций
    embui.action.add(A_ui_page_settings, page_system_settings);     // generate "settings" UI section
    embui.action.add(A_ui_page,  show_uipage);                      // display UI section template

    // обработка базовых настроек
    embui.action.add(A_set_sys_hostname, set_sys_hostname);             // hostname setup
#ifndef EMBUI_NOFTP
    embui.action.add(A_set_ntwrk_ftp, set_settings_ftp);           // обработка настроек FTP Client
#endif  // #ifdef EMBUI_NOFTP
    embui.action.add(A_set_sys_cfgclr, set_sys_cfgclear);               // clear sysconfig
    embui.action.add(A_set_sys_datetime, set_sys_datetime);             // set system date/time from a ISO string value
    embui.action.add(A_set_sys_language, set_language);                 // смена языка интерфейса
    embui.action.add(A_set_ntwrk_mqtt, set_settings_mqtt);              // обработка настроек MQTT
    embui.action.add(A_set_sys_reboot, set_sys_reboot);                 // ESP reboot action
    embui.action.add(A_set_sys_timeoptions, set_settings_time);         // установки даты/времени
    embui.action.add(A_set_ntwrk_wifi, set_settings_wifi);              // обработка настроек WiFi Client
    embui.action.add(A_set_ntwrk_wifiap, set_settings_wifiAP);          // обработка настроек WiFi AP
}

// dummy intro page that cimply calls for "system setup page"
void page_main(Interface *interf, const JsonObject *data, const char* action){

    interf->json_frame_interface();
    interf->json_section_manifest("BasicUI", embui.macid(), 0, "v1");       // app name/version manifest
    interf->json_section_end();

    // create menu
    interf->json_section_menu();
        menuitem_settings(interf);
    interf->json_section_end();

    interf->json_frame_flush();

    page_system_settings(interf, data, action);
}

/**
 * This code adds "Settings" section to the MENU
 * it is up to you to properly open/close Interface menu json_section
 */
void menuitem_settings(Interface *interf){
    interf->option(A_ui_page_settings, T_DICT[lang][TD::D_SETTINGS]);     // пункт меню "настройки"
}

/**
 * формирование секции "настроек",
 * вызывается либо по выбору из "меню" либо при вызове из
 * других блоков/обработчиков
 * 
 */
void page_system_settings(Interface *interf, const JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    interf->json_section_main(A_ui_page_settings, T_DICT[lang][TD::D_SETTINGS]);

    interf->select(V_LANGUAGE, lang, T_DICT[lang][TD::D_LANG], true);
        interf->option(0, "Eng");
        interf->option(1, "Rus");
    interf->json_section_end();

    interf->spacer();

    //interf->button_value(button_t::generic, A_ui_page, 0, T_GNRL_SETUP);                             // кнопка перехода в общие настройки
    interf->button_value(button_t::generic, A_ui_page, e2int(page::network), T_DICT[lang][TD::D_WiFi]);       // кнопка перехода в настройки сети
    interf->button_value(button_t::generic, A_ui_page, e2int(page::datetime), T_DICT[lang][TD::D_DATETIME]);  // кнопка перехода в настройки времени
    interf->button_value(button_t::generic, A_ui_page, e2int(page::mqtt), P_MQTT);                            // кнопка перехода в настройки MQTT

#ifndef EMBUI_NOFTP
    interf->button_value(button_t::generic, A_ui_page, e2int(page::ftp), "FTP Server");                       // кнопка перехода в настройки FTP
#endif
    interf->button_value(button_t::generic, A_ui_page, e2int(page::syssetup), T_DICT[lang][TD::D_SYSSET]);    // кнопка перехода в настройки System

    interf->spacer();

    // call for user_defined function that may add more elements to the "settings page"
    embui.action.exec(interf, nullptr, A_ui_blk_usersettings);

    interf->json_frame_flush();
}

/**
 * @brief choose UI section to display based on supplied index
 * 
 */
void show_uipage(Interface *interf, const JsonObject *data, const char* action){
    if (!interf || !data || (*data)[action].isNull()) return;  // bail out if no section specifier

    // find page enum index
    page idx = static_cast<page>((*data)[action].as<int>());

    switch (idx){
        case page::main :    // main page stub
            page_main(interf, data, NULL);
            break;
        case page::settings :   // general settings page
            page_system_settings(interf, nullptr, action);
        case page::network :    // WiFi network setup section
            page_settings_netw(interf, nullptr, action);
            break;
        case page::datetime :   // time setup section
            page_settings_time(interf, nullptr, action);
            break;
        case page::mqtt :       // MQTT setup section
            page_settings_mqtt(interf, nullptr, action);
            break;
        case page::ftp :        // FTP server setup section
            page_settings_ftp(interf, nullptr, action);
            break;
        case page::syssetup :   // system setup section
            page_settings_sys(interf, nullptr, action);
            break;
        default:;   // do not show anything
    }
}

/**
 *  BasicUI блок интерфейса настроек WiFi
 */
void page_settings_netw(Interface *interf, const JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(A_ui_page_network, T_EN_WiFi);

    // Hostname setup
    interf->json_section_hidden(A_set_sys_hostname, "Device name");
    interf->json_section_line();
    interf->comment(T_DICT[lang][TD::D_Hostname]);
    interf->constant(embui.hostname());
    interf->json_section_end(); // Line
    interf->text(V_hostname, P_EMPTY, "Redefine hostname, or clear to reset to default");
    interf->button(button_t::submit, A_set_sys_hostname, T_DICT[lang][TD::D_SAVE], P_GREEN);
    interf->json_section_end(); // Hostname setup

    // Wi-Fi Client setup block
    interf->json_section_hidden(A_set_ntwrk_wifi, T_DICT[lang][TD::D_WiFiClient]);
    interf->spacer(T_DICT[lang][TD::D_WiFiClientOpts]);
    interf->text(V_WCSSID, WiFi.SSID().c_str(), T_DICT[lang][TD::D_WiFiSSID]);
    interf->password(V_WCPASS, P_EMPTY, T_DICT[lang][TD::D_Password]);
    interf->button(button_t::submit, A_set_ntwrk_wifi, T_DICT[lang][TD::D_CONNECT], P_GRAY);
    interf->json_section_end();

    // Wi-Fi AP setup block
    interf->json_section_hidden(A_set_ntwrk_wifiap, T_DICT[lang][TD::D_WiFiAP]);
    interf->spacer(T_DICT[lang][TD::D_WiFiAPOpts]);

    interf->password(V_APpwd, embui.paramVariant(V_APpwd).as<const char*>(),  T_DICT[lang][TD::D_MSG_APProtect]);          // AP password

    interf->json_section_line();
    interf->comment("Access Point SSID (hostname)");
    interf->constant(embui.hostname());
    interf->json_section_end(); // Line

    interf->json_section_line();
    interf->checkbox(V_APonly, embui.paramVariant(V_APonly), T_DICT[lang][TD::D_APOnlyMode]);         // checkbox "AP-only mode"
    interf->comment(T_DICT[lang][TD::D_MSG_APOnly]);
    interf->json_section_end(); // Line

    interf->json_section_line();
    interf->checkbox(V_NOCaptP, embui.paramVariant(V_NOCaptP), "Disable WiFi Captive-Portal");         // checkbox "Disable Captive-portal"
    interf->comment("Do not run catch-all DNS in AP mode");
    interf->json_section_end(); // Line

    interf->button(button_t::submit, A_set_ntwrk_wifiap, T_DICT[lang][TD::D_SAVE], P_GRAY);

    interf->json_section_end(); // Wi-Fi AP

    interf->spacer();
    interf->button(button_t::submit, A_ui_page_settings, T_DICT[lang][TD::D_EXIT]);

    interf->json_frame_flush();
}

/**
 *  BasicUI блок настройки даты/времени
 */
void page_settings_time(Interface *interf, const JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(A_set_sys_timeoptions, T_DICT[lang][TD::D_DATETIME]);

    // Simple Clock display
    interf->json_section_line();
        String clk("Device date/time: "); TimeProcessor::getDateTimeString(clk);
        interf->constant(P_date, clk.c_str());
        interf->button(button_t::js, P_dtime, "Set browser's time");     // run js function that post browser's date/time to device
    interf->json_section_end(); // line

    interf->comment(T_DICT[lang][TD::D_MSG_TZSet01]);     // комментарий-описание секции

    // Current TIME Zone string from config
    interf->text(V_timezone, embui.paramVariant(V_timezone), T_DICT[lang][TD::D_MSG_TZONE]);

    // NTP servers section
    interf->json_section_line();
        interf->comment("NTP Servers");
        interf->checkbox(V_noNTPoDHCP, embui.paramVariant(V_noNTPoDHCP), "Disable NTP over DHCP");
    interf->json_section_end(); // line

    // a list of ntp servers
    interf->json_section_line();
        for (uint8_t i = 0; i <= CUSTOM_NTP_INDEX; ++i)
            interf->constant(TimeProcessor::getInstance().getserver(i));
    interf->json_section_end(); // line

    // user-defined NTP server field
    interf->text(V_userntp, embui.paramVariant(V_userntp), T_DICT[lang][TD::D_NTP_Secondary]);

    // manual date and time setup
    interf->comment(T_DICT[lang][TD::D_MSG_DATETIME]);
    interf->json_section_line();
        interf->datetime(P_datetime, P_EMPTY, P_EMPTY);   // placeholder for ISO date/time string
        interf->button_value(button_t::js, P_dtime, P_datetime, "Paste local time");  // call js function that paste browser's date into P_dtime field
    interf->json_section_end(); // line

    // send form button
    interf->button(button_t::submit, A_set_sys_timeoptions, T_DICT[lang][TD::D_SAVE], P_GRAY);

    interf->spacer();

    // exit button
    interf->button(button_t::submit, A_ui_page_settings, T_DICT[lang][TD::D_EXIT]);

    // close and send frame
    interf->json_frame_flush(); // main

    // формируем и отправляем кадр с запросом подгрузки внешнего ресурса со списком правил временных зон
    // полученные данные заместят предыдущее поле выпадающим списком с данными о всех временных зонах
    interf->json_frame(P_xload);
    interf->json_section_content();
                    //id           val                                 label    direct  URL for external data
    interf->select(V_timezone, embui.paramVariant(V_timezone), P_EMPTY, false,  "/js/tz.json");
    interf->json_section_end(); // select
    interf->json_frame_flush(); // xload

}

/**
 *  BasicUI блок интерфейса настроек MQTT
 */
void page_settings_mqtt(Interface *interf, const JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(A_set_ntwrk_mqtt, P_MQTT);

    // форма настроек MQTT
    interf->checkbox(V_mqtt_enable, embui.paramVariant(V_mqtt_enable), "Enable MQTT Client");
    interf->json_section_line();
        interf->text(V_mqtt_host, embui.paramVariant(V_mqtt_host).as<const char*>(), T_DICT[lang][TD::D_MQTT_Host]);
        interf->number(V_mqtt_port, embui.paramVariant(V_mqtt_port).as<int>(), T_DICT[lang][TD::D_MQTT_Port]);
    interf->json_section_end();

    interf->json_section_line();
        interf->text(V_mqtt_user, embui.paramVariant(V_mqtt_user).as<const char*>(), T_DICT[lang][TD::D_User]);
        interf->text(V_mqtt_pass, embui.paramVariant(V_mqtt_pass).as<const char*>(), T_DICT[lang][TD::D_Password]);
    interf->json_section_end(); // select

    interf->json_section_line();
        // comment about mqtt prefix 
        interf->comment(T_DICT[lang][TD::D_MQTT_Cmt]);
        // current MQTT prefix
        interf->constant(embui.mqttPrefix().c_str());
    interf->json_section_end();

    int t = embui.paramVariant(V_mqtt_ka);
    if (!t) t = 30;     // default mqtt interval

    interf->json_section_line();
        // mqtt prefix
        interf->text(V_mqtt_topic, embui.paramVariant(V_mqtt_topic).as<const char*>(), T_DICT[lang][TD::D_MQTT_Topic]);
        interf->number(V_mqtt_ka, t, T_DICT[lang][TD::D_MQTT_Interval]);
    interf->json_section_end();

    interf->button(button_t::submit, A_set_ntwrk_mqtt, T_DICT[lang][TD::D_SAVE]);

    interf->spacer();
    interf->button(button_t::generic, A_ui_page_settings, T_DICT[lang][TD::D_EXIT]);

    interf->json_frame_flush();
}

/**
 *  BasicUI блок настройки system
 */
void page_settings_sys(Interface *interf, const JsonObject *data, const char* action){
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

    interf->button(button_t::generic, A_set_sys_cfgclr, "Clear sys config", P_RED);

    interf->button(button_t::generic, A_set_sys_reboot, T_DICT[lang][TD::D_REBOOT], P_RED);

    interf->spacer();

    // exit button
    interf->button(button_t::generic, A_ui_page_settings, T_DICT[lang][TD::D_EXIT]);

    interf->json_frame_flush(); // main
}

/**
 * WiFi Client settings handler
 */
void set_settings_wifi(Interface *interf, const JsonObject *data, const char* action){
    if (!data) return;

    embui.var_remove(V_APonly);              // remove "force AP mode" parameter when attempting connection to external AP
    embui.wifi->connect((*data)[V_WCSSID].as<const char*>(), (*data)[V_WCPASS].as<const char*>());

    page_system_settings(interf, nullptr, NULL);           // display "settings" page
}

/**
 * Обработчик настроек WiFi в режиме AP
 */
void set_settings_wifiAP(Interface *interf, const JsonObject *data, const char* action){
    if (!data) return;

    embui.var_dropnulls(V_APonly, (*data)[V_APonly]);     // AP-Only chkbx
    embui.var_dropnulls(V_APpwd, (*data)[V_APpwd]);       // AP password
    embui.var_dropnulls(V_NOCaptP, (*data)[V_NOCaptP]);                 // captive portal chkbx

    embui.wifi->aponly((*data)[V_APonly]);
    //embui.wifi->setupAP(true);        // no need to apply settings now?

    if (interf) page_system_settings(interf, nullptr, NULL);                // go to "Options" page
}

/**
 * Обработчик настроек MQTT
 */
void set_settings_mqtt(Interface *interf, const JsonObject *data, const char* action){
    if (!data) return;
    // сохраняем настройки в конфиг
    embui.var_dropnulls(V_mqtt_enable, (*data)[V_mqtt_enable]);
    embui.var_dropnulls(V_mqtt_host, (*data)[V_mqtt_host]);
    embui.var_dropnulls(V_mqtt_port, (*data)[V_mqtt_port]);
    embui.var_dropnulls(V_mqtt_user, (*data)[V_mqtt_user]);
    embui.var_dropnulls(V_mqtt_pass, (*data)[V_mqtt_pass]);
    embui.var_dropnulls(V_mqtt_topic, (*data)[V_mqtt_topic]);
    embui.var_dropnulls(V_mqtt_ka, (*data)[V_mqtt_ka]);
    embui.save();

    // reconnect/disconnect MQTT
    if ((*data)[V_mqtt_enable])
        embui.mqttStart();
    else
        embui.mqttStop();

    if (interf) page_system_settings(interf, nullptr, NULL);
}

/**
 * Обработчик настроек даты/времени
 */
void set_settings_time(Interface *interf, const JsonObject *data, const char* action){
    if (!data) return;

    // save and apply timezone
    if ((*data)[V_timezone]) {
        embui.var(V_timezone, (*data)[V_timezone]);
        std::string_view tzrule((*data)[V_timezone].as<const char*>());
        TimeProcessor::getInstance().tzsetup(tzrule.substr(4).data());   // cutoff '000_' prefix
    }

    embui.var_dropnulls(V_userntp, (*data)[V_userntp]);
    TimeProcessor::getInstance().setcustomntp((*data)[V_userntp]);
    embui.var_dropnulls(V_noNTPoDHCP, (*data)[V_noNTPoDHCP]);
    TimeProcessor::getInstance().ntpodhcp(!(*data)[V_noNTPoDHCP]);

    // if there is a field with custom ISO date/time, call time setter
    if ((*data)[P_datetime])
        set_sys_datetime(nullptr, data, NULL);

    if (interf) page_settings_time(interf, nullptr, NULL);   // refresh same page
}

/**
 * @brief set system date/time from ISO string
 * data obtained through "time":"YYYY-MM-DDThh:mm:ss" param
 */
void set_sys_datetime(Interface *interf, const JsonObject *data, const char* action){
    if (!data) return;
    TimeProcessor::getInstance().setTime((*data)[P_datetime].as<const char*>());
    if (interf)
        page_settings_time(interf, nullptr, NULL);
}

void set_language(Interface *interf, const JsonObject *data, const char* action){
    if (!data) return;

    embui.var_dropnulls(V_LANGUAGE, (*data)[V_LANGUAGE]);
    lang = (*data)[V_LANGUAGE];
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

void set_sys_reboot(Interface *interf, const JsonObject *data, const char* action){
    Task *t = new Task(TASK_SECOND*5, TASK_ONCE, nullptr, &ts, false, nullptr, [](){ LOG(println, "Rebooting..."); ESP.restart(); });
    t->enableDelayed();
    if(interf){
        page_settings_sys(interf, nullptr, NULL);
    }
}

/**
 * @brief set system hostname
 * if empty/missing param provided, than use autogenerated hostname
 */
void set_sys_hostname(Interface *interf, const JsonObject *data, const char* action){
    if (!data) return;

    embui.hostname((*data)[V_hostname].as<const char*>());
    page_system_settings(interf, data, NULL);           // переходим в раздел "настройки"
}

/**
 * @brief clears EmbUI config
 */
void set_sys_cfgclear(Interface *interf, const JsonObject *data, const char* action){
    embui.cfgclear();
    if (interf) page_system_settings(interf, nullptr, NULL);
}

}   // end of "namespace basicui"
