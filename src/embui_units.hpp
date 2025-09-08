/*
  This file is a part of EmbUI project
  https://github.com/vortigont/EmbUI

  Copyright © 2023-2025 Emil Muratov (vortigont)

  EmbUI is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  EmbUI is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with EmbUI.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    A generic container class to simplify serialization/deserialization configuration of
    arbitrary units to integrate it with EmbUI's web UI and action handlers
*/

#pragma once
#include "ui.h"
#include "embui_constants.h"

/*

  UI data structure

  {
    "embuium":{                           // root container
      $namespace:{                        // multiple namespaces
        "list":{},                        // should be an index page for units list, implemented in derived implementations
        "unit":{                          // dict with units
          $unit_lbl:{                     // some specific unit
            "page":{                      // pages dict
              "idx":{},                   // an index page with unit's title page if any
              "cfg":{}                    // unit's main configuration page
            }
          }
        } 
      }
    }
  }


// a list of registered EmbUI actions for an instance of EmbUI_Unit_Manager

"embuium.{namespace}.list"                            // Builds a page with available units in a specified namespace, must be registered and implemented in derived classes
"embuium_{namespace}_*"                               // generic mask for namespace
"get_embuium_{namespace}_*"                           // getters mask
"set_embuium_{namespace}_*"                           // setters mask

// used actions
"set_embuium_{namespace}_unit_{unit_lbl}_state"            // start / stop unit with boolean
"set_embuium_{namespace}_unit_{unit_lbl}_cfg"              // apply unit's configuration
"set_embuium_{namespace}_unit_{unit_lbl}_presetsw"         // switch unit's configuration preset, preset index is extracted from 'data' object
"set_embuium_{namespace}_unit_{unit_lbl}_presetname"       // rename preset configuration, new name is extracted from data["preset_newlbl"]
"set_embuium_{namespace}_unit_{unit_lbl}_presetclone"      // clone preset configuration, new preset's index is extracted from data["clone_to"]
"get_embuium_{namespace}_unit_{unit_lbl}_page"             // show unit's UI page from UserData, the default page is "cfg", other page labes are extracted from 'data' object

*/


// keep this value at some sane number so not to bloat json array with empty presets
#define EMBUI_UNIT_DEFAULT_NUM_OF_PRESETS 10

static constexpr const char* T_UnitMgr                = "UnitMgr";
static constexpr const char* P_embuium                = "embuium";
static constexpr const char* P_embuium_idx_page_tlp   = "embuium.%ns.list";
static constexpr const char* P_Unit                   = "Unit";
static constexpr const char* P_unit                   = "unit";
static constexpr const char* T__idx                   = "_idx";
static constexpr const char* T__asterisk              = "_*";
static constexpr const char* T_cfg                    = "cfg";
static constexpr const char* T_embuium                = "embuium";
static constexpr const char* T_embuium_               = "embuium_";
static constexpr const char* T_set_embuium_           = "set_embuium_";
static constexpr const char* T_last_preset            = "last_preset";
static constexpr const char* T_page                   = "page";
static constexpr const char* T__page_idx              = "_page_idx";
static constexpr const char* T_page_idx               = ".page.idx";
static constexpr const char* T_preset                 = "preset";
static constexpr const char* T_presets                = "presets";
static constexpr const char* T_presetsw               = "_presetsw";
static constexpr const char* T__state                 = "_state";

// literals hashing
// https://learnmoderncpp.com/2020/06/01/strings-as-switch-case-labels/

inline constexpr auto hash_djb2a(const std::string_view sv) {
    unsigned long hash{ 5381 };
    for (unsigned char c : sv) {
      hash = ((hash << 5) + hash) ^ c;
    }
    return hash;
}

inline constexpr auto operator"" _sh(const char *str, size_t len) {
    return hash_djb2a(std::string_view{ str, len });
}


/**
 * @brief an abstract class to implement dynamically loaded components or units
 * practically it is just a class that is able to load/save it's state serialized,
 * has a periodic timer ticker and could attach/detach to event bus
 * 
 */
class EmbUIUnit {

protected:
  /**
   * @brief unit's label or "name", can't be changed once defined
   * @note must not be a 'null'! used as a key to serialize unit's data or as a file name's suffix
   * 
   */
  const char* const label;

  // Unit's namespace
  const char* const ns;

