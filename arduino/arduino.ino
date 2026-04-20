#include <Wire.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
static const uint32_t GPSBaud = 9600;
TinyGPS gps;
float flat;
float flon;
String msg;
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

// GSM on pins 2 & 3
SoftwareSerial gsm(2, 3);  // RX, TX

#define vibrationPin 4
#define firePin 5
#define buzzer 6

#define ADXL345 0x53

int16_t AcX, AcY, AcZ;



// flags to avoid SMS spam 😎
bool fireSent = false;
bool cutSent = false;
bool fallSent = false;

void setup() {
  Serial.begin(9600);
  gsm.begin(9600);
  Wire.begin();

  lcd.begin(16, 2);
  lcd.print("Forest Monitor");
  delay(2000);
  lcd.clear();

  pinMode(vibrationPin, INPUT);
  pinMode(firePin, INPUT);
  pinMode(buzzer, OUTPUT);

  // ADXL345 init
  Wire.beginTransmission(ADXL345);
  Wire.write(0x2D);
  Wire.write(8);
  Wire.endTransmission();

  delay(3000);
}



void loop() {

  // ---- READ ADXL345 ----
  Wire.beginTransmission(ADXL345);
  Wire.write(0x32);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345, 6, true);

  AcX = Wire.read() | Wire.read() << 8;
  AcY = Wire.read() | Wire.read() << 8;
  AcZ = Wire.read() | Wire.read() << 8;

  int vibration = digitalRead(vibrationPin);
  int fire = digitalRead(firePin);

  int fireStatus = 0;
  int vibStatus = 0;
  int fallStatus = 0;
  int buzStatus = 0;

  lcd.clear();

  // ---- CONDITIONS ----
  if (fire == LOW) {
    lcd.print("FIRE ALERT!");
    fireStatus = 1;
    buzStatus = 1;

    if (!fireSent) {
      msg = "fire detected";
      SendMessage();
      delay(2000);
      fireSent = true;
    }

    cutSent = false;
    fallSent = false;
  } else if (vibration == HIGH) {
    lcd.print("TREE CUTTING!");
    vibStatus = 1;
    buzStatus = 1;

    if (!cutSent) {
      msg = "tree cutting alert";
      SendMessage();
      delay(2000);
      cutSent = true;
    }

    fireSent = false;
    fallSent = false;
  } else if (abs(AcX) > 150 || abs(AcY) > 150) {
    lcd.print("TREE FALL!");
    fallStatus = 1;
    buzStatus = 1;

    if (!fallSent) {
      msg = "tree fall detected";
      SendMessage();
      delay(2000);
      fallSent = true;
    }

    fireSent = false;
    cutSent = false;
  } else {
    lcd.print("All Safe :)");

    // reset all flags
    fireSent = false;
    cutSent = false;
    fallSent = false;
  }

  digitalWrite(buzzer, buzStatus);

  // ---- SERIAL OUTPUT (for NodeMCU) ----
  Serial.print("a");
  Serial.print(fireStatus);
  Serial.print("b");
  Serial.print(vibStatus);
  Serial.print("c");
  Serial.print(fallStatus);
  Serial.println("d");

  delay(1000);
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;) {
    while (Serial.available()) {
      char c = Serial.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c))  // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData) {
    //    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    Serial.println("LAT=");
    Serial.println(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    //    Serial.println(flat);
    Serial.println(" LON=");
    Serial.println(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    //    Serial.println(flon);
    delay(2000);
  }
}
void SendMessage() {
  gsm.println("AT");  //test check communication with gsm module
  delay(1000);
  gsm.println("ATE0");  //ATE0: Switch echo off. ATE1: Switch echo on.
  delay(1000);
  gsm.println("AT+CMGF=1");  //set sms text mode
  delay(1000);
 gsm.println("AT+CMGS=\"+918008282486\"\r");  //send specfic number
  delay(1000);


  gsm.println(msg);
  gsm.println("LOCATION");
  // Convert flat and flon to 6-digit precision strings
  String flatStr = String(flat, 6);
  String flonStr = String(flon, 6);
  gsm.println("This is location click to direction");
  gsm.println("http://www.google.com/maps/place/" + flatStr + "," + flonStr);  //lin
  delay(1000);
  gsm.write(26);
  delay(5000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Message sent..");
  gsm.println("Message sent..");
  delay(1000);

  // Clear the LCD and display latitude and longitude
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LAT : ");
  lcd.print(flatStr);
  lcd.setCursor(0, 1);
  lcd.print("LNG : ");
  lcd.print(flonStr);
}
