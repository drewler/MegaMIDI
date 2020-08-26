#include "YM2612.h"

YM2612::YM2612()
{
  mode = UNIFIED_MODE;
  DDRF = 0xFF;
  PORTF = 0x00;
  DDRC = 0xFF;
  PORTC |= 0x3C;                  // _A1 LOW, _A0 LOW, _IC HIGH, _WR HIGH, _RD HIGH, _CS HIGH
  memset(bank0, 0, sizeof bank0); // Reset shadow registers
  memset(bank1, 0, sizeof bank1);
  for (uint8_t i = 0; i < MAX_CHANNELS_YM; i++)
  {
    slots[i].index = i;
  }
}

void YM2612::ChangeSlotParam(uint8_t slotIndex, SlotParameter parameter, uint8_t direction)
{
  YamahaSlot slotBuffer;
  switch (parameter)
  {
  case VOICE_INDEX:
    if (direction == NEXT)
      slots[slotIndex].voiceIndex++;
    if (direction == PREVIOUS)
      slots[slotIndex].voiceIndex--;
    slots[slotIndex].voiceIndex = min(slots[slotIndex].voiceIndex, maxValidVoices);
    slots[slotIndex].voiceIndex = max(slots[slotIndex].voiceIndex, 0);
    break;
  case MIDI_CHANNEL:
    if (direction == NEXT)
      slots[slotIndex].midiChannel++;
    if (direction == PREVIOUS)
      slots[slotIndex].midiChannel--;
    slots[slotIndex].midiChannel = min(slots[slotIndex].midiChannel, 16);
    slots[slotIndex].midiChannel = max(slots[slotIndex].midiChannel, 1);
    break;
  case OCTAVE_SHIFT:
    if (direction == NEXT)
      slots[slotIndex].octaveShift++;
    if (direction == PREVIOUS)
      slots[slotIndex].octaveShift--;
    slots[slotIndex].octaveShift = min(slots[slotIndex].octaveShift, OCTAVE_SHIFT_RANGE);
    slots[slotIndex].octaveShift = max(slots[slotIndex].octaveShift, -OCTAVE_SHIFT_RANGE);
    break;
  case TRANSPOSE_SHIFT:
    if (direction == NEXT)
      slots[slotIndex].transposeShift++;
    if (direction == PREVIOUS)
      slots[slotIndex].transposeShift--;
    slots[slotIndex].transposeShift = min(slots[slotIndex].transposeShift, TRANSPOSE_SHIFT_RANGE);
    slots[slotIndex].transposeShift = max(slots[slotIndex].transposeShift, -TRANSPOSE_SHIFT_RANGE);
    break;
  }
  if (mode == UNIFIED_MODE)
  {
    memcpy(&slotBuffer, &slots[slotIndex], sizeof(YamahaSlot));
    for (uint8_t i = 0; i < MAX_CHANNELS_YM; i++)
    {
      memcpy(&slots[i], &slotBuffer, sizeof(YamahaSlot));
    }
  }
}

void YM2612::Reset()
{
  digitalWriteFast(_IC, LOW); // _IC HIGH
  delayMicroseconds(25);
  digitalWriteFast(_IC, HIGH); // _IC HIGH
  delayMicroseconds(25);
  memset(bank0, 0, sizeof bank0); // Reset shadow registers
  memset(bank1, 0, sizeof bank1);
}

void YM2612::send(unsigned char addr, unsigned char data, bool setA1)
{
  // Store in shadow registers to keep track of written values
  if (setA1)
  {
    bank1[addr - 0x30] = data;
  }
  else
  {
    bank0[addr - 0x21] = data;
  }

  digitalWriteFast(_A1, setA1);
  digitalWriteFast(_A0, LOW);
  digitalWriteFast(_CS, LOW);
  PORTF = addr;
  digitalWriteFast(_WR, LOW);
  delayMicroseconds(1);
  digitalWriteFast(_WR, HIGH);
  digitalWriteFast(_CS, HIGH);
  digitalWriteFast(_A0, HIGH);
  digitalWriteFast(_CS, LOW);
  PORTF = data;
  digitalWriteFast(_WR, LOW);
  delayMicroseconds(1);
  digitalWriteFast(_WR, HIGH);
  digitalWriteFast(_CS, HIGH);
  digitalWriteFast(_A0, LOW);
}

