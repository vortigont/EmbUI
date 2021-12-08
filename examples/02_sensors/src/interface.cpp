#include "main.h"

#include "EmbUI.h"
#include "interface.h"

#include "uistrings.h"   // non-localized text-strings

/**
 * можно нарисовать свой собственный интефейс/обработчики с нуля, либо
 * подключить статический класс с готовыми формами для базовых системных натсроек и дополнить интерфейс.
 * необходимо помнить что существуют системные переменные конфигурации с зарезервированными именами.
 * Список имен системных переменных можно найти в файле "constants.h"
 */
#include "basicui.h"

// Include Tasker class from Framework to work with periodic tasks
#include "ts.h"

// TaskScheduler - Let the runner object be a global, single instance shared between object files.
extern Scheduler ts;

// Periodic task that runs every 5 sec and calls sensor publishing method
Task tDisplayUpdater(SENSOR_UPDATE_RATE * TASK_SECOND, TASK_FOREVER, &sensorPublisher, &ts, true );


/**
 * переопределяем метод из фреймворка, регистрирующий необходимы нам в проекте переменные и методы обработки
 * 
 */
void create_parameters(){
    LOG(println, F("UI: Creating application vars"));

   /**
    * регистрируем статические секции для web-интерфейса с системными настройками,
    * сюда входит:
    *  - WiFi-manager
    *  - установка часового пояса/правил перехода сезонного времени
    *  - установка текущей даты/времени вручную
    *  - базовые настройки MQTT
    *  - OTA обновление прошивки и образа файловой системы
    */
    basicui::add_sections();

    /**
     * регистрируем свои переменные
     */
    embui.var_create(FPSTR(V_LED), true);    // LED default status is on

    /**
     * добавляем свои обрабочки на вывод UI-секций
     * и действий над данными
     */
    embui.section_handle_add(FPSTR(T_DEMO), block_demopage);                // generate "Demo" UI section

    // обработчики
    embui.section_handle_add(FPSTR(V_LED), action_blink);               // обработка рычажка светодиода
    embui.section_handle_add(FPSTR(V_UPDRATE), setRate);                // sensor data publisher rate change
};

/**
 * Headlile section
 * this is an overriden weak method that builds our WebUI interface from the top
 * ==
 * Головная секция,
 * переопределенный метод фреймфорка, который начинает строить корень нашего Web-интерфейса
 * 
 */
void section_main_frame(Interface *interf, JsonObject *data){
  if (!interf) return;

  interf->json_frame_interface(FPSTR(T_HEADLINE));  // HEADLINE for EmbUI project page

  block_menu(interf, data);                         // Строим UI блок с меню выбора других секций
  interf->json_frame_flush();

  if(!embui.sysData.wifi_sta){                      // если контроллер не подключен к внешней AP, сразу открываем вкладку с настройками WiFi
    LOG(println, F("UI: Opening network setup section"));
    basicui::block_settings_netw(interf, data);
  } else {
    block_demopage(interf, data);                   // Строим блок с demo переключателями
  }

  interf->json_frame_flush();                       // Close interface section

  // Publish firmware version (visible under menu section)
  interf->json_frame_value();
  interf->value(F("fwver"), F("demo_1.1"), true);
  interf->json_frame_flush();
};


/**
 * This code builds UI section with menu block on the left
 * 
 */
void block_menu(Interface *interf, JsonObject *data){
    if (!interf) return;
    // создаем меню
    interf->json_section_menu();

    /**
     * пункт меню - "демо"
     */
    interf->option(FPSTR(T_DEMO), F("UI Demo"));

    /**
     * добавляем в меню пункт - настройки,
     * это автоматически даст доступ ко всем связанным секциям с интерфейсом для системных настроек
     * 
     */
    basicui::opt_setup(interf, data);       // пункт меню "настройки"
    interf->json_section_end();
}

/**
 * Demo controls
 * 
 */
