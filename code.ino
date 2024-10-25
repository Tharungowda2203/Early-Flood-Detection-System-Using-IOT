// Include the libraries
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>  // Include ThingSpeak library

// Define the pins
#define DHTPIN 2       // DHT11 sensor pin
#define DHTTYPE DHT11  // DHT11 sensor type
#define TRIG_PIN 3     // Ultrasonic sensor trigger pin
#define ECHO_PIN 4     // Ultrasonic sensor echo pin
#define FLOW_PIN 5     // Water flow sensor pin
#define RX_PIN 6       // ESP8266 RX pin
#define TX_PIN 7       // ESP8266 TX pin

// Initialize the objects
DHT dht(DHTPIN, DHTTYPE);            // DHT11 sensor object
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD display object
SoftwareSerial esp(RX_PIN, TX_PIN);  // ESP8266 module object
WiFiClient client;                   // WiFi client for ThingSpeak

// Define constants
const float MAX_LEVEL = 100.0;       // Maximum water level in cm
const float MAX_FLOW = 10.0;         // Maximum water flow rate in L/min
const char* ssid = "Your Wifi id";   // WiFi network SSID
const char* pass = "Wifi password";  // WiFi network password
unsigned long myChannelNumber = 2556539;
const char* myWriteAPIKey = "12BQB4WSU368ZUPM";

// Define some variables
float temp;      // Temperature in C
float hum;       // Humidity in %
float level;     // Water level in cm
float flow;      // Water flow rate in L/min
float duration;  // Duration of ultrasonic pulse in microseconds
float distance;  // Distance of ultrasonic pulse in cm
int count;
bool alert;

// Interrupt function for water flow sensor
void flowPulse() {
  count++;
}

/ void setup() {
  // Initialize the serial monitor
  Serial.begin(9600);
  // Initialize the DHT11 sensor
  dht.begin();
  // Initialize the LCD display
  lcd.init();
  lcd.backlight();
  // Initialize the ESP8266 module
  esp.begin(9600);
  // Set the pins as inputs or outputs
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FLOW_PIN, INPUT);
  // Attach an interrupt to the water flow sensor pin
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowPulse, RISING);
  // Clear the LCD display
  lcd.clear();
  // Print a welcome message
  lcd.print("Flood Detection");
  lcd.setCursor(0, 1);
  lcd.print("System");
  // Wait for 2 seconds
  delay(2000);
  // Clear the LCD display
  lcd.clear();

  // Connect to WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // DHT11 sensor readings
  temp = dht.readTemperature();
  hum = dht.readHumidity();
  if (isnan(temp) || isnan(hum)) {
    // Print an error message
    lcd.print("DHT11 Error");
  } else {
    // Print the temperature and humidity on the LCD display
    lcd.print("Temp: ");
    lcd.print(temp);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(hum);
    lcd.print(" %");

    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    // Add other sensor data to ThingSpeak as needed
    // ThingSpeak.setField(3, level);
    // ThingSpeak.setField(4, flow);
    // Write the fields that you've set to ThingSpeak
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Data sent to ThingSpeak");
    } else {
      Serial.println("Error sending data to ThingSpeak");
    }
  }
  // Wait for 2 seconds
  delay(2000);
  // Clear the LCD display
  lcd.clear();

  // Ultrasonic sensor readings
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  // Send a high pulse to the ultrasonic sensor trigger pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  // Send a low pulse to the ultrasonic sensor trigger pin
  digitalWrite(TRIG_PIN, LOW);
  // Read the duration of the ultrasonic pulse from the echo pin
  duration = pulseIn(ECHO_PIN, HIGH);
  // Calculate the distance of the ultrasonic pulse
  distance = (duration / 2) * 0.0343;
  // Calculate the water level
  level = MAX_LEVEL - distance;
  // Check if the water level is valid
  if (level < 0 || level > MAX_LEVEL) {
    // Print an error message
    lcd.print("Ultrasonic Error");
  } else {
    // Print the water level on the LCD display
    lcd.print("Level: ");
    lcd.print(level);
    lcd.print(" cm");
  }

  delay(2000);
  lcd.clear();

  // Calculate water flow rate
  count = 0;
  delay(1000);
  flow = (count / 7.5) * 60.0;
  if (flow < 0 || flow > MAX_FLOW) {
    lcd.print("Flow Error");
  } else {
    // Print the water flow rate on the LCD display
    lcd.print("Flow: ");
    lcd.print(flow);
    lcd.print(" L/min");
  }
  // Wait for 2 seconds
  delay(2000);
  // Clear the LCD display
  lcd.clear();

  // Flood alert check
  if (level > 80 || flow > 8) {
    alert = true;
    lcd.print("FLOOD ALERT!");
    sendSMS();
  } else {
    alert = false;
    lcd.print("No Flood");
  }

  delay(2000);
  lcd.clear();
}

// Function to send an SMS alert using the ESP8266 modulevoid sendSMS() {
  if (esp.available()) {
    // Check if the ESP8266 module is read
    esp.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80");
    delay(2000);
    if (esp.find("CONNECT")) {
      esp.println("AT+CIPSEND=51\r\nGET /apps/thinghttp/send_request?api_key=Your API key");
      // Check if the ESP8266 module is read
      delay(2000);
      // Check if the ESP8266 module has sent the SMS
      if (esp.find("SEND OK")) {
        // Print a success message on the serial monitor
        Serial.println("SMS sent successfully");
      } else {
        // Print a failure message on the serial monitor
        Serial.println("SMS sending failed");
      }
    } else {
      // Print a failure message on the serial monitor
      Serial.println("ESP8266 not connected");
    }
  } else {
    // Print a failure message on the serial monitor
    Serial.println("ESP8266 not ready");
  }
}
