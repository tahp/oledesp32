#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <WiFi.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Input pins ---
#define BTN_UP     2
#define BTN_DOWN   3
#define BTN_SELECT 4

#include <WebServer.h>
#include <Preferences.h>

// --- Web Server ---
WebServer server(80);
bool serverRunning = false;

// --- WiFi Auto-reconnect ---
Preferences preferences;
bool isReconnecting = false;

// --- Menu state ---
int menuIndex = 0;
int screenState = 0;

// --- Coms screen state ---
enum ComsState {
  COMS_DEFAULT,
  COMS_SCANNING,
  COMS_SCAN_RESULTS,
  COMS_ENTER_PASSWORD,
  COMS_CONNECTING,
  COMS_CONNECT_FAILED
};
ComsState comsScreenState = COMS_DEFAULT;
int selectedNetwork = 0;
int networkCount = 0;
String* foundNetworks = nullptr;
String currentPassword = "";
int charIndex = 0;

const char charSet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{};':\",./<>? `~";
const int charSetSize = sizeof(charSet) - 1;


const char* menuItems[] = {"Info", "Coms", "Conf", "?"};
const int menuLength = 4;

// --- Timing ---
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100;
unsigned long lastInput = 0;
const unsigned long inputInterval = 150;


// --- Forward declarations ---
void renderScreen();
void drawNav(int highlightIndex);
void drawMenu();
void drawInfo();
void drawComs();
void drawConfig();
void drawHelp();
void handleMainInput();
void handleComsInput();
void startScan();
void drawComsDefault();
void drawComsScanning();
void drawComsScanResults();
void drawComsEnterPassword();
void drawComsConnecting();
void drawComsConnectFailed();
void handleRoot();
void startServer();
void stopServer();
void saveWifiCredentials(const String& ssid, const String& password);
void autoConnectWifi();

void (*drawScreen[])() = {drawMenu, drawInfo, drawComs, drawConfig, drawHelp};

void setup() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Halt if OLED init fails
  }
  display.setRotation(2);  // 180 degrees
  display.clearDisplay();
  autoConnectWifi();
}

void loop() {
  unsigned long now = millis();

  // --- Handle auto-reconnect status ---
  if (isReconnecting) {
    if (WiFi.status() == WL_CONNECTED) {
      isReconnecting = false;
    } else if (millis() > 15000) { // 15s timeout
      isReconnecting = false;
      WiFi.disconnect();
    }
  }

  // --- Handle input ---
  if (now - lastInput > inputInterval) {
    if (screenState == 2) { // Coms screen
      handleComsInput();
    } else { // Main menu and other screens
      handleMainInput();
    }
  }

  // --- Web server ---
  if (WiFi.status() == WL_CONNECTED && !serverRunning) {
    startServer();
  } else if (WiFi.status() != WL_CONNECTED && serverRunning) {
    stopServer();
  }
  if(serverRunning) {
    server.handleClient();
  }


  // --- Update display at interval ---
  if(now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    renderScreen();
  }
}

void handleMainInput() {
  unsigned long now = millis();
    if(digitalRead(BTN_UP) == LOW) {
      if(screenState == 0 && !isReconnecting) {
        menuIndex = (menuIndex - 1 + menuLength) % menuLength;
      }
      lastInput = now;
    }
    if(digitalRead(BTN_DOWN) == LOW) {
      if(screenState == 0 && !isReconnecting) {
        menuIndex = (menuIndex + 1) % menuLength;
      }
      lastInput = now;
    }
    if(digitalRead(BTN_SELECT) == LOW) {
      if (screenState == 0 && !isReconnecting) {
        screenState = menuIndex + 1; // map menu to screen
      } else {
        screenState = 0; // Go back to menu
      }
      lastInput = now;
    }
}

void handleComsInput() {
  unsigned long now = millis();
  if (digitalRead(BTN_UP) == LOW) {
    if (comsScreenState == COMS_SCAN_RESULTS && networkCount > 0) {
      selectedNetwork = (selectedNetwork - 1 + networkCount) % networkCount;
    } else if (comsScreenState == COMS_ENTER_PASSWORD) {
      charIndex = (charIndex - 1 + (charSetSize + 2)) % (charSetSize + 2); // +2 for <-- and [OK]
    }
    lastInput = now;
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    if (comsScreenState == COMS_SCAN_RESULTS && networkCount > 0) {
      selectedNetwork = (selectedNetwork + 1) % networkCount;
    } else if (comsScreenState == COMS_ENTER_PASSWORD) {
      charIndex = (charIndex + 1) % (charSetSize + 2);
    }
    lastInput = now;
  }
  if (digitalRead(BTN_SELECT) == LOW) {
    switch(comsScreenState) {
      case COMS_DEFAULT:
        if (WiFi.status() != WL_CONNECTED) {
          comsScreenState = COMS_SCANNING;
          startScan();
        } else {
          screenState = 0; // Go back to main menu
        }
        break;
      case COMS_SCAN_RESULTS:
        if (networkCount > 0) {
          if (WiFi.encryptionType(selectedNetwork) == WIFI_AUTH_OPEN) {
            comsScreenState = COMS_CONNECTING;
            WiFi.begin(foundNetworks[selectedNetwork].c_str());
          } else {
            comsScreenState = COMS_ENTER_PASSWORD;
            currentPassword = "";
            charIndex = 0;
          }
        }
        break;
      case COMS_ENTER_PASSWORD:
        if (charIndex < charSetSize) { // A regular character
          currentPassword += charSet[charIndex];
        } else if (charIndex == charSetSize) { // Backspace
          if (currentPassword.length() > 0) {
            currentPassword.remove(currentPassword.length() - 1);
          }
        } else { // "OK"
          comsScreenState = COMS_CONNECTING;
          WiFi.begin(foundNetworks[selectedNetwork].c_str(), currentPassword.c_str());
        }
        break;
      case COMS_CONNECTING:
        // do nothing while connecting
        break;
      case COMS_CONNECT_FAILED:
        comsScreenState = COMS_DEFAULT;
        break;
      case COMS_SCANNING:
        // do nothing while scanning
        break;
    }
    lastInput = now;
  }
}

