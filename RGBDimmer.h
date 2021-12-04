#ifndef RGB_DIMMER_H_
#define RGB_DIMMER_H_

#include <color_animation.h>
#include <millis_utils.h>
#include "rgb_dimmer_commands.h"

#define W_PREFS 0
#define C_PREFS 8

static const uint16_t tempsPref[] = {
    ColorTone::low_limit,
    ColorTone::match,
    ColorTone::candle,
    ColorTone::warm,
    ColorTone::neutral,
    ColorTone::flash,
    ColorTone::cold,
    ColorTone::blue_sky

};

const uint16_t colorsPref[] = {
    ColorTone::red,
    ColorTone::cyan,
    ColorTone::green,
    ColorTone::magenta,
    ColorTone::blue,
    ColorTone::yellow,
    ColorTone::warm,
    ColorTone::cold

};

// DIMMER
#define D_TEMP_GAP 100
#define D_MIN_TEMPERATURE 1000
#define D_MAX_TEMPERATURE 12000

#define D_DEFAULT_TEMPERATURE 2400

// #define D_DEFAULT_SATURATION 255
#define D_DEFAULT_HUE 0

#define D_HUE_GAP 5
#define D_SAT_GAP 5
#define D_BRIGHTNESS_GAP 10

#define D_TRANSITION_TIME 200
#define D_FADE_OUT_TIME 500

class RGBDimmer
{

protected:
    // system
    byte mode;
    byte prevMode;

    byte status;

    // Brightness
    uint8_t brightness;
    uint8_t prevBrightness;
    TimedLinearInterpolation<uint8_t> brightTransition;

    // Temperature Mode
    uint16_t temperature;
    uint16_t prevTemp;
    TimedLinearInterpolation<uint16_t> tempTrans;

    // Color Mode
    uint16_t hue;
    uint16_t prevHue;
    TimedMinimunDistanceCircularInterpolation<uint16_t> hueTrans;

    uint16_t saturation;

    // Transition timers
    MillisTimer fadeOutTimer;
    MillisTimer modeTransitionTimer;

    // RGB Mode
    MillisTimer rgbModeTransitionTimer;
    RGBOutput rgbModeOutput;
    RGBOutput rgbModepPrevOutput;

    // Output and animation
    ColorAnimation c;

    // TEMPERATURE
    RGBOutput temperatureToRGB(uint16_t t);
    RGBOutput temperatureToRGB();
    uint16_t tempTransition();
    RGBOutput tempTransitionRGB();
    void resetTempTransition(uint16_t prev, uint16_t target);
    void resetTempTransition();

    RGBOutput hueToRGB(uint16_t h);
    RGBOutput hueToRGB();
    uint16_t hueTransition();
    RGBOutput hueTranstionRGB();
    void resetHueTransition(uint16_t prev, uint16_t target);
    void resetHueTransition();

    RGBOutput modeTranstionOutput(uint8_t progress);

    uint16_t brightnessTransition();
    void resetBrightnessTranstion();
    void resetBrightnessTranstion(uint8_t prev, uint8_t target);

    RGBOutput rgbModeTransition();

    RGBOutput outputTone();
    uint16_t outputBrightness();

    void update();
    bool canSetTone();

public:
    static const uint8_t MODE_TEMPERATURE = 1;
    static const uint8_t MODE_COLOR = 2;
    static const uint8_t MODE_RGB = 3;

    static const uint8_t STATUS_ON = 1;
    static const uint8_t STATUS_OFF = 2;

    static const uint8_t TRANSITION_MODE_TONE = 1;
    static const uint8_t TRANSITION_MODE_RGB = 2;

    static const uint16_t TEMPERATURE_MIN = D_MIN_TEMPERATURE;
    static const uint16_t TEMPERATURE_MAX = D_MAX_TEMPERATURE;

#ifndef HUE_MAX
    static const uint16_t HUE_MAX = 359;
#endif
#ifndef SATURATION_MAX
    static const uint8_t SATURATION_MAX = 255;
#endif
#ifndef BRIGHTNESS_MAX
    static const uint8_t BRIGHTNESS_MAX = 255;
#endif

    bool setStatusOnToneSet = false;
    bool transitionsEnabled = true;
    bool toneTransitionsEnabled = true;
    bool brightnessTransitionsEnabled = true;
    uint8_t hueTransitionMode = TRANSITION_MODE_RGB;
    uint16_t fadeOutTime = D_FADE_OUT_TIME;
    uint16_t toneTransitionTime = D_TRANSITION_TIME;
    uint16_t brightnessTransitionTime = D_TRANSITION_TIME;

    // uint8_t tempTransitionMode = TRANSITION_MODE_RGB;

    RGBDimmer();

    bool isOn();
    bool isOff();

    byte getMode();
    RGBOutput getRGB();

    void debug();

    void setTemperature(uint16_t t);
    void setHue(uint16_t h);

    void setTone(uint16_t tone);
    void toneUp();
    void toneDown();

    void setRGB(uint8_t r, uint8_t g, uint8_t b);
    void setRGB(const uint8_t *rgb);
    void setRGB(uint8_t *rgb);

    void setAnimation(uint16_t a);

    void displayPref(uint8_t n);
    
    void setStatus(byte s);
    void on();
    void off();
    void changeStatus();
    void setMode(uint8_t m);
    void setModeToColor();
    void setModeToTemperature();
    void changeMode();

    void setBrightness(uint8_t b);
    void dimBrightness(int v);
    void brightnessUp();
    void brightnessDown();

    void setSaturation(uint8_t s);
    void dimSaturation(int v);
    void saturationUp();
    void saturationDown();

    void commandInput(unsigned int command);
    void newCommandInput(uint8_t command);
};

#endif