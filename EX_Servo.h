//******************************************************************************************
//  File: EX_Servo.h
//  Authors: Dan G Ogorchock
//
//  Summary:  EX_Servo is a class which implements the SmartThings/Hubitat "Switch Level" device capability.
//			  It inherits from the st::Executor class.
//
//			  Create an instance of this class in your sketch's global variable section
//			  For Example:  st::EX_Servo executor1(F("servo1"), PIN_SERVO, INITIAL_ANGLE, false, 1000);
//
//			  st::EX_Servo() constructor requires the following arguments
//				- String &name - REQUIRED - the name of the object - must match the Groovy ST_Anything DeviceType tile name
//				- byte pin_pwm - REQUIRED - the Arduino Pin to be used as a pwm output
//				- int startingLevel - OPTIONAL - the value desired for the initial level of the servo motor (0-100, defaults to 50)
//              - bool detachAfterMove - OPTIONAL - determines if servo motor is powered down after move (defaults to false) 
//              - int servoMoveTime - OPTIONAL - determines how long after the servo is moved that the servo is powered down if the above is true (defaults to 1000ms)
//				- int zeroLevelAngle - OPTIONAL - servo angle in degrees to map to level 0 (defaults to 0 degrees)
//				- int benjLevelAngle - OPTIONAL - servo angle in degrees to map to level 100 (defaults to 180 degrees)
//
//  Change History:
//
//    Date        Who            What
//    ----        ---            ----
//    2018-06-23  Dan Ogorchock  Original Creation
//    2018-06-24  Dan Ogorchock  Since ESP32 does not support SERVO library, exclude all code to prevent compiler error
//    2018-08-19  Dan Ogorchock  Added feature to optionally allow servo to be powered down after a move
//	  2019-02-02  Jeff Albers	 Added Parameters to map servo endpoints, actively control rate of servo motion via duration input to device driver, intializes to level instead of angle
//
//
//******************************************************************************************
#ifndef ST_EX_SERVO
#define ST_EX_SERVO

#if not defined(ARDUINO_ARCH_ESP32)

#include "Executor.h"
#include <Servo.h>

namespace st
{
	class EX_Servo: public Executor
	{
		private:
			Servo m_Servo;          //Servo object
			byte m_nPinPWM;			//Arduino Pin used as a PWM Output for the switch level capability
			int m_nCurrentLevel;	//Servo Level value from SmartThings (0 to 100)
			int m_nOldAngle;		//starting angle for servo move
			int m_nTargetAngle;		//ending angle for servo move, value mapped to Servo Level (0 to 180 degrees maximum range)
			int m_nCurrentAngle;	//servo angle output while stepping from OldAngle to TargetAngle
			int m_nCurrentDuration;	//Servo move Duration value from SmartThings (1 to 10 seconds) 
			bool m_bDetachAfterMove;//Issue a servo.detach() after servo move is complete
			int m_nZeroLevelAngle;	//Angle (0-180 degrees)to map to level 0
			int m_nBenjLevelAngle;	//Angle (0-180 degrees)to map to level 100

			void writeAngleToPin();	//function to update the Arduino PWM Output Pin

		public:
			//constructor - called in your sketch's global variable declaration section
			EX_Servo(const __FlashStringHelper *name, byte pinPWM, int startingLevel = 50, bool detachAfterMove = false, int servoMoveTime = 1000, int zeroLevelAngle = 0, int benjLevelAngle = 180);
			
			//destructor
			virtual ~EX_Servo();

			//initialization routine
			virtual void init();

			//SmartThings Shield data handler (receives Level and Duration from ST/Hubitat)
			virtual void beSmart(const String &str);
			
			//called periodically to ensure state of the switch is up to date in the SmartThings Cloud / Hubitat Hub (in case an event is missed)
			virtual void refresh();
			
			//gets
			virtual byte getPWMPin() const { return m_nPinPWM; }

			virtual bool getAngle() const { return m_nTargetAngle; }	//Angle of the Servo mapped to Level
			virtual bool getLevel() const { return m_nCurrentLevel; }	//Servo Level from ST/Hubitat
			virtual bool getDuration() const { return m_nCurrentDuration; }	//Duration from ST/Hubitat

			//sets
			virtual void setPWMPin(byte pin);
	};
}

#endif
#endif