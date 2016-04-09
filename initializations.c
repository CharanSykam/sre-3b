//VCU initializations
//Object (sensors, controllers, etc) instantiations
//ONLY THIS FILE should have "true" version of object variables
//Everything else should have "extern" declarations of variables

#include "IO_Driver.h"  //Includes datatypes, constants, etc - probably should be included in every c file
#include "IO_ADC.h"
#include "IO_PWM.h"
#include "IO_CAN.h"
#include "IO_DIO.h"

#include "sensors.h"
#include "initializations.h"
#include "can.h"

/*****************************************************************************
* ADC
****************************************************************************/
//Turns on the VCU's ADC channels and power supplies.
void vcu_initializeADC(bool benchMode)
{
	//----------------------------------------------------------------------------
	//Power supplies/outputs
	//----------------------------------------------------------------------------
	//Analog sensor supplies
    Sensor_TPS0.ioErr_powerSet = Sensor_BPS0.ioErr_powerSet = IO_POWER_Set(IO_ADC_SENSOR_SUPPLY_0, IO_POWER_ON);
	Sensor_TPS1.ioErr_powerSet = IO_POWER_Set(IO_ADC_SENSOR_SUPPLY_1, IO_POWER_ON);

    //Variable power supply (used by BPS)
    //IO_POWER_Set(IO_SENSOR_SUPPLY_VAR, IO_POWER_14_5_V);    //IO_POWER_Set(IO_PIN_269, IO_POWER_8_5_V);

    //Digital/power outputs ---------------------------------------------------
    //Relay power outputs
    IO_DO_Init(IO_DO_00); IO_DO_Set(IO_DO_00, FALSE); //mcm0 Relay
    IO_DO_Init(IO_DO_01); IO_DO_Set(IO_DO_01, FALSE); //HVIL shutdown relay
    IO_DO_Init(IO_DO_02); IO_DO_Set(IO_DO_02, FALSE); //Water pump relay
    IO_DO_Init(IO_DO_03); IO_DO_Set(IO_DO_03, FALSE); //Motor fan relay

	//BMS supply - always on
	IO_DO_Init(IO_DO_04); IO_DO_Set(IO_DO_04, TRUE); //BMS power / Fault LED

    //Wheel Speed Sensor supplies
    Sensor_WSS_FL.ioErr_powerInit = Sensor_WSS_FR.ioErr_powerInit = IO_DO_Init(IO_DO_06); //Front x2
	Sensor_WSS_RL.ioErr_powerInit = Sensor_WSS_RR.ioErr_powerInit = IO_DO_Init(IO_DO_07); //Rear  x2

    //Digital PWM outputs ---------------------------------------------------
	IO_PWM_Init(IO_PWM_02, 500, TRUE, FALSE, 0, FALSE, NULL); IO_PWM_SetDuty(IO_PWM_02, 0, NULL);  //TCS Light
	IO_PWM_Init(IO_PWM_03, 500, TRUE, FALSE, 0, FALSE, NULL); IO_PWM_SetDuty(IO_PWM_03, 0, NULL);  //TCS Light
	IO_PWM_Init(IO_PWM_04, 500, TRUE, FALSE, 0, FALSE, NULL); IO_PWM_SetDuty(IO_PWM_04, 0, NULL);  //Eco Light
    IO_PWM_Init(IO_PWM_05, 500, TRUE, FALSE, 0, FALSE, NULL); IO_PWM_SetDuty(IO_PWM_05, 0, NULL);  //Error Light
    IO_PWM_Init(IO_PWM_06, 500, TRUE, FALSE, 0, FALSE, NULL); IO_PWM_SetDuty(IO_PWM_06, 0, NULL);  //RTD Light
    IO_PWM_Init(IO_PWM_07, 750, TRUE, FALSE, 0, FALSE, NULL); IO_PWM_SetDuty(IO_PWM_07, 0, NULL);  //RTD Sound

    //----------------------------------------------------------------------------
	//ADC channels
	//----------------------------------------------------------------------------
	//TPS
    //IO_ADC_ChannelInit(IO_ADC_5V_00, IO_ADC_RATIOMETRIC, 0, 0, IO_ADC_SENSOR_SUPPLY_0, NULL);
    //IO_ADC_ChannelInit(IO_ADC_5V_01, IO_ADC_RATIOMETRIC, 0, 0, IO_ADC_SENSOR_SUPPLY_1, NULL);

    //BPS
	//Sensor_BPS0.ioErr_init = IO_ADC_ChannelInit(IO_ADC_5V_02, IO_ADC_RATIOMETRIC, 0, 0, IO_ADC_SENSOR_SUPPLY_0, NULL);
	Sensor_BPS0.ioErr_signalInit = IO_ADC_ChannelInit(IO_ADC_5V_02, IO_ADC_RESISTIVE, 0, 0, 0, NULL);

    //Unused
    //IO_ADC_ChannelInit(IO_ADC_5V_03, IO_ADC_RATIOMETRIC, 0, 0, IO_ADC_SENSOR_SUPPLY_0, NULL);

    //Bench TPS
	extern Sensor Sensor_BenchTPS0;
	extern Sensor Sensor_BenchTPS1;
    Sensor_BenchTPS0.ioErr_signalInit = IO_ADC_ChannelInit(IO_ADC_5V_00, IO_ADC_RESISTIVE, 0, 0, 0, NULL);
	Sensor_BenchTPS1.ioErr_signalInit = IO_ADC_ChannelInit(IO_ADC_5V_01, IO_ADC_RESISTIVE, 0, 0, 0, NULL);

    //TCS Pot
    IO_ADC_ChannelInit(IO_ADC_5V_04, IO_ADC_RESISTIVE, 0, 0, 0, NULL);
	//Shockpots
	//IO_ADC_ChannelInit(IO_ADC_5V_05, IO_ADC_RESISTIVE, 0, 0, 0, NULL);
    //IO_ADC_ChannelInit(IO_ADC_5V_06, IO_ADC_RESISTIVE, 0, 0, 0, NULL);
    //IO_ADC_ChannelInit(IO_ADC_5V_07, IO_ADC_RESISTIVE, 0, 0, 0, NULL);

	//----------------------------------------------------------------------------
	//PWD channels
	//----------------------------------------------------------------------------
	//TPS
	Sensor_TPS0.ioErr_signalInit = IO_PWD_PulseInit(IO_PWM_00, IO_PWD_HIGH_TIME);
	Sensor_TPS1.ioErr_signalInit = IO_PWD_PulseInit(IO_PWM_01, IO_PWD_HIGH_TIME);
	
	//Wheel Speed Sensors (Pulse Width Detection)
    Sensor_WSS_FR.ioErr_signalInit = IO_PWD_FreqInit(IO_PWD_08, IO_PWD_RISING_VAR);  //Is there a reason to look for rising vs falling edge?
	Sensor_WSS_RR.ioErr_signalInit = IO_PWD_FreqInit(IO_PWD_09, IO_PWD_RISING_VAR);  //Is there a reason to look for rising vs falling edge?
	Sensor_WSS_FL.ioErr_signalInit = IO_PWD_FreqInit(IO_PWD_10, IO_PWD_RISING_VAR);  //Is there a reason to look for rising vs falling edge?
	Sensor_WSS_RL.ioErr_signalInit = IO_PWD_FreqInit(IO_PWD_11, IO_PWD_RISING_VAR);  //Is there a reason to look for rising vs falling edge?
    
    //----------------------------------------------------------------------------
	//Switches
	//----------------------------------------------------------------------------
	Sensor_RTDButton.ioErr_signalInit = IO_DI_Init(IO_DI_00, IO_DI_PU_10K); //RTD Button
    Sensor_EcoButton.ioErr_signalInit = IO_DI_Init(IO_DI_01, IO_DI_PU_10K); //Eco Button
    Sensor_TCSSwitchA.ioErr_signalInit = IO_DI_Init(IO_DI_02, IO_DI_PU_10K); //TCS Switch A
	Sensor_TCSSwitchB.ioErr_signalInit = IO_DI_Init(IO_DI_03, IO_DI_PU_10K); //TCS Switch B
    Sensor_HVILTerminationSense.ioErr_signalInit = IO_DI_Init(IO_DI_07, IO_DI_PD_10K); //HVIL Term sense, high = HV present

    //

}

