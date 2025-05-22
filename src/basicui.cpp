#include "basicui.h"
#include "ftpsrv.h"
#include "EmbUI.h"
#include "nvs_handle.hpp"

uint8_t lang = 0;

namespace basicui {

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
    embui.action.add(A_sys_hostname, set_sys_hostname);             // hostname setup
    embui.action.add(A_sys_cfgclr, set_sys_cfgclear);               // clear sysconfig
    embui.action.add(A_sys_datetime, set_sys_datetime);             // set system date/time from a ISO string value
    embui.action.add(A_sys_language, set_language);                 // смена языка интерфейса
    embui.action.add(A_sys_reboot, set_sys_reboot);                 // ESP reboot action
    embui.action.add(A_sys_timeoptions, set_settings_time);         // установки даты/времени
    embui.action.add(A_sys_ntwrk_wifi, set_settings_wifi);          // обработка настроек WiFi Client
    embui.action.add(A_sys_ntwrk_wifiap, set_settings_wifiAP);      // обработка настроек WiFi AP
    embui.action.add(A_sys_ntwrk_mqtt, set_settings_mqtt);          // обработка настроек MQTT
#ifndef EMBUI_NOFTP
    embui.action.add(A_sys_ntwrk_ftp, set_settings_ftp);           // обработка настроек FTP Client
#endif  // #ifdef EMBUI_NOFTP
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

    // settings buttons
    interf->json_section_uidata();
        interf->uidata_pick("sys.settings.settings");
    interf->json_section_end();

    interf->json_section_begin(P_callback);
        JsonObject o( interf->json_object_create() );
        o[P_action] = A_sys_language;
    interf->json_section_end();

    // call for user_defined function that may add more elements to the "settings page"
    embui.action.exec(interf, {}, A_ui_blk_usersettings);
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
        interf->value(V_NOCaptP, embui.getConfig()[V_NOCaptP]);         // checkbox "Disable Captive-portal"
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
        interf->json_section_end();

        interf->json_section_content();
            String clk("Device date/time: "); TimeProcessor::getDateTimeString(clk);
            interf->constant(P_date, clk.c_str());
        interf->json_section_end();

        // replace section with NTP servers information
        interf->json_section_begin(P_ntp_servers, "NTP Servers", false, false, true);
            for (size_t i = 0; i != SNTP_MAX_SERVERS; ++i){
                String srv(P_ntp);
                srv += i;
                interf->text(srv, TimeProcessor::getInstance().getserver(i), srv);
            }
        interf->json_section_end();

        interf->json_section_begin(P_value);
            interf->value(V_timezone, embui.getConfig()[V_timezone]);

    interf->json_frame_flush();
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

    embui.getConfig().remove(V_APonly);             // remove "force AP mode" parameter when attempting connection to external AP
    embui.wifi->connect(data[V_WCSSID].as<const char*>(), data[V_WCPASS].as<const char*>());

    page_system_settings(interf, {});               // display "settings" page
    embui.autosave();
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
    embui.autosave();
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
    embui.autosave();
}

/**
 * Обработчик настроек даты/времени
 */
void set_settings_time(Interface *interf, JsonObjectConst data, const char* action){
    // if no data supplied, consider it as request for data
    if (data.isNull()){
        if (!interf) return;    // todo: create new iface object

        interf->json_frame_value();
            interf->value(V_timezone, embui.getConfig()[V_timezone]);
        interf->json_frame_flush();
        return;
    }

    // save and apply timezone
    if (data[V_timezone]) {
        //LOGE("basicui", printf, "recv timezone:%s\n", data[V_timezone].as<const char*>());
        embui.getConfig()[V_timezone] = data[V_timezone];
        if (embui.getConfig()[V_timezone].is<const char*>()){
            LOGD("basicui", printf, "Set timezone:%s\n", embui.getConfig()[V_timezone].as<const char*>());
        }

        std::string_view tzrule(data[V_timezone].as<const char*>());
        TimeProcessor::getInstance().tzsetup(tzrule.substr(4).data());   // cutoff '000_' prefix
    }

    if (data[V_userntp])
        embui.getConfig()[V_userntp] = data[V_userntp];
    else
        embui.getConfig().remove(V_userntp);

    // open NVS storage
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(P_EmbUI, NVS_READWRITE, &err);

    if (err == ESP_OK) {
        for (size_t i = 0; i != SNTP_MAX_SERVERS; ++i){
            String srv(P_ntp);
            srv += i;

            if (data[srv].is<const char*>()){
                std::string_view s(data[srv].as<const char*>());
                // save only non-empy servers
                if (s.length() && s.compare("0.0.0.0") != 0){
                    handle->set_string(srv.c_str(), data[srv].as<const char*>());
                    LOGD("basicui", printf, "save ntp%u:%s\n", i, data[srv].as<const char*>());
                }
            }
        }
        // apply NVS changes
        TimeProcessor::getInstance().setNTPservers();
    }

#if LWIP_DHCP_GET_NTP_SRV
    if (data[V_noNTPoDHCP]){
        embui.getConfig()[V_noNTPoDHCP] = true;
    }
    else
        embui.getConfig().remove(V_noNTPoDHCP);

    embui.wifi->ntpodhcp(!data[V_noNTPoDHCP]);
#endif

    // if there is a field with custom ISO date/time, call time setter
    if (data[P_datetime])
        set_sys_datetime(nullptr, data, NULL);

    embui.autosave();
    if (interf) page_settings_time(interf, {});   // refresh same page
}

/**
 * @brief set system date/time from ISO string
 * data obtained through "time":"YYYY-MM-DDThh:mm:ss" param
 */
void set_sys_datetime(Interface *interf, JsonObjectConst data, const char* action){
    if (!data) return;
    TimeProcessor::getInstance().setTime(data[A_sys_datetime].as<const char*>());
    if (interf)
        page_settings_time(interf, {});
}

void set_language(Interface *interf, JsonObjectConst data, const char* action){
    JsonVariantConst lang = data[A_sys_language];

    // if variant exists then it's SET action, otherwise GET
    if (lang.is<const char*>()){
        embui.setLang(lang);
        embui.publish_language(interf);
        page_system_settings(interf, {});
    } else {
        interf->json_frame_value();
        interf->value(A_sys_language, embui.getLang());
        interf->json_frame_flush();
    }
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

    //LOGI("DBG", printf, "sync:%u, sr1:%s st1:%u\n", esp_sntp_enabled(), esp_sntp_getservername(0), esp_sntp_getreachability(0));
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
