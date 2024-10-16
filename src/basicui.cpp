#include "basicui.h"
#include "ftpsrv.h"
#include "EmbUI.h"

uint8_t lang = 0;

namespace basicui {

static constexpr const char* JSON_i18N = "/js/ui_sys.i18n.json";
static constexpr const char* JSON_LANG_LIST = "/js/ui_sys.lang.json";

/**
 * register handlers for system actions and setup pages
 * 
 */
void register_handlers(){
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

// dummy intro page that simply calls for "system setup page"
void page_main(Interface *interf, JsonObjectConst data, const char* action){

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
void page_system_settings(Interface *interf, JsonObjectConst data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    interf->json_section_main(A_ui_page_settings, T_DICT[lang][TD::D_SETTINGS]);

    interf->json_section_xload();
        // side load drop-down list of available languages /eff_list.json file
        interf->select(A_set_sys_language, embui.getLang(), T_DICT[lang][TD::D_LANG], true, JSON_LANG_LIST);
    interf->json_section_end(); // close xload section

    interf->spacer();

    // settings buttons
    interf->json_section_uidata();
        interf->uidata_pick("sys.settings.btns");
    interf->json_section_end();

    interf->spacer();

    // call for user_defined function that may add more elements to the "settings page"
    embui.action.exec(interf, {}, A_ui_blk_usersettings);

    interf->json_frame_flush();
}

/**
 * @brief choose UI section to display based on supplied index
 * 
 */
void show_uipage(Interface *interf, JsonObjectConst data, const char* action){
    if (!interf || !data || data[action].isNull()) return;  // bail out if no section specifier

    // find page enum index
    page idx = static_cast<page>(data[action].as<int>());

    switch (idx){
        case page::main :    // main page stub
            page_main(interf, data, NULL);
            break;
        case page::settings :   // general settings page
            page_system_settings(interf, {}, action);
            break;
        case page::network :    // WiFi network setup section
            page_settings_netw(interf, {}, action);
            break;
        case page::datetime :   // time setup section
            page_settings_time(interf, {}, action);
            break;
        case page::mqtt :       // MQTT setup section
            page_settings_mqtt(interf, {}, action);
            break;
        case page::ftp :        // FTP server setup section
            page_settings_ftp(interf, {}, action);
            break;
        case page::syssetup :   // system setup section
            page_settings_sys(interf, {}, action);
            break;
        default:;   // do not show anything
    }
}

/**
 *  BasicUI блок интерфейса настроек WiFi
 */
void page_settings_netw(Interface *interf, JsonObjectConst data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();
        interf->json_section_uidata();
            interf->uidata_pick("sys.settings.network");
    interf->json_frame_flush();     // frame must be closed before content/value could be alterred on rendered page

    interf->json_frame_interface();
        interf->json_section_content();
            interf->constant(P_hostname_const, embui.hostname());       // device hostname section
            interf->constant(P_apssid, embui.hostname());               // WiFi AP-SSID
    interf->json_frame_flush();

    interf->json_frame_value();
        interf->value(V_WCSSID, WiFi.SSID());                           // connected SSID
        interf->value(V_NOCaptP, embui.getConfig()[V_NOCaptP]);        // checkbox "Disable Captive-portal"
    interf->json_frame_flush();

}

/**
 *  BasicUI блок настройки даты/времени
 */
void page_settings_time(Interface *interf, JsonObjectConst data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();
        interf->json_section_uidata();
        interf->uidata_pick("sys.settings.datetime");
    interf->json_frame_flush();

    interf->json_frame_interface();
        interf->json_section_content();
            String clk("Device date/time: "); TimeProcessor::getDateTimeString(clk);
            interf->constant(P_date, clk.c_str());
        interf->json_section_end();

        // replace section with NTP servers information
        interf->json_section_begin(P_ntp_servers, "Configured NTP Servers", false, false, true);
            for (uint8_t i = 0; i <= CUSTOM_NTP_INDEX; ++i)
                interf->constant(TimeProcessor::getInstance().getserver(i));
    interf->json_frame_flush();

    // формируем и отправляем кадр с запросом подгрузки внешнего ресурса со списком правил временных зон
    // полученные данные заместят предыдущее поле выпадающим списком с данными о всех временных зонах
    interf->json_frame(P_xload);
    interf->json_section_content();
                   //id        val                             label    direct  URL for external data
    interf->select(V_timezone, embui.getConfig()[V_timezone], P_EMPTY, false,  "/js/tz.json");
    interf->json_section_end(); // select
    interf->json_frame_flush(); // xload

}

/**
 *  BasicUI блок интерфейса настроек MQTT
 */
void page_settings_mqtt(Interface *interf, JsonObjectConst data, const char* action){
    interf->json_frame_interface();
        interf->json_section_uidata();
        interf->uidata_pick("sys.settings.mqtt");
    interf->json_frame_flush();

    interf->json_frame_interface();
        interf->json_section_content();
            interf->constant(P_MQTTTopic, embui.mqttPrefix().c_str());
    interf->json_frame_flush();

    interf->json_frame_value();
        interf->value(V_mqtt_enable, embui.getConfig()[V_mqtt_enable]);    // enable MQTT checkbox
        interf->value(V_mqtt_host, embui.getConfig()[V_mqtt_host]);        // MQTT host text field
        interf->value(V_mqtt_port, embui.getConfig()[V_mqtt_port].as<int>());        // MQTT port
        interf->value(V_mqtt_user, embui.getConfig()[V_mqtt_user].as<const char*>());        // MQTT user
        interf->value(V_mqtt_pass, embui.getConfig()[V_mqtt_pass].as<const char*>());        // MQTT passwd
        interf->value(V_mqtt_topic, embui.getConfig()[V_mqtt_topic].as<const char*>());
        int t = embui.getConfig()[V_mqtt_ka];
        if (!t){    // default mqtt interval 30
            interf->value(V_mqtt_ka, t);
        }
    interf->json_frame_flush();

}

/**
 *  BasicUI блок настройки system
 */
void page_settings_sys(Interface *interf, JsonObjectConst data, const char* action){
    interf->json_frame_interface();
        interf->json_section_uidata();
        interf->uidata_pick("sys.settings.system");
    interf->json_frame_flush();

}

/**
 * WiFi Client settings handler
 */
void set_settings_wifi(Interface *interf, JsonObjectConst data, const char* action){
    if (!data) return;

    embui.getConfig().remove(V_APonly);              // remove "force AP mode" parameter when attempting connection to external AP
    embui.wifi->connect(data[V_WCSSID].as<const char*>(), data[V_WCPASS].as<const char*>());

    page_system_settings(interf, {});           // display "settings" page
}

/**
 * Обработчик настроек WiFi в режиме AP
 */
void set_settings_wifiAP(Interface *interf, JsonObjectConst data, const char* action){
    if (!data) return;

    // captive portal chkbx
    JsonVariantConst val = data[V_NOCaptP];
    if (val.as<bool>())
        embui.getConfig()[V_NOCaptP] = val;
    else
        embui.getConfig().remove(V_NOCaptP);

    // AP password
    val = data[V_APpwd];
    if (val.is<const char*>())
        embui.getConfig()[V_APpwd] = val;
    else    
        embui.getConfig().remove(V_APpwd);

    // AP-Only chkbx
    val = data[V_APonly];
    if (val.as<bool>())
        embui.getConfig()[V_APonly] = val;
    else
        embui.getConfig().remove(V_APonly);

    // apply AP-Only configuration
    embui.wifi->aponly(val.as<bool>());

    if (interf) page_system_settings(interf, {});                // go to "Options" page
}

/**
 * Обработчик настроек MQTT
 */
void set_settings_mqtt(Interface *interf, JsonObjectConst data, const char* action){
    if (!data) return;
    // сохраняем настройки в конфиг
    if (data[V_mqtt_enable])
        embui.getConfig()[V_mqtt_enable] = true;

    if (data[V_mqtt_host])
        embui.getConfig()[V_mqtt_host] = data[V_mqtt_host];
    else
        embui.getConfig().remove(V_mqtt_host);

    if (data[V_mqtt_port])
        embui.getConfig()[V_mqtt_port] = data[V_mqtt_port];
    else
        embui.getConfig().remove(V_mqtt_port);

    if (data[V_mqtt_user])
        embui.getConfig()[V_mqtt_user] = data[V_mqtt_user];
    else
        embui.getConfig().remove(V_mqtt_user);

    if (data[V_mqtt_port])
        embui.getConfig()[V_mqtt_pass] = data[V_mqtt_pass];
    else
        embui.getConfig().remove(V_mqtt_pass);

    if (data[V_mqtt_topic])
        embui.getConfig()[V_mqtt_topic] = data[V_mqtt_topic];
    else
        embui.getConfig().remove(V_mqtt_port);

    if (data[V_mqtt_ka])
        embui.getConfig()[V_mqtt_ka] = data[V_mqtt_ka];
    else
        embui.getConfig().remove(V_mqtt_ka);

    embui.autosave();

    // reconnect/disconnect MQTT
    if (data[V_mqtt_enable])
        embui.mqttStart();
    else
        embui.mqttStop();

    if (interf) page_system_settings(interf, {});
}

/**
 * Обработчик настроек даты/времени
 */
void set_settings_time(Interface *interf, JsonObjectConst data, const char* action){
    if (!data) return;

    // save and apply timezone
    if (data[V_timezone]) {
        embui.getConfig()[V_timezone] = data[V_timezone];
        std::string_view tzrule(data[V_timezone].as<const char*>());
        TimeProcessor::getInstance().tzsetup(tzrule.substr(4).data());   // cutoff '000_' prefix
    }

    if (data[V_userntp])
        embui.getConfig()[V_userntp] = data[V_userntp];
    else
        embui.getConfig().remove(V_userntp);

    TimeProcessor::getInstance().setcustomntp(data[V_userntp]);

    if (data[V_noNTPoDHCP])
        embui.getConfig()[V_noNTPoDHCP] = true;
    else
        embui.getConfig().remove(V_noNTPoDHCP);

    TimeProcessor::getInstance().ntpodhcp(!data[V_noNTPoDHCP]);

    // if there is a field with custom ISO date/time, call time setter
    if (data[P_datetime])
        set_sys_datetime(nullptr, data, NULL);

    if (interf) page_settings_time(interf, {});   // refresh same page
}

/**
 * @brief set system date/time from ISO string
 * data obtained through "time":"YYYY-MM-DDThh:mm:ss" param
 */
void set_sys_datetime(Interface *interf, JsonObjectConst data, const char* action){
    if (!data) return;
    TimeProcessor::getInstance().setTime(data[A_set_sys_datetime].as<const char*>());
    if (interf)
        page_settings_time(interf, {});
}

void set_language(Interface *interf, JsonObjectConst data, const char* action){
    if (!data) return;

    embui.setLang(data[A_set_sys_language]);

    String path(embui.getLang());
    path += (char)0x2e; // '.'
    path += "data";
    interf->json_frame_interface();
    // load translation data for new lang
    interf->json_section_uidata();
        interf->uidata_xmerge(JSON_i18N, P_sys, path.c_str());
    interf->json_frame_flush();

    page_system_settings(interf, {});
}

void embuistatus(Interface *interf){
    interf->json_frame_value();

    // system time
    interf->value(P_pTime, TimeProcessor::getInstance().getFormattedShortTime(), true);

    // memory
    char buff[20];
    if(psramFound())
        std::snprintf(buff, 20, "%luk/%luk", ESP.getFreeHeap()/1024, ESP.getFreePsram()/1024);
    else
        std::snprintf(buff, 20, "%luk", ESP.getFreeHeap()/1024U);

    interf->value(P_pMem, buff, true);

    // uptime
    uint32_t seconds = esp_timer_get_time() / 1000000;
    std::snprintf(buff, 20, "%lud%02lu:%02lu:%02lu", seconds/86400, (seconds/3600)%24, (seconds/60)%60, seconds%60);
    interf->value(P_pUptime, buff, true);

    // RSSI
    auto rssi = WiFi.RSSI();
    std::snprintf(buff, 20, "%hu%% (%ddBm)", static_cast<uint8_t>( map(rssi, -85, -40, 0, 100) ), rssi);
    interf->value(P_pRSSI, buff, true);

    interf->json_frame_flush();
}

void set_sys_reboot(Interface *interf, JsonObjectConst data, const char* action){
    Task *t = new Task(TASK_SECOND*5, TASK_ONCE, nullptr, &ts, false, nullptr, [](){ LOG(println, "Rebooting..."); ESP.restart(); });
    t->enableDelayed();
    if(interf){
        page_settings_sys(interf, {});
    }
}

/**
 * @brief set system hostname
 * if empty/missing param provided, than use autogenerated hostname
 */
void set_sys_hostname(Interface *interf, JsonObjectConst data, const char* action){
    if (!data) return;

    embui.hostname(data[V_hostname].as<const char*>());
    page_system_settings(interf, data, NULL);           // переходим в раздел "настройки"
}

/**
 * @brief clears EmbUI config
 */
void set_sys_cfgclear(Interface *interf, JsonObjectConst data, const char* action){
    embui.cfgclear();
    if (interf) page_system_settings(interf, {});
}

}   // end of "namespace basicui"
