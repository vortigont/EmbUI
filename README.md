# EmbUI
Embedded WebUI Interface framework

__[EXAMPLES](/examples) | [CHANGELOG](/CHANGELOG.md)__ | [![PlatformIO CI](https://github.com/vortigont/EmbUI/actions/workflows/pio_build.yml/badge.svg)](https://github.com/vortigont/EmbUI/actions/workflows/pio_build.yml)


Фреймворк построения web-интерфейса и элементов управления для проектов под Arduino 
## Поддерживаемые платформы
 - ESP8266 Arduino Core
 - ESP32, ESP32-S2, ESP32-C3 Arduino Core (ESP32-S3 not tested)

## Возможности
 - автопубликация контроллера в локальной сети через [mDNS](https://en.wikipedia.org/wiki/Multicast_DNS)/[ZeroConf](https://en.wikipedia.org/wiki/Zero-configuration_networking)
 - возможность обнаружения устройства:
    - [Service Browser](https://play.google.com/store/apps/details?id=com.druk.servicebrowser) Android
    - [SSDP](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol) for Windows
    - [Bonjour](https://en.wikipedia.org/wiki/Bonjour_(software)) iOS/MacOS
 - обмен данными с браузером через WebSocket
 - поддержка нескольких параллельных подключений, интерфейс обновляется одновременно на всех устройствах
 - self-hosted - нет зависимостей от внешних ресурсов/CDN/Cloud сервисов
 - встроенный WiFi менеджер, автопереключение в режим AP при потере клиентского соединения
 - полная поддержка всех существующих Временных Зон, автоматический переход на летнее/зимнее время, корректная калькуляция дат/временных интервалов
 - OTA, обновление прошивки/образа ФС через браузер. zlib-compressed FOTA see [example](/examples/02_sensors)
 - возможность подгружать данные/элементы интерфейса через AJAX

## Проекты на EmbUI
 - [ESPEM](https://github.com/vortigont/espem) - энергометр на основе измерителя PZEM-004
 - [InfoClock](https://github.com/vortigont/infoclock) - Часы-информер на матричных модулях Max72xx
 - [FireLamp_JeeUI](https://github.com/DmytroKorniienko/FireLamp_JeeUI/tree/dev) - огненная лампа на светодиодной матрице ws2812 _(исходный проект, не совместим с данным форком)_



## Примеры построения интерфейсов

<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/espemembui.png" alt="embui UI" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/espemembui_setup.png" alt="embui options" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/ui_datetime.png" alt="embui dtime" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/infoclock/master/doc/infoclock_embui02.png" alt="InfoClock" width="30%"/>


## Использование
Для работы WebUI необходимо залить в контроллер образ фаловой системы LittleFS с web-ресурсами.
Подготовленные ресурсы для создания образа можно развернуть [из архива](https://github.com/vortigont/EmbUI/raw/main/resources/data.zip).
В [Platformio](https://platformio.org/) это, обычно, каталог *data* в корне проекта.