void YM2612::SetFrequency(uint16_t frequency, uint8_t channel)
{
  int block = 2;

  uint16_t frq;
  while (frequency >= 2048)
  {
    frequency /= 2;
    block++;
  }
  frq = (uint16_t)frequency;
  bool setA1 = channel > 2;
  send(0xA4 + channel % 3, ((frq >> 8) & mask(3)) | ((block & mask(3)) << 3), setA1);
  send(0xA0 + channel % 3, frq, setA1);
}

float YM2612::NoteToFrequency(uint8_t note)
{
  //Elegant note/freq system by diegodorado
  //Check out his project at https://github.com/diegodorado/arduinoProjects/tree/master/ym2612
  const static float freq[12] =
      {
          //You can create your own note frequencies here. C4#-C5. There should be twelve entries.
          //YM3438 datasheet note set
          277.2, 293.7, 311.1, 329.6, 349.2, 370.0, 392.0, 415.3, 440.0, 466.2, 493.9, 523.3

      };
  const static float multiplier[] =
      {
          0.03125f, 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f};
  float f = freq[note % 12];
  return (f + (f * TUNE)) * multiplier[(note / 12) + octaveShift];
}

void YM2612::SetChannelOn(uint8_t key, uint8_t velocity, uint8_t slot, bool velocityEnabled)
{
  uint8_t offset = slot % MAX_CHANNELS_YM;
  bool setA1 = false; // ???
  slots[offset].status.keyOn = true;
  slots[offset].status.keyNumber = key;
  slots[offset].status.sustained = false;
  send(0x28, 0xF0 + offset + (setA1 << 2));
  SetFrequency(NoteToFrequency(key), offset);
}

uint8_t YM2612::GetShadowValue(uint8_t addr, bool bank)
{
  return bank ? bank1[addr - 0x30] : bank0[addr - 0x21];
}

void YM2612::SetChannelOff(uint8_t key, uint8_t slot)
{
  uint8_t offset = slot % MAX_CHANNELS_YM;
  slots[offset].status.keyOn = false;
  bool setA1 = false; // ???
  send(0x28, 0x00 + offset + (setA1 << 2));
}

void YM2612::ReleaseSustainedKeys()
{
  for (int i = 0; i < MAX_CHANNELS_YM; i++)
  {
    if (slots[i].status.sustained && slots[i].status.keyOn)
    {
      slots[i].status.sustained = false;
      SetChannelOff(slots[i].status.keyNumber, i);
    }
  }
}

void YM2612::ClampSustainedKeys()
{
  for (int i = 0; i < MAX_CHANNELS_YM; i++)
  {
    if (!slots[i].status.sustained && slots[i].status.keyOn)
    {
      slots[i].status.sustained = true;
    }
  }
}

void YM2612::SetVoiceManual(uint8_t slot, Voice v)
{
  SetFMFeedback(slot, v.CH[1]);
  SetAlgo(slot, v.CH[2]);
  SetAMSens(slot, v.CH[3]);
  SetFreqModSens(slot, v.CH[4]);

  SetAR(slot, 0, v.M1[0]);
  SetD1R(slot, 0, v.M1[1]);
  SetD2R(slot, 0, v.M1[2]);
  SetRR(slot, 0, v.M1[3]);
  SetD1L(slot, 0, v.M1[4]);
  SetTL(slot, 0, v.M1[5]);
  SetRateScaling(slot, 0, v.M1[6]);
  SetMult(slot, 0, v.M1[7]);
  SetDetune(slot, 0, v.M1[8]);
  SetAmplitudeModulation(slot, 0, v.M1[10]);

  SetAR(slot, 1, v.C1[0]);
  SetD1R(slot, 1, v.C1[1]);
  SetD2R(slot, 1, v.C1[2]);
  SetRR(slot, 1, v.C1[3]);
  SetD1L(slot, 1, v.C1[4]);
  SetTL(slot, 1, v.C1[5]);
  SetRateScaling(slot, 1, v.C1[6]);
  SetMult(slot, 1, v.C1[7]);
  SetDetune(slot, 1, v.C1[8]);
  SetAmplitudeModulation(slot, 1, v.C1[10]);

  SetAR(slot, 2, v.M2[0]);
  SetD1R(slot, 2, v.M2[1]);
  SetD2R(slot, 2, v.M2[2]);
  SetRR(slot, 2, v.M2[3]);
  SetD1L(slot, 2, v.M2[4]);
  SetTL(slot, 2, v.M2[5]);
  SetRateScaling(slot, 2, v.M2[6]);
  SetMult(slot, 2, v.M2[7]);
  SetDetune(slot, 2, v.M2[8]);
  SetAmplitudeModulation(slot, 2, v.M2[10]);

  SetAR(slot, 3, v.C2[0]);
  SetD1R(slot, 3, v.C2[1]);
  SetD2R(slot, 3, v.C2[2]);
  SetRR(slot, 3, v.C2[3]);
  SetD1L(slot, 3, v.C2[4]);
  SetTL(slot, 3, v.C2[5]);
  SetRateScaling(slot, 3, v.C2[6]);
  SetMult(slot, 3, v.C2[7]);
  SetDetune(slot, 3, v.C2[8]);
  SetAmplitudeModulation(slot, 3, v.C2[10]);
}

