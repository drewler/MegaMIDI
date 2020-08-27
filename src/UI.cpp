#include <LiquidCrystal.h>

#include "YM2612.h"
#include "SN76489.h"
#include "LCDChars.h"
#include "FileUtil.h"
#include "UI.h"

long encoderPos = 0;

const char backString[] = "Back              ";
const char yamahaString[] = "YAMAHA";
const char sn76489String[] = "SN76489";
const char unifiedString[] = "Unified";
const char separateString[] = "Separate";
const char settingsString[] = " settings";
const char voiceString[] = "Voice";
const char slotString[] = "Slot";
const char noiseString[] = "Noise";
const char midiChannelString[] = "MIDI channel";
const char octaveShiftString[] = "Octave shift";
const char transposeShiftString[] = "Transpose shift";

UI::UI(LiquidCrystal *lcd, Encoder *encoder, YM2612 *ym2612, SN76489 *sn76489, FileUtil *fileUtil)
{
  this->lcd = lcd;
  this->encoder = encoder;
  this->ym2612 = ym2612;
  this->sn76489 = sn76489;
  this->fileUtil = fileUtil;
  lcd->createChar(0, arrowCharLeft);
  lcd->createChar(1, arrowCharRight);
  lcd->createChar(2, heartChar);
  lcd->createChar(3, backChar);
  lcd->begin(LCD_COLS, LCD_ROWS);
}

void UI::PushScreen(Screen screen)
{
  stackLevel++;
  stateStack[stackLevel].scrollWindowPosition = 0;
  stateStack[stackLevel].cursorRow = 0;
  stateStack[stackLevel].cursorIndex = 0;
  stateStack[stackLevel].selecting = false;
  stateStack[stackLevel].screen = screen;
  switch (screen)
  {
  case MAIN_MENU:
  case YAMAHA_SETTINGS:
  case SN_SETTINGS:
    stateStack[stackLevel].itemsCount = 3;
    break;
  case YAMAHA_SLOTS:
    stateStack[stackLevel].itemsCount = 7;
    break;
  case SN_SLOTS:
    stateStack[stackLevel].itemsCount = 4;
    break;
  case YAMAHA_SLOT:
  case SN_SLOT:
    stateStack[stackLevel].itemsCount = 5;
    break;
  case BOOT:
    break;
  }
  redrawOnNextLoop = true;
}

void UI::PopScreen()
{
  stackLevel--;
  redrawOnNextLoop = true;
}

UIState *UI::GetCurrentState()
{
  return &stateStack[stackLevel];
}

void UI::HandleRotaryButtonDown()
{
  UIState *currentState = GetCurrentState();
  switch (currentState->screen)
  {
  case BOOT:
    return;
  case MAIN_MENU:
    switch (currentState->cursorIndex)
    {
    case 0: // File
      currentState->selecting = !currentState->selecting;
      break;
    case 1: // Yamaha Settings
      PushScreen(YAMAHA_SETTINGS);
      break;
    case 2: // SN Settings
      PushScreen(SN_SETTINGS);
      break;
    }
    break;
  case YAMAHA_SETTINGS:
    switch (currentState->cursorIndex)
    {
    case 0: // Back
      PopScreen();
      break;
    case 1: // Mode selector
      currentState->selecting = !currentState->selecting;
      break;
    case 2: // Unified or Slot settings
      if (ym2612->mode == UNIFIED_MODE)
      {
        PushScreen(YAMAHA_SLOT);
      }
      else if (ym2612->mode == SEPARATE_MODE)
      {
        PushScreen(YAMAHA_SLOTS);
      }
      break;
    }
    break;
  case YAMAHA_SLOTS:
    switch (currentState->cursorIndex)
    {
    case 0: // Back
      PopScreen();
      break;
    default: // Yamaha Slot
      PushScreen(YAMAHA_SLOT);
      break;
    }
    break;
  case YAMAHA_SLOT:
    switch (currentState->cursorIndex)
    {
    case 0: // Back
      PopScreen();
      break;
    default:
      currentState->selecting = !currentState->selecting;
      break;
    }
    break;
  case SN_SETTINGS:
    switch (currentState->cursorIndex)
    {
    case 0: // Back
      PopScreen();
      break;
    case 1: // Mode selector
      currentState->selecting = !currentState->selecting;
      break;
    case 2: // Unified or Slot settings
      if (sn76489->mode == UNIFIED_MODE)
      {
        PushScreen(SN_SLOT);
      }
      else if (sn76489->mode == SEPARATE_MODE)
      {
        PushScreen(SN_SLOTS);
      }
      break;
    }
    break;
  case SN_SLOTS:
    switch (currentState->cursorIndex)
    {
    case 0: // Back
      PopScreen();
      break;
    default: // SN Slot
      PushScreen(SN_SLOT);
      break;
    }
    break;
  case SN_SLOT:
    switch (currentState->cursorIndex)
    {
    case 0: // Back
      PopScreen();
      break;
    default:
      currentState->selecting = !currentState->selecting;
      break;
    }
    break;
  default:
    break;
  }
}

