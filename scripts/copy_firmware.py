#!/usr/bin/env python3
"""
PlatformIO extra script to copy firmware files for WebUSB flasher
This script copies the bootloader, partitions, and firmware binary files
to the webflasher directory after a successful build.
It also creates a merged nautilus.bin for single-file flashing tools.
"""

Import("env")
import os
import shutil
import subprocess

def copy_firmware_files(*args, **kwargs):
    """Copy firmware files to webflasher directory"""

    # Get the build directory and project directory
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")

    # Create webflasher directory if it doesn't exist
    webflasher_dir = os.path.join(project_dir, "webflasher", "firmware")
    os.makedirs(webflasher_dir, exist_ok=True)

    # Define the files to copy
    files_to_copy = [
        {
            "source": os.path.join(build_dir, "bootloader.bin"),
            "dest": os.path.join(webflasher_dir, "bootloader.bin"),
            "address": "0x0"
        },
        {
            "source": os.path.join(build_dir, "partitions.bin"),
            "dest": os.path.join(webflasher_dir, "partitions.bin"),
            "address": "0x8000"
        },
        {
            "source": os.path.join(build_dir, "firmware.bin"),
            "dest": os.path.join(webflasher_dir, "firmware.bin"),
            "address": "0x10000"
        }
    ]

    # Copy the files
    print("\n=== Copying firmware files for WebUSB flasher ===")
    for file_info in files_to_copy:
        if os.path.exists(file_info["source"]):
            shutil.copy2(file_info["source"], file_info["dest"])
            print(f"Copied: {os.path.basename(file_info['source'])} -> webflasher/firmware/")
        else:
            print(f"Warning: {file_info['source']} not found!")

    # Create a manifest file with flash addresses
    manifest_path = os.path.join(webflasher_dir, "manifest.json")
    manifest_content = """{{
  "name": "Nautilus Firmware",
  "version": "1.0.0",
  "builds": [
    {{
      "chipFamily": "ESP32-S3",
      "parts": [
        {{
          "path": "bootloader.bin",
          "offset": 0
        }},
        {{
          "path": "partitions.bin",
          "offset": 32768
        }},
        {{
          "path": "firmware.bin",
          "offset": 65536
        }}
      ]
    }}
  ]
}}"""

    with open(manifest_path, "w") as f:
        f.write(manifest_content)

    print(f"Created manifest: {manifest_path}")

    # Create merged binary for single-file flashing (ESP32 Launcher, etc.)
    merged_bin_path = os.path.join(webflasher_dir, "nautilus.bin")
    print("\n=== Creating merged binary ===")

    try:
        # Use esptool.py to merge the binaries
        # Format: esptool.py --chip esp32s3 merge_bin -o output.bin --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin

        bootloader_path = os.path.join(webflasher_dir, "bootloader.bin")
        partitions_path = os.path.join(webflasher_dir, "partitions.bin")
        firmware_path = os.path.join(webflasher_dir, "firmware.bin")

        if all(os.path.exists(f) for f in [bootloader_path, partitions_path, firmware_path]):
            merge_cmd = [
                "esptool.py",
                "--chip", "esp32s3",
                "merge_bin",
                "-o", merged_bin_path,
                "--flash_mode", "dio",
                "--flash_freq", "80m",
                "--flash_size", "16MB",
                "0x0", bootloader_path,
                "0x8000", partitions_path,
                "0x10000", firmware_path
            ]

            result = subprocess.run(merge_cmd, capture_output=True, text=True)

            if result.returncode == 0:
                file_size = os.path.getsize(merged_bin_path)
                file_size_mb = file_size / (1024 * 1024)
                print(f"✓ Created merged binary: nautilus.bin ({file_size_mb:.2f} MB)")
                print(f"  Flash with: esptool.py write_flash 0x0 nautilus.bin")
                print(f"  Or use with ESP32 Launcher and other single-file flashers")
            else:
                print(f"Warning: Failed to create merged binary")
                print(f"Error: {result.stderr}")
        else:
            print("Warning: Not all firmware files available for merging")

    except FileNotFoundError:
        print("Warning: esptool.py not found. Skipping merged binary creation.")
        print("Install with: pip install esptool")
    except Exception as e:
        print(f"Warning: Could not create merged binary: {e}")

    print("=== Firmware files ready for WebUSB flasher ===\n")

# Add the post-build action
env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware_files)
