/*
 * 298.666 MHz Frequency Counter using STM32F103C8T6
 * Project: 298.666mhz frequency counter using stm32f103c8t6 &
 *          vhf/uhf fm transmitter circuit
 *
 * POWERED BY [mikey:hexoid-sys]
 * Author: Yogesh R. Chauhan (GitHub: mikey-7x; also known as
 *         Chauhan Yogeshkumar R. and by the online pseudonym "mikey")
 * GitHub profile: https://github.com/mikey-7x
 * Project repository:
 *   https://github.com/mikey-7x/298.666mhz-frequency-counter-using-stm32f103c8t6-and-vhf-uhf-fm-transmitter-circuit
 * Copyright (c) 2026 Yogesh R. Chauhan (mikey-7x). All rights reserved.
 * Licence: mikey-7x Strict Personal & Educational Use Licence.
 * Personal and educational use only.  The author's name, the supplied
 * logo, this notice and the exact line "POWERED BY [mikey:hexoid-sys]"
 * must be retained wherever the licence requires attribution.  Selling,
 * commercial exploitation and industrial use are prohibited unless the
 * author gives separate prior written permission.  See the repository
 * LICENSE and NOTICE files for the complete binding terms.
 *
 * PLATFORM AND WIRING
 * STM32duino Blue Pill F103C8 (STM32F103C8T6), signal input on PA0 and a
 * 0.96-inch SSD1306 I2C OLED on PB6/PB7 or PB8/PB9.  No external Arduino
 * libraries are required.  PA0 expects a logic-level waveform that stays
 * within the microcontroller's electrical limits.
 *
 * RANGE ARCHITECTURE
 * TIM2's asynchronous ETR prescaler divides the input by eight before
 * the two-flip-flop timer resynchronizer.  TIM3 counts TIM2 overflows
 * through TRGO/ITR1, forming a coherent hardware TIM3:TIM2 32-bit
 * counter.  The verified timer clock is measured at startup and used to
 * derive two ceilings:
 *   designMaxHz  = 2 * TIM2CLK       (ETRP <= TIM2CLK/4 design rule)
 *   displayMaxHz = 8 * TIM2CLK / 3   (resynchronizer /3 boundary)
 * With the default 112 MHz timer clock these are 224 MHz and
 * 298.666... MHz.  Results above designMaxHz are displayed with the
 * honest unit marker "MHz EXP"; they are experimental and are not a
 * guarantee for every board, waveform, duty cycle or threshold level.
 *
 * A 1 GHz input would still present 125 MHz after ETR/8 and would need
 * at least a 375 MHz timer clock by the /3 relationship, so accurate
 * 1 GHz counting is not claimed and is not possible through this stock
 * STM32F103 input path by software alone.  TIM2's synchronous PSC sits
 * after resynchronization and cannot recover pulses already dropped.
 *
 * MEASUREMENT INTEGRITY
 * ETR/8 performs high-rate counting.  At and below 40 MHz, TIM2's TI1F
 * path on the same PA0 pin independently cross-checks the fundamental,
 * including a longer filter confirmation before selected integer-ratio
 * corrections are accepted.  ETR/1 edge timing is used below 2 kHz.
 * A changing or unsynchronized gate is rejected instead of displaying
 * a sweep average.  A disconnected input produces NO SIGNAL, never an
 * invented frequency.  Six 100 ms high-range tracking gates and two
 * 250 ms precision gates help reacquire a hand-tuned oscillator while
 * retaining the same stability qualification rules.  No table snapping,
 * GPIO-level frequency estimation, calibration disguise or internal
 * clock substitution is used.
 */
#include <Wire.h>

#define HEXOID_PROJECT_TITLE "298.666mhz frequency counter using stm32f103c8t6 & vhf/uhf fm transmitter circuit"
#define HEXOID_AUTHOR "mikey-7x"
#define HEXOID_AUTHOR_LEGAL_NAME "Yogesh R. Chauhan"
#define HEXOID_AUTHOR_ALSO_KNOWN_AS "Chauhan Yogeshkumar R."
#define HEXOID_AUTHOR_DISPLAY "Yogesh R. Chauhan (mikey-7x)"
#define HEXOID_GITHUB_PROFILE_URL "https://github.com/mikey-7x"
#define HEXOID_GITHUB_REPOSITORIES_URL "https://github.com/mikey-7x?tab=repositories"
#define HEXOID_REPOSITORY_URL "https://github.com/mikey-7x/298.666mhz-frequency-counter-using-stm32f103c8t6-and-vhf-uhf-fm-transmitter-circuit"
#define HEXOID_YOUTUBE_URL "https://youtube.com/@mikey-7x"
#define HEXOID_BLOG_URL "https://rtod-yv8.blogspot.com/?m=1"
#define HEXOID_COPYRIGHT "Copyright (c) 2026 Yogesh R. Chauhan (mikey-7x). All rights reserved."
#define HEXOID_POWERED_BY "POWERED BY [mikey:hexoid-sys]"
#define HEXOID_LICENCE_NAME "mikey-7x Strict Personal & Educational Use Licence"
#define HEXOID_OLED_BRAND "mikey-7x COUNTER"
#define HEXOID_OLED_POWERED_1 "POWERED BY"
#define HEXOID_OLED_POWERED_2 "[mikey:hexoid-sys]"
static const char HEXOID_FIRMWARE_IDENTITY[] =
  HEXOID_POWERED_BY
  " | Author: " HEXOID_AUTHOR_DISPLAY
  " | Also known as: " HEXOID_AUTHOR_ALSO_KNOWN_AS
  " | GitHub: " HEXOID_GITHUB_PROFILE_URL
  " | Repositories: " HEXOID_GITHUB_REPOSITORIES_URL
  " | Project: " HEXOID_REPOSITORY_URL
  " | YouTube: " HEXOID_YOUTUBE_URL
  " | Blog: " HEXOID_BLOG_URL
  " | " HEXOID_COPYRIGHT
  " | Licence: " HEXOID_LICENCE_NAME;
// setup() stores this pointer once so linker section garbage collection
// cannot discard the attribution string from the compiled firmware.
volatile const char *hexoidIdentityAnchor;
#define CAL_PPM 0.0  // positive if the counter reference runs fast
// The display ceiling is calculated from the verified TIM2 clock in setup:
// designMaxHz = 8*(TIM2CLK/4) (RM0008 rule) and
// displayMaxHz = 8*(TIM2CLK/3) (two-flop resynchronizer rule).
#define GATE_MS 1000u
#define LOW_TIMEOUT_MS 3500u
#define EDGE_PROBE_MS 1250u  // >1 s: guarantees a rising edge for a >=1 Hz input
#define COUNTER_CLOCK_MHZ 112  // 112: proven high-VHF margin; 96: comparison clock; 72: normal
#if (COUNTER_CLOCK_MHZ != 72) && (COUNTER_CLOCK_MHZ != 96) && \
    (COUNTER_CLOCK_MHZ != 112)
#error "COUNTER_CLOCK_MHZ must be 72, 96 or 112"
#endif
#define COUNTER_HIGH_CLOCK (COUNTER_CLOCK_MHZ != 72)
#define HIGH_RETRY_WINDOW_MS 12000u
#define HIGH_GATE_MS 250u
#define HIGH_TRACK_GATE_MS 100u
#define HIGH_TRACK_SLICE_US 25000u
#define TI_CHECK_MAX_HZ 40000000.0
#define TI_CORRECT_MAX_HZ 13000000.0
#define SSD1306_WHITE        1
#define SSD1306_SWITCHCAPVCC 0

