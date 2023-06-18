#include "basicui.h"
#include "ftpsrv.h"

uint8_t lang = 0;            // default language for text resources (english)

namespace basicui {

/**
 * Define configuration variables and controls handlers
 * 
 * Variables has literal names and are kept within json-configuration file on flash
 * Control handlers are bound by literal name with a particular method. This method is invoked
 * by manipulating controls
 * 
 */
void add_sections(){
    LOG(println, F("UI: Creating webui vars"));

    // variable for UI language (specific to basic UI translations)
    lang = embui.paramVariant(FPSTR(P_LANGUAGE)).as<uint8_t>();

    /**
     * UI action handlers
     */ 
    // вывод BasicUI секций
    embui.section_handle_add(FPSTR(T_SETTINGS), section_settings_frame);    // generate "settings" UI section
    embui.section_handle_add(FPSTR(T_SH_SECT),  show_section);              // display UI section template

    // обработка базовых настроек
    embui.section_handle_add(FPSTR(T_SET_HOSTNAME), set_hostname);          // hostname setup
#ifndef EMBUI_NOFTP
    embui.section_handle_add(FPSTR(T_SET_FTP), set_settings_ftp);           // обработка настроек FTP Client
#endif  // #ifdef EMBUI_NOFTP
    embui.section_handle_add(FPSTR(T_SET_WIFI), set_settings_wifi);         // обработка настроек WiFi Client
    embui.section_handle_add(FPSTR(T_SET_WIFIAP), set_settings_wifiAP);     // обработка настроек WiFi AP
#ifdef EMBUI_MQTT
    embui.section_handle_add(FPSTR(T_SET_MQTT), set_settings_mqtt);         // обработка настроек MQTT
#endif  // #ifdef EMBUI_MQTT
    embui.section_handle_add(FPSTR(T_SET_TIME), set_settings_time);         // установки даты/времени
    embui.section_handle_add(FPSTR(P_LANGUAGE), set_language);              // смена языка интерфейса
    embui.section_handle_add(FPSTR(T_REBOOT), set_reboot);                  // ESP reboot action
    embui.section_handle_add(FPSTR(P_time), set_datetime);                  // set system date/time from a ISO string value
    embui.section_handle_add(FPSTR(T_SET_CFGCLEAR), set_cfgclear);          // clear sysconfig
}

/**
 * This code adds "Settings" section to the MENU
 * it is up to you to properly open/close Interface menu json_section
 */
void menuitem_options(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->option(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_SETTINGS]));     // пункт меню "настройки"
}

/**
 * формирование секции "настроек",
 * вызывается либо по выбору из "меню" либо при вызове из
 * других блоков/обработчиков
 * 
 */
void section_settings_frame(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    interf->json_section_main(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_SETTINGS]));

    interf->select(FPSTR(P_LANGUAGE), String(lang), String(FPSTR(T_DICT[lang][TD::D_LANG])), true);
    interf->option(0, F("Eng"));
    interf->option(1, F("Rus"));
    interf->json_section_end();

    interf->spacer();

    //interf->button_value(FPSTR(T_SH_SECT), 0, FPSTR(T_GNRL_SETUP));                 // кнопка перехода в общие настройки
    interf->button_value(FPSTR(T_SH_SECT), 1, FPSTR(T_DICT[lang][TD::D_WiFi]));         // кнопка перехода в настройки сети
    interf->button_value(FPSTR(T_SH_SECT), 2, FPSTR(T_DICT[lang][TD::D_DATETIME]));     // кнопка перехода в настройки времени
#ifdef EMBUI_MQTT
    interf->button_value(FPSTR(T_SH_SECT), 3, FPSTR(T_EN_MQTT));     // кнопка перехода в настройки MQTT
#endif
#ifndef EMBUI_NOFTP
    interf->button_value(FPSTR(T_SH_SECT), 4, F("FTP Server"));                        // кнопка перехода в настройки FTP
#endif
    interf->button_value(FPSTR(T_SH_SECT), 0, FPSTR(T_DICT[lang][TD::D_SYSSET]));      // кнопка перехода в настройки System

    interf->spacer();

    // call for user_defined function that may add more elements to the "settings page"
    user_settings_frame(interf, data);

    interf->json_frame_flush();
}

/**
 * @brief choose UI section to display based on supplied index
 * 
 */
