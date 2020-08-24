#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "Voice.h"

#define MIDI_MFG_ID 0xFF
#define MIDI_DEVICE_ID 0x05

enum screens { MAIN_MENU, YAMAHA_SETTINGS, YAMAHA_SLOTS, YAMAHA_SLOT, SN_SETTINGS, SN_SLOTS, SN_SLOT };
enum instrumentMode { UNIFIED, SEPARATE };

static const unsigned char leds[] = {1, 3, 4, 5, 6, 7, 24, 27};
extern bool YMsustainEnabled;
extern bool PSGsustainEnabled;
extern uint8_t yamahaMode = UNIFIED;
extern uint8_t snMode = UNIFIED;

extern bool redrawLCDOnNextLoop;
extern uint8_t currentScreen;

typedef struct 
{
    Voice v;
    unsigned char index = 0xFF; //Use this as a simple way to check EEPROM for valid saved voice
    char fileName[20+1]; //+1 for null terminator
    unsigned char voiceNumber = 0;
    signed char octaveShift = 0;
} FavoriteVoice;

enum OperationMode
{
    STANDALONE, VST
};

static OperationMode operationMode = STANDALONE;

#endif