void YM2612::SetVoice(Voice v)
{
  currentVoice = v;
  bool resetLFO = lfoOn;
  if (lfoOn)
    ToggleLFO();
  send(0x22, 0x00); // LFO off
  send(0x27, 0x00); // CH3 Normal
  send(0x28, 0x00); // Turn off all channels
  send(0x2B, 0x00); // DAC off

  for (int a1 = 0; a1 <= 1; a1++)
  {
    for (int i = 0; i < 3; i++)
    {
      uint8_t DT1MUL, TL, RSAR, AMD1R, D2R, D1LRR = 0;

      // Operator 1
      DT1MUL = (v.M1[8] << 4) | v.M1[7];
      TL = v.M1[5];
      RSAR = (v.M1[6] << 6) | v.M1[0];
      AMD1R = (v.M1[10] << 7) | v.M1[1];
      D2R = v.M1[2];
      D1LRR = (v.M1[4] << 4) | v.M1[3];

      send(0x30 + i, DT1MUL, a1); // DT1/Mul
      send(0x40 + i, TL, a1);     // Total Level
      send(0x50 + i, RSAR, a1);   // RS/AR
      send(0x60 + i, AMD1R, a1);  // AM/D1R
      send(0x70 + i, D2R, a1);    // D2R
      send(0x80 + i, D1LRR, a1);  // D1L/RR
      send(0x90 + i, 0x00, a1);   // SSG EG

      // Operator 2
      DT1MUL = (v.C1[8] << 4) | v.C1[7];
      TL = v.C1[5];
      RSAR = (v.C1[6] << 6) | v.C1[0];
      AMD1R = (v.C1[10] << 7) | v.C1[1];
      D2R = v.C1[2];
      D1LRR = (v.C1[4] << 4) | v.C1[3];
      send(0x34 + i, DT1MUL, a1); // DT1/Mul
      send(0x44 + i, TL, a1);     // Total Level
      send(0x54 + i, RSAR, a1);   // RS/AR
      send(0x64 + i, AMD1R, a1);  // AM/D1R
      send(0x74 + i, D2R, a1);    // D2R
      send(0x84 + i, D1LRR, a1);  // D1L/RR
      send(0x94 + i, 0x00, a1);   // SSG EG

      // Operator 3
      DT1MUL = (v.M2[8] << 4) | v.M2[7];
      TL = v.M2[5];
      RSAR = (v.M2[6] << 6) | v.M2[0];
      AMD1R = (v.M2[10] << 7) | v.M2[1];
      D2R = v.M2[2];
      D1LRR = (v.M2[4] << 4) | v.M2[3];
      send(0x38 + i, DT1MUL, a1); // DT1/Mul
      send(0x48 + i, TL, a1);     // Total Level
      send(0x58 + i, RSAR, a1);   // RS/AR
      send(0x68 + i, AMD1R, a1);  // AM/D1R
      send(0x78 + i, D2R, a1);    // D2R
      send(0x88 + i, D1LRR, a1);  // D1L/RR
      send(0x98 + i, 0x00, a1);   // SSG EG

      // Operator 4
      DT1MUL = (v.C2[8] << 4) | v.C2[7];
      TL = v.C2[5];
      RSAR = (v.C2[6] << 6) | v.C2[0];
      AMD1R = (v.C2[10] << 7) | v.C2[1];
      D2R = v.C2[2];
      D1LRR = (v.C2[4] << 4) | v.C2[3];
      send(0x3C + i, DT1MUL, a1); //DT1/Mul
      send(0x4C + i, TL, a1);     //Total Level
      send(0x5C + i, RSAR, a1);   //RS/AR
      send(0x6C + i, AMD1R, a1);  //AM/D1R
      send(0x7C + i, D2R, a1);    //D2R
      send(0x8C + i, D1LRR, a1);  //D1L/RR
      send(0x9C + i, 0x00, a1);   //SSG EG

      uint8_t FBALGO = (v.CH[1] << 3) | v.CH[2];
      send(0xB0 + i, FBALGO, a1); // Ch FB/Algo
      send(0xB4 + i, 0xC0, a1);   // Both Spks on

      send(0x28, 0x00 + i + (a1 << 2)); //Keys off
    }
  }
  if (resetLFO)
    ToggleLFO();
}

