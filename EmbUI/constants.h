// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once
#include "i18n.h"           // localized GUI text-strings

// Empty string
#define P_EMPTY static_cast<const char*>(0)

// UI blocks generator actions (IDs)
static constexpr const char* UI_SETTINGS = "ui_settings";
static constexpr const char* UI_NETWORK = "ui_network";

// setter actions (IDs)
static constexpr const char* SET_FTP = "set_ntwrk_ftp";
static constexpr const char* SET_WIFI = "set_ntwrk_wifi";
static constexpr const char* SET_WIFIAP = "set_ntwrk_wifiAP";
static constexpr const char* SET_MQTT = "set_ntwrk_mqtt";
static constexpr const char* SET_TIMEOPTIONS = "set_sys_timeoptions";
static constexpr const char* SET_HOSTNAME = "set_sys_hostname";
static constexpr const char* SET_DATETIME = "set_sys_datetime";
static constexpr const char* SET_LANGUAGE = "set_ui_language";
// bare actions
static constexpr const char* SET_CFGCLEAR = "set_sys_cfgclr";
static constexpr const char* SET_REBOOT = "set_sys_reboot";

// GET actions than only query for data
static constexpr const char* GET_UIPAGE = "get_ui_page";

// other Action ID's
static constexpr const char* A_ui_mainpage = "ui_page_main";
static constexpr const char* A_ui_usersettings = "ui_usersettings";
static constexpr const char* A_publish = "publish";

// misc
static constexpr const char* T_DO_OTAUPD = "update";    // http OTA update URL /update


// Interface elements
static constexpr const char* P_action = "action";
static constexpr const char* P_app = "app";
static constexpr const char* P_appjsapi = "appjsapi";
static constexpr const char* P_block = "block";
static constexpr const char* P_button = "button";
static constexpr const char* P_color = "color";
static constexpr const char* P_chckbox = "checkbox";
static constexpr const char* P_class = "class";
static constexpr const char* P_comment = "comment";
static constexpr const char* P_const = "const";
static constexpr const char* P_data = "data";
static constexpr const char* P_date = "date";
static constexpr const char* P_datetime = "datetime-local";
static constexpr const char* P_display = "display";
static constexpr const char* P_div = "div";
static constexpr const char* P_directly = "directly";
static constexpr const char* P_dtime = "dtime";
static constexpr const char* P_email = "email";
static constexpr const char* P_empty_quotes = "";
static constexpr const char* P_file = "file";
static constexpr const char* P_final = "final";
static constexpr const char* P_form = "form";
static constexpr const char* P_frame = "frame";
static constexpr const char* P_ftp = "ftp";
static constexpr const char* P_ftp_usr = "ftp_usr";
static constexpr const char* P_ftp_pwd = "ftp_pwd";
static constexpr const char* P_hidden = "hidden";
static constexpr const char* P_html = "html";
static constexpr const char* P_id = "id";
static constexpr const char* P_iframe = "iframe";
static constexpr const char* P_js = "js";
static constexpr const char* P_input = "input";
static constexpr const char* P_interface = "interface";
static constexpr const char* P_label = "label";
static constexpr const char* P_MQTT = "MQTT";
static constexpr const char* P_manifest = "manifest";
static constexpr const char* P_max = "max";
static constexpr const char* P_menu = "menu";
static constexpr const char* P_min = "min";
static constexpr const char* P_number = "number";
static constexpr const char* P_options = "options";
static constexpr const char* P_params = "params";
static constexpr const char* P_password = "password";
static constexpr const char* P_pkg = "pkg";
static constexpr const char* P_progressbar = "pbar";
static constexpr const char* P_pMem = "pMem";
static constexpr const char* P_pRSSI = "pRSSI";
static constexpr const char* P_pTime = "pTime";
static constexpr const char* P_pUptime = "pUptime";
static constexpr const char* P_range = "range";
static constexpr const char* P_section = "section";
static constexpr const char* P_select = "select";
static constexpr const char* P_spacer = "spacer";
static constexpr const char* P_step = "step";
static constexpr const char* P_submit = "submit";
static constexpr const char* P_tcp = "tcp";
static constexpr const char* P_text = "text";
static constexpr const char* P_textarea = "textarea";
static constexpr const char* P_time = "time";
static constexpr const char* P_type = "type";
static constexpr const char* P_uijsapi = "uijsapi";
static constexpr const char* P_url = "url";
static constexpr const char* P_uiver = "uiver";
static constexpr const char* P_value = "value";
static constexpr const char* P_wifi = "wifi";
static constexpr const char* P_xload ="xload";


