

//////////////////////////////////////////////////////////////////////////////
// Transmitter specific code
void setup_transmitter();
void loop_transmitter();




//////////////////////////////////////////////////////////////////////////////
// Receiver specific code
void setup_receiver();
void loop_receiver();


//////////////////////////////////////////////////////////////////////////////
// Common code



#define PIN_NRF_CE  0 //<<< custom setting default
#define PIN_NRF_CSN 0 //<<< custom setting default

#define PIN_ADC_BATTERY_MEASUREMENT 0 //<<< custom setting default

#define PIN_LED_STATUS 13 //<<< custom setting default


#define SLEEP_TIME_MS 10
void go_to_sleep_ms(uint8_t ms_until_wakeup); //go to sleep to save battery
bool battery_voltage_ok();
void system_shutdown_transmitter();
void system_check_transmitter();
bool check_battery_voltage(int adc_pin, int min_usable_voltage_mv);


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
    //send out last message COMMAND_TYPE_SHUTDOWN_BATTERY_EMPTY

    //go to sleep forever
    system_shutdown_transmitter(); 
  }

  //check on/off switch or button
  {
    //send out last message COMMAND_TYPE_SHUTDOWN_USER
  
    //go to sleep
    //system_shutdown_transmitter(); 
  }
  
  
}

//////////////////////////////////////////////////////////////////////////////
void system_shutdown_transmitter()
{
  //gpio de-init, set state with lowest power consumption
  pinMode(PIN_ENABLE_POWER, INPUT_PULLUP);
  
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
  digitalWrite(PIN_ENABLE_POWER, LOW);


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
void go_to_sleep_ms(uint8_t ms_until_wakeup)
{

  //debug: 
  delay(1000);
  
  //config and start wakeup source RTC

  //go to sleep...
  

  //...wakeup

}

//////////////////////////////////////////////////////////////////////////////
bool check_battery_voltage(int adc_pin, int min_usable_voltage_mv)
{
  bool battery_voltage_ok = false;
  int adc_sum = 0;
  int adc_val = 0;

  //read adc
  for(int i=0;i<10;i++)
  {
    adc_sum += analogRead(adc_pin);
  }
  adc_val = adc_sum/10;

  //calc and check voltage
  //ADC reference: internal 1.1V
  //voltage divider (battery)-|10k|-(ADC)-|1k|-(GND)
  int measured_battery_voltage_mv = 0; //TODO 

  if(measured_battery_voltage_mv > min_usable_voltage_mv)
  {
    battery_voltage_ok = true;
  }
  
  return battery_voltage_ok;
}