//----------------------------------------------------------------------------
// Waste CPU cycles until we have valid data
//----------------------------------------------------------------------------
void vcu_ADCWasteLoop(void)
{
    bool tempFresh = FALSE;
    ubyte2 tempData;
    ubyte4 timestamp_sensorpoll = 0;
    IO_RTC_StartTime(&timestamp_sensorpoll);
    while (IO_RTC_GetTimeUS(timestamp_sensorpoll) < 1000000)
    {
        IO_Driver_TaskBegin();

        IO_PWM_SetDuty(IO_PWM_07, 0, NULL);  //Pin 103

        IO_DO_Set(IO_DO_00, FALSE); //False = low
        IO_DO_Set(IO_DO_01, FALSE); //HVIL shutdown relay
        IO_DO_Set(IO_DO_06, FALSE); //Front WSS x2
        IO_DO_Set(IO_DO_07, FALSE); //Rear  WSS x2

        //IO_DI (digital inputs) supposed to take 2 cycles before they return valid data
        IO_DI_Get(IO_DI_04, &tempData);
        IO_DI_Get(IO_DI_05, &tempData);
        IO_ADC_Get(IO_ADC_5V_00, &tempData, &tempFresh);
        IO_ADC_Get(IO_ADC_5V_01, &tempData, &tempFresh);

        IO_Driver_TaskEnd();
        //TODO: Find out if EACH pin needs 2 cycles or just the entire DIO unit
        while (IO_RTC_GetTimeUS(timestamp_sensorpoll) < 125000);   // wait until 1/8s (125ms) have passed
    }
}

