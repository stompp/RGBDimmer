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

#define D_TRANSITION_TIME 500
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
    RGBOutput temperatureToRGB(uint16_t t)
    {
        RGBOutput o = RGBOutput::FROM_TEMPERATURE(t);
        return o;
    }

    RGBOutput temperatureToRGB()
    {
        return temperatureToRGB(temperature);
    }



    RGBOutput hueToRGB(uint16_t h)
    {
        return RGBOutput::FROM_HSV(h, saturation, BRIGHTNESS_MAX);
    }
    
    RGBOutput hueToRGB()
    {
        return hueToRGB(hue);
    }

    RGBOutput modeTranstionOutput(uint8_t progress)
    {

        RGBOutput out;

        if (!transitionsEnabled || !toneTransitionsEnabled)
        {

            if (mode == MODE_COLOR)
                out = hueToRGB();
            else if (mode == MODE_TEMPERATURE)
                out = temperatureToRGB();
            else if (mode == MODE_RGB)
                out = rgbModeOutput;
        }
        else
        {

            if (mode == MODE_COLOR)
            {
                if (prevMode == MODE_TEMPERATURE)
                    out = RGBOutput::PROGRESSION100(progress, temperatureToRGB(), hueToRGB());
                else if (prevMode == MODE_RGB)
                    out = RGBOutput::PROGRESSION100(progress, rgbModeOutput, hueToRGB());
            }
            else if (mode == MODE_TEMPERATURE)
            {
                if (prevMode == MODE_COLOR)
                    out = RGBOutput::PROGRESSION100(progress, hueToRGB(), temperatureToRGB());
                else if (prevMode == MODE_RGB)
                    out = RGBOutput::PROGRESSION100(progress, rgbModeOutput, temperatureToRGB());
            }
            else if (mode == MODE_RGB)
            {
                if (prevMode == MODE_COLOR)
                    out = RGBOutput::PROGRESSION100(progress, hueToRGB(), rgbModeOutput);
                else if (prevMode == MODE_TEMPERATURE)
                    out = RGBOutput::PROGRESSION100(progress, temperatureToRGB(), rgbModeOutput);
            }
        }

        return out;
    }
   
   
    uint16_t hueTransition()
    {

        uint16_t o = hueTrans.value(prevHue, hue, HUE_MAX, toneTransitionTime, true);

        return o;
    }

    RGBOutput hueTranstionRGB()
    {

        uint16_t h = hueTransition();
        RGBOutput o;
        if (hueTransitionMode == TRANSITION_MODE_TONE)
            o = hueToRGB(h);
        else if (hueTransitionMode == TRANSITION_MODE_RGB)
            o = RGBOutput::PROGRESSION(millis(), hueTrans.startTime, hueTrans.startTime + toneTransitionTime, hueToRGB(prevHue), hueToRGB());

        if (transitionsEnabled && toneTransitionsEnabled)
            return o;

        return hueToRGB();
    }

    void resetHueTransition(uint16_t prev, uint16_t target)
    {
        hueTrans.reset();
        prevHue = prev;
        hue = target;
    }

    void resetHueTransition()
    {
        resetHueTransition(hue, hue);
    }



    uint16_t tempTransition()
    {

        return tempTrans.value(prevTemp, temperature, toneTransitionTime, true);
    }

    RGBOutput tempTransitionRGB()
    {

        uint16_t t = tempTransition();
        if (transitionsEnabled && toneTransitionsEnabled && tempTrans.active)
            return RGBOutput::PROGRESSION(millis(), tempTrans.startTime, tempTrans.startTime + toneTransitionTime, temperatureToRGB(prevTemp), temperatureToRGB());
        return temperatureToRGB();
    }

    void resetTempTransition(uint16_t prev, uint16_t target)
    {
        tempTrans.reset();
        prevTemp = prev;
        temperature = target;
    }

    void resetTempTransition()
    {
        resetTempTransition(temperature, temperature);
    }

    uint16_t brightnessTransition()
    {
        uint16_t b = brightTransition.value(prevBrightness, brightness, (unsigned long)brightnessTransitionTime, true);
        if (!transitionsEnabled || !brightnessTransitionsEnabled)
        {
            b = brightness;
        }

        return b;
    }

    void resetBrightnessTranstion()
    {
        brightTransition.reset();
        prevBrightness = brightness;
    }

    void resetBrightnessTranstion(uint8_t prev, uint8_t target)
    {
        prevBrightness = prev;
        brightness = target;
        brightTransition.reset();
    }
   
    void resetAllTranstions()
    {
        resetTempTransition();
        resetHueTransition();
        resetBrightnessTranstion();
    }

    RGBOutput rgbModeTransition()
    {
        rgbModeTransitionTimer.check();
        if (rgbModeTransitionTimer.isActive() && transitionsEnabled && toneTransitionsEnabled)
        {
            return RGBOutput::PROGRESSION100(rgbModeTransitionTimer.progress100(), rgbModepPrevOutput, rgbModeOutput);
        }
        else
        {
            rgbModepPrevOutput = rgbModeOutput;
        }
        return rgbModeOutput;
    }

    RGBOutput outputTone()
    {
        RGBOutput o;
        if (modeTransitionTimer.isActive())
        {
            modeTransitionTimer.check();
            o = modeTranstionOutput(modeTransitionTimer.progress100());
        }
        else if (mode == MODE_RGB)
        {

            o = rgbModeTransition();
        }
        else if (mode == MODE_TEMPERATURE)
        {

            o = tempTransitionRGB();
        }
        else if (mode == MODE_COLOR)
        {

            o = hueTranstionRGB();
        }

        return o;
    }

    uint16_t outputBrightness()
    {
        uint16_t b = 0;
        if (status == STATUS_ON)
        {
            b = brightnessTransition();
        }
        else if (status == STATUS_OFF)
        {

            if (fadeOutTimer.isActive())
            {

                fadeOutTimer.check();
                b = interpolate(fadeOutTimer.progress100(), 0UL, 100UL, (unsigned long)brightness, 0UL);
            }

            if (!transitionsEnabled || !brightnessTransitionsEnabled)
                return 0;
        }

        return b;
    }
   
    void update()
    {

        if (status == STATUS_OFF)
        {
            return;
        }

        if (c.isAnimating())
        {

            if ((c.isBrightnessAnimating()) && !c.isColorAnimating())
            {
                c.set(outputTone());
            }

            c.updateAnimation();
        }
        else
        {
            c.set(outputTone());
        }
    }

    bool canSetTone()
    {

        if (c.isColorAnimating())
        {
            return false;
        }

        if (isOff())
        {

            if (setStatusOnToneSet)
            {
                setStatus(STATUS_ON);
            }
        }

        return !c.isColorAnimating() && isOn();
    }

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

    RGBDimmer()
    {
        temperature = D_DEFAULT_TEMPERATURE;
        prevTemp = temperature;

        brightness = BRIGHTNESS_MAX;
        prevBrightness = brightness;

        hue = D_DEFAULT_HUE;
        prevHue = hue;

        saturation = SATURATION_MAX;

        rgbModeOutput = RGBOutput::FROM_HSV(hue, saturation, brightness);
        rgbModepPrevOutput = rgbModeOutput;

        status = STATUS_OFF;
        mode = prevMode = MODE_COLOR;
    }

    bool isOn()
    {
        return status == STATUS_ON;
    }
    bool isOff()
    {
        return status == STATUS_OFF;
    }

    byte getMode()
    {
        return mode;
    }

    RGBOutput getRGB()
    {
        uint16_t b = outputBrightness();
        update();

        RGBOutput o = RGBOutput(c.red(), c.green(), c.blue(), b, BRIGHTNESS_MAX);

        return o;
    }

    void debug()
    {

        RGBOutput o = getRGB();
        Color c = Color(o.red, o.green, o.blue);
        if (mode == MODE_TEMPERATURE)
        {
            debugValue("Temperature", temperature);
#ifdef HAVE_HWSERIAL1
            debugValue("Temperature", temperature, &Serial1);
#endif
        }
        // debugColor(k);

        char s[12];
        snprintf_P(s, 12, PSTR("%03d,%03d,%03d"), o.red, o.green, o.blue);
        debugValue("RGB", s);
#ifdef HAVE_HWSERIAL1
        debugValue("RGB", s, &Serial1);
#endif
        snprintf_P(s, 12, PSTR("%03d,%03d,%03d"), c.hue(), c.saturation(), c.brightness());
        debugValue("HSB", s);
#ifdef HAVE_HWSERIAL1
        debugValue("HSB", s, &Serial1);
#endif
    }

    void setTemperature(uint16_t t)
    {

        if (!canSetTone())
            return;

        if (mode != MODE_TEMPERATURE)
        {
          
            resetTempTransition(t,t);
            setMode(MODE_TEMPERATURE);
            return;
        }

        if (c.isAnimating())
        {
            if (t != temperature && tempTrans.active == false)
                resetTempTransition(temperature, t);

            if (c.isBrightnessAnimating())
                return;
        }
        else if (t != temperature && tempTrans.active == false)
        {
            resetTempTransition(temperature, t);
        }

     

        c.setColorAnimation(NO_ANIMATION);
      
    }

    void setHue(uint16_t h)
    {
        if (!canSetTone())
            return;

        uint16_t nh = hue_in_range(h);

        if (mode != MODE_COLOR)
        {
            resetHueTransition(nh, nh);
            setMode(MODE_COLOR);
            return;
        }

        if (c.isAnimating())
        {

            resetHueTransition(c.hue(), nh);
            if (c.isBrightnessAnimating())
                return;
        }
        else
        {

            if (nh != hue)
            {
                resetHueTransition(hue, nh);
            }
        }

        c.setColorAnimation(NO_ANIMATION);
    }

    void setTone(uint16_t tone)
    {
        if (tone <= HUE_MAX)
            setHue(tone);
        else
            setTemperature(tone);
    }

    void setRGB(uint8_t r, uint8_t g, uint8_t b)
    {

        if (!canSetTone())
            return;

        if (mode != MODE_RGB)
        {
            setMode(MODE_RGB);
            rgbModeOutput.set(r, g, b);
            brightness = rgbModeOutput.brightness();
            rgbModeOutput.maximize();
            rgbModepPrevOutput = rgbModeOutput;
            rgbModeTransitionTimer.stop();
        }
        else
        {

            RGBOutput no = RGBOutput(r, g, b);
            uint8_t nb = no.brightness();
            no.maximize();
            if (no != rgbModeOutput)
            {
                rgbModepPrevOutput = rgbModeTransition();
                rgbModeOutput = no;
                brightness = nb;
                rgbModeTransitionTimer.start(toneTransitionTime);
            }
        }
    }

    void setRGB(const uint8_t *rgb)
    {
        setRGB(rgb[0], rgb[1], rgb[2]);
    }
    void setRGB(uint8_t *rgb)
    {
        setRGB((const uint8_t *)rgb);
    }

    void setAnimation(uint16_t a)
    {
        if (isOff())
            return;

        if (a == NO_ANIMATION)
        {
            if (c.isHueAnimating())
            {
                resetHueTransition(c.hue(), c.hue());
            }

            if(c.isBrightnessAnimating()){
                resetBrightnessTranstion(c.brightness(),c.brightness());
            }
            c.setAnimation(a);
        }
        else
        {
            if ((mode == MODE_COLOR) || (a == AnimationFunctions::BEATING))
            {
                c.setAnimation(a);
            }
        }

        // update();
    }

    void displayPref(uint8_t n)
    {

        // if (status == STATUS_OFF)
        //     return;

        if (mode == MODE_TEMPERATURE || mode == MODE_RGB)
        {

            if (n < W_PREFS)
            {

                // on();

                setTone(tempsPref[n]);

                // setTemperature(tempsPref[n]);
                // temperature = tempsPref[n];
                // setTemperature();
            }
            else if (n < C_PREFS)
            {
                // on();
                // setHue(colorsPref[n]);
                setTone(colorsPref[n]);
                // hue = colorsPref[n];
                // setHue();
            }
        }
        else if (mode == MODE_COLOR || mode == MODE_RGB)
        {

            if (n < C_PREFS)
            {
                // on();
                // setHue(colorsPref[n]);
                setTone(colorsPref[n]);
                // hue = colorsPref[n];
                // setHue();
            }
        }
    }
    void setStatus(byte s)
    {

        // if ((status == s) || (s != STATUS_ON) || (s != STATUS_OFF))
        //     return;

        if (status == s)
            return;

        if (s == STATUS_ON)
        {
            prevBrightness = 0;
            brightTransition.active = false;

            if (brightness == 0)
            {
                brightness = D_BRIGHTNESS_GAP;
                // setBrightness(D_BRIGHTNESS_GAP);
            }

            // if (mode == MODE_TEMPERATURE)
            //     setTemperature();
            // else if (mode == MODE_COLOR)
            //     setHue();
        }
        else
        {
            fadeOutTimer.start(fadeOutTime);
        }
        status = s;
    }

    void on()
    {
        setStatus(STATUS_ON);
    }
    void off()
    {
        setStatus(STATUS_OFF);
    }
    void changeStatus()
    {
        setStatus(status == STATUS_ON ? STATUS_OFF : STATUS_ON);
    }

    void setMode(uint8_t m)
    {

        if ((m != mode) && (status == STATUS_ON) && ((m == MODE_COLOR) || (m == MODE_TEMPERATURE) || (m == MODE_RGB)) && (!c.isColorAnimating() || c.isBrightnessAnimating()))
        {

            modeTransitionTimer.start(toneTransitionTime);
            prevMode = mode;
            mode = m;
        }
    }

    void setModeToColor()
    {
        setMode(MODE_COLOR);
    }
    void setModeToTemperature()
    {
        setMode(MODE_TEMPERATURE);
    }

    void changeMode()
    {
        if (mode == MODE_RGB)
            setModeToColor();
        else
            setMode((mode == MODE_TEMPERATURE ? MODE_COLOR : MODE_TEMPERATURE));
    }

    void setBrightness(uint8_t b)
    {
        // uint16_t b = constrain(br, 0, BRIGHTNESS_MAX);

        if (b == 0)
        {
            setStatus(STATUS_OFF);
        }
        else
        {
            if (status == STATUS_OFF)
            {
                
                status = STATUS_ON;
                brightness = 0;
                brightTransition.active = false;
            }
        }

        if (b != brightness)
        {

            if (brightTransition.active)
            {
                prevBrightness = brightnessTransition();
            }
            else
            {
                prevBrightness = brightness;
            }
            brightTransition.active = false;
            brightness = b;
        }
    }
    void dimBrightness(int v)
    {

        bool doIt = false;
        if (v > 0)
        {
            if (status == STATUS_ON)
                doIt = true;
            else
            {
                setBrightness(D_BRIGHTNESS_GAP);
            }
        }
        else if (v < 0)
        {
            doIt = (status == STATUS_ON);
        }

        if (doIt)
            setBrightness(dimValue(brightness, v, (uint8_t)D_BRIGHTNESS_GAP, (uint8_t)BRIGHTNESS_MAX, false));
    }

    void brightnessUp()
    {
        dimBrightness(D_BRIGHTNESS_GAP);
    }

    void brightnessDown()
    {
        dimBrightness(-D_BRIGHTNESS_GAP);
    }

    void toneUp()
    {
        if (status == STATUS_OFF)
            return;

        if (mode == MODE_TEMPERATURE)
        {
            //   resetTempTransition();
            uint16_t t = dimValue(temperature, D_TEMP_GAP, TEMPERATURE_MIN, TEMPERATURE_MAX, false);
            setTemperature(t);
            // debug();
        }
        else if (mode == MODE_COLOR)
        {
            if (hue == HUE_MAX)
                hue = 0;
            else
                dimValue(hue, D_HUE_GAP, (uint16_t)0, (uint16_t)HUE_MAX);
        }
    }

    void toneDown()
    {
        if (status == STATUS_OFF)
            return;

        if (mode == MODE_TEMPERATURE)
        {
            // resetTempTransition();
            uint16_t t = dimValue(temperature, (int)(-D_TEMP_GAP), (uint16_t)TEMPERATURE_MIN, (uint16_t)TEMPERATURE_MAX, false);
            setTemperature(t);
            // debug();
        }
        else if (mode == MODE_COLOR)
        {

            if (hue == 0)
                hue = HUE_MAX;
            else
                dimValue(hue, -D_HUE_GAP, (uint16_t)0, (uint16_t)HUE_MAX);
           
        }
    }

    void setSaturation(uint8_t s)
    {
        if (status == STATUS_ON && mode == MODE_COLOR)
            saturation = s % (SATURATION_MAX + 1);
    }
    void dimSaturation(int v)
    {
        if (status == STATUS_ON && mode == MODE_COLOR)

            dimValue(saturation, v, (uint16_t)0, (uint16_t)SATURATION_MAX);
    }
    void saturationUp()

    {
        dimSaturation(D_SAT_GAP);
    }

    void saturationDown()
    {
        dimSaturation(-D_SAT_GAP);
    }

    void commandInput(unsigned int command)
    {

        switch (command)
        {

        // 0 - Brightness UP
        case 0:
            brightnessUp();
            break;

        // Brightness DOWN
        case 1:
            brightnessDown();
            break;

        // OFF
        case 2:
            off();

            break;
        // ON / CHANGE WHITE MODE OR COLOR MODE
        case 3:

            if (status == STATUS_OFF)
            {

                on();
            }
            else if (status == STATUS_ON)
            {
                changeMode();
            }

            break;

        // TONE_UP
        case 4:
            toneUp();
            break;
        // TONE_DOWN
        case 5:

            toneDown();
            break;
        // CLEAR ANIMATION
        case 6:
            setAnimation(NO_ANIMATION);
            break;
        case 7:
            setAnimation(AnimationFunctions::BEATING);
            break;
        case 8:
            displayPref(0);
            break;
        case 9:
            displayPref(1);
            break;
        case 10:
            setAnimation(AnimationFunctions::RAINBOW1);
            break;
        case 11:
            setAnimation(AnimationFunctions::RAINBOW2);
            break;
        case 12:
            displayPref(2);
            break;
        case 13:
            displayPref(3);
            break;

        case 14:
            setAnimation(AnimationFunctions::CIRCLE_RAINBOW);

            break;
        case 15:
            setAnimation(AnimationFunctions::FAST_RAINBOW);

            break;
        case 16:
            displayPref(4);
            break;

        case 17:
            displayPref(5);
            break;

        case 18:
            setAnimation(AnimationFunctions::SINE_PULSE);

            break;

        case 19:
            saturation = SATURATION_MAX;
            brightness = BRIGHTNESS_MAX;

            break;

        case 20:
            displayPref(6);
            break;

        case 21:
            displayPref(7);
            break;

        case 22:
            saturationUp();

            break;

        case 23:
            saturationDown();

            break;

        default:
            break;
        }
    }

    void newCommandInput(uint8_t command)
    {

        switch (command)
        {
        case RGBDimmerCommand::ON:
            on();
            break;
        case RGBDimmerCommand::OFF:
            off();
            break;
        case RGBDimmerCommand::ON_OR_OFF:
            changeStatus();
            break;
        case RGBDimmerCommand::ON_OR_CHANGE_TONE_MODE:
            if (isOn())
                changeMode();
            else
                on();
            break;
        case RGBDimmerCommand::COLOR_MODE:
            setModeToColor();
            break;
        case RGBDimmerCommand::TEMPERATURE_MODE:
            setModeToTemperature();
            break;
        case RGBDimmerCommand::CHANGE_TONE_MODE:
            changeMode();
            break;
        case RGBDimmerCommand::BRIGHTHNES_UP:
            brightnessUp();
            break;
        case RGBDimmerCommand::BRIGHTHNES_DOWN:

            break;
        case RGBDimmerCommand::MAX_BRIGHTNESS:

            break;
        case RGBDimmerCommand::MIN_BRIGHNTESS:

            break;
        case RGBDimmerCommand::TONE_UP:

            break;
        case RGBDimmerCommand::TONE_DOWN:

            break;
        case RGBDimmerCommand::MAX_TONE:

            break;
        case RGBDimmerCommand::MIN_TONE:

            break;
        case RGBDimmerCommand::SATURATION_UP:

            break;
        case RGBDimmerCommand::SATURATION_DOWN:

            break;
        case RGBDimmerCommand::MIN_SATURATION:

            break;
        case RGBDimmerCommand::MAX_SATURATION:

            break;
        case RGBDimmerCommand::ENABLE_TRANSITIONS:

            break;
        case RGBDimmerCommand::DISABLE_TRANSITIONS:

            break;
        case RGBDimmerCommand::ANIMATION_PLAY:

            break;
        case RGBDimmerCommand::ANIMATION_PAUSE:

            break;
        case RGBDimmerCommand::ANIMATION_STOP:

            break;
        case RGBDimmerCommand::ANIMATION_SPEED_UP:

            break;
        case RGBDimmerCommand::ANIMATION_SPEED_DOWN:

            break;
        case RGBDimmerCommand::ANIMATION_BEAT:

            break;
        case RGBDimmerCommand::ANIMATION_RAINBOW:

            break;
        case RGBDimmerCommand::ANIMATION_FAST_RAINBOW:

            break;
        default:
            break;
        }
    }
};

#endif