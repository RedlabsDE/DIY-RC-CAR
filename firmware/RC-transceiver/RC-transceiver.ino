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
//defines to customize the system
#define DEBUG_REPLACE_RADIO_BY_SERIAL true
#define USE_BATTERY_MONITOR false
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

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
    init_hmi_data();
    setup_transmitter();
  #else if( USED_TARGET == TARGET_RECEIVER)
    Serial.println("Start receiver mode");
    setup_receiver();
  #endif

#if (DEBUG_REPLACE_RADIO_BY_SERIAL == false)
  //common setup
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  //radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0, 15);                // Smallest time between retries, max no. of retries
  radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed
  //radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
  //radio.openReadingPipe(1, pipes[0]);
  radio.startListening();                 // Start listening
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
#endif

  //rx/tx specific setup
  #if(USED_TARGET == TARGET_TRANSMITTER)
    //rc_send_command_type(COMMAND_TYPE_STARTUP);
  #else if( USED_TARGET == TARGET_RECEIVER)

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
  //system_check_transmitter();
  
  //check if HMI data changed
  if(hmi_has_changed())
  {
    //send out last_hmi_data via radio
    rc_send_command_type(COMMAND_TYPE_DATA_HMI);
  }
  

  digitalWrite(PIN_LED_STATUS, !digitalRead(PIN_LED_STATUS)); //toggle yellow LED while Alarm is present
  
  go_to_sleep_ms(SLEEP_TIME_MS);
}
//////////////////////////////////////////////////////////////////////////////
void loop_receiver()
{
  if( radio.available())
  {
    struct RC_COMMAND rc_command_received;
    struct RC_COMMAND* p_command = &rc_command_received;
    radio.read(p_command, sizeof(p_command));
    rc_handle_received_data(p_command);   
  }

  //go_to_sleep_ms(SLEEP_TIME_MS);
}




void rc_send_command_type(enum RC_COMMAND_TYPE command_type)
{
  //increment by 1, for each sent packet
  static uint8_t packetNumber = 0;
  
  //create
  struct RC_COMMAND rc_command_to_send;
  struct RC_COMMAND* p_command = &rc_command_to_send;

  //prepare
  p_command->command_identifier[0] = 'R';
  p_command->command_identifier[1] = 'C';
  p_command->command_identifier[2] = 'C';

  p_command->protocol_version = 0;
  p_command->packet_number = packetNumber;
  packetNumber++;
  
  p_command->command_type = command_type;

  if(command_type == COMMAND_TYPE_DATA_HMI) //fill data, if command type matches and data is available
  {
    struct RC_HMI_DATA* p_global_last_hmi_data = hmi_get_last_data();
    memcpy(&p_command->hmi_data, p_global_last_hmi_data, sizeof(p_command->hmi_data));   
  }

  p_command->checksum = rc_calculateSum(((uint8_t*)p_command),  sizeof(*p_command) - 1);

  //send
  Nrf_TransmitData(p_command);

}

//TODO
bool rc_handle_received_data(struct RC_COMMAND* p_command)
{
  //create
  //struct RC_COMMAND rc_command_received;
  //struct RC_COMMAND* p_command = rc_command_received;

  //read
  //radio.read(p_command, sizeof(p_command));

  //check
  if(rc_check_crc(p_command))
  {
    //ok
    //handle type and data
    if(p_command->command_type == COMMAND_TYPE_DATA_HMI) //fill data
    {
      //handle new hmi data from RC      
    }
    //else if ...

    return true;
  }
  else
  {
    return false;
  }
  
}

//////////////////////////////////////////////////////////////////////////////
bool Nrf_TransmitData(struct RC_COMMAND* pPacket)
{
#if (DEBUG_REPLACE_RADIO_BY_SERIAL)
    Serial.println();
    Serial.print("TX payload: ");
    
    printStruct(((uint8_t*)pPacket),  sizeof(*pPacket));
    return true;
#else
  radio.stopListening(); 
  bool tx_success = radio.write(pPacket, sizeof(*pPacket) ); //auto ack will return true if packet was sent and received
  radio.startListening(); 
  
  return tx_success;
#endif
}

void printStruct(const uint8_t* pData, uint8_t len)
{
    //Serial.println();
    Serial.print(" Struct (");
    Serial.print(len);
    Serial.print("): ");
    
    
    uint8_t res = 0;
    const uint8_t* pEnd = pData + len;
    while (pData != pEnd)
    {
        res = *pData;
        Serial.print(res);
        Serial.print(" ");        
        pData++;
    }
}
