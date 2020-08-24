#ifndef MEGAMIDI_UI_H_
#define MEGAMIDIUI_H_

#include <Arduino.h>
#include <Encoder.h>
#include <LiquidCrystal.h>

#define LCD_ROWS 4
#define LCD_COLS 20

//INPUT
#define PROG_UP 5
#define PROG_DOWN 6
#define LFO_TOG 7
#define ENC_BTN 0

void HandleRotaryButtonDown();
void HandleRotaryEncoder();

void ScrollFileNameLCD(LiquidCrystal *lcd);
void DrawMainMenu(LiquidCrystal *lcd, bool SDok, char *filename);
void DrawYamahaSettings(LiquidCrystal *lcd);

#endif