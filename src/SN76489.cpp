#include "SN76489.h"

//Referenced from https://github.com/cdodd/teensy-sn76489-midi-synth/blob/master/teensy-sn76489-midi-synth.ino

SN76489::SN76489()
{
  mode = UNIFIED_MODE;
  DDRF = 0xFF;
  PORTF = 0x00;
  pinMode(_WE, OUTPUT);
  digitalWriteFast(_WE, HIGH);
  for (uint8_t i = 0; i < MAX_CHANNELS_PSG; i++)
  {
    slots[i].index = i;
  }
}

void SN76489::Reset()
{
  send(0x9F);
  send(0xBF);
  send(0xDF);
  send(0xFF);
}

void SN76489::ChangeSlotParam(uint8_t slotIndex, SlotParameter parameter, uint8_t direction)
{
  SNSlot slotBuffer;
  switch (parameter)
  {
  case NOISE:
    // TODO
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
    memcpy(&slotBuffer, &slots[slotIndex], sizeof(SNSlot));
    for (uint8_t i = 0; i < MAX_CHANNELS_PSG; i++)
    {
      memcpy(&slots[i], &slotBuffer, sizeof(SNSlot));
    }
  }
}

void SN76489::send(uint8_t data)
{
  //Byte 1
  // 1   REG ADDR        DATA
  //|1| |R0|R1|R2| |F6||F7|F8|F9|

  //Byte 2
  //  0           DATA
  //|0|0| |F0|F1|F2|F3|F4|F5|

  digitalWriteFast(_WE, HIGH);
  PORTF = data;
  digitalWriteFast(_WE, LOW);
  delayMicroseconds(25);
  digitalWriteFast(_WE, HIGH);
}

void SN76489::MIDISetNoiseControl(byte control, byte value)
{
  SetSquareFrequency(2, ((127 - value) << 3) + 1);
}

bool SN76489::UpdateNoiseControl()
{
  byte noiseControlData;

  switch (currentNote[noise])
  {
  case 60:
    // Note: C4, Periodic noise, shift rate = clock speed (Hz) / 512
    noiseControlData = 0x00;
    break;

  case 62:
    // Note: D4, Periodic noise, shift rate = clock speed (Hz) / 1024
    noiseControlData = 0x01;
    break;

  case 64:
    // Note: E4, Periodic noise, shift rate = clock speed (Hz) / 2048
    noiseControlData = 0x02;
    break;

  case 65:
    // Note: F4, Perioic noise, shift rate = Square voice 3 frequency
    noiseControlData = 0x03;
    break;

  case 67:
    // Note: G4, White noise, shift rate = clock speed (Hz) / 512
    noiseControlData = 0x04;
    break;

  case 69:
    // Note: A4, White noise, shift rate = clock speed (Hz) / 1024
    noiseControlData = 0x05;
    break;

  case 71:
    // Note: B4, White noise, shift rate = clock speed (Hz) / 2048
    noiseControlData = 0x06;
    break;

  case 72:
    // Note: C5, White noise, shift rate = Square voice 3 frequency
    noiseControlData = 0x07;
    break;

  default:
    return false;
  }

  // Send the noise control byte to the SN76489 and return true
  send(0x80 | 0x60 | noiseControlData);
  return true;
}

void SN76489::SetNoiseOn(uint8_t key, uint8_t velocity, bool velocityEnabled)
{
  currentVelocity[noise] = 127;
  currentNote[noise] = key;
  bool updateAttenuationFlag = UpdateNoiseControl();
  if (updateAttenuationFlag)
    UpdateAttenuation(noise);
}

void SN76489::SetNoiseOff(uint8_t key)
{
  currentVelocity[noise] = 0;
  currentNote[noise] = key;
  bool updateAttenuationFlag = UpdateNoiseControl();
  if (updateAttenuationFlag)
    UpdateAttenuation(noise);
}

void SN76489::SetChannelOn(uint8_t key, uint8_t velocity, uint8_t midiChannel)
{
  bool updateAttenuationFlag;
  for (uint8_t i = 0; i < MAX_CHANNELS_PSG; i++)
  {
    SNSlot *slot = &slots[i];
    if (slot->midiChannel == midiChannel &&
        key >= slot->keyRangeStart &&
        key <= slot->keyRangeEnd &&
        !slot->status.keyOn)
    {
      slot->status.keyOn = true;
      slot->status.keyNumber = key;
      slot->status.sustained = sustainEnabled;
      if (i == MAX_CHANNELS_PSG - 1)
      {
        SetNoiseOn(key, velocity, velocityEnabled);
      }
      else
      {
        currentVelocity[i] = velocityEnabled ? velocity : 127;
        currentNote[i] = key;
        updateAttenuationFlag = UpdateSquarePitch(i);
        if (updateAttenuationFlag)
          UpdateAttenuation(i);
      }
      break;
    }
  }
}

void SN76489::SetChannelOff(uint8_t key, uint8_t midiChannel)
{
  for (uint8_t i = 0; i < MAX_CHANNELS_PSG; i++)
  {
    SNSlot *slot = &slots[i];
    if (slot->midiChannel == midiChannel &&
        slot->status.keyNumber == key &&
        key >= slot->keyRangeStart &&
        key <= slot->keyRangeEnd &&
        slot->status.keyOn)
    {
      slot->status.keyOn = false;
      slot->status.keyNumber = 0;
      slot->status.sustained = false;
      if (i == MAX_CHANNELS_PSG - 1)
      {
        SetNoiseOff(key);
      }
      else
      {
        currentVelocity[i] = 0;
        UpdateAttenuation(i);
      }
    }
  }
}

void SN76489::PitchChange(uint8_t channel, int pitch)
{
  if (channel < 0 || channel > 2)
    return;
  currentPitchBend[channel] = pitch;
  UpdateSquarePitch(channel);
}

bool SN76489::UpdateSquarePitch(uint8_t voice)
{
  float pitchInHz;
  unsigned int frequencyData;
  if (voice < 0 || voice > 2)
    return false;
  pitchInHz = 440 * pow(2, (float(currentNote[voice] - 69) / 12) + (float(currentPitchBend[voice] - 8192) / ((unsigned int)4096 * 12)));
  frequencyData = clockHz / float(32 * pitchInHz);
  if (frequencyData > 1023)
    return false;
  SetSquareFrequency(voice, frequencyData);
  return true;
}

void SN76489::SetSquareFrequency(uint8_t voice, int frequencyData)
{
  if (voice < 0 || voice > 2)
    return;
  send(0x80 | frequencyRegister[voice] | (frequencyData & 0x0f));
  send(frequencyData >> 4);
}

void SN76489::UpdateAttenuation(uint8_t voice)
{
  uint8_t attenuationValue;
  if (voice < 0 || voice > 3)
    return;
  attenuationValue = (127 - currentVelocity[voice]) >> 3;
  send(0x80 | attenuationRegister[voice] | attenuationValue);
}

void SN76489::ReleaseSustainedKeys()
{
  for (int i = 0; i < MAX_CHANNELS_PSG; i++)
  {
    if (slots[i].status.sustained && slots[i].status.keyOn)
    {
      slots[i].status.sustained = false;
    }
  }
}

void SN76489::ClampSustainedKeys()
{
  for (int i = 0; i < MAX_CHANNELS_PSG; i++)
  {
    if (!slots[i].status.sustained && slots[i].status.keyOn)
    {
      slots[i].status.sustained = true;
    }
  }
}

//Notes
// DIGITAL BUS = PF0-PF7
// WE = PE4/36
// RDY = PE5/37