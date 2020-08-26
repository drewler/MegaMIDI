#ifndef MEGAMIDI_UI_H_
#define MEGAMIDI_UI_H_

#include <LiquidCrystal.h>
#include <Encoder.h>

#include "YM2612.h"
#include "SN76489.h"
#include "FileUtil.h"

#define MAX_UI_NESTING 5

enum Screen
{
  BOOT,
  MAIN_MENU,
  YAMAHA_SETTINGS,
  YAMAHA_SLOTS,
  YAMAHA_SLOT,
  SN_SETTINGS,
  SN_SLOTS,
  SN_SLOT,
};

typedef struct
{
  Screen screen = BOOT;
  uint8_t scrollWindowPosition = 0;
  uint8_t itemsCount = 0;
  uint8_t cursorRow = 0;
  uint8_t cursorIndex = 0;
  bool selecting = false;
} UIState;

class UI
{
private:
  LiquidCrystal *lcd;
  Encoder *encoder;
  YM2612 *ym2612;
  SN76489 *sn76489;
  FileUtil *fileUtil;
  uint8_t filenameScrollEnabled = false;
  uint8_t fileNameScrollIndex = 0;
  uint32_t prevMilli = 0;
  uint32_t scrollDelay = 500;
  uint8_t filenameColumn;
  uint8_t filenameRow;
  uint8_t filenameMaxLength;
  UIState stateStack[MAX_UI_NESTING];
  uint8_t stackLevel = 0;
  bool redrawOnNextLoop = false;
  void DrawBoot();
  void DrawMainMenu();
  void DrawYamahaSettings();
  void DrawYamahaSlotLine(uint8_t row, YamahaSlot *slot);
  void DrawYamahaSlots();
  void DrawYamahaSlot();
  void DrawSNSettings();
  void DrawSNSlotLine(uint8_t row, SNSlot *slot);
  void DrawSNSlots();
  void DrawSNSlot();
  void DrawEndIcon(uint8_t row, char icon);
  void DrawFromEnd(uint8_t row, uint8_t maxLength, String *text);
  void DrawOutOfSelector(uint8_t row, uint8_t numerator, uint8_t denominator);

public:
  UI(LiquidCrystal *lcd, Encoder *encoder, YM2612 *ym2612, SN76489 *sn76489, FileUtil *fileUtil);
  UIState *GetCurrentState();
  void PushScreen(Screen screen);
  void PopScreen();
  void TentativeRedraw();
  void TurnOffLEDs();
  void IntroLEDs();
  void DrawCurrentScreen();
  void DrawBackLine();
  void DrawCursor();
  void HandleRotaryEncoder();
  void HandleRotaryButtonDown();
  void ScrollFileNameLCD();
};

#endif