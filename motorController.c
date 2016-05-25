#include <stdlib.h>  //Needed for malloc
#include "IO_Driver.h"
#include "IO_DIO.h"     //TEMPORARY - until MCM relay control  / ADC stuff gets its own object
#include "IO_RTC.h"
#include "IO_CAN.h"

#include "motorController.h"
#include "mathFunctions.h"
#include "sensors.h"
#include "sensorCalculations.h"

#include "torqueEncoder.h"
#include "readyToDriveSound.h"


extern Sensor Sensor_BenchTPS0;
extern Sensor Sensor_BenchTPS1;

extern Sensor Sensor_RTDButton;
extern Sensor Sensor_EcoButton;
extern Sensor Sensor_TCSSwitchA;
extern Sensor Sensor_TCSSwitchB;
extern Sensor Sensor_HVILTerminationSense;

/*****************************************************************************
 * Motor Controller (MCM)
 ******************************************************************************
 *
 ****************************************************************************/

struct _MotorController {
	//----------------------------------------------------------------------------
	// Controller statuses/properties
	//----------------------------------------------------------------------------
	// These represent the state of the controller (set at run time, not compile
	// time.)  These are updated by canInput.c
	//----------------------------------------------------------------------------
	ubyte2 canMessageBaseId;  //Starting message ID for messages that will come in from this controller
	ubyte4 timeStamp_inverterEnabled;

    //All torque units are in 10ths (500 = 50.0 Nm)
    //Positive = accel, negative = regen
    //Reverse not allowed
	sbyte2 torqueMaximum;  //Max torque that can be commanded in deciNewton*meters ("100" = 10.0 Nm)
    sbyte2 torqueMaximum_Regen;
    sbyte2 torqueRegenAtZeroPedal;
    float4 torquePercentBPSForMaxRegen;

    bool relayState;
    bool previousHVILState;
    ubyte4 timeStamp_HVILLost;

    ubyte1 startupStage;
    Status lockoutStatus;
	Status inverterStatus;
	//bool startRTDS;
	/*ubyte4 vsmStatus0;      //0xAA Byte 0,1
    ubyte4 vsmStatus1;      //0xAA Byte 0,1
    ubyte4 vsmStatus2;      //0xAA Byte 0,1
    ubyte4 vsmStatus3;      //0xAA Byte 0,1
    ubyte4 faultCodesPOST; //0xAB Byte 0-3
    ubyte4 faultCodesRUN;  //0xAB Byte 4-7*/

	//----------------------------------------------------------------------------
	// Control parameters
	//----------------------------------------------------------------------------
	// These are updated by ??? and will be sent to the VCU over CAN
	//----------------------------------------------------------------------------
	//struct _commands {
	ubyte4 timeStamp_lastCommandSent;  //from IO_RTC_StartTime(&)
	ubyte2 updateCount; //Number of updates since lastCommandSent

	sbyte2 commands_torque;
	sbyte2 commands_torqueLimit;
	ubyte1 commands_direction;
    ubyte1 faultHistory[8];

	sbyte2 motor_temp;
	sbyte4 DC_Voltage;
	sbyte4 DC_Current;

	sbyte2 commandedTorque;
	ubyte4 currentPower;

	sbyte2 motorRPM;
	//unused/unused/unused/unused unused/unused/Discharge/Inverter Enable
	Status commands_discharge;
	Status commands_inverter;
	//ubyte1 controlSwitches; // example: 0b00000001 = inverter is enabled, discharge is disabled.

    /*
    //----------------------------------------------------------------------------
    // Control functions - for functions nested within struct
    //----------------------------------------------------------------------------
    void(*setTorque)(MotorController* me, ubyte2 torque); //Will be divided by 10 e.g. pass in 100 for 10.0 Nm
    void(*setDirection)(MotorController* me, Direction rotation);
    void(*setInverter)(MotorController* me, Status inverterState);
    void(*setDischarge)(MotorController* me, Status dischargeState);
    void(*setTorqueLimit)(MotorController* me, ubyte2 torqueLimit);
    //        void(*motorController_setTorque)(MotorController* me, ubyte2 torque); //Will be divided by 10 e.g. pass in 100 for 10.0 Nm
    //        void(*motorController_setDirection)(MotorController* me, Direction rotation);
    //        void(*motorController_setInverter)(MotorController* me, Status inverterState);
    //        void(*motorController_setDischarge)(MotorController* me, Status dischargeState);
    //        void(*motorController_setTorqueLimit)(MotorController* me, ubyte2 torqueLimit);
    void(*updateLockoutStatus)(MotorController* me, Status newState);
    void(*updateInverterStatus)(MotorController* me, Status newState);
    void(*getLockoutStatus)(MotorController* me);
    void(*getInverterStatus)(MotorController* me);
    }
    */
    //_commands commands;
    //};
};

