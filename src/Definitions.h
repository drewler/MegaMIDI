#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#include <Arduino.h>

#define FW_VERSION "1.3"

#define TUNE -0.065 // Use this constant to tune your instrument!

// MIDI settings
#define MIDI_MFG_ID 0xFF
#define MIDI_DEVICE_ID 0x05

// LCD size
#define LCD_ROWS 4
#define LCD_COLS 20

#define NEXT 1
#define PREVIOUS 0
#define OCTAVE_SHIFT_RANGE 4
#define TRANSPOSE_SHIFT_RANGE 9
#define MAX_VOICES 32

enum OperationMode
{
  UNIFIED_MODE,
  SEPARATE_MODE,
  VST_MODE
};

typedef struct
{
  bool keyOn = false;
  bool sustained = false;
  uint8_t keyNumber = 0;
} SlotStatus;

enum SlotParameter
{
  MIDI_CHANNEL,
  OCTAVE_SHIFT,
  TRANSPOSE_SHIFT,
  VOICE_INDEX,
  NOISE
};

typedef struct
{
  uint8_t index;
  byte midiChannel = 1;
  signed char octaveShift = 0;
  signed char transposeShift = 0;
  SlotStatus status;
  uint8_t voiceIndex;
} YamahaSlot;

typedef struct
{
  uint8_t index;
  byte midiChannel = 2;
  signed char octaveShift = 0;
  signed char transposeShift = 0;
  SlotStatus status;
  byte noise;
} SNSlot;

// OPM File Format https://vgmrips.net/wiki/OPM_File_Format
typedef struct
{
  unsigned char LFO[5];
  unsigned char CH[7];
  unsigned char M1[11];
  unsigned char C1[11];
  unsigned char M2[11];
  unsigned char C2[11];
} Voice;

extern Voice voices[MAX_VOICES];
extern unsigned char maxValidVoices;

static const unsigned char leds[] = {1, 3, 4, 5, 6, 7, 24, 27};

#endif