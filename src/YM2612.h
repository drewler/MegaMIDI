#ifndef YM2612_H_
#define YM2612_H_
#include <Arduino.h>

#include "Definitions.h"

#define mask(s) (~(~0 << s))
#define MAX_CHANNELS_YM 6

class YM2612
{
private:
  uint8_t _IC = 10;
  uint8_t _CS = 11;
  uint8_t _WR = 12;
  uint8_t _RD = 13;
  uint8_t _A0 = 14;
  uint8_t _A1 = 15;
  uint8_t lfoFrq = 0;
  uint8_t lfoSens = 7;
  int8_t octaveShift = 0;
  unsigned char bank0[0xB7 - 0x21]; //Shadow registers
  unsigned char bank1[0xB7 - 0x30];
  Voice currentVoice;

public:
  YM2612();
  YamahaSlot slots[MAX_CHANNELS_YM];
  OperationMode mode;
  bool sustainEnabled = false;
  bool velocityEnabled = false;
  unsigned char pitchBendYMRange = 2;
  int pitchBendYM = 2;
  bool lfoOn = false;
  void SetChannelOn(uint8_t key, uint8_t velocity, uint8_t slot);
  void SetChannelOff(uint8_t key, uint8_t slot);
  void SetVoice(Voice v);
  void ChangeSlotParam(uint8_t slotIndex, SlotParameter parameter, uint8_t direction);

  float NoteToFrequency(uint8_t note);
  void SetFrequency(uint16_t frequency, uint8_t channel);
  void AdjustLFO(uint8_t value);
  void AdjustPitch(uint8_t channel, int pitch);
  void ReleaseSustainedKeys();
  void ClampSustainedKeys();
  uint16_t CalcFNumber(float note);
  void ToggleLFO();
  void Reset();
  void send(unsigned char addr, unsigned char data, bool setA1 = 0);
  uint8_t GetShadowValue(uint8_t addr, bool bank);

  // Manual register setting for MIDI exposure
  void SetTL(uint8_t slot, uint8_t op, uint8_t value);
  void SetAR(uint8_t slot, uint8_t op, uint8_t value);
  void SetD1R(uint8_t slot, uint8_t op, uint8_t value);
  void SetD1L(uint8_t slot, uint8_t op, uint8_t value);
  void SetD2R(uint8_t slot, uint8_t op, uint8_t value);
  void SetRR(uint8_t slot, uint8_t op, uint8_t value);
  void SetDetune(uint8_t slot, uint8_t op, uint8_t value);
  void SetMult(uint8_t slot, uint8_t op, uint8_t value);
  void SetRateScaling(uint8_t slot, uint8_t op, uint8_t value);
  void SetAmplitudeModulation(uint8_t slot, uint8_t op, bool value);
  void SetVoiceManual(uint8_t slot, Voice v);

  // Globals
  void SetLFOEnabled(bool value);
  void SetLFOFreq(bool value);
  void SetFreqModSens(uint8_t slot, uint8_t value);
  void SetAlgo(uint8_t slot, uint8_t value);
  void SetAMSens(uint8_t slot, uint8_t value);
  void SetFMFeedback(uint8_t slot, uint8_t value);
};

#endif

// Notes
// DIGITAL BUS = PF0-PF7
// IC = PC0/10
// CS = PC1/11
// WR = PC2/12
// RD = PC3/13
// A0 = PC4/14
// A1 = PC5/15