MotorController* MotorController_new(ubyte2 canMessageBaseID, Direction initialDirection, sbyte2 torqueMaxInDNm)
{
	MotorController* me = (MotorController*)malloc(sizeof(struct _MotorController));

	me->canMessageBaseId = canMessageBaseID;
	//Dummy timestamp for last MCU message
	MCM_commands_resetUpdateCountAndTime(me);

	me->lockoutStatus = UNKNOWN;
	me->inverterStatus = UNKNOWN;
	//me->startRTDS = FALSE;

	me->commands_direction = initialDirection;
	me->torqueMaximum = torqueMaxInDNm;
    me->torqueMaximum_Regen = torqueMaxInDNm * -.2;
    me->torqueRegenAtZeroPedal = me->torqueMaximum_Regen * .1;

    me->torquePercentBPSForMaxRegen = 0.5;

    //me->faultHistory = { 0,0,0,0,0,0,0,0 };  //Todo: read from eeprom instead of defaulting to 0

	me->startupStage = 0; //Off
    
    me->relayState = FALSE; //Low

	/*
me->setTorque = &setTorque;
me->setInverter = &setInverter;
me->setDischarge = &setDischarge;
me->setTorqueLimit = &setTorqueLimit;
me->updateLockoutStatus = &updateLockoutStatus;
me->updateInverterStatus = &updateInverterStatus;
me->getLockoutStatus = &getLockoutStatus;
me->getInverterStatus = &getInverterStatus;
		*/
	return me;
}





/*****************************************************************************
* Motor Control Functions
* Reads sensor objects and sets MCM control object values, which will be picked up
* later by CAN function
* > Direction
* > Torque
*   > Delay command after inverter enable (temporary until noise fix)
*   > Calculate Nm to request based on pedal position
*   > Keep track of update count to prevent CANbus overload //MOVE ALL COUNT UPDATES INTO INTERNAL MCM FUNCTIONS
* > Torque limit
*   > Get from TCS function
* Manages the different phases startup/ready-to-drive procedure
* > Turn on MCM relay?  Should this be done elsewhere?
* > Disable inverter lockout
* > Listen for driver to complete startup sequence
* > Enable inverter
* > Play RTDS
****************************************************************************/
void MCM_calculateCommands(MotorController* me, TorqueEncoder* tps, BrakePressureSensor* bps)
{
	//----------------------------------------------------------------------------
	// Control commands
	//----------------------------------------------------------------------------
	//Temp hardcode
	MCM_commands_setDischarge(me, DISABLED);

	//1 = forwards for our car, 0 = reverse
	MCM_commands_setDirection(me, FORWARD);

    //Note: Safety checks (torque command limiting) are done EXTERNALLY.  This is a preliminary calculation
    //which should return the intended torque based on pedals
    
    //NOTE: Brake/regen is currently prioritized over accel for safety.  However, plausibility check can still
    //fail even though car is not even accelerating if both pedals are actuated simultaneously.

    sbyte2 torqueOutput = 0;
    if (bps > 0)
    {
        torqueOutput = me->torqueMaximum_Regen * getPercent(bps->percent, 0, .5, TRUE) + me->torqueRegenAtZeroPedal;
    }
    else
    {
        torqueOutput = (me->torqueMaximum - me->torqueRegenAtZeroPedal) * tps->percent + me->torqueRegenAtZeroPedal;
    }

    MCM_commands_setTorque(me, torqueOutput);
    
}

