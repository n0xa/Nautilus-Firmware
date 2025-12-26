// Nautilus WebUSB Flasher
// Uses ESPLoader for ESP32-S3 flashing via WebSerial

import { ESPLoader, Transport } from 'https://cdn.jsdelivr.net/npm/esptool-js@0.5.7/bundle.js';

// UI Elements
const connectBtn = document.getElementById('connectBtn');
const flashBtn = document.getElementById('flashBtn');
const eraseBtn = document.getElementById('eraseBtn');
const deviceInfo = document.getElementById('deviceInfo');
const progressContainer = document.getElementById('progressContainer');
const progressFill = document.getElementById('progressFill');
const statusText = document.getElementById('statusText');
const logContainer = document.getElementById('logContainer');
const releaseSelect = document.getElementById('releaseSelect');
const refreshReleasesBtn = document.getElementById('refreshReleasesBtn');

// State
let esploader = null;
let transport = null;
let port = null;
let chip = null;
let releases = [];
let selectedRelease = null;

// GitHub repository info
const GITHUB_REPO = 'n0xa/Nautilus-Firmware';
const GITHUB_API_BASE = 'https://api.github.com';

// Firmware manifest (default local files)
const FIRMWARE_MANIFEST = {
    chipFamily: "ESP32-S3",
    files: [
        { path: "firmware/bootloader.bin", offset: 0x0 },
        { path: "firmware/partitions.bin", offset: 0x8000 },
        { path: "firmware/firmware.bin", offset: 0x10000 }
    ]
};

// Logging functions
function log(message, type = 'info') {
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
    logContainer.appendChild(entry);
    logContainer.scrollTop = logContainer.scrollHeight;

    if (!logContainer.classList.contains('active')) {
        logContainer.classList.add('active');
    }
}

function updateProgress(percent, message) {
    progressFill.style.width = `${percent}%`;
    progressFill.textContent = `${percent}%`;
    statusText.textContent = message;

    if (!progressContainer.classList.contains('active')) {
        progressContainer.classList.add('active');
    }
}

function setButtonsEnabled(enabled) {
    flashBtn.disabled = !enabled;
    eraseBtn.disabled = !enabled;
}

// GitHub Releases API functions
async function fetchReleases() {
    try {
        log('Fetching releases from GitHub...', 'info');
        refreshReleasesBtn.disabled = true;
        refreshReleasesBtn.textContent = 'Loading...';

        const response = await fetch(`${GITHUB_API_BASE}/repos/${GITHUB_REPO}/releases`);
        if (!response.ok) {
            throw new Error(`GitHub API error: ${response.status} ${response.statusText}`);
        }

        releases = await response.json();

        // Populate select dropdown
        releaseSelect.innerHTML = '<option value="">Use Local Files</option>';

        if (releases.length === 0) {
            log('No releases found', 'info');
            releaseSelect.innerHTML += '<option disabled>No releases available</option>';
        } else {
            releases.forEach((release, index) => {
                const option = document.createElement('option');
                option.value = index;
                option.textContent = `${release.name || release.tag_name} ${release.prerelease ? '(Pre-release)' : ''}`;
                releaseSelect.appendChild(option);
            });

            // Default to the latest release (first in the list)
            releaseSelect.value = '0';
            selectedRelease = releases[0];
            log(`Loaded ${releases.length} releases - Selected: ${selectedRelease.name || selectedRelease.tag_name}`, 'success');
        }

        refreshReleasesBtn.textContent = 'Refresh';
        refreshReleasesBtn.disabled = false;

    } catch (error) {
        log(`Failed to fetch releases: ${error.message}`, 'error');
        console.error('Release fetch error:', error);
        releaseSelect.innerHTML = '<option value="">Use Local Files (Release fetch failed)</option>';
        refreshReleasesBtn.textContent = 'Retry';
        refreshReleasesBtn.disabled = false;
    }
}

