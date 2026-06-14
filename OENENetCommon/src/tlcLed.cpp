#include "tlcLed.h"
#include <Adafruit_TLC59711.h>
#include <esp32core.h>
#include <OscUtils.h>
#include <math.h>

// -----------------------------------------------------------------------------
// CONFIGURATION
// -----------------------------------------------------------------------------

static int NUM_RGB_LOCAL = 0;
static int NUM_LED_LOCAL = 0;

void tlcLedConfigure(int numRgb, int numLed) {
    NUM_RGB_LOCAL = numRgb;
    NUM_LED_LOCAL = numLed;
}

// -----------------------------------------------------------------------------
// CHANNEL MAPPING
// -----------------------------------------------------------------------------

static int tlcRgbIndexLocal[16][3];
static int tlcLedIndexLocal[32];

void tlcLedInitMapping() {
    int ch = 0;

    for (int i = 0; i < NUM_RGB_LOCAL; i++) {
        tlcRgbIndexLocal[i][0] = ch++;
        tlcRgbIndexLocal[i][1] = ch++;
        tlcRgbIndexLocal[i][2] = ch++;
    }

    for (int i = 0; i < NUM_LED_LOCAL; i++) {
        tlcLedIndexLocal[i] = ch++;
    }
}

// -----------------------------------------------------------------------------
// ERROR CALLBACK
// -----------------------------------------------------------------------------

static TlcErrorCallback _errorCb = nullptr;

void tlcLedSetErrorCallback(TlcErrorCallback cb) {
    _errorCb = cb;
}

static void raiseError(const char* code) {
    if (_errorCb)
        _errorCb(code);
}

// -----------------------------------------------------------------------------
// INTERNAL TLC STATE
// -----------------------------------------------------------------------------

static Adafruit_TLC59711* _tlc = nullptr;

static int _numDrivers = 0;
static int _channels = 0;

static uint16_t* _logical = nullptr;
static uint16_t gammaTable[4096];

struct TlcFadeState {
    bool active;
    uint16_t startLogical;
    uint16_t targetLogical;
    uint32_t startTime;
    uint32_t duration;
};

static TlcFadeState* _fade = nullptr;

// -----------------------------------------------------------------------------
// GAMMA TABLE
// -----------------------------------------------------------------------------

static void initGammaTable(float gamma) {
    for (int i = 0; i < 4096; i++) {
        float normalized = (float)i / 4095.0f;
        float corrected  = powf(normalized, gamma);
        gammaTable[i]    = (uint16_t)(corrected * 4095.0f);
    }
}

// -----------------------------------------------------------------------------
// COMPUTE CURRENT LOGICAL VALUE
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
// HARDWARE INITIALIZATION
// -----------------------------------------------------------------------------

void tlcLedInitHardware(int clkPin, int dataPin, int numDrivers) {
    _numDrivers = numDrivers;
    _channels   = numDrivers * 12;

    _tlc = new Adafruit_TLC59711(_numDrivers, clkPin, dataPin);

    _logical = new uint16_t[_channels];
    _fade    = new TlcFadeState[_channels];

    initGammaTable(2.2f);

    _tlc->begin();
    _tlc->write();

    for (int i = 0; i < _channels; i++) {
        _logical[i] = 0;
        _fade[i].active = false;
        _fade[i].startLogical = 0;
        _fade[i].targetLogical = 0;
        _fade[i].startTime = 0;
        _fade[i].duration = 0;
    }
}

// -----------------------------------------------------------------------------
// INTERNAL FADE ENGINE
// -----------------------------------------------------------------------------

