#include "RGBDimmer.h"
RGBOutput RGBDimmer::temperatureToRGB(uint16_t t)
{

    return RGBOutput::FROM_TEMPERATURE(t);
}

RGBOutput RGBDimmer::temperatureToRGB()
{
    return temperatureToRGB(temperature);
}

uint16_t RGBDimmer::tempTransition()
{

    return tempTrans.value(prevTemp, temperature, toneTransitionTime, true);
}

RGBOutput RGBDimmer::tempTransitionRGB()
{

    uint16_t t = tempTransition();
    if (transitionsEnabled && toneTransitionsEnabled && tempTrans.active)
        return RGBOutput::PROGRESSION(millis(), tempTrans.startTime, tempTrans.startTime + toneTransitionTime, temperatureToRGB(prevTemp), temperatureToRGB());
    return temperatureToRGB();
}

void RGBDimmer::resetTempTransition(uint16_t prev, uint16_t target)
{
    tempTrans.reset();
    prevTemp = prev;
    temperature = target;
}

void RGBDimmer::resetTempTransition()
{
    resetTempTransition(temperature, temperature);
}

RGBOutput RGBDimmer::hueToRGB(uint16_t h)
{
    return RGBOutput::FROM_HSV(h, saturation, BRIGHTNESS_MAX);
}

RGBOutput RGBDimmer::hueToRGB()
{
    return hueToRGB(hue);
}

uint16_t RGBDimmer::hueTransition()
{

    uint16_t o = hueTrans.value(prevHue, hue, HUE_MAX, toneTransitionTime, true);

    return o;
}

RGBOutput RGBDimmer::hueTranstionRGB()
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

void RGBDimmer::resetHueTransition(uint16_t prev, uint16_t target)
{

    prevHue = prev;
    hue = target;
    hueTrans.reset();
}

void RGBDimmer::resetHueTransition()
{
    resetHueTransition(hue, hue);
}

RGBOutput RGBDimmer::modeTranstionOutput(uint8_t progress)
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

uint16_t RGBDimmer::brightnessTransition()
{
    uint16_t b = brightTransition.value(prevBrightness, brightness, (unsigned long)brightnessTransitionTime, true);
    if (!transitionsEnabled || !brightnessTransitionsEnabled)
    {
        b = brightness;
    }

    return b;
}

void RGBDimmer::resetBrightnessTranstion()
{
    brightTransition.reset();
    prevBrightness = brightness;
}

void RGBDimmer::resetBrightnessTranstion(uint8_t prev, uint8_t target)
{
    prevBrightness = prev;
    brightness = target;
    brightTransition.reset();
}

// void resetAllTranstions()
// {
//     resetTempTransition();
//     resetHueTransition();
//     resetBrightnessTranstion();
// }

RGBOutput RGBDimmer::rgbModeTransition()
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

RGBOutput RGBDimmer::outputTone()
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

uint16_t RGBDimmer::outputBrightness()
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

void RGBDimmer::update()
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

bool RGBDimmer::canSetTone()
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
RGBDimmer::RGBDimmer()
{
    temperature = prevTemp = D_DEFAULT_TEMPERATURE;
    // prevTemp = temperature;

    brightness = prevBrightness = BRIGHTNESS_MAX;
    // prevBrightness = brightness;

    hue = prevHue = D_DEFAULT_HUE;
    // prevHue = hue;

    saturation = SATURATION_MAX;

    rgbModeOutput = RGBOutput::FROM_HSV(hue, saturation, brightness);
    rgbModepPrevOutput = rgbModeOutput;

    status = STATUS_OFF;
    mode = prevMode = MODE_COLOR;
}

bool RGBDimmer::isOn()
{
    return status == STATUS_ON;
}
bool RGBDimmer::isOff()
{
    return status == STATUS_OFF;
}

byte RGBDimmer::getMode()
{
    return mode;
}

