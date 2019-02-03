//******************************************************************************************
//  File: ST_Anything_Ethernet_Mega_Hubitat_Test_Servo_V1_2
//  Based on 019 Jan 01 download of: ST_Anything_Multiples_EthernetW5100_UNO.ino
//  Authors: Dan G Ogorchock & Daniel J Ogorchock (Father and Son)
//
//  Summary:  This Arduino Sketch, along with the ST_Anything library and the revised SmartThings 
//            library, demonstrates the ability of one Arduino + Ethernet W5200 Shield to 
//            implement a multi input/output custom device for integration into SmartThings.
//            The ST_Anything library takes care of all of the work to schedule device updates
//            as well as all communications with the Ethernet W5200 Shield.
//
//              - 2 x Servo
//
//              - 2 total devices
//
//            During the development of this re-usable library, it became apparent that the 
//            Arduino UNO R3's very limited 2K of SRAM was very limiting in the number of 
//            devices that could be implemented simultaneously.  A tremendous amount of effort
//            has gone into reducing the SRAM usage, including siginificant improvements to
//            the SmartThings Arduino library.
//
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2015-01-03  Dan & Daniel   Original Creation
//    2017-02-12  Dan Ogorchock  Revised to use the new SMartThings v2.0 library
//    2017-04-16  Dan Ogorchock  New sketch to demonstrate multiple SmartThings Capabilties of each type
//    2017-04-22  Dan Ogorchock  Added Voltage, Carbon Monoxide, and Alarm with Strobe
//    2017-07-04  Dan Ogorchock  Added MQ-2 based Smoke Detection
//    2018-04-15  Dan Ogorchock  Modified/simplified for use with an Arduino UNO R3
//    2019-01-06  Jeff Albers    Set up for Arduino Mega with Ethernet W5200 shield connected to Hubitat using 1.1 internal Vref
//    2019-02-02  Jeff Albers    Added servo control arguments for custom travel endpoints, controlled movement rate
//
//******************************************************************************************
//******************************************************************************************
// SmartThings Library for Arduino Ethernet W5200 Shield
//******************************************************************************************
#include <SmartThingsEthernetW5200.h>    //Library to provide API to the SmartThings Ethernet W5200 Shield.  Based on ST W5500 library

//******************************************************************************************
// ST_Anything Library 
//******************************************************************************************
#include <Constants.h>       //Constants.h is designed to be modified by the end user to adjust behavior of the ST_Anything library
#include <Device.h>          //Generic Device Class, inherited by Sensor and Executor classes
#include <Sensor.h>          //Generic Sensor Class, typically provides data to ST Cloud (e.g. Temperature, Motion, etc...)
#include <Executor.h>        //Generic Executor Class, typically receives data from ST Cloud (e.g. Switch)
#include <InterruptSensor.h> //Generic Interrupt "Sensor" Class, waits for change of state on digital input 
#include <PollingSensor.h>   //Generic Polling "Sensor" Class, polls Arduino pins periodically
#include <Everything.h>      //Master Brain of ST_Anything library that ties everything together and performs ST Shield communications

#include <EX_Servo.h>        //Implements Executor (EX)as an Switch Level capability via a PWM output to a servo motor

//**********************************************************************************************************
//Define which Arduino Pins will be used for each device
//  Notes: Arduino communicates with both the W5200 and SD card using the SPI bus (through the ICSP header). 
//         This is on digital pins 10, 11, 12, and 13 on the Uno and pins 50, 51, and 52 on the Mega. 
//         On both boards, pin 10 is used to select the W5200 and pin 4 for the SD card. 
//         These pins cannot be used for general I/O. On the Mega, the hardware SS pin, 53, 
//         is not used to select either the W5200 or the SD card, but it must be kept as an output 
//         or the SPI interface won't work.
//         See https://www.seeedstudio.com/W5200-Ethernet-Shield-p-1577.html for details on the W5200 Shield
//**********************************************************************************************************
//"RESERVED" pins for W5200 Ethernet Shield - best to avoid
#define PIN_4_RESERVED            4   //reserved by W5200 Shield on both UNO and MEGA
#define PIN_1O_RESERVED           10  //reserved by W5200 Shield on both UNO and MEGA
#define PIN_11_RESERVED           11  //reserved by W5200 Shield on UNO
#define PIN_12_RESERVED           12  //reserved by W5200 Shield on UNO
#define PIN_13_RESERVED           13  //reserved by W5200 Shield on UNO
#define PIN_50_RESERVED           50  //reserved by W5200 Shield on MEGA
#define PIN_51_RESERVED           51  //reserved by W5200 Shield on MEGA
#define PIN_52_RESERVED           52  //reserved by W5200 Shield on MEGA
#define PIN_53_RESERVED           53  //reserved by W5200 Shield on MEGA

//Servo Pins
#define PIN_SERVO_1    5  //SmartThings Capabilty "Switch Level"
#define PIN_SERVO_2    6  //SmartThings Capabilty "Switch Level"


