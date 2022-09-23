
#define HMI_BUTTON_COUNT  5
#define HMI_ANALOG_8BIT_COUNT 1

#define ADC_HYSTERESIS_COUNTS 5 //a change of ADC measured counts (compared to last saved value) below this value will be dismissed

#define PIN_BUTTON_1 2 //<<< custom setting
#define PIN_BUTTON_2 3 //<<< custom setting
#define PIN_BUTTON_3 4 //<<< custom setting
#define PIN_BUTTON_4 5 //<<< custom setting
#define PIN_BUTTON_5 6 //<<< custom setting

#define PIN_ADC_POTI_1 0 //<<< custom setting
#define PIN_ADC_POTI_2 1 //<<< custom setting



#define PIN_SERVO_1 9


int pin_list_buttons[HMI_BUTTON_COUNT] = {PIN_BUTTON_1,PIN_BUTTON_2,PIN_BUTTON_3,PIN_BUTTON_4,PIN_BUTTON_5};
int pin_list_adc_8bit[HMI_ANALOG_8BIT_COUNT] = {PIN_ADC_POTI_1};//,PIN_ADC_POTI_2};

enum BUTTON_STATE
{
  BS_INVALID = 0,
  BS_NOT_PRESSED,
  BS_PRESSED,
  BS_REPEATED,
  BS_LONG_PRESS,
};

//example short press: ..., BS_NOT_PRESSED, BS_PRESSED, BS_REPEATED, ... , BS_REPEATED, BS_NOT_PRESSED, ...
//example  long press: ..., BS_NOT_PRESSED, BS_PRESSED, BS_REPEATED, ... , BS_REPEATED, BS_LONG_PRESS, ..., BS_LONG_PRESS, BS_NOT_PRESSED, ...

struct RC_HMI_DATA
{
    //enum BUTTON_STATE button_state[HMI_BUTTON_COUNT];
    uint8_t button_state[HMI_BUTTON_COUNT];
    uint8_t analog_values[HMI_ANALOG_8BIT_COUNT];  
}__attribute__ ((packed, aligned(1)));

//global
struct RC_HMI_DATA global_last_hmi_data;


void printStruct2(const uint8_t* pData, uint8_t len)
{
    Serial.println();
    Serial.print(" Struct2 (");
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
struct RC_HMI_DATA* hmi_get_last_data()
{
    return &global_last_hmi_data;
}

void hmi_set_last_data(struct RC_HMI_DATA* new_hmi_data)
{
  memcpy(&global_last_hmi_data, new_hmi_data, sizeof(*new_hmi_data));
}

// Do ADC sampling and mean calculation
int get_averaged_Adc(int adcPin)
{
  int adcVal = 0;
  for (int i = 0; i < 10; i++)
  {
    adcVal += analogRead(adcPin);
  }
  adcVal = adcVal / 10;

  return adcVal;
}

//read and return all hmi states (buttons, potis, ...)
void hmi_read_current_data(struct RC_HMI_DATA* current_hmi_data)
{
  struct RC_HMI_DATA* p_last_hmi_data = hmi_get_last_data();
  
  //Buttons
  for(int buttonIndex = 0; buttonIndex<HMI_BUTTON_COUNT; buttonIndex++)
  {  
    //read pin state
    uint8_t pinState = digitalRead(pin_list_buttons[buttonIndex]);
    //pinState = 1; //debug
    uint8_t newState = BS_INVALID;
    
    if(pinState == 1) //high: no press
    {
      newState = BS_NOT_PRESSED;
    }
    else
    {
      //low: pressed
      switch(p_last_hmi_data->button_state[buttonIndex])
      {
        case BS_INVALID:
        case BS_NOT_PRESSED:
          newState = BS_PRESSED;
          break;
        case BS_PRESSED:
          newState = BS_REPEATED;
          break;
        case BS_REPEATED:
            newState = BS_REPEATED;
          //TODO: add timer/counter to set BS_LONG_PRESS
          break;
        default:
          break;
      }      
    }

    current_hmi_data->button_state[buttonIndex]  = newState; 
  }

  static uint8_t fooCounter = 0;

  //ADC
  // use external ADC reference voltage
  // if (swichted-) supply voltage of potentiometers is connected to AREF, max poti position will result in max adc value. Independent of actual voltage level of supply.
  analogReference(EXTERNAL);
  
  for(int adcIndex = 0; adcIndex<HMI_ANALOG_8BIT_COUNT; adcIndex++)
  { 
    int newAdc = get_averaged_Adc(pin_list_adc_8bit[adcIndex]);
    //apply hystersis    
    int diff = abs(p_last_hmi_data->analog_values[adcIndex] - (uint8_t) map(newAdc,0,1023,0,255));
    if(diff > ADC_HYSTERESIS_COUNTS)
    {
      current_hmi_data->analog_values[adcIndex] = (uint8_t) map(newAdc,0,1023,0,255);
    } 
    else
    {
      current_hmi_data->analog_values[adcIndex] = p_last_hmi_data->analog_values[adcIndex];
    }
  }

  //printStruct2(((uint8_t*)current_hmi_data),  sizeof(*current_hmi_data)); //debug
    
}




//global last_hmi_data
bool hmi_has_changed()
{
  bool hmi_changed = false;

  struct RC_HMI_DATA current_hmi_data;
  hmi_read_current_data(&current_hmi_data);

  struct RC_HMI_DATA* p_last_hmi_data = hmi_get_last_data();

  if(memcmp(p_last_hmi_data, &current_hmi_data ,sizeof(*p_last_hmi_data)) != 0) //not equal
  {
    hmi_changed = true;  
    hmi_set_last_data(&current_hmi_data); 
  }

  return hmi_changed;
}


void init_hmi_data()
{
  struct RC_HMI_DATA current_hmi_data;
  current_hmi_data.button_state[0] = 1;
  
  hmi_set_last_data(&current_hmi_data); 
}
