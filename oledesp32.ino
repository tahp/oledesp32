#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Buttons ---
#define BTN_UP     2   // Back / Up
#define BTN_DOWN   3   // Next / Move
#define BTN_SELECT 4   // Enter / Confirm

// --- Menu ---
const char* menuItems[] = {"Control", "Status", "Effects", "Info"};
const int menuLength = 4;
int menuIndex = 0;

// --- Navigation state ---
bool inPanel = false;   // false = top bar, true = inside panel
int screenState = 0;    // 0 = root, 1..4 = panels

// --- Panel field indices (for navigation inside panels) ---
int controlField = 0;
int effectField = 0;

// --- Timing ---
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100;

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------
void setup() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }

  display.clearDisplay();
  display.display();
}

// ------------------------------------------------------------
// MAIN LOOP
// ------------------------------------------------------------
void loop() {

  // --- BACK / UP ---
  if (digitalRead(BTN_UP) == LOW) {
    if (!inPanel) {
      // Move left in top bar
      menuIndex = (menuIndex - 1 + menuLength) % menuLength;
    } else {
      // Back to root
      inPanel = false;
      screenState = 0;
    }
  }

  // --- NEXT / MOVE ---
  if (digitalRead(BTN_DOWN) == LOW) {
    if (!inPanel) {
      // Move right in top bar
      menuIndex = (menuIndex + 1) % menuLength;
    } else {
      // Move between fields inside panel
      handlePanelMove();
    }
  }

  // --- ENTER / CONFIRM ---
  if (digitalRead(BTN_SELECT) == LOW) {
    if (!inPanel) {
      // Enter selected panel
      inPanel = true;
      screenState = menuIndex + 1;
    } else {
      // Confirm/toggle inside panel
      handlePanelSelect();
    }
  }

  // --- Update display ---
  unsigned long now = millis();
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    renderScreen();
  }
}

// ------------------------------------------------------------
// RENDERING
// ------------------------------------------------------------
void renderScreen() {
  display.clearDisplay();

  if (!inPanel) {
    drawTopBar();
    display.setCursor(0, 12);
    display.println("Select a menu item");
  } else {
    drawActivePanel();
  }

  display.display();
}

// ------------------------------------------------------------
// TOP BAR (root level)
// ------------------------------------------------------------
void drawTopBar() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  for (int i = 0; i < menuLength; i++) {
    if (i == menuIndex) {
      display.print("[");
      display.print(menuItems[i]);
      display.print("] ");
    } else {
      display.print(menuItems[i]);
      display.print(" ");
    }
  }
}

// ------------------------------------------------------------
// PANEL ROUTER
// ------------------------------------------------------------
void drawActivePanel() {
  display.setCursor(0, 0);

  if (screenState == 1) drawControl();
  else if (screenState == 2) drawStatus();
  else if (screenState == 3) drawEffects();
  else if (screenState == 4) drawInfo();
}

// ------------------------------------------------------------
// PANELS
// ------------------------------------------------------------
void drawControl() {
  display.println("Control Panel");
  display.setCursor(0, 12);
  display.print("Field: ");
  display.println(controlField);
}

void drawStatus() {
  display.println("Status Panel");
  display.setCursor(0, 12);
  display.println("WiFi: (placeholder)");
}

void drawEffects() {
  display.println("Effects Panel");
  display.setCursor(0, 12);
  display.print("Field: ");
  display.println(effectField);
}

void drawInfo() {
  display.println("Info Panel");
  display.setCursor(0, 12);
  display.println("Device: ESP32-S2");
  display.setCursor(0, 22);
  display.println("OLED: 128x32");
}

// ------------------------------------------------------------
// PANEL NAVIGATION
// ------------------------------------------------------------
void handlePanelMove() {
  if (screenState == 1) {
    controlField = (controlField + 1) % 3; // example: 3 fields
  }
  if (screenState == 3) {
    effectField = (effectField + 1) % 3; // example: 3 fields
  }
}

void handlePanelSelect() {
  if (screenState == 1) {
    // Control panel actions
    // (placeholder)
  }
  if (screenState == 3) {
    // Effects panel actions
    // (placeholder)
  }
}