static void tlcLedFadeInternal(int ch, uint16_t value, uint32_t timeMs) {
    if (ch < 0 || ch >= _channels)
        return;

    if (value > 4095)
        value = 4095;

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

// -----------------------------------------------------------------------------
// PUBLIC RGB API
// -----------------------------------------------------------------------------

void tlcRgbLedFade(int rgbIndex,
                   uint16_t r, uint16_t g, uint16_t b,
                   uint32_t timeMs)
{
    if (rgbIndex < 0 || rgbIndex >= NUM_RGB_LOCAL)
        return;

    if (r > 4095) r = 4095;
    if (g > 4095) g = 4095;
    if (b > 4095) b = 4095;

    int chR = tlcRgbIndexLocal[rgbIndex][0];
    int chG = tlcRgbIndexLocal[rgbIndex][1];
    int chB = tlcRgbIndexLocal[rgbIndex][2];

    tlcLedFadeInternal(chR, r, timeMs);
    tlcLedFadeInternal(chG, g, timeMs);
    tlcLedFadeInternal(chB, b, timeMs);
}

// -----------------------------------------------------------------------------
// OSC HANDLERS
// -----------------------------------------------------------------------------

static void handleTlcLed(OSCMessage &msg) {
    if (msg.size() < 3) return;

    int ledIndex = msg.getInt(0) - 1;
    uint16_t target = msg.getInt(1);
    uint32_t timeMs = msg.getInt(2);

    if (ledIndex < 0 || ledIndex >= NUM_LED_LOCAL) {
        raiseError("LED_INDEX_OUT_OF_RANGE");
        return;
    }

    int ch = tlcLedIndexLocal[ledIndex];
    tlcLedFadeInternal(ch, target, timeMs);
}

static void handleTlcLedAll(OSCMessage &msg) {
    if (msg.size() < 2) {
        raiseError("LEDALL_INVALID_ARGS");
        return;
    }

    if (NUM_LED_LOCAL == 0) {
        raiseError("NO_SINGLE_LEDS_DEFINED");
        return;
    }

    uint16_t target = msg.getInt(0);
    uint32_t timeMs = msg.getInt(1);

    for (int i = 0; i < NUM_LED_LOCAL; i++) {
        int ch = tlcLedIndexLocal[i];
        tlcLedFadeInternal(ch, target, timeMs);
    }
}

static void handleTlcRgb(OSCMessage &msg) {
    if (msg.size() < 5) return;

    int rgbIndex = msg.getInt(0) - 1;
    uint16_t r = msg.getInt(1);
    uint16_t g = msg.getInt(2);
    uint16_t b = msg.getInt(3);
    uint32_t t = msg.getInt(4);

    if (rgbIndex < 0 || rgbIndex >= NUM_RGB_LOCAL) {
        raiseError("RGB_INDEX_OUT_OF_RANGE");
        return;
    }

    tlcRgbLedFade(rgbIndex, r, g, b, t);
}

static void handleTlcRgbAll(OSCMessage &msg) {
    if (msg.size() < 4) {
        raiseError("RGBALL_INVALID_ARGS");
        return;
    }

    uint16_t r = msg.getInt(0);
    uint16_t g = msg.getInt(1);
    uint16_t b = msg.getInt(2);
    uint32_t t = msg.getInt(3);

    for (int i = 0; i < NUM_RGB_LOCAL; i++) {
        tlcRgbLedFade(i, r, g, b, t);
    }
}

// -----------------------------------------------------------------------------
// OSC ROUTER
// -----------------------------------------------------------------------------

void tlcOscRouter(OSCMessage &msg) {
    msg.dispatch("/tlcLed", handleTlcLed);
    msg.dispatch("/tlcLedAll", handleTlcLedAll);
    msg.dispatch("/tlcRgb", handleTlcRgb);
    msg.dispatch("/tlcRgbAll", handleTlcRgbAll);
}

// -----------------------------------------------------------------------------
// UPDATE
// -----------------------------------------------------------------------------

void tlcLedUpdate() {
    uint32_t now = millis();

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

    for (int ch = 0; ch < _channels; ch++) {
        uint16_t logical  = _logical[ch];
        uint16_t gamma12  = gammaTable[logical];
        uint16_t physical = (uint32_t)gamma12 * 16;

        _tlc->setPWM(ch, physical);
    }

    _tlc->write();
}
