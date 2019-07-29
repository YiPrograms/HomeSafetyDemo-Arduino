#include <Arduino.h>
#include <ESP8266WiFi.h>

extern "C" {
#include "gpio.h"
}
extern "C" {
#include "user_interface.h"
}

void wakeup(void) {
  gpio_pin_wakeup_disable();
  wifi_fpm_close();
  Serial.println("yawn");
}

// This doesn't work. i tried!
void sleepNow(uint8_t wakepin) {
  Serial.println("disconnecting wifi");
  wifi_station_disconnect();
  Serial.println("wifi null mode");
  wifi_set_opmode(NULL_MODE);
  Serial.println("enabling sleep");
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); //light sleep mode
  wifi_fpm_set_wakeup_cb(wakeup);
  gpio_pin_wakeup_enable(GPIO_ID_PIN(wakepin), GPIO_PIN_INTR_LOLEVEL); //set the interrupt to look for LOW pulses on Pin 4 
  wifi_fpm_open();
  wifi_fpm_do_sleep(0xFFFFFFF); 
  delay(100);
  
  gpio_pin_wakeup_disable();
  Serial.println("woke up from light sleep...");
  wifi_fpm_close();
  Serial.println("WiFi turn on!");
  wifi_set_opmode(STATION_MODE);
  Serial.println("Reconnect");
  wifi_station_connect();
  Serial.println("Connected");
  delay(3000);
}