// order of elements MUST match with 'enum class ui_element_t' in ui.h
// increase index in case of new elements
static constexpr std::array<const char *const, 25> UI_T_DICT = {
  NULL,         // custom 0
  P_button,
  P_chckbox,
  P_color,
  P_comment,
  P_const,
  P_date,
  P_datetime,
  P_display,
  P_div,
  P_email,      // 10
  P_file,
  P_form,
  P_hidden,
  P_iframe,
  P_input,
  NULL,         // 'option' (for selects)
  P_password,
  P_range,
  P_select,
  P_spacer,     // 20
  P_text,
  P_textarea,
  P_time,
  P_value
};


// order of elements MUST match with 'enum class ui_param_t' in ui.h
static constexpr std::array<const char *const, 5> UI_KEY_DICT {
  P_html,
  P_id,
  P_hidden,
  P_type,
  P_value,
};

// UI colors
static constexpr const char* P_RED = "red";
static constexpr const char* P_ORANGE = "orange";
static constexpr const char* P_YELLOW = "yellow";
static constexpr const char* P_GREEN = "green";
static constexpr const char* P_BLUE = "blue";
static constexpr const char* P_GRAY = "gray";
static constexpr const char* P_BLACK = "black";
static constexpr const char* P_WHITE = "white";

// System configuration variables and constants
static constexpr const char* C_cfgfile = "/config.json";
static constexpr const char* C_cfgfile_bkp = "/config_bkp.json";

static constexpr const char* V_APonly = "APonly";        // AccessPoint-only mode
static constexpr const char* V_APpwd = "APpwd";          // AccessPoint password
static constexpr const char* V_timezone = "timezone";          // TimeZone rule variable
static constexpr const char* V_NOCaptP = "ncapp";        // Captive Portal Disabled
static constexpr const char* V_hostname = "hostname";    // System hostname
static constexpr const char* V_LANGUAGE = "lang";        // UI language
static constexpr const char* V_noNTPoDHCP = "ntpod";     // Disable NTP over DHCP
static constexpr const char* V_userntp = "userntp";      // user-defined NTP server

// WiFi vars
static constexpr const char* V_WCSSID = "wcssid";        // WiFi-Client SSID
static constexpr const char* V_WCPASS = "wcpass";        // WiFi-Client password

// MQTT related vars, topic names, etc
static constexpr const char* V_mqtt_enable = "mqtt_ena";
static constexpr const char* V_mqtt_host = "mqtt_host";
static constexpr const char* V_mqtt_pass = "mqtt_pass";
static constexpr const char* V_mqtt_port = "mqtt_port";
static constexpr const char* V_mqtt_topic = "mqtt_topic";
static constexpr const char* V_mqtt_user = "mqtt_user";
static constexpr const char* V_mqtt_ka = "mqtt_ka";       // mqtt keep-alive interval
static constexpr const char* C_sys = "sys/";              // mqtt 'sys/' suffix
static constexpr const char* C_pub = "pub/";              // mqtt 'pub/' suffix

// http-related constants
static constexpr const char* PGgzip = "gzip";
static constexpr const char* PGhdrcachec = "Cache-Control";
static constexpr const char* PGhdrcontentenc = "Content-Encoding";
static constexpr const char* PGmimecss  = "text/css";
static constexpr const char* PGmimejson = "application/json";
static constexpr const char* PGmimexml  = "text/xml";
static constexpr const char* PGnocache = "no-cache, no-store, must-revalidate";
static constexpr const char* PG404  = "Not found";
static constexpr const char* PGimg = "img";

// LOG Messages
static constexpr const char* P_ERR_obj2large  = "UI: ERORR - can't add object to frame, too large!";

#ifdef USE_SSDP
  #ifndef EXTERNAL_SSDP
    #define __SSDPNAME      ("EmbUI")
    #define __SSDPURLMODEL  ("https://github.com/vortigont/")
    #define __SSDPMODEL     EMBUI_VERSION_STRING
    #define __SSDPURLMANUF  ("https://github.com/anton-zolotarev")
    #define __SSDPMANUF     ("obliterator")
  #endif

  static constexpr const char* PGnameModel = TOSTRING(__SSDPNAME);
  static constexpr const char* PGurlModel = TOSTRING(__SSDPURLMODEL);
  static constexpr const char* PGversion = EMBUI_VERSION_STRING;
  static constexpr const char* PGurlManuf = TOSTRING(__SSDPURLMANUF);
  static constexpr const char* PGnameManuf = TOSTRING(__SSDPMANUF);
#endif
