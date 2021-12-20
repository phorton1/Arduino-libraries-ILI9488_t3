
#include <ili9488_t3_font_ArialBold.h>
#include <ILI9488_t3.h>

#define TRY_EXTMEM
#define UPDATE_HALF_FRAME

#define ROTATION 3

#include "SPI.h"

//#define USE_SPI1
#if defined(USE_SPI1)
#if defined(__IMXRT1062__)  // Teensy 4.x 
#define TFT_DC 2
#define TFT_CS  0
#define TFT_RST 3

#define TFT_SCK 27
#define TFT_MISO 1
#define TFT_MOSI 26

#else
#define TFT_DC 31
#define TFT_CS 10 // any pin will work not hardware
#define TFT_RST 8
#define TFT_SCK 32
#define TFT_MISO 5
#define TFT_MOSI 21
//#define DEBUG_PIN 13
#endif
ILI9488_t3 tft = ILI9488_t3(&SPI1, TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCK, TFT_MISO);

//------------------------------------
#else // default pins
#define TFT_DC  9  // only CS pin 
#define TFT_CS 10   // using standard pin
#define TFT_RST 8
ILI9488_t3 tft = ILI9488_t3(&SPI, TFT_CS, TFT_DC, TFT_RST);
//--------------------------------------
#endif

uint16_t our_pallet[] = {
  ILI9488_BLACK,  ILI9488_RED, ILI9488_GREEN,  ILI9488_BLUE,
  ILI9488_YELLOW, ILI9488_ORANGE, ILI9488_CYAN, ILI9488_PINK
};

#define COUNT_SHUTDOWN_FRAMES 16
volatile uint8_t shutdown_cont_update_count = 0xff;

#ifdef ARDUINO_TEENSY41
extern "C" {
  extern uint8_t external_psram_size;
  EXTMEM RAFB extmem_frame_buffer[ILI9488_TFTWIDTH * ILI9488_TFTHEIGHT];
}
#endif


void setup() {
  while (!Serial && (millis() < 4000)) ;
  Serial.begin(115200);
  Serial.printf("Begin: CS:%d, DC:%dRST: %d\n", TFT_CS, TFT_DC, TFT_RST);
  Serial.printf("  Size of RAFB: %d\n", sizeof(RAFB));
  tft.begin(26000000);

#ifdef ARDUINO_TEENSY41
  if (external_psram_size) tft.setFrameBuffer(extmem_frame_buffer);
  else Serial.println("Warning this sketch is setup to run on Teensy 4.1 with external memory");
#else
  Serial.println("Warning this sketch is setup to run on Teensy 4.1 with external memory");
#endif

  tft.setRotation(ROTATION);
  tft.useFrameBuffer(true);
  tft.fillScreen(ILI9488_BLACK);
  tft.setCursor(ILI9488_t3::CENTER, ILI9488_t3::CENTER);
  tft.setTextColor(ILI9488_RED);
  tft.setFont(Arial_20_Bold);
  tft.println("*** Press key to start ***");
  tft.updateScreen();


  tft.setFrameCompleteCB(&frame_callback, true);
}

void frame_callback() {
  //Serial.printf("FCB: %d %d\n", tft.frameCount(), tft.subFrameCount());
  uint32_t frameCount = tft.frameCount();
  // See if end of test signalled.
  if (shutdown_cont_update_count == COUNT_SHUTDOWN_FRAMES) {
    uint8_t color_index = (frameCount >> 4) & 0x7;
    tft.setCursor(ILI9488_t3::CENTER, ILI9488_t3::CENTER);
    tft.setTextColor(our_pallet[(color_index + 3) & 7]);
    tft.setFont(Arial_20_Bold);
    tft.println("Stop Signalled");
    shutdown_cont_update_count--;
    #ifdef ARDUINO_TEENSY41
    arm_dcache_flush(extmem_frame_buffer, sizeof(extmem_frame_buffer));
    #endif
  } else if (shutdown_cont_update_count == 0) {
    tft.setCursor(ILI9488_t3::CENTER, tft.getCursorY());
    tft.println("endUpdateAsync");
    tft.endUpdateAsync();
    Serial.println("after endUpdateAsync");
    #ifdef ARDUINO_TEENSY41
    arm_dcache_flush(extmem_frame_buffer, sizeof(extmem_frame_buffer));
    #endif
  } else if (shutdown_cont_update_count < COUNT_SHUTDOWN_FRAMES) {
    shutdown_cont_update_count--;
  } else {
#ifdef UPDATE_HALF_FRAME
    bool draw_frame = false;
    if (((frameCount & 0xf) == 0) && tft.subFrameCount()) {
      draw_frame = true;
      tft.setClipRect(0, 0, tft.width(), tft.height() / 2);
    } else if (((frameCount & 0xf) == 1) && !tft.subFrameCount()) {
      draw_frame = true;
      tft.setClipRect(0, tft.height() / 2, tft.width(), tft.height() / 2);
    }
    if (draw_frame)
#else
    if (tft.subFrameCount()) {
      // lets ignore these right now
      return;
    }
    if ((frameCount & 0xf) == 0)
#endif
    {
      // First pass ignore subframe...
      uint8_t color_index = (frameCount >> 4) & 0x7;
      tft.fillScreen(our_pallet[color_index]);
      tft.drawRect(5, 5, tft.width() - 10, tft.height() - 10, our_pallet[(color_index + 1) & 7]);
      tft.drawRect(25, 25, tft.width() - 50, tft.height() - 50, our_pallet[(color_index + 2) & 7]);

      static uint8_t display_other = 0;
      switch (display_other) {
        case 0:
          tft.fillRect(50, 50, tft.width() - 100, tft.height() - 100, our_pallet[(color_index + 1) & 7]);
          break;
        case 1:
          tft.fillCircle(tft.width() / 2, tft.height() / 2, 100, our_pallet[(color_index + 1) & 7]);
          break;
        case 2:
          tft.fillTriangle(50, 50, tft.width() - 50, 50, tft.width() / 2, tft.height() - 50, our_pallet[(color_index + 1) & 7]);
          break;
      }
      if (!tft.subFrameCount()) {
        display_other++;
        if (display_other > 2) display_other =  0 ;
      }

      #ifdef ARDUINO_TEENSY41
      arm_dcache_flush(extmem_frame_buffer, sizeof(extmem_frame_buffer));
      #endif
      tft.setClipRect();
    }
  }

}

void loop(void) {
  // See if any text entered
  int ich;
  if ((ich = Serial.read()) != -1) {
    while (Serial.read() != -1) ;

    if (!tft.asyncUpdateActive()) {
      // We are not running DMA currently so start it up.
      Serial.println("Starting up DMA Updates");
      shutdown_cont_update_count = 0xff;
      tft.updateScreenAsync(true);
    } else {
      shutdown_cont_update_count = COUNT_SHUTDOWN_FRAMES;
      while (shutdown_cont_update_count) ;
      tft.waitUpdateAsyncComplete();
      tft.setCursor(ILI9488_t3::CENTER, tft.getCursorY());
      tft.print("Finished Test\n");
      Serial.println("after waitUpdateAsyncComplete");
      Serial.println("Finished test");

      delay(2000);
      Serial.println("Do normal update to see if data is there");
      tft.updateScreen();
    }
  }
}