/* classic 5x7 font, ASCII 0x20..0x7E, 5 bytes per glyph (column 0 first,
   bit 0 = top row). Same data Adafruit_GFX ships in glcdfont.c.          */
static const uint8_t OLED_FONT5X7[95][5] = {
  {0x00, 0x00, 0x00, 0x00, 0x00},  {0x00, 0x00, 0x5F, 0x00, 0x00},  {0x00, 0x07, 0x00, 0x07, 0x00},  {0x14, 0x7F, 0x14, 0x7F, 0x14},  {0x24, 0x2A, 0x7F, 0x2A, 0x12},  {0x23, 0x13, 0x08, 0x64, 0x62},  {0x36, 0x49, 0x56, 0x20, 0x50},  {0x00, 0x08, 0x07, 0x03, 0x00},
  {0x00, 0x1C, 0x22, 0x41, 0x00},  {0x00, 0x41, 0x22, 0x1C, 0x00},  {0x2A, 0x1C, 0x7F, 0x1C, 0x2A},  {0x08, 0x08, 0x3E, 0x08, 0x08},  {0x00, 0x80, 0x70, 0x30, 0x00},  {0x08, 0x08, 0x08, 0x08, 0x08},  {0x00, 0x00, 0x60, 0x60, 0x00},  {0x20, 0x10, 0x08, 0x04, 0x02},
  {0x3E, 0x51, 0x49, 0x45, 0x3E},  {0x00, 0x42, 0x7F, 0x40, 0x00},  {0x72, 0x49, 0x49, 0x49, 0x46},  {0x21, 0x41, 0x49, 0x4D, 0x33},  {0x18, 0x14, 0x12, 0x7F, 0x10},  {0x27, 0x45, 0x45, 0x45, 0x39},  {0x3C, 0x4A, 0x49, 0x49, 0x31},  {0x41, 0x21, 0x11, 0x09, 0x07},
  {0x36, 0x49, 0x49, 0x49, 0x36},  {0x46, 0x49, 0x49, 0x29, 0x1E},  {0x00, 0x00, 0x14, 0x00, 0x00},  {0x00, 0x40, 0x34, 0x00, 0x00},  {0x00, 0x08, 0x14, 0x22, 0x41},  {0x14, 0x14, 0x14, 0x14, 0x14},  {0x00, 0x41, 0x22, 0x14, 0x08},  {0x02, 0x01, 0x59, 0x09, 0x06},
  {0x3E, 0x41, 0x5D, 0x59, 0x4E},  {0x7C, 0x12, 0x11, 0x12, 0x7C},  {0x7F, 0x49, 0x49, 0x49, 0x36},  {0x3E, 0x41, 0x41, 0x41, 0x22},  {0x7F, 0x41, 0x41, 0x41, 0x3E},  {0x7F, 0x49, 0x49, 0x49, 0x41},  {0x7F, 0x09, 0x09, 0x09, 0x01},  {0x3E, 0x41, 0x41, 0x51, 0x73},
  {0x7F, 0x08, 0x08, 0x08, 0x7F},  {0x00, 0x41, 0x7F, 0x41, 0x00},  {0x20, 0x40, 0x41, 0x3F, 0x01},  {0x7F, 0x08, 0x14, 0x22, 0x41},  {0x7F, 0x40, 0x40, 0x40, 0x40},  {0x7F, 0x02, 0x1C, 0x02, 0x7F},  {0x7F, 0x04, 0x08, 0x10, 0x7F},  {0x3E, 0x41, 0x41, 0x41, 0x3E},
  {0x7F, 0x09, 0x09, 0x09, 0x06},  {0x3E, 0x41, 0x51, 0x21, 0x5E},  {0x7F, 0x09, 0x19, 0x29, 0x46},  {0x26, 0x49, 0x49, 0x49, 0x32},  {0x03, 0x01, 0x7F, 0x01, 0x03},  {0x3F, 0x40, 0x40, 0x40, 0x3F},  {0x1F, 0x20, 0x40, 0x20, 0x1F},  {0x3F, 0x40, 0x38, 0x40, 0x3F},
  {0x63, 0x14, 0x08, 0x14, 0x63},  {0x03, 0x04, 0x78, 0x04, 0x03},  {0x61, 0x59, 0x49, 0x4D, 0x43},  {0x00, 0x7F, 0x41, 0x41, 0x41},  {0x02, 0x04, 0x08, 0x10, 0x20},  {0x00, 0x41, 0x41, 0x41, 0x7F},  {0x04, 0x02, 0x01, 0x02, 0x04},  {0x40, 0x40, 0x40, 0x40, 0x40},
  {0x00, 0x03, 0x07, 0x08, 0x00},  {0x20, 0x54, 0x54, 0x78, 0x40},  {0x7F, 0x28, 0x44, 0x44, 0x38},  {0x38, 0x44, 0x44, 0x44, 0x28},  {0x38, 0x44, 0x44, 0x28, 0x7F},  {0x38, 0x54, 0x54, 0x54, 0x18},  {0x00, 0x08, 0x7E, 0x09, 0x02},  {0x18, 0xA4, 0xA4, 0x9C, 0x78},
  {0x7F, 0x08, 0x04, 0x04, 0x78},  {0x00, 0x44, 0x7D, 0x40, 0x00},  {0x20, 0x40, 0x40, 0x3D, 0x00},  {0x7F, 0x10, 0x28, 0x44, 0x00},  {0x00, 0x41, 0x7F, 0x40, 0x00},  {0x7C, 0x04, 0x78, 0x04, 0x78},  {0x7C, 0x08, 0x04, 0x04, 0x78},  {0x38, 0x44, 0x44, 0x44, 0x38},
  {0xFC, 0x18, 0x24, 0x24, 0x18},  {0x18, 0x24, 0x24, 0x18, 0xFC},  {0x7C, 0x08, 0x04, 0x04, 0x08},  {0x48, 0x54, 0x54, 0x54, 0x24},  {0x04, 0x04, 0x3F, 0x44, 0x24},  {0x3C, 0x40, 0x40, 0x20, 0x7C},  {0x1C, 0x20, 0x40, 0x20, 0x1C},  {0x3C, 0x40, 0x30, 0x40, 0x3C},
  {0x44, 0x28, 0x10, 0x28, 0x44},  {0x4C, 0x90, 0x90, 0x90, 0x7C},  {0x44, 0x64, 0x54, 0x4C, 0x44},  {0x00, 0x08, 0x36, 0x41, 0x00},  {0x00, 0x00, 0x77, 0x00, 0x00},  {0x00, 0x41, 0x36, 0x08, 0x00},  {0x02, 0x01, 0x02, 0x04, 0x02},
};

class OledLite {
  public:
    OledLite() : addr(0x3C), cx(0), cy(0), size(1), color(1) {}

    void setAddress(uint8_t i2cAddr) { addr = i2cAddr; }

