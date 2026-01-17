## Changelog

### v4.3.1
 - basicui - change button types to literals
 - fix saving WiFi AP options
 - Using "extend id" for value frames

### v4.3.0
 - change callbacks type to JsonVariantConst
  now `data` argument passed to callback has JsonVariantConst
  it allows to pass simple data types as bools, ints, etc as-is and avoid
  embedding it into data[action] member
 - bugs
    - hostname was not saved properly
     - MQTT section exit button was not working
     - removed excessibe debuggin
 - embui.js support array objects in uidata
 - fix key name in language data
 - {"show_btn":false} property in "hidden" section would disable creation of show/hide button to control the visibility of section
 - RGB hex to RGB565 color conversion in color picker
 - Embui Units
 - fixed bug when hostname change was not saved unless some other change also was done
 - reformat log messages
 - Update html template with a new features
 - Bump TaskScheduler lib ver to 4.0


### v4.2.3
  - fix - mqtt disable was not saved in config file

### v4.2.2
 - ActionHandler accepts function arg as reference
 - fix: refactor NTPoDHCP
 - embuifs - make deserializeFile as templates
 - small time config related changes
 - fix AP password converted to 'null' if unconfigured
 - update lib deps


### v4.2.0
 - ArduinoJson 7.3 compliant
 - fix deprecated sntp_init() for Arduino core 3.x
 - add find,findIndex into lodash build
 - embuifs - add merge and deepmerge functions

ui.h
 - Frame send async js - refactor class for HTTP API, execute callbacks out of feeders chain

mqtt:
 - fix missing topic prefix when config contains "" value

js api:
 - on ui_data pick allow to override element's id with additional parameter
 - add findBlockElement() function to recoursively find elements in nested sections
 - on_js function call passes arbitrary argument and callee id to the function

template
 - add string types for button
 - align with FireLamp's template
 - fix 'disabled' state for checkbox

### v4.1.0
JS API
  - add "value"-type section, allow to interleave 'interface' frame with ui and data objects
  could be handy when creating sections with async xload'ed data, i.e. drop-down lists
NTP refactoring
some action handlers fixes

### v4.0.1
major js API braking changes. In fact it should all be in 4.0.0 :)
improved async xloads in ui data
added set/merge objects
fixed wrong scope in jscall pkg's
reworked basicui main page loading

### v4.0.0
- ArduinoJson to 7.2
- remove deprecated containskey()
* match WiFi event handler input types to Arduino core version
+ Implement language translation for UI data
+ update UI resources with lang translations Rus/Eng
+ make /data dir with web resources, obsolete resources/data.zip
+ Rework of Interface class, now JsonDocument could be manipulated in-place avoiding useless copies
  - Method JsonArray Interface::get_block() allows to get last section's block array
  - add new method Interface::make_new_object()
  - add Interface::json_frame_send() method, send accumulated data and clears memory while preserving sections stack
  - Interface methods return JsonObject on UI objects addition
  - reimplement Interface::button* methods
  - obsolete UIelement related objects
* refactor Interface::json* methods
  - added
  - Interface::json_block_get() to access block array
  - Interface::json_object_get() to access last added object
  - Interface::json_object_create() replaces Interface::make_new_object()
  - Interface::json_frame_add replaced with Interface::json_object_add 
  - Interface::json_frame_* methods new return reference to JsonObjectwith a frame (in fact private json member)
* replace FrameSend::send() with const char* argument
- reduce EmbUI::save* metods, remove backup file save/load, use bare minimum
* change actionCallback_t prototype to accept json data via JsonObjectConst value
- remove all EmbUI config related methods
- obsolete SSDP related code
* fix Interface::_json_frame_next()


### v3.2.4
+ Arduino Core 3.x support
+ add Core3 to CI builder
* updated htpm template
 - line'd selctions now could use class specifiers for each element
 - add optional 'disabled' property to checkboxes
 - add constbtn element
* fix some log errors
* js frontend: fix 'hidden' form fileds are checked for numeric value on sending to WS

