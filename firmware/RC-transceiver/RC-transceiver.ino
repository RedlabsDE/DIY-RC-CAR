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

#define USED_TARGET TARGET_RECEIVER   //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< select mode to download to your arduino

//////////////////////////////////////////////////////////////////////////////
//defines to customize the system
#define DEBUG true //used for debug serial output
#define DEBUG_REPLACE_RADIO_BY_SERIAL true
#define USE_BATTERY_MONITOR false
//////////////////////////////////////////////////////////////////////////////

enum STATUS_LED
{
  SL_OFF,
  SL_ON,
  SL_BLINK_FAST,
  SL_BLINK_SLOW,
};
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
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };              // Radio pipe addresses for the 2 nodes to communicate.

#include <Servo.h>
Servo myservo1;  //use this servo to set direcetion (angle of front wheel)

#include "rc_dc_motor.h"
struct DC_MOTOR_CONTROL dc_motor_1; //use this motor as main engine to drive forward/backward

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
  radio.openWritingPipe(pipes[0]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1, pipes[0]);
  
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
  dc_motor1_init();
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
    //last_tx_success = rc_send_command_type(COMMAND_TYPE_PING);
    last_tx_timeout = 0;
  }

  indicate_status_led(last_tx_success==true ? SL_BLINK_SLOW : SL_BLINK_FAST); //last data received within timout: blink fast, timeout expired: blink slow

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
    receiver_connection_lost();
  }

  if( radio.available())
  {
    struct RC_COMMAND rc_command_received;
    struct RC_COMMAND* p_command = &rc_command_received;

    radio.read(p_command, sizeof(*p_command));

    Serial.println();
    Serial.print(" RX RC_COMMAND: ");    
    printStruct(((uint8_t*)p_command),  sizeof(*p_command));

    rc_handle_received_data(p_command);  
    last_rx_timeout = 0;
    connected = true;
  }

  dc_motor_control(&dc_motor_1);

  indicate_status_led(connected==true ? SL_BLINK_SLOW : SL_BLINK_FAST); //last data received within timout: blink fast, timeout expired: blink slow

  loopCounter++;
  //GO_TO_SLEEP(true); //sleep some time to save energy
  _delay_ms(10); //dummy
}

void receiver_connection_lost()
{
  //stop motor
  //dc_motor_enable(&dc_motor_1, false);
  dc_motor_set_direction(&dc_motor_1, DCM_STOP);

  //indicate connection loss

}



void indicate_status_led(uint8_t status)
{
  int blink_time = 0;

  switch(status)
  {
    case SL_OFF:
      digitalWrite(PIN_LED_STATUS, LOW);
    break;
    case SL_ON:
      digitalWrite(PIN_LED_STATUS, HIGH);
    break;
    case SL_BLINK_FAST:
      blink_time = LOOP_MS_TO_COUNT(100);
    break;
    case SL_BLINK_SLOW:
      blink_time = LOOP_MS_TO_COUNT(1000);
    break;
    default:
    break;
  }

  if( (blink_time!=0) && (loopCounter % blink_time == 0))
  {
    digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS));
  }  
}


void dc_motor1_init()
{
  //TODO add pin defs
  dc_motor_1.pin_gateP_neg = 0; //digital pin to set direction
  dc_motor_1.pin_gateP_pos = 0; //digital pin to set direction
  dc_motor_1.pin_gateN_neg = 0; //digital pin to set speed via PWM
  dc_motor_1.pin_gateN_pos = 0; //digital pin to set speed via PWM

  dc_motor_init(&dc_motor_1);

  //Set Motor PWM frequency, (default: 490 Hz), //this changes the delay() time!!!!
  setPwmFrequency(dc_motor_1.pin_gateN_pos,1); //set PWM freq to 32kHz (31250/1 Hz)
  setPwmFrequency(dc_motor_1.pin_gateN_neg,1); //set PWM freq to 32kHz (31250/1 Hz)
}


//TODO test
void setPwmFrequency(int pin, int divisor)
{
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10)
  {
    switch(divisor)
    {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6)
    {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else
    {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x07; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}