    /* Send only the controller configuration.  Recovery uses this without
       clearing the framebuffer, so the exact frame that failed can be
       retransmitted from page 0 instead of leaving old/new pages mixed. */
    bool initController(void)
    {
      static const uint8_t init[] = {
        0xAE,             /* display off                                       */
        0xD5, 0x80,       /* clock divide / suggested ratio                    */
        0xA8, 0x3F,       /* multiplex 1/64                                    */
        0xD3, 0x00,       /* display offset 0                                  */
        0x40,             /* start line 0                                      */
        0x8D, 0x14,       /* charge pump on                                    */
        0x20, 0x02,       /* memory mode: page addressing                           */
        0xA1,             /* segment remap: column 127 -> SEG0                 */
        0xC8,             /* COM scan direction remapped                       */
        0xDA, 0x12,       /* COM pins: alternative, no left/right remap        */
        0x81, 0xCF,       /* contrast                                          */
        0xD9, 0xF1,       /* pre-charge period                                 */
        0xDB, 0x40,       /* VCOMH                                             */
        0xA4,             /* resume RAM content; never force all pixels on                                */
        0xA6,             /* normal (not inverted)                             */
        0x2E,             /* scrolling off                                     */
        0xAF              /* display on                                        */
      };
      return writeCommand(init, sizeof(init));
    }

    bool begin(uint8_t i2cAddr)
    {
      setAddress(i2cAddr);
      if (!initController()) return false;
      clearDisplay();
      return display();
    }

    void clearDisplay(void) { memset(buf, 0, sizeof(buf)); }

    void setTextColor(uint16_t c) { color = (c != 0) ? 1u : 0u; }
    void setTextSize(uint8_t s)   { size = (s < 1) ? 1 : s; }
    void setCursor(int16_t x, int16_t y) { cx = x; cy = y; }

    /* the sketch asks for the bounding box; a glyph cell is 6x8 pixels */
    void getTextBounds(const char *s, int16_t x, int16_t y,
                       int16_t *bx, int16_t *by, uint16_t *bw, uint16_t *bh)
    {
      (void)x; (void)y;
      if (bx) *bx = 0;
      if (by) *by = 0;
      if (bw) *bw = (uint16_t)(strlen(s) * 6u * size);
      if (bh) *bh = (uint16_t)(8u * size);
    }

    size_t print(const char *s)
    {
      while (*s) writeChar(*s++);
      return strlen(s);
    }

    void drawPixel(int16_t x, int16_t y, uint16_t c)
    {
      if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
      uint8_t *p = &buf[(y >> 3) * 128 + x];
      if (c != 0) *p |=  (uint8_t)(1u << (y & 7));
      else        *p &= (uint8_t)~(1u << (y & 7));
    }

    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c)
    {
      for (int16_t i = 0; i < w; i++) drawPixel((int16_t)(x + i), y, c);
    }

    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
    {
      drawFastHLine(x, y, w, c);
      drawFastHLine(x, (int16_t)(y + h - 1), w, c);
      for (int16_t i = 0; i < h; i++) {
        drawPixel(x, (int16_t)(y + i), c);
        drawPixel((int16_t)(x + w - 1), (int16_t)(y + i), c);
      }
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c)
    {
      for (int16_t j = 0; j < h; j++) drawFastHLine(x, (int16_t)(y + j), w, c);
    }

    /* Push one complete framebuffer pass.  The three page-address bytes
       are one I2C transaction (not three separately corruptible command
       transactions); data uses 16-byte chunks that fit every Wire buffer.
       A caller-level retry always restarts at page 0 after recovery. */
    bool display(void)
    {
      for (uint8_t page = 0; page < 8; page++) {
        const uint8_t pageCommand[3] = {
          (uint8_t)(0xB0u | page), 0x00u, 0x10u
        };
        if (!writeCommand(pageCommand, sizeof(pageCommand))) return false;
        for (uint8_t col = 0; col < 128; col += 16) {
          Wire.beginTransmission(addr);
          Wire.write(0x40);
          for (uint8_t k = 0; k < 16; k++) Wire.write(buf[page * 128u + col + k]);
          if (Wire.endTransmission() != 0) return false;
        }
      }
      return true;
    }

  private:
    uint8_t  addr, cx, cy, size, color;
    uint8_t  buf[128 * 8];              /* 1024 B, one bit per pixel        */

    bool writeCommand(const uint8_t *c, uint8_t n)
    {
      Wire.beginTransmission(addr);
      Wire.write(0x00);                 /* Co = 0, D/C = 0: command stream  */
      for (uint8_t i = 0; i < n; i++) Wire.write(c[i]);
      return (Wire.endTransmission() == 0);
    }

    bool writeCommand1(uint8_t c) { return writeCommand(&c, 1); }

    void writeChar(char ch)
    {
      if (ch < 0x20 || ch > 0x7E) ch = '?';
      const uint8_t *g = OLED_FONT5X7[ch - 0x20];
      for (uint8_t col = 0; col < 6; col++) {          /* 5 columns + gap   */
        const uint8_t bits = (col < 5) ? g[col] : 0u;
        for (uint8_t row = 0; row < 8; row++) {
          const bool on = (bits >> row) & 1u;
          if (!on && color == 0) continue;             /* transparent bg    */
          for (uint8_t dx = 0; dx < size; dx++)
            for (uint8_t dy = 0; dy < size; dy++)
              drawPixel((int16_t)(cx + col * size + dx),
                        (int16_t)(cy + row * size + dy),
                        on ? SSD1306_WHITE : 0);
        }
      }
      cx = (int16_t)(cx + 6 * size);
    }
};

static OledLite oled;   /* section 3b: our own driver, no library */

static bool screenOK = false;
static bool screenFound = false;
static uint8_t screenPins = 0;
static uint8_t screenAddress = 0x3C;
static bool screenFastI2C = false;
#if COUNTER_HIGH_CLOCK
static bool highClockOK = false;
#endif
static uint32_t actualClockMHz = 72;
static uint32_t timerHz;
static double designMaxHz = 144000000.0;   // 2*TIM2CLK, set in setup()
static double displayMaxHz = 192000000.0;  // 8*TIM2CLK/3, set in setup()
static bool registersOK, gateValid;
static bool inputChanging=false;
static uint32_t countsLast, gapLast;
static bool highRangeMemory = false;
static uint32_t highRangeLastMs = 0;
static char measureLabel[24] = "ETR/8 GATE";
static char etrGateLabel[24] = "ETR/8 1s GATE";
static void setLabel(const char *text) { snprintf(measureLabel,sizeof(measureLabel),"%s",text); }
static void setEtrGateLabel(const char *text) {
  snprintf(etrGateLabel,sizeof(etrGateLabel),"%s",text);
  setLabel(etrGateLabel);
}

static void line(uint8_t y, const char *text, uint8_t sz=1) {
  oled.setTextSize(sz); oled.setTextColor(1);
  int x = (128 - (int)strlen(text)*6*sz)/2;
  oled.setCursor(x > 0 ? x : 0, y); oled.print(text);
}
static bool wireBegun = false;

static void beginScreenWire(uint8_t pins, uint32_t clockHz) {
  if (wireBegun) Wire.end();
  Wire.setSDA(pins ? PB9 : PB7); Wire.setSCL(pins ? PB8 : PB6);
  Wire.begin(); wireBegun=true; Wire.setClock(clockHz);
}

