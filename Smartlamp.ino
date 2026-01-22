/**
 * Gesture Controlled Smart Lamp
 * * Hardware: Arduino, APDS9960 Sensor, WS2812B LEDs
 * Libraries: FastLED, SparkFun_APDS9960
 * * Features:
 * - Proximity-based ON/OFF
 * - Manual brightness control (hold to unlock, lift hand to save)
 * - Color picker with breathing effect
 * - Auto-brightness on startup
 */

#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

SparkFun_APDS9960 apds;

// ================= LED =================
#define LED_PIN      6
#define NUM_LEDS     60
CRGB leds[NUM_LEDS];

// ================= PROX LOGIC =================
#define ON_MIN 50
#define ON_MAX 255
#define OFF_PROX 254

#define COLOR_HOLD_MIN 50
#define COLOR_HOLD_MAX 200
#define COLOR_HOLD_TIME 3000
#define COLOR_STEADY_TOL 20   

#define ON_HOLD_TIME 1000
#define OFF_HOLD_TIME 1200
#define AUTO_TIME 3000

#define DIMMEST 10
#define BRIGHTEST 255

#define FADE_IN_DURATION 120
#define FADE_OUT_DURATION 120

#define PICKER_GRACE_PERIOD 2000
#define OFF_IGNORE_TIME 2000   

// ================= STATES =================
enum State {
  OFF_STATE,
  AUTO_ADJUSTING,
  MANUAL_LOCKED,
  MANUAL_UNLOCKED,
  COLOR_PICKER_MODE
};
State state = OFF_STATE;

// ================= TIMERS =================
unsigned long holdStart = 0;
unsigned long autoStart = 0;
unsigned long offHoldStart = 0;
unsigned long colorHoldStart = 0;
unsigned long pickerGraceStart = 0;
unsigned long offIgnoreStart = 0;
unsigned long previousBrightnessTime = 0;

// ================= VARIABLES =================
int lastBrightness = -1;
int previousBrightness = -1;
int lastProx = -1;
int colorHoldProx = -1;
int pickerLastColorIndex = 0;

uint8_t currentR = 255, currentG = 255, currentB = 255;

// ================= COLORS =================
const CRGB pickerColors[5] = {
  CRGB::White,
  CRGB::DeepPink,       // Pink
  CRGB::Gold,           // Reading Friendly Yellow
  CRGB::BlueViolet,     // Blue stronger towards purple
  CRGB::Tomato          // Light Red (Replacing Orange)
};

// ================= FADE ENGINE =================
bool fading = false;
unsigned long fadeStart;
int fadeFrom, fadeTo;
unsigned long fadeDuration;

void startFade(int from, int to, unsigned long duration) {
  fadeFrom = from;
  fadeTo = to;
  fadeDuration = duration;
  fadeStart = millis();
  fading = true;
}

void updateFade() {
  if (!fading) return;
  float p = float(millis() - fadeStart) / fadeDuration;
  if (p >= 1.0) {
    FastLED.setBrightness(fadeTo);
    FastLED.show();
    fading = false;
    lastBrightness = fadeTo;
  } else {
    FastLED.setBrightness(fadeFrom + (fadeTo - fadeFrom) * p);
    FastLED.show();
  }
}

// ================= NONLINEAR BRIGHTNESS =================
int nonlinearBrightness(uint8_t prox) {
  float norm = constrain((prox - ON_MIN) / float(ON_MAX - ON_MIN), 0, 1);
  float curved = pow(norm, 0.45);
  return constrain(BRIGHTEST - curved * (BRIGHTEST - DIMMEST), DIMMEST, BRIGHTEST);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Wire.begin();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(0);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  if (!apds.init()) {
    Serial.println("APDS INIT FAILED");
    while (1);
  }
  apds.enableProximitySensor(false);
  apds.enableLightSensor(false);
  apds.setProximityGain(PGAIN_8X);

  Serial.println("=== SMART LAMP FINAL STABLE BUILD ===");
}