void startScan() {
  if (foundNetworks != nullptr) {
    delete[] foundNetworks;
    foundNetworks = nullptr;
    networkCount = 0;
  }
  networkCount = WiFi.scanNetworks();
  if (networkCount > 0) {
    foundNetworks = new String[networkCount];
    for (int i = 0; i < networkCount; i++) {
      foundNetworks[i] = WiFi.SSID(i);
    }
  }
  comsScreenState = COMS_SCAN_RESULTS;
}

void renderScreen() {
  display.clearDisplay();
  // Special handling for Coms screen state machine
  if (screenState == 2) {
    drawComs();
  } else {
    drawScreen[screenState]();
  }
  display.display();
}

// --- GUI functions ---
void drawNav(int highlightIndex) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int16_t x = 0;
  for (int i = 0; i < menuLength; i++) {
    if (i == highlightIndex) {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Highlight
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(x, 0);
    display.print(menuItems[i]);
    x += (strlen(menuItems[i]) * 6) + 12; // 6 is char width
  }
  display.setTextColor(SSD1306_WHITE);
  display.drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);
}

void drawMenu() {
  if (isReconnecting) {
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println("Reconnect");
    display.setTextSize(1);
    preferences.begin("wifi-creds", true);
    display.println("Connecting to:");
    display.println(preferences.getString("ssid", ""));
    preferences.end();
  } else {
    drawNav(menuIndex);
    display.setCursor(0, 15);
    display.setTextSize(2);
    display.println(menuItems[menuIndex]);
  }
}

void drawInfo() {
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println("Info");
  display.setTextSize(1);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("IP: ");
    display.println(WiFi.localIP());
  } else {
    display.println("WiFi Disconnected");
  }
}

void drawComs() {
  switch (comsScreenState) {
    case COMS_DEFAULT:
      drawComsDefault();
      break;
    case COMS_SCANNING:
      drawComsScanning();
      break;
    case COMS_SCAN_RESULTS:
      drawComsScanResults();
      break;
    case COMS_ENTER_PASSWORD:
      drawComsEnterPassword();
      break;
    case COMS_CONNECTING:
      drawComsConnecting();
      break;
    case COMS_CONNECT_FAILED:
      drawComsConnectFailed();
      break;
  }
}

void drawComsDefault() {
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println("WiFi");
  display.setTextSize(1);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("Connected to: ");
    display.println(WiFi.SSID());
  } else {
    display.println("Disconnected");
    display.setCursor(0, 24);
    display.println("> Scan for networks");
  }
}

void drawComsScanning() {
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println("Scanning...");
}

void drawComsScanResults() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Select Network:");
  if (networkCount == 0) {
    display.println("No networks found");
  } else {
    for (int i = 0; i < networkCount; i++) {
      if (i == selectedNetwork) {
        display.print("> ");
      } else {
        display.print("  ");
      }
      display.println(foundNetworks[i]);
    }
  }
}

void drawComsEnterPassword() {
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Pass for ");
  display.println(foundNetworks[selectedNetwork]);

  String hiddenPass = "";
  for (uint i = 0; i < currentPassword.length(); i++) {
    hiddenPass += '*';
  }
  display.println(hiddenPass);

  display.setCursor(0, 24);
  display.print("Char: ");
  if (charIndex < charSetSize) {
    display.print(charSet[charIndex]);
  } else if (charIndex == charSetSize) {
    display.print("<--");
  } else {
    display.print("[OK]");
  }
}


void drawComsConnecting() {
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Connecting to ");
  display.println(foundNetworks[selectedNetwork]);
  
  // This is a simplified connection check.
  // In a real app, you'd want a timeout.
  if(WiFi.status() == WL_CONNECTED) {
    comsScreenState = COMS_DEFAULT;
    saveWifiCredentials(foundNetworks[selectedNetwork], currentPassword);
  } else if (WiFi.status() == WL_CONNECT_FAILED) {
    comsScreenState = COMS_CONNECT_FAILED;
  }
}

void drawComsConnectFailed() {
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Connection Failed!");
    display.setCursor(0, 16);
    display.println("Press Select to go back.");
}

void drawConfig() {
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println("Config");
  display.setTextSize(1);
  display.println("This is the config page");
}

void drawHelp() {
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println("Help");
  display.setTextSize(1);
  display.println("UP/DOWN to nav");
  display.println("SELECT to choose/back");
}

void handleRoot() {
  long uptime = millis() / 1000;
  String html = "<html><head><title>ESP32 Monitor</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:sans-serif;text-align:center;}</style></head>";
  html += "<body><h1>Hello from your ESP32!</h1>";
  html += "<p>Uptime: " + String(uptime) + " seconds</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void startServer() {
  server.on("/", handleRoot);
  server.begin();
  serverRunning = true;
}

void stopServer() {
  server.stop();
  serverRunning = false;
}

void saveWifiCredentials(const String& ssid, const String& password) {
  preferences.begin("wifi-creds", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
}

void autoConnectWifi() {
  preferences.begin("wifi-creds", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  if (ssid.length() > 0) {
    isReconnecting = true;
    WiFi.begin(ssid.c_str(), password.c_str());
  }
}