void show_section(Interface *interf, JsonObject *data){
    if (!interf || !data || (*data)[FPSTR(T_SH_SECT)].isNull()) return;  // bail out if no section specifier

    // find section index "sh_sec"
    int idx = (*data)[FPSTR(T_SH_SECT)];

    switch (idx){
//        case 0 :    // general setup section
//            block_settings_gnrl(interf, data);
//            break;
        case 1 :    // WiFi network setup section
            block_settings_netw(interf, nullptr);
            break;
        case 2 :    // time setup section
            block_settings_time(interf, nullptr);
            break;
    #ifdef EMBUI_MQTT
        case 3 :    // MQTT setup section
            block_settings_mqtt(interf, nullptr);
            break;
    #endif  // #ifdef EMBUI_MQTT
        case 4 :    // FTP server setup section
            block_settings_ftp(interf, nullptr);
            break;
        default:
            block_settings_sys(interf, nullptr);
    }
}

/**
 *  BasicUI блок интерфейса настроек WiFi
 */
void block_settings_netw(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(FPSTR(T_OPT_NETW), FPSTR(T_EN_WiFi));

    // Hostname setup
    interf->json_section_hidden(FPSTR(T_SET_HOSTNAME), F("Device name"));
    interf->json_section_line();
    interf->comment(FPSTR(T_DICT[lang][TD::D_Hostname]));
    interf->constant(embui.hostname());
    interf->json_section_end(); // Line
    interf->text(FPSTR(P_hostname), "", F("Redefine hostname, or clear to reset to default"));
    interf->button_submit(FPSTR(T_SET_HOSTNAME), FPSTR(T_DICT[lang][TD::D_SAVE]), FPSTR(P_GREEN));
    interf->json_section_end(); // Hostname setup

    // Wi-Fi Client setup block
    interf->json_section_hidden(FPSTR(T_SET_WIFI), FPSTR(T_DICT[lang][TD::D_WiFiClient]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_WiFiClientOpts]));
    interf->text(FPSTR(P_WCSSID), WiFi.SSID(), FPSTR(T_DICT[lang][TD::D_WiFiSSID]));
    interf->password(FPSTR(P_WCPASS), "", FPSTR(T_DICT[lang][TD::D_Password]));
    interf->button_submit(FPSTR(T_SET_WIFI), FPSTR(T_DICT[lang][TD::D_CONNECT]), FPSTR(P_GRAY));
    interf->json_section_end();

    // Wi-Fi AP setup block
    interf->json_section_hidden(FPSTR(T_SET_WIFIAP), FPSTR(T_DICT[lang][TD::D_WiFiAP]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_WiFiAPOpts]));

    interf->password(FPSTR(P_APpwd),  FPSTR(T_DICT[lang][TD::D_MSG_APProtect]));    // AP password

    interf->json_section_line();
    interf->comment(F("Access Point SSID (hostname)"));
    interf->constant(embui.hostname());
    interf->json_section_end(); // Line

    interf->json_section_line();
    interf->checkbox(FPSTR(P_APonly), FPSTR(T_DICT[lang][TD::D_APOnlyMode]));       // checkbox "AP-only mode"
    interf->comment(FPSTR(T_DICT[lang][TD::D_MSG_APOnly]));
    interf->json_section_end(); // Line

    interf->json_section_line();
    interf->checkbox(P_NOCaptP, "Disable WiFi Captive-Portal");                     // checkbox "Disable Captive-portal"
    interf->comment("Do not run catch-all DNS in AP mode");
    interf->json_section_end(); // Line

    interf->button_submit(FPSTR(T_SET_WIFIAP), FPSTR(T_DICT[lang][TD::D_SAVE]), FPSTR(P_GRAY));

    interf->json_section_end(); // Wi-Fi AP

    interf->spacer();
    interf->button(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_EXIT]));

    interf->json_frame_flush();
}

/**
 *  BasicUI блок настройки даты/времени
 */