void UI::HandleRotaryEncoder()
{
  long enc = encoder->read();
  if (enc != encoderPos && enc % 4 == 0)
  {
    bool isEncoderUp = !(enc > encoderPos);
    encoderPos = enc;
    UIState *currentState = GetCurrentState();
    if (currentState->selecting)
    {
      if (currentState->screen == MAIN_MENU)
      {
        if (currentState->cursorIndex == 0 && currentState->selecting)
        {
          fileUtil->LoadFile(isEncoderUp ? NEXT_FILE : PREV_FILE);
        }
      }
      else if (currentState->screen == YAMAHA_SETTINGS)
      {
        if (currentState->cursorIndex == 1)
        {
          // Mode selector
          ym2612->mode = ym2612->mode == UNIFIED_MODE ? SEPARATE_MODE : UNIFIED_MODE;
        }
      }
      else if (currentState->screen == SN_SETTINGS)
      {
        if (currentState->cursorIndex == 1)
        {
          // Mode selector
          sn76489->mode = sn76489->mode == UNIFIED_MODE ? SEPARATE_MODE : UNIFIED_MODE;
        }
      }
      else if (currentState->screen == YAMAHA_SLOT)
      {
        UIState previousState = stateStack[stackLevel - 1];
        uint8_t slotIndex = previousState.screen == YAMAHA_SLOTS ? previousState.cursorIndex - 1 : 0;
        switch (currentState->cursorIndex)
        {
        case 1: // Voice
          ym2612->ChangeSlotParam(slotIndex, VOICE_INDEX, isEncoderUp ? NEXT : PREVIOUS);
          break;
        case 2: // MIDI channel
          ym2612->ChangeSlotParam(slotIndex, MIDI_CHANNEL, isEncoderUp ? NEXT : PREVIOUS);
          break;
        case 3: // Octave shift
          ym2612->ChangeSlotParam(slotIndex, OCTAVE_SHIFT, isEncoderUp ? NEXT : PREVIOUS);
          break;
        case 4: // Transpose shift
          ym2612->ChangeSlotParam(slotIndex, TRANSPOSE_SHIFT, isEncoderUp ? NEXT : PREVIOUS);
          break;
        }
      }
      else if (currentState->screen == SN_SLOT)
      {
        UIState previousState = stateStack[stackLevel - 1];
        uint8_t slotIndex = previousState.screen == SN_SLOTS ? previousState.cursorIndex - 1 : 0;
        switch (currentState->cursorIndex)
        {
        case 1: // Noise
          // sn76489->ChangeSlotParam(slotIndex, NOISE, isEncoderUp ? NEXT : PREVIOUS);
          break;
        case 2: // MIDI channel
          sn76489->ChangeSlotParam(slotIndex, MIDI_CHANNEL, isEncoderUp ? NEXT : PREVIOUS);
          break;
        case 3: // Octave shift
          sn76489->ChangeSlotParam(slotIndex, OCTAVE_SHIFT, isEncoderUp ? NEXT : PREVIOUS);
          break;
        case 4: // Transpose shift
          sn76489->ChangeSlotParam(slotIndex, TRANSPOSE_SHIFT, isEncoderUp ? NEXT : PREVIOUS);
          break;
        }
      }
      redrawOnNextLoop = true;
      return;
    }
    if (currentState->itemsCount == 0)
      return;

    if (isEncoderUp)
    {
      if (currentState->cursorIndex < (currentState->itemsCount - 1))
      {
        currentState->cursorIndex++;
        if (currentState->cursorRow == 3)
        {
          currentState->scrollWindowPosition++;
          redrawOnNextLoop = true;
        }
        else
          currentState->cursorRow++;
      }
    }
    else
    {
      if (currentState->cursorIndex > 0)
      {
        currentState->cursorIndex--;
        if (currentState->cursorRow == 0)
        {
          currentState->scrollWindowPosition--;
          redrawOnNextLoop = true;
        }
        else
          currentState->cursorRow--;
      }
    }
    DrawCursor();
  }
}