### v3.2.3
* bump lib dependencies
+ frontend js: cast to numeric certain input fields on form send
* add platformio patch for flashz lib
* CI fix


### v3.2.2
* bump lib dependencies
* respack.sh - use zopfli to compress resources

### v3.2.0
+ code refactoring to use ArduinoJson 7 code style
* minor code syntax and style cleanup 

### v3.1.3
- removed LinkedList dependency, use std::list from STL instead
* ui.h remove default template params in method implementations

### v3.1.2
* Switch to mathieucarbou's for of AsynWebserver
+ ui_data pick section call can mutate stored UI objects
! rework log macro's
* update workflow actions

### v3.1.1
+ add noappend parameter to Interface::json_section_begin
* update default styles with more compact input fields
* switch AsynWebServer branch to master
+ implement Interface::json_frame_jscall() method
+ maker js lib - allow user defined function to process all unknown packer types
* js/maker.js update renderer.value, make sys UI objects loading async
* ws.onmessage skip reassembly for new  messages that arrives with 'final' flag
+ add Interface::jobject() method, add any user-constructed object to the frame
+ add RSSI value display on left-side menu
! fix bug when range sliders were not updated with 'value' packets
* update purecss to 3.0.0



### v3.1.0 - 2023.11.28
+ introduce UI objects storage on front-end
  This feature allows to pre-compile UI objects and store it in json file on file system.
  Front-end will fetch this data and keep the objects, while back-end could send a specific section with a request
  to get UI objects from the local storage on the front-end side.
  This will allow to save codespace on the backend for generating UI objects structure and just
  simply update retrieved objects with value data.

+ implement 'merge' strategy for uidata_xload sections (not fully tested yet)

* minor fixes and improvements in TimeProcessor
 - TimeProcessor::setTime() method accept value string with or withour seconds

+ render unknown non-main sections appending it to 'main'
    'interface' frame could contain 1st level sections without 'mian' flag,
    those sections were considered as a replacement content for the existing sections with same ID in DOM,
    so, if any non-main sections were found without any existing matching ID, those sections were silently dropped.
    From now on such sections are appended to 'main' and rendered below existing conted on the main page block

+ implement js api / uidata versioning checking
 - if version number  supplied in manifest/uidata_xload frames is newer then defined in js/json resource files,
   then notification message will be displayed at the top of main page letting user know that resource FS files are outdated

* bump jsapi to ver 3



### v3.0.0 - 2023.11.15
+ implement getters for Interface class
  - Interface::json_section_begin() methods now returns an JsonArrayConst object that points to the newly created section
    it could be used to inspect existing objects or serialize part of the Interface object
  - Interface::get_last_section() method return a const pointer to the last section in a stack
  - JsonObject Interface::get_last_object() method return and object of a recently created html element
    this method could be used to add/extend html object with arbitrary key:value objects that are not present in existing html element creation methods


* Interface::value(const ID id, const T val, bool html) will make a simple key:value pairs if 'html' is false
  -  i.e. if html parameter is false, an added items will be constructed {"device_pwrswitch":false},
    otherwise old behavior is used {"id":"device_pwrswitch", "value":false, "html": true}

* add constness to actionCallback_t parameter
  -  from now it's 'const JsonObject *data' to ensure that action data object can't not be changed inside action callbacks

+ implement section_xload()
    this function recoursevily looks through sections and picks ones with name 'xload'
    for such sections all html elements with 'url' object key will be side-load with object content into
    and object named 'block'. Same way as for xload() function.

* Interface - all member functions accept only 'const char*' as id parameter
  -  since js lib uses stringified id, it makes no sence for any other templaed types

+ add action id matching with wildcard prefix "*_some_id"
  -  works same way as wildcard suffix

+ FrameSendAsyncJS http API handler
  -  feeder is hooked to /api URI and processes API requests via HTTP
  -  only 'value' frame replies are supported due to inability to pipe multiple json onjects via single http request

