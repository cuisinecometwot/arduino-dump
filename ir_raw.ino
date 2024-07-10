/* IR Receiver (NEC Protocol)
 * The program receives 32-bit IR signal (0 or 1)
 * and print them on OLED Display.
 * No external library was used.
 * Requirements: ESP32, OLED Display SH1106, IR Receiver Module
 * Date: 07/07/2024
 */
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Configure
#define i2c_Address   0x3c
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define IR_PIN        25        // IR output pin
#define MAX_PULSES    32        // 32-bit signal

unsigned int pulseDuration;
bool receivedSignal = false;
unsigned long bitStartTime = 0;
unsigned int bitIndex = 0;
uint8_t receivedData[MAX_PULSES];   // Array to store received bits (0 or 1)

// Configure OLED
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,OLED_RESET);

void setup() 
{
  Serial.begin(9600);               // Initialize serial communication for printing
  pinMode(IR_PIN, INPUT);           // Set IR pin as input
  attachInterrupt(digitalPinToInterrupt(IR_PIN), IRpulse, FALLING); // Trigger interrupt function

  display.begin(i2c_Address, true); // Address 0x3C by default  
  display.display();  
  delay(2000);
  // Clear the buffer.  
  display.clearDisplay();   
  // text display  
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);  
  display.println("IR SIGNAL");  
  display.display();
}

void loop() 
{
  if (receivedSignal) 
  {
    display.clearDisplay();
    display.setCursor(0, 0);  
    for (int i = 0; i < MAX_PULSES; i++) 
    {
      display.print(receivedData[i]);
      if ((i+1)%8==0) display.println();
    }
    display.display();
    // Reset for next signal
    receivedSignal = false;
    bitIndex = 0;
  }
}

void IRpulse() // trigger when there is a FALLING EDGE
{
  unsigned long currentTime = micros();
  if (!bitStartTime) // First pulse, initialize start time
  { 
    bitStartTime = currentTime;
  }
  else
  {
    pulseDuration = currentTime - bitStartTime;
    bitStartTime = currentTime;
    if (bitIndex < MAX_PULSES)
    {
      if (pulseDuration > 4000);        // first pulse, ignore it 
      else if (pulseDuration > 2000)    // enough to consider as '1'
      {
        receivedData[bitIndex++] = 1;
      }
      else if (pulseDuration > 1000)    // consider as '0'
      {
        receivedData[bitIndex++] = 0;
      }
    }
    if (bitIndex == MAX_PULSES)         // done
    {
      receivedSignal = true;
    }
  }
}