void UI::DrawCursor()
{
  UIState *currentState = GetCurrentState();
  uint8_t cursorDrawRow = currentState->cursorRow;
  for (uint8_t i = 0; i < 4; i++)
  {
    lcd->setCursor(0, i);
    lcd->write(' ');
  }
  if (currentState->screen == MAIN_MENU)
  {
    lcd->setCursor(0, 0);
    lcd->write('|');
    cursorDrawRow++;
  }
  lcd->setCursor(0, cursorDrawRow);
  lcd->write(byte(0));
}

void UI::DrawEndIcon(uint8_t row, char icon)
{
  lcd->setCursor(LCD_COLS - 1, row);
  lcd->write(icon);
}

void UI::DrawBackLine()
{
  lcd->setCursor(1, 0);
  lcd->print(backString);
  lcd->setCursor(LCD_COLS - 1, 0);
  lcd->write(byte(3));
}

void UI::DrawFromEnd(uint8_t row, uint8_t maxLength, String *text)
{
  String shortenedString = *text;
  shortenedString.remove(maxLength);
  lcd->setCursor(LCD_COLS - shortenedString.length(), row);
  lcd->print(shortenedString);
}

void UI::DrawBoot()
{
  lcd->clear();
  lcd->setCursor(5, 0);
  lcd->print("Welcome To");
  lcd->setCursor(6, 1);
  lcd->print("MEGA MIDI");
  lcd->setCursor(3, 2);
  lcd->print("Aidan Lawrence");
  lcd->setCursor(8, 3);
  lcd->print("2019  ");
  lcd->print(FW_VERSION);
}

void UI::DrawMainMenu()
{
  fileNameScrollIndex = 0;
  lcd->clear();
  lcd->print("||   MEGA  MIDI   ||");
  lcd->setCursor(1, 1);
  lcd->print("File");
  ScrollFileNameLCD();
  lcd->setCursor(1, 2);
  lcd->print(yamahaString);
  lcd->print(settingsString);
  DrawEndIcon(2, 0x7E);
  lcd->setCursor(1, 3);
  lcd->print(sn76489String);
  lcd->print(settingsString);
  DrawEndIcon(3, 0x7E);
}

void UI::DrawYamahaSettings()
{
  lcd->clear();
  DrawBackLine();
  lcd->setCursor(1, 1);
  lcd->print(yamahaString);
  String modeString = ym2612->mode == UNIFIED_MODE ? unifiedString : separateString;
  DrawFromEnd(1, 9, &modeString);
  lcd->setCursor(1, 2);
  lcd->print(ym2612->mode == UNIFIED_MODE ? voiceString : slotString);
  lcd->print(settingsString);
  DrawEndIcon(2, 0x7E);
}

void UI::DrawYamahaSlots()
{
  enum LineType
  {
    BACK_LINE,
    SLOT_LINE
  };
  LineType items[7] = {BACK_LINE,
                       SLOT_LINE,
                       SLOT_LINE,
                       SLOT_LINE,
                       SLOT_LINE,
                       SLOT_LINE,
                       SLOT_LINE};

  UIState *currentState = GetCurrentState();
  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t itemOffset = i + currentState->scrollWindowPosition;
    LineType lt = items[itemOffset];
    if (lt == BACK_LINE)
      DrawBackLine();
    else
      DrawYamahaSlotLine(i, &ym2612->slots[itemOffset - 1]);
  }
}

