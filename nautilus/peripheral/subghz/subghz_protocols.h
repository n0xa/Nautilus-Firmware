/**
 * SubGHz Protocol System
 *
 * Central header for SubGHz protocol framework.
 * Include this file to access all protocol functionality.
 */

#ifndef __SUBGHZ_PROTOCOLS_H__
#define __SUBGHZ_PROTOCOLS_H__

#include "protocol_base.h"
#include "protocol_registry.h"

// Protocol implementations
#include "protocols/protocol_came.h"
#include "protocols/protocol_secplus_v1.h"
#include "protocols/protocol_secplus_v2.h"
#include "protocols/protocol_princeton.h"
#include "protocols/protocol_binraw.h"

/**
 * Initialize SubGHz protocol system
 * Registers all available protocols with the registry
 *
 * Call this once during system initialization
 */
void subghz_protocols_init();

/**
 * Deinitialize SubGHz protocol system
 * Clears all registered protocols
 */
void subghz_protocols_deinit();

#endif // __SUBGHZ_PROTOCOLS_H__
