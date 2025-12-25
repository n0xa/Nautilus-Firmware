#!/bin/bash
# Manual script to create merged nautilus.bin from individual firmware files
# This is normally done automatically by copy_firmware.py during build

set -e

FIRMWARE_DIR="webflasher/firmware"
OUTPUT_FILE="$FIRMWARE_DIR/nautilus.bin"

# Check if firmware files exist
if [ ! -f "$FIRMWARE_DIR/bootloader.bin" ] || \
   [ ! -f "$FIRMWARE_DIR/partitions.bin" ] || \
   [ ! -f "$FIRMWARE_DIR/firmware.bin" ]; then
    echo "Error: Missing firmware files in $FIRMWARE_DIR"
    echo "Please build firmware first with: pio run"
    exit 1
fi

# Check if esptool.py is available
if ! command -v esptool.py &> /dev/null; then
    echo "Error: esptool.py not found"
    echo "Install with: pip install esptool"
    exit 1
fi

echo "Creating merged binary..."

# Create merged binary
esptool.py --chip esp32s3 merge_bin \
    -o "$OUTPUT_FILE" \
    --flash_mode dio \
    --flash_freq 80m \
    --flash_size 16MB \
    0x0 "$FIRMWARE_DIR/bootloader.bin" \
    0x8000 "$FIRMWARE_DIR/partitions.bin" \
    0x10000 "$FIRMWARE_DIR/firmware.bin"

if [ $? -eq 0 ]; then
    SIZE=$(du -h "$OUTPUT_FILE" | cut -f1)
    echo "✓ Successfully created $OUTPUT_FILE ($SIZE)"
    echo ""
    echo "Flash with:"
    echo "  esptool.py --chip esp32s3 write_flash 0x0 $OUTPUT_FILE"
    echo ""
    echo "Or use with ESP32 Launcher and other single-file flash tools"
else
    echo "✗ Failed to create merged binary"
    exit 1
fi
