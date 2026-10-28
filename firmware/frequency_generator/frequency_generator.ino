/*
 * Precision Frequency Generator for STM32F103C8T6
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
 * WIRING AND OPERATION
 * PA8 -> 47 ohm -> counter PA0; both boards share GND.  An optional
 * normally-open push button connects PB12 to GND.  The generator starts
 * at 1 Hz and automatically advances every 15 seconds.  A short press
 * advances on release; holding PB12 for at least one second pauses or
 * resumes the sweep without stopping TIM1 or interrupting PA8.
 *
 * EXACT OUTPUT PLAN
 * Default normal-clock sweep:
 *   1, 10, 100 Hz; 1, 10, 100 kHz;
 *   1, 2, 4, 8, 12, 18, 24, 36 MHz.
 * Setting TOP_EXPERIMENT_64MHZ to 1 appends exact 40, 48, 56 and
 * 64 MHz steps by re-clocking TIM1 to 80, 96, 112 and 128 MHz.  Those
 * steps exceed the STM32F103's 72 MHz specification.  The 64 MHz step
 * is RAM-isolated: TIM1 is staged at 112 MHz, SRAM-resident code
 * switches the core to 128 MHz for the dwell, then restores 72 MHz
 * before flash execution resumes.  PB12 is ignored during that isolated
 * dwell and in every fixed-frequency diagnostic mode.
 *
 * There is deliberately no fictitious 65 MHz or 80 MHz substitution.
 * SWEEP_MODE 1 exposes the exact requested diagnostic list and forces
 * unsupported entries to PA8 LOW with a repeated four-flash error code;
 * unsupported requests are never silently emitted at another rate.
 * PA8 slew is selected by frequency to avoid unnecessarily fast edges.
 * PC13 reports three flashes at successful boot configuration, two for
 * a supported step, four for unsupported and five for a timer-clock or
 * configuration fault.  A single heartbeat indicates automatic sweep;
 * a double heartbeat indicates pause.  A heartbeat proves that code is
 * running, not that a load is receiving a valid physical waveform.
 *
 * No MCO route, serial dependency or external Arduino library is used.
 */

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
static const char HEXOID_FIRMWARE_IDENTITY[] =
  HEXOID_POWERED_BY
  "\nAuthor: " HEXOID_AUTHOR_DISPLAY
  "\nAlso known as: " HEXOID_AUTHOR_ALSO_KNOWN_AS
  "\nGitHub: " HEXOID_GITHUB_PROFILE_URL
  "\nRepositories: " HEXOID_GITHUB_REPOSITORIES_URL
  "\nProject: " HEXOID_REPOSITORY_URL
  "\nYouTube: " HEXOID_YOUTUBE_URL
  "\nBlog: " HEXOID_BLOG_URL
  "\n" HEXOID_COPYRIGHT
  "\nLicence: " HEXOID_LICENCE_NAME;
// setup() stores this pointer once so linker section garbage collection
// cannot discard the attribution string from the compiled firmware.
volatile const char *hexoidIdentityAnchor;
#define SWEEP_MODE 0       // 0 attainable exact sweep; 1 requested list with explicit skips
#define DWELL_MS 15000UL
#define FIXED_TEST_8MHZ 0  // 1: hold exact 8 MHz, default attainable table only
#define FIXED_TEST_36MHZ 0 // 1: hold exact 36 MHz, default attainable table only
#define FIXED_TEST_64MHZ 0 // 1: hold exact 64 MHz; requires TOP_EXPERIMENT_64MHZ 1
#define TOP_EXPERIMENT_64MHZ 0  // 1: append 40/48/56/64 MHz overclocked steps
#if TOP_EXPERIMENT_64MHZ && SWEEP_MODE != 0
  #error TOP_EXPERIMENT_64MHZ requires SWEEP_MODE 0
#endif
#if (FIXED_TEST_8MHZ + FIXED_TEST_36MHZ + FIXED_TEST_64MHZ) > 1
  #error Select only one fixed-frequency test
#endif
#if FIXED_TEST_8MHZ || FIXED_TEST_36MHZ || FIXED_TEST_64MHZ
  #if SWEEP_MODE != 0
    #error Fixed-frequency tests require SWEEP_MODE 0
  #endif
  #if (FIXED_TEST_8MHZ || FIXED_TEST_36MHZ) && TOP_EXPERIMENT_64MHZ
    #error Fixed 8/36 MHz tests use the normal 72 MHz table: set TOP_EXPERIMENT_64MHZ 0
  #endif
  #if FIXED_TEST_64MHZ && !TOP_EXPERIMENT_64MHZ
    #error Fixed 64 MHz test requires TOP_EXPERIMENT_64MHZ 1
  #endif
  #define AUTO_SWEEP 0
  #define START_INDEX (FIXED_TEST_8MHZ ? 9 : (FIXED_TEST_36MHZ ? 13 : 17))
