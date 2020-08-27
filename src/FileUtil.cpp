#include "Definitions.h"

#include "FileUtil.h"
#include "UI.h"

FileUtil::FileUtil()
{
  if (!SD.begin(SD_CHIP_SELECT, SPI_HALF_SPEED))
  {
    // TODO - failed
  }
  // Count files
  File countFile;
  while (countFile.openNext(SD.vwd(), O_READ))
  {
    countFile.close();
    numberOfFiles++;
  }
  countFile.close();
  SD.vwd()->rewind();
}

// Request a file with NEXT, PREV, FIRST commands
bool FileUtil::LoadFile(byte strategy)
{
  if (numberOfFiles == 0)
  {
    return false;
  }
  File nextFile;
  memset(fileName, 0x00, MAX_FILE_NAME_SIZE);
  switch (strategy)
  {
  case FIRST_FILE:
  {
    SD.vwd()->rewind();
    if (file.isOpen())
    {
      file.close();
    }
    file.openNext(SD.vwd(), O_READ);
    if (!file)
    {
      //TODO
    }
    file.getName(fileName, MAX_FILE_NAME_SIZE);
    currentFileNumber = 0;
    break;
  }
  case NEXT_FILE:
  {
    if (currentFileNumber + 1 >= numberOfFiles)
    {
      SD.vwd()->rewind();
      currentFileNumber = 0;
    }
    else
      currentFileNumber++;
    nextFile.openNext(SD.vwd(), O_READ);
    nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
    nextFile.close();
    break;
  }
  case PREV_FILE:
  {
    if (currentFileNumber != 0)
    {
      currentFileNumber--;
      SD.vwd()->rewind();
      for (uint32_t i = 0; i <= currentFileNumber; i++)
      {
        nextFile.close();
        nextFile.openNext(SD.vwd(), O_READ);
      }
      nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      nextFile.close();
    }
    else
    {
      currentFileNumber = numberOfFiles - 1;
      SD.vwd()->rewind();
      for (uint32_t i = 0; i <= currentFileNumber; i++)
      {
        nextFile.close();
        nextFile.openNext(SD.vwd(), O_READ);
      }
      nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      nextFile.close();
    }
    break;
  }
  }
  if (file.isOpen())
    file.close();
  file = SD.open(fileName, FILE_READ);
  if (!file)
  {
    digitalWrite(leds[0], HIGH);

    // TODO
  }
  ReadVoiceData();
  return true;
}

void FileUtil::ReadVoiceData()
{
  size_t n;
  uint8_t voiceCount = 0;
  char *pEnd;
  uint8_t vDataRaw[6][11];
  const size_t LINE_DIM = 60;
  char line[LINE_DIM];
  bool foundNoName = false;
  while ((n = file.fgets(line, sizeof(line))) > 0)
  {
    String l = line;
    //Ignore comments
    if (l.startsWith("//"))
      continue;
    if (l.startsWith("@:" + String(voiceCount) + " no Name"))
    {
      maxValidVoices = voiceCount;
      foundNoName = true;
      break;
    }
    else if (l.startsWith("@:" + String(voiceCount)))
    {
      for (int i = 0; i < 6; i++)
      {
        file.fgets(line, sizeof(line));
        l = line;
        l.replace("LFO: ", "");
        l.replace("CH: ", "");
        l.replace("M1: ", "");
        l.replace("C1: ", "");
        l.replace("M2: ", "");
        l.replace("C2: ", "");
        l.toCharArray(line, sizeof(line), 0);

        vDataRaw[i][0] = strtoul(line, &pEnd, 10);
        for (int j = 1; j < 11; j++)
        {
          vDataRaw[i][j] = strtoul(pEnd, &pEnd, 10);
        }
      }

      for (int i = 0; i < 5; i++) //LFO
        voices[voiceCount].LFO[i] = vDataRaw[0][i];
      for (int i = 0; i < 7; i++) //CH
        voices[voiceCount].CH[i] = vDataRaw[1][i];
      for (int i = 0; i < 11; i++) //M1
        voices[voiceCount].M1[i] = vDataRaw[2][i];
      for (int i = 0; i < 11; i++) //C1
        voices[voiceCount].C1[i] = vDataRaw[3][i];
      for (int i = 0; i < 11; i++) //M2
        voices[voiceCount].M2[i] = vDataRaw[4][i];
      for (int i = 0; i < 11; i++) //C2
        voices[voiceCount].C2[i] = vDataRaw[5][i];
      voiceCount++;
    }
    if (voiceCount == MAX_VOICES - 1)
      break;
  }
  if (!foundNoName)
  {
    maxValidVoices = voiceCount;
  }
  if (voiceCount == 0)
    isFileValid = false;
  else
    isFileValid = true;
}