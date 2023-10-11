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

// Strings

/*
    перенакрываемая функция для добавления пользовательских пунктов в меню "Настройки"
    вызывается в конце section_settings_frame()
*/
void __attribute__((weak)) user_settings_frame(Interface *interf, JsonObject *data);

/*
    A namespace with functions to handle basic EmbUI WebUI interface
*/
namespace basicui {

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
  void add_sections();

  /**
   * This code adds "Settings" section to the MENU
   * it is up to you to properly open/close Interface json_section
   */
  void menuitem_options(Interface *interf, JsonObject *data);
  inline void opt_setup(Interface *interf, JsonObject *data){ menuitem_options(interf, data); };       // deprecated

  void show_section(Interface *interf, JsonObject *data);
  void block_settings_gnrl(Interface *interf, JsonObject *data);
  void block_settings_netw(Interface *interf, JsonObject *data);
  void block_settings_mqtt(Interface *interf, JsonObject *data);
  void block_settings_time(Interface *interf, JsonObject *data);
  void block_settings_sys(Interface *interf, JsonObject *data);
  void section_settings_frame(Interface *interf, JsonObject *data);
  void set_settings_wifi(Interface *interf, JsonObject *data);
  void set_settings_wifiAP(Interface *interf, JsonObject *data);
  void set_settings_mqtt(Interface *interf, JsonObject *data);
  void set_settings_time(Interface *interf, JsonObject *data);
  void set_language(Interface *interf, JsonObject *data);
  void embuistatus(Interface *interf);

  /**
   * @brief publish to webui live system info
   * i.e. free ram, uptime, etc... 
   */
  void embuistatus();
  void set_reboot(Interface *interf, JsonObject *data);
  void set_hostname(Interface *interf, JsonObject *data);
  void set_datetime(Interface *interf, JsonObject *data);
  void set_cfgclear(Interface *interf, JsonObject *data);
    //uint8_t uploadProgress(size_t len, size_t total);

}   // end of "namespace basicui"