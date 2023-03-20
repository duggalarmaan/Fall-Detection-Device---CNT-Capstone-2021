#include "SoftwareSerial.h"
// MPU-6050 Short Example Sketch
// Public Domain
#include<Wire.h>
//#include <ESP8266WiFi.h>
#include <LiquidCrystal.h> 
int Contrast=150;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  
const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
float ax=0, ay=0, az=0, gx=0, gy=0, gz=0;

//int data[STORE_SIZE][5]; //array for saving past data
//byte currentIndex=0; //stores current data array index (0-255)
boolean fall = false; //stores if a fall has occurred
boolean trigger1=false; //stores if first trigger (lower threshold) has occurred
boolean trigger2=false; //stores if second trigger (upper threshold) has occurred
boolean trigger3=false; //stores if third trigger (orientation change) has occurred

byte trigger1count=0; //stores the counts past since trigger 1 was set true
byte trigger2count=0; //stores the counts past since trigger 2 was set true
byte trigger3count=0; //stores the counts past since trigger 3 was set true
int angleChange=0;

String ssid ="NAIT";

String password="**********";

SoftwareSerial esp(8, 9);// RX, TX

String data;

String server = "www.thewebdeveloper.ca"; // www.example.com

String uri = "/webservice.php";// our example is /esppost.php

String wall, message;

void setup() {

esp.begin(9600);

Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

Serial.begin(9600);

 pinMode(11, OUTPUT);
  digitalWrite(11, HIGH);

  analogWrite(6,Contrast);
     lcd.begin(16, 2);
     lcd.clear();

reset();

connectWifi();

}

//reset the esp8266 module

void reset() {

esp.println("AT+RST");

delay(1000);

if(esp.find("OK") ) Serial.println("Module Reset");

}

//connect to your wifi network

void connectWifi() {

String cmd = "AT+CWJAP=\"" +ssid+"\",\"" + password + "\"";

esp.println(cmd);

delay(4000);

if(esp.find("OK")) {

Serial.println("Connected!");

}

else {

connectWifi();

Serial.println("Cannot connect to wifi"); }

}


void loop(){
  
  mpu_read();
  //2050, 77, 1947 are values for calibration of accelerometer
  // values may be different for you
  ax = (AcX-673)/16384.00;
  ay = (AcY-5563)/16384.00;
  az = (AcZ+1928)/16384.00;
  
  //270, 351, 136 for gyroscope
  gx = (GyX-199)/131.07;
  gy = (GyY+75)/131.07;
  gz = (GyZ+35)/131.07;
  
  // calculating Amplitute vactor for 3 axis
  float Raw_AM = pow(pow(ax,2)+pow(ay,2)+pow(az,2),0.5);
  int AM = Raw_AM * 10;  // as values are within 0 to 1, I multiplied 
                         // it by for using if else conditions 
  
  Serial.println(AM);
//  Serial.println(ax);
//  Serial.println(ay);
//  Serial.println(az);
//  Serial.println(gx);
//  Serial.println(gy);
//  Serial.println(gz);
//  Serial.println(PM);
  delay(500);

  if (trigger3==true){
     trigger3count++;
     //Serial.println(trigger3count);
     if (trigger3count>=10){ 
        angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5);
        //delay(10);
        Serial.println(angleChange); 
        lcd.setCursor(0, 0);
        //lcd.print("Fall Detected");
        if ((angleChange>=0) && (angleChange<=10)){ //if orientation changes remains between 0-10 degrees
            fall=true; trigger3=false; trigger3count=0;
            Serial.println(angleChange);
              }
        else{ //user regained normal orientation
           trigger3=false; trigger3count=0;
           Serial.println("TRIGGER 3 DEACTIVATED");
        }
      }
   }
  if (fall==true){ //in event of a fall detection
    Serial.println("FALL DETECTED");
    lcd.print("Fall Detected");
    digitalWrite(11, LOW);
    delay(20);
    digitalWrite(11, HIGH);
    delay(100);
    // convert the bit data to string form
    
    wall = "detected";
    message = "Fall Happened";
    
    
    data = "action=" + wall + "&message=" + message;// data sent must be under this form //name1=value1&name2=value2.
    
    httppost();
    
    delay(1000);
        
        fall=false;
       // exit(1);
        }
      if (trigger2count>=6){ //allow 0.5s for orientation change
        trigger2=false; trigger2count=0;
        Serial.println("TRIGGER 2 DECACTIVATED");
        }
      if (trigger1count>=6){ //allow 0.5s for AM to break upper threshold
        trigger1=false; trigger1count=0;
        Serial.println("TRIGGER 1 DECACTIVATED");
        }
      if (trigger2==true){
        trigger2count++;
        //angleChange=acos(((double)x*(double)bx+(double)y*(double)by+(double)z*(double)bz)/(double)AM/(double)BM);
        angleChange = pow(pow(gx,2)+pow(gy,2)+pow(gz,2),0.5); Serial.println(angleChange);
        if (angleChange>=30 && angleChange<=400){ //if orientation changes by between 80-100 degrees
          trigger3=true; trigger2=false; trigger2count=0;
          Serial.println(angleChange);
          Serial.println("TRIGGER 3 ACTIVATED");
            }
        }
      if (trigger1==true){
        trigger1count++;
        if (AM>=12){ //if AM breaks upper threshold (3g)
          trigger2=true;
          Serial.println("TRIGGER 2 ACTIVATED");
          trigger1=false; trigger1count=0;
          }
        }
      if (AM<=4 && trigger2==false){ //if AM breaks lower threshold (0.4g)
        trigger1=true;
        Serial.println("TRIGGER 1 ACTIVATED");
        }
    //It appears that delay is needed in order not to clog the port
      delay(100);
}

void mpu_read(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
}

void httppost () {

esp.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");//start a TCP connection.

if( esp.find("OK")) {

Serial.println("TCP connection ready");

} delay(1000);

String postRequest =

"POST " + uri + " HTTP/1.0\r\n" +

"Host: " + server + "\r\n" +

"Accept: *" + "/" + "*\r\n" +

"Content-Length: " + data.length() + "\r\n" +

"Content-Type: application/x-www-form-urlencoded\r\n" +

"\r\n" + data;

String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.

esp.print(sendCmd);

esp.println(postRequest.length() );

delay(500);

if(esp.find(">")) { Serial.println("Sending.."); esp.print(postRequest);

if( esp.find("SEND OK")) { Serial.println("Packet sent");

while (esp.available()) {

String tmpResp = esp.readString();

Serial.println(tmpResp);

}

// close the connection

esp.println("AT+CIPCLOSE");

}

}}
