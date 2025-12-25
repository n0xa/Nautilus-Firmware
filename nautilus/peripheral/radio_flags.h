/**
 * Radio Interrupt Flag Helpers
 * Standardizes interrupt flag patterns across radio peripheral modules
 */

#ifndef __RADIO_FLAGS_H__
#define __RADIO_FLAGS_H__

// Macro to declare a radio interrupt flag and its setter function
#define DECLARE_RADIO_FLAG(name) \
    static volatile bool name##Flag = false; \
    static void name##SetFlag(void) { \
        name##Flag = true; \
    }

// Macro to check and reset a flag (returns true if flag was set)
#define CHECK_AND_RESET_FLAG(name) \
    (name##Flag ? (name##Flag = false, true) : false)

// Macro to manually reset a flag
#define RESET_FLAG(name) \
    do { name##Flag = false; } while(0)

// Macro to check flag without resetting
#define CHECK_FLAG(name) \
    (name##Flag)

#endif // __RADIO_FLAGS_H__
