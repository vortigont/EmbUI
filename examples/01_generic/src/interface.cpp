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
    embui.var_create(FPSTR(V_LED), "1");    // LED default status is on
    embui.var_create(FPSTR(V_VAR1), "");    // заводим пустую переменную по умолчанию

    /**
     * добавляем свои обрабочки на вывод UI-секций
     * и действий над данными
     */
    embui.section_handle_add(FPSTR(T_DEMO), block_demopage);                // generate "Demo" UI section

    // обработчики
    embui.section_handle_add(FPSTR(T_SET_DEMO), action_demopage);           // обработка данных из секции "Demo"

    embui.section_handle_add(FPSTR(V_LED), action_blink);               // обработка рычажка светодиода

};

/**
 * Headline section
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
  interf->value(F("fwver"), F("demo_1.1"), true);   // just an example
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
    interf->json_section_main(FPSTR(T_SET_DEMO), F("Some demo controls"));
    interf->comment(F("Комментарий: набор контролов для демонстрации"));     // комментарий-описание секции

    // переключатель, связанный со светодиодом. Изменяется синхронно 
    interf->checkbox(FPSTR(V_LED), F("Onboard LED"), true);

    interf->text(FPSTR(V_VAR1), F("текстовое поле"));                                 // текстовое поле со значением переменной из конфигурации
    interf->text(FPSTR(V_VAR2), F("some default val"), F("Второе текстовое поле"), false);   // текстовое поле со значением "по-умолчанию"

    /*  кнопка отправки данных секции на обработку
     *  первый параметр FPSTR(T_DEMO) определяет алиас акшена обработчика данных формы 
     *  обработчк должен быть зарегистрирован через embui.section_handle_add()
     */ 
    interf->button_submit(FPSTR(T_SET_DEMO), FPSTR(T_DICT[lang][TD::D_Send]), FPSTR(P_GRAY));
    interf->json_frame_flush();
}

/**
 * @brief action handler for demo form data
 * 
 */
void action_demopage(Interface *interf, JsonObject *data){
    if (!data) return;

    LOG(println, F("processing section demo"));

    // сохраняем значение 1-й переменной в конфиг фреймворка
    SETPARAM(FPSTR(V_VAR1));

    // выводим значение 1-й переменной в serial
    const char *text = (*data)[FPSTR(V_VAR1)];
    Serial.printf_P(PSTR("Varialble_1 value:%s\n"), text );

    // берем указатель на 2-ю переменную
    text = (*data)[FPSTR(V_VAR2)];
    // или так:
    // String var2 = (*data)[FPSTR(V_VAR2)];
    // выводим значение 2-й переменной в serial
    Serial.printf_P(PSTR("Varialble_2 value:%s\n"), text);

}

/**
 * @brief interactive handler for LED switchbox
 * 
 */
void action_blink(Interface *interf, JsonObject *data){
  if (!data) return;  // здесь обрабатывает только данные

  SETPARAM(FPSTR(V_LED));  // save new LED state to the config

  // set LED state to the new checkbox state
  digitalWrite(LED_BUILTIN, !(*data)[FPSTR(V_LED)]); // write inversed signal for build-in LED
  Serial.printf("LED: %u\n", (*data)[FPSTR(V_LED)].as<bool>());
}

/**
 * обработчик статуса (периодического опроса контроллера веб-приложением)
 */
void pubCallback(Interface *interf){
    basicui::embuistatus(interf);
}