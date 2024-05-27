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
 * Headlile section
 * this is an overriden weak method that builds our WebUI interface from the top
 * ==
 * Головная секция,
 * переопределенный метод фреймфорка, который начинает строить корень нашего Web-интерфейса
 * 
 */
void section_main_frame(Interface *interf, const JsonObject *data, const char* action){

  interf->json_frame_interface();

  interf->json_section_manifest("EmbUI Example", embui.macid(), 0, "v1.0");      // app name/jsapi/version manifest
  interf->json_section_end();                                     // manifest section MUST be closed!

  block_menu(interf, data, NULL);                         // Строим UI блок с меню выбора других секций
  interf->json_frame_flush();                       // flush frame, we will create a new one later

  if(WiFi.getMode() & WIFI_MODE_STA){            // if WiFI is no connected to external AP, than show page with WiFi setup
    block_demopage(interf, data, NULL);                   // Строим блок с demo переключателями
  } else {
    LOG(println, "UI: Opening network setup page");
    basicui::page_settings_netw(interf, data, NULL);
  }

};


/**
 * This code builds UI section with menu block on the left
 * 
 */
void block_menu(Interface *interf, const JsonObject *data, const char* action){
    if (!interf) return;
    // создаем меню
    interf->json_section_menu();

    /**
     * пункт меню - "демо"
     */
    interf->option(T_DEMO, "UI Demo");

    /**
     * добавляем в меню пункт - настройки,
     * это автоматически даст доступ ко всем связанным секциям с интерфейсом для системных настроек
     * 
     */
    basicui::menuitem_settings(interf);       // пункт меню "настройки"
    interf->json_section_end();
}

/**
 * Demo controls
 * 
 */
void block_demopage(Interface *interf, const JsonObject *data, const char* action){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    // параметр T_SET_DEMO определяет зарегистрированный обработчик данных для секции
    interf->json_section_main(T_SET_DEMO, "Some demo sensors");

    // переключатель, связанный со светодиодом. Изменяется синхронно
    interf->checkbox(V_LED, embui.paramVariant(V_LED), "Onboard LED", true);

    interf->comment("A comment: simple live-displays");     // комментарий-описание секции

    interf->json_section_line();            // Open line section - next elements will be placed in a line

    // Now I create a string of text that will follow LED's state
    String cmt = "Onboard LED is ";

    if ( embui.paramVariant(V_LED) ) // get LED's state from a configuration variable
      cmt += "ON!";
    else
      cmt += "OFF!";

    interf->comment("ledcmt", cmt);        // Create a comment string  - LED state text, "ledcmt" is an element ID I will use to change text later

    // Voltage display, shows ESPs internal voltage
#ifdef ESP8266
    interf->display("vcc", ESP.getVcc()/1000.0);
#endif

#ifdef ESP32
    interf->display("vcc", 3.3); // set static voltage
#endif

    /*
      Some display based on user defined CSS class - "mycssclass". CSS must be loaded in WebUI
      Resulting CSS classes are defined as: class='mycssclass vcc'
      So both could be used to customize display appearance 
      interf->display("vcc", 42, "mycssclass");
    */

    // Fake temperature sensor
    interf->display("temp", 24);
    interf->json_section_end();     // end of line

    // Simple Clock display
    String clk; TimeProcessor::getInstance().getDateTimeString(clk);
    interf->display("clk", clk);    // Clock DISPLAY


    // Update rate slider
    interf->range(V_UPDRATE, (int)tDisplayUpdater.getInterval()/1000, 0, 30, 1, "Update Rate, sec", true);
    interf->json_frame_flush();
}

/**
 * @brief interactive handler for LED switchbox
 * every change of a checkbox triggers this action
 */
void action_blink(Interface *interf, const JsonObject *data, const char* action){
  if (!data) return;  // process only data

  SETPARAM(V_LED);  // save new LED state to the config

  // set LED state to the new checkbox state
  digitalWrite(LED_BUILTIN, !(*data)[V_LED]); // write inversed signal to the build-in LED's GPIO
  Serial.printf("LED: %u\n", (*data)[V_LED].as<bool>());

  // if we have an interface ws object, 
  // than we can publish some changes to the web pages change comment text to reflect LED state
  if (!interf)
    return;

  interf->json_frame_interface();   // make a new interface frame
  interf->json_section_content();   // we only update existing elemtns, not pulishing new ones

  String cmt = "Onboard LED is ";

  if ( (*data)[V_LED] )      // find new LED's state from an incoming data
    cmt += "ON!";
  else
    cmt += "Off!";

  interf->comment("ledcmt", cmt);        // update comment object with ID "ledcmt"
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

    Interface interf(&embui.ws);
    interf.json_frame_value();
    // Update voltage sensor
#ifdef ESP8266
    float v = ESP.getVcc();
#else
    float v = 3.3;
#endif

    //  id, value, html=true
    // html must be set 'true' so this value could be handeled properly for div elements
    // add some random voltage spikes to make display change it's value
    interf.value("vcc", (100*v + random(-15,15))/100.0, true);

    // add some random spikes to the temperature :)
    interf.value("temp", 24 + random(-30,30)/10.0, true);

    String clk; TimeProcessor::getInstance().getDateTimeString(clk);
    interf.value("clk", clk, true); // Current date/time for Clock display

    interf.json_frame_flush();
}

/**
 * Change sensor update rate callback
 */
void setRate(Interface *interf, const JsonObject *data, const char* action) {
  if (!data) return;

  if (!(*data)[V_UPDRATE]){    // disable update on interval '0'
      tDisplayUpdater.disable();
  } else {
    tDisplayUpdater.setInterval( (*data)[V_UPDRATE].as<unsigned int>() * TASK_SECOND ); // set update rate in seconds
    tDisplayUpdater.enableIfNot();
  }

}

/**
 * функция регистрации переменных и активностей
 * 
 */
void create_parameters(){
    LOG(println, "UI: Creating application vars");

    /**
     * регистрируем свои переменные
     */
    embui.var_create(V_LED, true);    // LED default status is on

    // регистрируем обработчики активностей
    embui.action.set_mainpage_cb(section_main_frame);                            // заглавная страница веб-интерфейса

    /**
     * добавляем свои обрабочки на вывод UI-секций
     * и действий над данными
     */
    embui.action.add(T_DEMO, block_demopage);                // generate "Demo" UI section

    // обработчики
    embui.action.add(V_LED, action_blink);               // обработка рычажка светодиода
    embui.action.add(V_UPDRATE, setRate);                // sensor data publisher rate change
};