static bool initScreen() {
  screenFastI2C=false;
  for (uint8_t pins=0; pins<2; ++pins) {
    beginScreenWire(pins,100000u);
    for (uint8_t address=0x3C; address<=0x3D; ++address) {
      Wire.beginTransmission(address);
      if (Wire.endTransmission()==0 && oled.begin(address)) {
        screenPins=pins; screenAddress=address; screenFound=true;
        // The panel was proven at 100 kHz during probing.  Use the
        // SSD1306's 400 kHz I2C rate for the much longer frame transfers
        // to shorten the RF exposure window; recovery drops back to
        // 100 kHz permanently if a 400 kHz frame is ever NACKed.
        Wire.setClock(400000u); screenFastI2C=true;
        return true;
      }
    }
  }
  screenFound=false;
  return false;
}

/* Recover the already-discovered OLED without probing the wrong pin pair
 * and without clearing oled's framebuffer.  The caller then retries the
 * identical complete frame from page 0. */
static bool recoverScreen() {
  if (!screenFound) return initScreen();
  screenFastI2C=false;  // any failed 400 kHz frame permanently selects 100 kHz
  beginScreenWire(screenPins,100000u);
  oled.setAddress(screenAddress);
  delay(2);
  return oled.initController();
}

static bool ensureScreen() {
  if (screenOK) return true;
  screenOK = screenFound ? recoverScreen() : initScreen();
  return screenOK;
}

/* An earlier display path stopped at the first failed I2C transaction, leaving pages from
 * two different screens visible together, and the next show() returned
 * after re-initialization without drawing its own frame.  This design therefore
 * treats a frame as atomic: recover the known bus/controller and retry
 * the same framebuffer from page 0, up to three complete passes. */
static bool pushScreen() {
  for (uint8_t attempt=0; attempt<3u; ++attempt) {
    if (oled.display()) { screenOK=true; return true; }
    screenOK=false;
    if (attempt==2u || !recoverScreen()) break;
  }
  screenOK=false;
  return false;
}

// Re-clock among HSE x9 (72 MHz), x12 (96 MHz) and x14 (112 MHz).  This
// is deliberately bounded and called before Wire/OLED or any measurement
// starts.  TIM2 is on APB1: PPRE1=/2 gives TIM2CLK=HCLK (PCLK1 is 36,
// 48 or 56 MHz respectively).  In the two experimental modes PPRE2=/2
// also bounds the APB2/GPIO domain to 48 or 56 MHz instead of leaving it
// at 96/112 MHz.  Those frequencies remain outside the F103 specification;
// setup() reports the verified clock and falls back x14 -> x12 -> x9 if a
// requested PLL switch cannot be verified.
#if COUNTER_HIGH_CLOCK
static bool waitClockBits(volatile uint32_t *reg, uint32_t mask, uint32_t want) {
  for (volatile uint32_t n=0;n<4000000u;++n) {
    if ((*reg & mask)==want) return true;
  }
  return false;
}
static bool selectPllMultiplier(uint32_t mul) {
  if (mul!=9u && mul!=12u && mul!=14u) return false;
  if ((RCC->CR & (1u<<17))==0u) {              // HSERDY
    RCC->CR |= (1u<<16);                       // HSEON
    if (!waitClockBits(&RCC->CR,(1u<<17),(1u<<17))) return false;
  }
  FLASH->ACR=(FLASH->ACR & ~7u) | 2u;           // maximum encoded latency: 2 WS
  uint32_t cfgr=RCC->CFGR;
  cfgr &= ~((15u<<4) | (7u<<8) | (7u<<11) | (3u<<14) | (1u<<22) | (7u<<24));
  cfgr |= (4u<<8);                              // PPRE1=/2: TIM2CLK=HCLK
  if (mul!=9u) cfgr |= (4u<<11);                // PPRE2=/2 in x12/x14 modes
  cfgr |= (3u<<14);                             // ADCPRE=/8 (ADC unused)
  // USBPRE remains 0 (PLL/1.5).  That is the correct 48 MHz USB clock at
  // x9 only; this counter never uses USB in the x12/x14 experiments.
  RCC->CFGR=cfgr;

  RCC->CFGR=(RCC->CFGR & ~3u) | 1u;             // SW=HSE
  if (!waitClockBits(&RCC->CFGR,(3u<<2),(1u<<2))) return false;
  RCC->CR &= ~(1u<<24);                         // PLLON=0
  if (!waitClockBits(&RCC->CR,(1u<<25),0u)) return false;

  cfgr=RCC->CFGR;
  cfgr &= ~((1u<<17) | (15u<<18));
  cfgr |= (1u<<16) | ((mul-2u)<<18);            // PLLSRC=HSE, PLLXTPRE=/1
  RCC->CFGR=cfgr;
  RCC->CR |= (1u<<24);                          // PLLON=1
  if (!waitClockBits(&RCC->CR,(1u<<25),(1u<<25))) return false;
  RCC->CFGR=(RCC->CFGR & ~3u) | 2u;             // SW=PLL
  if (!waitClockBits(&RCC->CFGR,(3u<<2),(2u<<2))) return false;

  SystemCoreClockUpdate();
  if (SystemCoreClock!=8000000u*mul) return false;
  const uint32_t systickCtrl=SysTick->CTRL;
  SysTick->CTRL=0;
  SysTick->LOAD=(SystemCoreClock/1000u)-1u;
  SysTick->VAL=0;
  SysTick->CTRL=systickCtrl;
  return true;
}
#endif

// Hardware-tested explicit reset/routing sequence.  PA0 reaches
// TIM2 by both ETR and CH1/TI1.  The TI1 path is synchronized/filtered at
// the TIM2 timer clock BEFORE TIM2->PSC divides it by eight; IC1F=0001
// requires two consecutive timer-clock samples at the new level.  This rejects
// narrow multiplied edges that the asynchronous ETR prescaler accepts.
// Hardware testing proved the unfiltered TI1 register routing (SMCR=0x57).
static uint32_t expectedSMCR(uint8_t divider, bool internal, bool tiFiltered) {
  if (internal) return 0u;
  if (tiFiltered) return 0x57u; // TS=101 TI1FP1, SMS=111 external clock 1
  return divider==8 ? 0x7000u : 0x4000u; // ECE + ETPS /8 or /1
}
static uint32_t expectedCCMR1(uint8_t tiFilter) {
  // CC1S=01 in both cases; IC1F=0001 -> N=2, IC1F=0010 -> N=4.
  return tiFilter==4u ? 0x21u : tiFilter==2u ? 0x11u : 0u;
}

/* Hardware 32-bit extension: TIM2 MMS=010 places its update event on
 * TRGO.  For slave TIM3, ITR1 (TS=001) is TIM2 TRGO; SMS=111 selects
 * external-clock mode 1, so TIM3 increments exactly once per TIM2
 * 65536-count overflow.  TIM3:TIM2 is therefore a 32-bit count and the
 * CPU no longer has to win a 2.73 ms poll-before-wrap race at 192 MHz. */
#define TIM2_CR2_TRGO_UPDATE 0x0020u
#define TIM3_SMCR_FROM_TIM2  0x0017u
static bool countSnapshotOK = true;

static void stopTimer() {
  TIM2->CR1=0;              // stop the low half first: no new overflow
  TIM3->CR1=0;
}

static bool timerChainOK() {
  return (TIM2->CR1 & 1u) && (TIM3->CR1 & 1u) &&
         TIM2->CR2==TIM2_CR2_TRGO_UPDATE &&
         TIM3->SMCR==TIM3_SMCR_FROM_TIM2 &&
         TIM3->PSC==0u && TIM3->ARR==65535u;
}