void UI::DrawYamahaSlotLine(uint8_t row, YamahaSlot *slot)
{
  lcd->setCursor(1, row);
  char buffer[LCD_COLS + 1];
  sprintf(
      (char *)&buffer,
      "S%d V%02d CH%02d T%+d O%+d",
      slot->index + 1,
      slot->voiceIndex + 1,
      slot->midiChannel,
      slot->transposeShift,
      slot->octaveShift);
  lcd->print(buffer);
}

void UI::DrawOutOfSelector(uint8_t row, uint8_t numerator, uint8_t denominator)
{
  char buffer[6];
  sprintf(
      (char *)&buffer,
      "%02d/%02d",
      numerator,
      denominator);
  String selectorString = buffer;
  DrawFromEnd(row, 5, &selectorString);
}

void UI::DrawYamahaSlot()
{
  UIState *currentState = GetCurrentState();
  UIState previousState = stateStack[stackLevel - 1];

  lcd->clear();
  enum LineType
  {
    BACK_LINE,
    VOICE_LINE,
    MIDI_CHANNEL_LINE,
    OCTAVE_SHIFT_LINE,
    TRANSPOSE_SHIFT_LINE,
  };
  LineType items[5] = {
      BACK_LINE,
      VOICE_LINE,
      MIDI_CHANNEL_LINE,
      OCTAVE_SHIFT_LINE,
      TRANSPOSE_SHIFT_LINE,
  };

  uint8_t slotIndex = previousState.screen == YAMAHA_SLOTS ? previousState.cursorIndex - 1 : 0;
  YamahaSlot slot = ym2612->slots[slotIndex];
  String shiftText;
  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t itemOffset = i + currentState->scrollWindowPosition;
    LineType lt = items[itemOffset];
    lcd->setCursor(1, i);
    char shiftBuffer[3];
    if (lt == BACK_LINE)
      DrawBackLine();
    else if (lt == VOICE_LINE)
    {
      lcd->print(voiceString);
      DrawOutOfSelector(i, slot.voiceIndex + 1, maxValidVoices + 1);
    }
    else if (lt == MIDI_CHANNEL_LINE)
    {
      lcd->print(midiChannelString);
      DrawOutOfSelector(i, slot.midiChannel, 16);
    }
    else if (lt == OCTAVE_SHIFT_LINE)
    {
      lcd->print(octaveShiftString);

      sprintf(shiftBuffer, "%+d", slot.octaveShift);
      shiftText = shiftBuffer;
      DrawFromEnd(i, 3, &shiftText);
    }
    else if (lt == TRANSPOSE_SHIFT_LINE)
    {
      lcd->print(transposeShiftString);
      sprintf(shiftBuffer, "%+d", slot.transposeShift);
      shiftText = shiftBuffer;
      DrawFromEnd(i, 3, &shiftText);
    }
  }
}

void UI::DrawSNSettings()
{
  lcd->clear();
  DrawBackLine();
  lcd->setCursor(1, 1);
  lcd->print(sn76489String);
  String modeString = sn76489->mode == UNIFIED_MODE ? unifiedString : separateString;
  DrawFromEnd(1, 9, &modeString);
  lcd->setCursor(1, 2);
  lcd->print(sn76489->mode == UNIFIED_MODE ? unifiedString : slotString);
  lcd->print(settingsString);
  DrawEndIcon(2, 0x7E);
}

void UI::DrawSNSlots()
{
  enum LineType
  {
    BACK_LINE,
    SLOT_LINE
  };
  LineType items[7] = {BACK_LINE,
                       SLOT_LINE,
                       SLOT_LINE,
                       SLOT_LINE};

  UIState *currentState = GetCurrentState();
  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t itemOffset = i + currentState->scrollWindowPosition;
    LineType lt = items[itemOffset];
    if (lt == BACK_LINE)
      DrawBackLine();
    else
      DrawSNSlotLine(i, &sn76489->slots[itemOffset - 1]);
  }
}

void UI::DrawSNSlotLine(uint8_t row, SNSlot *slot)
{
  lcd->setCursor(1, row);
  char buffer[LCD_COLS + 1];
  sprintf(
      (char *)&buffer,
      "S%d --- CH%02d T%+d O%+d",
      slot->index + 1,
      slot->midiChannel,
      slot->transposeShift,
      slot->octaveShift);
  lcd->print(buffer);
}

