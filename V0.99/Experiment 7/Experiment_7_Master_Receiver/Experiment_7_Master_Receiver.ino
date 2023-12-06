/*
Title: Weather Station Receiver Experiment 7
Programmer: Justin Lam
Date: 26/02/23
Description: Data logging experiment, master arduino that receives the data and sends in via I2C.
*/

#include <SPI.h>
#include <Wire.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BLINK_COUNT 5
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3D

const int blueLedPin = 3; // RGB blue pin.
const int greenLedPin = 5; // RGB green pin.
const int redLedPin = 6; // RGB red pin.
const int CEPin = 7; // nRF24 chip enable pin.
const int CSNPin = 8; // nRF24 chip select pin.
const byte address[6] = "00001"; // Define the address through which the radios will use to communicate.
bool ackSuccess = false; // Acknowledgement status.

struct dataPackage { // Declaration and definition of a struct that contains the data.
  float temperature = 0.0;
  float humidity = 0.0;
  float pressure = 0.0;
  float altitude = 0.0;
  float rpm = 0.0;
  int index = 0;
  int rain = 1;
};

struct acknowledge { // Declaration and definition of a struct that contains the acknowledge data.
  bool isAck = false;    
};

RF24 radio(CEPin, CSNPin); // Declare and initialise the radio where CE and CSN are digital pins 7 and 8 respectively.
Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Declare and initialise the SSD1306 setting I2C address 0x3D.
dataPackage data; // Declare the data struct named "data".
acknowledge received; // Declare and intitialise acknowledge struct named "received".

void setup() {
  Serial.begin(9600); // Initialise serial communication.
  Wire.begin(); // Begin I2C communciation.

  // Debug LED Initialisation
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  digitalWrite(redLedPin, HIGH);
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(blueLedPin, HIGH);

  #ifndef ESP8266
  while(!Serial) { // Check if the serial port has connected.
    Serial.println("Error: Failed to start serial communication!");
    blinkLEDDebug(255, 0 , 0, 500, BLINK_COUNT);
  }
  #endif

  // SSD1306 Initialisation
  if (!OLED.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) { // Check if the OLED display is connected.
    Serial.print("Error: SSD1306 not responding!");
    blinkLEDDebug(255, 0, 0, 500, BLINK_COUNT);
    LEDDebug(255, 0, 0, true);
    while(1);
  }
  else { // After initialisation clear the display buffer.
    OLED.clearDisplay();
    displayText(1, WHITE, 15, 28, "Initialising...");
    delay(1500);
  }

  // nRF24 Initialisation
  if (!radio.begin()) { // Check if the module is connected to the SPI bus.
    Serial.println("Error: nRF24 radio not responding!");
    Serial.println("Please check wiring...");
    displayText(1, WHITE, 0, 28, "nRF24 not responding!");
    blinkLEDDebug(255, 0, 0, 500, BLINK_COUNT);
    LEDDebug(255, 0, 0, true);
    while(1);
  }
  else {
    radio.openReadingPipe(1, address); // Set the address for transmission.
    radio.setPALevel(RF24_PA_LOW); // Set the amplification of the radio to low.
    radio.enableAckPayload(); // Enable the acknowledgement payload.
    radio.writeAckPayload(1, &received, sizeof(acknowledge)); // Send an acknowledgement packet.
    radio.startListening(); // Start receiving.
    Serial.println("nRF24 Initialised!");
    displayText(1, WHITE, 0, 28, "nRF24 Initialised!");
    blinkLEDDebug(0, 255, 0, 200, BLINK_COUNT);
    delay(250);
  }
  
  displayText(1, WHITE, 0, 26, "All intitalised!");
  delay(500);
}

