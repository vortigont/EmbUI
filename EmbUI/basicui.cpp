#include "basicui.h"

uint8_t lang;            // default language for text resources


/**
 * Define configuration variables and controls handlers
 * 
 * Variables has literal names and are kept within json-configuration file on flash
 * Control handlers are bound by literal name with a particular method. This method is invoked
 * by manipulating controls
 * 
 * this method owerrides weak definition in framework
 * 
 */
void BasicUI::add_sections(){
    LOG(println, F("UI: Creating webui vars"));

    // variable for UI language (specific to basic UI translations)
    lang = embui.paramVariant(FPSTR(P_LANGUAGE)).as<uint8_t>();

    /**
     * обработчики действий
     */ 
    // вывод BasicUI секций
    embui.section_handle_add(FPSTR(T_SETTINGS), section_settings_frame);    // generate "settings" UI section
    embui.section_handle_add(FPSTR(T_SH_SECT),  show_section);              // display UI section template

    // обработка базовых настроек
    embui.section_handle_add(FPSTR(T_SET_HOSTNAME), set_hostname);          // hostname setup
    embui.section_handle_add(FPSTR(T_SET_WIFI), set_settings_wifi);         // обработка настроек WiFi Client
    embui.section_handle_add(FPSTR(T_SET_WIFIAP), set_settings_wifiAP);     // обработка настроек WiFi AP
    embui.section_handle_add(FPSTR(T_SET_MQTT), set_settings_mqtt);         // обработка настроек MQTT
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
void BasicUI::opt_setup(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->option(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_SETTINGS]));     // пункт меню "настройки"
}

/**
 * формирование секции "настроек",
 * вызывается либо по выбору из "меню" либо при вызове из
 * других блоков/обработчиков
 * 
 */
void BasicUI::section_settings_frame(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    interf->json_section_main(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_SETTINGS]));

    interf->select(FPSTR(P_LANGUAGE), String(lang), String(FPSTR(T_DICT[lang][TD::D_LANG])), true);
    interf->option(0, F("Rus"));
    interf->option(1, F("Eng"));
    interf->json_section_end();

    interf->spacer();

    //interf->button_value(FPSTR(T_SH_SECT), 0, FPSTR(T_GNRL_SETUP));                 // кнопка перехода в общие настройки
    interf->button_value(FPSTR(T_SH_SECT), 1, FPSTR(T_DICT[lang][TD::D_WiFi]));     // кнопка перехода в настройки сети
    interf->button_value(FPSTR(T_SH_SECT), 2, FPSTR(T_DICT[lang][TD::D_DATETIME]));     // кнопка перехода в настройки времени
    interf->button_value(FPSTR(T_SH_SECT), 3, FPSTR(T_EN_MQTT));     // кнопка перехода в настройки MQTT
    interf->button_value(FPSTR(T_SH_SECT), 4, FPSTR(T_DICT[lang][TD::D_SYSSET]));      // кнопка перехода в настройки System

    interf->spacer();

    // call for user_defined function that may add more elements to the "settings page"
    user_settings_frame(interf, data);

    interf->json_frame_flush();
}

/**
 * @brief choose UI section to display based on supplied index
 * 
 */
void BasicUI::show_section(Interface *interf, JsonObject *data){
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
        case 3 :    // MQTT setup section
            block_settings_mqtt(interf, nullptr);
            break;
        case 4 :    // System setup section
            block_settings_sys(interf, nullptr);
            break;
        default:
            //LOG(println, "Not found");
            return;
    }
}

/**
 *  BasicUI - general settings
 */
/*
void BasicUI::block_settings_gnrl(Interface *interf, JsonObject *data){
    if (!interf) return;

    interf->json_frame_interface();

    // Headline
    interf->json_section_main(FPSTR(T_SET_HOSTNAME), FPSTR(T_GNRL_SETUP));

    interf->json_section_line();
    interf->comment(FPSTR(T_DICT[lang][TD::D_Hostname]));
    interf->constant(FPSTR(T_EN_OTHER), "", embui.hostname());
    interf->json_section_end(); // Line

    interf->text(FPSTR(P_hostname), "", F("Set hostname"), false);
    interf->button_submit(FPSTR(T_SET_HOSTNAME), FPSTR(T_DICT[lang][TD::D_SAVE]), FPSTR(P_GREEN));

    interf->spacer();
    interf->button(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_EXIT]));

    interf->json_frame_flush();
};
*/

