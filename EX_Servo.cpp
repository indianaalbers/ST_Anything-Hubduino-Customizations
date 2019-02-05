//******************************************************************************************
//  File: EX_Servo.cpp
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
#include "EX_Servo.h"

#include "Constants.h"
#include "Everything.h"

#if not defined(ARDUINO_ARCH_ESP32)

namespace st
{
	//private
	void EX_Servo::writeAngleToPin()
	{
		if (!m_Servo.attached()) {
			m_Servo.attach(m_nPinPWM);
		}

		if(m_nTargetAngle < 0) {
			m_nTargetAngle = 0;
		}
		else if(m_nTargetAngle > 180) {
			m_nTargetAngle = 180;
		}

		if (m_nTargetAngle == m_nOldAngle) {  
			m_Servo.write(m_nTargetAngle);
			delay(1000);
		}
		else {
			int n = abs(m_nTargetAngle - m_nOldAngle);
			int timeStep = (m_nCurrentDuration *1000 / 100);  //Constant servo step rate assumes duration is the time desired for maximum level change of 100
			m_nCurrentAngle = m_nOldAngle;
			for(int i = 1; i <= n; ++i) {
				if (m_nTargetAngle >= m_nOldAngle) {
					m_nCurrentAngle = m_nCurrentAngle + 1;
				}
				else {
					m_nCurrentAngle = m_nCurrentAngle - 1;
				}
				
				//if (st::Executor::debug) {
				//	Serial.print(F("EX_Servo::writeAngle ToPin  currentAngle = "));
				//	Serial.println(m_nCurrentAngle);
				//}
				
				m_Servo.write(m_nCurrentAngle);
				delay(timeStep / 1.8);  // divide by fudge factor to adjust theoretical delay to compensate for processing time.  1.8 works on Mega.
			}
		}
		
		if (st::Executor::debug) {
			Serial.print(F("EX_Servo:: Servo motor angle set to "));
			Serial.println(m_nTargetAngle);
		}
		
		if (m_bDetachAfterMove) {
			//delay(m_nCurrentDuration * 1000);   //to do: remove this after creating method for smoothly moving servo over time specified by duration
			m_Servo.detach();
		}

	}

	//public
	//constructor
	EX_Servo::EX_Servo(const __FlashStringHelper *name, byte pinPWM, int startingLevel, bool detachAfterMove, int servoMoveTime,  int zeroLevelAngle, int benjLevelAngle) :
		Executor(name),
		m_Servo(),
		m_nCurrentLevel(startingLevel),
		m_bDetachAfterMove(detachAfterMove),
		m_nCurrentDuration(servoMoveTime / 1000),
		m_nZeroLevelAngle(zeroLevelAngle),
		m_nBenjLevelAngle(benjLevelAngle)
	{
		setPWMPin(pinPWM);
		
		m_nTargetAngle = map(m_nCurrentLevel, 0, 100, m_nZeroLevelAngle, m_nBenjLevelAngle);
		m_nOldAngle = m_nTargetAngle;
	}

	//destructor
	EX_Servo::~EX_Servo()
	{

	}

	void EX_Servo::init()
	{
		writeAngleToPin();
		refresh();
	}

	void EX_Servo::beSmart(const String &str)  
	{
		String level = str.substring(str.indexOf(' ') + 1, str.indexOf(':'));
		String duration = str.substring(str.indexOf(':') + 1);
       
		level.trim();
		duration.trim();
		
		if (st::Executor::debug) {
			Serial.print(F("EX_Servo::beSmart level = "));
			Serial.println(level);
			Serial.print(F("EX_Servo::beSmart duration = "));
			Serial.println(duration);
		}
				
		m_nCurrentLevel = int(level.toInt());
		m_nCurrentDuration = int(duration.toInt());
		m_nOldAngle = m_nTargetAngle;
		m_nTargetAngle = map(m_nCurrentLevel, 0, 100, m_nZeroLevelAngle, m_nBenjLevelAngle);

		if (st::Executor::debug) {
			Serial.print(F("EX_Servo::beSmart OldAngle = "));
			Serial.println(m_nOldAngle);
			Serial.print(F("EX_Servo::beSmart TargetAngle = "));
			Serial.println(m_nTargetAngle);
		}
		writeAngleToPin();
		refresh();

	}

	void EX_Servo::refresh()
	{
		Everything::sendSmartString(getName() + " " + String(m_nCurrentLevel) + ":" + String(m_nTargetAngle) + ":" + String(m_nCurrentDuration));
	}

	void EX_Servo::setPWMPin(byte pin)
	{
		m_nPinPWM = pin;
	}
}

#endif