void loop() {
  if (radio.available()) { // Wait for data transmission.
    radio.read(&data, sizeof(dataPackage)); // Read data received.
    LEDDebug(0, 0, 255, true);
    received.isAck = true; // The receiver has received a data package.
    ackSuccess = radio.writeAckPayload(1, &received, sizeof(acknowledge)); // Return an acknowledgement package.

    if (ackSuccess) { // Check if acknowledgement payload was received.
    // Print the data on the serial monitor.
      blinkLEDDebug(255, 255, 0, 50, 10);
      Serial.print("Temperature: ");
      Serial.print(data.temperature);
      Serial.print("C | Humidity: ");
      Serial.print(data.humidity);
      Serial.print("% | Pressure: ");
      Serial.print(data.pressure);
      Serial.print("Pa | Altitude: ");
      Serial.print(data.altitude);
      Serial.println("m");
      Serial.print("VOC Index: ");
      Serial.print(data.index);
      Serial.print(" | Wind Speed: ");
      Serial.print(data.rpm);
      Serial.print(" RPM | ");
      // Display the data on the OLED display.
      OLED.clearDisplay();
      OLED.setTextSize(1);
      OLED.setTextColor(WHITE);
      OLED.setCursor(0,0);
      OLED.print("Temperature: ");
      OLED.print(data.temperature);
      OLED.println(" C");
      OLED.print("Humidity: ");
      OLED.print(data.humidity);
      OLED.println('%');
      OLED.print("Pressure: ");
      OLED.print(data.pressure);
      OLED.println("Pa");
      OLED.print("Altitude: ");
      OLED.print(data.altitude);
      OLED.println('m');
      OLED.print("Wind Speed: ");
      OLED.print(data.rpm);
      OLED.println("RPM");
      OLED.print("VOC Index: ");
      OLED.println(data.index);

      if (data.rain == 0) { // Print yes if rain sensor returns a logic low.
        Serial.println("Rain: Yes");
        OLED.println("Rain: Yes");
      }
      else { // Print no if rain sensor returns a logic high.
        Serial.println("Rain: No");
        OLED.println("Rain: No");
      }

      OLED.display();
      // Transmit the data to the slave Arduino via I2C
      Wire.beginTransmission(6);
      Wire.write((byte *)&data, sizeof(dataPackage));
      Wire.endTransmission();
      delay(250);
    }
    else { // Acknowledgement package failed.
      Serial.println("Error: Payload acknowledgement failed!");
      blinkLEDDebug(255, 0, 0, 100, BLINK_COUNT);
    }

    received.isAck = false; // Reset the acknowledgement status.    
  }
  else { // Blink blue while on standby.
    blinkLEDDebug(0, 0, 255, 250, 1);
  }
}

void LEDDebug(int red, int green, int blue, bool status) { // Turn on or off RGB LED and set its colour.
  int redVal = 255 - red;
  int greenVal = 255 - green;
  int blueVal = 255 - blue;

  if (status) {
    analogWrite(redLedPin, redVal);
    analogWrite(greenLedPin, greenVal);
    analogWrite(blueLedPin, blueVal);
  }
  else {
    digitalWrite(redLedPin, HIGH);
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(blueLedPin, HIGH);
  }
}

void blinkLEDDebug(int red, int green, int blue, int time, int count) { // Set RGB LED colour and blink the LED.
  int redVal = 255 - red;
  int greenVal = 255 - green;
  int blueVal = 255 - blue;

  for (int i = 0; i < count; i++) {
    analogWrite(redLedPin, redVal);
    analogWrite(greenLedPin, greenVal);
    analogWrite(blueLedPin, blueVal);
    delay(time);
    digitalWrite(redLedPin, HIGH);
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(blueLedPin, HIGH);
    delay(time);
  }
}

void displayText(uint8_t size, uint16_t colour, uint16_t x, uint16_t y, String text) { // Display a string on the OLED.
  OLED.clearDisplay();
  OLED.setTextColor(colour);
  OLED.setTextSize(size);
  OLED.setCursor(x, y);
  OLED.println(text);
  OLED.display();
}