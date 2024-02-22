#define USE_ESP_IDF_LOG
#include <esp_log.h>
#include <Arduino.h>
#include <M5Unified.h>
#include <math.h>
#include <esp_task.h>
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
#include <freertos/task.h>
#include <freertos/semphr.h>
// Library instance
TFT_eSPI    tft = TFT_eSPI();         // Declare object "tft"


uint8_t tftDebugDrawCount = 0;
#define DOBULE_BUFFERING 1
#ifdef DOBULE_BUFFERING
  TFT_eSprite tftBuf[2] = {TFT_eSprite(&tft),TFT_eSprite(&tft)};  // Declare Sprite object "spr" with pointer to "tft" object
#else
  TFT_eSprite tftBuf[1] = {TFT_eSprite(&tft)};  // Declare Sprite object "spr" with pointer to "tft" object  
#endif
uint8_t currentTftBuffer = 0;

SemaphoreHandle_t tftBufferXSemaphore = NULL;

void startDrawTransaction(){
  xSemaphoreTake(tftBufferXSemaphore, portMAX_DELAY);
}

void endDrawTransaction(){
  xSemaphoreGive(tftBufferXSemaphore);
}

bool bufferInitCompleted = false;
void initBuffers(){
  tftBufferXSemaphore = xSemaphoreCreateMutex();
  startDrawTransaction();
  tftBuf[0].createSprite(TFT_WIDTH, TFT_HEIGHT);
  #ifdef DOBULE_BUFFERING
    tftBuf[1].createSprite(TFT_WIDTH, TFT_HEIGHT);
  #endif
  endDrawTransaction();
  bufferInitCompleted = true;
}

TFT_eSprite* getTftBuffer(){
  if(!bufferInitCompleted){
    return nullptr;
  }
  TFT_eSprite *localBuffer = nullptr;
  localBuffer = &tftBuf[currentTftBuffer];
  return localBuffer;
}

// Returns the old sprite
TFT_eSprite* swapTftBuffer(){  
  if(!bufferInitCompleted){
    return nullptr;
  }
  #ifdef DOBULE_BUFFERING
    startDrawTransaction();
    uint8_t oldSprite = currentTftBuffer;
    currentTftBuffer = 1 - currentTftBuffer;
    tftBuf[oldSprite].pushToSprite(&tftBuf[currentTftBuffer],0,0);
    endDrawTransaction();
    return &tftBuf[oldSprite];
  #else
    return &tftBuf[0];  
  #endif 
}




TaskHandle_t  displayTask[1];
enum TaskIndex{
  HEAVY_TASK1 = 0,
  HEAVY_TASK2 = 1,
  HEAVY_TASK3 = 2,
  TFT_TASK = 4,
  MAIN_TASK = 99,
};

TaskHandle_t  suspendableTask[3];


#define TEXT_X 10
#define TEXT_Y 100
#define TEXT_SIZE 4  
#define COLOR_COUNT 6  
uint16_t bgColors[COLOR_COUNT] = {TFT_WHITE, TFT_RED, TFT_YELLOW, TFT_GREEN, TFT_BLUE, TFT_PURPLE};
uint16_t fgColors[COLOR_COUNT] = {TFT_RED, TFT_BLUE, TFT_GREEN, TFT_RED, TFT_WHITE, TFT_BLACK};
uint8_t colorIndex = 0;
uint16_t cpuLoad = 100;
uint16_t cpuLPerc = 0;


void suspendTasks(){
  for(int i=0; i<1; i++){
    vTaskSuspend( suspendableTask[i] );
  }
}

void resumeTasks(){
  for(int i=0; i<1; i++){
    vTaskResume( suspendableTask[i] );
  }
}

//Heavy task simulation
void heavyTask(void *args) {
  double sinTest = 0.0f, cosTest = 0.0f;
  //delay(100);
   TickType_t xLastWakeTime;
   const TickType_t xFrequency = 10;
   BaseType_t xWasDelayed;
   
  while (1) {
    /*if(cpuLoad==0){
      vTaskSuspend( NULL );
    }*/
    for(int i=0; i<cpuLoad; i++){
      float randomVal = random(100) / 100;
      sinTest = sin(randomVal);
      cosTest = cos(randomVal);
      ESP_LOGI("MAIN", "Heavy task loop %d", i);
    }
    xWasDelayed = xTaskDelayUntil( &xLastWakeTime, xFrequency );
    xLastWakeTime = xTaskGetTickCount ();
  }
}


