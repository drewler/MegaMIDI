// CPU settings
// #undef F_CPU
#define F_CPU 16000000UL

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Encoder.h>
#include <EEPROM.h>
#include <MIDI.h>

#include "usb_midi_serial.h"

#include "Definitions.h"
#include "YM2612.h"
#include "SN76489.h"
#include "FileUtil.h"
#include "UI.h"

// Pin settings
#define ENC_BTN 0
#define PROG_UP 5
#define PROG_DOWN 6
#define LFO_TOG 7
#define DLED 8
#define ENCODER_PIN_1 18
#define ENCODER_PIN_2 19
#define YM_CLOCK 25
#define SN_CLOCK 16
#define PSG_READY 37

// Clocks
uint32_t masterClockFrequency = 8000000;

// Prototypes
void HandleRotaryButtonDown();
void KeyOn(byte channel, byte key, byte velocity);
void KeyOff(byte channel, byte key, byte velocity);
void ResetSoundChips();

LiquidCrystal lcd(17, 26, 38, 39, 40, 41, 42, 43, 44, 45); //PC7 & PB6 + Same data bus as sound chips
Encoder encoder(ENCODER_PIN_1, ENCODER_PIN_2);
YM2612 ym2612 = YM2612();
SN76489 sn76489 = SN76489();
FileUtil fileUtil;
UI ui = UI(&lcd, &encoder, &ym2612, &sn76489, &fileUtil);

// TODO - avoid initializing USB MIDI if some key is held at boot
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

void setup()
{
  // YM2612 and PSG Clock Generation
  pinMode(YM_CLOCK, OUTPUT);
  pinMode(SN_CLOCK, OUTPUT);
  // 8MHz on PB5 (YM2612)
  // set up Timer 1
  TCCR1A = bit(COM1A0);            // toggle OC1A on Compare Match
  TCCR1B = bit(WGM12) | bit(CS10); // CTC, no prescaling
  OCR1A = 0;                       // Divide by 2

  // 4MHz on PC6 (PSG)
  // set up Timer 3
  TCCR3A = bit(COM3A0);            // toggle OC3A on Compare Match
  TCCR3B = bit(WGM32) | bit(CS30); // CTC, no prescaling
  OCR3A = 1;                       // Divide by 4

  // Initialize UI
  ui.DrawCurrentScreen();

  MIDI.begin(MIDI_CHANNEL_OMNI);

  delay(20); // Wait for clocks to start
  ResetSoundChips();

  usbMIDI.setHandleNoteOn(KeyOn);
  usbMIDI.setHandleNoteOff(KeyOff);

  MIDI.setHandleNoteOn(KeyOn);
  MIDI.setHandleNoteOff(KeyOff);

  pinMode(DLED, OUTPUT);
  pinMode(PROG_UP, INPUT_PULLUP);
  pinMode(PROG_DOWN, INPUT_PULLUP);
  pinMode(LFO_TOG, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);
  pinMode(PSG_READY, INPUT);

  MIDI.turnThruOff();
  UCSR1B &= ~(1UL << 3); // Release the Hardware Serial1 TX pin from USART transmitter.

  DDRA = 0x00;
  PORTA = 0xFF;

  ui.TurnOffLEDs();
  ui.IntroLEDs();

  attachInterrupt(digitalPinToInterrupt(ENC_BTN), HandleRotaryButtonDown, FALLING);
  fileUtil.LoadFile(FIRST_FILE);
  if (maxValidVoices > 0)
  {
    for (uint8_t i = 0; i < MAX_CHANNELS_YM; i++)
    {
      ym2612.SetVoiceManual(i, voices[0]);
    }
  }
  ui.PushScreen(MAIN_MENU);
}

void HandleRotaryButtonDown()
{
  ui.HandleRotaryButtonDown();
}

void ResetSoundChips()
{
  ym2612.Reset();
  sn76489.Reset();
}

/**
 * @brief MIDI Key On message
 * 
 * @param channel MIDI channel [0-15]
 * @param key Key number [0-127]
 * @param velocity Velocity [0-127]
 */
void KeyOn(byte channel, byte key, byte velocity)
{
  sn76489.SetChannelOn(key, velocity, channel);
  ym2612.SetChannelOn(key, velocity, channel);
}

void KeyOff(byte channel, byte key, byte velocity)
{
  sn76489.SetChannelOff(key, channel);
  ym2612.SetChannelOff(key, channel);
}

void loop()
{
  while (usbMIDI.read())
  {
  };
  MIDI.read();
  ui.HandleRotaryEncoder();
  UIState *currentState = ui.GetCurrentState();
  if (currentState->screen == MAIN_MENU)
    ui.ScrollFileNameLCD();
  ui.TentativeRedraw();
}