void UI::DrawSNSlot()
{
  UIState *currentState = GetCurrentState();
  UIState previousState = stateStack[stackLevel - 1];

  lcd->clear();
  enum LineType
  {
    BACK_LINE,
    NOISE_LINE,
    MIDI_CHANNEL_LINE,
    OCTAVE_SHIFT_LINE,
    TRANSPOSE_SHIFT_LINE,
  };
  LineType items[5] = {
      BACK_LINE,
      NOISE_LINE,
      MIDI_CHANNEL_LINE,
      OCTAVE_SHIFT_LINE,
      TRANSPOSE_SHIFT_LINE,
  };

  uint8_t slotIndex = previousState.screen == SN_SLOTS ? previousState.cursorIndex - 1 : 0;
  SNSlot slot = sn76489->slots[slotIndex];
  String shiftText;
  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t itemOffset = i + currentState->scrollWindowPosition;
    LineType lt = items[itemOffset];
    lcd->setCursor(1, i);
    char shiftBuffer[3];
    if (lt == BACK_LINE)
      DrawBackLine();
    else if (lt == NOISE_LINE)
    {
      lcd->print(noiseString);
      // DrawOutOfSelector(i, slot.voiceIndex + 1, maxValidVoices);
    }
    else if (lt == MIDI_CHANNEL_LINE)
    {
      lcd->print(midiChannelString);
      DrawOutOfSelector(i, slot.midiChannel, 16);
    }
    else if (lt == OCTAVE_SHIFT_LINE)
    {
      lcd->print(octaveShiftString);

      sprintf(shiftBuffer, "%+d", slot.octaveShift);
      shiftText = shiftBuffer;
      DrawFromEnd(i, 3, &shiftText);
    }
    else if (lt == TRANSPOSE_SHIFT_LINE)
    {
      lcd->print(transposeShiftString);
      sprintf(shiftBuffer, "%+d", slot.transposeShift);
      shiftText = shiftBuffer;
      DrawFromEnd(i, 3, &shiftText);
    }
  }
}

void UI::TurnOffLEDs()
{
  for (int i = 0; i < 8; i++)
  {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }
}

void UI::IntroLEDs()
{
  int i = 0;
  for (i = 0; i < 8; i++)
  {
    digitalWrite(leds[i], HIGH);
    delay(100);
  }

  for (i = 0; i < 8; i++)
  {
    digitalWrite(leds[i], LOW);
    delay(100);
  }
}

void UI::ScrollFileNameLCD()
{
  String filenameString = fileUtil->fileName;
  String sbStr = fileUtil->fileName;
  int filenameLength = filenameString.length();
  if (filenameLength > filenameMaxLength)
  {
    uint32_t curMilli = millis();
    sbStr = sbStr.substring(fileNameScrollIndex, fileNameScrollIndex + filenameMaxLength);
    if (curMilli - prevMilli >= scrollDelay)
    {
      prevMilli = curMilli;
      fileNameScrollIndex++;
      if (fileNameScrollIndex + filenameMaxLength > filenameString.length())
      {
        fileNameScrollIndex = 0;
        scrollDelay *= 2.5;
      }
      else
      {
        scrollDelay = 500;
      }
    }
  }
  DrawFromEnd(1, filenameMaxLength, &sbStr);
}

void UI::DrawCurrentScreen()
{
  switch (GetCurrentState()->screen)
  {
  case BOOT:
    DrawBoot();
    return;
  case MAIN_MENU:
    DrawMainMenu();
    break;
  case YAMAHA_SETTINGS:
    DrawYamahaSettings();
    break;
  case YAMAHA_SLOTS:
    DrawYamahaSlots();
    break;
  case YAMAHA_SLOT:
    DrawYamahaSlot();
    break;
  case SN_SETTINGS:
    DrawSNSettings();
    break;
  case SN_SLOTS:
    DrawSNSlots();
    break;
  case SN_SLOT:
    DrawSNSlot();
    break;
  default:
    break;
  }
  DrawCursor();
}

void UI::TentativeRedraw()
{
  if (!redrawOnNextLoop)
    return;
  redrawOnNextLoop = false;
  DrawCurrentScreen();
}