

//////////////////////////////////////////////////////////////////////////////
// Transmitter specific code
void setup_transmitter();
void loop_transmitter();




//////////////////////////////////////////////////////////////////////////////
// Receiver specific code
void setup_receiver();
void loop_receiver();

void servo_set_position_from_adc(uint8_t adcValue);

//////////////////////////////////////////////////////////////////////////////
// Common code





#define PIN_ADC_BATTERY_MEASUREMENT 0 //<<< custom setting default

#define PIN_ENABLE_POWER 0 //<<< custom setting
#define SUPPLY_SWITCH_ENABLE   digitalWrite(PIN_ENABLE_POWER, LOW)
#define SUPPLY_SWITCH_DISABLE  digitalWrite(PIN_ENABLE_POWER, HIGH)

#define PIN_LED_STATUS 10 //<<< custom setting default

#define LOOP_SLEEP_TIME_MS 10
#define SLEEP_TIME_MS 200
#define LOOP_MS_TO_COUNT(x) (x/LOOP_SLEEP_TIME_MS)

bool battery_voltage_ok();
void system_shutdown_transmitter();
void system_check_transmitter();
bool check_battery_voltage(int adc_pin, int min_usable_voltage_mv);
int CalculateVoltage(int adcValue);
void GO_TO_SLEEP(bool enableWakeup); //go to sleep to save battery

bool Nrf_TransmitData(struct RC_COMMAND* pPacket);
bool rc_send_command_type(enum RC_COMMAND_TYPE command_type);

void printStruct(const uint8_t* pData, uint8_t len);
void clearStruct( uint8_t* pData, uint8_t len);

void servo1_set_position_from_adc(uint8_t adcValue);
bool rc_handle_received_data(struct RC_COMMAND* p_command);

// Battery Voltage Measurement
#define BATTERY_CELL_COUNT  3   //<<< custom setting
#define BATTERY_CELL_MIN_MV 800 // 800mV min feasable battery voltage for NiMH rechargeable
#define BATTERY_MIN_MV (BATTERY_CELL_MIN_MV*BATTERY_CELL_COUNT)
bool check_battery_voltage(int adc_pin);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void system_check_transmitter()
{
  //check battery voltage every x-seconds
  if(! battery_voltage_ok())
  {
    rc_send_command_type(COMMAND_TYPE_SHUTDOWN_BATTERY_EMPTY); //send out last message
    
    system_shutdown_transmitter(); //go to sleep forever, wakeup only by mcu reset
  }

  bool user_turn_off = false;
  //TODO check on/off switch or button
  if(user_turn_off)
  {
    rc_send_command_type(COMMAND_TYPE_SHUTDOWN_USER); //send out last message
    system_shutdown_transmitter(); //go to sleep forever, wakeup only by mcu reset
  }
}

//////////////////////////////////////////////////////////////////////////////
void system_shutdown_transmitter()
{
  //gpio de-init, set state with lowest power consumption
  //pinMode(PIN_ENABLE_POWER, INPUT_PULLUP);

  GO_TO_SLEEP(false); //disable everything that draws current, and go to sleep without option to wake up
  
}

void system_init_transmitter()
{
  //gpio init  
  for(int buttonIndex = 0; buttonIndex<HMI_BUTTON_COUNT; buttonIndex++)
  {
    pinMode(pin_list_buttons[buttonIndex], INPUT_PULLUP);    
  }
  
  pinMode(PIN_LED_STATUS, OUTPUT); 
  digitalWrite(PIN_LED_STATUS, HIGH); //Status LED ON

  //enable power for extern circuits (radio module, buttons, potentiometers, ...)
  pinMode(PIN_ENABLE_POWER, OUTPUT); 
  SUPPLY_SWITCH_ENABLE;
}

void system_init_receiver()
{
  pinMode(PIN_LED_STATUS, OUTPUT); 
  digitalWrite(PIN_LED_STATUS, HIGH); //Status LED ON
}
//////////////////////////////////////////////////////////////////////////////
bool battery_voltage_ok()
{
  #if (USE_BATTERY_MONITOR)
    return check_battery_voltage(PIN_ADC_BATTERY_MEASUREMENT, BATTERY_MIN_MV);
  #else
    return true;
  #endif
}

//////////////////////////////////////////////////////////////////////////////
bool check_battery_voltage(int adc_pin, int min_usable_voltage_mv)
{
  bool battery_voltage_ok = false;
  int adc_sum = 0;
  int adc_val = 0;

  // use intern 1.1V ADC reference voltage
  analogReference(INTERNAL);
  //read adc
  for(int i=0;i<10;i++)
  {
    adc_sum += analogRead(adc_pin);
  }
  adc_val = adc_sum/10;

  int measured_battery_voltage_mv = CalculateVoltage(adc_val);

  if(measured_battery_voltage_mv > min_usable_voltage_mv)
  {
    battery_voltage_ok = true;
  }
  
  return battery_voltage_ok;
}

