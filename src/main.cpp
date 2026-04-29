// RP2040 + 240x240 TFT (GC9A01 / ST7789 style) using Arduino framework + TFT_eSPI
// Make sure User_Setup.h or User_Setup_Select.h is configured first.

#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

unsigned long lastUpdate = 0;
int counter = 0;

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);        // 0..3
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);   // middle center

  // Startup screen
  tft.fillScreen(TFT_BLACK);
  tft.drawCircle(120, 120, 118, TFT_DARKGREY);
  tft.drawCircle(120, 120, 117, TFT_DARKGREY);

  tft.setTextSize(2);
  tft.drawString("RP2040 TFT", 120, 90);

  tft.setTextSize(1);
  tft.drawString("TFT_eSPI Example", 120, 115);

  tft.fillCircle(120, 160, 8, TFT_GREEN);

  delay(1500);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    counter++;

    // Clear screen
    tft.fillScreen(TFT_BLACK);

    // Border ring (nice on round GC9A01 displays too)
    tft.drawCircle(120, 120, 118, TFT_BLUE);

    // Title
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("HELLO", 120, 40);

    // Counter
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(4);
    tft.drawString(String(counter), 120, 120);

    // Footer
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("Arduino + RP2040", 120, 210);
  }
}