void block_demopage(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    // параметр FPSTR(T_SET_DEMO) определяет зарегистрированный обработчик данных для секции
    interf->json_section_main(FPSTR(T_SET_DEMO), F("Some demo sensors"));

    // переключатель, связанный со светодиодом. Изменяется синхронно
    interf->checkbox(FPSTR(V_LED), F("Onboard LED"), true);

    interf->comment(F("Комментарий: набор демонстрационных сенсоров"));     // комментарий-описание секции

    interf->json_section_line();            // Open line section - next elements will be placed in a line

    // Now I create a string of text that will follow LED's state
    String cmt = F("Onboard LED is ");

    if ( embui.paramVariant(FPSTR(V_LED)) ) // get LED's state from a configuration variable
      cmt += "ON!";
    else
      cmt += "OFF!";

    interf->comment(F("ledcmt"), cmt);        // Create a comment string  - LED state text, "ledcmt" is an element ID I will use to change text later

    // Voltage display, shows ESPs internal voltage
#ifdef ESP8266
    interf->display(F("vcc"), ESP.getVcc()/1000.0);
#endif

#ifdef ESP32
    interf->display(F("vcc"), 220); // supercharged esp :)
#endif

    /*
      Some display based on user defined CSS class - "mycssclass". CSS must be loaded in WebUI
      Resulting CSS classes are defined as: class='mycssclass vcc'
      So both could be used to customize display appearance 
      interf->display(F("vcc"), 42, "mycssclass");
    */

    // Fake temperature sensor
    interf->display(F("temp"), 24);
    interf->json_section_end();     // end of line

    // Simple Clock display
    String clk; TimeProcessor::getInstance().getDateTimeString(clk);
    interf->display(F("clk"), clk);    // Clock DISPLAY


    // Update rate slider
    interf->range(FPSTR(V_UPDRATE), tDisplayUpdater.getInterval()/1000, 0, 30, 1, F("Update Rate, sec"), true);
    interf->json_frame_flush();
}

/**
 * @brief interactive handler for LED switchbox
 * every change of a checkbox triggers this action
 */
void action_blink(Interface *interf, JsonObject *data){
  if (!data) return;  // process only data

  SETPARAM(FPSTR(V_LED));  // save new LED state to the config

  // set LED state to the new checkbox state
  digitalWrite(LED_BUILTIN, !(*data)[FPSTR(V_LED)]); // write inversed signal to the build-in LED's GPIO
  Serial.printf("LED: %u\n", (*data)[FPSTR(V_LED)].as<bool>());

  // if we have an interface ws object, 
  // than we can publish some changes to the web pages change comment text to reflect LED state
  if (!interf)
    return;

  interf->json_frame_interface();   // make a new interface frame
  interf->json_section_content();   // we only update existing elemtns, not pulishing new ones

  String cmt = F("Onboard LED is ");

  if ( (*data)[FPSTR(V_LED)] )      // find new LED's state from an incoming data
    cmt += "ON!";
  else
    cmt += "Off!";

  interf->comment(F("ledcmt"), cmt);        // update comment object with ID "ledcmt"
  interf->json_frame_flush();
}

/**
 * обработчик статуса (периодического опроса контроллера веб-приложением)
 */
void pubCallback(Interface *interf){
    basicui::embuistatus(interf);
}

/**
 * Call-back for Periodic publisher (truggered via Task Scheduler)
 * it reads (virtual) sensors and publishes values to the WebUI
 */
void sensorPublisher() {
    if (!embui.ws.count())    // send new values only if there are WebSocket clients
      return;

    Interface *interf = new Interface(&embui, &embui.ws, SMALL_JSON_SIZE);
    interf->json_frame_value();
    // Voltage sensor
    //  id, value, html=true
#ifdef ESP8266
    interf->value(F("vcc"), (ESP.getVcc() + random(-100,100))/1000.0, true); // html must be set 'true' so this value could be handeled properly for div elements
#endif
    interf->value(F("temp"), 24 + random(-30,30)/10, true);                // add some random spikes to the temperature :)

    String clk; TimeProcessor::getInstance().getDateTimeString(clk);
    interf->value(F("clk"), clk, true); // Current date/time for Clock display

    interf->json_frame_flush();
    delete interf;
}

/**
 * Change sensor update rate callback
 */
void setRate(Interface *interf, JsonObject *data) {
  if (!data) return;

  if (!(*data)[FPSTR(V_UPDRATE)]){    // disable update on interval '0'
      tDisplayUpdater.disable();
  } else {
    tDisplayUpdater.setInterval( (*data)[FPSTR(V_UPDRATE)].as<unsigned int>() * TASK_SECOND ); // set update rate in seconds
    tDisplayUpdater.enableIfNot();
  }

}