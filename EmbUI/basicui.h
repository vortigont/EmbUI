/*
    Here is a set of predefined WebUI elements for system settings setup like WiFi, time, MQTT, etc...
*/
#pragma once

#include "ui.h"

/**
 * List of UI languages in predefined i18n resources
 */
enum LANG : uint8_t {
    EN = (0U),
    RU = (1U),
};

extern uint8_t lang;

/*
    A namespace with functions to handle basic EmbUI WebUI interface
*/
namespace basicui {

    // numeric indexes for pages
    enum class page : uint16_t {
        main = 0,
        settings,
        network,
        datetime,
        mqtt,
        ftp,
        syssetup
    };

  /**
   * register handlers for system actions and setup pages
   * 
   */
  void register_handlers();

  /**
   * This code adds "Settings" section to the MENU
   * it is up to you to properly open/close Interface json_section
   */
  void menuitem_settings(Interface *interf);

  void show_uipage(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void page_settings_gnrl(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void page_settings_netw(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void page_settings_mqtt(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void page_settings_time(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void page_settings_sys(Interface *interf, JsonObjectConst data, const char* action = NULL);

  /**
   * @brief Build WebUI "Settings" page
   * it will create system settings page and call action for user callback to append user block to the settings page
   * 
   * 
   * @param interf 
   * @param data 
   * @param action 
   */
  void page_system_settings(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void set_settings_wifi(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void set_settings_wifiAP(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void set_settings_mqtt(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void set_settings_time(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void set_language(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void embuistatus(Interface *interf);

  /**
   * @brief publish to webui live system info
   * i.e. free ram, uptime, etc... 
   */
  void embuistatus();
  void set_sys_reboot(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void set_sys_hostname(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void set_sys_datetime(Interface *interf, JsonObjectConst data, const char* action = NULL);
  void set_sys_cfgclear(Interface *interf, JsonObjectConst data, const char* action = NULL);

  /**
   * @brief default main_page with a simple "settings" menu entry
   * 
   */
  void page_main(Interface *interf, JsonObjectConst data, const char* action = NULL);
}   // end of "namespace basicui"