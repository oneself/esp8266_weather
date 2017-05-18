#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "esp8266_weather_config.h"

// Constatnts to access the API
const char*  HOST = "api.shoulditakeanumbrella.com";
const String URL  = String("/api/hourly/") + LOCATION + "?key=" + KEY;
const int    PORT = 80;

// Percent battery to show a warning for.
const int   LOW_BATT = 50;

// How long to keep the LEDs on after movement is no longer detencted.
const int   FLASH_DELAY = 5000;
// Number of hours to show.
const int   STATE_COUNT = 12;
// Initialization array
const int   UMBRELLA_STATE_UNKNOWN[STATE_COUNT] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
// Variable to keep current state.
int umbrellaState[STATE_COUNT];
// Variable to keep track of when we turned on the LEDs.  So, we can turn them off.
long pixelFlashTime = 0;
// Keep cache update time
long cacheRefreshTime = 0;

// LED strip
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(LED_COUNT, PIN_PIXEL, NEO_GRB + NEO_KHZ800);
// Some colors
const uint32_t OFF    = pixels.Color(0,   0,   0);
const uint32_t RED    = pixels.Color(16,  0,   0);
const uint32_t GREEN  = pixels.Color(0,   64,  0);
const uint32_t YELLOW = pixels.Color(178, 77,  0);
const uint32_t ORANGE = pixels.Color(255, 77,  0);
const uint32_t BLUE   = pixels.Color(0,   0,   255);
const uint32_t WHITE  = pixels.Color(128, 128, 128);
const uint32_t GREY   = pixels.Color(16,  16,  16);

/**
 * Sets all the LEDs to the same color.
 */
void setPixels(uint32_t color) {
  int n = pixels.numPixels();
  for(int i = 0; i < n; ++i) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
}

/**
 * Sets a specific segment of the LED strip to a specified color.  Take the total size of the strip
 * into account.
 */
void setPixel(int i, uint32_t color) {
  int n = pixels.numPixels();
  if (n < STATE_COUNT + 2) {
    // Need at least STATE_COUNT + 2 LEDs for the edges.
    pixels.setPixelColor(i, RED);
    return;
  }
  // if we have more LEDs then states (or the same), use more LEDs for each state.
  // m is the multiplier (how many LEDs per state)
  int m = n / STATE_COUNT;
  // s is the shift count, if we have an uneven destribution, shift LEDs so they light up in the middle.
  int s = (n - (m * STATE_COUNT)) / 2;
  for (int l = 0; l < m; ++l) {
    pixels.setPixelColor(s + (i * m + l) , color);
  }

  // Mark edges.  If battery is low, mark in red.
  uint32_t c = batteryLevel() > LOW_BATT ? GREY : RED;
  pixels.setPixelColor(s - 1, c);
  pixels.setPixelColor(n - s, c);
}

/**
 * Query weather API and populate the hourly forecasts for the chance it might rain.
 */
void fetchUmbrellaState() {
  if (cacheRefreshTime > 0 && millis() - cacheRefreshTime < CACHE_TTL) {
    // It's been less than the cache refresh time, don't go online.
    Serial.println("Cache still fresh.");
    return;
  }
  // Get a web client.
  WiFiClient client;
  // Connect to weather API service
  if (!client.connect(HOST, PORT)) {
    Serial.println("connection failed");
    return;
  }

  // Get data
  client.print(String("GET ") + URL + " HTTP/1.1\r\nHost: " + HOST + "\r\n" + "Connection: close\r\n\r\n");
  delay(500);
  String resp;
  // Read last line only.
  while (client.available()) {
    resp = client.readStringUntil('\n');
  }

  // Initialize umbrella state to unknown (-1)
  memcpy(umbrellaState, UMBRELLA_STATE_UNKNOWN, sizeof(UMBRELLA_STATE_UNKNOWN));
  int s = 0;
  int i = 0;
  // Parse CSV string
  while(s < resp.length() && i < STATE_COUNT) {
    int c = resp.indexOf(',', s);
    String seg = resp.substring(s, c);
    umbrellaState[i] = seg.toInt();
    s = c + 1;
    i += 1;
  }
  // Set cache refresh time.
  cacheRefreshTime = millis();
}
/**
 * Use the umbrellaState and display the LEDs depending on the chance of rain.
 */
void showUmbrellaState() {
  Serial.println("Displaying Umbrella:");
  for (int i = 0; i < STATE_COUNT; ++i) {
    int u = umbrellaState[i];
    if        (u < 0)    { setPixel(i, RED);   Serial.printf("%3d: RED\n", u);   // Unknown
    } else if (u <= 30)  { setPixel(i, OFF);   Serial.printf("%3d: OFF\n", u);   // No rain
    } else if (u <= 70)  { setPixel(i, WHITE); Serial.printf("%3d: WHITE\n", u); // Maybe rain
    } else if (u <= 100) { setPixel(i, BLUE);  Serial.printf("%3d: BLUE\n", u);  // Rain
    } else               { setPixel(i, WHITE); Serial.printf("%3d: WHITE\n", u); // Error
    }
  }
  pixels.show();
  Serial.println("Done checking");
}

/**
 * Get battery level as a percent.
 */
int batteryLevel() {
  // read the battery level from the ESP8266 analog in pin.
  // analog read level is 10 bit 0-1023 (0V-1V).
  // our 1M & 220K voltage divider takes the max
  // lipo value of 4.2V and drops it to 0.758V max.
  // this means our min analog read value should be 580 (3.14V)
  // and the max analog read value should be 774 (4.2V).
  int level = analogRead(A0);
  // convert battery level to percent
  int percent = map(level, 580, 774, 0, 100);
  Serial.printf("Battery level: %d, %d%%\n", level, percent);
  return percent;
}

/**
 * Setup (runs once).
 */
void setup() {
  // Set up some variables for printing.
  Serial.begin(115200);
  delay(100);

  // Set up motion sensor pin.
  pinMode(PIN_PIR, INPUT);

  // Set up LED strip.
  pixels.begin();
  pixels.setBrightness(16);
  pixels.show();

  // Set to green, this means we're trying to connect to the wifi.
  setPixels(GREEN);

  // Connect to wifi.
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // LEDs off, we're connected.
  setPixels(OFF);
}

/**
 * Loop (runs continuously).
 */
void loop() {
  // If we are detecting motion.
  if (digitalRead(PIN_PIR) == HIGH) {
    // And the LEDs are not currently on.
    if (pixelFlashTime == 0) {
      Serial.println("\nChecking weather");
      // Check the weather from the weather API.
      fetchUmbrellaState();
      // And light up the right LEDs.
      showUmbrellaState();
    }
    // Update timestamp since we've just turned on the LEDs.
    pixelFlashTime = millis();
  } else if (pixelFlashTime > 0 && millis() - pixelFlashTime > FLASH_DELAY) {
    Serial.printf("No motion detected, turning off (%d)\n", pixelFlashTime);
    // If no motion is detected and the time is up, turn everything off.
    setPixels(OFF);
    // Unset timer.
    pixelFlashTime = 0;
  }
  delay(1000);
}
