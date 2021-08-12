
// Main headers
#include "main.h"
#include "EmbUI.h"
#include "uistrings.h"   // non-localized text-strings

/**
 * построение интерфейса осуществляется в файлах 'interface.*'
 *
 */

#ifdef USE_FTP
 #include "ftpSrv.h"
#endif

#ifdef ESP8266
  ADC_MODE(ADC_VCC);  // read internal Vcc
#endif

// MAIN Setup
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println("Starting test...");

  pinMode(LED_BUILTIN, OUTPUT); // we are goning to blink this LED

  // Start EmbUI framework
  embui.begin();

  LOG(println, "restore LED state from configuration param");
  digitalWrite( LED_BUILTIN, !embui.paramVariant(FPSTR(V_LED)) );

  #ifdef USE_FTP
      ftp_setup(); // запуск ftp-сервера
  #endif
}


// MAIN loop
void loop() {
  embui.handle();

#ifdef USE_FTP
    ftp_loop(); // цикл обработки событий фтп-сервера
#endif
}