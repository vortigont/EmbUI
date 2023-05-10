// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

// Empty string
static const char P_EMPTY[] = "";

// Interface
static const char P_action[] PROGMEM = "action";
static const char P_block[] PROGMEM = "block";
static const char P_button[] PROGMEM = "button";
static const char P_color[] PROGMEM = "color";
static const char P_chckbox[] PROGMEM = "checkbox";
static const char P_class[] PROGMEM = "class";
static const char P_comment[] PROGMEM = "comment";
static const char P_const[] PROGMEM = "const";
static const char P_data[] PROGMEM = "data";
static const char P_date[] PROGMEM = "date";
static const char P_datetime[] PROGMEM = "datetime-local";
static const char P_display[] PROGMEM = "display";
static const char P_div[] PROGMEM = "div";
static const char P_directly[] PROGMEM = "directly";
static const char P_email[] PROGMEM = "email";
static const char P_false[] PROGMEM = "0";
static const char P_file[] PROGMEM = "file";
static const char P_final[] PROGMEM = "final";
static const char P_form[] PROGMEM = "form";
static const char P_frame[] PROGMEM = "frame";
static const char P_iframe[] PROGMEM = "iframe";
static const char P_ftp[] PROGMEM = "ftp";
static const char P_ftp_usr[] PROGMEM = "ftp_usr";
static const char P_ftp_pwd[] PROGMEM = "ftp_pwd";
static const char P_hidden[] PROGMEM = "hidden";
static const char P_html[] PROGMEM = "html";
static const char P_id[] PROGMEM = "id";
static const char P_iframe[] PROGMEM = "iframe";
static const char P_js[] PROGMEM = "js";
static const char P_input[] PROGMEM = "input";
static const char P_interface[] PROGMEM = "interface";
static const char P_label[] PROGMEM = "label";
static const char P_manifest[] PROGMEM = "manifest";
static const char P_max[] PROGMEM = "max";
static const char P_menu[] PROGMEM = "menu";
static const char P_min[] PROGMEM = "min";
static const char P_null[] PROGMEM = "null";
static const char P_number[] PROGMEM = "number";
static const char P_options[] PROGMEM = "options";
static const char P_params[] PROGMEM = "params";
static const char P_password[] PROGMEM = "password";
static const char P_pkg[] PROGMEM = "pkg";
static const char P_progressbar[] PROGMEM = "pbar";
static const char P_range[] PROGMEM = "range";
static const char P_section[] PROGMEM = "section";
static const char P_select[] PROGMEM = "select";
static const char P_spacer[] PROGMEM = "spacer";
static const char P_step[] PROGMEM = "step";
static const char P_submit[] PROGMEM = "submit";
static const char P_text[] PROGMEM = "text";
static const char P_textarea[] PROGMEM = "textarea";
static const char P_time[] PROGMEM = "time";
static const char P_true[] PROGMEM = "1";
static const char P_type[] PROGMEM = "type";
static const char P_url[] PROGMEM = "url";
static const char P_value[] PROGMEM = "value";
static const char P_wifi[] PROGMEM = "wifi";









// order of elements MUST match with 'enum class ui_element_t' in ui.h
#define UI_T_DICT_SIZE  30      // increase index in case of new elements
static const char *const UI_T_DICT[UI_T_DICT_SIZE] PROGMEM = {
  NULL,         // custom
  P_button,
  P_chckbox,
  P_color,
  P_comment,
  P_const,
  P_date,
  P_datetime,
  P_display,
  P_div,
  P_email,
  P_file,
  P_form,
  P_hidden,
  P_iframe,
  P_input,
  NULL,     // 'option' (for selects)
  P_password,
  P_range,
  P_select,
  P_spacer,
  P_text,
  P_textarea,
  P_time,
  P_value
};


// order of elements MUST match with 'enum class ui_param_t' in ui.h
#define UI_KEY_DICT_SIZE  10      // increase index in case of new elements
static const char *const UI_KEY_DICT[UI_KEY_DICT_SIZE] PROGMEM = {
  P_html,
  P_id,
  P_hidden,
  P_type,
  P_value,

};