  /**
   * @brief shared file for data serialization
   * if 'true' - then use shared file to serialize unit's data under a 'label' named dictionary object,
   * if 'false - serialize unit's data into a dedicated file named 'prefix_${label}.json'
   * 
   */
  const bool use_shared_file;

public:

  /**
   * @brief Construct a new Generic Unit object
   * 
   * @param label - unit label identifier
   */
  EmbUIUnit(const char* label, const char* name_space = NULL, bool default_cfg_file = true) : label(label), ns(name_space), use_shared_file(default_cfg_file) {};

  virtual ~EmbUIUnit(){};

  /**
   * @brief load unit's config from persistent storage and calls start()
   * 
   */
  virtual void load();

  /**
   * @brief save current unit's configuration to file
   * 
   */
  virtual void save();

  // start or initialize unit
  virtual void start() = 0;

  // stop or deactivate unit without destroying it
  virtual void stop() = 0;

  /**
   * @brief Get unit's configuration packed into a nested json object ['unit_label']
   * used to feed configuration values to WebUI/MQTT, calls virtual generate_cfg() under the hood
   */
  void getConfig(JsonObject obj) const;

  /**
   * @brief Set unit's configuration packed into json object
   * this call will also SAVE supplied configuration to persistent storage
   */
  void setConfig(JsonVariantConst cfg);

  /**
   * @brief Get unit's Label
   * 
   * @return const char* 
   */
  const char* getLabel() const { return label; }

  // Configuration presets handling

  /**
   * @brief switch to specific preset number
   * 
   * @param value preset number to swich to
     * @param keepcurrent - if true, then do not load preset's setting, just change the index and keep current config
   */
  virtual void switchPreset(int32_t value, bool keepcurrent = false){};

  /**
   * @brief Get the Current preset Num value
   * 
   * @return uint32_t 
   */
  virtual uint32_t getCurrentPresetNum() const { return 0; };

  /**
   * @brief returns number of available slots for stored presets
   * 
   * @return uint32_t number of slots, if 0 is returned then presets are not supported
   */
  virtual uint32_t presetsAvailable() const { return 0; }

  /**
   * @brief set a name for currently loaded preset
   * 
   * @param lbl 
   */
  virtual void setPresetLabel(const char* lbl){};

  /**
   * @brief Construct an EmbUI page with unit's state/configuration
   * 
   * @param interf 
   * @param data 
   * @param action 
   */
  virtual void mkEmbUIpage(Interface *interf, JsonVariantConst data, const char* action);

  /**
   * @brief generate content frame for presets drop-down list
   * 
   * @param interf 
   */
  virtual void mkEmbUI_preset(Interface *interf){};

  /**
   * @brief fills provided array with a list of available config presets
   * it generates data for drop-down selector list for EmbUI in a form of:
   * [ { "value":0, "label":"Somepreset" } ]
   * 
   * @param arr Json array reference to fill-in
   * @return size_t number of elements in index
   */
  virtual size_t mkPresetsIndex(JsonArray arr) { return 0; };


protected:

  /**
   * @brief derived method should generate object's configuration into provided JsonVariant
   * 
   * @param cfg 
   * @return JsonVariantConst 
   */
  virtual void generate_cfg(JsonVariant cfg) const = 0;

  /**
   * @brief load configuration from a json object
   * method should be implemented in derived class to process
   * class specific json object
   * @param cfg 
   */
  virtual void load_cfg(JsonVariantConst cfg) = 0;

  /**
   * @brief generate configuration file's name
   * @note path should be an absolute path on FS
   * @note for shared config filename would be '/{path}/{id}.json', if id is not NULL, and '/{path}/{label}.json' otherwise
   * for Units with dedicated config filename would be '/{path}/{id}_{label}.json'
   * @param prefix prepended to the file's name
   * @return String 
   */
  String mkFileName(const char* id = NULL, const char* path = "/");

};



using EmbUIUnit_pt = std::unique_ptr<EmbUIUnit>;



/**
 * @brief Unit that supports configuration presets that could be stored and switched on-demand
 * i.e. themes or so...
 * 
 */
class EmbUIUnit_Presets : public EmbUIUnit {
  const size_t _max_presets; 
  int32_t _presetnum{0};
  String _presetname;