void block_settings_time(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(FPSTR(T_SET_TIME), FPSTR(T_DICT[lang][TD::D_DATETIME]));

    // Simple Clock display
    interf->json_section_line();
    String clk(F("Device date/time: ")); TimeProcessor::getDateTimeString(clk);
    interf->constant(FPSTR(P_date), nullptr, clk);
    interf->button_js(FPSTR(P_DTIME), F("Set local time"));     // run js function that post browser's date/time to device
    interf->json_section_end(); // line

    interf->comment(FPSTR(T_DICT[lang][TD::D_MSG_TZSet01]));     // комментарий-описание секции

    // Current TIME Zone string from config
    interf->text(FPSTR(P_TZSET), FPSTR(T_DICT[lang][TD::D_MSG_TZONE]));

    // NTP servers section
    interf->json_section_line();
    interf->comment(F("NTP Servers"));

#if ARDUINO <= 10805
    // ESP32's Arduino Core <=1.0.6 miss NTPoDHCP feature, see https://github.com/espressif/esp-idf/pull/7336
#else
    interf->checkbox(FPSTR(P_noNTPoDHCP), F("Disable NTP over DHCP"));
#endif
    interf->json_section_end(); // line

    // a list of ntp servers
    interf->json_section_line();
    for (uint8_t i = 0; i <= CUSTOM_NTP_INDEX; ++i)
        interf->constant(String(i), nullptr, TimeProcessor::getInstance().getserver(i));
    interf->json_section_end(); // line

    // user-defined NTP server field
    interf->text(FPSTR(P_userntp), FPSTR(T_DICT[lang][TD::D_NTP_Secondary]));

    // manual date and time setup
    interf->comment(FPSTR(T_DICT[lang][TD::D_MSG_DATETIME]));
    interf->json_section_line();
    interf->datetime(FPSTR(P_time), "", "");   // placeholder for ISO date/time string
    interf->button_js_value(FPSTR(P_DTIME), FPSTR(P_time), F("Paste local time"));  // js function that paste browser's date into FPSTR(P_time) field
    interf->json_section_end(); // line

    interf->button_submit(FPSTR(T_SET_TIME), FPSTR(T_DICT[lang][TD::D_SAVE]), FPSTR(P_GRAY));

    interf->spacer();

    // exit button
    interf->button(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_EXIT]));

    // close and send frame
    interf->json_frame_flush(); // main

    // формируем и отправляем кадр с запросом подгрузки внешнего ресурса со списком правил временных зон
    // полученные данные заместят предыдущее поле выпадающим списком с данными о всех временных зонах
    interf->json_frame(F("xload"));
    interf->json_section_content();
                    //id           val                                 label    direct  URL for external data
    interf->select(FPSTR(P_TZSET), embui.paramVariant(FPSTR(P_TZSET)), (char*)0, false,  F("/js/tz.json"));
    interf->json_section_end(); // select
    interf->json_frame_flush(); // xload

}

#ifdef EMBUI_MQTT
/**
 *  BasicUI блок интерфейса настроек MQTT
 */
void block_settings_mqtt(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(FPSTR(T_SET_MQTT), FPSTR(T_EN_MQTT));

    // форма настроек MQTT
    interf->text(FPSTR(P_m_host), FPSTR(T_DICT[lang][TD::D_MQTT_Host]));
    interf->number(FPSTR(P_m_port), FPSTR(T_DICT[lang][TD::D_MQTT_Port]));
    interf->text(FPSTR(P_m_user), FPSTR(T_DICT[lang][TD::D_User]));
    interf->text(FPSTR(P_m_pass), FPSTR(T_DICT[lang][TD::D_Password]));
    interf->text(FPSTR(P_m_pref), FPSTR(T_DICT[lang][TD::D_MQTT_Topic]));
    interf->number(FPSTR(P_m_tupd), FPSTR(T_DICT[lang][TD::D_MQTT_Interval]));
    interf->button_submit(FPSTR(T_SET_MQTT), FPSTR(T_DICT[lang][TD::D_CONNECT]), FPSTR(P_GRAY));

    interf->spacer();
    interf->button(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_EXIT]));

    interf->json_frame_flush();
}
#endif  // #ifdef EMBUI_MQTT

/**
 *  BasicUI блок настройки system
 */
void block_settings_sys(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main("sys", FPSTR(T_DICT[lang][TD::D_SYSSET]));

    // FW update
    interf->json_section_hidden(FPSTR(T_DO_OTAUPD), FPSTR(T_DICT[lang][TD::D_UPDATEFW]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_FWLOAD]));
    interf->file_form(FPSTR(T_DO_OTAUPD), FPSTR(T_DO_OTAUPD), FPSTR(T_DICT[lang][TD::D_UPLOADFW]), F("fw"));
    interf->file_form(FPSTR(T_DO_OTAUPD), FPSTR(T_DO_OTAUPD), FPSTR(T_DICT[lang][TD::D_UPLOADFS]), F("fs"));
    interf->json_section_end();

    interf->button(FPSTR(T_SET_CFGCLEAR), F("Clear sys config"), FPSTR(P_RED));

    interf->button(FPSTR(T_REBOOT), FPSTR(T_DICT[lang][TD::D_REBOOT]), FPSTR(P_RED));

    interf->spacer();

    // exit button
    interf->button(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_EXIT]));

    interf->json_frame_flush(); // main
}

/**
 * WiFi Client settings handler
 */
void set_settings_wifi(Interface *interf, JsonObject *data){
    if (!data) return;

    embui.var_remove(FPSTR(P_APonly));              // remove "force AP mode" parameter when attempting connection to external AP
    embui.wifi->connect((*data)[P_WCSSID].as<const char*>(), (*data)[P_WCPASS].as<const char*>());

    section_settings_frame(interf, nullptr);           // display "settings" page
}

/**
 * Обработчик настроек WiFi в режиме AP
 */