+ implement FrameSendMQTT class for EmbUI feeders
  -  a transport class to send Interface object data to MQTT server
  -  default configuration is:
    - "value" packets are sent to ~/jpub/value topic
    - "interface", "xload", "section" packets are sent to ~/jpub/interface topic

    all other packets are sent to "jpub/etc" (could be removed in future)

+   unbind class Interface out of EmbUI class, implement FrameSend chain
  -  Interface is no longer uses EmbUI pointer, removed all deps on config-related vars
  -  EmbUI instance now has FrameSendChain feeders member, where various downstream transports could be regitered and used by Interface instances


+   replace weak functions with funtional callbacks
  -  implement user definable callbacks for the following weak functions
  -  section_main_frame()
  -  pubCallback()
  -  create_parameters()
  -  replaced with callbacks via ActionHandler class

+ FrameSend class refactoring
  - implement FrameSend chain list
  -  FrameSend abstract class has a member availabe() that allows to find if downstream is ready to send data
  -  FrameSendChain is a chained list of FrameSend objects that could send data over mutiple downstreams.
  -  Could be used as a common EmbUI data feeder with attachable plugins.
  - FrameSend::send() accepts generic type JsonVariantConst instead of only JsonObject

+ mDNS setup callback
  -  it is required to run mDNS service registration in proper order, so hooking to WiFi events does not work on system startup

**Major MQTT reimplementation**
mqttClient member is made public, User could use a flexibility to access it and set it's own hooks and features
     - make configuration have a flag to enable/disable mqtt clietn
     - rework a connector task that configures and connects the client
     - removed some ugly hangle() from a loop()
     - removed useless bool flags
     - removed tons of useless overloads
     - make default topic based on efuse MAC
     - user defined callbacks are completely removed
     - mqttClient is created on demand from now on, only of enabled and configured properly
     - EmbUI::publish() has templated overload to accept String contructable types for payload
     - _mqtt_pub_sys_status() pulishes life system params to mqtt in scope of send_pub() call
     - _onMqttConnect() will publish system data to ~/sys/ topic

    BasicUI - update MQTT setup form
    - add field to show current MQTT topicprefix
    - add comment for topic setup and $id macro


*   EmbUI::var* methods refactoring
     - methods use const char* as key params, removed String& overload
     - using string type traits for value evaluation and discarding null/zero valued keys

+ publish psram, rssi, uptime info in a more readable way
* basicui: fix save button on ftp page
* fix: when config variable was set false oom message was generated
* replace 'directly' with 'onChange', update ui templated methods

-  obsolete EmbUI::sysData struct
- disable uploadProgress() function
- remove mandatory creation of config varibales


### v2.8.1 - 2023.10.04
* fix issues with Interface::display() Interface::div
*  basicui: fix NTP servers button labels
* basicui: fix Simple Clock display value on const button
* protect section LOG message from crash if section has empty name
* fix Interface::constant, element was using wrong parameter as button text


### v2.8.0 - 2023.08.18
+ from now on building EmbUI requires c++17 for type traits
* reworked Interface class method templates
  - methods are no longer use implicit conversion to String&
  - added value/type checks for specific UI elements
  - use type traits for dirrefete string/literal type checks
  - removed some overloads for methods using values from system "config"
+ refactor UIelement class to templated class instantiating from various literal types
+ UIelement templates based on literal type traits
+ empty literals validation via type traits
* reworked button methods based on derived UIelement class
- removed 8266 legacy code - FPSTR macro's


### v2.7.2 - 2023.06.21
+ add inject option to post() method
+ bump ArduinoJson version to 6.21.x
+ add UI iframe method
+ embed FTP server support
- remove empty mqtt credentials from sysconfig
* correct issue with text/pwd field based on missing config key
* correct issues when overwriting null keys
* fix using JsonVariant for shallow copy
* fix builds with ESP32 Arduino core 2.0.8 for EmbUI and dependent libs

### v2.7.1 - 2023.01.26
+ progress bar css resources
+ use ArduinoJson shallow copy feature where applicapble
* fix in old "section lookup" algo (kept for compat)
* fix issue with arduinojson v6.20.0
* fix issue when trying to connect to AP's without password from webui
* small fixes and improvements
  - fix 'params' element
  + do not add '"html" = false' to UI elements
  + create some Interface obj on stack


