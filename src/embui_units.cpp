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

#include "EmbUI.h"
#include "embui_units.hpp"
#include "nvs_handle.hpp"
#include "embui_log.h"


// ****  EmbUIUnit methods

void EmbUIUnit::getConfig(JsonObject obj) const {
  LOGD(P_Unit, printf, "getConfig for unit:%s\n", label);
  generate_cfg(obj);
}

void EmbUIUnit::setConfig(JsonVariantConst cfg){
  LOGD(P_Unit, printf, "%s: setConfig()\n", label);

  // apply supplied configuration to unit 
  load_cfg(cfg);
  // save supplied config to NVS
  save();
}

void EmbUIUnit::load(){
  JsonDocument doc;
  embuifs::deserializeFile(doc, mkFileName().c_str());
  load_cfg(  use_shared_file ? doc[label] : doc);
  start();
}

void EmbUIUnit::save(){
  JsonDocument doc;
  embuifs::deserializeFile(doc, mkFileName().c_str());
  if (doc.isNull())
    doc.to<JsonObject>();

  JsonObject o;
  if (  use_shared_file){
    o = doc[label].isNull() ? doc[label].to<JsonObject>() : doc[label];
  } else 
    o = doc.as<JsonObject>();

  getConfig(  use_shared_file ? doc[label].to<JsonObject>() : doc.to<JsonObject>());
  LOGD(P_Unit, printf, "writing cfg to file: %s\n", mkFileName().c_str());
  embuifs::serialize2file(doc, mkFileName().c_str());
}

String EmbUIUnit::mkFileName(const char* id, const char* path){
  String fname( path );
  // append namespace prefix if set
  if (ns){
    fname += ns;
    fname += (char)0x5f;  // '_' char
  }

  if ( use_shared_file){
    fname += id ? id : label;
  } else {
    if (id){
      fname += id;
      fname += (char)0x5f;  // '_' char
    }
    fname += label;
  }

  fname += ".json";
  return fname;
}

void EmbUIUnit::mkEmbUIpage(Interface *interf, JsonVariantConst data, const char* action){
  std::string key(T_embuium);
  key += (char)0x2e;          // '.' char
  if (ns){
    key.append(ns);
    key += (char)0x2e;        // '.' char
  }
  key += P_unit,
  key += (char)0x2e;          // '.' char
  key += label;
  key += (char)0x2e;          // '.' char
  key += T_page;
  key += (char)0x2e;          // '.' char

  if (data.is<const char*>()){
    // if req contains additional specificator, use it!
    key += data.as<const char*>();
    // todo: here must be a callback for derived class own implementation
  } else {
    // otherwise build default config page
    key += T_cfg;
  }

  // load Unit's structure from a EmbUI's UI data
  interf->json_frame_interface();
  interf->json_section_uidata();
  interf->uidata_pick( key.c_str() );
  interf->json_frame_flush();
  // serialize and send unit's configuration as a 'value' frame
  JsonDocument doc;
  getConfig(doc.to<JsonObject>());
  interf->json_frame_value(doc);

  // if this unit is multi-preset, then send current preset's value
  if (presetsAvailable()){
    // create action string "set_embuium_{namespace}_unit_{unit_lbl}_presetsw"
    std::string s(T_set_embuium_);
    if (ns){
      s.append(ns);
      s += (char)0x5f;  // '_' char
    }
    key += P_unit,
    s += (char)0x5f;    // '_' char
    s += getLabel();
    s += (char)0x5f;    // '_' char
    s += "_presetsw";   // action suffix
    interf->value(s, getCurrentPresetNum());
  }
  interf->json_frame_flush();
}


// ****  EmbUIUnit_Presets methods