#else
  #define AUTO_SWEEP 1
  #define START_INDEX 0
#endif
#define BUTTON_PIN PB12
#define LED_PIN PC13
#if SWEEP_MODE == 0
#if TOP_EXPERIMENT_64MHZ
static const uint32_t steps[]={1,10,100,1000,10000,100000,1000000,
  2000000,4000000,8000000,12000000,18000000,24000000,36000000,
  40000000,48000000,56000000,64000000};
#else
static const uint32_t steps[]={1,10,100,1000,10000,100000,1000000,
  2000000,4000000,8000000,12000000,18000000,24000000,36000000};
#endif
#elif SWEEP_MODE == 1
static const uint32_t steps[]={1,10,100,1000,10000,100000,1000000,
  10000000,20000000,27000000,33000000,50000000,68000000,72000000,80000000};
#else
#error SWEEP_MODE must be 0 or 1
#endif
static const uint8_t STEP_N=sizeof(steps)/sizeof(steps[0]);
static_assert(START_INDEX < STEP_N, "Invalid START_INDEX");
static uint8_t stepIndex=START_INDEX, state=5;
static uint32_t stepAt, blinkAt;
static bool sweeping=(AUTO_SWEEP != 0);

static void flash(uint8_t n) {
  for (uint8_t i=0;i<n;++i) {
    digitalWrite(LED_PIN,LOW); delay(90);
    digitalWrite(LED_PIN,HIGH); delay(150);
  }
}
#if TOP_EXPERIMENT_64MHZ
static bool waitGenClockBits(volatile uint32_t *reg, uint32_t mask, uint32_t want) {
  for (volatile uint32_t n=0;n<4000000u;++n) {
    if ((*reg & mask)==want) return true;
  }
  return false;
}
// Exact core clocks from the stock 8 MHz HSE: x9=72 through x16=128 MHz.
// TIM1 is on APB2/1 in the Arduino clock tree, so TIM1CLK follows HCLK.
static bool setCoreMHz(uint32_t mhz) {
  SystemCoreClockUpdate();
  if (SystemCoreClock==mhz*1000000u && ((RCC->CFGR>>2)&3u)==2u) return true;
  if (mhz<72u || mhz>128u || (mhz%8u)!=0u) return false;
  if ((RCC->CR & (1u<<17))==0u) {
    RCC->CR |= (1u<<16);
    if (!waitGenClockBits(&RCC->CR,(1u<<17),(1u<<17))) return false;
  }
  FLASH->ACR=(FLASH->ACR & ~7u) | 2u;
  // Keep peripheral buses bounded in the overclocked steps.  TIM1 is on
  // APB2; with PPRE2=/2 its timer clock is 2*PCLK2=HCLK, so the exact
  // core/2 outputs are retained while PCLK2 itself is HCLK/2.  At
  // 128 MHz this gives PCLK1=32 MHz and PCLK2=64 MHz instead of 64/128.
  uint32_t prescalers=RCC->CFGR;
  prescalers &= ~((15u<<4) | (7u<<8) | (7u<<11)); // HPRE, PPRE1, PPRE2
  if (mhz==72u) prescalers |= (4u<<8);             // standard: PPRE1 /2, PPRE2 /1
  else          prescalers |= (5u<<8) | (4u<<11);  // top: PPRE1 /4, PPRE2 /2
  RCC->CFGR=prescalers;
  RCC->CFGR=(RCC->CFGR & ~3u) | 1u;              // SW=HSE
  if (!waitGenClockBits(&RCC->CFGR,(3u<<2),(1u<<2))) return false;
  RCC->CR &= ~(1u<<24);                          // PLL off
  if (!waitGenClockBits(&RCC->CR,(1u<<25),0u)) return false;
  uint32_t mul=mhz/8u;
  uint32_t cfgr=RCC->CFGR;
  cfgr &= ~((1u<<17) | (15u<<18));
  cfgr |= (1u<<16) | ((mul-2u)<<18);             // PLLSRC=HSE
  RCC->CFGR=cfgr;
  RCC->CR |= (1u<<24);
  if (!waitGenClockBits(&RCC->CR,(1u<<25),(1u<<25))) return false;
  RCC->CFGR=(RCC->CFGR & ~3u) | 2u;              // SW=PLL
  if (!waitGenClockBits(&RCC->CFGR,(3u<<2),(2u<<2))) return false;
  SystemCoreClockUpdate();
  if (SystemCoreClock!=mhz*1000000u) return false;
  const uint32_t systickCtrl=SysTick->CTRL;
  SysTick->CTRL=0;
  SysTick->LOAD=(SystemCoreClock/1000u)-1u;
  SysTick->VAL=0;
  SysTick->CTRL=systickCtrl;
  return true;
}
#endif


