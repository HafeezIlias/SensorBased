#include <WiFi.h>
#include <EEPROM.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include "DHT.h"
#include <Adafruit_SSD1306.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

// === Firebase Configuration ===
#define API_KEY "AIzaSyDlcY8ulZWXQUfHhJHd0vbuUD0vM-6k4bA"
#define DATABASE_URL "https://sensorbased-16dee-default-rtdb.asia-southeast1.firebasedatabase.app/"

// === Firebase Helpers ===
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// === OLED ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === EEPROM ===
#define EEPROM_SIZE 128
#define SSID_ADDR 0
#define PASS_ADDR 32
#define USER_ADDR 64
#define DISPLAYTEXT_ADDR 96

// === Pins ===
#define DHTPIN 4
#define DHTTYPE DHT11
#define RELAY_PIN 2
#define CONFIG_BUTTON 0

// === Firebase & Web Objects ===
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
DNSServer dns;
AsyncWebServer server(80);
bool signupOK = false;

// === Flags ===
bool configMode = false;
DHT dht(DHTPIN, DHTTYPE);

// === Declare global variables ===
String displayText = "";  // Declare displayText globally
String user = "";  // Declare user globally
bool relayStatus = false;  // Store the relay status (true = HIGH, false = LOW)

// === EEPROM Helpers ===
void writeStringToEEPROM(int addr, const String &str) {
  for (int i = 0; i < 32; ++i)
    EEPROM.write(addr + i, i < str.length() ? str[i] : 0);
  EEPROM.commit();
}

String readStringFromEEPROM(int addr) {
  char data[33];
  for (int i = 0; i < 32; ++i)
    data[i] = EEPROM.read(addr + i);
  data[32] = '\0';
  return String(data);
}

// === Captive Portal ===
void startCaptivePortal() {
  WiFi.softAP("ESP32_Config_Hafeez", "");
  dns.start(53, "*", WiFi.softAPIP());

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  String relayStatus = digitalRead(RELAY_PIN) == HIGH ? "ON" : "OFF";

  // Read existing values from EEPROM
  String ssid = readStringFromEEPROM(SSID_ADDR);
  String pass = readStringFromEEPROM(PASS_ADDR);
  user = readStringFromEEPROM(USER_ADDR);
  displayText = readStringFromEEPROM(DISPLAYTEXT_ADDR);

  String html = "<!DOCTYPE html><html><head><title>ESP32 Config</title><meta name='viewport' content='width=device-width, initial-scale=1'>";

  // Adding some styles for modern look
  html += "<style>";
  html += "body { font-family: 'Arial', sans-serif; background-color: #f3f4f9; color: #333; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh;}"; 
  html += "h2 { color: #1e3a8a; font-size: 24px; margin-bottom: 20px; text-align: center;}"; 
  html += ".container { background-color: #ffffff; border-radius: 15px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); padding: 40px; width: 100%; max-width: 400px;}"; 
  html += ".form-input { width: 100%; padding: 10px; margin: 8px 0; border-radius: 8px; border: 1px solid #ddd; font-size: 16px; background-color: #f9fafb;}"; 
  html += ".form-input:focus { border-color: #4CAF50; outline: none;}"; 
  html += ".btn { background-color: #4CAF50; color: white; border: none; padding: 12px 24px; width: 100%; border-radius: 8px; font-size: 16px; cursor: pointer;}"; 
  html += ".btn:hover { background-color: #45a049;}"; 
  html += ".status { font-size: 16px; color: #555; margin-top: 20px; text-align: center;}"; 
  html += ".status span { color: #4CAF50; font-weight: bold;}"; 
  html += "@media screen and (max-width: 600px) { .container { padding: 20px; } h2 { font-size: 20px; }}"; 
  html += "</style>";

  html += "</head><body>";

  html += "<div class='container'>";
  html += "<h2>ESP32 Config Portal</h2>";

    // Form with pre-filled values from EEPROM
  html += "<form action='/save'>";
  html += "<input class='form-input' name='ssid' placeholder='WiFi SSID' value='" + ssid + "' required><br>";
  html += "<input class='form-input' name='pass' placeholder='WiFi Password' value='" + pass + "' required><br>";
  html += "<input class='form-input' name='user' placeholder='Username' value='" + user + "' required><br>";
  html += "<input class='form-input' name='displayText' placeholder='Display Text' value='" + displayText + "' required><br>";
  html += "<label for='relay'>Relay:</label>";
  html += "<select class='form-input' name='relay'>";

  // Check relay status from EEPROM or default
  html += "<option value='on'" + String(relayStatus == "ON" ? " selected" : "") + ">ON</option>";
  html += "<option value='off'" + String(relayStatus == "OFF" ? " selected" : "") + ">OFF</option>";
  html += "</select><br>";
  html += "<input class='btn' type='submit' value='Save & Restart'>";
  html += "</form>";

  html += "<div class='status'><strong>Current Status:</strong><br>";
  html += "Temperature:  <span>" + String(temp, 1) + "°C</span><br>";
  html += "Humidity:  <span>" + String(hum, 1) + "%</span><br>";
  html += "Relay: <span>" + relayStatus + "</span></div>";

  html += "</div>";
  html += "</body></html>";

  // Handle requests
  server.on("/", HTTP_GET, [html](AsyncWebServerRequest *request) {
    request->send(200, "text/html", html);
  });

  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request) {
    String ssid = request->getParam("ssid")->value();
    String pass = request->getParam("pass")->value();
    user = request->getParam("user")->value();
    displayText = request->getParam("displayText")->value();

    writeStringToEEPROM(SSID_ADDR, ssid);
    writeStringToEEPROM(PASS_ADDR, pass);
    writeStringToEEPROM(USER_ADDR, user);
    writeStringToEEPROM(DISPLAYTEXT_ADDR, displayText);

    request->send(200, "text/html", "Saved. Rebooting...");
    delay(1000);
    ESP.restart();
  });

  server.begin();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("AP Mode: ESP32_Config");
  display.setCursor(0, 10);
  display.println("Go to: 192.168.4.1");
  display.setCursor(0, 20);
  display.println("Config WiFi & Relay");
  display.display();
}