static uint32_t readCount32() {
  // Two low-word observations with an unchanged high word make the
  // snapshot coherent even while an overflow/TRGO pulse is crossing the
  // TIM3 resynchronizer.  Eight attempts are far more than the one retry
  // an actual overflow can require at these sample intervals.
  for (uint8_t attempt=0; attempt<8u; ++attempt) {
    const uint16_t highA=(uint16_t)TIM3->CNT;
    const uint16_t lowA=(uint16_t)TIM2->CNT;
    const uint16_t highB=(uint16_t)TIM3->CNT;
    if (highA!=highB) continue;
    const uint16_t lowB=(uint16_t)TIM2->CNT;
    const uint16_t highC=(uint16_t)TIM3->CNT;
    if (highC==highA && lowB>=lowA) {
      countSnapshotOK=true;
      return ((uint32_t)highA<<16) | lowB;
    }
  }
  countSnapshotOK=false;
  return 0;
}

static bool startTimer(uint8_t divider, bool internal=false,
                       uint8_t tiFilter=0) {
  const bool tiFiltered=(tiFilter!=0u);
  if (tiFilter!=0u && tiFilter!=2u && tiFilter!=4u) return false;
  RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
  (void)RCC->APB2ENR;
  AFIO->MAPR &= ~(3u << 8);
  GPIOA->BRR=1u;
  GPIOA->CRL=(GPIOA->CRL & ~15u) | 8u;
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN;
  (void)RCC->APB1ENR;
  RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST;
  RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST);
  const uint32_t wantPSC=internal ? 71u : (tiFiltered ? 7u : 0u);
  const uint32_t wantCCMR1=expectedCCMR1(tiFilter);
  const uint32_t wantCCER=tiFiltered ? 1u : 0u;    // CC1E, rising edge
  const uint32_t wantSMCR=expectedSMCR(divider,internal,tiFiltered);
  TIM2->CR1=0; TIM2->CR2=TIM2_CR2_TRGO_UPDATE; TIM2->ARR=65535u;
  TIM2->PSC=wantPSC;
  TIM2->CCER=0;                 // CC1S is writable only with CC1E=0
  TIM2->CCMR1=wantCCMR1; TIM2->CCER=wantCCER; TIM2->DIER=0;
  TIM2->SMCR=wantSMCR;
  TIM3->CR1=0; TIM3->CR2=0; TIM3->ARR=65535u; TIM3->PSC=0;
  TIM3->CCMR1=0; TIM3->CCMR2=0; TIM3->CCER=0; TIM3->DIER=0;
  TIM3->SMCR=TIM3_SMCR_FROM_TIM2;
  // Generate/load both prescalers while both counters are disabled, then
  // clear any update event produced by EGR before enabling TIM3 first.
  TIM2->EGR=1u; TIM2->SR=0; TIM2->CNT=0;
  TIM3->EGR=1u; TIM3->SR=0; TIM3->CNT=0;
  TIM3->CR1=1u;
  TIM2->CR1=1u;
  return timerChainOK() && TIM2->ARR==65535u &&
    TIM2->PSC==wantPSC && TIM2->CCMR1==wantCCMR1 &&
    TIM2->CCER==wantCCER && TIM2->SMCR==wantSMCR &&
    (AFIO->MAPR & (3u<<8))==0u && (GPIOA->CRL & 15u)==8u;
}

// No I2C, LED, serial, or button calls inside a measurement.
// TIM3 hardware-counts every TIM2 overflow, so the gate is sampled only
// about once per millisecond.  A CPU/service delay no longer creates the
// old false POLL GAP failure merely because it approached the 16-bit
// wrap interval.  gateValid now means that every TIM3:TIM2 snapshot was
// coherent and monotonically increasing.
static double countGate(uint8_t divider, uint32_t gateMs, bool internal=false,
                        uint8_t tiFilter=0, uint32_t sliceUs=100000u) {
  const bool tiFiltered=(tiFilter!=0u);
  const uint32_t gateUs=gateMs*1000u;
  if (sliceUs<10000u) sliceUs=10000u;
  inputChanging=false;
  gateValid=true; gapLast=0; countsLast=0; countSnapshotOK=true;
  registersOK=startTimer(divider,internal,tiFilter);
  if (!registersOK) { stopTimer(); gateValid=false; return 0; }
  uint32_t tick=millis(); while (millis()==tick) {}
  const uint32_t startCounts=readCount32();
  uint32_t start=micros(), previousTime=start;
  if (!countSnapshotOK) gateValid=false;
  uint32_t elapsed=0;
  uint32_t sliceStart=start, sliceCounts=0;
  double minRate=1.0e30, maxRate=0.0;
  uint8_t slices=0;
  while (elapsed < gateUs) {
    // Sleep between snapshots; SysTick wakes this once per millisecond.
    // TIM2/TIM3 continue counting in hardware while the CPU is idle.
    uint32_t wakeStamp;
    do {
      __WFI();
      wakeStamp=micros();
    } while ((uint32_t)(wakeStamp-previousTime)<1000u &&
             (uint32_t)(wakeStamp-start)<gateUs);

    const uint32_t totalCounts=readCount32();
    const uint32_t stamp=micros();
    if (!countSnapshotOK || totalCounts<startCounts) gateValid=false;
    const uint32_t gap=stamp-previousTime;
    if (gap>gapLast) gapLast=gap;   // diagnostic only; TIM3 owns wraps
    if (gateValid) countsLast=totalCounts-startCounts;
    previousTime=stamp;
    elapsed=stamp-start;
    // Consecutive subwindows within the SAME uninterrupted gate.
    // A sweep change creates an average, not a settled-frequency reading.
    if ((uint32_t)(stamp-sliceStart)>=sliceUs) {
      double rate=(double)(countsLast-sliceCounts)*divider*1000000.0 /
                  (uint32_t)(stamp-sliceStart);
      if (rate<minRate) minRate=rate;
      if (rate>maxRate) maxRate=rate;
      ++slices; sliceStart=stamp; sliceCounts=countsLast;
    }
  }
  // Include any partial final slice long enough to be meaningful.
  if ((uint32_t)(previousTime-sliceStart)>=10000u) {
    double rate=(double)(countsLast-sliceCounts)*divider*1000000.0 /
                (uint32_t)(previousTime-sliceStart);
    if (rate<minRate) minRate=rate;
    if (rate>maxRate) maxRate=rate;
    ++slices;
  }
  // 0.5% stability tolerance, with 1600 Hz floor for short final
  // windows and /8 quantization. This is NOT calibration or averaging.
  double tolerance=maxRate*0.005;
  if (tolerance<1600.0) tolerance=1600.0;
  if (!internal && slices>=2 && maxRate-minRate>tolerance)
    inputChanging=true;
  registersOK=registersOK && timerChainOK() &&
    TIM2->SMCR==expectedSMCR(divider,internal,tiFiltered) &&
    TIM2->PSC==(internal ? 71u : (tiFiltered ? 7u : 0u)) &&
    TIM2->CCMR1==expectedCCMR1(tiFilter) &&
    TIM2->CCER==(tiFiltered ? 1u : 0u);
  stopTimer();
  if (!registersOK || !gateValid || !elapsed) return 0;
  return (double)countsLast * divider * 1000000.0 / elapsed;
}