//******************************************************************************************
//W5200 Ethernet Shield Information
//****************************************************************************************** 
byte mac[] = {0x06,0xFF,0xFF,0xFF,0xFF,0x18}; //MAC address, leave first octet 0x06, change others to be unique //  <---You must edit this line!
IPAddress ip(192, 168, 1, 18);               //Arduino device IP Address                   //  <---You must edit this line!
IPAddress gateway(192, 168, 1, 1);            //router gateway                              //  <---You must edit this line!
IPAddress subnet(255, 255, 255, 0);           //LAN subnet mask                             //  <---You must edit this line!
IPAddress dnsserver(192, 168, 1, 1);          //DNS server                                  //  <---You must edit this line!
const unsigned int serverPort = 8118;         // port to run the http server on

// Smartthings / Hubitat Hub TCP/IP Address
IPAddress hubIp(192, 168, 1, 21);    // smartthings/hubitat hub ip //  <---You must edit this line!

// SmartThings / Hubitat Hub TCP/IP Address: UNCOMMENT line that corresponds to your hub, COMMENT the other
//const unsigned int hubPort = 39500;   // smartthings hub port
const unsigned int hubPort = 39501;   // hubitat hub port

//******************************************************************************************
//st::Everything::callOnMsgSend() optional callback routine.  This is a sniffer to monitor 
//    data being sent to ST.  This allows a user to act on data changes locally within the 
//    Arduino sktech withotu having to rely on the ST Cloud for time-critical tasks.
//******************************************************************************************
void callback(const String &msg)
{
  //Uncomment if it weould be desirable to using this function
  //Serial.print(F("ST_Anything_Miltiples Callback: Sniffed data = "));
  //Serial.println(msg);
  
  //TODO:  Add local logic here to take action when a device's value/state is changed
  
  //Masquerade as the ThingShield to send data to the Arduino, as if from the ST Cloud (uncomment and edit following line(s) as you see fit)
  //st::receiveSmartString("Put your command here!");  //use same strings that the Device Handler would send
}

//******************************************************************************************
//Arduino Setup() routine
//******************************************************************************************
void setup()
{
  //******************************************************************************************
  //Declare each Device that is attached to the Arduino
  //  Notes: - For each device, there is typically a corresponding "tile" defined in your 
  //           SmartThings Device Hanlder Groovy code, except when using new COMPOSITE Device Handler
  //         - For details on each device's constructor arguments below, please refer to the 
  //           corresponding header (.h) and program (.cpp) files.
  //         - The name assigned to each device (1st argument below) must match the Groovy
  //           Device Handler names.  (Note: "temphumid" below is the exception to this rule
  //           as the DHT sensors produce both "temperature" and "humidity".  Data from that
  //           particular sensor is sent to the ST Hub in two separate updates, one for 
  //           "temperature" and one for "humidity")
  //         - The new Composite Device Handler is comprised of a Parent DH and various Child
  //           DH's.  The names used below MUST not be changed for the Automatic Creation of
  //           child devices to work properly.  Simply increment the number by +1 for each duplicate
  //           device (e.g. contact1, contact2, contact3, etc...)  You can rename the Child Devices
  //           to match your specific use case in the ST Phone Application.
  //******************************************************************************************

  static st::EX_Servo executor1(F("servo1"), PIN_SERVO_1, 50, true, 1000, 0, 180);  //pin, start angle, detach flag, detach time (msec) upon init, level 0 angle, level 100 angle
  static st::EX_Servo executor2(F("servo2"), PIN_SERVO_2, 50, false, 1000, 135, 45);  //pin, start angle, detach flag, detach time (msec) upon init, level 0 angle, level 100 angle
  //to do: add optional arguments for non-linear mapping coefficients to EX_Servo
  
  //*****************************************************************************
  //  Configure debug print output from each main class 
  //*****************************************************************************
  st::Everything::debug=true;
  st::Executor::debug=true;
  st::Device::debug=true;
  st::PollingSensor::debug=true;
  st::InterruptSensor::debug=true;

  //*****************************************************************************
  //Initialize the "Everything" Class
  //*****************************************************************************
  
  //Initialize the optional local callback routine (safe to comment out if not desired)
  st::Everything::callOnMsgSend = callback;
  
  //Create the SmartThings EthernetW5200 Communications Object
    //STATIC IP Assignment - Recommended
    st::Everything::SmartThing = new st::SmartThingsEthernetW5200(mac, ip, gateway, subnet, dnsserver, serverPort, hubIp, hubPort, st::receiveSmartString);
 
    //DHCP IP Assigment - Must set your router's DHCP server to provice a static IP address for this device's MAC address
    //st::Everything::SmartThing = new st::SmartThingsEthernetW5200(mac, serverPort, hubIp, hubPort, st::receiveSmartString);

  //Run the Everything class' init() routine which establishes Ethernet communications with the SmartThings Hub
  st::Everything::init();
  
  //*****************************************************************************
  //Add each executor to the "Everything" Class
  //*****************************************************************************
  st::Everything::addExecutor(&executor1);
  st::Everything::addExecutor(&executor2);
  
  //*****************************************************************************
  //Initialize each of the devices which were added to the Everything Class
  //*****************************************************************************
  st::Everything::initDevices();
}

//******************************************************************************************
//Arduino Loop() routine
//******************************************************************************************
void loop()
{
  //*****************************************************************************
  //Execute the Everything run method which takes care of "Everything"
  //*****************************************************************************
  st::Everything::run();
}