void YM2612::AdjustLFO(uint8_t value)
{
  lfoFrq = map(value, 0, 127, 0, 7);
  if (lfoOn)
  {
    uint8_t lfo = (1 << 3) | lfoFrq;
    send(0x22, lfo);
    if (lfoSens > 7)
    {
      Serial.println("LFO Sensitivity out of range! (Must be 0-7)");
      return;
    }
    uint8_t lrAmsFms = 0xC0 + (3 << 4);
    lrAmsFms |= lfoSens;
    for (int a1 = 0; a1 <= 1; a1++)
    {
      for (int i = 0; i < 3; i++)
      {
        send(0xB4 + i, lrAmsFms, a1); // Speaker and LMS
      }
    }
  }
}

void YM2612::AdjustPitch(uint8_t channel, int pitch)
{
  float freqFrom = NoteToFrequency(slots[channel].status.keyNumber - pitchBendYMRange);
  float freqTo = NoteToFrequency(slots[channel].status.keyNumber + pitchBendYMRange);
  pitchBendYM = pitch;
  SetFrequency(map(pitch, -8192, 8192, freqFrom, freqTo), channel);
}

void YM2612::ToggleLFO()
{
  lfoOn = !lfoOn;
  Serial.print("LFO: ");
  Serial.println(lfoOn == true ? "ON" : "OFF");
  Voice v = currentVoice;
  uint8_t AMD1R = 0;
  if (lfoOn)
  {
    uint8_t lfo = (1 << 3) | lfoFrq;
    send(0x22, lfo);
    //This is a bulky way to do this, but it works so....
    for (int a1 = 0; a1 <= 1; a1++)
    {
      for (int i = 0; i < 3; i++)
      {
        // Op. 1
        AMD1R = (v.M1[10] << 7) | v.M1[1];
        AMD1R |= 1 << 7;
        send(0x60 + i, AMD1R, a1);

        // Op. 2
        AMD1R = (v.C1[10] << 7) | v.C1[1];
        AMD1R |= 1 << 7;
        send(0x64 + i, AMD1R, a1);

        // Op. 3
        AMD1R = (v.M2[10] << 7) | v.M2[1];
        AMD1R |= 1 << 7;
        send(0x68 + i, AMD1R, a1);

        // Op. 4
        AMD1R = (v.C2[10] << 7) | v.C2[1];
        AMD1R |= 1 << 7;
        send(0x6C + i, AMD1R, a1);

        uint8_t lrAmsFms = 0xC0 + (3 << 4);
        lrAmsFms |= lfoSens;
        send(0xB4 + i, lrAmsFms, a1); // Speaker and LMS
      }
    }
  }
  else
  {
    send(0x22, 0x00); // LFO off
    Reset();
    delay(1);
    SetVoice(v);
  }
  // digitalWriteFast(leds[0], lfoOn);
}

// DRY OMEGALUL
void YM2612::SetTL(uint8_t slot, uint8_t op, uint8_t value)
{
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x40 + (0x04 * op)) + slot;
  send(addr, value, a1);
}

