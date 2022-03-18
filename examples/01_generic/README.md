# EmbUI Example

### Generic template

Шаблон проекта реализующий базовую функциональность фреймворка

  - WiFi-manager
    - работа в режиме WiFi-Client/WiFi-AP
    - автопереключение режимов при потере соединения с точкой доступа
    - настройки SSID, пароля доступа
  - Дата/время
    - автоматическая синхронизация по ntp
    - установка часового пояса/правил перехода сезонного времени
    - установка текущей даты/времени вручную  
  - Локализация интерфейса
    -  пример построения мультиязычного web-интерфейса
    -  переключение языка интерфейса в настройках WebUI на лету
  - Базовые настройки MQTT
  - OTA обновление прошивки и образа файловой системы

Установка:

 - в папку ./data развернуть файлы фреймворка из архива /resources/data.zip
 - собрать проект в platformio
 - залить FS в контроллер `pio run -t uploadfs`
 - залить прошивку в контроллер `pio run -t upload`

To upload LitlleFS image for ESP32 (until core v2 is out) it is required to use an uploader binary *mklittlefs*. Pls, download version for your OS from [here]
(https://github.com/earlephilhower/mklittlefs/releases) and put the binary to the root dir of the project.