/*****************************************************************************
* CAN
****************************************************************************/
//Defaults
const ubyte1 canMessageLimit = 10;
IO_CAN_DATA_FRAME canMessages[10]; //MUST BE THE SAME NUMBER AS ABOVE
//extern IO_CAN_DATA_FRAME canMessages[]; //  = { { { 0 } } };

//IO_CAN_DATA_FRAME canMessages[canMessageLimit];// = { { { 0 } } };

const ubyte2 canMessageBaseId_VCU = 0x500;
const ubyte2 canSpeed_Channel0 = 500;
const ubyte2 canSpeed_Channel1 = 500;

//These are our four FIFO queues.  All messages should come/go through one of these queues.
ubyte1 canFifoHandle_HiPri_Read;
ubyte1 canFifoHandle_HiPri_Write;
ubyte1 canFifoHandle_LoPri_Read;
ubyte1 canFifoHandle_LoPri_Write;

//Initializes all four can FIFO queues
void vcu_initializeCAN(void)
{
    IO_CAN_DATA_FRAME canMessages[canMessageLimit];// = { { { 0 } } };
    //Activate the CAN channels --------------------------------------------------
    IO_CAN_Init(IO_CAN_CHANNEL_0, canSpeed_Channel0, 0, 0, 0);
    IO_CAN_Init(IO_CAN_CHANNEL_1, canSpeed_Channel1, 0, 0, 0);

    //Configure the FIFO queues
    //This specifies: The handle names for the queues
    //, which channel the queue belongs to
    //, the # of messages (or maximum count?)
    //, the direction of the queue (in/out)
    //, the frame size
    //, and other stuff?
    IO_CAN_ConfigFIFO(&canFifoHandle_HiPri_Read, IO_CAN_CHANNEL_1, canMessageLimit, IO_CAN_MSG_READ, IO_CAN_STD_FRAME, 0, 0);
    IO_CAN_ConfigFIFO(&canFifoHandle_HiPri_Write, IO_CAN_CHANNEL_1, canMessageLimit, IO_CAN_MSG_WRITE, IO_CAN_STD_FRAME, 0, 0);
    IO_CAN_ConfigFIFO(&canFifoHandle_LoPri_Read, IO_CAN_CHANNEL_0, canMessageLimit, IO_CAN_MSG_READ, IO_CAN_STD_FRAME, 0, 0);
    IO_CAN_ConfigFIFO(&canFifoHandle_LoPri_Write, IO_CAN_CHANNEL_0, canMessageLimit, IO_CAN_MSG_WRITE, IO_CAN_STD_FRAME, 0, 0);

/*
    IO_CAN_ConfigMsg(&canFifoHandle_HiPri_Read, IO_CAN_CHANNEL_1, IO_CAN_MSG_READ, IO_CAN_STD_FRAME, 0, 0);
    IO_CAN_ConfigMsg(&canFifoHandle_HiPri_Write, IO_CAN_CHANNEL_1, IO_CAN_MSG_WRITE, IO_CAN_STD_FRAME, 0, 0);
    IO_CAN_ConfigMsg(&canFifoHandle_LoPri_Read, IO_CAN_CHANNEL_0, IO_CAN_MSG_READ, IO_CAN_STD_FRAME, 0, 0);
    IO_CAN_ConfigMsg(&canFifoHandle_LoPri_Write, IO_CAN_CHANNEL_0, IO_CAN_MSG_WRITE, IO_CAN_STD_FRAME, 0, 0);
*/
}