/************************************************************************************************************************************************/
/* ADC -> Voltage calculation
/************************************************************************************************************************************************/
/* Calculate Battery Voltage in mV from ADC raw data

   Vref: 1.1V (intern)
   R_up:    10k
   R_down:  1k

   Vbat 0...10V (normal operation range 4...6V)
   Vpin 0...1V

   21.12.2020
   Calibration point #1: 6000 mV = 507 ADC (measured)
   Calibration point #2: 4000 mV = 332 ADC (measured)

   slope = (V_CAL_#1 - V_CAL_#2) / (ADC_CAL_#1 - ADC_CAL_#2)
   offset = (slope * ADC_CAL_#1) - V_CAL_#1
*/
int CalculateVoltage(int adcValue)
{
  int voltage_mV;

  // set slope & offset of the ADC transfer function
  float slope = (6000.0 - 4000.0) / (507.0 - 332.0); //10bit ADC, 10V range //2000/(175) = 11,42857
  float offset = -205.714;

  voltage_mV = slope * adcValue - offset;

#if DEBUG
/*
  //use adc output for calibration
  Serial.print("ADC: ");
  Serial.print(adcValue);
  Serial.print(" Vbat: ");
  Serial.print(voltage_mV);
  Serial.println();
  */  
#endif

  return voltage_mV;
}


/************************************************************************************************************************************************/
/** Set MCU to sleep mode

    @param bool enableWakeup - (true): use timer to wake up (false): no option to wake up mcu, only by reset
    @return /
*/
/************************************************************************************************************************************************/
void GO_TO_SLEEP(bool enableWakeup)
{
  #if DEBUG
    Serial.write(" go to sleep ... ");

    if(!enableWakeup)
    {
      Serial.write(" ...forever... ");
    }

    Serial.end();
  #endif 
  
  if (enableWakeup) // normal sleep - use RTC timer to wake up MCU
  {    
    //TODO: attachInterrupt
    //https://www.makerguides.com/how-do-i-wake-up-my-arduino-from-sleep-mode/

    //TODO: use stupid delay, until wakeup is done via timer interrupt
    delay(SLEEP_TIME_MS); // important delay!
  }
  else // sleep because battery is empty - no option to wake up again
  {
    SUPPLY_SWITCH_DISABLE;  

    sleep_enable();
    delay(10); // important delay!
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); //set full sleep mode
    sleep_cpu(); // got to sleep ...
  }



}

//////////////////////////////////////////////////////////////////////////////
bool Nrf_TransmitData(struct RC_COMMAND* pPacket)
{
//#if (DEBUG_REPLACE_RADIO_BY_SERIAL)
    Serial.println();
    Serial.print("TX payload: ");
    
    printStruct(((uint8_t*)pPacket),  sizeof(*pPacket));
    //return true;
//#else
  radio.stopListening(); 
  bool tx_success = radio.write(pPacket, sizeof(*pPacket) ); //auto ack will return true if packet was sent and received
  radio.startListening(); 

  if(tx_success)
  {
    Serial.print(" TX success ");
  }
  else
  {
    Serial.print(" TX no success! ");
  }
  
  return tx_success;
//#endif
}

bool rc_send_command_type(enum RC_COMMAND_TYPE command_type)
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
  else
  {
    //clear struct
    //clearStruct(p_command->hmi_data, sizeof(p_command->hmi_data));
    //clearStruct(((uint8_t*)p_command->hmi_data),  sizeof(*p_command->hmi_data));
    //printStruct(((uint8_t*)pPacket),  sizeof(*pPacket));
  }

  p_command->checksum = rc_calculateSum(((uint8_t*)p_command),  sizeof(*p_command) - 1);

  //send
  bool tx_success = Nrf_TransmitData(p_command);

  //rc_handle_received_data(p_command); //only for debug

  return tx_success;

}


//debug: serial output of a struct
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
//debug
void clearStruct( uint8_t* pData, uint8_t len)
{
    const uint8_t* pEnd = pData + len;
    while (pData != pEnd)
    {
        *pData = 0;     
        pData++;
    }
}


//TODO
bool rc_handle_received_data(struct RC_COMMAND* p_command)
{
  //check
  if(rc_check_crc(p_command))
  {
    Serial.print(" CRC OK"); 
    //handle type and data
    if(p_command->command_type == COMMAND_TYPE_DATA_HMI)
    {
      //Servo  
      Serial.print(" handle hmi data"); 
      servo1_set_position_from_adc(p_command->hmi_data.analog_values[0]);

      //DC Motor
      //TODO
    }
    //else if ...

    return true;
  }
  else
  {
    Serial.print(" CRC NOT OK"); 
    return false;
  }
  
}

// Use analog value of potentiometer (0...255) to set servo position
void servo1_set_position_from_adc(uint8_t adcValue)
{
      int newServoPosition = map(adcValue,0,255,0,180);// scale it to use it with the servo (value between 0 and 180)
      
      //apply range for driving straight ahead
      int midPosition = 180/2;
      int distanceToMid = abs(newServoPosition-midPosition);
      if(distanceToMid < 5)
      {
        newServoPosition = midPosition;
      }
      else
      {
        myservo1.write(newServoPosition);
        delay(10); //TODO test: give servo some time
      }
}
