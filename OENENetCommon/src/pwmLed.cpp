#include "pwmLed.h"
#include <math.h>

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------

static const int* _pins = nullptr;
static int _count = 0;

static int _numRgb  = 0;
static int _numLed  = 0;

static int _rgbIndexMap[16][3];
static int _ledIndexMap[32];

static uint16_t gammaTable[4096];

struct PwmFadeState {
    uint16_t startDuty;
    uint16_t targetDuty;
    uint32_t startTime;
    uint32_t duration;
    bool active;
};

static PwmFadeState* fadeState = nullptr;

// -----------------------------------------------------------------------------
// Error callback
// -----------------------------------------------------------------------------

static PwmErrorCallback _errorCb = nullptr;

void pwmLedSetErrorCallback(PwmErrorCallback cb) {
    _errorCb = cb;
}

static void raiseError(const char* code) {
    if (_errorCb)
        _errorCb(code);
}

// -----------------------------------------------------------------------------
// Gamma table
// -----------------------------------------------------------------------------

static void initGammaTable(float gamma) {
    for (int i = 0; i < 4096; i++) {
        float n = (float)i / 4095.0f;
        gammaTable[i] = (uint16_t)(powf(n, gamma) * 4095.0f);
    }
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static uint16_t interpolateDuty(const PwmFadeState& f, uint32_t nowMs) {
    if (!f.active)
        return f.targetDuty;

    uint32_t elapsed = nowMs - f.startTime;
    if (elapsed >= f.duration)
        return f.targetDuty;

    int32_t start  = (int32_t)f.startDuty;
    int32_t target = (int32_t)f.targetDuty;
    int32_t delta  = target - start;

    int32_t num    = delta * (int32_t)elapsed;
    int32_t interp = start + num / (int32_t)f.duration;

    if (interp < 0)    interp = 0;
    if (interp > 4095) interp = 4095;

    return (uint16_t)interp;
}

static void stopFade(int ch) {
    if (ch < 0 || ch >= _count) return;
    fadeState[ch].active = false;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void pwmLedInit(const int* pins, int count) {
    _pins = pins;
    _count = count;

    fadeState = new PwmFadeState[count];

    initGammaTable(2.2f);

    for (int i = 0; i < count; i++) {
        ledcAttach(_pins[i], 5000, 12);
        ledcWrite(_pins[i], 0);

        fadeState[i].active     = false;
        fadeState[i].startDuty  = 0;
        fadeState[i].targetDuty = 0;
        fadeState[i].startTime  = 0;
        fadeState[i].duration   = 0;
    }
}

void pwmLedConfigure(int numRgb, int numSingle) {
    _numRgb = numRgb;
    _numLed = numSingle;

    int needed = _numRgb * 3 + _numLed;
    if (needed > _count) {
        raiseError("PWM_LAYOUT_TOO_LARGE");
        _numRgb = 0;
        _numLed = 0;
    }
}

void pwmLedInitMapping() {
    int needed = _numRgb * 3 + _numLed;

    if (needed > _count) {
        raiseError("PWM_NOT_ENOUGH_CHANNELS");
        return;
    }

    int ch = 0;

    for (int i = 0; i < _numRgb; i++) {
        _rgbIndexMap[i][0] = ch++;
        _rgbIndexMap[i][1] = ch++;
        _rgbIndexMap[i][2] = ch++;
    }

    for (int i = 0; i < _numLed; i++) {
        _ledIndexMap[i] = ch++;
    }
}

// -----------------------------------------------------------------------------
// Fade engines
// -----------------------------------------------------------------------------

void pwmLedFade(int ch, uint16_t target, uint32_t duration) {
    if (ch < 0 || ch >= _count)
        return;

    if (target > 4095)
        target = 4095;

    // Immediate update
    if (duration == 0) {
        fadeState[ch].active     = false;
        fadeState[ch].startDuty  = target;
        fadeState[ch].targetDuty = target;
        ledcWrite(_pins[ch], gammaTable[target]);
        return;
    }

    uint32_t now = millis();
    uint16_t current = interpolateDuty(fadeState[ch], now);

    PwmFadeState &f = fadeState[ch];
    f.startDuty  = current;
    f.targetDuty = target;
    f.startTime  = now;
    f.duration   = duration;
    f.active     = true;
}

void pwmRgbLedFade(int rgbIndex,
                   uint16_t r, uint16_t g, uint16_t b,
                   uint32_t duration)
{
    if (rgbIndex < 0 || rgbIndex >= _numRgb)
        return;

    if (r > 4095) r = 4095;
    if (g > 4095) g = 4095;
    if (b > 4095) b = 4095;

    pwmLedFade(_rgbIndexMap[rgbIndex][0], r, duration);
    pwmLedFade(_rgbIndexMap[rgbIndex][1], g, duration);
    pwmLedFade(_rgbIndexMap[rgbIndex][2], b, duration);
}

void pwmSingleLedFade(int ledIndex,
                      uint16_t target,
                      uint32_t duration)
{
    if (ledIndex < 0 || ledIndex >= _numLed)
        return;

    pwmLedFade(_ledIndexMap[ledIndex], target, duration);
}

// -----------------------------------------------------------------------------
// High-level API (same semantics as OSC, 0-based indices)
// -----------------------------------------------------------------------------

void pwmLed(int ledIndex, uint16_t value, uint32_t timeMs) {
    if (ledIndex < 0 || ledIndex >= _numLed)
        return;
    pwmSingleLedFade(ledIndex, value, timeMs);
}

void pwmLedAll(uint16_t value, uint32_t timeMs) {
    // Stop fades on RGB channels (same behavior as /pwmLedAll)
    for (int i = 0; i < _numRgb; i++) {
        stopFade(_rgbIndexMap[i][0]);
        stopFade(_rgbIndexMap[i][1]);
        stopFade(_rgbIndexMap[i][2]);
    }

    for (int i = 0; i < _numLed; i++) {
        pwmSingleLedFade(i, value, timeMs);
    }
}

void pwmRgb(int rgbIndex, uint16_t r, uint16_t g, uint16_t b, uint32_t timeMs) {
    if (rgbIndex < 0 || rgbIndex >= _numRgb)
        return;
    pwmRgbLedFade(rgbIndex, r, g, b, timeMs);
}

void pwmRgbAll(uint16_t r, uint16_t g, uint16_t b, uint32_t timeMs) {
    for (int i = 0; i < _numRgb; i++) {
        pwmRgbLedFade(i, r, g, b, timeMs);
    }
}

// -----------------------------------------------------------------------------
// Update loop
// -----------------------------------------------------------------------------

void pwmLedUpdate() {
    uint32_t now = millis();

    for (int i = 0; i < _count; i++) {
        PwmFadeState &f = fadeState[i];
        if (!f.active)
            continue;

        uint16_t duty = interpolateDuty(f, now);

        if (duty == f.targetDuty) {
            f.active = false;
        }

        ledcWrite(_pins[i], gammaTable[duty]);
    }
}

// -----------------------------------------------------------------------------
// OSC HANDLERS
// -----------------------------------------------------------------------------

static void handlePwmLed(OSCMessage &msg) {
    if (msg.size() < 3) {
        raiseError("PWM_LED_INVALID_ARGS");
        return;
    }

    int ledIndex = msg.getInt(0) - 1;
    uint16_t value = msg.getInt(1);
    uint32_t timeMs = msg.getInt(2);

    if (ledIndex < 0 || ledIndex >= _numLed) {
        raiseError("PWM_LED_INDEX_OUT_OF_RANGE");
        return;
    }

    pwmLed(ledIndex, value, timeMs);
}

static void handlePwmLedAll(OSCMessage &msg) {
    if (msg.size() < 2) {
        raiseError("PWM_LEDALL_INVALID_ARGS");
        return;
    }

    uint16_t value = msg.getInt(0);
    uint32_t timeMs = msg.getInt(1);

    pwmLedAll(value, timeMs);
}

static void handlePwmRgb(OSCMessage &msg) {
    if (msg.size() < 5) {
        raiseError("PWM_RGB_INVALID_ARGS");
        return;
    }

    int rgbIndex = msg.getInt(0) - 1;
    uint16_t r = msg.getInt(1);
    uint16_t g = msg.getInt(2);
    uint16_t b = msg.getInt(3);
    uint32_t t = msg.getInt(4);

    if (rgbIndex < 0 || rgbIndex >= _numRgb) {
        raiseError("PWM_RGB_INDEX_OUT_OF_RANGE");
        return;
    }

    pwmRgb(rgbIndex, r, g, b, t);
}

static void handlePwmRgbAll(OSCMessage &msg) {
    if (msg.size() < 4) {
        raiseError("PWM_RGBALL_INVALID_ARGS");
        return;
    }

    uint16_t r = msg.getInt(0);
    uint16_t g = msg.getInt(1);
    uint16_t b = msg.getInt(2);
    uint32_t t = msg.getInt(3);

    pwmRgbAll(r, g, b, t);
}

// -----------------------------------------------------------------------------
// OSC ROUTER
// -----------------------------------------------------------------------------

void pwmOscRouter(OSCMessage &msg) {
    msg.dispatch("/pwmLed",     handlePwmLed);
    msg.dispatch("/pwmLedAll",  handlePwmLedAll);
    msg.dispatch("/pwmRgb",     handlePwmRgb);
    msg.dispatch("/pwmRgbAll",  handlePwmRgbAll);
}