// Presence probe only: it answers "did at least one real ETR edge occur?"
// It deliberately makes NO display call.  An earlier state machine printed MEASURING before
// knowing this, so a disconnected PA0 spent most of every cycle falsely
// labelled MEASURING while lowRate() timed out.  1250 ms is longer than one
// period at the counter's 1 Hz lower limit, so a continuous >=1 Hz signal
// cannot be missed merely because of its starting phase.
static bool probeEdge() {
  inputChanging=false;
  gateValid=true;
  registersOK=startTimer(1);
  if (!registersOK) { stopTimer(); return false; }
  const uint16_t firstCnt=TIM2->CNT;
  const uint32_t beginMs=millis();
  bool seen=false;
  while ((uint32_t)(millis()-beginMs)<EDGE_PROBE_MS) {
    if ((uint16_t)(TIM2->CNT-firstCnt)!=0u) { seen=true; break; }
  }
  registersOK=registersOK && timerChainOK() && TIM2->SMCR==0x4000u;
  stopTimer();
  return seen;
}

// Low-frequency reciprocal timing using TIM2 hardware edge counts, not
// sampled GPIO levels. Nothing is displayed until at least two rising
// edges have been seen. A quiet input never produces a frequency.
static double lowRate() {
  inputChanging=false;
  registersOK=startTimer(1);
  gateValid=true; countsLast=0;
  if (!registersOK) { stopTimer(); return 0; }
  uint16_t last=TIM2->CNT;
  uint32_t beginMs=millis(), first=0, accumulated=0, lastPoll=micros();
  uint32_t previousEdge=0, minPeriod=0xFFFFFFFFu, maxPeriod=0;
  bool started=false;
  double result=0;
  while ((uint32_t)(millis()-beginMs)<LOW_TIMEOUT_MS) {
    uint16_t now=TIM2->CNT;
    uint32_t stamp=micros();
    uint32_t gap=stamp-lastPoll; lastPoll=stamp;
    if (gap>=2000u) { gateValid=false; break; }
    uint16_t delta=(uint16_t)(now-last); last=now;
    if (!delta) continue;
    // If the source jumped to a high frequency during the low-rate
    // window, abort; the next loop re-tests ETR/8 before retrying here.
    if (delta>1u) { inputChanging=true; break; }
    if (!started) { started=true; first=stamp; previousEdge=stamp; continue; }
    uint32_t period=stamp-previousEdge; previousEdge=stamp;
    if (period<minPeriod) minPeriod=period;
    if (period>maxPeriod) maxPeriod=period;
    uint32_t tolerance=maxPeriod/50u; // 2%, minimum 10 us polling allowance
    if (tolerance<10u) tolerance=10u;
    if (maxPeriod-minPeriod>tolerance) { inputChanging=true; break; }
    accumulated += delta;
    uint32_t elapsed=stamp-first;
    if (elapsed>=200000u && accumulated>=2u) {
      result=(double)accumulated*1000000.0/elapsed;
      countsLast=accumulated;
      if (result>2000.0) { inputChanging=true; result=0; }
      break;
    }
  }
  registersOK=registersOK && timerChainOK() && TIM2->SMCR==0x4000u;
  stopTimer();
  return registersOK && gateValid && !inputChanging ? result : 0;
}

// Integer-only decimal output: does not require newlib-nano _printf_float.
// Called with a positive finite input <= displayMaxHz (<1 GHz for every
// supported clock), so the scaled MHz integer remains far inside uint32_t.
static const char *formatReading(double hz, char *value, size_t length) {
  uint32_t divisor = hz >= 1000000.0 ? 1000000u : hz >= 1000.0 ? 1000u : 1u;
  uint32_t scaled = (uint32_t)(hz * (10000.0 / divisor) + 0.5);
  // Promote if rounding crosses a unit boundary (999.99995 -> 1 kHz).
  if (scaled >= 10000000u && divisor < 1000000u) {
    divisor *= 1000u;
    scaled = (uint32_t)(hz * (10000.0 / divisor) + 0.5);
  }
  snprintf(value, length, "%lu.%04lu",
           (unsigned long)(scaled / 10000u),
           (unsigned long)(scaled % 10000u));
  return divisor == 1000000u ? "MHz" : divisor == 1000u ? "kHz" : "Hz";
}

static void show(double hz, const char *fault=nullptr) {
  if (!ensureScreen()) return;
  oled.clearDisplay();
  line(0,HEXOID_OLED_BRAND); oled.drawFastHLine(0,10,128,1);
  char value[24], details[24];
  if (fault) {
    line(18,"CHECK",2); line(39,fault);
  } else if (!(hz>0)) {
    line(18,"NO SIGNAL",2); line(39,"NO VALID EDGE RATE");
  } else if (hz>displayMaxHz) {
    line(18,"OVER RANGE",2);
    snprintf(value,sizeof(value),">%luMHZ SYNC LIMIT",
             (unsigned long)(displayMaxHz/1000000.0));
    line(36,value);
    snprintf(details,sizeof(details),"N=%lu",(unsigned long)countsLast);
    line(46,details);
  } else {
    const char *unit = formatReading(hz, value, sizeof(value));
    // Above designMaxHz the count is displayed, not hidden, but EXP marks
    // that ETRP is between TIM2CLK/4 and TIM2CLK/3 on this board.
    line(15,value,2); line(33,hz>designMaxHz ? "MHz EXP" : unit);
    snprintf(details,sizeof(details),"N=%lu",(unsigned long)countsLast);
    line(44,details);
  }
  line(55,measureLabel);
  (void)pushScreen();
}

void setup() {
  hexoidIdentityAnchor=HEXOID_FIRMWARE_IDENTITY;  // retain identity in firmware image
  pinMode(PC13,OUTPUT); digitalWrite(PC13,HIGH);
#if COUNTER_HIGH_CLOCK
#if COUNTER_CLOCK_MHZ == 112
  highClockOK=selectPllMultiplier(14u);
  if (!highClockOK) highClockOK=selectPllMultiplier(12u);
#else
  highClockOK=selectPllMultiplier(12u);
#endif
  if (!highClockOK) (void)selectPllMultiplier(9u); // verified/normal fallback path
#endif
  SystemCoreClockUpdate();
  actualClockMHz=SystemCoreClock/1000000u;
  const uint8_t divs[]={1,1,1,1,2,4,8,16};
  uint32_t apb=(RCC->CFGR>>8)&7u;
  timerHz=SystemCoreClock/divs[apb]; if (apb>=4u) timerHz*=2u;
  // ETR /8 feeds ETRP to the two-flop resynchronizer.  RM0008's design
  // ceiling is ETRP <= TIM2CLK/4; ST AN4776's resynchronizer relationship
  // is TIM2CLK >= 3*ETRP.  At 112 MHz: 224 MHz design, 298.667 MHz sync.
  designMaxHz=2.0*(double)timerHz;
  displayMaxHz=8.0*(double)timerHz/3.0;
  screenOK=initScreen();
  // Self-test is labelled, never submitted as an external reading.  The
  // 70 ms internal gate intentionally exceeds 65,536 TIM2 counts at every
  // supported clock (70k at 72 MHz, 109k at 112 MHz), so SELF PASS also
  // proves that TIM3 counted a real TIM2 TRGO overflow.
  double internal=countGate(1,70,true);
  bool ok=registersOK && gateValid && internal>timerHz/72.0*0.95 &&
          internal<timerHz/72.0*1.05;
  if (screenOK) {
    char clockLine[24];
    if (actualClockMHz==72u) snprintf(clockLine,sizeof(clockLine),"SYSCLK 72MHZ");
    else if (actualClockMHz==96u || actualClockMHz==112u)
      snprintf(clockLine,sizeof(clockLine),"SYSCLK %luMHZ EXP",
               (unsigned long)actualClockMHz);
    else snprintf(clockLine,sizeof(clockLine),"SYSCLK %luMHZ CHECK",
                  (unsigned long)actualClockMHz);
    char rangeLine[24];
    snprintf(rangeLine,sizeof(rangeLine),"MAX %luMHZ EXP",
             (unsigned long)(displayMaxHz/1000000.0));
    oled.clearDisplay();
    // The 128-pixel line cannot contain the complete lockup at the
    // built-in 6-pixel font pitch, so the exact powered-by wording is
    // split across two centred lines without changing its text.
    line(0,HEXOID_OLED_POWERED_1);
    line(9,HEXOID_OLED_POWERED_2);
    oled.drawFastHLine(14,18,100,1);
    line(21,ok ? "SELF PASS" : "SELF FAIL",2);
    line(39,clockLine); line(47,rangeLine);
    line(56,"BY mikey-7x"); (void)pushScreen();
  }
  delay(1500);
  stopTimer();
}

