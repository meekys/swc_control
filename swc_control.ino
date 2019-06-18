#include "TrinketHidCombo.h"
#include "limits.h"

#define VOLTAGE_IN 5
#define RESISTANCE_KNOWN 1000

#define PIN_ANALOG_A A6
#define PIN_ANALOG_B A7
#define TOLERANCE 0.1 // 10%

#define DEBUG

struct MAP
{
  int resistance;
  int multimedia;
  int key;
};

static MAP map_a[] = {
  {    0, 1, MMKEY_VOL_UP },   // (+) Vol Up
  {   50, 1, MMKEY_VOL_DOWN }, // (-) Vol Down
  {  120, 0, 0 },              // (>) Tune Up
  {  230, 0, 0 },              // (<) Tune Down
  {  430, 0, 0 },              // List
  { 1000, 0, KEYCODE_ESC }     // Return
};

static MAP map_b[] = {
  {   0, 1, MMKEY_PLAYPAUSE }, // Mute/Enter
  {  50, 0, KEYCODE_M },       // Talk
  { 120, 0, 0 },               // Source
  { 230, 0, KEYCODE_P },       // On Hook
  { 430, 0, KEYCODE_O }        // Off Hook
};

static MAP *keyPressed = NULL;

void setup()
{
  TrinketHidCombo.begin(); // start the USB device engine and enumerate
}

void write_number(int value)
{
#ifdef DEBUG
  char buffer[12];
  itoa(value, buffer, 10);
  write(buffer);
#endif
}

void write(char* buffer)
{
#ifdef DEBUG
  TrinketHidCombo.write(buffer);
#endif
}

int read_value(int pin)
{
  int raw = analogRead(pin);

  if (!raw)
    return 0;

  if (raw == 1023) // max value, assume open circuit?
    return INT_MAX;

  float voltage_out = (raw * VOLTAGE_IN) / 1024.0;
  float resistance = (voltage_out * RESISTANCE_KNOWN) / (VOLTAGE_IN - voltage_out);

  return (int)resistance;
}

MAP* get_key(int pin, MAP map[], int size)
{
  int value = read_value(pin);
  for (int i = 0; i < size; i++)
  {
    if (value >= map[i].resistance * (1 - TOLERANCE)
      && value <= map[i].resistance * (1 + TOLERANCE))
    {
      return &map[i];
    }
  }

  return NULL;
}

void pressKey(MAP* newKey)
{
  if (!newKey || !newKey->key)
    return;

  write("kd\n");
  if (newKey->multimedia)
  {
    TrinketHidCombo.pressMultimediaKey(newKey->key);
  }
  else
  {
    TrinketHidCombo.pressKey(0, newKey->key);
    // Immediately release
    TrinketHidCombo.pressKey(0, 0);
  }
  write("\n");
}

void loop()
{
  MAP* newKey = get_key(PIN_ANALOG_A, map_a, sizeof(map_a) / sizeof(MAP));
  if (!newKey)
    newKey = get_key(PIN_ANALOG_B, map_b, sizeof(map_b) / sizeof(MAP));
    
  if (newKey)
  {
    if (!keyPressed)
    {
      pressKey(newKey);

      delay(5); // debounce delay
      keyPressed = newKey;
    }
  }
  else
  {
    if (keyPressed)
    {
      delay(5); // debounce delay
      keyPressed = NULL;
    }
  }

  TrinketHidCombo.poll(); // check if USB needs anything done
}
