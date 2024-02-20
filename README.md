
# TEST-M5Dial_TFT_eSPI

Just a test for the **M5Dial** using poorly documented **TFT_eSPI** for this dev board
To run any sample you MUST edit the **TFT_eSPI** configuration to make it works with **M5Dial**:
Edit the **TFT_eSPI** lib configuration.


## 1 for VSCode: Locate the TFT_eSPI User_Setup_Select
In **VSCode** (once lib are loaded, under the project you should find *.pio/libdeps/m5stack-stamps3/TFT_eSPI/User_Setup_Select.h*

## 1 for Arduino: Locate the TFT_eSPI User_Setup_Select
Find the **Arduino** lib folder (depends on your OS) and find the *TFT_eSPI/User_Setup_Select.h*

## 2: Include both M5Unified and TFT_eSPI in any project

    #include <M5Unified.h>
    #define USE_DMA_TO_TFT 1
    #define CONFIG_TFT_GC9A01_DRIVER 1
    #define GC9A01_DRIVER 1
    #include <TFT_eSPI.h>

## 3: Add the Driver Include line Setup70h_ESP32_S3_GC9A01.h, ANY other driver include MUST be a comment:

    ...
    ...
    // Only ONE line below should be uncommented to define your setup. Add extra lines and files as needed.
    #include <User_Setup.h> // Default setup is root library folder
    #include <User_Setups/Setup70h_ESP32_S3_GC9A01.h> // <- THIS IS THE DRIVER FOR M5Dial with ESP32S3
    //#include <User_Setups/Setup1_ILI9341.h> // Setup file for ESP8266 configured for my ILI9341
    //#include <User_Setups/Setup2_ST7735.h> // Setup file for ESP8266 configured for my ST7735
    ...
    ...
    ...

## 4: Edit the Setup70h_ESP32_S3_GC9A01.h because the default one is not working, here there's the FULL file content

    // Setup for the ESP32 S3 with GC9A01 display
    #define USER_SETUP_ID 70
    #define GC9A01_DRIVER
    #define TFT_WIDTH 240
    #define TFT_HEIGHT 240
    // Based on M5Unified/MGFX Pins for the M5Dial
    #define TFT_CS 7
    #define TFT_MOSI 5
    #define TFT_SCLK 6
    #define TFT_MISO -1
    #define TFT_DC 4
    #define TFT_RST 8
    #define TFT_BL 9
    #define LOAD_GLCD
    #define LOAD_FONT2
    #define LOAD_FONT4
    #define LOAD_FONT6
    #define LOAD_FONT7
    #define LOAD_FONT8
    #define LOAD_GFXFF
    #define SMOOTH_FONT
    #define USE_HSPI_PORT //WHITOUT THIS THE DISPLAY IS NOT WORKING
    #define SPI_FREQUENCY 80000000 // Based on value found in M5Unified for M5Dial
    #define SPI_READ_FREQUENCY 16000000 // Based on value found in M5Unified for M5Dial
    #define TFT_BACKLIGHT_ON HIGH

### NOTE for the TFT_* pins
the pins has been detected by looking around in the M5Unified lib, you can find these lines in it:

    _pin_reset(GPIO_NUM_8, use_reset); // LCD RST
    bus_cfg.pin_mosi = GPIO_NUM_5;
    bus_cfg.pin_miso = GPIO_NUM_NC;
    bus_cfg.pin_sclk = GPIO_NUM_6;
    bus_cfg.pin_dc = GPIO_NUM_4;
    bus_cfg.spi_mode = 0;
    bus_cfg.spi_3wire = true;
    bus_spi->config(bus_cfg);
    bus_spi->init();
    id = _read_panel_id(bus_spi, GPIO_NUM_7);

## 5: Correct setup() procedure, note, any order different from this will prob make the screen look wired (slow/semi-painted/wrong color/broken screen look)

    void setup() {
    ...
    ...
    M5.begin();
    delay(50);
    #ifdef USE_DMA_TO_TFT
    // NOTE: >>>>>> DMA IS FOR SPI DISPLAYS ONLY <<<<<<
    tft.initDMA(); // Initialise the DMA engine (tested with STM32F446 and STM32F767)
    #endif
    tft.fillScreen(BGCOLOR);
    // NOW YOU CAN USE TFT_eSPI
    ...
    ...
    ...

## FAQ:
- *Is it really working?*
I tested quickly but you can try yourself some of the examples.

- *Why just 3 examples?*
As I said, I tested quickly but you can try yourself some of the examples you can find in the TFT_eSPI repo.

- *Will you support this project?*
Mh, prob not, I'm just sharing a few hours I worked on it mostly because there's almost zero documentation about this and I need to reproduce an issue.

- *Why do I have to call M5.begin() if I'm using TFT_eSPI?*
First without it the display is not working (prob because the init process in TFT_eSPI for GC9A01 is buggy/wrong/incomplete, you can try yourself). Second the M5 lib will give you access to the other features like Speaker, Encoder, Buttons, etc

- *Is it useful?*
I don't know, this implementation will not be used in production

- *Are there any alternatives to TFT_eSPI?*
Yes: MGFX are working quite well for very simple tasks. LVGL are VERY VERY complex to use but are almost the best out there for this board.

# This project will end here