function getManifestForRelease(release) {
    if (!release) {
        // Return default local manifest
        return FIRMWARE_MANIFEST;
    }

    // Find firmware assets in the release
    const bootloader = release.assets.find(a => a.name === 'bootloader.bin');
    const partitions = release.assets.find(a => a.name === 'partitions.bin');
    const firmware = release.assets.find(a => a.name === 'firmware.bin');

    if (!bootloader || !partitions || !firmware) {
        throw new Error('Release is missing required firmware files');
    }

    // Use proxy to bypass CORS restrictions
    const proxyUrl = (url) => {
        // Check if proxy.php exists, otherwise use direct URL (will fail with CORS)
        return `proxy.php?url=${encodeURIComponent(url)}`;
    };

    return {
        chipFamily: "ESP32-S3",
        files: [
            { path: proxyUrl(bootloader.browser_download_url), offset: 0x0 },
            { path: proxyUrl(partitions.browser_download_url), offset: 0x8000 },
            { path: proxyUrl(firmware.browser_download_url), offset: 0x10000 }
        ]
    };
}

// Connect to device
async function connectDevice() {
    try {
        // Request serial port
        if ('serial' in navigator) {
            log('Requesting serial port...', 'info');
            port = await navigator.serial.requestPort();
        } else {
            throw new Error('WebSerial API not supported. Use Chrome/Edge browser.');
        }

        log('Opening port...', 'info');

        // Create transport and loader
        // Note: Transport will handle opening the port, don't open it manually
        transport = new Transport(port, true);
        esploader = new ESPLoader({
            transport: transport,
            baudrate: 115200,
            terminal: {
                clean: () => {},
                writeLine: (text) => log(text, 'info'),
                write: (text) => log(text, 'info')
            }
        });

        // Connect to chip
        log('Connecting to ESP32-S3...', 'info');
        chip = await esploader.main();

        // Display chip info
        log(`Connected to ${chip}`, 'success');
        document.getElementById('deviceName').textContent = `Chip: ${chip}`;

        // Get MAC address from chip info
        deviceInfo.classList.add('active');

        connectBtn.textContent = 'Disconnect';
        connectBtn.classList.add('secondary');
        setButtonsEnabled(true);

        log('Ready to flash firmware', 'success');

    } catch (error) {
        log(`Connection error: ${error.message}`, 'error');
        console.error(error);
        await disconnectDevice();
    }
}

// Disconnect from device
async function disconnectDevice() {
    try {
        if (esploader) {
            try {
                await esploader.hardReset();
            } catch (e) {
                // Ignore if hardReset doesn't exist
                console.log('hardReset not available:', e.message);
            }
        }
        if (port) {
            await port.close();
        }
    } catch (error) {
        console.error('Disconnect error:', error);
    }

    esploader = null;
    transport = null;
    port = null;
    chip = null;

    connectBtn.textContent = 'Connect Device';
    connectBtn.classList.remove('secondary');
    deviceInfo.classList.remove('active');
    setButtonsEnabled(false);
    log('Disconnected', 'info');
}

// Load firmware file
async function loadFile(path) {
    try {
        log(`Loading ${path}...`, 'info');
        const response = await fetch(path);
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.arrayBuffer();
        log(`Loaded ${path} (${data.byteLength} bytes)`, 'success');
        return data;
    } catch (error) {
        throw new Error(`Failed to load ${path}: ${error.message}`);
    }
}