void MCM_relayControl(MotorController* me, Sensor* HVILTermSense)
{
    //If HVIL Term Sense is low (HV is down)
    if (HVILTermSense->sensorValue == FALSE)
    {
        //If we just noticed the HVIL went low
        if (me->previousHVILState == TRUE)
        {
            IO_RTC_StartTime(&me->timeStamp_HVILLost);
        }

        //If the MCM is on (and we lost HV)
        if (me->relayState == TRUE)
        {
            //Okay to turn MCM off once 0 torque is commanded, or after 2 sec
            //SIMILAR CODE SHOULD BE EMPLOYED AT HVIL SHUTDOWN CONTROL PIN
            if (me->commandedTorque == 0 || IO_RTC_GetTimeUS(me->timeStamp_HVILLost) > 2000000)
            {
                IO_DO_Set(IO_DO_00, FALSE);  //Need MCM relay object
                me->relayState = FALSE;
            }
            else
            {
                //Safety.c will command zero torque
                //For now do nothing
            }
        }
        MCM_setStartupStage(me, 0);
    }
    else
    {
        //If the motor controller is off, don't turn it on until the pedals are calibrated
        //if (MCM_getStartupStage(mcm) == 0)
        //{
        IO_DO_Set(IO_DO_00, FALSE);
        me->relayState = TRUE;
        //MCM_setStartupStage(mcm, 1);
        //}
    }
}

//See diagram at https://onedrive.live.com/redir?resid=F9BB8F0F8FDB5CF8!30410&authkey=!ABSF-uVH-VxQRAs&ithint=file%2chtml
void MotorControllerPowerManagement(MotorController* mcm, TorqueEncoder* tps, ReadyToDriveSound* rtds)
{
	//----------------------------------------------------------------------------
	// Determine inverter state
	//----------------------------------------------------------------------------
	//New Handshake NOTE: Switches connected to ground.. TRUE = high = off = disconnected = open circuit, FALSE = low = grounded = on = connected = closed circuit
	//if (MCM_getLockoutStatus(mcm) == ENABLED)

	//Set inverter to disabled until RTD procedure is done.
	//This disables the lockout ahead of time.
	if (MCM_getStartupStage(mcm) < 3)
	{
		MCM_commands_setInverter(mcm, DISABLED);
		Light_set(Light_dashRTD, 0);
		//MCM_setStartupStage(mcm, 2);
	}

	//if (MCM_getStartupStage(mcm) == 1 && MCM_getRTDSFlag(mcm) == )
	//{
	//	MCM_commands_setInverter(mcm, DISABLED);
	//	MCM_setStartupStage(mcm, 2);
	//}

	//case DISABLED: //Lockout is disabled
	switch (MCM_getInverterStatus(mcm))
	{
	case DISABLED:
		MCM_setStartupStage(mcm, 3); //Lockout disabled, waiting for RTD procedure
		//If not on gas and YES on brake and RTD is pressed
		//BRAKE CODE NEEDS TO BE ADDED HERE
		if (tps->percent < .05 && Sensor_RTDButton.sensorValue == FALSE)
		{
			MCM_commands_setInverter(mcm, ENABLED);
			//MCM_setRTDSFlag(mcm, TRUE);  //Now, start the RTDS if the inverter is successfully enabled
			MCM_setStartupStage(mcm, 4); //RTD complete, waiting for confirmation
		}
		break;

	case ENABLED:
		Light_set(Light_dashRTD, 1);
		//If the inverter was successfully enabled AND we haven't started the RTDS yet
		//if (MCM_getRTDSFlag(mcm) == TRUE)
		if (MCM_getStartupStage(mcm) == 4) //If we're waiting to start the motor controller
		{
			RTDS_setVolume(rtds, .01, 1500000);
			//MCM_setRTDSFlag(mcm, FALSE);  //RTDS started, so don't restart it next loop
			MCM_setStartupStage(mcm, 5); //RTD confirmed
		}
		else
		{
			MCM_setStartupStage(mcm, 6); //Driving
		}
		break;

	case UNKNOWN: default:
		break;
	}

	//break;
	//case UNKNOWN: default:
	//    break;
	//}

	/*
//TEMPORARY Eco Switch startup code
if (Sensor_EcoButton.sensorValue == FALSE)
{
    MCM_commands_setInverter(mcm, DISABLED);
}
else
{
    MCM_commands_setInverter(mcm, ENABLED);
}
//If the inverter is disabled, but we're turning it on now
if (MCM_getInverterStatus(mcm) == DISABLED && MCM_commands_getInverter(mcm) == ENABLED)
{
    MCM_setRTDSFlag(mcm, TRUE);
}
if (MCM_getInverterStatus(mcm) == ENABLED && MCM_getRTDSFlag(mcm) == TRUE)
{
    RTDS_setVolume(rtds, .005, 1500000);
    MCM_setRTDSFlag(mcm, FALSE);  //RTDS started, so don't restart it next loop
}
		*/

}




