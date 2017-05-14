#ifndef ESP8266_WEATHER_CONFIG_H
#define ESP8266_WEATHER_CONFIG_H

// Wifi name and password
const char* SSID     = "<SSID>";
const char* PASSWORD = "<PASSWORD>";

// Get from maps.google.com, right click "what's here" (e.g. 40.74857,-73.9879617)
const char* LOCATION = "<LON>,<LAT>";
// Get from https://darksky.net/dev/
const char* KEY      = "<API KEY>";
// How often to check the weather (one hour is reasonable)
const long CACHE_TTL = 1000 * 60 * 60;

// Where did you connect your motion detector?
const int PIN_PIR   = <PIN>;
// Where did you connect your NeoPixels?
const int PIN_PIXEL = <PIN>;
// How many NeoPixels do you have?
const int LED_COUNT = <COUNT>;

#endif
