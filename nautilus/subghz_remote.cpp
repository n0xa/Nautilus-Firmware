#include "subghz_remote.h"

// Expected directory for SubGHz remote configs
#define SUBGHZ_REMOTES_DIR "/sgremotes"

/**
 * Parse a Nautilus SubGHz Remote .txt file
 */
bool parseSubGHzRemoteFile(const char* filepath, SubGHzRemote& remote) {
    remote.clear();
    remote.filepath = filepath;

    // Open file directly (SD card is mounted at root)
    File file = SD.open(filepath, FILE_READ);
    if (!file) {
        Serial.printf("[SubGHz Remote] Failed to open: %s\n", filepath);
        return false;
    }

    // Parse file line by line
    String line;
    String current_name = "";
    String current_file = "";
    bool header_validated = false;
    int line_num = 0;

    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        line_num++;

        // Skip empty lines and comments (lines starting with #)
        if (line.length() == 0 || line.startsWith("#")) {
            // If we have a complete button (name + file), add it before moving on
            if (!current_name.isEmpty() && !current_file.isEmpty()) {
                SubGHzButton button(current_name, current_file);
                remote.addButton(button);
                current_name = "";
                current_file = "";
            }
            continue;
        }

        // Validate header on first line
        if (line_num == 1) {
            if (!line.startsWith("Filetype: Nautilus SubGHz Remote file")) {
                Serial.printf("[SubGHz Remote] Invalid header: %s\n", line.c_str());
                file.close();
                return false;
            }
            header_validated = true;
            continue;
        }

        // Parse key-value pairs
        int colon_pos = line.indexOf(':');
        if (colon_pos > 0) {
            String key = line.substring(0, colon_pos);
            String value = line.substring(colon_pos + 1);
            key.trim();
            value.trim();

            if (key.equalsIgnoreCase("Version")) {
                // Version check (currently only version 1 supported)
                int version = value.toInt();
                if (version != 1) {
                    Serial.printf("[SubGHz Remote] Unsupported version: %d\n", version);
                    file.close();
                    return false;
                }
            } else if (key.equalsIgnoreCase("name")) {
                current_name = value;
            } else if (key.equalsIgnoreCase("file")) {
                current_file = value;
            }
        }

        // If we have both name and file, add the button
        if (!current_name.isEmpty() && !current_file.isEmpty()) {
            SubGHzButton button(current_name, current_file);
            remote.addButton(button);
            current_name = "";
            current_file = "";
        }
    }

    // Add any remaining button
    if (!current_name.isEmpty() && !current_file.isEmpty()) {
        SubGHzButton button(current_name, current_file);
        remote.addButton(button);
    }

    file.close();
    Serial.printf("[SubGHz Remote] Loaded %s with %d buttons\n", filepath, remote.size());
    return header_validated;
}

/**
 * Write a Nautilus SubGHz Remote .txt file
 */
bool writeSubGHzRemoteFile(const char* filepath, const SubGHzRemote& remote) {
    // Try multiple SD mount points
    // Open file directly (SD card is mounted at root)
    File file = SD.open(filepath, FILE_WRITE);
    if (!file) {
        Serial.printf("[SubGHz Remote] Failed to write: %s\n", filepath);
        return false;
    }

    // Write header
    file.println("Filetype: Nautilus SubGHz Remote file");
    file.println("Version: 1");

    // Write each button
    for (const SubGHzButton& button : remote.buttons) {
        file.println("#");
        file.print("name: ");
        file.println(button.name);
        file.print("file: ");
        file.println(button.filepath);
    }

    file.close();
    Serial.printf("[SubGHz Remote] Wrote %s with %d buttons\n", filepath, remote.size());
    return true;
}

/**
 * Create an empty SubGHz remote .txt file
 */
bool createEmptySubGHzRemote(const char* filepath) {
    // Create /sgremotes directory if it doesn't exist
    if (!SD.exists("/sgremotes")) {
        SD.mkdir("/sgremotes");
        Serial.println("[SubGHz Remote] Created /sgremotes directory");
    }

    SubGHzRemote empty_remote;
    empty_remote.filepath = filepath;
    return writeSubGHzRemoteFile(filepath, empty_remote);
}

/**
 * Append a button to an existing .txt remote file
 */
bool appendButtonToSubGHzRemote(const char* filepath, const SubGHzButton& button) {
    // Load existing remote
    SubGHzRemote remote;
    if (!parseSubGHzRemoteFile(filepath, remote)) {
        Serial.printf("[SubGHz Remote] Failed to load for append: %s\n", filepath);
        return false;
    }

    // Add new button
    remote.addButton(button);

    // Write back
    return writeSubGHzRemoteFile(filepath, remote);
}

/**
 * Replace a button at a specific index
 */
bool replaceButtonInSubGHzRemote(const char* filepath, int index, const SubGHzButton& button) {
    // Load existing remote
    SubGHzRemote remote;
    if (!parseSubGHzRemoteFile(filepath, remote)) {
        Serial.printf("[SubGHz Remote] Failed to load for replace: %s\n", filepath);
        return false;
    }

    // Validate index
    if (index < 0 || index >= remote.size()) {
        Serial.printf("[SubGHz Remote] Invalid index for replace: %d (size: %d)\n", index, remote.size());
        return false;
    }

    // Replace button
    remote.buttons[index] = button;

    // Write back
    return writeSubGHzRemoteFile(filepath, remote);
}

/**
 * Delete a button at a specific index
 */
bool deleteButtonFromSubGHzRemote(const char* filepath, int index) {
    // Load existing remote
    SubGHzRemote remote;
    if (!parseSubGHzRemoteFile(filepath, remote)) {
        Serial.printf("[SubGHz Remote] Failed to load for delete: %s\n", filepath);
        return false;
    }

    // Validate index
    if (index < 0 || index >= remote.size()) {
        Serial.printf("[SubGHz Remote] Invalid index for delete: %d (size: %d)\n", index, remote.size());
        return false;
    }

    // Remove button
    remote.buttons.erase(remote.buttons.begin() + index);

    // If remote is now empty, delete the file entirely
    if (remote.size() == 0) {
        // Delete file directly (SD card is mounted at root)
        bool deleted = SD.remove(filepath);
        Serial.printf("[SubGHz Remote] Deleted empty remote: %s\n", filepath);
        return deleted;
    }

    // Write back modified remote
    return writeSubGHzRemoteFile(filepath, remote);
}

/**
 * Generate sequential remote filename
 */
String generateSubGHzRemoteName() {
    int index = 0;
    String base_path = String(SUBGHZ_REMOTES_DIR) + "/Remote_";

    // Check files directly (SD card is mounted at root)
    while (true) {
        String filename = "Remote_" + String(index) + ".txt";
        String full_path = base_path + String(index) + ".txt";

        if (!SD.exists(full_path.c_str())) {
            return filename;  // Return just the filename, not full path
        }

        index++;
    }
}

/**
 * Generate sequential button name
 */
String generateSubGHzButtonName(const SubGHzRemote& remote) {
    int index = 0;

    while (true) {
        String candidate = "Btn_" + String(index);

        // Check if this name already exists
        bool exists = false;
        for (const SubGHzButton& button : remote.buttons) {
            if (button.name.equalsIgnoreCase(candidate)) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            return candidate;
        }

        index++;
    }
}

/**
 * Validate that a .sub file exists and is readable
 */
bool validateSubFile(const char* filepath) {
    // Check file directly (SD card is mounted at root)
    if (SD.exists(filepath)) {
        File file = SD.open(filepath, FILE_READ);
        if (file) {
            file.close();
            return true;
        }
    }
    return false;
}
