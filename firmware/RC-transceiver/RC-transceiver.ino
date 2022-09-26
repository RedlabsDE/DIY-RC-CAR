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
///             Arduino Pro or Pro Mini
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

#define USED_TARGET TARGET_RECEIVER    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< select mode to download to your arduino

//////////////////////////////////////////////////////////////////////////////
//defines to customize the system
#define DEBUG true //used for debug serial output
#define DEBUG_REPLACE_RADIO_BY_SERIAL true
#define USE_BATTERY_MONITOR false
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////

#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <avr/sleep.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define PIN_NRF_CE  7 //<<< custom setting default
#define PIN_NRF_CSN 8 //<<< custom setting default
/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(PIN_NRF_CE,PIN_NRF_CSN);

#include <Servo.h>
Servo myservo1;  // create servo object to control a servo

#include "rc_hmi.h" //user interface (buttons, ...)
#include "rc_message_types.h"
#include "rc-transceiver.h"



//////////////////////////////////////////////////////////////////////////////
void setup() 
{
  Serial.begin(115200);
  Serial.println("Redlabs - RC Tranceiver Project");


  //rx/tx specific setup
  #if(USED_TARGET == TARGET_TRANSMITTER)
    Serial.println("Start transmitter mode");
    init_hmi_data();
    setup_transmitter();
  #else if( USED_TARGET == TARGET_RECEIVER)
    Serial.println("Start receiver mode");
    setup_receiver();
  #endif

//#if (DEBUG_REPLACE_RADIO_BY_SERIAL == false)
  //common setup
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  //radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0, 15);                // Smallest time between retries, max no. of retries
  radio.setPayloadSize(RC_COMMAND_PAYLOAD_SIZE);                // Here we are sending 1-byte payloads to test the call-response speed
  //radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
  //radio.openReadingPipe(1, pipes[0]);
  
  radio.startListening();                 // Start listening
  //radio.printDetails();                   // Dump the configuration of the rf unit for debugging
  Serial.println(" rx started");
//#endif

  //rx/tx specific setup
  #if(USED_TARGET == TARGET_TRANSMITTER)
    rc_send_command_type(COMMAND_TYPE_STARTUP);
  #else if( USED_TARGET == TARGET_RECEIVER)
    //rc_send_command_type(COMMAND_TYPE_STARTUP);
  #endif
  
}

void setup_transmitter()
{
  //check battery voltage
  if(battery_voltage_ok())
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
  system_init_receiver();

  myservo1.attach(PIN_SERVO_1);  // attaches the servo on pin x to the servo object
  myservo1.write(180/2); //move to mid position  

  //dc motor
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

int loopCounter = 0;
//////////////////////////////////////////////////////////////////////////////
void loop_transmitter()
{  
  static bool last_tx_success = false;
  static int last_tx_timeout = 0;
  last_tx_timeout++;

  //check battery and powerbutton state
  //system_check_transmitter();
  
  //check if HMI data changed
  if(hmi_has_changed())
  {
    //send out last_hmi_data via radio
    last_tx_success = rc_send_command_type(COMMAND_TYPE_DATA_HMI);
    last_tx_timeout = 0;
  }
  else if(last_tx_timeout > LOOP_MS_TO_COUNT(3000))
  {
    last_tx_success = rc_send_command_type(COMMAND_TYPE_PING);
    last_tx_timeout = 0;
  }

  if( loopCounter % (LOOP_MS_TO_COUNT(100) + last_tx_success*LOOP_MS_TO_COUNT(900)) == 0) //successfull transmission with response from receiver: 1000ms, no response from receiver: 100ms
  {
    digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
    loopCounter = 0;
  }

  loopCounter++;  
  //GO_TO_SLEEP(true); //sleep some time to save energy
  _delay_ms(10); //dummy
}
//////////////////////////////////////////////////////////////////////////////




void loop_receiver()
{
  static bool connected = false;
  static int last_rx_timeout = 0;
  
  if(last_rx_timeout<LOOP_MS_TO_COUNT(5000))
  {
    last_rx_timeout++;
  }
  else
  {
    connected = false; //5 seconds without receiving any data
  }

  if( radio.available())
  {
    struct RC_COMMAND rc_command_received;
    struct RC_COMMAND* p_command = &rc_command_received;

    radio.read(p_command, sizeof(p_command));

    Serial.println();
    Serial.print(" RX RC_COMMAND: ");    
    printStruct(((uint8_t*)p_command),  sizeof(*p_command));

    rc_handle_received_data(p_command);  
    last_rx_timeout = 0;
    connected = true;
  }

  
  if(loopCounter % (LOOP_MS_TO_COUNT(100) + connected*LOOP_MS_TO_COUNT(900)) == 0) //successfull transmission with response from receiver: 1000ms, no response from receiver: 100ms
  {
    digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
  }

  loopCounter++;
  //GO_TO_SLEEP(true); //sleep some time to save energy
  _delay_ms(10); //dummy
}

void receiver_connection_lost()
{
  //stop motor

  //indicate connection loss

}


enum DC_MOTOR_DIRECTION
{
  DCM_STOP,
  DCM_FWD,
  DCM_RWD,
};

struct DC_MOTOR_CONTROL
{
  bool enable;

  uint8_t speed_current; //actual motor state
  uint8_t speed_destination; 

  enum DC_MOTOR_DIRECTION direction_current; //actual motor state
  enum DC_MOTOR_DIRECTION direction_destination;
};

struct DC_MOTOR_CONTROL dc_motor_1;

/*
void dc_motor_set_direction(enum DC_MOTOR_DIRECTION dir)
{
  dc_motor_1.direction_destination = dir;
}
*/

void dc_motor_set_speed(uint8_t speed)
{
  dc_motor_1.speed_destination = speed;
}

//stop motor
void dc_motor_stop()
{
  dc_motor_1.enable = false;
}

void dc_motor_start()
{
  dc_motor_1.enable = true;
}

void dc_motor_output()
{
  //update pwm

  //update direction

}

void dc_motor_process()
{
  //check if enable, speed, dir is changed. Fade to destination value
  if(dc_motor_1.enable)
  {
    if(dc_motor_1.direction_current == dc_motor_1.direction_destination)
    {
      if(dc_motor_1.speed_destination < dc_motor_1.speed_current)
      {
        dc_motor_1.speed_current --;
      }
      else  if(dc_motor_1.speed_destination > dc_motor_1.speed_current)
      {
        dc_motor_1.speed_current ++;
      }
    }
    else
    {
      //stop motor
      dc_motor_1.speed_current = 0;
      dc_motor_output();
      delay(100);

      //change direction
      dc_motor_1.direction_current = dc_motor_1.direction_destination;

      //start motor
    }

  }
  else
  {
    dc_motor_stop();
  }

  dc_motor_output();

}
