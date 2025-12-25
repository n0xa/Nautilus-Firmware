#ifndef DEBUG_H
#define DEBUG_H

// Uncomment to enable debug output
// #define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTF(fmt, ...)
  #define DEBUG_PRINTLN(x)
#endif

#endif // DEBUG_H
