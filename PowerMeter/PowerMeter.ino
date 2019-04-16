/*  
  RC Power Meter
  wyfinger@yahoo.com, 2019

*/

#include <stdio.h>
#include <SPI.h>
#include <Servo.h>
#include <OneButton.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(6, 5, 4, 3, 2);
OneButton button = OneButton(8, HIGH);
Servo servo;

float valCurrent    = 0;    float valMaxCurrent = 0;   // Ampere
float valAbsCurrent = 0;
float valVoltage    = 0;    float valMinVoltage = 100; // Volts
float valPower      = 0;    float valMaxPower   = 0;   // Watts
float valAh         = 0;                               // Ampere * Hour
float valWh         = 0;                               // Watt * Hour
long  valTime       = 0;                               // mills

byte  screenNo = 0;
byte  itemNo = 0;
byte  servoMode = 0;
byte  valServo = 0;
byte  servoDir = 0; 

long  stmScreenUpdate = 0;
long  stmButton = 0;
long  stmMeasure = 0;
long  stmSendData = 0;
long  stmServo = 0;

long avgCurrent[5];
long avgVoltage[5];

int val, test;

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(8, INPUT);
  digitalWrite(8, HIGH);
  pinMode(10, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  //
  servo.attach(10);  
  //button.setDebounceTicks(20);
  //button.setClickTicks(50);
  //button.setPressTicks(500);
  button.attachClick(shortClick);
  button.attachDoubleClick(doubleClick);
  button.attachPress(longClick);
  //
  for (int i=0; i<sizeof(avgCurrent)/sizeof(avgCurrent[0]); i++) {
    avgCurrent[i] = 0;
    avgVoltage[i] = 0;
  }  
  //
  display.begin();
  display.setContrast(50); 
  drawSplash();  
  delay(1500);
  //
  Serial.begin(9600);
  Serial.println("PowerMeter");
  Serial.println("Use SerialPlot to log values (https://bitbucket.org/hyOzd/serialplot)");
  Serial.println("----");
}

// the loop function runs over and over again forever
void loop() {    
  checkButton();
  updateScreen(false);
  sendData();
  checkMeasurement();
  checkServo();
}

void checkMeasurement() {
  // check measuremet current and voltage
  if (millis() - stmMeasure > 50) {
    int C = analogRead(A0);
    int V = analogRead(A1);
    for (int i=sizeof(avgCurrent)/sizeof(avgCurrent[0])-1; i>=1; i--) {  // push data right
      avgCurrent[i] = avgCurrent[i-1];
      avgVoltage[i] = avgVoltage[i-1];
    }
    avgCurrent[0] = C;
    avgVoltage[0] = V;
    C = 0;
    V = 0;
    for (int i=0; i<sizeof(avgCurrent)/sizeof(avgCurrent[0]); i++) {  // calc sum of avg window      
      C = C+avgCurrent[i];
      V = V+avgVoltage[i];      
    }
    // calculate main electrical values? calibration si here
    valCurrent = (580-C)/36.0;
    valCurrent = (-0.1 < valCurrent) && (valCurrent < 0.1) ? 0 : valCurrent;
    valAbsCurrent = abs(valCurrent);
    valVoltage = V/135.6832;
    valPower = valAbsCurrent * valVoltage;
    // min / max
    if (valAbsCurrent > valMaxCurrent)  valMaxCurrent = valAbsCurrent;
    if ((valVoltage < valMinVoltage) && (valVoltage > 1))  valMinVoltage = valVoltage;
    if (valPower > valMaxPower)  valMaxPower = valPower;    
    if (avgCurrent[4] == 0) { // avg window are clear
      valMaxCurrent = 0;
      valMinVoltage = 100;
      valMaxPower = 0;
    }
    // accumulating
    if (valAbsCurrent >= 0.1) {
      int mill = (millis() - stmMeasure); // time chank
      valTime = valTime + mill;
      valAh = valAh + (mill/1000.0 * valAbsCurrent) / 1000.0;
      valWh = valWh + (mill/1000.0 * valAbsCurrent * valVoltage) / 1000.0;
    }
    stmMeasure = millis();
  }
}

void clearSummVals() {
  valMaxCurrent = 0;
  valMinVoltage = 100;
  valMaxPower = 0;
  valTime = 0;
  valAh = 0;
  valWh = 0;
}

void checkButton() {
  // check button, short or long press 
  int state; 
  if (millis() - stmButton > 10) {
    button.tick(); 
    stmButton = millis();
    }
}

void checkServo() {
  // check servo
  byte newServo;
  if (millis() - stmServo > (servoMode == 1 ? 1000 : 50)) { // 1 sec for discrete sweep
    switch (servoMode) {
      case 0:  // knob
        newServo = map(analogRead(A2), 0, 1023, 0, 180);  // potentiomener pin
        break;
      case 1: // sweep
        newServo = valServo == 180 ? 0 : 180; // discrete
        /*if (servoDir == 0) {
          newServo = valServo + 5;
          if (newServo == 180) servoDir = 1;
        } else {
          newServo = valServo - 5;
          if (newServo == 0) servoDir = 0;
        } */
        break;
      case 2: // center
        newServo = 90;
        break;
    }    
    if (newServo != valServo) {
      valServo = newServo;
      servo.write(valServo);
      updateScreen(true);
    }
    stmServo = millis();
    }
}

void sendData() {
  // send data to USB/Serial
  if (millis() - stmSendData > 250) {
    digitalWrite(LED_BUILTIN, HIGH);
    // [Current],[Voltage],[Voltage],[Power],[Ah],[Wh],[Servo]
    Serial.println(String(valCurrent)+","+String(valVoltage)+","+String(valPower)+","+String(valAh)+","+String(valWh)+","+String(valServo));
    digitalWrite(LED_BUILTIN, LOW);  
    stmSendData = millis();  
  }
}