void YM2612::SetAR(uint8_t slot, uint8_t op, uint8_t value)
{
  if (value > 0x1F)
    value = 0x1F;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x50 + (0x04 * op)) + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b11100000; // Mask RS
  data |= value;
  send(addr, data, a1);
}

void YM2612::SetD1R(uint8_t slot, uint8_t op, uint8_t value)
{
  if (value > 0x1F)
    value = 0x1F;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x60 + (0x04 * op)) + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b10000000; // Mask AM
  data |= value;
  send(addr, data, a1);
}

void YM2612::SetD1L(uint8_t slot, uint8_t op, uint8_t value)
{
  if (value > 0x0F)
    value = 0x0F;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x80 + (0x04 * op)) + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b00001111; // Mask RR
  data |= value << 4;
  send(addr, data, a1);
}

void YM2612::SetD2R(uint8_t slot, uint8_t op, uint8_t value)
{
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x70 + (0x04 * op)) + slot;
  send(addr, value, a1);
}

void YM2612::SetRR(uint8_t slot, uint8_t op, uint8_t value)
{
  if (value > 0x0F)
    value = 0x0F;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x80 + (0x04 * op)) + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b11110000; // Mask D1L
  data |= value;
  send(addr, data, a1);
}

void YM2612::SetDetune(uint8_t slot, uint8_t op, uint8_t value)
{
  if (value > 0x07)
    value = 0x07;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x30 + (0x04 * op)) + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b00001111; // Mask MUL
  data |= value << 4;
  send(addr, data, a1);
}

void YM2612::SetMult(uint8_t slot, uint8_t op, uint8_t value)
{
  if (value > 0x0F)
    value = 0x0F;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x30 + (0x04 * op)) + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b01110000; // Mask DT1
  data |= value;
  send(addr, data, a1);
}

void YM2612::SetRateScaling(uint8_t slot, uint8_t op, uint8_t value)
{
  if (value > 0x03)
    value = 0x03;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x50 + (0x04 * op)) + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b00011111; // Mask AR
  data |= value << 6;
  send(addr, data, a1);
}

void YM2612::SetAmplitudeModulation(uint8_t slot, uint8_t op, bool value)
{
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = (0x60 + (0x04 * op)) + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b01111111; // Mask AR
  data |= value << 7;
  send(addr, data, a1);
}

void YM2612::SetLFOEnabled(bool value)
{
  uint8_t data = GetShadowValue(0x22, 0);
  data &= 0b11110111; //Mask LFOFrq
  data |= value << 3;
  send(0x22, data, false);
}

void YM2612::SetLFOFreq(bool value)
{
  uint8_t data = GetShadowValue(0x22, 0);
  data &= 0b11111000; // Mask LFOEnable
  data |= value;
  send(0x22, data, false);
}

void YM2612::SetFreqModSens(uint8_t slot, uint8_t value)
{
  if (value > 0x07)
    value = 0x07;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = 0xB4 + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b11111000; // Mask L_R_AMS
  data |= value;
  send(addr, data, a1);
}

void YM2612::SetAMSens(uint8_t slot, uint8_t value)
{
  if (value > 0x07)
    value = 0x07;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = 0xB4 + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b11000111; // Mask L_R_FMS
  data |= value << 3;
  data |= 0b11000000; // Set L_R to true for now
  send(addr, data, a1);
}

void YM2612::SetAlgo(uint8_t slot, uint8_t value)
{
  if (value > 0x07)
    value = 0x07;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = 0xB0 + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b11111000; // Mask feedback
  data |= value;
  send(addr, data, a1);
}

void YM2612::SetFMFeedback(uint8_t slot, uint8_t value)
{
  if (value > 0x07)
    value = 0x07;
  bool a1 = (slot > 2);
  slot %= 3;
  uint8_t addr = 0xB0 + slot;
  uint8_t data = GetShadowValue(addr, a1);

  data &= 0b11000111; // Mask Algo
  data |= value << 3;
  send(addr, data, a1);
}

//Notes
// DIGITAL BUS = PF0-PF7
// IC = PC0/10
// CS = PC1/11
// WR = PC2/12
// RD = PC3/13
// A0 = PC4/14
// A1 = PC5/15