// === Send Data to Firebase ===
void sendDataSetupToFirebase() {
  // Send displayText and user to Firebase
  if (Firebase.RTDB.setString(&fbdo, "/displayText", displayText)) {
    Serial.println("Display text sent to Firebase.");
  } else {
    Serial.print("Error sending displayText data: ");
    Serial.println(fbdo.errorReason());
  }

  if (Firebase.RTDB.setString(&fbdo, "/user", user)) {
    Serial.println("User data sent to Firebase.");
  } else {
    Serial.print("Error sending user data: ");
    Serial.println(fbdo.errorReason());
  }

  if (Firebase.RTDB.setBool(&fbdo, "/relay/status", relayStatus)) {
    Serial.print("Relay status updated to: ");
    Serial.println(relayStatus ? "HIGH" : "LOW");

    // Update the relay pin accordingly
    digitalWrite( RELAY_PIN, relayStatus ? HIGH : LOW);
  } else {
    Serial.println("Error updating relay status.");
    Serial.println(fbdo.errorReason());  // Print the error reason
  }
}


// === Setup ===
void setup() {
  Serial.begin(115200); // Start serial communication for debugging
  pinMode(CONFIG_BUTTON, INPUT_PULLUP);
  EEPROM.begin(EEPROM_SIZE);
  pinMode(RELAY_PIN,OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();
  delay(1000);

  String ssid = readStringFromEEPROM(SSID_ADDR);
  String pass = readStringFromEEPROM(PASS_ADDR);
  user = readStringFromEEPROM(USER_ADDR);
  displayText = readStringFromEEPROM(DISPLAYTEXT_ADDR);  // Initialize displayText

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), pass.c_str());
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry++ < 20) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed. Starting Captive Portal...");
    startCaptivePortal();
    return;
  }

  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signUp successful");
    signupOK = true;
  } else {
    Serial.printf("SignUp failed: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("\n✅ Firebase Ready");

  sendDataSetupToFirebase();
  Serial.println("\n✅ Sent EEPROM Data to Firebase");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected");
  display.setCursor(0, 10);
  display.println("User: " + user);
  display.setCursor(0, 20);
  display.println("Firebase OK & Data Sent");
  display.display();
  delay(1000);
}

void loop() {

    // Read DHT11 data
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    sendDataDHTToFirebase(temp,hum);
   // Fetch data from Firebase and display it
    getDataFromFirebase();
    // Update OLED display
    updateDisplay(temp,hum);
  // Handle button press to enter configuration mode (if necessary)
    handleConfigButtonPress();
    delay(1000);
}
// === Send Data to Firebase ===
void sendDataDHTToFirebase(float temp, float hum) {

    // After getting data, send temperature and humidity to Firebase
    if (Firebase.RTDB.setFloat(&fbdo, "/dht/temperature", temp)) {
      Serial.println("Temperature data sent to Firebase.");
    } else {
      Serial.print("Error sending temperature data: ");
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "/dht/humidity", hum)) {
      Serial.println("Humidity data sent to Firebase.");
    } else {
      Serial.print("Error sending humidity data: ");
      Serial.println(fbdo.errorReason());
    }

  }