void MCM_parseCanMessage(MotorController* me, IO_CAN_DATA_FRAME* mcmCanMessage)
{
    switch (mcmCanMessage->id)
    {
    case 0x0A0:
        //0,1 module A temperature
        //2,3 module B temperature
        //4,5 module C temperature
        //6,7 gate driver board temperature
        break;

    case 0x0A1:
        //0,1 control board temp
        //2,3 rtd 1 temp
        //4,5 rtd 2 temp
        //6,7 rtd 3 temp
        break;

    case 0x0A2:
        //0,1 rtd 4 temp
        //2,3 rtd 5 temp
        //4,5 motor temperature***
        me->motor_temp = ((ubyte2)mcmCanMessage->data[5] << 8 | mcmCanMessage->data[4]) / 10;
        //6,7 torque shudder
        break;

    case 0x0A3:
        //0,1 voltage analog input #1
        //2,3 voltage analog input #2
        //4,5 voltage analog input #3
        //6,7 voltage analog input #4
        break;

    case 0x0A4:
        // booleans //
        // 0 digital input #1
        // 1 digital input #2
        // 2 digital input #3
        // 4 digital input #5
        // 5 digital input #6
        // 6 digital input #7
        // 7 digital input #8
        break;

    case 0x0A5:
        //0,1 motor angle (electrical)
        //2,3 motor speed*** // in rpms
        //Cast may be required - needs testing
        me->motorRPM = (ubyte2)mcmCanMessage->data[3] << 8 | mcmCanMessage->data[2];
        //me->motorRPM = ((mcmCanMessage->data[2] << 8) | (mcmCanMessage->data[3]));
        //4,5 electrical output frequency
        //6,7 delta resolver filtered
        break;

    case 0x0A6:
        //0,1 Phase A current
        //2,3 Phase B current
        //4,5 Phase C current
        //6,7 DC bus current
        me->DC_Current = ((ubyte2)mcmCanMessage->data[7] << 8 | mcmCanMessage->data[6]) / 10;
        //me->DC_Current = (((mcmCanMessage->data[6] << 8) | (mcmCanMessage->data[7])) / 10);
        break;

    case 0x0A7:
        //0,1 DC bus voltage***
        me->DC_Voltage = ((ubyte2)mcmCanMessage->data[1] << 8 | mcmCanMessage->data[0]) / 10;
        //me->DC_Voltage = (((mcmCanMessage->data[0] << 8) | (mcmCanMessage->data[1])) / 10);
        //2,3 output voltage
        //4,5 Phase AB voltage
        //6,7 Phase BC voltage
        break;

    case 0x0A8:
        //0,1 Flux Command
        //2,3 flux feedback
        //4,5 id feedback
        //6,7 iq feedback
        break;

    case 0x0A9:
        // 0,1 1.5V reference voltage
        // 2,3 2.5V reference voltage
        // 4,5 5.0V reference voltage
        // 6,7 12V reference voltage
        break;

    case 0x0AA:

        //0,1 VSM state
        //2   Inverter state
        //3   Relay State
        //4   bit-0 inverter run mode
        //4   bit5-7 inverter active discharge state
        //5   inverter command mode
        //6   bit0 inverter enable state***
        me->inverterStatus = mcmCanMessage->data[6];
        //6   bit7 inverter enable lockout***
        me->lockoutStatus = mcmCanMessage->data[6];
        //7   direction command
        break;


    case 0x0AB: //Faults
        //mcmCanMessage->data;
        //me->faultHistory |= data stuff //????????

        break;


    case 0x0AC:
        //0,1 Commanded Torque
        me->commandedTorque = ((ubyte2)mcmCanMessage->data[1] << 8 | mcmCanMessage->data[0]) / 10;
        //2,3 Torque Feedback
        break;

    }
}