void set_settings_wifiAP(Interface *interf, JsonObject *data){
    if (!data) return;

    embui.var_dropnulls(FPSTR(P_APonly), (*data)[FPSTR(P_APonly)]);     // AP-Only chkbx
    embui.var_dropnulls(FPSTR(P_APpwd), (*data)[FPSTR(P_APpwd)]);       // AP password
    embui.var_dropnulls(P_NOCaptP, (*data)[P_NOCaptP]);                 // captive portal chkbx

    embui.wifi->aponly((*data)[P_APonly]);
    //embui.wifi->setupAP(true);        // no need to apply settings now?

    if (interf) section_settings_frame(interf, nullptr);                // go to "Options" page
}

#ifdef EMBUI_MQTT
/**
 * Обработчик настроек MQTT
 */
void set_settings_mqtt(Interface *interf, JsonObject *data){
    if (!data) return;
    // сохраняем настройки в конфиг
    var_dropnulls(P_m_host, (*data)[P_m_host]);
    var_dropnulls(P_m_user, (*data)[P_m_user]);
    var_dropnulls(P_m_pass, (*data)[P_m_pass]);
    var_dropnulls(P_m_pref, (*data)[P_m_pref]);
    var_dropnulls(P_m_tupd, (*data)[P_m_tupd]);
    embui.save();

    section_settings_frame(interf, data);
}
#endif  // #ifdef EMBUI_MQTT

/**
 * Обработчик настроек даты/времени
 */
void set_settings_time(Interface *interf, JsonObject *data){
    if (!data) return;

    // Save and apply timezone rules
    SETPARAM_NONULL(FPSTR(P_TZSET))

    String tzrule = (*data)[FPSTR(P_TZSET)];
    if (!tzrule.isEmpty()){
        TimeProcessor::getInstance().tzsetup(tzrule.substring(4).c_str());   // cutoff '000_' prefix key
    }

    SETPARAM_NONULL(FPSTR(P_userntp), TimeProcessor::getInstance().setcustomntp((*data)[FPSTR(P_userntp)]));

#if ARDUINO <= 10805
    // ESP32's Arduino Core <=1.0.6 miss NTPoDHCP feature
#else
    SETPARAM_NONULL( FPSTR(P_noNTPoDHCP), TimeProcessor::getInstance().ntpodhcp(!(*data)[FPSTR(P_noNTPoDHCP)]) )
#endif

    // if there is a field with custom ISO date/time, call time setter
    if ((*data)[FPSTR(P_time)])
        set_datetime(nullptr, data);

    section_settings_frame(interf, data);   // redirect to 'settings' page
}

/**
 * @brief set system date/time from ISO string
 * data obtained through "time":"YYYY-MM-DDThh:mm:ss" param
 */
void set_datetime(Interface *interf, JsonObject *data){
    if (!data) return;
    TimeProcessor::getInstance().setTime((*data)[FPSTR(P_time)].as<String>());
    if (interf)
        block_settings_time(interf, nullptr);
}

void set_language(Interface *interf, JsonObject *data){
    if (!data) return;

    embui.var_dropnulls(FPSTR(P_LANGUAGE), (*data)[FPSTR(P_LANGUAGE)]);
    lang = (*data)[FPSTR(P_LANGUAGE)];
    section_settings_frame(interf, data);
}

void embuistatus(Interface *interf){
    if (!interf) return;
    interf->json_frame_value();
    interf->value(F("pTime"), TimeProcessor::getInstance().getFormattedShortTime(), true);
    interf->value(F("pMem"), ESP.getFreeHeap(), true);
    interf->value(F("pUptime"), millis()/1000, true);
    interf->json_frame_flush();
}

void set_reboot(Interface *interf, JsonObject *data){
    Task *t = new Task(TASK_SECOND*5, TASK_ONCE, nullptr, &ts, false, nullptr, [](){ LOG(println, F("Rebooting...")); ESP.restart(); });
    t->enableDelayed();
    if(interf){
        interf->json_frame_interface();
        block_settings_sys(interf, nullptr);
        interf->json_frame_flush();
    }
}

/**
 * @brief set system hostname
 * if empty/missing param provided, than use autogenerated hostname
 */
void set_hostname(Interface *interf, JsonObject *data){
    if (!data) return;

    embui.hostname((*data)[FPSTR(P_hostname)].as<const char*>());
    section_settings_frame(interf, data);           // переходим в раздел "настройки"
}

/**
 * @brief clears EmbUI config
 */
void set_cfgclear(Interface *interf, JsonObject *data){
    embui.cfgclear();
    if (interf) section_settings_frame(interf, nullptr);
}

}   // end of "namespace basicui"

// stub function - переопределяется в пользовательском коде при необходимости добавить доп. пункты в меню настройки
void user_settings_frame(Interface *interf, JsonObject *data){};
