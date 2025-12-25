/**
 * SubGHz Protocol Registry
 *
 * Manages registration and lookup of all available SubGHz protocols.
 * Provides auto-detection by attempting all registered decoders.
 */

#ifndef __PROTOCOL_REGISTRY_H__
#define __PROTOCOL_REGISTRY_H__

#include "protocol_base.h"
#include <vector>

/**
 * Protocol Registry - Singleton pattern
 */
class ProtocolRegistry {
private:
    std::vector<SubGhzProtocol*> protocols;

    // Singleton instance
    static ProtocolRegistry* instance;

    ProtocolRegistry() {}

public:
    /**
     * Get singleton instance
     */
    static ProtocolRegistry* getInstance() {
        if (instance == nullptr) {
            instance = new ProtocolRegistry();
        }
        return instance;
    }

    /**
     * Register a protocol
     * Registry takes ownership of the pointer
     *
     * @param protocol Protocol instance to register
     */
    void registerProtocol(SubGhzProtocol* protocol) {
        if (protocol != nullptr) {
            protocols.push_back(protocol);
            Serial.printf("[Registry] Registered protocol: %s\n", protocol->getName());
        }
    }

    /**
     * Get protocol by name
     *
     * @param name Protocol name (e.g., "CAME", "Princeton")
     * @return Protocol instance or nullptr if not found
     */
    SubGhzProtocol* getProtocol(const char* name) {
        for (auto* proto : protocols) {
            if (strcmp(proto->getName(), name) == 0) {
                return proto;
            }
        }
        return nullptr;
    }

    /**
     * Auto-detect protocol from captured RMT data
     * Attempts all registered decoders in sequence
     *
     * @param data RMT items array
     * @param len Number of RMT items
     * @param result Output: decode result (only valid if return is not nullptr)
     * @return Protocol that successfully decoded the signal, or nullptr if none matched
     */
    SubGhzProtocol* autoDetect(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) {
        if (data == nullptr || len == 0) {
            return nullptr;
        }

        // Try each protocol decoder
        for (auto* proto : protocols) {
            // Skip if signal is too short for this protocol
            size_t min_len = proto->getMinimumLength();

            if (len < min_len) {
                continue;
            }

            // Attempt decode
            if (proto->decode(data, len, result)) {
                Serial.printf("[Registry] Detected %s (key=0x%llX, bits=%d)\n",
                             proto->getName(), result.key, result.bit_count);
                return proto;
            }
        }

        return nullptr;
    }

    /**
     * Auto-detect protocol from edge events (Flipper Zero style)
     * Feeds edges sequentially to each decoder's state machine
     */
    SubGhzProtocol* autoDetectEdges(const std::pair<bool, uint32_t>* edges, size_t edge_count, ProtocolDecodeResult& result) {
        if (edges == nullptr || edge_count == 0) {
            return nullptr;
        }

        // Try each protocol decoder
        for (auto* proto : protocols) {
            // Reset decoder state
            proto->reset();

            // Feed all edges to the decoder
            for (size_t i = 0; i < edge_count; i++) {
                proto->feed(edges[i].first, edges[i].second);

                // Check if decoder has a result
                if (proto->decode_check(result)) {
                    Serial.printf("[Registry] Detected %s (key=0x%llX, bits=%d)\n",
                                 proto->getName(), result.key, result.bit_count);
                    return proto;
                }
            }
        }

        return nullptr;
    }

    /**
     * Get list of all registered protocol names
     */
    std::vector<const char*> getProtocolNames() {
        std::vector<const char*> names;
        for (auto* proto : protocols) {
            names.push_back(proto->getName());
        }
        return names;
    }

    /**
     * Get count of registered protocols
     */
    size_t getProtocolCount() const {
        return protocols.size();
    }

    /**
     * Clear all registered protocols
     * Deletes all protocol instances
     */
    void clear() {
        for (auto* proto : protocols) {
            delete proto;
        }
        protocols.clear();
    }

    /**
     * Destructor - cleans up all protocols
     */
    ~ProtocolRegistry() {
        clear();
    }
};

// Global convenience functions

/**
 * Get the global protocol registry instance
 */
inline ProtocolRegistry* getProtocolRegistry() {
    return ProtocolRegistry::getInstance();
}

/**
 * Register a protocol with the global registry
 */
inline void registerProtocol(SubGhzProtocol* protocol) {
    ProtocolRegistry::getInstance()->registerProtocol(protocol);
}

/**
 * Get protocol by name from global registry
 */
inline SubGhzProtocol* getProtocol(const char* name) {
    return ProtocolRegistry::getInstance()->getProtocol(name);
}

/**
 * Auto-detect protocol from global registry (RMT items)
 */
inline SubGhzProtocol* autoDetectProtocol(const rmt_item32_t* data, size_t len, ProtocolDecodeResult& result) {
    return ProtocolRegistry::getInstance()->autoDetect(data, len, result);
}

/**
 * Auto-detect protocol from edge events (Flipper-style)
 * Edge format: pair<level (bool), duration (us)>
 */
inline SubGhzProtocol* autoDetectProtocolEdges(const std::pair<bool, uint32_t>* edges, size_t edge_count, ProtocolDecodeResult& result) {
    return ProtocolRegistry::getInstance()->autoDetectEdges(edges, edge_count, result);
}

#endif // __PROTOCOL_REGISTRY_H__
