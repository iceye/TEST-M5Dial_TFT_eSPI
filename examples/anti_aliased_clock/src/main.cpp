#include <Arduino.h>
#include <M5Unified.h>
// TFT_eSPI library demo, principally for STM32F processors with DMA:
// https://en.wikipedia.org/wiki/Direct_memory_access

// Tested with ESP32, Nucleo 64 STM32F446RE and Nucleo 144 STM32F767ZI
// TFT's with SPI can use DMA, the sketch also works with 8-bit
// parallel TFT's (tested with ILI9341 and ILI9481)

// The sketch will run on processors without DMA and also parallel
// interface TFT's. Comment out line 29 for no DMA.

// Library here:
// https://github.com/Bodmer/TFT_eSPI

// NOTE: FOR SPI DISPLAYS ONLY
#define USE_DMA_TO_TFT 1
#define CONFIG_TFT_GC9A01_DRIVER 1
#define GC9A01_DRIVER 1

#include <TFT_eSPI.h>

// Library instance
TFT_eSPI    tft = TFT_eSPI();         // Declare object "tft"
TFT_eSprite spr = TFT_eSprite(&tft);  // Declare Sprite object "spr" with pointer to "tft" object

#define LOOP_DELAY 0 // This controls how frequently the meter is updated
                     // for test purposes this is set to 0

#define DARKER_GREY 0x18E3

uint32_t runTime = 0;       // time for next update

int reading = 0; // Value to be displayed
int d = 0; // Variable used for the sinewave test waveform
bool range_error = 0;
int8_t ramp = 1;

bool initMeter = true;

/////////////////////////////////////////////////////////// setup ///////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  ESP_LOGI("MAIN", "Initing M5Dial TFT_eSPI");
  M5.begin();
  
  ESP_LOGI("MAIN", "Calling tft.init()");
  delay(150);

  


#ifdef USE_DMA_TO_TFT
  // DMA - should work with ESP32, STM32F2xx/F4xx/F7xx processors
  // NOTE: >>>>>> DMA IS FOR SPI DISPLAYS ONLY <<<<<<
  tft.initDMA(); // Initialise the DMA engine (tested with STM32F446 and STM32F767)
#endif
  ESP_LOGI("MAIN", "Loop");
  
  tft.fillScreen(TFT_NAVY);
}