RGBOutput RGBDimmer::getRGB()
{
    uint16_t b = outputBrightness();
    update();

    RGBOutput o = RGBOutput(c.red(), c.green(), c.blue(), b, BRIGHTNESS_MAX);

    return o;
}

void RGBDimmer::debug()
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

void RGBDimmer::setTemperature(uint16_t t)
{

    if (!canSetTone())
        return;

    if (mode != MODE_TEMPERATURE)
    {

        resetTempTransition(t, t);
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

void RGBDimmer::setHue(uint16_t h)
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

void RGBDimmer::setTone(uint16_t tone)
{
    if (tone <= HUE_MAX)
        setHue(tone);
    else
        setTemperature(tone);
}

void RGBDimmer::setRGB(uint8_t r, uint8_t g, uint8_t b)
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

void RGBDimmer::setRGB(const uint8_t *rgb)
{
    setRGB(rgb[0], rgb[1], rgb[2]);
}
void RGBDimmer::setRGB(uint8_t *rgb)
{
    RGBDimmer::setRGB((const uint8_t *)rgb);
}

void RGBDimmer::setAnimation(uint16_t a)
{
    if (isOff())
        return;

    if (a == NO_ANIMATION)
    {
        if (c.isHueAnimating())
        {
            resetHueTransition(c.hue(), c.hue());
        }

        // if (c.isBrightnessAnimating())
        // {
        //     resetBrightnessTranstion(c.brightness(), c.brightness());
        // }
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

void RGBDimmer::displayPref(uint8_t n)
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
void RGBDimmer::setStatus(byte s)
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

void RGBDimmer::on()
{
    setStatus(STATUS_ON);
}
void RGBDimmer::off()
{
    setStatus(STATUS_OFF);
}
void RGBDimmer::changeStatus()
{
    setStatus(status == STATUS_ON ? STATUS_OFF : STATUS_ON);
}

void RGBDimmer::setMode(uint8_t m)
{

    if ((m != mode) && (status == STATUS_ON) && ((m == MODE_COLOR) || (m == MODE_TEMPERATURE) || (m == MODE_RGB)) && (!c.isColorAnimating() || c.isBrightnessAnimating()))
    {

        modeTransitionTimer.start(toneTransitionTime);
        prevMode = mode;
        mode = m;
    }
}

void RGBDimmer::setModeToColor()
{
    setMode(MODE_COLOR);
}
void RGBDimmer::setModeToTemperature()
{
    setMode(MODE_TEMPERATURE);
}

void RGBDimmer::changeMode()
{
    if (mode == MODE_RGB)
        setModeToColor();
    else
        setMode((mode == MODE_TEMPERATURE ? MODE_COLOR : MODE_TEMPERATURE));
}

void RGBDimmer::setBrightness(uint8_t b)
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
void RGBDimmer::dimBrightness(int v)
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

void RGBDimmer::brightnessUp()
{
    dimBrightness(D_BRIGHTNESS_GAP);
}

void RGBDimmer::brightnessDown()
{
    dimBrightness(-D_BRIGHTNESS_GAP);
}

void RGBDimmer::toneUp()
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

void RGBDimmer::toneDown()
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

void RGBDimmer::setSaturation(uint8_t s)
{
    if (status == STATUS_ON && mode == MODE_COLOR)
        saturation = s % (SATURATION_MAX + 1);
}
void RGBDimmer::dimSaturation(int v)
{
    if (status == STATUS_ON && mode == MODE_COLOR)

        dimValue(saturation, v, (uint16_t)0, (uint16_t)SATURATION_MAX);
}
void RGBDimmer::saturationUp()

{
    dimSaturation(D_SAT_GAP);
}

void RGBDimmer::saturationDown()
{
    dimSaturation(-D_SAT_GAP);
}

void RGBDimmer::commandInput(unsigned int command)
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

void RGBDimmer::newCommandInput(uint8_t command)
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