// === Fetch Data from Firebase ===
void getDataFromFirebase() {

  if (Firebase.RTDB.getString(&fbdo, "/displayText")) {
    displayText = fbdo.stringData();
    Serial.print("Display Text: ");
    Serial.println(displayText);
    // Save displayText to EEPROM for persistence
    writeStringToEEPROM(DISPLAYTEXT_ADDR, displayText);
  } else {
    Serial.println("Error fetching displayText from Firebase.");
  }

  if (Firebase.RTDB.getString(&fbdo, "/user")) {
    user = fbdo.stringData();
    Serial.print("User: ");
    Serial.println(user);
    // Save user to EEPROM for persistence
    writeStringToEEPROM(USER_ADDR, user);
  } else {
    Serial.println("Error fetching user from Firebase.");
  }
  if (Firebase.RTDB.getBool(&fbdo, "/relay/status")) {
    relayStatus = fbdo.boolData();  // Get the boolean data from Firebase response
    Serial.print("Relay: ");
    Serial.println(relayStatus ? "HIGH" : "LOW");  // Print the relay status as HIGH or LOW
    
    // Set the relay pin according to the fetched status
    digitalWrite(RELAY_PIN, relayStatus ? HIGH : LOW);
  } else {
    Serial.println("Error fetching data from Firebase.");
    Serial.println(fbdo.errorReason());  // Print the error reason
  }
  
}

// === Update OLED Display ===
void updateDisplay(float temp,float hum) {
  display.clearDisplay();
  display.setCursor(0, 0);

  // Display WiFi status
  display.println(WiFi.status() == WL_CONNECTED ? "WiFi: Connected" : "WiFi: Not connected");

  // Display the first 20 characters of the display text
  display.setCursor(0, 10);
  display.print("Msg: ");
  display.print(displayText.substring(0, 20));

  // Display the first 10 characters of the user
  display.setCursor(0, 20);
  display.print("User: ");
  display.print(user.substring(0, 10));

  display.display();

  delay(1000);

  display.clearDisplay();
  display.setCursor(0, 0);

  // Display WiFi status
  display.println(WiFi.status() == WL_CONNECTED ? "WiFi: Connected" : "WiFi: Not connected");

  // Display Temperature
  display.setCursor(0, 10);
  display.print("Temperature: ");
  display.print(temp);

  // Display Humidity
  display.setCursor(0, 20);
  display.print("Humidity: ");
  display.print(hum);

  display.display();
}

// === Handle Button Press for Config Mode ===
void handleConfigButtonPress() {
  static unsigned long buttonPressTime = 0;
  static bool inConfig = false;

  if (digitalRead(CONFIG_BUTTON) == LOW && !inConfig) {
    if (buttonPressTime == 0) {
      buttonPressTime = millis();
    } else if (millis() - buttonPressTime > 2000) {
      inConfig = true;
      Serial.println("Button pressed, entering config mode...");
      startCaptivePortal();
      while (true) delay(100);  // Block after entering config mode
    }
  } else {
    buttonPressTime = 0;
  }
}
