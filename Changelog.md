## Changelog

### v2.4.3  milestone
WiFi: Mode switching code optimization
      Gracefull mode switching for WiFi, now modes are switched with a slight delay
      that allows WebUI to reflect all the changes happening and prevent dangling ws connections.
JS lib: removed duplicate data repost on each ws reconnect
esp8266: Multiple fixes for Arduino Core 3.0.0
      + Enable WiFi persistency for Arduino Core
      * build fixes
      - obsolete MemInfo lib
TimeProcessor:
      * timeSync event now works for esp32 platform also
      - Obsolete WorldTimeApi methods, kept in a sepparate class for reference

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
