/**
 * SubGHz Protocol System Implementation
 */

#include "subghz_protocols.h"

static bool protocols_initialized = false;

/**
 * Initialize SubGHz protocol system
 * Registers all available protocols
 */
void subghz_protocols_init() {
    if (protocols_initialized) {
        return;
    }

    Serial.println("[SubGHz Protocols] Initializing protocol system...");

    ProtocolRegistry* registry = getProtocolRegistry();

    // Register all available protocols
    // IMPORTANT: Registration order matters for auto-detection!
    // More specific protocols should be registered before more permissive ones
    // CAME has an opportunistic mode that can falsely match other protocols
    registry->registerProtocol(new SecPlusV1Protocol());
    registry->registerProtocol(new SecPlusV2Protocol());
    registry->registerProtocol(new PrincetonProtocol());  // Before CAME
    registry->registerProtocol(new CAMEProtocol());       // Last (most permissive)
    registry->registerProtocol(new BinRAWProtocol());     // Playback-only format

    // Future protocols:
    // etc.

    Serial.printf("[SubGHz Protocols] Initialized with %d protocols\n",
                 registry->getProtocolCount());

    protocols_initialized = true;
}

/**
 * Deinitialize protocol system
 */
void subghz_protocols_deinit() {
    if (!protocols_initialized) {
        return;
    }

    Serial.println("[SubGHz Protocols] Deinitializing protocol system...");
    getProtocolRegistry()->clear();
    protocols_initialized = false;
}
