
#ifndef FILEUTILS_H_
#define FILEUTILS_H_

#include "SdFat.h"

#define FIRST_FILE 0x00
#define NEXT_FILE 0x01
#define PREV_FILE 0x02

#define MAX_FILE_NAME_SIZE 128

#define SD_CHIP_SELECT SS // PB0

class FileUtil
{
private:
  File file;
  uint32_t numberOfFiles = 0;
  uint32_t currentFileNumber = 0;
  bool isFileValid = false;
  SdFat SD;

public:
  char fileName[MAX_FILE_NAME_SIZE];
  FileUtil();
  bool LoadFile(byte strategy);
  bool LoadFile(String req);
  void ReadVoiceData();
};

#endif