#ifndef _TEMPERATURESENSOR_H
#define _TEMPERATURESENSOR_H
/**********************************************************************//**
 *
 * \author         Charan Sykam
 * \reference    file:///C:/Users/saisy/OneDrive/Documents/SJSU/SRE/SRE%20SDK%202017-18/VCU/Documentation/User%20Manuals/HY-TTC_50_Family_User_Manual.pdf
 * \date         11-7-2017
 **************************************************************************/
/**********************************************************************//**
 * \file temperatureSenor.h
 *
 * \brief Temperature Sensor is able detect high and low temperature reading
          from different parts of the car.
 *
          Temperature Rating has to be between -40*C to +120*C in order to be
          safe state.

 *        The application has to keep track of the on board temparatures of
          of the HY-TTC 90. The on-board temperature exceeds 120*C, the safe
          state shall be activate by the application.
 *
 **************************************************************************/

/**************************************************************************
 *
 * I N C L U D E S
 *
 **************************************************************************/

#include "IO_Driver.h"    /**< Initializing drivers for laptop to VCU communication */
#include "IO_CAN.h"      /**< Output is CAN signal */
#include "cooling.h"    /**< */
#include "bms.h"
#include "serial.h"
#include "" // Need to find battery management system

/**************************************************************************
 *
 * D E F I N I T I O N S
 *
 **************************************************************************/

/**
 * \name Pull up / down settings
 * \anchor dio_pupd_settings
 *
 * Selects a pull-up or pull-down configuration for a digital input
 */
/*@{*/

#define minTemp minTemp /*minimum temperature -40C*/
#define maxTemp maxTemp /*maximum temperature 120C*/
#define ambientTempLow ambientTempLow /*minimun temperature -40C*/
#define ambientTempHigh ambientTempHigh /*maximun temperature 85C*/
#define HY_TTC50TempRange HY_TTC50TempRange /* proper operation of the HY-TTC 50 in its temperature range is -40C to 85C*/
#define batteryTemp batteryTemp /*Temperature of the battery*/
#define brakeTemp brakeTemp /*Temperature of the brake*/

/*@}*/

/**************************************************************************
 *
 * F U N C T I O N S
 *
 **************************************************************************/
/**********************************************************************//**
 *
 * \brief
 *
 * \param di_channel                Analog input:
 *                                      -
 * \param mode                      Input configuration:
 *                                      -
 *
 * \return ubyte4:
 * \retval
 *
 ***************************************************************************/
void calibrateBatteryTemp();
/*-------------------------------------------------------------------
* GetBatteryTemp
* Description: Reads the appropiate pins and gets the temperature of the battery.
* Parameters:  None
* Inputs:      Assumes TPS#.sensorValue has been set by main loop
* Returns:     battery temperature value
* Notes:       valid temperature is -40C to 120C (Need to verify!!!!!)
* Throws:      NOT SURE!!!! NEED TO DO SOME RESEARCH
-------------------------------------------------------------------*/
void calibrateBrakeTemp();
/*-------------------------------------------------------------------
* GetBrakeTemp
* Description: Reads the appropiate pins and gets the temperature of the brake
* Parameters:  None
* Inputs:      Assumes TPS#.sensorValue has been set by main loop
* Returns:     brake temperature value
* Notes:       Valid temperature is from -40C to 120C (Need to verify!!!!!)
* Throws:      NOT SURE!!!! NEED TO DO SOME RESEARCH
-------------------------------------------------------------------*/
void messageToCANManager(calibrateBrakeTemp, calibrateBatteryTemp, minTemp, maxTemp);
/*-------------------------------------------------------------------
* MessageToCANManager
* Description: Sends a message to the CAN manager depending on the temperatures
* Parameters:  calibrateBatteryTemp, calibrateBrakeTemp, minTemp, and maxTemp
* Inputs:      Takes no inputs
* Returns:     returns string with the appropiate message
* Notes:       The message should contain the part of the car that is heating up or abnormally high temp
* Throws:      Throws nothing
-------------------------------------------------------------------*/
void errorInTemperatureValue();
/*-------------------------------------------------------------------
* ErrorInTemperatureValue
* Description: This method handles the case when the temp values that are returned by the car are not valid inputs
* Parameters:  None
* Inputs:      None
* Returns:     String stating the problem of the specific sensor
* Notes:       Make sure to check all the input values
* Throws:      None
-------------------------------------------------------------------*/

#endif // _TEMPERATURESENSOR_H