/**
 *  BasicUI блок интерфейса настроек WiFi
 */
void BasicUI::block_settings_netw(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(FPSTR(T_OPT_NETW), FPSTR(T_EN_WiFi));

    // Hostname setup
    interf->json_section_hidden(FPSTR(T_SET_HOSTNAME), F("Device name"));
    interf->json_section_line();
    interf->comment(FPSTR(T_DICT[lang][TD::D_Hostname]));
    interf->constant(FPSTR(T_EN_OTHER), "", embui.hostname());
    interf->json_section_end(); // Line
    interf->text(FPSTR(P_hostname), "", F("Redefine hostname, or clear to reset to default"), false);
    interf->button_submit(FPSTR(T_SET_HOSTNAME), FPSTR(T_DICT[lang][TD::D_SAVE]), FPSTR(P_GREEN));
    interf->json_section_end(); // Hostname setup

    // форма настроек Wi-Fi Client
    interf->json_section_hidden(FPSTR(T_SET_WIFI), FPSTR(T_DICT[lang][TD::D_WiFiClient]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_WiFiClientOpts]));
    interf->text(FPSTR(P_WCSSID), WiFi.SSID(), FPSTR(T_DICT[lang][TD::D_WiFiSSID]), false);
    interf->password(FPSTR(P_WCPASS), "", FPSTR(T_DICT[lang][TD::D_Password]));
    interf->button_submit(FPSTR(T_SET_WIFI), FPSTR(T_DICT[lang][TD::D_CONNECT]), FPSTR(P_GRAY));
    interf->json_section_end();

    // форма настроек Wi-Fi AP
    interf->json_section_hidden(FPSTR(T_SET_WIFIAP), FPSTR(T_DICT[lang][TD::D_WiFiAP]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_WiFiAPOpts]));

    interf->json_section_line();
    interf->comment(F("Access Point SSID (hostname)"));
    interf->constant(FPSTR(T_EN_OTHER), "", embui.hostname());
    interf->json_section_end(); // Line

    interf->comment(FPSTR(T_DICT[lang][TD::D_MSG_APOnly]));

    interf->json_section_line();
    interf->checkbox(FPSTR(P_APonly), FPSTR(T_DICT[lang][TD::D_APOnlyMode]));
    interf->password(FPSTR(P_APpwd),  FPSTR(T_DICT[lang][TD::D_MSG_APProtect]));
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
void BasicUI::block_settings_time(Interface *interf, JsonObject *data){
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

// esp32 is not (yet) supported, see https://github.com/espressif/esp-idf/pull/7336
#ifndef ESP32
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
    interf->datetime(FPSTR(P_time), nullptr, "");   // placeholder for ISO date/time string
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
    interf->json_frame_custom(F("xload"));
    interf->json_section_content();
                    //id           val                                 label direct  skipl URL for external data
    interf->select(FPSTR(P_TZSET), embui.paramVariant(FPSTR(P_TZSET)),   "", false,  true, F("/js/tz.json"));
    interf->json_section_end(); // select
    interf->json_frame_flush(); // xload

}

/**
 *  BasicUI блок интерфейса настроек MQTT
 */
void BasicUI::block_settings_mqtt(Interface *interf, JsonObject *data){
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

/**
 *  BasicUI блок настройки system
 */
void BasicUI::block_settings_sys(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main("", FPSTR(T_DICT[lang][TD::D_SYSSET]));

    // FW update
    interf->json_section_hidden(FPSTR(T_DO_OTAUPD), FPSTR(T_DICT[lang][TD::D_Update]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_FWLOAD]));
    interf->file(FPSTR(T_DO_OTAUPD), FPSTR(T_DO_OTAUPD), FPSTR(T_DICT[lang][TD::D_UPLOAD]));
    interf->json_section_end();

    interf->button(FPSTR(T_SET_CFGCLEAR), F("Clear sys config"), FPSTR(P_RED));

    interf->button(FPSTR(T_REBOOT), FPSTR(T_DICT[lang][TD::D_REBOOT]), FPSTR(P_RED));

    interf->spacer();

    // exit button
    interf->button(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_EXIT]));

    interf->json_frame_flush(); // main
}

/**
 * Обработчик настроек WiFi в режиме клиента
 */
