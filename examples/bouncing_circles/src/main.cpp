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

// Number of circles to draw
#define CNUMBER 42

// Define the width and height according to the TFT and the
// available memory. The sprites will require:
//     DWIDTH * DHEIGHT * 2 bytes of RAM
// Note: for a 240 * 240 area this is 150 Kbytes!
#define DWIDTH  TFT_WIDTH
#define DHEIGHT TFT_HEIGHT

// Create two sprites for a DMA toggle buffer
TFT_eSprite spr[2] = {TFT_eSprite(&tft), TFT_eSprite(&tft)};

// Pointers to start of Sprites in RAM (these are then "image" pointers)
uint16_t* sprPtr[2];

// Used for fps measuring
uint16_t counter = 0;
int32_t startMillis = millis();
uint16_t interval = 100;
String fps = "xx.xx fps";

// Structure to hold circle plotting parameters
typedef struct circle_t {
  int16_t   cx[CNUMBER] = { 0 }; // x coordinate of centre
  int16_t   cy[CNUMBER] = { 0 }; // y coordinate of centre
  int16_t   cr[CNUMBER] = { 0 }; // radius
  uint16_t col[CNUMBER] = { 0 }; // colour
  int16_t   dx[CNUMBER] = { 0 }; // x movement & direction
  int16_t   dy[CNUMBER] = { 0 }; // y movement & direction
} circle_param;

// Create the structure and get a pointer to it
circle_t *circle = new circle_param;





// #########################################################################
// Render circles to sprite 0 or 1 and initiate DMA
// #########################################################################
void drawUpdate (bool sel) {
  spr[sel].fillSprite(TFT_BLACK);
  for (uint16_t i = 0; i < CNUMBER; i++) {
    // Draw (Note sprite 1 datum was moved, so coordinates do not need to be adjusted
    spr[sel].fillCircle(circle->cx[i], circle->cy[i], circle->cr[i], circle->col[i]);
    spr[sel].drawCircle(circle->cx[i], circle->cy[i], circle->cr[i], TFT_WHITE);
    spr[sel].setTextColor(TFT_BLACK, circle->col[i]);
    spr[sel].drawNumber(i + 1, 1 + circle->cx[i], circle->cy[i], 2);
  }

  tft.pushImageDMA(0, sel * DHEIGHT / 2, DWIDTH, DHEIGHT / 2, sprPtr[sel]);

  // Update circle positions after bottom half has been drawn
  if (sel) {
    for (uint16_t i = 0; i < CNUMBER; i++) {
      circle->cx[i] += circle->dx[i];
      circle->cy[i] += circle->dy[i];
      if (circle->cx[i] <= circle->cr[i]) {
        circle->cx[i] = circle->cr[i];
        circle->dx[i] = -circle->dx[i];
      }
      else if (circle->cx[i] + circle->cr[i] >= DWIDTH - 1) {
        circle->cx[i] = DWIDTH - circle->cr[i] - 1;
        circle->dx[i] = -circle->dx[i];
      }
      if (circle->cy[i] <= circle->cr[i]) {
        circle->cy[i] = circle->cr[i];
        circle->dy[i] = -circle->dy[i];
      }
      else if (circle->cy[i] + circle->cr[i] >= DHEIGHT - 1) {
        circle->cy[i] = DHEIGHT - circle->cr[i] - 1;
        circle->dy[i] = -circle->dy[i];
      }
    }
  }
}

// #########################################################################
// Return a 16 bit rainbow colour
// #########################################################################
uint16_t rainbow(byte value)
{
  // If 'value' is in the range 0-159 it is converted to a spectrum colour
  // from 0 = red through to 127 = blue to 159 = violet
  // Extending the range to 0-191 adds a further violet to red band

  value = value % 192;

  byte red   = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0; // Green is the middle 6 bits, but only top 5 bits used here
  byte blue  = 0; // Blue is the bottom 5 bits

  byte sector = value >> 5;
  byte amplit = value & 0x1F;

  switch (sector)
  {
    case 0:
      red   = 0x1F;
      green = amplit; // Green ramps up
      blue  = 0;
      break;
    case 1:
      red   = 0x1F - amplit; // Red ramps down
      green = 0x1F;
      blue  = 0;
      break;
    case 2:
      red   = 0;
      green = 0x1F;
      blue  = amplit; // Blue ramps up
      break;
    case 3:
      red   = 0;
      green = 0x1F - amplit; // Green ramps down
      blue  = 0x1F;
      break;
    case 4:
      red   = amplit; // Red ramps up
      green = 0;
      blue  = 0x1F;
      break;
    case 5:
      red   = 0x1F;
      green = 0;
      blue  = 0x1F - amplit; // Blue ramps down
      break;
  }

  return red << 11 | green << 6 | blue;
}


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
  
  tft.fillScreen(TFT_BLACK);

  // Create the 2 sprites, each is half the size of the screen
  sprPtr[0] = (uint16_t*)spr[0].createSprite(DWIDTH, DHEIGHT / 2);
  sprPtr[1] = (uint16_t*)spr[1].createSprite(DWIDTH, DHEIGHT / 2);

  // Move the sprite 1 coordinate datum upwards half the screen height
  // so from coordinate point of view it occupies the bottom of screen
  spr[1].setViewport(0, -DHEIGHT / 2, DWIDTH, DHEIGHT);

  // Define text datum for each Sprite
  spr[0].setTextDatum(MC_DATUM);
  spr[1].setTextDatum(MC_DATUM);

  // Seed the random number generator
  randomSeed(digitalRead(22));

  // Initialise circle parameters
  for (uint16_t i = 0; i < CNUMBER; i++) {
    circle->cr[i] = random(12, 24);
    circle->cx[i] = random(circle->cr[i], DWIDTH - circle->cr[i]);
    circle->cy[i] = random(circle->cr[i], DHEIGHT - circle->cr[i]);
    
    circle->col[i] = rainbow(4 * i);
    circle->dx[i] = random(1, 5);
    if (random(2)) circle->dx[i] = -circle->dx[i];
    circle->dy[i] = random(1, 5);
    if (random(2)) circle->dy[i] = -circle->dy[i];
  }

  tft.startWrite(); // TFT chip select held low permanently

  startMillis = millis();
}

/////////////////////////////////////////////////////////// loop ///////////////////////////////////////////////////
void loop() {
  drawUpdate(0); // Update top half
  drawUpdate(1); // Update bottom half

  // Calculate the fps every <interval> iterations.
  counter++;  
  if (counter % interval == 0) {
    long millisSinceUpdate = millis() - startMillis;
    fps = String((interval * 1000.0 / (millisSinceUpdate))) + " fps";
    Serial.println(fps);
    startMillis = millis();
  }
}
