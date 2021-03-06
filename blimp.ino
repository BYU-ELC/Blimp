
//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "string.h"
#include "math.h"


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;


//Each motor has three GPIO pins, input1 and input2 control the direction the motor is spinning and enable is a PWM pin that controls horizontalSpeed

//Pin numbers for forward facing right motor
const int forward_right_enablePin = 16;
const int foward_right_input1 = 33;
const int foward_right_input2 = 15;
const int forward_right_pwmChannel = 0;

//Pin numbers for forward facing left motor
const int forward_left_enablePin = 13;
const int foward_left_input1 = 17;
const int foward_left_input2 = 4;
const int forward_left_pwmChannel = 1;


//Pin numbers for vertical forward motor
const int vertical_forward_enablePin = 14;
const int vertical_forward_input1 = 18;
const int vertical_forward_input2 = 19;
const int vertical_forward_pwmChannel = 2;


//pin numberfs for vertical rear motor
const int vertical_rear_enablePin = 27;
const int vertical_rear_input1 = 12;
const int vertical_rear_input2 = 32;
const int vertical_rear_pwmChannel = 3;

//pin numbers for voltage detection
const int battery_voltage = 36;
int voltage = 0;
float voltageMovingAverage = 4.2;

//pin numbers for accelerometer data



// Setting PWM properties
const int freq = 30000;
const int resolution = 8;




int turn = 0;
int horizontalSpeed = 0;
int verticalSpeed = 0;


//Bluetooth app setup
int update_interval=100; // time interval in ms for updating panel indicators 
unsigned long last_time=0; // time of last update
bool connectedFlag = false;
int timeRemaining = 10;
int fuelRemaining = 100;

//Keyed Entry
char dataIn;
String passkey = "";
String code = "1234";


//Accelerometer Setup

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
//MPU6050 accelgyro;
MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;



// uncomment "OUTPUT_READABLE_ACCELGYRO" if you want to see a tab-separated
// list of the accel X/Y/Z and then gyro X/Y/Z values in decimal. Easy to read,
// not so easy to parse, and slow(er) over UART.
#define OUTPUT_READABLE_ACCELGYRO

// uncomment "OUTPUT_BINARY_ACCELGYRO" to send all 6 axes of data as 16-bit
// binary, one right after the other. This is very fast (as fast as possible
// without compression or data loss), and easy to parse, but impossible to read
// for a human.
//#define OUTPUT_BINARY_ACCELGYRO





//horizontalSpeed is the forward velocity of the blimp, an integer from -255-255
//turn is the amount the vehichle will turn -255-255
void moveBlimpHorizontal(int horizontalSpeed, int turn) {

  int righthorizontalSpeed = horizontalSpeed + turn/2;
  int lefthorizontalSpeed = horizontalSpeed - turn/2;
  
  if(lefthorizontalSpeed > 255) {
    lefthorizontalSpeed = 255;
  }
  if(righthorizontalSpeed > 255) {
    righthorizontalSpeed = 255;
  }
  if(lefthorizontalSpeed < -255) {
    lefthorizontalSpeed = -255;
  }
    if(righthorizontalSpeed < -255) {
    righthorizontalSpeed = -255;
  }

  Serial.print("Right: "); Serial.println(righthorizontalSpeed);
  Serial.print("Left: "); Serial.println(lefthorizontalSpeed);
  
  if(righthorizontalSpeed < 0 ) {
    digitalWrite(foward_right_input1,HIGH);
    digitalWrite(foward_right_input2,LOW);
    ledcWrite(forward_right_pwmChannel, -righthorizontalSpeed);
  }
  
  else if(righthorizontalSpeed > 0) {
    Serial.println(turn);
    digitalWrite(foward_right_input1,LOW);
    digitalWrite(foward_right_input2,HIGH);
    ledcWrite(forward_right_pwmChannel, righthorizontalSpeed);

  }

  if(lefthorizontalSpeed < 0){
    digitalWrite(foward_left_input1,HIGH);
    digitalWrite(foward_left_input2,LOW);
    ledcWrite(forward_left_pwmChannel, -lefthorizontalSpeed);
   }
  else if(lefthorizontalSpeed > 0) {
    digitalWrite(foward_left_input1,LOW);
    digitalWrite(foward_left_input2,HIGH);
    ledcWrite(forward_left_pwmChannel, lefthorizontalSpeed);
   }

  //if the horizontalSpeed of both motors is 0 disable the motors
  if(righthorizontalSpeed == 0) {
    digitalWrite(foward_right_input1,LOW);
    digitalWrite(foward_right_input2,LOW);
    
  }

  if(lefthorizontalSpeed == 0) {
    digitalWrite(foward_left_input1,LOW);
    digitalWrite(foward_left_input2,LOW);
  }
}