void EmbUIUnit_Presets::switchPreset(int32_t idx, bool keepcurrent){
  // check I do not need to load preset's config
  if (keepcurrent){
    if (idx >= 0 && idx < EMBUI_UNIT_DEFAULT_NUM_OF_PRESETS)
      _presetnum = idx;
    return;
  }

  JsonDocument doc;
  embuifs::deserializeFile(doc, mkFileName().c_str());

  // restore last used preset if specified one is wrong or < 0
  if (idx < 0 || idx >= EMBUI_UNIT_DEFAULT_NUM_OF_PRESETS)
    _presetnum = doc[T_last_preset];
  else
    _presetnum = idx;


  LOGD(P_Unit, printf, "%s switch preset:%d\n", label, _presetnum);
  JsonArray presets = doc[T_presets].as<JsonArray>();
  // load name
  JsonVariant v = presets[_presetnum][P_label];
  if (v.is<const char*>())
    _presetname = v.as<const char*>();
  else {
    _presetname = T_preset;
    _presetname += idx;
  }
  // load unit's config
  load_cfg(presets[_presetnum][T_cfg]);
  start();
}

void EmbUIUnit_Presets::save(){
  JsonDocument doc;
  embuifs::deserializeFile(doc, mkFileName().c_str());

  JsonVariant arr = doc[T_presets].isNull() ? doc[T_presets].to<JsonArray>() : doc[T_presets];
  // if array does not have proper num of objects, prefill it with empty ones
  if (arr.size() < EMBUI_UNIT_DEFAULT_NUM_OF_PRESETS){
    size_t s = arr.size();
    JsonObject empty;
    while (s++ != EMBUI_UNIT_DEFAULT_NUM_OF_PRESETS){
      arr.add(empty);
    } 
  }

  // generate config to current preset cell
  JsonObject o = arr[_presetnum].to<JsonObject>();
  o[P_label] = _presetname;
  getConfig(o[T_cfg].to<JsonObject>());    // place config under {"cfg":{}} object

  doc[T_last_preset] = _presetnum;

  LOGD(P_Unit, printf, "%s: writing cfg to file\n", label);
  embuifs::serialize2file(doc, mkFileName().c_str());
}

void EmbUIUnit_Presets::mkEmbUIpage(Interface *interf, JsonVariantConst data, const char* action){
  // load generic page
  EmbUIUnit::mkEmbUIpage(interf, data, action);

  // in addition need to update preset's drop-down selector
  mkEmbUI_preset(interf);
}

void EmbUIUnit_Presets::mkEmbUI_preset(Interface *interf){
  interf->json_frame_interface();
  interf->json_section_content();

  // make drop-down selector's id
  std::string id(T_set_embuium_);
  if (ns){
    id.append(ns);
    id += (char)0x5f;  // '_' char
  }
  id += getLabel();

  JsonVariant d = interf->select(id.c_str(), getCurrentPresetNum(), P_EMPTY, true);
  // fill in drop-down list with available presets
  mkPresetsIndex(d[P_block]);
  interf->json_frame_flush();
}

size_t EmbUIUnit_Presets::mkPresetsIndex(JsonArray arr){
  JsonDocument doc;
  embuifs::deserializeFile(doc, mkFileName().c_str());

  JsonArray presets = doc[T_presets];

  size_t idx{0};
  String p; 
  for(JsonVariant v : presets) {
    JsonObject d = arr.add<JsonObject>();

    // check if preset config and label exists indeed
    if (v[P_label].is<JsonVariant>()){
      p = idx;
      p += " - ";
      p += v[P_label].as<const char*>();
    } else {
      // generate "preset1" string
      p = T_preset;
      p += idx;
    }

    d[P_label] = p;
    d[P_value] = idx;
    ++idx;
  }

  LOGD(P_Unit, printf, "make index of %u presets\n", idx);

  return idx;
}



// ****  EmbUI_Unit_Manager methods

EmbUI_Unit_Manager::EmbUI_Unit_Manager(const char* name_space) : ns(name_space), a_getters("get_embuium_"), a_setters("set_embuium_") {
  a_getters.append(name_space);
  a_getters.append(T__asterisk);
  a_setters.append(name_space);
  a_setters.append(T__asterisk);
};

void EmbUI_Unit_Manager::setHandlers(){
  // generic setter
  embui.action.add(a_setters.c_str(), [this](Interface *interf, JsonVariantConst data, const char* action){ _unit_setter(interf, data, action); });

  // generic getter
  embui.action.add(a_getters.c_str(), [this](Interface *interf, JsonVariantConst data, const char* action){ _unit_getter(interf, data, action); });
}

