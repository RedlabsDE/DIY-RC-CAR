
#define HMI_BUTTON_COUNT  5
#define HMI_ANALOG_8BIT_COUNT 2

enum BUTTON_STATE
{
  BS_INVALID = -1,
  BS_NOT_PRESSED = 0,
  BS_PRESSED,
  BS_REPEATED,
  BS_LONG_PRESS,
};

//example short press: ..., BS_NOT_PRESSED, BS_PRESSED, BS_REPEATED, ... , BS_REPEATED, BS_NOT_PRESSED, ...
//example  long press: ..., BS_NOT_PRESSED, BS_PRESSED, BS_REPEATED, ... , BS_REPEATED, BS_LONG_PRESS, ..., BS_LONG_PRESS, BS_NOT_PRESSED, ...

struct RC_HMI_DATA
{
    enum BUTTON_STATE button_state[HMI_BUTTON_COUNT];
    uint8_t analog_values[HMI_ANALOG_8BIT_COUNT];  
}__attribute__ ((packed, aligned(1)));


//read and return all hmi states (buttons, potis, ...)
struct RC_HMI_DATA hmi_read_current_data()
{
  
}

//return last hmi state. This is updated if hmi is changed by user
struct RC_HMI_DATA hmi_get_last_data()
{
  
}

//global last_hmi_data
bool hmi_has_changed()
{
  bool hmi_changed = false;
  
  /*
  current_hmi_data = hmi_read_current_data()
  
  if(last_hmi_data != current_hmi_data)
  {
    hmi_changed = true;
    last_hmi_data = current_hmi_data
  }
  */

  return hmi_changed;
}
