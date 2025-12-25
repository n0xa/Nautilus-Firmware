#ifndef SUBGHZ_REMOTE_H
#define SUBGHZ_REMOTE_H

#include <Arduino.h>
#include <vector>
#include <SD.h>

/**
 * Single SubGHz remote button
 * Each button has a name and points to a .sub file
 */
struct SubGHzButton {
    String name;        // Button name (e.g., "On", "Off", "Doorbell")
    String filepath;    // Path to .sub file (e.g., "/rf/On.sub")

    // Constructor
    SubGHzButton() : name(""), filepath("") {}
    SubGHzButton(const String& n, const String& f) : name(n), filepath(f) {}
};

/**
 * SubGHz Remote - collection of buttons in a single .txt file
 * Stored in /sgremotes directory
 */
struct SubGHzRemote {
    String filepath;                    // Path to .txt remote config file
    std::vector<SubGHzButton> buttons;  // List of buttons

    // Constructor
    SubGHzRemote() : filepath("") {}

    // Helper methods
    void clear() {
        filepath = "";
        buttons.clear();
    }

    int size() const {
        return buttons.size();
    }

    void addButton(const SubGHzButton& button) {
        buttons.push_back(button);
    }
};

// Function declarations

/**
 * Parse a Nautilus SubGHz Remote .txt file
 * Format:
 *   Filetype: Nautilus SubGHz Remote file
 *   Version: 1
 *   #
 *   name: ButtonName
 *   file: /rf/signal.sub
 *   #
 *   name: AnotherButton
 *   file: /rf/another.sub
 *
 * @param filepath Path to .txt remote config file
 * @param remote SubGHzRemote structure to populate
 * @return true if successful, false on error
 */
bool parseSubGHzRemoteFile(const char* filepath, SubGHzRemote& remote);

/**
 * Write a Nautilus SubGHz Remote .txt file
 * @param filepath Path to .txt file
 * @param remote SubGHzRemote structure to write
 * @return true if successful, false on error
 */
bool writeSubGHzRemoteFile(const char* filepath, const SubGHzRemote& remote);

/**
 * Create an empty SubGHz remote .txt file
 * @param filepath Path to create file at (in /sgremotes)
 * @return true if created successfully
 */
bool createEmptySubGHzRemote(const char* filepath);

/**
 * Append a button to an existing .txt remote file
 * @param filepath Path to .txt file
 * @param button SubGHzButton to append
 * @return true if successful
 */
bool appendButtonToSubGHzRemote(const char* filepath, const SubGHzButton& button);

/**
 * Replace a button at a specific index in a .txt remote file
 * @param filepath Path to .txt file
 * @param index Index of button to replace (0-based)
 * @param button New SubGHzButton
 * @return true if successful
 */
bool replaceButtonInSubGHzRemote(const char* filepath, int index, const SubGHzButton& button);

/**
 * Delete a button at a specific index from a .txt remote file
 * @param filepath Path to .txt file
 * @param index Index of button to delete (0-based)
 * @return true if successful
 */
bool deleteButtonFromSubGHzRemote(const char* filepath, int index);

/**
 * Generate sequential remote filename (Remote_0.txt, Remote_1.txt, etc.)
 * Checks /sgremotes directory for existing files
 * @return String with available filename (not full path, just filename)
 */
String generateSubGHzRemoteName();

/**
 * Generate sequential button name (Btn_0, Btn_1, etc.) for a given remote
 * @param remote SubGHzRemote to check existing button names
 * @return String with available button name
 */
String generateSubGHzButtonName(const SubGHzRemote& remote);

/**
 * Validate that a .sub file exists and is readable
 * @param filepath Path to .sub file
 * @return true if file exists and can be opened
 */
bool validateSubFile(const char* filepath);

#endif // SUBGHZ_REMOTE_H
