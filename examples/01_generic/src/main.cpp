
// Main headers
#include "main.h"
#include "EmbUI.h"
#include "uistrings.h"   // non-localized text-strings
#include "interface.h"

/**
 * построение интерфейса осуществляется в файлах 'interface.*'
 *
 */

// MAIN Setup
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.println("Starting test...");

  pinMode(LED_BUILTIN, OUTPUT); // we are goning to blink this LED

  // Start EmbUI framework
  embui.begin();

  // register our actions
  create_parameters();

  LOG(println, "restore LED state from configuration param");
  digitalWrite( LED_BUILTIN, !embui.getConfig()[V_LED] );
}


// MAIN loop
void loop() {
  embui.handle();
}