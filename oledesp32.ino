#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Input pins ---
#define BTN_UP     2
#define BTN_DOWN   3
#define BTN_SELECT 4

// --- Menu state ---
int menuIndex = 0;
int screenState = 0; // 0 = Menu, 1 = Status, 2 = Settings, 3 = Info

const char* menuItems[] = {"Status", "Settings", "Info"};
const int menuLength = 3;

// --- Timing ---
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100; // refresh every 100ms

void setup() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Halt if OLED init fails
  }
  display.clearDisplay();
}

void loop() {
  // --- Handle input ---
  if(digitalRead(BTN_UP) == LOW) {
    menuIndex = (menuIndex - 1 + menuLength) % menuLength;
  }
  if(digitalRead(BTN_DOWN) == LOW) {
    menuIndex = (menuIndex + 1) % menuLength;
  }
  if(digitalRead(BTN_SELECT) == LOW) {
    screenState = menuIndex + 1; // map menu to screen
  }

  // --- Update display at interval ---
  unsigned long now = millis();
  if(now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    renderScreen();
  }
}

void renderScreen() {
  display.clearDisplay();
  if(screenState == 0) {
    drawMenu();
  } else if(screenState == 1) {
    drawStatus();
  } else if(screenState == 2) {
    drawSettings();
  } else if(screenState == 3) {
    drawInfo();
  }
  display.display();
}

// --- GUI functions ---
void drawMenu() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Menu:");

  for(int i=0; i<menuLength; i++) {
    display.setCursor(0, 12 + i*10);
    if(i == menuIndex) {
      display.print(">");
    } else {
      display.print(" ");
    }
    display.println(menuItems[i]);
  }
}

void drawStatus() {
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Status Screen");
  display.setCursor(0,12);
  display.println("WiFi: Connected");
  display.setCursor(0,22);
  display.println("Battery: 3.7V");
}

void drawSettings() {
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Settings Screen");
  display.setCursor(0,12);
  display.println("Brightness: 80%");
}

void drawInfo() {
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Info Screen");
  display.setCursor(0,12);
  display.println("Device: Wemos S2");
  display.setCursor(0,22);
  display.println("OLED: 128x32");
}
