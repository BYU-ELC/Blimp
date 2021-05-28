
//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"

#include "string.h"


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;


//Each motor has three GPIO pins, input1 and input2 control the direction the motor is spinning and enable is a PWM pin that controls horizontalSpeed

//Pin numbers for forward facing right motor
const int forward_right_enablePin = 2;
const int foward_right_input1 = 15;
const int foward_right_input2 = 0;
const int forward_right_pwmChannel = 0;

//Pin numbers for forward facing left motor
const int forward_left_enablePin = 16;
const int foward_left_input1 = 17;
const int foward_left_input2 = 4;
const int forward_left_pwmChannel = 1;


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





//horizontalSpeed is the forward velocity of the blimp, an integer from -255-255
//turn is the amount the vehichle will turn -255-255
void moveBlimpHorizontal(int horizontalSpeed, int turn) {

  int righthorizontalSpeed = horizontalSpeed + turn;
  int lefthorizontalSpeed = horizontalSpeed - turn;
  
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
  if(righthorizontalSpeed == 0 && lefthorizontalSpeed == 0) {
    digitalWrite(foward_right_input1,LOW);
    digitalWrite(foward_right_input2,LOW);
    digitalWrite(foward_left_input1,LOW);
    digitalWrite(foward_left_input2,LOW);
  }
}


void moveBlimpVertical(int verticalSpeed) {
  if(verticalSpeed > 0) {
    //Move up
  }
  else if(verticalSpeed < 0) {
    //move down
  }
  
  else {
    //disable motors
  }
}


void setup() {
  Serial.begin(115200);

  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");


  pinMode(forward_right_enablePin,OUTPUT);
  pinMode(foward_right_input1,OUTPUT);
  pinMode(foward_right_input2,OUTPUT);
  
  pinMode(forward_left_enablePin,OUTPUT);
  pinMode(foward_left_input1,OUTPUT);
  pinMode(foward_left_input2,OUTPUT);


    // configure LED PWM functionalitites
  ledcSetup(forward_right_pwmChannel, freq, resolution);
  ledcSetup(forward_left_pwmChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(forward_right_enablePin, forward_right_pwmChannel);
  ledcAttachPin(forward_left_enablePin, forward_left_pwmChannel);

  //  bluetooth app setup
  while(!connectedFlag)
{
  SerialBT.println("*.kwl");
  SerialBT.println("clear_panel()");
  SerialBT.println("set_grid_size(17,8)");
  SerialBT.println("add_text(8,3,xlarge,C,Please enter the flight code:,245,240,245,)");
  SerialBT.println("add_send_box(6,5,5,,C,E)");
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
    SerialBT.println("add_gauge(10,2,4,0,100,0,F,,,10,5)");
    SerialBT.println("add_gauge(1,2,4,0,10,0,T,,,10,5)");
    SerialBT.println("set_panel_notes(,,,)");
    SerialBT.println("run()");
    SerialBT.println("*");
    Serial.println("Setup complete");
    delay(1000);
  }
}


  
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
  moveBlimpHorizontal(horizontalSpeed,turn);
  moveBlimpVertical(verticalSpeed);
//  delay(20);
}
