// RF CONTROL
#define RF_RECEIVE_PIN 2
#define RF_REFRESH_MS 50

// REMOTE CONTROL
#define REMOTE_REFRESH_MS 10

#define DECODE_NEC 1
#define REMOTE_ADDRESS 0xEF00

#ifdef HAVE_HWSERIAL1
#define IR_RECEIVE_PIN 4
#else
#define IR_RECEIVE_PIN 8
#endif

// #define MODE_CHANGE_REFRESH_MS 500
#define BOUNCED_INPUT_MS 500
#include <IRremote.h>

#include <arduino_utils.h>
#include <millis_utils.h>

#include <RGBDimmer.h>
#include <rgb_leds.h>

#include <cross.h>
// LED PINS
#ifdef HAVE_HWSERIAL1
#define PIN_R 7
#define PIN_G 8
#define PIN_B 12
#else
#define PIN_R 6
#define PIN_G 9
#define PIN_B 10
#endif

CrossReceiver receiver;
MillisTimer rfInputTimer(RF_REFRESH_MS);

RGBDimmer rgbDimmer;
RGBLed led = RGBLed(PIN_R, PIN_G, PIN_B);

bool doDebug = false;

MillisTimer remoteInputTimer(REMOTE_REFRESH_MS, true);
MillisTimer bouncedInputTimer(BOUNCED_INPUT_MS);

int checkRemoteInput(RGBDimmer &dimmer)
{
    int input = -1;
    if (remoteInputTimer)
    {

        if (IrReceiver.decode())
        {
            IrReceiver.resume(); // Enable receiving of the next value

            if (IrReceiver.decodedIRData.address == REMOTE_ADDRESS)
            {
                uint16_t c = IrReceiver.decodedIRData.command;
                if (c == 3)
                {
                    if (bouncedInputTimer)
                    {
                        bouncedInputTimer.start();
                    }
                    else
                    {
                        c = -1;
                    }
                }

                input = c;
                // return c;
            }
        }

        // int input = checkRemoteInput();
        if (input >= 0)
        {
            dimmer.commandInput(input);
            doDebug = true;
        }
    }

    return input;
}

int checkSerialInput(RGBDimmer &dimmer, Stream &s)
{
    if (s.available())
    {

        // Serial.print(msgStartMarker);

        int c = s.read();
        if (isAscii(c))
        {
            c = toUpperCase(c);
            doDebug = true;
            if (c == 'C')
            {
                int n = s.parseInt();

                dimmer.commandInput(n);
                doDebug = true;
                return n;
            }
            else if (c == 'R')
            {
                RGBOutput o;
                o.red = s.parseInt();
                s.read();
                o.green = s.parseInt();
                s.read();
                o.blue = s.parseInt();
                dimmer.setRGB(o.red, o.green, o.blue);
                doDebug = true;
            }
            else if (c == 'H')
            {
                dimmer.setHue(s.parseInt());
                doDebug = true;
            }
            else if (c == 'T')
            {
                dimmer.setTone(s.parseInt());
                doDebug = true;
            }
            else if (c == 'K')
            {
                dimmer.setTemperature(s.parseInt());
                doDebug = true;
            }
            else if (c == 'B')
            {
                dimmer.setBrightness(s.parseInt());
                doDebug = true;
            }
            else if (c == 'S')
            {
                dimmer.setSaturation(s.parseInt());
                doDebug = true;
            }
            else if (c == 'd')
            {
                doDebug = true;
            }
            else
            {
                doDebug = false;
            }
        }
    }

    return -1;
}

void checkSerialInputs(RGBDimmer &dimmer)
{

#ifdef HAVE_HWSERIAL1
    checkSerialInput(dimmer, Serial1);
    // int c = checkSerialInput(dimmer, Serial1);
    // if (c > -1)
    //     return c;
#endif
    checkSerialInput(dimmer, Serial);
    // return checkSerialInput(dimmer, Serial);
}

void rfInput(RGBDimmer &dimmer)
{

    if (receiver.available())
    {

        if (!rfInputTimer.check())
            return;

        // receiver.message.debug(Serial);
        rfInputTimer.start();

        if (receiver.message.channel() == 1)
        {
            switch (receiver.message.command())
            {
            case CrossMessage::UP_PRESS_CODE:
                dimmer.brightnessUp();
                break;
            case CrossMessage::STOP_CODE:
                if (bouncedInputTimer)
                {
                    dimmer.changeStatus();
                    bouncedInputTimer.start();
                }

                break;
            case CrossMessage::DOWN_PRESS_CODE:
                dimmer.brightnessDown();
                break;

            default:
                break;
            }
        }
        else if (receiver.message.channel() == 2)
        {
            switch (receiver.message.command())
            {
            case CrossMessage::UP_PRESS_CODE:
                dimmer.toneUp();
                break;
            case CrossMessage::STOP_CODE:
                if (bouncedInputTimer)
                {
                    bouncedInputTimer.start();
                    if (dimmer.isOn())
                        dimmer.changeMode();
                    else
                        (dimmer.on());
                }

                break;
            case CrossMessage::DOWN_PRESS_CODE:
                dimmer.toneDown();
                break;

            default:
                break;
            }
        }
        else if (receiver.message.channel() == 3)
        {
            switch (receiver.message.command())
            {
            case CrossMessage::UP_PRESS_CODE:
                dimmer.setAnimation(AnimationFunctions::BEATING);
                break;
            case CrossMessage::STOP_CODE:
                if (dimmer.isOn())
                    dimmer.setAnimation(NO_ANIMATION);
                else
                    (dimmer.on());
                break;
            case CrossMessage::DOWN_PRESS_CODE:
                dimmer.setAnimation(AnimationFunctions::CIRCLE_RAINBOW);
                break;

            default:
                break;
            }
        }
    }
}

void setup()
{

    // serials
    Serial.begin(115200);
    Serial.println("{!!}");
#ifdef HAVE_HWSERIAL1
    Serial1.begin(115200);
#endif

    // ir remote
    IrReceiver.begin(IR_RECEIVE_PIN);
    remoteInputTimer.start();

    // rf control
    receiver.init(RF_RECEIVE_PIN);
    rfInputTimer.start();

    // rgbDimmer
    rgbDimmer.toneTransitionTime = 300;
    rgbDimmer.brightnessTransitionTime = 200;
    rgbDimmer.fadeOutTime = 200;

    bouncedInputTimer.start();

    // for serials
    delay(1000);
}

void loop()
{
    // RF
    rfInput(rgbDimmer);
    // IR
    checkRemoteInput(rgbDimmer);
    // Serials
    checkSerialInputs(rgbDimmer);

    RGBOutput o = rgbDimmer.getRGB();
    led.write(o.red, o.green, o.blue);

    if (doDebug)
    {
        rgbDimmer.debug();
        doDebug = false;
    }
}