/*****************************************************************************
* Accessors / Mutators (Set/Get)
******************************************************************************
*
****************************************************************************/
//Will be divided by 10 e.g. pass in 100 for 10.0 Nm
void MCM_commands_setTorque(MotorController* me, ubyte2 newTorque)
{
	me->updateCount += (me->commands_torque == newTorque) ? 0 : 1;
	me->commands_torque = newTorque;
}

void MCM_commands_setDirection(MotorController* me, Direction newDirection)
{
	switch (newDirection)
	{
	case _0:
	case CLOCKWISE:
	case REVERSE:
		me->updateCount += (me->commands_direction == 0) ? 0 : 1;
		me->commands_direction = 0;
		break;

	case _1:
	case COUNTERCLOCKWISE:
	case FORWARD:
		me->updateCount += (me->commands_direction == 1) ? 0 : 1;
		me->commands_direction = 1;
		break;

	default:
		//Invalid direction?
		break;
	}
}
void MCM_commands_setInverter(MotorController* me, Status newInverterState)
{
	me->updateCount += (me->commands_inverter == newInverterState) ? 0 : 1;
	me->commands_inverter = newInverterState;
}
void MCM_commands_setDischarge(MotorController* me, Status setDischargeTo)
{
	me->updateCount += (me->commands_discharge == setDischargeTo) ? 0 : 1;
	me->commands_discharge = setDischargeTo;
}
void MCM_commands_setTorqueLimit(MotorController* me, ubyte2 newTorqueLimit)
{
	me->updateCount += (me->commands_torqueLimit == newTorqueLimit) ? 0 : 1;
	me->commands_torqueLimit = newTorqueLimit;
}
ubyte2 MCM_commands_getTorque(MotorController* me)
{
	return me->commands_torque;
}
Direction MCM_commands_getDirection(MotorController* me)
{
	return me->commands_direction;
}
Status MCM_commands_getInverter(MotorController* me)
{
	return me->commands_inverter;
}
Status MCM_commands_getDischarge(MotorController* me)
{
	return me->commands_discharge;
}
ubyte2 MCM_commands_getTorqueLimit(MotorController* me)
{
	return me->commands_torqueLimit;
}

void MCM_updateLockoutStatus(MotorController* me, Status newState)
{
	me->lockoutStatus = newState;
}
void MCM_updateInverterStatus(MotorController* me, Status newState)
{
	me->inverterStatus = newState;
}

Status MCM_getLockoutStatus(MotorController* me)
{
	return me->lockoutStatus;
}

Status MCM_getInverterStatus(MotorController* me)
{
	return me->inverterStatus;
}

void MCM_setRTDSFlag(MotorController* me, bool enableRTDS)
{
	//me->updateCount += (me->commands_torqueLimit == newTorqueLimit) ? 0 : 1;
	//me->startRTDS = enableRTDS;
}
bool MCM_getRTDSFlag(MotorController* me)
{
	//me->updateCount += (me->commands_torqueLimit == newTorqueLimit) ? 0 : 1;
	//return me->startRTDS;
	return FALSE;
}

ubyte2 MCM_commands_getUpdateCount(MotorController* me)
{
	return me->updateCount;
}

void MCM_commands_resetUpdateCountAndTime(MotorController* me)
{
	me->updateCount = 0;
	IO_RTC_StartTime(&(me->timeStamp_lastCommandSent));
}

ubyte4 MCM_commands_getTimeSinceLastCommandSent(MotorController* me)
{
	return IO_RTC_GetTimeUS(me->timeStamp_lastCommandSent);
}


ubyte2 MCM_getTorqueMax(MotorController* me)
{
	return me->torqueMaximum;
}



sbyte4 MCM_getPower(MotorController* me)
{
	return ((me->DC_Voltage) * (me->DC_Current));
}

ubyte2 MCM_getCommandedTorque(MotorController* me)
{
	return me->commandedTorque;
}




void MCM_setStartupStage(MotorController* me, ubyte1 stage)
{
	me->startupStage = stage;
}

ubyte1 MCM_getStartupStage(MotorController* me)
{
	return me->startupStage;
}