#if TOP_EXPERIMENT_64MHZ
#define GEN_RAMFUNC __attribute__((section(".RamFunc"), noinline, used))

__attribute__((always_inline)) static inline bool ramWaitGenClockBits(
    volatile uint32_t *reg, uint32_t mask, uint32_t want) {
  for (volatile uint32_t n=0;n<4000000u;++n) {
    if ((*reg & mask)==want) return true;
  }
  return false;
}

// This function is copied to SRAM by the startup code (.RamFunc is part of
// .data in the Blue Pill linker script).  It is deliberately self-contained:
// direct register accesses only, no millis/delay/digitalWrite/System* calls,
// no 64-bit arithmetic and no division.  TIM1 must already be running with
// PSC=0, ARR=1, CCR1=1 at a 112 MHz core (56 MHz output).  The function
// switches HCLK to 128 MHz while executing from SRAM, so PA8 becomes 64 MHz
// without the CPU having to fetch flash at 128 MHz.  It polls SysTick with
// its interrupt disabled for the dwell, then switches back to 72 MHz before
// returning to flash.  dwellMs=0 means hold indefinitely (fixed/paused 64).
GEN_RAMFUNC static uint8_t ramRun64Step(uint32_t dwellMs) {
  const uint32_t savedSysTickCtrl=SysTick->CTRL;
  const uint32_t savedSysTickLoad=SysTick->LOAD;
  uint8_t result=0;
  uint32_t elapsedMs=0;
  uint32_t lastToggleMs=0;
  bool ledOn=true;
  __disable_irq();

  RCC->CR |= (1u<<16);                         // HSEON
  if (!ramWaitGenClockBits(&RCC->CR,(1u<<17),(1u<<17))) {
    SysTick->CTRL=0;
    SysTick->LOAD=savedSysTickLoad;
    SysTick->VAL=0;
    SysTick->CTRL=savedSysTickCtrl;
    __enable_irq();
    return 5;
  }

  FLASH->ACR=(FLASH->ACR & ~7u) | 2u;           // maximum defined latency
  uint32_t cfgr=RCC->CFGR;
  cfgr &= ~((15u<<4) | (7u<<8) | (7u<<11));     // HPRE /1
  cfgr |= (5u<<8) | (4u<<11);                   // PPRE1 /4, PPRE2 /2
  RCC->CFGR=cfgr;

  RCC->CFGR=(RCC->CFGR & ~3u) | 1u;             // SW=HSE
  if (!ramWaitGenClockBits(&RCC->CFGR,(3u<<2),(1u<<2))) { result=5; goto restore72; }
  RCC->CR &= ~(1u<<24);                         // PLL off
  if (!ramWaitGenClockBits(&RCC->CR,(1u<<25),0u)) { result=5; goto restore72; }
  cfgr=RCC->CFGR;
  cfgr &= ~((1u<<17) | (15u<<18));
  cfgr |= (1u<<16) | (14u<<18);                 // HSE x16 = 128 MHz
  RCC->CFGR=cfgr;
  RCC->CR |= (1u<<24);                          // PLL on
  if (!ramWaitGenClockBits(&RCC->CR,(1u<<25),(1u<<25))) { result=5; goto restore72; }
  RCC->CFGR=(RCC->CFGR & ~3u) | 2u;             // SW=PLL: PA8 is now 64 MHz
  if (!ramWaitGenClockBits(&RCC->CFGR,(3u<<2),(2u<<2))) { result=5; goto restore72; }

  // Poll SysTick at 1 ms (128,000 core cycles), TICKINT off.  Reading CTRL
  // clears COUNTFLAG.  Toggle PC13 directly every 500 ms as a RAM heartbeat.
  SysTick->CTRL=0;
  SysTick->LOAD=128000u-1u;
  SysTick->VAL=0;
  SysTick->CTRL=(1u<<2) | 1u;                   // CLKSOURCE=HCLK, ENABLE
  GPIOC->BRR=(1u<<13);                          // PC13 LED on (active low)
  while (dwellMs==0u || elapsedMs<dwellMs) {
    if ((SysTick->CTRL & (1u<<16))!=0u) {
      ++elapsedMs;
      if ((uint32_t)(elapsedMs-lastToggleMs)>=500u) {
        lastToggleMs=elapsedMs;
        ledOn=!ledOn;
        if (ledOn) GPIOC->BRR=(1u<<13);
        else       GPIOC->BSRR=(1u<<13);
      }
    }
  }

restore72:
  // Return to HSE first, restore the standard bus prescalers, then PLL x9.
  // This entire sequence remains in SRAM; flash execution resumes only
  // after the switch back to the 72 MHz PLL (or, on a status failure, HSE).
  RCC->CFGR=(RCC->CFGR & ~3u) | 1u;             // SW=HSE
  if (!ramWaitGenClockBits(&RCC->CFGR,(3u<<2),(1u<<2))) result=5;
  cfgr=RCC->CFGR;
  cfgr &= ~((15u<<4) | (7u<<8) | (7u<<11));
  cfgr |= (4u<<8);                              // PPRE1 /2, PPRE2 /1
  RCC->CFGR=cfgr;
  RCC->CR &= ~(1u<<24);                         // PLL off
  if (!ramWaitGenClockBits(&RCC->CR,(1u<<25),0u)) result=5;
  cfgr=RCC->CFGR;
  cfgr &= ~((1u<<17) | (15u<<18));
  cfgr |= (1u<<16) | (7u<<18);                  // HSE x9 = 72 MHz
  RCC->CFGR=cfgr;
  RCC->CR |= (1u<<24);                          // PLL on
  if (!ramWaitGenClockBits(&RCC->CR,(1u<<25),(1u<<25))) result=5;
  RCC->CFGR=(RCC->CFGR & ~3u) | 2u;             // SW=PLL
  if (!ramWaitGenClockBits(&RCC->CFGR,(3u<<2),(2u<<2))) result=5;

  SysTick->CTRL=0;
  SysTick->LOAD=72000u-1u;
  SysTick->VAL=0;
  SysTick->CTRL=savedSysTickCtrl;
  GPIOC->BSRR=(1u<<13);                         // PC13 LED off
  __enable_irq();
  return result;
}
#endif