void drawSplash() {
  display.clearDisplay();  
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(6,8);    display.print("            ");
  display.setCursor(6,16);   display.print(" PowerMeter ");
  display.setCursor(6,24);   display.print("            ");
  display.setTextColor(BLACK);
  display.setCursor(9,36);  
  display.print("by Wyfinger");
  display.display();
}

String calcTimeStr(long time) {
  byte m,s;
  m = time / 60000;
  s = (time - m*60000) / 1000;
  return String(m) + ':' + (s>9 ? String(s) : "0"+String(s));
}

void drawScreen0() {
  // Current, Voltage, Power and Time
  char ch[8];

  display.clearDisplay();  
  display.setTextSize(1);    
  display.setCursor(0,2);  dtostrf(valCurrent, 2, 1, ch); display.print("I=" + String(ch));
  display.setCursor(0,14); dtostrf(valVoltage, 2, 1, ch); display.print("U=" + String(ch));
  display.setCursor(0,26);  display.print("P=" + String(round(valPower)));
  display.setCursor(0,38);  display.print("T=" + calcTimeStr(valTime));
  display.setTextSize(2);
  display.setCursor(45,17); // show params with large text size
  switch (itemNo) {
    case 0: // display items are not selected
      break;
    case 1:  
      display.print(String(abs(round(valCurrent)))); // show absolute Current 
      display.drawLine(0, 9, 38, 9, BLACK);
      break;
    case 2:  
      display.print(String(round(valVoltage)));
      display.drawLine(0, 21, 38, 21, BLACK);
      break;    
    case 3:
      display.print(String(round(valPower)));
      display.drawLine(0, 33, 38, 33, BLACK);
      break;
    default:
      break;
  }  
  display.display();
}

void drawScreen1() {
  // Max, Max Current, Voltage, Power
  char ch[8];
  // if Min Voltage = 100 (initial value) then display 0
  float mv = (valMinVoltage == 100) ? 0 : valMinVoltage;
  display.clearDisplay();  
  display.setTextSize(1);    
  display.setCursor(0,2);  dtostrf(valMaxCurrent, 2, 1, ch); display.print("Imax=" + String(ch));
  display.setCursor(0,14); dtostrf(mv, 2, 1, ch); display.print("Umin=" + String(ch));
  display.setCursor(0,26);  display.print("Pmax=" + String(round(valMaxPower)));
  display.display();
}

void drawScreen2() {
  // Energy
  char ch[8];
  display.clearDisplay();  
  display.setTextSize(1);    
  display.setCursor(0,2);  dtostrf(valWh, 2, 2, ch); display.print("W=" + String(ch) + "Wh");
  display.setCursor(0,14); dtostrf(valAh, 2, 2, ch); display.print("W=" + String(ch) + "Ah");
  display.setCursor(0,26); display.print("double click\nfor clear");
  display.display();
}

// x,y == coords of centre of arc
// start_angle = 0 - 359
// seg_count = number of 3 degree segments to draw (120 => 360 degree arc)
// rx = x axis radius
// yx = y axis radius
// w  = width (thickness) of arc in pixels
// colour = 16 bit colour value
// Note if rx and ry are the same an arc of a circle is drawn
int fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour) {
// https://forum.arduino.cc/index.php?topic=413186.0
  #define DEG2RAD 0.0174532925

  byte seg = 3; // Segments are 3 degrees wide = 120 segments for 360 degrees
  byte inc = 3; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * DEG2RAD);
    float sy = sin((i - 90) * DEG2RAD);
    uint16_t x0 = sx * (rx - w) + x;
    uint16_t y0 = sy * (ry - w) + y;
    uint16_t x1 = sx * rx + x;
    uint16_t y1 = sy * ry + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    display.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    display.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
  }
}

void drawScreen3() {
  // Servo tester
  display.clearDisplay(); 
  display.setCursor(4,0);
  display.print("Mode: ");
  switch (servoMode) {  // servo screen mode
      case 0:  
        display.print("knob"); // potentiometer
        break;
      case 1:  
        display.print("sweep");
        break;  
      case 2:  
        display.print("center");
        break;    
  }      
  fillArc(42, 48, -90, round(valServo/3.0), 40, 36, 12, BLACK);
  display.setCursor(33,36);  // 84x48
  display.print(valServo);
  display.display();
}

void updateScreen(bool immediately) {
  // update screen graphics every 500 ms
  if (immediately || (millis() - stmScreenUpdate > 500)) {
    switch (screenNo) {
      case 0: 
        drawScreen0(); 
        break;
      case 1: 
        drawScreen1(); 
        break;  
      case 2: 
        drawScreen2(); 
        break;         
      case 3: 
        drawScreen3();
        break;    
    }   
  stmScreenUpdate = millis();
  }  
}

void shortClick() {
  switch (screenNo) {
    case 0 :   // main screen - select value to big font display: current, voltage, power
      itemNo = (itemNo == 3) ? 0 : itemNo + 1;
      break; 
    case 3 :   // servo screen - knob / sweep / center
      servoMode = (servoMode == 2) ? 0 : servoMode + 1;
      break; 
    default:
      break;
  }
  updateScreen(true);
}

void longClick() {
  // change screen
  screenNo = (screenNo == 3) ? 0 : screenNo + 1;
}

void doubleClick() {
  switch (screenNo) {
    case 0 :
      break;
    case 1 :
      break;
    case 2 :
      clearSummVals();
      break;
  }
  updateScreen(true);
}