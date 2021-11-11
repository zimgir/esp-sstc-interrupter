#include <Arduino.h>

#include "utils.h"
#include "config.h"

#include "SavedConfig.h"
#include "PWMController.h"
#include "AppServer.h"

SavedConfig config;
PWMController control(config);
AppServer server(config, control);

void setup()
{
	pinMode(PIN_INPUT, INPUT);
  pinMode(PIN_OUTPUT, OUTPUT);
	digitalWrite(PIN_OUTPUT, LOW);

  Serial.begin(SERIAL_FREQ);

  LOGI("*** BOOT ***");

  config.init();
  control.init();
  server.init();

}

void loop()
{
  control.loop();
  server.loop();
  MDNS.update();
}