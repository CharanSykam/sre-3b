#ifndef _SAFETY_H
#define _SAFETY_H

#include "IO_Driver.h"

#include "torqueEncoder.h"
#include "brakePressureSensor.h"
#include "sensors.h"
#include "motorController.h"
#include "bms.h"
#include "serial.h"

/*
typedef enum { CHECK_tpsOutOfRange    , CHECK_bpsOutOfRange
			 , CHECK_tpsOpenOrShort   , CHECK_bpsOpenOrShort
			 , CHECK_tpsNotCalibrated , CHECK_bpsNotCalibrated 
			 , CHECK_tpsOutOfSync     , CHECK_tpsbpsImplausible
			 } SafetyCheck;
*/

typedef struct _SafetyChecker SafetyChecker;

SafetyChecker* SafetyChecker_new(SerialManager* sm);
void SafetyChecker_update(SafetyChecker* me, TorqueEncoder* tps, BrakePressureSensor* bps, Sensor* HVILTermSense, Sensor* LVBattery);
bool SafetyChecker_allSafe(SafetyChecker* me);
ubyte4 SafetyChecker_getFaults(SafetyChecker* me);
ubyte4 SafetyChecker_getWarnings(SafetyChecker* me);
//bool SafetyChecker_getError(SafetyChecker* me, SafetyCheck check);
//bool SafetyChecker_getErrorByte(SafetyChecker* me, ubyte1* errorByte);

//ubyte2 checkPowerDraw(BatteryManagementSystem* bms, MotorController* mcm);
void checkBatteryPackTemp(BatteryManagementSystem* bms);
ubyte2 checkPowerDraw(BatteryManagementSystem* bms, MotorController* mcm);

#endif //  _SAFETY_H