### v2.7.0 - 2022.12.20
### core
+ WiFI Captive Portal feature, upon connecting to esp's AP a pop-up will arise advising to open EmbUI's setup page
- Obsolete Task Garbage collector, use task destructor instead
+ abstracted WiFi control into it's own class
  - WiFi conection manager turned into state machine
  - removed user callbacks for WiFi events. Duplicate fuctionality
  - hostname-related funcs now are members of EmbUI class
* detached TimeProcessor() from EmbUI class, let it handle WiFi event on it's own
+ pick ws action depending on post pkgs carry 'action' key (*API change*)
- removed support for esp8266, will backport most critical features to v2.6 branch
- removed legacy led/button/udp features, better to rewrite from scratch
+ compat with yubox mod of WebServerAsync and yubox AsyncTCP lib
* fix potential mem leaks with tasks and Interface class
* fix running with a version 3.7 of TaskScheduler lib
* code cleanup and optimization


### user UI/FrontEnd
+ WebUI post pkgs carry 'action' key (*API change*)
  -  EmbUI posted packages now includes id of the callee action is held in "action":"val" key/value,
  -  other form data follows within an "data": array as before
  -  simplify action lookups and helps to avoid key clashing on complex forms
* update [pure-css](https://github.com/pure-css/pure/releases/) to v2.1.0
+ add UI element "href button"
* fixed password/text area fields were casted to numeric upon sending via webui

### v2.6.3 - 2022.10.02
=======
* switch to own fork of [ESPAsyncServer](https://github.com/vortigont/ESPAsyncWebServer#hotfixes) with a bunch of  fixes required to properly support ESP-s2/ESP-c3 platforms (ESP-S3 not tested yet)
+ zlib-compressed OTA updates for EPS32-* via [esp32-flashz](https://github.com/vortigont/esp32-flashz)
* removed dependency on external LittleFS lib for esp32
* issues fixed in wifi, basic_ui, html and other...

### v2.6.2 - 2022.01.11
=======
* revert DHCP client hostname WA for esp32 Arduino core >2.x
* corrected NTPoDHCP feature for esp32 in a recent 2.x core
#### user UI
+ 3D buttons with a better looking 'press' feedback
* pure.css resources updated to 2.0.6
* small corrections in css

### v2.6.1 - 2021.11.27
* Use embeded LittleFS lib for ESP32 Arduino core >=2.0.0
* building for ESP32 Arduino core v2.0.0
* comment() html element could also be updated with json_section_content()

### v2.6.0 - 2021.10.28
* BasicUI methods moved into namespace
* fixed buffer size for TimeProcessor
* CI workflow added (builds examples)
* ui section log format corrections, some bugs cleaned
* remove bundled LinkedList library and declare as an external dependency
* adopted to LL iterators

### v2.5.0 - 2021.09.3
#### EmbUI:
  * saving/accessing config params as JsonVariant instead of cast to String's
  * do not save/overwrite config.json on param set if it's value has not changed
  + cfg param read/save/update tend to Null object design pattern
    + var_remove() method - allows removing existing keys from cfg
    + var_dropnulls() method - create/update cfg key only with non-null value, remove empty ones
    + removed almost all default values from EmbUI cfg
      + default autogenerated hostname is non-persistent and does not need to be saved in config file
      + default WiFi-related params is not saved in cfg file, no need to recreate it each time
      + default time/date options are no longer stored in cfg
  + post() data from WebUI is checked for 'submit' key first to match submited forms with proper action (simplify section lookup and avoid collisions)
  - disabled UDP support by default, could be reenabled with EMBUI_UDP build flag
  - disabled MQTT loop by default, could be reenabled with EMBUI_MQTT build flag

#### JS frontend:
  * Submited form/buttons data is casted it's native types when serializing post to backend
    - checkboxes casted to true/false bool
    - text/numeric/range fields casted to Numeric types if not NaN
    - empty strings casted to null (results in simplier backend operations on keys/string checking)
    - strings otherwise
  - treat received null values the same way as 'undef' (cast to empty string ""), fixes sporadic '[object Object]' strings in fields
  + templater action for 'button' - js, triggers user-defined js function without sending any data to backend
    could be used to interact with UI elements or external data without posting calls to backend
  - user-js function to apply/copy current datetime from the browser via button's press

#### HTML elemetns:
  * default values from config used as JsonVariant (keeping it's type on save/load) instead of String'ifying
  + Button has types now - 'simple' button, 'submit' and 'js'
    + button can pass specified value on press along with it's element id
    + button_submit uses a special 'submit' key to pass action id to backend along with id:value
    + button_js can trigger user-defined front-end js function on press

#### TimeProcessor:
  + method to customize user ntp server
  + method to get a list of active ntp servers/IP's
  + setter to enable/disable NTP over DHCP

#### Networking:
  + Hostname get/set via specific method, removed default network vars from cfg
  + autogenerated hostname is non-persistent run-time only var

#### BasicUI:
  + Hosname setup moved into it's own field with constant display
  + ability to reset hosname to autogenereated default
  + set/remove custom ntp server
  + active ntp servers could be seen on a setup page
  + current device date/time display (updates on page refresh)
  + buttons to apply/copy current time from the browser via front-end js-call
  + "System Options" section -  clear EmbUI config, reboot, sysupdate

### v2.4.3  milestone
#### WiFi: Mode switching code optimization
    Gracefull mode switching for WiFi, now modes are switched with a slight delay
    that allows WebUI to reflect all the changes happening and prevent dangling ws connections.
#### JS lib:
  - removed duplicate data repost on each ws reconnect
#### esp8266: Multiple fixes for Arduino Core 3.0.0
    + Enable WiFi persistency for Arduino Core
    * build fixes
    - obsolete MemInfo lib
#### TimeProcessor:
  * timeSync event now works for esp32 platform also
  * Obsolete WorldTimeApi methods, kept in a sepparate class for reference
#### Other
  - EmbUI version defines, make it easy to control deps
  - ChipID gen code cleanup
  - unify json_frame*() methods for Interface class

#### resources:
  - generic html template now contains separate version placeholders for EmbUI and for Firmware


### v2.4.2  milestone
js: ws.onvalue() accepts raw key:value assoc array
ui: - common method to draw 'displays' with sensor's data, etc...
      display's appearance could be customized with CSS
    - weak function user_settings() for basicui class,
        allows adding custom sections to the 'Settings' menu
* some fixes and optimization

### v2.4.0  Release
Added:
  - Optimizations:
    - frameSend Class now can serialize json directly to the ws buffer
    - ws post data processing
    - no data echo on single ws client
    - config file save/load optimization
  - method allows actions unregistering
  - ws packet 'xload' triggers ajax request for block: object
  - Ticker has been replaced with TaskScheduler lib
  - cpp templates for html elements
  - custom named section mapped to <div> template
  - websocket rawdata packet type
  - Time Zones list loading via ajax


Fixes:
  - ws post dangling pointer fix
  - mqtt
  - range int/float overloads
  - WiFi ap/sta switching
  - sending current date/time from browser


### v2.2.0  Release

##### WEB/HTML:
- dark/light themes

##### ESP32:
- OTA for ESP32 fixed for fw/fsimages. LittleFS requires 3-rd party tool to generate image
- Upload progress fix
- DHCP-client workaround for hostname missing in requests (espressif/arduino-esp32#2537)
 - fixed updating WiFi station variable (ESP32)
 
##### ESP32/esp8266:
- Delayed reconnection, allows websocket to finish section transaction. Fix #1
- TimeLib: Workaround for newlib bug with <+-nn> timezone names (esp8266/Arduino#7702)

##### EMBUI:
- Embed system-dependent  settings vars/GUI into framework code
  - predefined code for 'settings' web-page
  - generic example changed to adopt predefined settings UI

##### + other fixes:
- mqtt default pub time
- etc...