/*****************************************************************************
* Sensors
****************************************************************************/
Sensor Sensor_TPS0;  // = { 0, 0.5, 4.5 };
Sensor Sensor_TPS1;  // = { 0, 4.5, 0.5 };
Sensor Sensor_BPS0;  // = { 1, 0.5, 4.5 };  //Brake system pressure (or front only in the future)
//Sensor Sensor_BPS1;  // = { 2, 0.5, 4.5 }; //Rear brake system pressure (separate address in case used for something else)
Sensor Sensor_WSS_FL;  // = { 2 };
Sensor Sensor_WSS_FR;  // = { 2 };
Sensor Sensor_WSS_RL;  // = { 2 };
Sensor Sensor_WSS_RR;  // = { 2 };
Sensor Sensor_BenchTPS0;
Sensor Sensor_BenchTPS1;
Sensor Sensor_WPS_FL;  // = { 3 };
Sensor Sensor_WPS_FR;  // = { 3 };
Sensor Sensor_WPS_RL;  // = { 3 };
Sensor Sensor_WPS_RR;  // = { 3 };
Sensor Sensor_SAS;  // = { 4 };
Sensor Sensor_LVBattery;

Sensor Sensor_RTDButton;
Sensor Sensor_EcoButton;
Sensor Sensor_TCSSwitchA;
Sensor Sensor_TCSSwitchB;
Sensor Sensor_HVILTerminationSense;

//Switches
//precharge failure

//Other
extern Sensor Sensor_LVBattery; // = { 0xA };  //Note: There will be no init for this "sensor"

//Initialize the sensors with default values
void vcu_initializeSensors(void)
{
    //Torque Encoders (TPS is not really accurate since there's no throttle to position in an EV)
    //TODO: specMin/specMax are ints, won't store decimal values!!!!!!!

	//TPS: Ratiometric voltage sensor
	//Sensor_TPS0.specMin = 0.5;
	//Sensor_TPS0.specMax = 4.5;
	//Sensor_TPS1.specMin = 4.5;
	//Sensor_TPS1.specMax = 0.5;

	//TPS: PWM
	//1kHz frequency = 1ms period (high + low time), 5-95% duty cycle
	//VCU max frequency = 10kHz
	//VCU max sensible pulse: 100ms
	//Sensor min: 1ms * 5% = .000050 = 50 microseconds
	//Sensor max: 1ms * 95% = .000950 = 950 microseconds
	Sensor_TPS0.specMin = .001 * .05;  //1kHz = .001s/cycle...
	Sensor_TPS0.specMax = .001 * .95;
	Sensor_TPS1.specMin = .001 * .95;
	Sensor_TPS1.specMax = .001 * .05;

	//TPS: Pot
	Sensor_BenchTPS0.specMin = 1;
    Sensor_BenchTPS0.specMax = 5001;
    Sensor_BenchTPS1.specMin = 1;
    Sensor_BenchTPS1.specMax = 5001;

    //Brake Position Sensors
	Sensor_BPS0.specMin = 1;
	Sensor_BPS0.specMax = 10000;
	//Sensor_BPS0.specMin = 0.5;
	//Sensor_BPS0.specMax = 4.5;
	//Sensor_BPS1.specMin = 0.5;
    //Sensor_BPS1.specMax = 4.5;

    /*
    //Wheel Speed Sensors (like an ABS sensor)
    Sensor_WSS_FL;
    Sensor_WSS_FR;
    Sensor_WSS_RL;
    Sensor_WSS_RR;

    //Wheel Position Sensors (Shock pots)
    Sensor_WPS_FL;
    Sensor_WPS_FR;
    Sensor_WPS_RL;
    Sensor_WPS_RR;

    //Steering position Sensor (SPS) - continuous rotation sensor, works like TPS, probably ratiometric
    Sensor_SAS;

    //Switches
    //precharge failure

    //Other
    Sensor_LVBattery;// = { 0xA };  //Note: There will be no init for this "sensor"
    */
}

/*****************************************************************************
* MCU
****************************************************************************/
#include "motorController.h"
/*
MotorController MCU0;
//Initialize the motor controllers (assign the can message IDs)
void vcu_initializeMCU(void)
{
    MCU0.canMessageBaseId = 0xA0;
    //Dummy timestamp for last MCU message
    IO_RTC_StartTime(&MCU0.commands.timeStamp_lastCommandSent);

    MCU0.lockoutStatus = UNKNOWN;
    MCU0.inverterStatus = UNKNOWN;
    MCU0.startRTDS = FALSE;
}
*/