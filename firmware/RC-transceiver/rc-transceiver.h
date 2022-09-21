

//////////////////////////////////////////////////////////////////////////////
// Transmitter specific code
void setup_transmitter();
void loop_transmitter();

#define PIN_BUTTON_1 0 //<<< custom setting
#define PIN_BUTTON_2 0 //<<< custom setting
#define PIN_BUTTON_3 0 //<<< custom setting
#define PIN_BUTTON_4 0 //<<< custom setting
#define PIN_BUTTON_5 0 //<<< custom setting

#define PIN_ADC_POTI_1 0 //<<< custom setting
#define PIN_ADC_POTI_2 0 //<<< custom setting

//////////////////////////////////////////////////////////////////////////////
// Receiver specific code
void setup_receiver();
void loop_receiver();


//////////////////////////////////////////////////////////////////////////////
// Common code

#define PIN_NRF_CE  0 //<<< custom setting default
#define PIN_NRF_CSN 0 //<<< custom setting default

#define PIN_ADC_BATTERY_MEASUREMENT 0 //<<< custom setting default

#define PIN_LED_STATUS 0 //<<< custom setting default


#define SLEEP_TIME_MS 10
void go_to_sleep_ms(uint8_t ms_until_wakeup); //go to sleep to save battery


// Battery Voltage Measurement
#define BATTERY_CELL_COUNT  3   //<<< custom setting
#define BATTERY_CELL_MIN_MV 800 // 800mV min feasable battery voltage for NiMH rechargeable
#define BATTERY_MIN_MV (BATTERY_CELL_MIN_MV*BATTERY_CELL_COUNT)
bool check_battery_voltage(int adc_pin);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
void go_to_sleep_ms(uint8_t ms_until_wakeup)
{
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
