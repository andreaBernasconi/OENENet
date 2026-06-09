#include "tlcLed.h"
#include <Adafruit_TLC59711.h>
#include <math.h>

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------

static Adafruit_TLC59711* _tlc = nullptr;

static int _numDrivers = 0;
static int _channels = 0;

static uint16_t* _logical = nullptr;     // logical values 0–4095
static uint16_t gammaTable[4096];        // gamma correction table

struct TlcFadeState {
    bool active;
    uint16_t startLogical;
    uint16_t targetLogical;
    uint32_t startTime;
    uint32_t duration;
};

static TlcFadeState* _fade = nullptr;

// -----------------------------------------------------------------------------
// Gamma table generation
// -----------------------------------------------------------------------------

static void initGammaTable(float gamma) {
    for (int i = 0; i < 4096; i++) {
        float normalized = (float)i / 4095.0f;
        float corrected  = powf(normalized, gamma);
        gammaTable[i]    = (uint16_t)(corrected * 4095.0f);
    }
}

// -----------------------------------------------------------------------------
// Compute current logical value (used to avoid jumps when a new fade starts)
// -----------------------------------------------------------------------------

static uint16_t computeCurrentLogical(int ch) {
    TlcFadeState &f = _fade[ch];

    if (!f.active)
        return _logical[ch];

    uint32_t now = millis();
    uint32_t elapsed = now - f.startTime;

    if (elapsed >= f.duration)
        return f.targetLogical;

    int32_t start  = f.startLogical;
    int32_t target = f.targetLogical;
    int32_t delta  = target - start;

    int64_t num    = (int64_t)delta * (int64_t)elapsed;
    int32_t interp = start + (int32_t)(num / (int64_t)f.duration);

    if (interp < 0)    interp = 0;
    if (interp > 4095) interp = 4095;

    return (uint16_t)interp;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void tlcLedInit(int clkPin, int dataPin, int numDrivers) {
    _numDrivers = numDrivers;
    _channels   = numDrivers * 12;

    _tlc = new Adafruit_TLC59711(_numDrivers, clkPin, dataPin);

    _logical = new uint16_t[_channels];
    _fade    = new TlcFadeState[_channels];

    initGammaTable(2.2f);

    _tlc->begin();
    _tlc->write();   // clear all outputs

    for (int i = 0; i < _channels; i++) {
        _logical[i] = 0;
        _fade[i].active = false;
        _fade[i].startLogical = 0;
        _fade[i].targetLogical = 0;
        _fade[i].startTime = 0;
        _fade[i].duration = 0;
    }
}

void tlcLedFade(int ch, uint16_t value, uint32_t timeMs) {
    if (ch < 0 || ch >= _channels)
        return;

    if (value > 4095)
        value = 4095;

    // Update current logical value before starting a new fade
    _logical[ch] = computeCurrentLogical(ch);

    TlcFadeState &f = _fade[ch];
    f.active        = true;
    f.startLogical  = _logical[ch];
    f.targetLogical = value;
    f.startTime     = millis();
    f.duration      = timeMs;

    if (timeMs == 0) {
        _logical[ch] = value;
        f.active = false;
    }
}

void tlcLedFadeAll(uint16_t value, uint32_t timeMs) {
    if (value > 4095)
        value = 4095;

    uint32_t now = millis();

    for (int ch = 0; ch < _channels; ch++) {
        // Update current logical value before starting a new fade
        _logical[ch] = computeCurrentLogical(ch);

        TlcFadeState &f = _fade[ch];
        f.active        = true;
        f.startLogical  = _logical[ch];
        f.targetLogical = value;
        f.startTime     = now;
        f.duration      = timeMs;

        if (timeMs == 0) {
            _logical[ch] = value;
            f.active = false;
        }
    }
}

void tlcLedUpdate() {
    uint32_t now = millis();

    // 1) Update logical values
    for (int ch = 0; ch < _channels; ch++) {
        TlcFadeState &f = _fade[ch];
        if (!f.active)
            continue;

        uint32_t elapsed = now - f.startTime;

        if (elapsed >= f.duration) {
            _logical[ch] = f.targetLogical;
            f.active = false;
            continue;
        }

        int32_t start  = f.startLogical;
        int32_t target = f.targetLogical;
        int32_t delta  = target - start;

        int64_t num    = (int64_t)delta * (int64_t)elapsed;
        int32_t interp = start + (int32_t)(num / (int64_t)f.duration);

        if (interp < 0)    interp = 0;
        if (interp > 4095) interp = 4095;

        _logical[ch] = (uint16_t)interp;
    }

    // 2) Apply gamma and scale to 16‑bit physical output
    for (int ch = 0; ch < _channels; ch++) {
        uint16_t logical  = _logical[ch];
        uint16_t gamma12  = gammaTable[logical];
        uint16_t physical = (uint32_t)gamma12 * 16;   // 0–65520

        _tlc->setPWM(ch, physical);
    }

    // 3) Synchronized write
    _tlc->write();
}
