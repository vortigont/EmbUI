__[ENGLISH](/README.en.md) | [EXAMPLES](/examples) | [CHANGELOG](/CHANGELOG.md)__ | [![PlatformIO CI](https://github.com/vortigont/EmbUI/actions/workflows/pio_build.yml/badge.svg)](https://github.com/vortigont/EmbUI/actions/workflows/pio_build.yml)

# EmbUI
Embedded WebUI Interface framework


Фреймворк для построения web-интерфейса и динамических элементов управления для проектов Arduino

## Поддерживаемые платформы
 - ESP32, ESP32-S2, ESP32-C3, ESP32-S3 Arduino Core
 - ESP8266 Arduino Core [branch v2.6](https://github.com/vortigont/EmbUI/tree/v2.6) only

## Возможности
 - асинхронный интерфейс обмена данными с браузером через WebSocket
 - построение элементов DOM на основе шаблонизатора [{{ mustache }}](https://mustache.github.io/)
 - пакетный интерфейс обмена с WebUI, возможность передавать большие объекты из множества элементов по частям
 - динамическое обновление отдельных элементов интерфейса безе перерисовки всего документа
 - возможность подгружать данные/элементы интерфейса через AJAX
 - поддержка нескольких параллельных WebUI подключений с обратной связью, интерфейс обновляется одновременно на всех устройствах
 - встроенный WiFi менеджер, автопереключение в режим AP при потере клиентского соединения
 - полная поддержка всех существующих часовых зон, автоматический переход на летнее/зимнее время, корректная калькуляция дат/временных интервалов
 - встроенная синхронизация часов через интернет, поддержка NTPoDHCP, пользовательский NTP
 - OTA, обновление прошивки/образа ФС через браузер/cli
 - поддрежка обновления из сжатых образов (zlib-compressed FOTA) через библиотеку [esp32-flashz](https://github.com/vortigont/esp32-flashz)
 - автопубликация контроллера в локальной сети через [mDNS](https://en.wikipedia.org/wiki/Multicast_DNS)/[ZeroConf](https://en.wikipedia.org/wiki/Zero-configuration_networking)
 - возможность обнаружения устройства:
    - WiFi Captive Portal detection - при подключении к WiFi AP контроллера устройтсво/браузер покажет всплюывающее окно с предложением открыть страницу настройки EmbUI
    - [Service Browser](https://play.google.com/store/apps/details?id=com.druk.servicebrowser) Android
    - [SSDP](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol) for Windows
    - [Bonjour](https://en.wikipedia.org/wiki/Bonjour_(software)) iOS/MacOS
 - self-hosted - нет зависимостей от внешних ресурсов/CDN/Cloud сервисов

## Проекты на EmbUI
 - [ESPEM](https://github.com/vortigont/espem) - энергометр на основе измерителя PZEM-004
 - [InfoClock](https://github.com/vortigont/infoclock) - Часы-информер на матричных модулях Max72xx
 - [FireLamp_JeeUI](https://github.com/DmytroKorniienko/FireLamp_JeeUI/tree/dev) - огненная лампа на светодиодной матрице ws2812 _(API не совместим с данным форком)_
 - [EmbUI](https://github.com/DmytroKorniienko/) - исходный проект данного форка



## Примеры построения интерфейсов

<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/espemembui.png" alt="embui UI" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/espemembui_setup.png" alt="embui options" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/espem/master/examples/ui_datetime.png" alt="embui dtime" width="30%"/>
<img src="https://raw.githubusercontent.com/vortigont/infoclock/master/doc/infoclock_embui02.png" alt="InfoClock" width="30%"/>


## Использование
Для работы WebUI необходимо залить в контроллер образ фаловой системы LittleFS с web-ресурсами.
Подготовленные ресурсы для создания образа можно развернуть [из архива](https://github.com/vortigont/EmbUI/raw/main/resources/data.zip).
В [Platformio](https://platformio.org/) это, обычно, каталог *data* в корне проекта.

