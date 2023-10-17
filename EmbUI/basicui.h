/*
    Here is a set of predefined WebUI elements for system settings setup like WiFi, time, MQTT, etc...
*/
#pragma once

#include "ui.h"
#include "i18n.h"        // localized GUI text-strings

/**
 * List of UI languages in predefined i18n resources
 */
enum LANG : uint8_t {
    EN = (0U),
    RU = (1U),
};

extern uint8_t lang;

// UI blocks
static constexpr const char* T_SETTINGS = "settings";
static constexpr const char* T_OPT_NETW = "netwrk";

// UI handlers
static constexpr const char* T_DO_OTAUPD = "update";
static constexpr const char* T_SET_FTP = "set_ftp";
static constexpr const char* T_SET_WIFI = "set_wifi";
static constexpr const char* T_SET_WIFIAP = "set_wifiAP";
static constexpr const char* T_SET_MQTT = "set_mqtt";
static constexpr const char* T_SET_TIME = "set_time";
static constexpr const char* T_SET_HOSTNAME = "shname";
static constexpr const char* T_SET_CFGCLEAR = "s_cfgcl";


static constexpr const char* T_SH_SECT = "sh_sec";
static constexpr const char* T_REBOOT = "reboot";

/*
    A namespace with functions to handle basic EmbUI WebUI interface
*/
namespace basicui {

  /**
   * register handlers for system actions and setup pages
   * 
   */
  void register_handlers();

  /**
   * This code adds "Settings" section to the MENU
   * it is up to you to properly open/close Interface json_section
   */
  void menuitem_settings(Interface *interf, JsonObject *data, const char* action);

  void show_section(Interface *interf, JsonObject *data, const char* action);
  void block_settings_gnrl(Interface *interf, JsonObject *data, const char* action);
  void block_settings_netw(Interface *interf, JsonObject *data, const char* action);
  void block_settings_mqtt(Interface *interf, JsonObject *data, const char* action);
  void block_settings_time(Interface *interf, JsonObject *data, const char* action);
  void block_settings_sys(Interface *interf, JsonObject *data, const char* action);

  /**
   * @brief Build WebUI "Settings" page
   * it will create system settings page and call action for user callback to append user block to the settings page
   * 
   * 
   * @param interf 
   * @param data 
   * @param action 
   */
  void page_system_settings(Interface *interf, JsonObject *data, const char* action);
  void set_settings_wifi(Interface *interf, JsonObject *data, const char* action);
  void set_settings_wifiAP(Interface *interf, JsonObject *data, const char* action);
  void set_settings_mqtt(Interface *interf, JsonObject *data, const char* action);
  void set_settings_time(Interface *interf, JsonObject *data, const char* action);
  void set_language(Interface *interf, JsonObject *data, const char* action);
  void embuistatus(Interface *interf);

  /**
   * @brief publish to webui live system info
   * i.e. free ram, uptime, etc... 
   */
  void embuistatus();
  void set_reboot(Interface *interf, JsonObject *data, const char* action);
  void set_hostname(Interface *interf, JsonObject *data, const char* action);
  void set_datetime(Interface *interf, JsonObject *data, const char* action);
  void set_cfgclear(Interface *interf, JsonObject *data, const char* action);

  /**
   * @brief default main_page with a simple "settings" menu entry
   * 
   */
  void page_main(Interface *interf, JsonObject *data, const char* action);
}   // end of "namespace basicui"