// ================= LOOP =================
void loop() {
  updateFade();

  uint8_t prox;
  uint16_t ambient;
  if (!apds.readProximity(prox)) return;
  apds.readAmbientLight(ambient);

  unsigned long now = millis();

  // ===== Ignore proximity after OFF =====
  if (state == OFF_STATE && offIgnoreStart && now - offIgnoreStart < OFF_IGNORE_TIME) {
    return;
  }

  // ===== Track previous brightness =====
  if ((state == MANUAL_UNLOCKED || state == COLOR_PICKER_MODE) &&
      now - previousBrightnessTime > 500) {
    previousBrightness = lastBrightness;
    previousBrightnessTime = now;
  }

  // ===== COLOR PICKER ENTRY =====
  if ((state == MANUAL_LOCKED || state == MANUAL_UNLOCKED) &&
      prox >= COLOR_HOLD_MIN && prox <= COLOR_HOLD_MAX) {

    if (colorHoldProx == -1) {
      colorHoldProx = prox;
      colorHoldStart = now;
    }

    if (abs(prox - colorHoldProx) <= COLOR_STEADY_TOL) {
      if (now - colorHoldStart >= COLOR_HOLD_TIME) {
        state = COLOR_PICKER_MODE;
        pickerGraceStart = 0;
        Serial.println(">>> COLOR PICKER MODE");
        colorHoldProx = -1;
      }
    } else {
      colorHoldProx = prox;
      colorHoldStart = now;
    }
  } else {
    colorHoldProx = -1;
  }

  // ===== OFF GESTURE (ONLY WHEN STABLE ON) =====
  if ((state == MANUAL_LOCKED || state == MANUAL_UNLOCKED) && prox >= OFF_PROX) {
    if (!offHoldStart) offHoldStart = now;
    if (now - offHoldStart >= OFF_HOLD_TIME) {
      startFade(lastBrightness > 0 ? lastBrightness : BRIGHTEST, 0, FADE_OUT_DURATION);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      state = OFF_STATE;
      offIgnoreStart = now;
      Serial.println(">>> OFF");
      return;
    }
  } else offHoldStart = 0;

  // ================= STATE MACHINE =================
  switch (state) {

    case OFF_STATE:
      if (prox >= ON_MIN && prox <= ON_MAX) {
        if (!holdStart) holdStart = now;
        if (now - holdStart >= ON_HOLD_TIME) {
          state = AUTO_ADJUSTING;
          autoStart = now;
          int b = map(ambient, 0, 1000, DIMMEST, BRIGHTEST);
          fill_solid(leds, NUM_LEDS, CRGB(currentR, currentG, currentB));
          startFade(0, b, FADE_IN_DURATION);
          Serial.println(">>> ON → AUTO");
        }
      } else holdStart = 0;
      break;

    case AUTO_ADJUSTING:
      if (now - autoStart >= AUTO_TIME) {
        state = MANUAL_LOCKED;
        Serial.println(">>> AUTO DONE → LOCKED");
      }
      break;

    case MANUAL_LOCKED:
      if (prox >= ON_MIN && prox <= ON_MAX) {
        if (!holdStart) holdStart = now;
        if (now - holdStart >= ON_HOLD_TIME) {
          state = MANUAL_UNLOCKED;
          lastProx = prox;
          Serial.println(">>> MANUAL UNLOCKED");
        }
      } else holdStart = 0;
      break;

    case MANUAL_UNLOCKED:
      // Exit if hand removed (prox not in range)
      if (prox >= ON_MIN && prox <= ON_MAX) {
        int b = nonlinearBrightness(prox);
        FastLED.setBrightness(b);
        fill_solid(leds, NUM_LEDS, CRGB(currentR, currentG, currentB));
        FastLED.show();
        lastBrightness = b;
        lastProx = prox;
      } else {
        // Hand removed -> Lock and Save
        state = MANUAL_LOCKED;
        Serial.println(">>> HAND REMOVED → LOCKED (SAVED)");
      }
      break;

    case COLOR_PICKER_MODE:
      // Generate Breathing Effect (Sine wave)
      uint8_t breatheVal = beatsin8(30, 50, 150);

      if (prox >= 60 && prox <= 255) {
        pickerGraceStart = 0;
        float norm = (prox - 60) / 140.0;
        pickerLastColorIndex = constrain(int((1 - norm) * 5), 0, 4);
        
        fill_solid(leds, NUM_LEDS, pickerColors[pickerLastColorIndex]);
        
        // Apply Breathing Brightness
        FastLED.setBrightness(breatheVal);
        FastLED.show();
      } else {
        if (!pickerGraceStart) pickerGraceStart = now;
        if (now - pickerGraceStart >= PICKER_GRACE_PERIOD) {
          CRGB c = pickerColors[pickerLastColorIndex];
          currentR = c.r; currentG = c.g; currentB = c.b;
          state = MANUAL_LOCKED;
          
          // Ensure full brightness when saving/exiting
          fill_solid(leds, NUM_LEDS, CRGB(currentR, currentG, currentB));
          FastLED.setBrightness(BRIGHTEST); 
          FastLED.show();
          
          Serial.println(">>> COLOR SAVED → EXIT PICKER");
        }
      }
      break;
  }

  delay(40);
}