void EmbUI_Unit_Manager::unsetHandlers(){
  embui.action.remove(a_setters.c_str());
  embui.action.remove(a_getters.c_str());
}


void EmbUI_Unit_Manager::getConfig(std::string_view label, JsonObject obj){
  LOGV(T_UnitMgr, printf, "getConfig for: %s\n", label.data());   // in general it's wrong to pass sv's data, but this sv was instantiated from a valid null-terminated string, so should be OK

  auto i = std::find_if(units.begin(), units.end(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  if ( i != units.end() ) {
    (*i)->getConfig(obj);
    // if Unit has presets, need to add preset's index value
    if ((*i)->presetsAvailable()){
      // drop-down selector shoil have id "set_embuium_{namespace}_{unit_lbl}_presetsw"
      std::string s(T_set_embuium_);
      s.append(ns);
      s.append(1, (char)0x5f);  // '_'
      s.append(label);
      s.append(T_presetsw);
      obj[s] = (*i)->getCurrentPresetNum();
    }
    return;
  }

  // unit instance is not created, spawn a unit and call to return it's config again
  // not sure if that is needed, it could loop if unit creation fails
  //spawn(label);
  //getConfig(label, obj);
}

void EmbUI_Unit_Manager::setConfig(std::string_view label, JsonVariantConst cfg){
  LOGD(T_UnitMgr, printf, "setConfig for: %s\n", std::string(label).c_str());   // in general it's wrong to pass sv's data, but this sv was instantiated from a valid null-terminated string, so should be OK

  auto i = std::find_if(units.begin(), units.end(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  if ( i == units.end() ) {
    LOGV(T_UnitMgr, println, "unit does not exist, spawning a new one");
    // such unit does not exist currently, spawn a new one and run same call again, cause we can't bew sure it was created
    spawn(label);
    auto j = std::find_if(units.begin(), units.end(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
    if ( j == units.end() ) return; // failed to spawn a unit
    // now I can set new configuration
    (*j)->setConfig(cfg);
    return;
  }

  // apply and save unit's configuration
  (*i)->setConfig(cfg);
}

void EmbUI_Unit_Manager::getUnitsStatuses(Interface *interf) const {
  if (!units.size()) return;

  interf->json_frame_value();
  // generate values with each unit's state started/not started
  // unit's state id format "set_embuium_{namespace}_unit_{unit_lbl}_state"
  for ( auto i = units.cbegin(); i != units.cend(); ++i){
    std::string s(T_set_embuium_);
    s.append(ns);
    s.append(1, (char)0x5f);  // '_'
    s.append(P_unit);
    s.append(1, (char)0x5f);  // '_'
    s.append((*i)->getLabel());
    s.append(T__state);
    LOGD(T_UnitMgr, printf, "Unit enabled:%s\n", s.c_str());
    interf->value( s, true);
  }
  // not needed
  //interf->json_frame_flush();
}

EmbUIUnit* EmbUI_Unit_Manager::getUnitPtr(std::string_view label){
  if (!units.size()) return nullptr;
  auto i = std::find_if(units.begin(), units.end(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  if ( i == units.end() )
    return nullptr;

  return (*i).get();
}

bool EmbUI_Unit_Manager::getUnitStatus(const char* label) const {
  auto i = std::find_if(units.cbegin(), units.cend(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  return (i != units.end());
}

bool EmbUI_Unit_Manager::getUnitStatus(std::string_view label) const {
  auto i = std::find_if(units.cbegin(), units.cend(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  return (i != units.end());
}

void EmbUI_Unit_Manager::switchPreset(std::string_view label, int32_t idx){
  auto i = std::find_if(units.begin(), units.end(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  if (i != units.end())
    (*i)->switchPreset(idx);
}

uint32_t EmbUI_Unit_Manager::presetsAvailable(const char* label) const {
  auto i = std::find_if(units.begin(), units.end(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  if (i != units.end())
    return (*i)->presetsAvailable();

  return 0;
}

void EmbUI_Unit_Manager::_make_embui_page(Interface *interf, JsonVariantConst data, const char* action, std::string_view unit){
  // check if such unit instance exist, if not - then spawn it
  //if (!getUnitStatus(unit))
  //  spawn(unit);

  auto p = getUnitPtr(unit);
  if (p){
    // call unit's method to build it's UI page
    LOGD(T_UnitMgr, printf, "_make_embui_page:%s\n", unit.data()); // it's not right, but pretty safe, 'cause 'unit' is just a substring of null terminated string
    p->mkEmbUIpage(interf, data, action);
  } else {
    LOGD(T_UnitMgr, printf, "unit '%s' not found\n", unit.data()); // it's not right, but pretty safe, 'cause 'unit' is just a substring of null terminated string
  }
}

void EmbUI_Unit_Manager::_unit_setter(Interface *interf, JsonVariantConst data, const char* action){
  std::string_view sv(action);  //  "set_embuium_{namespace}_unit_{unit_lbl}_"
  // trim prefix 12 (set_embuium_) + 1 (_) + len of ns
  sv.remove_prefix(std::string_view("set_embuium_").length() + strlen(ns) + std::string_view("_unit_").length());    // now it's "{unit_lbl}_some_action"

  // check for "set state"
  if (sv.ends_with(T__state)){
    sv.remove_suffix(std::string_view(T__state).length());  // this is constexpr
    // try start or stop unit
    data.as<bool>() ? start(sv) : stop(sv);
    return;
  }

  // check for "set cfg" - setting configuration
  if (sv.ends_with("_cfg")){
    sv.remove_suffix(std::string_view("_cfg").length());  // this is constexpr
    setConfig(sv, data);
    return;
  }

  // check for "set swpreset" - switch preset
  if (sv.ends_with(T_presetsw)){
    sv.remove_suffix(std::string_view(T_presetsw).length());  // this is constexpr
    switchPreset(sv, data);
    // send to webUI refreshed unit's config
    JsonDocument doc;
    getConfig(sv, doc.to<JsonObject>());
    interf->json_frame_value(doc);
    interf->json_frame_flush();
    return;
  }

  // check for "set presetname" - rename preset
  if (sv.ends_with("_presetname")){
    sv.remove_suffix(std::string_view("_presetname").length());  // this is constexpr
    _set_unit_preset_lbl(interf, data, sv);
    return;
  }

  // check for "set presetclone" - clone preset
  if (sv.ends_with("_presetname")){
    sv.remove_suffix(std::string_view("_presetname").length());  // this is constexpr
    _set_unit_preset_lbl(interf, data, sv);
    return;
  }
  
}

void EmbUI_Unit_Manager::_unit_getter(Interface *interf, JsonVariantConst data, const char* action){
  LOGD(T_UnitMgr, printf, "_unit_getter: '%s'\n", action);
  std::string_view sv(action);  //  "get_embuium_{namespace}_{unit_lbl}_"
  
  // check for "_page" reqest - i.e. load some unit's UI page - action 'get_embuium_{namespace}_unit_{unit_lbl}_page'
  if (sv.ends_with("_page")){
    sv.remove_prefix(std::string_view("get_embuium_").length() + strlen(ns) + std::string_view("_unit_").length());     // chop off prefix till {unit_lbl}
    sv.remove_suffix(std::string_view("_page").length());                             // chop off suffix, leaving only unit's label
    _make_embui_page(interf, data, action, sv);
/*
    std::string s(T_embuium);
    s += (char)0x2e;          // '.' char
    s += ns;
    s += (char)0x2e;          // '.' char
    s += P_unit;
    s += (char)0x2e;          // '.' char,  now it is 'embuium.{namespace}.unit.'
    std::string_view unit(action);
    s += unit;                // now it is 'embuium.{namespace}.unit.{unit_lbl}.page'

    if (data.is<const char*>()){
      // req page, and pass call to the unit's own handler
      // todo
    } else {
      // build default config page
      s += (char)0x2e;          // '.' char
      s += T_cfg;

      // load unti's page from UI user data - 'embuium_{namespace}_unit_{unit_lbl}_page'
      interf->json_frame_interface();
      interf->json_section_uidata();
      interf->uidata_pick(s.c_str());
      interf->json_frame_value();
      interf->json_object_get()
      interf->json_frame_flush();
      return;  
    }
*/
  }

}

void EmbUI_Unit_Manager::_set_unit_preset_lbl(Interface *interf, JsonVariantConst data, std::string_view unit_lbl){
  JsonVariantConst v = data["preset_newlbl"];
  if (!v.is<const char*>()) return;

  auto i = std::find_if(units.begin(), units.end(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(unit_lbl));
  if (i == units.end()) return;

  (*i)->setPresetLabel(v);
  (*i)->save();
  (*i)->mkEmbUI_preset(interf);
}

void EmbUI_Unit_Manager::_set_unit_preset_clone(Interface *interf, JsonVariantConst data, std::string_view unit_lbl){
  JsonVariantConst v = data["clone_to"];
  if (!v.is<int>()) return;

  auto i = std::find_if(units.begin(), units.end(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(v.as<const char*>()));
  if (i == units.end()) return;

  (*i)->switchPreset(v, true);
  (*i)->save();
  (*i)->mkEmbUI_preset(interf);
}


//  ***** EmbUI_Unit_Manager_NVS *****
void EmbUI_Unit_Manager_NVS::begin(){
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(ns, NVS_READONLY, &err);
  if (err != ESP_OK) {
    // if NVS handle is unavailable then just quit
    LOGD(T_UnitMgr, printf, "Err opening NVS handle:%s (%d)\n", esp_err_to_name(err), err);
    return;
  }

  for (auto &l : units_list){
    uint32_t state = 0; // value will default to 0, if not yet set in NVS
    handle->get_item(l, state);
    LOGI(T_UnitMgr, printf, "Boot state for %s: %u\n", l, state);
    // if saved state is >0 then unit is active, we can restore it
    if (state)
      spawn(l);
  }
}

void EmbUI_Unit_Manager_NVS::start(std::string_view label){
  LOGD(T_UnitMgr, printf, "start: %s\n", label.length() ? label.data() : "");   // in general it's wrong to pass sv's data, but this sv was instantiated from a valid null-terminated string, so should be OK, just could have some unexpected trailing suffix
  // check if such unit is already spawned
  auto i = std::find_if(units.cbegin(), units.cend(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  if ( i != units.cend() ){
    LOGD(T_UnitMgr, println, "already running");
    return;
  }

  // create a specific unit y label
  spawn(label);
}

void EmbUI_Unit_Manager_NVS::stop(std::string_view label){
  if (!label.length()) return;
  // check if such unit is already spawned
  auto i = std::find_if(units.cbegin(), units.cend(), EmbUIUnit_MatchLabel<EmbUIUnit_pt>(label));
  if ( i != units.cend() ){
    LOGI(T_UnitMgr, printf, "deactivate %s\n", label.data()); // in general it's wrong to pass sv's data, but this sv was instantiated from a valid null-terminated string, so should be OK, just could have some unexpected trailing suffix
    // stop the Unit first
    (*i)->stop();
    units.erase(i);
    // remove state flag from NVS
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(ns, NVS_READWRITE);
    handle->erase_item(std::string(label).c_str()); // here need to be on a safe side
  }
}

void EmbUI_Unit_Manager_NVS::spawn(std::string_view label){
  LOGD(T_UnitMgr, printf, "spawn: %s\n", label.data()); // in general it's wrong to pass sv's data, but this sv was instantiated from a valid null-terminated string, so should be OK, just could have some unexpected trailing suffix
  // spawn a new unit based on label
  std::unique_ptr<EmbUIUnit> w = _maker(label);
  if (!w) return;   // maker was unable to produce an object

  // load unit's config from file
  w->load();
  // move it into container
  units.emplace_back(std::move(w));

  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle(ns, NVS_READWRITE, &err);

  if (err != ESP_OK)
    return;

  uint32_t state = 1;
  handle->set_item(std::string(label).c_str(), state);
}