// #########################################################################
//  Draw the meter on the screen, returns x coord of righthand side
// #########################################################################
// x,y is centre of meter, r the radius, val a number in range 0-100
// units is the meter scale label
void ringMeter(int x, int y, int r, int val, const char *units)
{
  static uint16_t last_angle = 30;

  if (initMeter) {
    initMeter = false;
    last_angle = 30;
    tft.fillCircle(x, y, r, DARKER_GREY);
    tft.drawSmoothCircle(x, y, r, TFT_SILVER, DARKER_GREY);
    uint16_t tmp = r - 3;
    tft.drawArc(x, y, tmp, tmp - tmp / 5, last_angle, 330, TFT_BLACK, DARKER_GREY);
  }

  r -= 3;

  // Range here is 0-100 so value is scaled to an angle 30-330
  int val_angle = map(val, 0, 100, 30, 330);


  if (last_angle != val_angle) {
    // Could load the required font here
    //if (ofr.loadFont(TTF_FONT, sizeof(TTF_FONT))) {
    //  Serial.println("Render initialize error");
    //  return;
    //}
#ifdef DRAW_DIGITS
    ofr.setDrawer(spr); // Link renderer to sprite (font will be rendered in sprite spr)

    // Add value in centre if radius is a reasonable size
    if ( r >= 25 ) {
      // This code gets the font dimensions in pixels to determine the required the sprite size
      ofr.setFontSize((6 * r) / 4);
      ofr.setFontColor(TFT_WHITE, DARKER_GREY);


      // The OpenFontRender library only has simple print functions...
      // Digit jiggle for chaging values often happens with proportional fonts because
      // digit glyph width varies ( 1 narrower that 4 for example). This code prints up to
      // 3 digits with even spacing.
      // A few experiemntal fudge factors are used here to position the
      // digits in the sprite...
      // Create a sprite to draw the digits into
      uint8_t w = ofr.getTextWidth("444");
      uint8_t h = ofr.getTextHeight("4") + 4;
      spr.createSprite(w, h + 2);
      spr.fillSprite(DARKER_GREY); // (TFT_BLUE); // (DARKER_GREY);
      char str_buf[8];         // Buffed for string
      itoa (val, str_buf, 10); // Convert value to string (null terminated)
      uint8_t ptr = 0;         // Pointer to a digit character
      uint8_t dx = 4;          // x offfset for cursor position
      if (val < 100) dx = ofr.getTextWidth("4") / 2; // Adjust cursor x for 2 digits
      if (val < 10) dx = ofr.getTextWidth("4");      // Adjust cursor x for 1 digit
      while ((uint8_t)str_buf[ptr] != 0) ptr++;      // Count the characters
      while (ptr) {
        ofr.setCursor(w - dx - w / 20, -h / 2.5);    // Offset cursor position in sprtie
        ofr.rprintf(str_buf + ptr - 1);              // Draw a character
        str_buf[ptr - 1] = 0;                        // Replace character with a null
        dx += 1 + w / 3;                             // Adjust cursor for next character
        ptr--;                                       // Decrement character pointer
      }
      spr.pushSprite(x - w / 2, y - h / 2); // Push sprite containing the val number
      spr.deleteSprite();                   // Recover used memory

      // Make the TFT the print destination, print the units label direct to the TFT
      ofr.setDrawer(tft);
      ofr.setFontColor(TFT_GOLD, DARKER_GREY);
      ofr.setFontSize(r / 2.0);
      ofr.setCursor(x, y + (r * 0.4));
      ofr.cprintf("Watts");
    }
#endif

    //ofr.unloadFont(); // Recover space used by font metrics etc

    // Allocate a value to the arc thickness dependant of radius
    uint8_t thickness = r / 5;
    if ( r < 25 ) thickness = r / 3;

    // Update the arc, only the zone between last_angle and new val_angle is updated
    if (val_angle > last_angle) {
      tft.drawArc(x, y, r, r - thickness, last_angle, val_angle, TFT_SKYBLUE, TFT_BLACK); // TFT_SKYBLUE random(0x10000)
    }
    else {
      tft.drawArc(x, y, r, r - thickness, val_angle, last_angle, TFT_BLACK, DARKER_GREY);
    }
    last_angle = val_angle; // Store meter arc position for next redraw
  }
}

/////////////////////////////////////////////////////////// loop ///////////////////////////////////////////////////
void loop() {
  static uint16_t maxRadius = 0;
  int8_t ramp = 1;
  static uint8_t radius = 0;
  static int16_t xpos = tft.width() / 2;
  static int16_t ypos = tft.height() / 2;
  bool newMeter = false;

  if (maxRadius == 0) {
    maxRadius = tft.width();
    if (tft.height() < maxRadius) maxRadius = tft.height();
    maxRadius = (0.6 * maxRadius) / 2;
    radius = maxRadius;
  }

  // Choose a random meter radius for test purposes and draw for one range cycle
  // Clear old meter first
  tft.fillCircle(xpos, ypos, radius + 1, TFT_NAVY);
  radius = random(20, maxRadius); // Random radius
  initMeter = true;



  initMeter = true;
  reading = 0;
  ramp = 1;
  while (!newMeter) {
    if (millis() - runTime >= LOOP_DELAY) {
      runTime = millis();

      reading += ramp;
      ringMeter(xpos, ypos, radius, reading, "Watts"); // Draw analogue meter

      if (reading > 99) ramp = -1;
      if (reading <=  0) ramp = 1;

      if (reading > 99) delay(1000);
      if (reading <= 0) {
        delay(1000);
        newMeter = true;
      }
    }
  }


}