void moveBlimpVertical(int verticalSpeed) {
  Serial.println(verticalSpeed);
  if(verticalSpeed > 0) {
    digitalWrite(vertical_forward_input1,LOW);
    digitalWrite(vertical_forward_input2,HIGH);
    ledcWrite(vertical_forward_pwmChannel, verticalSpeed);
    digitalWrite(vertical_rear_input1,LOW);
    digitalWrite(vertical_rear_input2,HIGH);
    ledcWrite(vertical_rear_pwmChannel, verticalSpeed);
  }
  else if(verticalSpeed < 0) {
    digitalWrite(vertical_forward_input1,HIGH);
    digitalWrite(vertical_forward_input2,LOW);
    ledcWrite(vertical_forward_pwmChannel, verticalSpeed);
    digitalWrite(vertical_rear_input1,HIGH);
    digitalWrite(vertical_rear_input2,LOW);
    ledcWrite(vertical_rear_pwmChannel, verticalSpeed);
  }
  
  else {
    digitalWrite(vertical_forward_input1,LOW);
    digitalWrite(vertical_forward_input2,LOW);

    digitalWrite(vertical_rear_input1,LOW);
    digitalWrite(vertical_rear_input2,LOW);

  }
}


void setup() {
  Serial.begin(115200);

  SerialBT.begin("Blimp"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");


  pinMode(forward_right_enablePin,OUTPUT);
  pinMode(foward_right_input1,OUTPUT);
  pinMode(foward_right_input2,OUTPUT);
  
  pinMode(forward_left_enablePin,OUTPUT);
  pinMode(foward_left_input1,OUTPUT);
  pinMode(foward_left_input2,OUTPUT);

  pinMode(vertical_forward_enablePin,OUTPUT);
  pinMode(vertical_forward_input1,OUTPUT);
  pinMode(vertical_forward_input2,OUTPUT);

  pinMode(vertical_rear_enablePin,OUTPUT);
  pinMode(vertical_rear_input1,OUTPUT);
  pinMode(vertical_rear_input2,OUTPUT);

  pinMode(battery_voltage,INPUT);


    // configure LED PWM functionalitites
  ledcSetup(forward_right_pwmChannel, freq, resolution);
  ledcSetup(forward_left_pwmChannel, freq, resolution);
  ledcSetup(vertical_forward_pwmChannel, freq, resolution);
  ledcSetup(vertical_rear_pwmChannel, freq, resolution);

  
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(forward_right_enablePin, forward_right_pwmChannel);
  ledcAttachPin(forward_left_enablePin, forward_left_pwmChannel);
  ledcAttachPin(vertical_forward_enablePin, vertical_forward_pwmChannel);
  ledcAttachPin(vertical_rear_enablePin, vertical_rear_pwmChannel);


  

  //  bluetooth app setup
  while(!connectedFlag)
{
  voltage = analogRead(battery_voltage);
  float voltageFloat = voltage * (6.6 / 4095);
  String voltageToReport = "*F";
  voltageToReport = voltageToReport + voltageFloat + "*";
  SerialBT.println(voltageToReport);


  
  SerialBT.println("*.kwl");
  SerialBT.println("clear_panel()");
  SerialBT.println("set_grid_size(17,8)");
  SerialBT.println("add_text(8,3,xlarge,C,Please enter the flight code:,245,240,245,)");
  SerialBT.println("add_send_box(6,5,5,,C,E)");
  SerialBT.println("add_accelerometer(5,5,1,100,A,*)");
  SerialBT.println("set_panel_notes(-,,,)");
  SerialBT.println("run()");
  SerialBT.println("*");
  delay(1000);
  if(SerialBT.available())
  {
    while(!passkey.equals(code))
    {
      dataIn = SerialBT.read();
     
      if(dataIn == 'C')
      {
        passkey = "";
        while(dataIn != 'E')
        {
          if(SerialBT.available())
          {
            dataIn = SerialBT.read();
            Serial.println(dataIn);
            if(dataIn != 'E') passkey += dataIn;
          }
        }
      }
    }

    connectedFlag = true;
    SerialBT.println("*.kwl");
    SerialBT.println("clear_panel()");
    SerialBT.println("set_grid_size(17,8)");
    SerialBT.println("add_text(3,1,xlarge,L,  Time,245,240,245,)");
    SerialBT.println("add_text(12,1,xlarge,L,  Fuel,245,240,245,)");
    SerialBT.println("add_free_pad(13,4,0,100,0,100,R,)");
    SerialBT.println("add_free_pad(1,4,0,100,0,0,L,)");
    SerialBT.println("add_gauge(10,2,4,300,450,365,F,3,4.5,3,5)");
    SerialBT.println("add_gauge(1,2,4,0,10,0,T,,,10,5)");
    SerialBT.println("add_text(8,5,xlarge,C,,245,0,0,E)");
    SerialBT.println("add_xy_graph(7,0,3,-100.0,100.0,-100.0,100.0,5,H,,Pitch,Roll,0,0,0,0,1,1,0,0,0,none,large,1,1,42,97,222)");
    SerialBT.println("set_panel_notes(,,,)");
    SerialBT.println("run()");
    SerialBT.println("*");
    Serial.println("Setup complete");
    delay(1000);
  }
}


//Accelerometer Setup

// join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif

    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    // use the code below to change accel/gyro offset values
    
    Serial.println("Updating internal sensor offsets...");
    // -76  -2359 1688  0 0 0
    accelgyro.setXAccelOffset(473);
    accelgyro.setYAccelOffset(-3973);
    accelgyro.setZAccelOffset(1325);
    accelgyro.setXGyroOffset(142);
    accelgyro.setYGyroOffset(12);
    accelgyro.setZGyroOffset(26);


  
  // testing
  Serial.print("Testing DC Motor...");
}

void loop() {

  if (SerialBT.available()) {

    char temp = SerialBT.read();
    // Serial.write(temp);
    if (temp == 'L'){
      if (SerialBT.read() == 'X'){
        char c;
        String num = "";
        while(isdigit((c = SerialBT.read()))) {
   
          num += c;
         
        }
       
        int Xval = num.toInt();
        turn = map(Xval,0,100,-255,255);

       
        num = "";
        while(isdigit((c = SerialBT.read()))) {
   
          num += c;
         
        }
        int Yval = num.toInt();
        verticalSpeed = map(Yval, 0, 100, -255, 255);

       
      }
    }
    else {
      if (SerialBT.read() == 'X'){
        char c;
        String num = "";
        while(isdigit((c = SerialBT.read()))) {
   
          num += c;
         
        }
       
        int Xval = num.toInt();


       
        num = "";
        while(isdigit((c = SerialBT.read()))) {
   
          num += c;
         
        }
        int Yval = num.toInt();
        horizontalSpeed = map(Yval,0,100,-255,255);

       
      }
    }
   
   
  }

//  }

// read raw accel/gyro measurements from device
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // these methods (and a few others) are also available
    //accelgyro.getAcceleration(&ax, &ay, &az);
    //accelgyro.getRotation(&gx, &gy, &gz);
  float axg,ayg,azg;


        // display tab-separated accel/gyro x/y/z values
  axg = (5.9814*pow(10,-4)) * ax + 2.1984*pow(10,-16);
  ayg = (5.9814*pow(10,-4)) * ay + 2.1984*pow(10,-16);
  azg = (5.9814*pow(10,-4)) * az + 2.1984*pow(10,-16);
  Serial.print("a/g:\t");
  Serial.print(axg); Serial.print("\t");
  Serial.print(ayg); Serial.print("\t");
  Serial.print(azg); Serial.print("\t");
  Serial.print(gx); Serial.print("\t");
  Serial.print(gy); Serial.print("\t");
  Serial.println(gz);

  float deltaP = acos(azg/11) * 180/3.14;
  float deltaR = asin(axg/19.6) * 180/3.14;

  Serial.print("Pitch: "); Serial.println(deltaP);
  Serial.print("Roll: "); Serial.println(deltaR);
  
  String mpuData = "*HX";
  mpuData = mpuData + deltaR + "Y" + deltaP + "*";
  SerialBT.println(mpuData);

  moveBlimpHorizontal(horizontalSpeed,turn);
  moveBlimpVertical(verticalSpeed);

  
  voltage = analogRead(battery_voltage);
  float voltageFloat = (voltage * 660) / 4095;
  voltageMovingAverage = (7 * voltageFloat + voltageMovingAverage)/8;
  int voltageInt = round(voltageFloat) + 20;
  String voltageToReport = "*F";
  voltageToReport = voltageToReport + voltageInt + "*";
  SerialBT.println(voltageToReport);
  if(voltageInt <= 340)
  {
    SerialBT.println("*ETime to Land*");
  }
  else{
    SerialBT.println("*E*");
  }
//  delay(20);
}
