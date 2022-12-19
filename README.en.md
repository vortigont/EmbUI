__[RUSSIAN](/README.md) | [EXAMPLES](/examples) | [CHANGELOG](/CHANGELOG.md)__ | [![PlatformIO CI](https://github.com/vortigont/EmbUI/actions/workflows/pio_build.yml/badge.svg)](https://github.com/vortigont/EmbUI/actions/workflows/pio_build.yml)

# EmbUI
Embedded WebUI Interface framework


A framework that helps to create WebUI and dynamic control elements for Arduino projects. It offers _Interface-as-a-code_ approach to segregate web frontend and MCU backend firmware development.

## Supported platforms
 - ESP32, ESP32-S2, ESP32-C3, ESP32-S3 Arduino Core
 - ESP8266 Arduino Core [branch v2.6](https://github.com/vortigont/EmbUI/tree/v2.6) only

## Features
 - asynchronous data exchange with frontend via WebSocket
 - creating html elements using DOM and [{{ mustache }}](https://mustache.github.io/) templater
 - packet based data exchange with WebUI, allows transfering large objects containing multiple elements split into chunks and using less MCU memory
 - dynamic UI elements update without page refresh
 - allows fetching data/ui objects via 3rd party AJAX calls
 - supports multiple parallel connections, WebUI updates simultaneously on all devices
 - embedded WiFi manager, AP-failover on connection lost, autoreconnection
 - full support for TimeZones, daylight saving autoswitchover
 - embedded NTP sync, NTPoDHCP support, custom NTP servers
 - OTA firmware updates via WebUI/CLI tools
 - zlib-compressed FOTA using [esp32-flashz](https://github.com/vortigont/esp32-flashz) lib
 - [mDNS](https://en.wikipedia.org/wiki/Multicast_DNS)/[ZeroConf](https://en.wikipedia.org/wiki/Zero-configuration_networking) publisher, discovery
 - device autodiscovery via:
    - WiFi Captive Portal detection - upon connecting to esp's WiFi AP a device/browser will show a pop-up advising to open an EmbUI's setup page
    - [Service Browser](https://play.google.com/store/apps/details?id=com.druk.servicebrowser) Android
    - [SSDP](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol) for Windows
    - [Bonjour](https://en.wikipedia.org/wiki/Bonjour_(software)) iOS/MacOS
 - self-hosted - all resources resides on LittleFS locally, no calls for CDN resources

## Projects using EmbUI
 - [ESPEM](https://github.com/vortigont/espem) - ESP32 energy meter for Peacfair PZEM-004
 - [InfoClock](https://github.com/vortigont/infoclock) - Clock/weather ticker based on Max72xx modules
 - [FireLamp_JeeUI](https://github.com/DmytroKorniienko/FireLamp_JeeUI/tree/dev) - DIY firelamp based on ws2812 led matrix _(API is NOT compatible with this fork of EmbUI)_
 - [EmbUI](https://github.com/DmytroKorniienko/) - upstream project for this fork (API is not compatible anymore)



## WebUI samples based on EmbUI framework

<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/espemembui.png" alt="embui UI" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/espemembui_setup.png" alt="embui options" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/ui_datetime.png" alt="embui dtime" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/infoclock/master/doc/infoclock_embui02.png" alt="InfoClock" width="30%"/>


## Usage
To run EmbUI it is required to upload an FS image with resources to LittleFS partition.
Prebuild resources are available [as archive](https://github.com/vortigont/EmbUI/raw/main/resources/data.zip).
Unpack it into *data* folder to the root of [Platformio](https://platformio.org/) project

## Documentation/API description
to be continued...
