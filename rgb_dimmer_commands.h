#ifndef RGB_DIMMER_COMMANDS_H_
#define RGB_DIMMER_COMMANDS_H_


struct RGBDimmerCommand
{
    static const uint8_t NONE = 0;
    static const uint8_t ON = 1;
    static const uint8_t OFF = 2;
    static const uint8_t ON_OR_OFF = 3;
    static const uint8_t ON_OR_CHANGE_TONE_MODE = 4;
    static const uint8_t COLOR_MODE = 5;
    static const uint8_t TEMPERATURE_MODE = 6;
    static const uint8_t CHANGE_TONE_MODE = 7;
    static const uint8_t BRIGHTHNES_UP = 8;
    static const uint8_t BRIGHTHNES_DOWN = 9;
    static const uint8_t MAX_BRIGHTNESS = 10;
    static const uint8_t MIN_BRIGHNTESS = 11;
    static const uint8_t TONE_UP = 12;
    static const uint8_t TONE_DOWN = 13;
    static const uint8_t MAX_TONE = 14;
    static const uint8_t MIN_TONE = 15;

    static const uint8_t SATURATION_UP = 16;
    static const uint8_t SATURATION_DOWN = 17;
    static const uint8_t MIN_SATURATION = 18;
    static const uint8_t MAX_SATURATION = 19;

    static const uint8_t ENABLE_TRANSITIONS = 20;
    static const uint8_t DISABLE_TRANSITIONS = 21;

    static const uint8_t ANIMATION_PLAY = 22;
    static const uint8_t ANIMATION_PAUSE = 23;
    static const uint8_t ANIMATION_STOP = 24;
    static const uint8_t ANIMATION_SPEED_UP = 25;
    static const uint8_t ANIMATION_SPEED_DOWN = 26;
    static const uint8_t ANIMATION_BEAT = 27;
    static const uint8_t ANIMATION_RAINBOW = 28;
    static const uint8_t ANIMATION_FAST_RAINBOW = 29;

    static const uint8_t PRESET_0 = 30;
    static const uint8_t PRESET_1 = 31;
    static const uint8_t PRESET_2 = 32;
    static const uint8_t PRESET_3 = 33;
    static const uint8_t PRESET_4 = 34;
    static const uint8_t PRESET_5 = 35;
    static const uint8_t PRESET_6 = 36;
    static const uint8_t PRESET_7 = 37;
    static const uint8_t PRESET_8 = 38;
    static const uint8_t PRESET_9 = 39;
    static const uint8_t PRESET_10 = 40;
    static const uint8_t PRESET_11 = 41;
    static const uint8_t PRESET_12 = 42;
};


#endif