void tftTask(void *args) {
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = portTICK_RATE_MS*50; // 20ms
  BaseType_t xWasDelayed;
  while (1) {
    TFT_eSprite* readyBuffer = swapTftBuffer();
    #ifndef DOBULE_BUFFERING
      startDrawTransaction();
    #endif
    suspendTasks();
    readyBuffer->setCursor(TEXT_X+40, TEXT_Y+40, TEXT_SIZE);
    readyBuffer->fillRect(TEXT_X+40, TEXT_Y+40, 110, 25, TFT_WHITE);
    readyBuffer->setTextColor(TFT_BLACK);
    readyBuffer->printf("Draw %d", tftDebugDrawCount++);
    readyBuffer->pushSprite(0, 0);
    #ifndef DOBULE_BUFFERING
      endDrawTransaction();
    #endif  
    resumeTasks();
    vTaskDelay( xFrequency );
    xLastWakeTime = xTaskGetTickCount ();
  }
}


int serial_vprintf(const char *format, va_list args) {
  return Serial.printf(format, args);
}

/////////////////////////////////////////////////////////// setup ///////////////////////////////////////////////////
void setup() {
  esp_log_set_vprintf(serial_vprintf);
  Serial.begin(115200);
   while (!Serial)
  {
  }
  Serial.setDebugOutput(true);
  delay(1000); // give me time to start the monitor
  ESP_LOGI("MAIN", "Initing M5Dial TFT_eSPI");
  M5.begin();
  M5.Display.setRotation(2);
  ESP_LOGI("MAIN", "Calling tft.init()");
  delay(150);

  

#ifdef USE_DMA_TO_TFT
  // DMA - should work with ESP32, STM32F2xx/F4xx/F7xx processors
  // NOTE: >>>>>> DMA IS FOR SPI DISPLAYS ONLY <<<<<<
  tft.initDMA(); // Initialise the DMA engine (tested with STM32F446 and STM32F767)
#endif
  // Create a sprite of defined size
  initBuffers();
  ESP_LOGI("MAIN", "Loop");
  startDrawTransaction();
  getTftBuffer()->fillSprite(TFT_BLACK);
  getTftBuffer()->setTextColor(TFT_WHITE);
  getTftBuffer()->println("Setting up...");
  endDrawTransaction();
  cpuLoad=0;
  
  xTaskCreatePinnedToCore(heavyTask, "heavyTask1", 4096, NULL, ESP_TASK_TCPIP_PRIO+2, &suspendableTask[0], 1);
  xTaskCreate(tftTask, "tftTask", 4096, NULL, ESP_TASK_MAIN_PRIO, &displayTask[0]);
  delay(500);
}






/////////////////////////////////////////////////////////// loop ///////////////////////////////////////////////////

void loop() {
        //#ifndef DOBULE_BUFFERING
          startDrawTransaction();
        //#endif  
        getTftBuffer()->fillSprite(bgColors[colorIndex]);
        getTftBuffer()->setTextColor(fgColors[colorIndex]);
        getTftBuffer()->setCursor(TEXT_X, TEXT_Y, TEXT_SIZE);
        getTftBuffer()->printf("CPU LOAD %d%%", cpuLPerc);
        //#ifndef DOBULE_BUFFERING
          endDrawTransaction();
       // #endif  
        colorIndex++;
        if(colorIndex>=COLOR_COUNT){
          colorIndex=0;
          if(cpuLPerc>=100){
              cpuLPerc=0;
          }
          else
          if(cpuLPerc<80){
              // Skipping the 10-30-50-70 cpu Load, as it has no major visual difference, but 90 and 100 have a lot
              cpuLPerc+=20;
          }
          else{
              cpuLPerc+=10;
          }
          cpuLoad = cpuLPerc*4.48f;
          resumeTasks();
        }
        delay(800);   
}
