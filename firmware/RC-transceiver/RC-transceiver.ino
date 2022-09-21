//////////////////////////////////////////////////////////////////////////////
///
/// \file       RC-transceiver
/// \brief      Remote Control Transmitter and Receiver
/// \author     Julian Schindler
/// \date       21.09.2022
/// \version    1.0
/// \par        Editor
///             Arduino IDE
/// \par        Board
///             Arduino Nano
/// \par        Prozessor
///             ATMega32
/// \par        COM Port Setting (debug)
///             Baud: 115200 / 8N1
///
/// \par        Organization
///             redlabs.de
/// \par        Repository
///             https://github.com/Julian-Schindler/RC-Transceiver
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#define TARGET_TRANSMITTER  1 //Remote Control
#define TARGET_RECEIVER     2 //Car

#define USED_TARGET TARGET_TRANSMITTER    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< select mode to download to your arduino

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
#include <SPI.h>
#include "RF24.h"

#include "rc_hmi.h" //user interface (buttons, ...)
#include "rc_message_types.h"
#include "rc-transceiver.h"

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(PIN_NRF_CE,PIN_NRF_CSN);

//////////////////////////////////////////////////////////////////////////////
void setup() 
{
  Serial.begin(115200);
  Serial.println("Redlabs - RC Tranceiver Project");


  //rx/tx specific setup
  #if(USED_TARGET == TARGET_TRANSMITTER)
    Serial.println("Start transmitter mode");
    setup_transmitter();
  #else if( USED_TARGET == TARGET_RECEIVER)
    Serial.println("Start receiver mode");
    setup_receiver();
  #endif

  //common setup
  radio.begin(); //use default settings
  radio.startListening();

  //rx/tx specific setup
  #if(USED_TARGET == TARGET_TRANSMITTER)
    //send out radio packet COMMAND_TYPE_STARTUP
  #else if( USED_TARGET == TARGET_RECEIVER)

  #endif
  
}

void setup_transmitter()
{
  //check battery voltage
  if(check_battery_voltage(PIN_ADC_BATTERY_MEASUREMENT, BATTERY_MIN_MV))
  {
    //procceed if it is in range ...
    system_init_transmitter();
  }
  else
  {  
    //go to sleep forever, start again only by (power-)reset
    system_shutdown_transmitter();
  }
}
void setup_receiver()
{
  
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{
  // put your main code here, to run repeatedly:
  #if(USED_TARGET == TARGET_TRANSMITTER)
    loop_transmitter();
  #else if( USED_TARGET == TARGET_RECEIVER)
    loop_receiver();
  #endif
}

//////////////////////////////////////////////////////////////////////////////
void loop_transmitter()
{
  //check battery and powerbutton state
  system_check_transmitter();
  
  //check if HMI data changed
  if(hmi_has_changed())
  {
    //send out last_hmi_data via radio
  }

  go_to_sleep_ms(SLEEP_TIME_MS);
}
//////////////////////////////////////////////////////////////////////////////
void loop_receiver()
{
  if( radio.available())
  {
    
  }

  //go_to_sleep_ms(SLEEP_TIME_MS);
}