static void outputOff() {
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
  (void)RCC->APB2ENR;
  TIM1->CR1=0; TIM1->CCER=0; TIM1->BDTR=0;
  RCC->CFGR &= ~(7u<<24);
  GPIOA->BRR=1u<<8;
  GPIOA->CRH=(GPIOA->CRH & ~15u)|3u;  // PA8 GPIO push-pull LOW
}
static uint8_t setOutput(uint32_t hz) {
  outputOff();
#if TOP_EXPERIMENT_64MHZ
  const uint32_t coreMHz=hz<=36000000u ? 72u : hz/500000u; // 40..64 -> 80..128
  if (!setCoreMHz(coreMHz)) {
    (void)setCoreMHz(72u);
    outputOff();
    return 5;
  }
  const uint32_t expectedClock=coreMHz*1000000u;
#else
  const uint32_t expectedClock=72000000UL;
#endif
  SystemCoreClockUpdate();
  const uint8_t d[]={1,1,1,1,2,4,8,16};
  uint32_t apb=(RCC->CFGR>>11)&7u;
  uint32_t clock=SystemCoreClock/d[apb]; if (apb>=4u) clock*=2u;
  if (clock!=expectedClock) return 5;
  if (!hz || clock%hz || hz>clock/2u) return 4;
  uint32_t total=clock/hz, psc=1, arr=0;
  // Factor into two exact 16-bit divisors. This is essential for 1 Hz:
  // ARR alone cannot hold 72,000,000 ticks.
  for (psc=1;psc<=65536u;++psc) {
    if (total%psc==0 && total/psc>=2u && total/psc<=65536u) {
      arr=total/psc; break;
    }
  }
  if (!arr) return 4;
  AFIO->MAPR &= ~(3u<<6);
  RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM1RST;
  // STM32F1 AF push-pull nibble: 0xA=2 MHz slew, 0x9=10 MHz, 0xB=50 MHz.
  // Use the slowest specified slew that supports the selected output.
  uint32_t pa8Mode=hz<=2000000u ? 10u : (hz<=10000000u ? 9u : 11u);
  GPIOA->CRH=(GPIOA->CRH & ~15u)|pa8Mode;
  TIM1->PSC=psc-1u; TIM1->ARR=arr-1u; TIM1->CCR1=arr/2u;
  TIM1->CCMR1=6u<<4; TIM1->CCER=1u; TIM1->BDTR=1u<<15;
  TIM1->EGR=1u; TIM1->SR=0; TIM1->CR1=1u;
  return 0;
}
#if TOP_EXPERIMENT_64MHZ
static uint8_t run64Step() {
  // Stage the identical TIM1 /2 register configuration at the already
  // hardware-proven 112 MHz core.  ramRun64Step then changes only the
  // clock; it does not need to touch TIM1 while HCLK is 128 MHz.
  uint8_t staged=setOutput(56000000u);
  if (staged) return staged;
  if (TIM1->PSC!=0u || TIM1->ARR!=1u || TIM1->CCR1!=1u ||
      TIM1->CCER!=1u || TIM1->BDTR!=(1u<<15) || !(TIM1->CR1 & 1u)) {
    outputOff();
    return 5;
  }
  const uint32_t dwellMs=sweeping ? (uint32_t)DWELL_MS : 0u;
  uint8_t ran=ramRun64Step(dwellMs);
  if (ran) {
    outputOff();
    (void)setCoreMHz(72u);
  }
  return ran;
}
#endif
static void selectStep(bool boot=false) {
#if TOP_EXPERIMENT_64MHZ
  if (steps[stepIndex]==64000000u) {
    state=run64Step();
#if FIXED_TEST_64MHZ
    // A successful fixed-64 run never returns; reaching here is a failure.
    flash(state ? state : 5);
    stepAt=millis(); blinkAt=stepAt;
    return;
#else
    if (!state && sweeping) {
      // The RAM routine has already held 64 MHz for the full dwell and
      // restored a 72 MHz core.  Move directly to the wrap step instead
      // of holding the temporarily restored TIM1/2 = 36 MHz output.
      stepIndex=0;
      state=setOutput(steps[stepIndex]);
      flash(state ? state : 2);
      stepAt=millis(); blinkAt=stepAt;
      return;
    }
    // When paused, a successful 64 MHz run holds in RAM and does not
    // return; reaching here means failure.  Keep the failed step/state.
    flash(state ? state : 5);
    stepAt=millis(); blinkAt=stepAt;
    return;
#endif
  }
#endif
  state=setOutput(steps[stepIndex]);
  flash(state ? state : (boot ? 3 : 2));
  stepAt=millis(); blinkAt=stepAt;
}
void setup() {
  hexoidIdentityAnchor=HEXOID_FIRMWARE_IDENTITY;  // retain identity in firmware image
  pinMode(LED_PIN,OUTPUT); digitalWrite(LED_PIN,HIGH);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  selectStep(true);
}
// Short press/release: next frequency. Hold >=1 s: pause/resume sweep.
// Pausing does not stop/reset TIM1 and does not interrupt the output.
void loop() {
#if FIXED_TEST_8MHZ || FIXED_TEST_36MHZ || FIXED_TEST_64MHZ
  // A diagnostic fixed-frequency source must be fixed in fact, not merely
  // start paused.  Do not read PB12 at all: no short-press step and no
  // long-press resume can alter TIM1 or stepIndex in this build.
  sweeping=false;
#else
  static bool rawLast=HIGH, stable=HIGH, longHandled=false;
  static uint32_t changed=0, pressedAt=0;
  bool raw=digitalRead(BUTTON_PIN), advance=false;
  if (raw!=rawLast) { rawLast=raw; changed=millis(); }
  if (raw!=stable && (uint32_t)(millis()-changed)>=35u) {
    stable=raw;
    if (stable==LOW) { pressedAt=millis(); longHandled=false; }
    else if (!longHandled) advance=true;
  }
  if (stable==LOW && !longHandled && (uint32_t)(millis()-pressedAt)>=1000u) {
    longHandled=true; sweeping=!sweeping;
    stepAt=millis(); // full dwell when resuming
    flash(sweeping ? 3 : 1); // does not touch the output timer
  }
  if (advance || (sweeping && stable==HIGH && (uint32_t)(millis()-stepAt)>=DWELL_MS)) {
    stepIndex=(stepIndex+1u)%STEP_N;
    selectStep();
  }
#endif
  if (state) {
    if ((uint32_t)(millis()-blinkAt)>=2500u) { flash(state); blinkAt=millis(); }
  } else {
    // 50 ms pulse once per second; does not touch TIM1.
    uint32_t phase=(millis()-stepAt)%1000u;
    bool on=phase<50u || (!sweeping && phase>=150u && phase<200u);
    digitalWrite(LED_PIN,on ? LOW : HIGH);
  }
}