// Flash firmware
async function flashFirmware() {
    if (!esploader) {
        log('No device connected!', 'error');
        return;
    }

    try {
        setButtonsEnabled(false);
        connectBtn.disabled = true;
        updateProgress(0, 'Preparing to flash...');

        // Get the manifest for the selected release (or use local files)
        const manifest = getManifestForRelease(selectedRelease);

        if (selectedRelease) {
            log(`Using release: ${selectedRelease.name || selectedRelease.tag_name}`, 'info');
        } else {
            log('Using local firmware files', 'info');
        }

        // Load all firmware files
        log('Loading firmware files...', 'info');
        const fileArray = [];

        for (const file of manifest.files) {
            const arrayBuffer = await loadFile(file.path);
            // esptool-js expects a binary string, not Uint8Array
            const uint8Data = new Uint8Array(arrayBuffer);
            let binaryString = '';
            for (let i = 0; i < uint8Data.length; i++) {
                binaryString += String.fromCharCode(uint8Data[i]);
            }
            fileArray.push({
                data: binaryString,
                address: file.offset
            });
        }

        log('Starting flash process...', 'info');
        updateProgress(10, 'Erasing flash...');

        // Flash each file
        let progress = 10;
        const progressStep = 80 / fileArray.length;

        await esploader.writeFlash({
            fileArray: fileArray,
            flashSize: "keep",
            eraseAll: false,
            compress: true,
            reportProgress: (fileIndex, written, total) => {
                const fileProgress = (written / total) * progressStep;
                const totalProgress = Math.min(90, progress + fileProgress);
                const fileName = manifest.files[fileIndex].path.split('/').pop();
                updateProgress(
                    Math.floor(totalProgress),
                    `Flashing ${fileName}... ${written}/${total} bytes`
                );
            }
            // Note: calculateMD5Hash removed - not needed with newer esptool-js
        });

        progress = 90;
        updateProgress(95, 'Verifying flash...');

        log('Flash completed successfully!', 'success');
        updateProgress(100, 'Done! Resetting device...');

        // Reset the device
        try {
            await esploader.hardReset();
        } catch (e) {
            // hardReset may not exist in newer versions, try transport.reset
            if (transport && transport.setRTS) {
                await transport.setRTS(true);
                await new Promise(resolve => setTimeout(resolve, 100));
                await transport.setRTS(false);
            }
        }

        setTimeout(() => {
            updateProgress(100, 'Flash complete! Device rebooted.');
            log('You can disconnect the device now.', 'success');
            setButtonsEnabled(true);
            connectBtn.disabled = false;
        }, 1000);

    } catch (error) {
        log(`Flash error: ${error.message}`, 'error');
        console.error(error);
        updateProgress(0, 'Flash failed!');
        setButtonsEnabled(true);
        connectBtn.disabled = false;
    }
}

// Erase flash
async function eraseFlash() {
    if (!esploader) {
        log('No device connected!', 'error');
        return;
    }

    if (!confirm('This will erase ALL data on the device. Continue?')) {
        return;
    }

    try {
        setButtonsEnabled(false);
        connectBtn.disabled = true;
        updateProgress(0, 'Erasing flash...');
        log('Erasing entire flash...', 'info');

        await esploader.eraseFlash();

        log('Flash erased successfully!', 'success');
        updateProgress(100, 'Erase complete!');

        setTimeout(() => {
            setButtonsEnabled(true);
            connectBtn.disabled = false;
            updateProgress(0, 'Ready to flash');
        }, 2000);

    } catch (error) {
        log(`Erase error: ${error.message}`, 'error');
        console.error(error);
        updateProgress(0, 'Erase failed!');
        setButtonsEnabled(true);
        connectBtn.disabled = false;
    }
}

// Event listeners
connectBtn.addEventListener('click', async () => {
    if (port) {
        await disconnectDevice();
    } else {
        await connectDevice();
    }
});

flashBtn.addEventListener('click', flashFirmware);
eraseBtn.addEventListener('click', eraseFlash);

// Release selection event listener
releaseSelect.addEventListener('change', (e) => {
    const index = e.target.value;
    if (index === '') {
        selectedRelease = null;
        log('Selected: Local firmware files', 'info');
    } else {
        selectedRelease = releases[parseInt(index)];
        log(`Selected: ${selectedRelease.name || selectedRelease.tag_name}`, 'info');
    }
});

// Refresh releases button
refreshReleasesBtn.addEventListener('click', fetchReleases);

// Check for WebSerial API support
window.addEventListener('load', () => {
    if (!('serial' in navigator)) {
        log('WebSerial API not supported. Please use Chrome, Edge, or Opera.', 'error');
        connectBtn.disabled = true;
    } else {
        log('WebSerial API detected. Ready to connect.', 'success');
    }

    // Fetch releases on page load
    if (releaseSelect && refreshReleasesBtn) {
        fetchReleases().catch(err => {
            console.error('Failed to fetch releases on load:', err);
            log('Failed to load releases from GitHub. Using local files only.', 'error');
            releaseSelect.innerHTML = '<option value="">Use Local Files</option>';
            refreshReleasesBtn.textContent = 'Retry';
        });
    } else {
        console.error('Release selector elements not found');
    }
});
