#include "UI.h"
#include "Globals.h"

uint32_t prevMilli = 0;
uint16_t scrollDelay = 500;
uint16_t fileNameScrollIndex = 0;

uint8_t filenameColumn = 0;
uint8_t filenameRow = 0;
uint8_t maxFilenameDisplayLength = 0;

String scrollFilename;

uint8_t cursorIndex = 0;
uint8_t maxCurrentOptions = 0;

Encoder encoder(18, 19);
long encoderPos = 0;

void HandleRotaryButtonDown()
{
  cursorIndex++;
  cursorIndex %= maxCurrentOptions;
  redrawLCDOnNextLoop = true;
}

void HandleRotaryEncoder()
{
  long enc = encoder.read();
  if(enc != encoderPos && enc % 4 == 0)
  {
    encoderPos = enc;
    cursorIndex = encoderPos % maxCurrentOptions;
    cursorIndex = max(cursorIndex, 0);
    redrawLCDOnNextLoop = true;
  }
}

void ScrollFileNameLCD(LiquidCrystal* lcd)
{
  String sbStr = scrollFilename;
  int filenameLength = sbStr.length();;
  if(filenameLength > maxFilenameDisplayLength)
  {
    uint32_t curMilli = millis();
    sbStr = sbStr.substring(fileNameScrollIndex, fileNameScrollIndex+maxFilenameDisplayLength);
    if(curMilli - prevMilli >= scrollDelay)
    {
      prevMilli = curMilli;
      fileNameScrollIndex++;
      if(fileNameScrollIndex+maxFilenameDisplayLength >= scrollFilename.length())
      {
        fileNameScrollIndex = 0;
        scrollDelay *= 5;
      }
      else
      {
        scrollDelay = 500;
      }
    }
  }
  filenameColumn = LCD_COLS - sbStr.length();
  lcd->setCursor(filenameColumn, filenameRow);
  lcd->print(sbStr);
}

void DrawMainMenu(LiquidCrystal* lcd, bool SDok, char* filename)
{
  maxCurrentOptions = 3;
  lcd->clear();
  lcd->print("||   MEGA  MIDI   ||");
  if (!SDok)
  {
    lcd->setCursor(6, 2);
    lcd->print("SD CARD");
    lcd->setCursor(4, 3);
    lcd->print("READ FAILURE");
    return;
  }
  if (strlen(filename) == 0)
  {
    lcd->setCursor(2, 2);
    lcd->print("FILE READ FAILED");
    return;
  }
  lcd->setCursor(1, 1);
  lcd->print("File");
  filenameRow = 1;
  scrollFilename = filename;
  maxFilenameDisplayLength = 14;
  if (cursorIndex != 0) {
    fileNameScrollIndex = 0;
  }
  ScrollFileNameLCD(lcd);    
  lcd->setCursor(1, 2);
  lcd->print("YAMAHA settings   >");
  lcd->setCursor(1, 3);
  lcd->print("SN76489 settings  >");
  lcd->setCursor(0, cursorIndex + 1);
  lcd->write(byte(0));
}

void DrawYamahaSettings(LiquidCrystal* lcd)
{
  maxCurrentOptions = 3;
  lcd->clear();
  lcd->setCursor(0, 1);
  lcd->print("Back              <");
  if (yamahaMode == UNIFIED) {
    lcd->setCursor(1, 1);
    lcd->print("YAMAHA      Unified");
    lcd->setCursor(1, 2);
    lcd->print("Voice settings    >");
  } else if (yamahaMode == SEPARATE) {
    lcd->setCursor(1, 1);
    lcd->print("YAMAHA     Separate");
    lcd->setCursor(1, 2);
    lcd->print("Slot settings     >");
  }
}