// Acquisition is NOT evidence of absence; show it without a stale number.
static void showStatus(const char *headline, const char *detail) {
  if (!ensureScreen()) return;
  oled.clearDisplay(); line(0,HEXOID_OLED_BRAND);
  oled.drawFastHLine(0,10,128,1);
  line(20,headline); line(38,detail); line(55,measureLabel);
  (void)pushScreen();
}

static void noteHighRange(double hz) {
  if (hz>TI_CHECK_MAX_HZ) {
    highRangeMemory=true;
    highRangeLastMs=millis();
  }
}

static bool highMemoryActive() {
  if (!highRangeMemory) return false;
  if ((uint32_t)(millis()-highRangeLastMs)<=HIGH_RETRY_WINDOW_MS) return true;
  highRangeMemory=false;
  return false;
}

/* A marginal VHF gate can collapse toward zero when the ETR resynchronizer
 * starts dropping cycles.  In an earlier state machine that false <2 kHz result entered the
 * ETR/1 low-rate path, producing WAITING FOR EDGES and then NO SIGNAL even
 * though the immediately preceding qualified measurement was >40 MHz.
 * While that recent high-range evidence is fresh, stay on ETR/8.
 * This design first tries six 100 ms tracking gates, each stability-checked in
 * four 25 ms slices.  At 192 MHz this still contains 2,400,000 prescaled
 * counts (80 Hz/count), so it is a real qualified gate, not a sweep
 * average.  Two 250 ms gates (32 Hz/count) follow as the precision path.
 * countGate() returns the gate average even when it sets inputChanging;
 * that unqualified average may refresh the 12 s high-range memory while
 * the oscillator is moving, but every caller below rejects it for display
 * unless inputChanging is false. */
static bool reacquireHigh(double &outHz) {
  outHz=0;
  for (uint8_t attempt=0; attempt<6u; ++attempt) {
    setEtrGateLabel("ETR/8 100ms TRACK");
    const double retryHz=countGate(8,HIGH_TRACK_GATE_MS,false,0,
                                   HIGH_TRACK_SLICE_US);
    if (!registersOK) return false;
    if (gateValid && retryHz>TI_CHECK_MAX_HZ) noteHighRange(retryHz);
    if (gateValid && !inputChanging && retryHz>TI_CHECK_MAX_HZ) {
      outHz=retryHz;
      return true;
    }
  }
  for (uint8_t attempt=0; attempt<2u; ++attempt) {
    setEtrGateLabel("ETR/8 250ms GATE");
    const double retryHz=countGate(8,HIGH_GATE_MS);
    if (!registersOK) return false;
    if (gateValid && retryHz>TI_CHECK_MAX_HZ) noteHighRange(retryHz);
    if (gateValid && !inputChanging && retryHz>=2000.0) {
      outHz=retryHz;
      return true;
    }
  }
  return false;
}