  void _load_preset(int idx);

public:
  EmbUIUnit_Presets(const char* label, const char* name_space = NULL, size_t max_presets = EMBUI_UNIT_DEFAULT_NUM_OF_PRESETS) : EmbUIUnit(label, name_space, false), _max_presets(max_presets) {}

  /**
   * @brief load unit's config from persistent storage and calls start()
   * here it loads last used preset
   */
  void load() override final { switchPreset(-1); };

  // save current Unit's presest to file
  void save() override final;

  /**
   * @brief switch to specific preset number
   * loads preset config from file, if specified argument is <0 or wrong, loads last used preset
   * @param value 
   * @param keepcurrent - if 'true' then only preset index is changed, but configuration is not loaded, could be used to clone presets
   */
  void switchPreset(int32_t idx, bool keepcurrent = false) override final;

  /**
   * @brief Get the Current preset Num value
   * 
   * @return uint32_t 
   */
  uint32_t getCurrentPresetNum() const override final { return _presetnum; };

  /**
   * @brief returns number of available slots for stored presets
   * 
   * @return uint32_t number of slots, if 0 is returned then presets are not supported
   */
  uint32_t presetsAvailable() const override { return _max_presets; }

  /**
   * @copydoc EmbUIUnit::setPresetLabel(const char* lbl)
   */
  void setPresetLabel(const char* lbl) override { if (lbl) _presetname = lbl; };

  /**
   * @brief Construct an EmbUI page with unit's state/configuration
   * this override will additionally load preset's selector list
   */
  virtual void mkEmbUIpage(Interface *interf, JsonVariantConst data, const char* action) override;

  /**
   * @copydoc EmbUIUnit::mkEmbUI_preset(Interface *interf)
   */
  virtual void mkEmbUI_preset(Interface *interf) override;

  /**
   * @copydoc EmbUIUnit::mkPresetsIndex(JsonArray arr)
   */
  size_t mkPresetsIndex(JsonArray arr) override;

};


/**
 * @brief a container object to spawn/destroy EmbUI Units on start/demand
 * and manages configuration/presets load/save
 * 
 */
class EmbUI_Unit_Manager {
protected:
  /**
   * @brief Manager's namespace
   * a unique namespace that is used to identify filenames (via prefix) NVS namespace, etc.. related
   * to managed objects. Should be unique for different instances of class
   * 
   */
  const char* ns;
  // embui actions placeholders
  std::string a_getters;
  std::string a_setters;

public:
  // callback function that could create Units by label
  using unit_maker_t = std::function< EmbUIUnit_pt&& (std::string_view unit_id)>;

  EmbUI_Unit_Manager(const char* name_space);

  virtual ~EmbUI_Unit_Manager(){ unsetHandlers(); };

  // copy semantics is not implemented
  EmbUI_Unit_Manager(const EmbUI_Unit_Manager&) = delete;
  EmbUI_Unit_Manager& operator=(const EmbUI_Unit_Manager &) = delete;
  EmbUI_Unit_Manager(EmbUI_Unit_Manager &&) = delete;
  EmbUI_Unit_Manager & operator=(EmbUI_Unit_Manager &&) = delete;

    /**
   * @brief bootstrap - load units list from some persistent storage,
   * depending on saved state spawn needed units
   * 
   */
  virtual void begin() = 0;

  /**
   * @brief start unit
   * 
   * @param label 
   */
  virtual void start(std::string_view label) = 0;


  /**
   * @brief Stop specific unit if it's instance does exist
   * 
   * @param label unit label to start
   */
  virtual void stop(std::string_view label) = 0;

  const char* getNameSpace() const { return ns; }

  /**
   * @brief load unit's configuration into provided JsonObject
   * usually called from a WebUI/MQTT handler
   * @param obj 
   * @param label 
   */
  void getConfig(std::string_view label, JsonObject obj);

  /**
   * @brief Set the Configuration for specifit unit object
   * 
   * @param label 
   * @param cfg 
   */
  void setConfig(std::string_view label, JsonVariantConst cfg);

  /**
   * @brief generate Interface values object representing boolen states
   * of currently active/inactive units
   * 
   * @param interf 
   */
  virtual void getUnitsStatuses(Interface *interf) const;

