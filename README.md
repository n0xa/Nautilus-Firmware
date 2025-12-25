# Nautilus Firmware User Manual

**Hardware:** LilyGo T-Embed CC1101

---

## Table of Contents

1. [Introduction](#introduction)
2. [History](#history)
3. [Flashing the Firmware](#flashing-the-firmware)
4. [Installing Launcher](#installing-launcher)
5. [Navigation Guide](#navigation-guide)
6. [SD Card Setup](#sd-card-setup)
7. [Flipper Zero Compatibility](#flipper-zero-compatibility)
8. [Configuration (nautilus.json)](#configuration-nautilusjson)
9. [Applications](#applications)
10. [Tips and Tricks](#tips-and-tricks)
11. [Troubleshooting](#troubleshooting)

---

## Introduction

Nautilus is a custom firmware for the LilyGo T-Embed CC1101 that brings Flipper Zero-like functionality to your device. It supports:

- **Sub-GHz** radio transmission/reception (300-928 MHz)
- **Infrared** capture and playback
- **NFC** tag reading (13.56 MHz)
- **WiFi** scanning and analysis
- **Portal** captive portal for security testing
- **File management** with SD card support
- **LED control** (WS2812)
- **Audio playback**

---

## History

Nautilus was originally conceived as an extension of the [LilyGo T-Embed CC1101 Factory Test](https://github.com/Xinyuan-LilyGO/T-Embed-CC1101/tree/master/examples/factory_test) firmware. I had tried using several popular firmwares for this device, and was not satisfied with the user interfaces nor the technical capabilities. In contrast, I thought the Factory Test was modern and aesthetically pleasing. I wanted to learn the deep details of working with the CC1101 transceiver at a low level, and this device seemed like a great one to expand my ESP32 skills beyond the M5Stack devices I've spent most of my time working on. As I got more comfortable and confident, I tried to build firmware to turn this into a device I really wanted to carry with me. Where possible, I analyzed [Momentum-Firmware](https://github.com/Next-Flip/Momentum-Firmware) for the Flipper Zero to ensure files are cross-compatible. The larger Flipper Zero community has also done some great analysis of RF signals, and of course I'm also familiar with older groundbreaking work from the SDR community, such as [argilo/secplus](https://github.com/argilo/secplus). Inspired by those, I've created some thoughtful reimplementations of a few key SubGHz protocols that I encounter most often. Most of these required several hours of research and experimentation with tools like HackRF One + Universal Radio Hacker.  

Like most of my projects, this started out as a personal challenge to myself just to see if I could do it. Perhaps you'll like it as much as I do.

---

## Flashing the Firmware

### Method 1: Web Flasher (Easiest)

1. **Visit the web flasher:**
   - Navigate to: **[nautilus.h-i-r.net](https://nautilus.h-i-r.net)**
   - Or use: **[https://bmorcelli.github.io/Launcher/webflasher.html](https://bmorcelli.github.io/Launcher/webflasher.html)**

2. **Put device in bootloader mode** (if flashing fails):
   - Hold the **BOOT** button (center of encoder)
   - While holding BOOT, press and release the **RESET** button (back of device)
   - Release the **BOOT** button
   - Device is now in bootloader mode

3. **Connect and flash:**
   - Click "Connect Device" in the web browser
   - Select your device's COM port
   - Click "Flash Firmware"
   - Wait for completion (2-5 minutes)

4. **Reboot:**
   - After flashing completes, press the **RESET** button on the back of the device

### Bootloader Mode Tips

If the device won't enter flash mode automatically:

1. **Symptom:** Web flasher or PlatformIO can't detect device
2. **Solution:** Manually enter bootloader mode:
   - :one: Press and hold **BOOT** button (don't release)
   - :two: Press and release **RESET** button
   - :three: Release **BOOT** button
3. **Verify:** Device should now be detected by flasher

---

## Installing Launcher

The **Launcher** allows you to switch between multiple firmware images stored on your SD card without reflashing.

### Step 1: Flash Launcher to Device

1. Visit: **[https://bmorcelli.github.io/Launcher/webflasher.html](https://bmorcelli.github.io/Launcher/webflasher.html)**
2. Put device in bootloader mode (if needed - see above)
3. Flash the **Launcher** firmware to your device

### Step 2: Download 
Download `nautilus.bin` from the link on the Nautilus Web Flasher page

### Step 3: Copy to SD Card

1. Insert SD card into computer
2. (Optional) Create a folder for firmware files: `/firmware/` (if it doesn't exist)
3. Copy `nautilus.bin` to the appropriate folder on your SD card
4. Eject SD card and insert into device

### Step 4: Launch from Menu

1. Power on device with Launcher firmware
2. Navigate to **Apps** menu
3. Select **nautilus.bin**
4. Device will boot into Nautilus firmware

---

## Navigation Guide

### Hardware Controls

| Control | Function |
|---------|----------|
| **Rotary Encoder** | Scroll through menus and options |
| **Encoder Press** | Select highlighted item |
| **User Button** (top side) | Back / Sleep device |

### On-Screen UI Elements

#### The "<" Back Button

- **Location:** Top-left corner of most screens
- **Function:** Navigate back to previous screen
- **Access:** Scroll to it with encoder, then press 

#### Hardware User Button

- **Location:** Physical button on top edge of device
- **Functions:**
  - **Single press:** Go back one screen
  - **Long press:** Sleep device
  - **Press during sleep:** Wake device

#### Long-Press Actions

Many items support **long-press** for additional options:

- **Sub-GHz files:** Long-press to Rename/Delete
- **IR buttons:** Long-press to Edit/Delete
- **Remote files:** Long-press for context menu
- **Signal transmission:** Long-press to continuously transmit

#### On-Screen Keyboard

When entering text (e.g., naming files, setting frequencies):

1. **Encoder rotation:** Move cursor between characters
2. **Encoder press:** Select character
3. **Navigate to OK:** Press encoder to confirm
4. **Navigate to Cancel:** Press encoder to abort

**Keyboard tips:**
- Special characters available via character selection
- Use encoder to quickly navigate
- Look for "Shift" button to toggle uppercase

---

## SD Card Setup

### Required SD Card

- **Type:** microSD card
- **Size:** Up to 32GB (FAT32 formatted)
- **Recommended brand:** SanDisk
- **Note:** Some cards may not work - use SanDisk for best compatibility

### Directory Structure

After first boot, Nautilus creates these folders:

```
/
├── nautilus.json          # Main configuration file
├── ir/                    # Infrared signal files (.ir)
├── rf/                    # Sub-GHz signal files (.sub)
├── sgremotes/             # Sub-GHz remote definitions (.txt)
├── nfc/                   # NFC tag dumps
├── remotes/               # IR remote definitions
├── portal/                # Captive portal files
│   ├── index.html        # Custom portal page (optional)
│   ├── post.html         # Post-login page (optional)
│   └── creds.txt         # Captured credentials (auto-created)
├── music/                 # Audio files for playback
```

### Mounting and Unmounting

**Note:** The SD card should always mounted during normal operation.

#### Browsing Files

1. Navigate to **SD Card** app from main menu
2. Browse folders and files
3. Supported actions:
   - View `.txt` files
   - Play `.mp3` audio files
   - Navigate directories

#### Safely Removing SD Card

1. Unmount the SD Card from the SD Card screen 
2. Remove SD card
3. Remain in the SD Card screen while card is removed for best results
4. Re-insert into device
5. Mount SD card 

SD Card may also be safely be removed and reinserted while in deep sleep mode (long-press user button)

---

## Flipper Zero Compatibility

Nautilus is designed for **Flipper Zero file format compatibility**.

### Sub-GHz Files (.sub)

Nautilus can create, read and transmit Flipper Zero `.sub` files.

**Compatible protocols:**
- Princeton
- CAME
- Security+ v1.0 (garage door openers)
- Security+ v2.0 (garage door openers)
- BinRAW (raw captures)

**Example `.sub` file format:**
```
Filetype: Flipper SubGhz Key File
Version: 1
Frequency: 315000000
Preset: FuriHalSubGhzPresetOok650Async
Protocol: Princeton
Bit: 24
Key: 00 00 00 00 00 45 AB CD
```

**Where to get files:**
- Capture signals using Nautilus Record feature
- Download from Flipper Zero repositories
- Copy from your Flipper Zero's SD card (`/subghz/`)

### Infrared Files (.ir)

Nautilus supports Flipper Zero `.ir` file format.

**Example `.ir` file format:**
```
Filetype: IR signals file
Version: 1
#
name: Power
type: raw
frequency: 38000
duty_cycle: 0.330000
data: 9357 4487 621 531 625 532 622...
#
name: Volume_Up
type: raw
frequency: 38000
duty_cycle: 0.330000
data: 9298 4529 662 482 665 485...
```

**Where to get files:**
- Capture IR signals using Nautilus IR Capture
- Download from Flipper Zero IR database
- Copy from Flipper Zero SD card (`/infrared/`)

### File Transfer

**From Flipper Zero to Nautilus:**
1. Remove SD card from Flipper Zero
2. Copy `.sub` files from `/subghz/` to Nautilus `/rf/`
3. Copy `.ir` files from `/infrared/` to Nautilus `/ir/`
4. Insert SD card into Nautilus device

**From Nautilus to Flipper Zero:**
1. Remove SD card from Nautilus
2. Copy `.sub` files from `/rf/` to Flipper `/subghz/`
3. Copy `.ir` files from `/ir/` to Flipper `/infrared/`
4. Insert SD card into Flipper Zero

---

## Configuration (nautilus.json)

The `nautilus.json` file stores all device settings.

### Location

`/nautilus.json` (root of SD card)

### Full Structure

```json
{
  "version": "1.0",
  "ws2812": {
    "color": 65280,
    "brightness": 1,
    "mode": 0
  },
  "wifi": {
    "ssid": "",
    "password": "",
    "portal": {
      "ssid": "Nautilus WiFi"
    }
  },
  "subghz": {
    "custom_frequencies": [],
    "mod": "am650",
    "raw": {
      "last_frequency": 315
    },
    "scan": {
      "thresh": -60,
      "type": "custom",
      "range": "300-928"
    }
  },
  "display": {
    "rotation": 3,
    "theme": 0
  },
  "audio": {
    "volume": 8
  }
}
```

### Configuration Options

#### WS2812 LED Settings

```json
"ws2812": {
  "color": 65280,        // RGB color (0-16777215)
  "brightness": 1,       // 0-255
  "mode": 0              // Animation mode
}
```

#### WiFi Settings

```json
"wifi": {
  "ssid": "MyNetwork",       // WiFi network name
  "password": "MyPassword",  // WiFi password
  "portal": {
    "ssid": "Free WiFi"      // Portal hotspot name
  }
}
```

#### Sub-GHz Settings

```json
"subghz": {
  "custom_frequencies": [315000000, 433920000],  // Custom freq list (Hz)
  "mod": "am650",                                 // Modulation: am650, am270, fm238, fm476
  "raw": {
    "last_frequency": 315                        // Last used freq (MHz)
  },
  "scan": {
    "thresh": -60,                               // RSSI threshold (dBm)
    "type": "custom",                            // single, band, custom
    "range": "300-928"                           // Frequency range
  }
}
```

**Modulation options:**
- `am650` - AM (OOK) 650kHz bandwidth
- `am270` - AM (OOK) 270kHz bandwidth
- `fm238` - FM 238kHz deviation
- `fm476` - FM 476kHz deviation

#### Display Settings

```json
"display": {
  "rotation": 3,    // Screen rotation (0-3)
  "theme": 0        // 0=Dark, 1=Light
}
```

#### Audio Settings

```json
"audio": {
  "volume": 8      // 0-21 (higher = louder)
}
```

### Editing Configuration

1. Remove SD card from device
2. Insert into computer
3. Edit `nautilus.json` with text editor
4. **Validate JSON syntax** (use jsonlint.com)
5. Save and eject SD card
6. Re-insert into device and reboot

---

## Applications

### Sub-GHz (Sub-G)

Transmit and receive radio signals in the 300-928 MHz range.

#### Record Raw Signal

1. Navigate to **Sub-G > Record Raw**
2. Select frequency (or use last frequency)
3. Press encoder to start recording
4. Trigger the remote you want to capture
5. Press encoder to stop recording
6. Signal saved automatically to `/rf/capture_N.sub`

#### Scan/Record

1. Navigate to **Sub-G > Scan/Record**
2. Configure scan settings:
   - **Modulation:** AM270 / AM650 / FM238 / FM476 (press to cycle through them)
   - **Frequency Mode:** Single / Range / Custom
   - **RSSI Threshold:** -40 to -80 dBm (more negative = more sensitive; -80 captures weak signals, -40 requires stronger or closer transmissions)
3. Press **Start Scan**
4. Stop scanning and select "Save" to write the most recently identified signal to a .sub file
5. if **Auto** is selected, all detected signals are automatically saved

**Tip:** For best results detecting unknown signals:
- Use **Range** mode (e.g., 300-348 MHz)
- Set threshold to **-60 dBm**
- Try scanning multiple times in the same frequency range

#### Playback

1. Navigate to **Sub-G > Playback**
2. Shows files and folders in `/rf/` folder
3. Select a `.sub` file
4. **Single press encoder:** Preview signal details
5. **Press and hold encoder:** Transmit signal
6. **Keep holding:** Continuous transmission (for protocols that need it)

**Tips:** 
- Long-pressing is required for some garage door openers and gate systems.
- Supported rolling codes can be incremented and decremented for testing

#### Remotes

Create custom remote controls with multiple buttons.

1. Navigate to **Sub-G > Remotes**
2. Select existing remote or create new
3. **Add buttons:**
   - Select "(New Button)"
   - Browse to `.sub` file
   - Enter button name
4. **Use remote:**
   - Select button
   - Press encoder to transmit once
   - **Long-press to continuously transmit**

#### Custom Frequencies

Add your own frequencies for scanning/recording.

1. Navigate to **Sub-G > Custom Frequencies**
2. Select "(Add Frequency)"
3. Enter frequency in MHz (e.g., 315.00)
4. Frequency added to custom list
5. Available in frequency selectors

**Useful frequencies:**
- **315 MHz** - US garage doors, car remotes
- **433.92 MHz** - EU remotes, sensors
- **868 MHz** - EU alarm systems
- **915 MHz** - US ISM band

---

### Infrared (IR)

Capture and replay infrared remote control signals.

#### TV-B-Gone

Universal TV power-off transmitter.

1. Navigate to **Infrared > TV-B-Gone**
2. Select region: Americas or Europe/Middle East/Africa 
3. Press encoder to start transmission
4. Aim at TV (within 5 meters)
5. Cycles through all known TV power codes
6. Press encoder to stop

**Tip:** IR transmitter is on opposite end of the screen from the encoder dial. Point directly at TV sensor for best results.

#### IR Remotes

Create custom multi-button remotes.

1. Navigate to **Infrared > Remotes**
2. Create new remote or edit existing
3. **Add buttons:**
   - Capture new signal or select from file
   - Name the button (e.g., "Power", "Vol+")
4. **Use remote:**
   - Select button from list
   - Press to transmit
   - **Long-press for context menu (Edit/Delete/Test)**

---

### NFC (Beta)

Read and interact with 13.56 MHz RFID/NFC tags. Very basic support only at this time.

**Supported tag types:**
- Mifare Classic (1K, 4K)
- NTAG (203, 213, 215, 216)
- Other ISO14443A tags

**Not supported:**
- 125kHz low-frequency cards
- CPU cards / EMV credit cards
- Encrypted cards (Mifare DESFire, etc.)

#### Read Tag

1. Navigate to **NFC > Read Tag**
2. Place tag on side of device (NFC antenna is next to the screen opposite the user/back button))
3. Wait for detection
4. Tag information displayed:
   - UID
   - Tag type
   - Memory size
   - NDEF data (if present)

**Tip:** UI will freeze until an NFC tag is detected or will time out in 15 seconds

#### Save Tag

1. Read tag (see above)
2. Select "Save" option
3. Enter filename
4. Tag data saved to `/nfc/` folder

**Note:** NFC writing is not yet supported.

#### Tag Info

1. Select a saved tag from the NFC (Beta) screen.
2. Select the "Details" button to examine NDEF records.

---

### WiFi

Scan and analyze WiFi networks.

#### WiFi Scanner

1. Navigate to **WiFi > Scanner**
2. Press encoder to start scan
3. View detected networks:
   - SSID
   - Signal strength (RSSI)
   - Channel
   - Encryption type
4. **Select network:** View details

#### Deauth Hunter

Detect WiFi deauthentication attacks.

1. Navigate to **WiFi > Deauth Hunter**
2. Select threshold for # of packets to trigger the alarm and calibrate the RSSI meter. -20 dBm is usually good.
3. Press start
4. Deauth attacks are shown in real-time
5. Press the pause button to lock the channel and stop the alarm
6. Use the RSSI meter to help you find the source of the signal

**Use case:** Detect WiFi jamming attacks

#### PineAP Hunter

Detect rogue access points (PineAP/WiFi Pineapple).

1. Navigate to **WiFi > PineAP Hunter**
2. Select threshold for # of SSIDs on a single access point (recommend 3 or 5), and calibrate the RSSI meter.
3. Start scan. Green light shines while scanning. UI will be unresponsive while lit up.
4. Multiple scans are required to make a positive pineapple identification. Alarm will sound.
5. Suspicious BSSIDs highlighted
6. Select a BSSID to stop the alarm, view SSIDs and RSSI meter.
7. Use the RSSI meter to help you find the source of the signal.

---

### Portal (Security Testing)

**WARNING:** For authorized security testing only (pentests, CTFs, education).

Create a WiFi captive portal.

#### Starting the Portal

1. Navigate to **Portal** from main menu
2. Portal SSID shown (configured in `nautilus.json`)
3. Press **Start Portal**
4. Device creates WiFi hotspot
5. Credentials saved to `/portal/creds.txt`

#### Viewing Captured Data

1. While portal running, navigate to **View Data**
2. Credentials displayed on screen
3. Also accessible at: `http://172.0.0.1/creds`

#### Custom Portal Pages

1. Create `/portal/index.html` and `/portal/post.html` on SD card
2. Design your own HTML form (login page, survey, announcements, etc)
3. POST form data to `/post` endpoint
4. All form variables will be stored in `/portal/post.json`
5. Users will be redirected to `/portal/post.html` 

**Example HTML:**
```html
<form method="POST" action="/post">
  <input name="email" placeholder="Email">
  <input name="password" type="password" placeholder="Password">
  <button type="submit">Login</button>
</form>
```

#### Stopping the Portal

1. Navigate back in Portal menu
2. Select **Stop Portal**
3. WiFi hotspot disabled

#### Changing Portal SSID

**Option 1:** Edit SSID Directly
1. Select "Edit" button on the Nautilus Portal screen
2. Use On-Screen Keyboard to change the SSID  
Your new SSID will be saved to Nautilus.json

**Option 2:**
Edit `nautilus.json`:
```json
"wifi": {
  "portal": {
    "ssid": "Coffee Shop Free WiFi"
  }
}
```

**Authorization Required:** Only use in:
- Authorized penetration tests
- CTF competitions
- Security education/research
- With explicit permission

---

### SD Card Browser

Browse and manage files on SD card.

1. Navigate to **SD Card** from main menu
2. Browse folders
3. File actions:
   - **Text files (.txt/.html/.json):** View contents
   - **Audio files (.mp3):** Play audio
   - **SubGHz files (.sub):** Jump to SubGHz app with file pre-loaded
   - **Navigate folders:** Select to enter
   - **Long-press for context menu**

---

### Settings / Battery

#### Battery Information

1. Navigate to **Battery** from main menu
2. View battery status:
   - State of charge (%)
   - Voltage
   - Current
   - Temperature
   - Charging status
   - Time remaining

#### WS2812 LED Control

1. Navigate to **WS2812** from main menu
2. Adjust settings:
   - **Color wheel:** Select RGB color
   - **Brightness:** 0-255
   - **Mode:** Animation patterns

Changes saved to `nautilus.json`.

#### Display Settings

1. Navigate to **Settings** from main menu
2. Configure:
   - **Screen rotation:** 0-3 (90° increments)
   - **Theme:** Dark / Light
   - **SD card info**

---

## Tips and Tricks

### Sub-GHz

**Long-press to transmit:**
- Most Sub-GHz remotes (garage doors, gates) require long-press
- Hold encoder while transmitting
- Release to stop transmission

**Try receiving multiple times:**
- If a signal shows as "RAW" protocol:
  - Capture the same signal 2-3 times
  - This helps protocol detection
  - System learns pattern recognition

**Frequency selection:**
- **US:** 315 MHz, 390 MHz, 915 MHz
- **EU:** 433.92 MHz, 868 MHz
- **General:** Try 315 and 433.92 first

**RSSI threshold:**
- **-40 dBm:** Very strong signals only (close range)
- **-60 dBm:** Balanced (recommended)
- **-80 dBm:** Very sensitive (may pick up noise)

### Infrared

**Capture distance:**
- Hold remote 5-15 cm from device
- Point at top edge (IR receiver location)
- Avoid bright sunlight interference

### NFC

**Tag placement:**
- Place card on side of device (antenna is next to SD card slot)
- Hold steady for 1-2 seconds

**Reading problems:**
- Remove phone case if using phone NFC
- Try different positions near NFC antenna
- Some metal cases block NFC

### General

**Save battery:**
- Lower WS2812 LED brightness
- Long-press user button to sleep

**SD card:**
- Use SanDisk brand for reliability
- Format as FAT32
- Max size: 32GB
- Back up important files regularly

**Encoder tips:**
- Rotate slowly for precise control
- Quick rotation for fast scrolling
- Press firmly to select

---

## Troubleshooting

### SD Card Not Detected

**Symptoms:**
- "SD card not found" error
- Files not appearing

**Solutions:**
1. Try a different SD card (SanDisk recommended)
2. Reformat card as FAT32
3. Use 32GB or smaller capacity
4. Check card is fully inserted
5. Reboot device after inserting card

### Flashing Failed

**Symptoms:**
- Web flasher can't connect
- Upload timeout errors

**Solutions:**
1. Enter bootloader mode manually (see Flashing section)
2. Try different USB cable (use data cable, not charge-only)
3. Try different USB port
4. Close other serial programs (Arduino IDE, etc.)
5. Restart computer

### Screen Won't Turn On

**Symptoms:**
- Device powers on but screen black

**Solutions:**
1. Press RESET button on back of device
2. Check battery is charged
3. Connect USB power
4. Try entering bootloader mode and reflashing

### Sub-GHz Not Transmitting

**Symptoms:**
- Files load but nothing transmits

**Solutions:**
1. Check frequency matches your region
2. Verify `.sub` file format is correct
3. Try long-pressing encoder (required for some protocols)
4. Check antenna connection

### NFC Not Reading

**Symptoms:**
- Tags not detected

**Solutions:**
1. Place tag directly over antenna (next to SD card slot)
2. Hold tag steady for 2+ seconds
3. Verify tag type is supported (Mifare Classic, NTAG)
4. Try different tag
5. Check PN532 is initialized (main menu shows NFC option)

### IR Not Working

**Symptoms:**
- Signals captured but won't control devices

**Solutions:**
1. Point directly at device IR sensor
2. Try from closer distance (1-2 meters)
3. Use long-press for volume/channel controls
4. Verify frequency is 38kHz (most common)
5. Some devices use uncommon protocols

### Configuration Not Saving

**Symptoms:**
- Settings reset after reboot

**Solutions:**
1. Verify `nautilus.json` exists on SD card
2. Check JSON syntax (use jsonlint.com)
3. Ensure SD card is not write-protected
4. Format SD card and try again

### WiFi Portal Not Starting

**Symptoms:**
- Portal won't create hotspot

**Solutions:**
1. Check `nautilus.json` has portal SSID configured
2. Verify `/portal/` folder exists on SD card
3. Disconnect from any WiFi networks first
4. Reboot device and try again

---

## Additional Resources

### Legal Notice

**Sub-GHz and IR transmission:**
- Ensure compliance with local RF regulations
- Do not interfere with emergency or licensed frequencies
- Use responsibly and legally

**Portal feature:**
- For authorized security testing only
- Requires explicit permission
- Educational and defensive use only

**NFC:**
- Do not clone access cards without authorization
- Respect privacy and security policies

---

## Credits

**Nautilus Firmware:**
- Based on LilyGo T-Embed-CC1101 factory firmware
- Flipper Zero protocol implementations
- Community contributions and beta testers

**Hardware:**
- LilyGo T-Embed CC1101
- ESP32-S3 microcontroller
- CC1101 Sub-GHz transceiver
- PN532 NFC module

---

**Document Version:** 1.0
**Last Updated:** 2025-12-24