void BasicUI::set_settings_wifi(Interface *interf, JsonObject *data){
    if (!data) return;

    const char *ssid = (*data)[FPSTR(P_WCSSID)];    // переменные доступа в конфиге не храним
    const char *pwd = (*data)[FPSTR(P_WCPASS)];     // фреймворк хранит последнюю доступную точку самостоятельно

    embui.var_remove(FPSTR(P_APonly)); // сборосим режим принудительного AP, при попытке подключения к роутеру
    embui.wifi_connect(ssid, pwd);

    section_settings_frame(interf, data);           // переходим в раздел "настройки"
}

/**
 * Обработчик настроек WiFi в режиме AP
 */
void BasicUI::set_settings_wifiAP(Interface *interf, JsonObject *data){
    if (!data) return;

    embui.var_dropnulls(FPSTR(P_APonly), (*data)[FPSTR(P_APonly)]);
    embui.var_dropnulls(FPSTR(P_APpwd), (*data)[FPSTR(P_APpwd)]);

    embui.wifi_updateAP();

    if (interf) section_settings_frame(interf, data);   // переходим в раздел "настройки"
}

/**
 * Обработчик настроек MQTT
 */
void BasicUI::set_settings_mqtt(Interface *interf, JsonObject *data){
    if (!data) return;
    // сохраняем настройки в конфиг
    SETPARAM(FPSTR(P_m_host));
    SETPARAM(FPSTR(P_m_port));
    SETPARAM(FPSTR(P_m_user));
    SETPARAM(FPSTR(P_m_pass));
    SETPARAM(FPSTR(P_m_pref));
    SETPARAM(FPSTR(P_m_tupd));
    //SETPARAM(FPSTR(P_m_tupd), some_mqtt_object.semqtt_int((*data)[FPSTR(P_m_tupd)]));

    embui.save();

    section_settings_frame(interf, data);
}

/**
 * Обработчик настроек даты/времени
 */
void BasicUI::set_settings_time(Interface *interf, JsonObject *data){
    if (!data) return;

    // Save and apply timezone rules
    SETPARAM_NONULL(FPSTR(P_TZSET))

    String tzrule = (*data)[FPSTR(P_TZSET)];
    if (!tzrule.isEmpty()){
        embui.timeProcessor.tzsetup(tzrule.substring(4).c_str());   // cutoff '000_' prefix key
    }

    SETPARAM_NONULL(FPSTR(P_userntp), embui.timeProcessor.setcustomntp((*data)[FPSTR(P_userntp)]));

#ifndef ESP32
    SETPARAM_NONULL( FPSTR(P_noNTPoDHCP), embui.timeProcessor.ntpodhcp(!(*data)[FPSTR(P_noNTPoDHCP)]) )
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
void BasicUI::set_datetime(Interface *interf, JsonObject *data){
    if (!data) return;
    TimeProcessor::getInstance().setTime((*data)[FPSTR(P_time)].as<String>());
    if (interf)
        block_settings_time(interf, nullptr);
}

void BasicUI::set_language(Interface *interf, JsonObject *data){
    if (!data) return;

    embui.var_dropnulls(FPSTR(P_LANGUAGE), (*data)[FPSTR(P_LANGUAGE)]);
    lang = (*data)[FPSTR(P_LANGUAGE)];
    section_settings_frame(interf, data);
}

void BasicUI::embuistatus(Interface *interf){
    if (!interf) return;
    interf->json_frame_value();
    interf->value(F("pTime"), embui.timeProcessor.getFormattedShortTime(), true);
    interf->value(F("pMem"), ESP.getFreeHeap(), true);
    interf->value(F("pUptime"), millis()/1000, true);
    interf->json_frame_flush();
}

void BasicUI::set_reboot(Interface *interf, JsonObject *data){
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
void BasicUI::set_hostname(Interface *interf, JsonObject *data){
    if (!data) return;

    embui.hostname((*data)[FPSTR(P_hostname)].as<const char*>());
    section_settings_frame(interf, data);           // переходим в раздел "настройки"
}

/**
 * @brief clears EmbUI config
 */
void BasicUI::set_cfgclear(Interface *interf, JsonObject *data){
    embui.cfgclear();
    if (interf) section_settings_frame(interf, nullptr);
}


// stub function - переопределяется в пользовательском коде при необходимости добавить доп. пункты в меню настройки
void user_settings_frame(Interface *interf, JsonObject *data){};