  /**
   * @brief Get state of the specific Unit active/inactive
   * 
   * @param label unit's label
   * @return true
   * @return false 
   */
  bool getUnitStatus(const char* label) const;
  bool getUnitStatus(std::string_view label) const;

  /**
   * @brief Get pointer to the instance of an active Unit by it's label
   * returns nullptr if requested unit is not currently runnning
   * @note a care should be taken when unit pointer is used outside of manager object,
   * currently there is no exclusive locking performed and unit instance could be deleted at any time via other call
   * 
   * @param[in] label 
   * @return EmbUIUnit* 
   */
  EmbUIUnit* getUnitPtr(std::string_view label);

  /**
   * @brief switch unit's configuration preset
   * 
   * @param label 
   * @param idx 
   */
  void switchPreset(std::string_view label, int32_t idx);

  uint32_t presetsAvailable(const char* label) const;

protected:
  // unit instances container
  std::list<EmbUIUnit_pt> units;

  /**
   * @brief spawn a new instance of a unit with supplied config
   * used with configuration is suplied via webui for non existing units
   * @param label 
   */
  virtual void spawn(std::string_view  label) = 0;

  /**
   * @brief register handlers for EmbUI and event bus
   * required to enable EmbUI control actions for UI pages, etc...
   * 
   */
  void setHandlers();

  // unregister handlers
  void unsetHandlers();



private:
  /**
   * @brief a callback method for EmbUI to generate units UI pages
   * will render a default unit's setup/state page based on serialized configuration data 
   * 
   * @param interf 
   * @param data 
   * @param action 
   */
  void _make_embui_page(Interface *interf, JsonVariantConst data, const char* action, std::string_view unit);

  // EmbUI's handler - generic setter, catches "set_embuium_{namespace}_*" actions
  void _unit_setter(Interface *interf, JsonVariantConst data, const char* action);

  // EmbUI's handler - generic getter, catches "get_embuium_{namespace}_*" actions
  void _unit_getter(Interface *interf, JsonVariantConst data, const char* action);

  void _set_unit_cfg(Interface *interf, JsonVariantConst data, const char* action);

  // rename current preset for the unit
  void _set_unit_preset_lbl(Interface *interf, JsonVariantConst data, std::string_view unit_lbl);

  // save current config into another preset slot
  void _set_unit_preset_clone(Interface *interf, JsonVariantConst data, std::string_view unit_lbl);

};

/**
 * @brief manager that loads from a static vector config and stores units states in NVS
 * 
 */
class EmbUI_Unit_Manager_NVS : public EmbUI_Unit_Manager {
protected:
  const std::vector<const char*>& units_list;
  unit_maker_t _maker;

public:


  EmbUI_Unit_Manager_NVS(const char* name_space, const std::vector<const char*>& units, unit_maker_t maker) : EmbUI_Unit_Manager(name_space), units_list(units), _maker(maker) {}

  // copy semantics is not implemented
  EmbUI_Unit_Manager_NVS(const EmbUI_Unit_Manager_NVS&) = delete;
  EmbUI_Unit_Manager_NVS& operator=(const EmbUI_Unit_Manager_NVS &) = delete;
  EmbUI_Unit_Manager_NVS(EmbUI_Unit_Manager_NVS &&) = delete;
  EmbUI_Unit_Manager_NVS & operator=(EmbUI_Unit_Manager_NVS &&) = delete;
  
  void begin() override;

  /**
   * @brief Creates and loads unit
   * if label is not given, then start all units based on settings from NVRAM
   * 
   * @param label 
   */
  void start(std::string_view label) override;

  /**
   * @brief Stop specific unit if it's instance does exist
   * 
   * @param label 
   */
  void stop(std::string_view label) override;

protected:
  /**
   * @brief spawn a new instance of a unit with supplied config
   * used with configuration is suplied via webui for non existing units
   * @param label 
   */
  void spawn(std::string_view label) override;

};

// Unary predicate for Unit's label search match
template <class T>
class EmbUIUnit_MatchLabel {
    std::string_view _lookup;
public:
  explicit EmbUIUnit_MatchLabel(const char* label) : _lookup(label) {}
  explicit EmbUIUnit_MatchLabel(std::string_view& label) : _lookup(label) {}
    bool operator() (const T& item ){
        // T is EmbUIUnit_pt
        return _lookup.compare(item->getLabel()) == 0;
    }
};