// UI colors
static const char P_RED[] PROGMEM = "red";
static const char P_ORANGE[] PROGMEM = "orange";
static const char P_YELLOW[] PROGMEM = "yellow";
static const char P_GREEN[] PROGMEM = "green";
static const char P_BLUE[] PROGMEM = "blue";
static const char P_GRAY[] PROGMEM = "gray";
static const char P_BLACK[] PROGMEM = "black";
static const char P_WHITE[] PROGMEM = "white";

// System configuration variables
static const char P_cfgfile[] PROGMEM = "/config.json";
static const char P_cfgfile_bkp[] PROGMEM = "/config_bkp.json";

static const char P_APonly[] PROGMEM = "APonly";        // AccessPoint-only mode
static const char P_APpwd[] PROGMEM = "APpwd";          // AccessPoint password
static const char P_TZSET[] PROGMEM = "TZSET";          // TimeZone rule variable
static const char P_DTIME[] PROGMEM = "dtime";
static const char P_NOCaptP[] PROGMEM = "ncapp";        // Captive Portal Disabled
static const char P_hostname[] PROGMEM = "hostname";    // System hostname
static const char P_LANGUAGE[] PROGMEM = "lang";        // UI language
static const char P_noNTPoDHCP[] PROGMEM = "ntpod";     // Disable NTP over DHCP
static const char P_userntp[] PROGMEM = "userntp";      // user-defined NTP server

// WiFi vars
static const char P_WCSSID[] PROGMEM = "wcssid";        // WiFi-Client SSID
static const char P_WCPASS[] PROGMEM = "wcpass";        // WiFi-Client password

// MQTT vars
static const char P_m_host[] PROGMEM = "m_host";
static const char P_m_pass[] PROGMEM = "m_pass";
static const char P_m_port[] PROGMEM = "m_port";
static const char P_m_pref[] PROGMEM = "m_pref";
static const char P_m_user[] PROGMEM = "m_user";
static const char P_m_tupd[] PROGMEM = "m_tupd";     // mqtt update interval

// http-related constants
static const char PGgzip[] PROGMEM = "gzip";
static const char PGhdrcachec[] PROGMEM = "Cache-Control";
static const char PGhdrcontentenc[] PROGMEM = "Content-Encoding";
static const char PGmimecss[] PROGMEM  = "text/css";
static const char PGmimejson[] PROGMEM = "application/json";
static const char PGmimexml[] PROGMEM  = "text/xml";
static const char PGnocache[] PROGMEM = "no-cache, no-store, must-revalidate";
static const char PG404[] PROGMEM  = "Not found";
static const char PGimg[] PROGMEM = "img";

#ifndef ESP32
static const char PGmimetxt[] PROGMEM  = "text/plain";
static const char PGmimehtml[] PROGMEM = "text/html; charset=utf-8";
#endif

// LOG Messages
static const char P_ERR_obj2large[] PROGMEM  = "UI: ERORR - can't add object to frame, too large!";

#ifdef USE_SSDP
  #ifndef EXTERNAL_SSDP
    #define __SSDPNAME      ("EmbUI")
    #define __SSDPURLMODEL  ("https://github.com/vortigont/")
    #define __SSDPMODEL     EMBUI_VERSION_STRING
    #define __SSDPURLMANUF  ("https://github.com/anton-zolotarev")
    #define __SSDPMANUF     ("obliterator")
  #endif

  static const char PGnameModel[] PROGMEM = TOSTRING(__SSDPNAME);
  static const char PGurlModel[] PROGMEM = TOSTRING(__SSDPURLMODEL);
  static const char PGversion[] PROGMEM = EMBUI_VERSION_STRING;
  static const char PGurlManuf[] PROGMEM = TOSTRING(__SSDPURLMANUF);
  static const char PGnameManuf[] PROGMEM = TOSTRING(__SSDPMANUF);
#endif
