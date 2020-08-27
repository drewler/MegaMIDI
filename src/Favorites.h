#ifndef FAVORITES_H_
#define FAVORITES_H_

#include <Arduino.h>

class Favorites
{
private:
public:
  void Save(uint8_t index);
  void Load(uint8_t index);
};

#endif