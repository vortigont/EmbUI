
// Main headers
#include "main.h"
#include "EmbUI.h"
#include "uistrings.h"   // non-localized text-strings

/**
 * построение интерфейса осуществляется в файлах 'interface.*'
 *
 */

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
  embui.setPubInterval(0);

  // restore LED state from configuration
  digitalWrite( LED_BUILTIN, !embui.param(FPSTR(V_LED)) );
}


// MAIN loop
void loop() {
  embui.handle();
}