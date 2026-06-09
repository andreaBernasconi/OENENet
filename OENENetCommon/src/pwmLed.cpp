#include "pwmLed.h"
#include <math.h>

// -----------------------------------------------------------------------------
// Internal state (private)
// -----------------------------------------------------------------------------

static const int* _pins = nullptr;   // Pointer to pin array provided by the sketch
static int _count = 0;               // Number of PWM channels

static uint16_t gammaTable[4096];    // Gamma correction lookup table

// Fade state for each LED
struct PwmFadeState {
    uint16_t startDuty;
    uint16_t targetDuty;
    uint32_t startTime;
    uint32_t duration;
    bool active;
};

static PwmFadeState* fadeState = nullptr;

// -----------------------------------------------------------------------------
// Gamma table generation
// -----------------------------------------------------------------------------
static void initGammaTable(float gamma) {
    for (int i = 0; i < 4096; i++) {
        float normalized = (float)i / 4095.0f;
        float corrected = powf(normalized, gamma);
        gammaTable[i] = (uint16_t)(corrected * 4095.0f);
    }
}

// -----------------------------------------------------------------------------
// Internal helper: compute current interpolated duty
// -----------------------------------------------------------------------------
static uint16_t getCurrentLogicalDuty(int ledIndex) {
    PwmFadeState &f = fadeState[ledIndex];

    if (!f.active)
        return f.targetDuty;

    uint32_t now = millis();
    uint32_t elapsed = now - f.startTime;

    if (elapsed >= f.duration)
        return f.targetDuty;

    int32_t start = f.startDuty;
    int32_t target = f.targetDuty;
    int32_t delta = target - start;

    int64_t num = (int64_t)delta * (int64_t)elapsed;
    int32_t interp = start + (int32_t)(num / (int64_t)f.duration);

    if (interp < 0) interp = 0;
    if (interp > 4095) interp = 4095;

    return (uint16_t)interp;
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
        // Attach LEDC channel with 5 kHz and 12-bit resolution
        ledcAttach(_pins[i], 5000, 12);
        ledcWrite(_pins[i], 0);

        fadeState[i].active = false;
        fadeState[i].startDuty = 0;
        fadeState[i].targetDuty = 0;
        fadeState[i].startTime = 0;
        fadeState[i].duration = 0;
    }
}

void pwmLedFade(int ledIndex, uint16_t targetDuty, uint32_t durationMs) {
    if (ledIndex < 0 || ledIndex >= _count)
        return;

    if (targetDuty > 4095)
        targetDuty = 4095;

    uint16_t current = getCurrentLogicalDuty(ledIndex);

    PwmFadeState &f = fadeState[ledIndex];
    f.startDuty = current;
    f.targetDuty = targetDuty;
    f.startTime = millis();
    f.duration = durationMs;
    f.active = true;
}

void pwmLedUpdate() {
    for (int i = 0; i < _count; i++) {
        PwmFadeState &f = fadeState[i];
        if (!f.active)
            continue;

        uint32_t now = millis();
        uint32_t elapsed = now - f.startTime;

        if (elapsed >= f.duration) {
            uint16_t duty = f.targetDuty;
            ledcWrite(_pins[i], gammaTable[duty]);
            f.active = false;
            continue;
        }

        int32_t start = f.startDuty;
        int32_t target = f.targetDuty;
        int32_t delta = target - start;

        int64_t num = (int64_t)delta * (int64_t)elapsed;
        int32_t interp = start + (int32_t)(num / (int64_t)f.duration);

        if (interp < 0) interp = 0;
        if (interp > 4095) interp = 4095;

        uint16_t currentDuty = (uint16_t)interp;
        ledcWrite(_pins[i], gammaTable[currentDuty]);
    }
}
