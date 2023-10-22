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
    enum class page : uint8_t {
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

  void show_uipage(Interface *interf, JsonObject *data, const char* action);
  void page_settings_gnrl(Interface *interf, JsonObject *data, const char* action);
  void page_settings_netw(Interface *interf, JsonObject *data, const char* action);
  void page_settings_mqtt(Interface *interf, JsonObject *data, const char* action);
  void page_settings_time(Interface *interf, JsonObject *data, const char* action);
  void page_settings_sys(Interface *interf, JsonObject *data, const char* action);

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