void loop() {
  setEtrGateLabel("ETR/8 1s GATE");
  double hz=countGate(8,GATE_MS);
  if (!registersOK) { show(0,"TIMER CONFIG FAILED"); delay(300); return; }

  // High-range hysteresis is deliberately placed before the low-rate
  // branch.  A qualified >40 MHz result (or a changing 1 s average that
  // still contains >40 MHz worth of real ETR counts) proves that PA0 is
  // not a <2 kHz input.  A subsequent collapsed/changing/sync-lost gate
  // is therefore reacquired on ETR/8, never by ETR/1 edge timing.
  const bool primaryChanging=inputChanging;
  if (!gateValid || primaryChanging || hz<2000.0) {
    if (gateValid && hz>TI_CHECK_MAX_HZ) noteHighRange(hz);
    if (highMemoryActive()) {
      double retryHz=0;
      if (reacquireHigh(retryHz)) {
        hz=retryHz;
        // A stable 2 kHz..40 MHz 250 ms gate is a genuine down-range
        // transition: release memory and let the normal TI1F cross-check
        // below judge it.  Above 40 MHz the label remains 100ms TRACK or
        // 250ms GATE, exactly as set by the gate that qualified.
        if (hz<=TI_CHECK_MAX_HZ) highRangeMemory=false;
      } else {
        if (!registersOK) {
          show(0,"TIMER CONFIG FAILED"); delay(300); return;
        }
        setLabel("ETR/8 HIGH RANGE");
        if (primaryChanging) showStatus("INPUT CHANGING", "WAIT FOR STABLE RATE");
        else showStatus("HIGH SIGNAL LOST", "ETR/8 REACQUIRING");
        return;
      }
    } else {
      if (!gateValid) { show(0,"COUNT SYNC LOST"); delay(300); return; }
      if (primaryChanging) {
        showStatus("INPUT CHANGING", "WAIT FOR STABLE RATE"); return;
      }
    }
  }

  if (hz>=2000.0 && hz<=TI_CHECK_MAX_HZ) {
    // Snapshot ETR before the second TIM2 configuration overwrites state.
    const double etrHz=hz;
    const uint32_t etrCounts=countsLast;

    // 250 ms through PA0's independent CH1/TI1 synchronizer + IC1F N=2
    // filter, then TIM2 PSC /8.  The ETR asynchronous prescaler and this
    // synchronous filtered path must agree on a clean periodic signal.
    double tiHz=countGate(8,250,false,2);
    const bool tiRegistersOK=registersOK, tiGateValid=gateValid;
    const bool tiChanging=inputChanging;
    const uint32_t tiCounts=countsLast;
    if (!tiRegistersOK) { setLabel(etrGateLabel); show(0,"TIMER CONFIG FAILED"); delay(300); return; }
    if (!tiGateValid && etrHz<=TI_CORRECT_MAX_HZ) { setLabel(etrGateLabel); show(0,"COUNT SYNC LOST"); delay(300); return; }
    if (!tiGateValid || tiChanging) {
      if (etrHz<=TI_CORRECT_MAX_HZ) {
        // TI1F is in its reliable range here, so instability is meaningful.
        setLabel("ETR/8 + TI1F2"); showStatus("INPUT CHANGING", "WAIT FOR STABLE RATE"); return;
      }
      // Above 13 MHz the 2-sample TI1 path can alias or go blind (notably
      // at 36 MHz).  It is a correction check only; it must never veto a
      // stable completed ETR gate in that range.
      hz=etrHz; countsLast=etrCounts; setLabel(etrGateLabel);
    } else {
    double difference=etrHz>tiHz ? etrHz-tiHz : tiHz-etrHz;
    // 1% plus the prescaled-count quantization of the 1 s and 250 ms
    // gates (8 Hz and 32 Hz per count).  The floor prevents a clean
    // few-kHz signal from being rejected solely by count granularity.
    double agreementTol=etrHz*0.01+48.0;
    if (tiHz>0.0 && difference<=agreementTol) {
      // Clean agreement: retain the completed 1-second ETR measurement.
      hz=etrHz; countsLast=etrCounts;
      setLabel("ETR/8 TI1F2 OK");
    } else {
      uint8_t multiple=0;
      if (tiHz>=2000.0 && tiHz<=TI_CORRECT_MAX_HZ) {
        for (uint8_t k=2;k<=4;++k) {
          double expected=(double)k*tiHz;
          double d=etrHz>expected ? etrHz-expected : expected-etrHz;
          if (d<=etrHz*0.01+48.0) { multiple=k; break; }
        }
      }
      if (multiple) {
        // An exact x2/x3/x4 relationship is not, by itself, proof that
        // tiHz is the fundamental when the ETR result is above 13 MHz.
        // The sampled N=2 filter aliases clean high waveforms (for
        // example 36->12 MHz and 40->8 MHz on a 96 MHz grid; 40->8 MHz
        // on the 112 MHz grid).  Require a 250 ms N=4 gate to
        // reproduce tiHz before correcting
        // an above-13-MHz ETR result.  A clean 36/40 MHz half-cycle is far
        // too short to pass four consecutive samples, while a genuine
        // 8-12 MHz fundamental can.  If N=4 does not confirm it, the N=2
        // result is an alias/unproven and the stable ETR gate stands.
        bool fundamentalConfirmed=(etrHz<=TI_CORRECT_MAX_HZ);
        if (!fundamentalConfirmed) {
          const double antiAliasHz=countGate(8,250,false,4);
          if (!registersOK) { show(0,"TIMER CONFIG FAILED"); delay(300); return; }
          const double antiAliasDiff=antiAliasHz>tiHz ?
            antiAliasHz-tiHz : tiHz-antiAliasHz;
          fundamentalConfirmed=gateValid && !inputChanging &&
            antiAliasHz>0.0 && antiAliasDiff<=tiHz*0.01+48.0;
        }
        if (!fundamentalConfirmed) {
          hz=etrHz; countsLast=etrCounts; setLabel(etrGateLabel);
        } else {
          // Do not just divide the ETR result: acquire a fresh full-second
          // TI1F gate and require it to confirm the fundamental within 1%.
          char label[24];
          snprintf(label,sizeof(label),"TI1F2 ETRx%u",(unsigned)multiple);
          setLabel(label);
          double fullTiHz=countGate(8,GATE_MS,false,2);
          if (!registersOK) { show(0,"TIMER CONFIG FAILED"); delay(300); return; }
          if (!gateValid) { show(0,"COUNT SYNC LOST"); delay(300); return; }
          if (inputChanging) { showStatus("INPUT CHANGING", "WAIT FOR STABLE RATE"); return; }
          double confirmDiff=fullTiHz>tiHz ? fullTiHz-tiHz : tiHz-fullTiHz;
          if (!(fullTiHz>0.0) || confirmDiff>tiHz*0.01+48.0) {
            showStatus("INPUT CHANGING", "WAIT FOR STABLE RATE"); return;
          }
          hz=fullTiHz; // countsLast already belongs to this full TI1F gate
        }
      } else if (tiHz>=2000.0 && tiHz<=TI_CORRECT_MAX_HZ) {
        if (etrHz>TI_CORRECT_MAX_HZ) {
          // A stable in-range N=2 alias can also have a non-integer ratio
          // to a genuine above-13-MHz input.  It is outside the sampled
          // filter's certifying range and therefore cannot veto ETR here.
          hz=etrHz; countsLast=etrCounts; setLabel(etrGateLabel);
        } else {
          // Both results are in the TI filter's reliable range yet disagree
          // by a non-integer ratio: suppress the number rather than guess.
          setLabel("ETR/8 + TI1F2");
          showStatus("INPUT CHANGING", "WAIT FOR STABLE RATE"); return;
        }
      } else {
        // Above the reliable filtered-TI1 range, or the TI1 filter found
        // no qualifying edges (for example a genuine short-pulse signal):
        // TI1F cannot overturn ETR.  Restore the completed ETR result.
        hz=etrHz; countsLast=etrCounts;
        setLabel(etrHz>TI_CORRECT_MAX_HZ ? etrGateLabel : "ETR/8 TI1F N/A");
        (void)tiCounts;
      }
    }
    }
    registersOK=true; gateValid=true; inputChanging=false;
  }

  if (hz<2000.0) {
    // MEASURING is a claim that edges exist; it must never be the default
    // screen for a disconnected input.  The completed ETR/8 gate already
    // proves presence above roughly 8 Hz (countsLast > 0).  Below that,
    // silently probe ETR/1 for one full 1 Hz period plus margin while the
    // previous screen (NO SIGNAL, or the last valid reading) stays up.
    bool edgePresent=(countsLast>0u);
    if (!edgePresent) {
      edgePresent=probeEdge();
      if (!registersOK) {
        setLabel("ETR/1 EDGE PROBE");
        show(0,"TIMER CONFIG FAILED"); delay(300); return;
      }
    }
    if (!edgePresent) {
      // No ETR/8 count in 1 s AND no ETR/1 edge in another 1250 ms.
      // This is a measured absence, not a pre-measurement guess: display
      // NO SIGNAL and keep it displayed throughout the next silent cycle.
      setLabel("NO EDGE DETECTED");
      show(0);
      digitalWrite(PC13,LOW); delay(30); digitalWrite(PC13,HIGH);
      return;
    }
    // Only now is MEASURING truthful: at least one physical edge was seen,
    // but more edges are still needed for a qualified low-rate result.
    setLabel("ETR/1 EDGE TIMING");
    showStatus("MEASURING", "WAITING FOR EDGES");
    hz=lowRate();
  }
  hz /= (1.0+CAL_PPM/1000000.0);
  if (!registersOK) show(0,"TIMER CONFIG FAILED");
  else if (!gateValid) show(0,"COUNT SYNC LOST");
  else if (inputChanging) showStatus("INPUT CHANGING", "WAIT FOR STABLE RATE");
  else {
    // Only a qualified display refreshes/releases high-range memory.
    // A result above displayMaxHz is also qualified evidence of a high
    // input, even though OVER RANGE intentionally withholds its number.
    if (hz>TI_CHECK_MAX_HZ) noteHighRange(hz);
    else if (hz>0.0) highRangeMemory=false;
    show(hz);
  }
  digitalWrite(PC13,LOW); delay(30); digitalWrite(PC13,HIGH);
}
