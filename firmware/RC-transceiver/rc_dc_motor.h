#define MOTOR_HBRIDGE 1


enum DC_MOTOR_DIRECTION
{
  DCM_STOP,
  DCM_FWD,
  DCM_RWD,
};

struct DC_MOTOR_CONTROL
{
  bool enable;
  bool initDone;

  uint8_t speed_current; //actual motor state
  uint8_t speed_destination; 

  enum DC_MOTOR_DIRECTION direction_current; //actual motor state
  enum DC_MOTOR_DIRECTION direction_destination;

  //H-Bridge-Driver
  uint8_t pin_gateP_neg; //digital pin to set direction
  uint8_t pin_gateP_pos; //digital pin to set direction
  uint8_t pin_gateN_neg; //digital pin to set speed via PWM
  uint8_t pin_gateN_pos; //digital pin to set speed via PWM

};

void dc_motor_set_direction(struct DC_MOTOR_CONTROL* dc_motor, enum DC_MOTOR_DIRECTION dir)
{
  dc_motor->direction_destination = dir;
}

void dc_motor_set_speed(struct DC_MOTOR_CONTROL* dc_motor, uint8_t speed)
{
  dc_motor->speed_destination = speed;
}

void dc_motor_enable(struct DC_MOTOR_CONTROL* dc_motor, bool enable)
{
  dc_motor->enable = enable;
}

void dc_motor_init(struct DC_MOTOR_CONTROL* dc_motor)
{
  pinMode(dc_motor->pin_gateP_neg, OUTPUT);
  pinMode(dc_motor->pin_gateP_pos, OUTPUT);
  pinMode(dc_motor->pin_gateN_pos, OUTPUT); //PWM
  pinMode(dc_motor->pin_gateN_neg, OUTPUT); //PWM

  digitalWrite(dc_motor->pin_gateP_neg, LOW);
  digitalWrite(dc_motor->pin_gateP_pos, LOW);
  digitalWrite(dc_motor->pin_gateN_pos, LOW);
  digitalWrite(dc_motor->pin_gateN_neg, LOW);

  dc_motor->initDone = true;
}

//////////////////////////////////////////////////////////////////////////////
//set motor speed / direction
void dc_motor_control(struct DC_MOTOR_CONTROL* dc_motor)
{
 
  if(dc_motor->direction_current != dc_motor->direction_destination) //ramp speed to 0, change direction, ramp speed up
  {
    //change of direction from FWD or RWD
    if((dc_motor->direction_current==DCM_FWD)||(dc_motor->direction_current==DCM_RWD))
    {
      dc_motor->speed_current -= 5; //ramp down speed

      if(dc_motor->speed_current<10) //if speed is low enough to stop, change direction
      {
        dc_motor->direction_current = dc_motor->direction_destination;
        //currentSenseDeactivate = CURRENT_SENSE_DEACTIVATE_TIME;
      }
      #if DEBUG
        Serial.print("x");
      #endif
    }
    else //change from stop to other direction
    {
      dc_motor->direction_current = dc_motor->direction_destination;
    }
  }
  else if(dc_motor->speed_current != dc_motor->speed_destination) //change in motor speed, without change of direction
  {
    if(dc_motor->speed_current > dc_motor->speed_destination+1) //if actual speed is higher than new speed
    {
      dc_motor->speed_current -= 2; //ramp down speed

      #if DEBUG
        Serial.print("<");
      #endif
    }
    else if(dc_motor->speed_current < dc_motor->speed_destination-1) //if actual speed is lower than new speed
    {
      dc_motor->speed_current += 2; //ramp up speed
      #if DEBUG
        Serial.print(">");
      #endif
    }
  }
  else //no change
  {
    dc_motor->speed_current = dc_motor->speed_destination; //just set direction again
  }

    if(dc_motor->enable == false)
    {
        dc_motor->speed_current = 0;
        dc_motor->direction_current = DCM_STOP;
    }
    if(dc_motor->initDone == false)
    {
        return;
    }

  //output speed and direction to motor driver (use one dirction GPIO and one PWM)
  #if MOTOR_L9110
    if(dc_motor->direction_current == DCM_FWD)
    {
      digitalWrite(MOTOR_PIN_DIR, LOW);
      analogWrite(MOTOR_PIN_PWM,dc_motor->speed_current); //PWM output to motor driver
    }
    else if (dc_motor->direction_current == DCM_RWD)
    {
      digitalWrite(MOTOR_PIN_DIR, HIGH);
      analogWrite(MOTOR_PIN_PWM,255-dc_motor->speed_current); //PWM inverse output to motor driver
    }
    else
    {
      digitalWrite(MOTOR_PIN_DIR, LOW);
      analogWrite(MOTOR_PIN_PWM,0); //PWM output to motor driver
    }
  #endif

  //output speed and direction to H-Bridge (use two GPIOs and two PWMs)
  #if MOTOR_HBRIDGE
    if(dc_motor->direction_current == DCM_FWD)
    {
      digitalWrite(dc_motor->pin_gateP_pos, HIGH); //set Motor-Pos to supply voltage
      digitalWrite(dc_motor->pin_gateP_neg, LOW); //disconnect Motor-Neg from supply voltage
      analogWrite(dc_motor->pin_gateN_neg,dc_motor->speed_current); //PWM output to low-side switch on Motor-Neg
      analogWrite(dc_motor->pin_gateN_pos,0); //stop PWM output to low-side switch on Motor-Pos
    }
    else if (dc_motor->direction_current == DCM_RWD)
    {
      digitalWrite(dc_motor->pin_gateP_neg, HIGH); //set Motor-Neg to supply voltage
      digitalWrite(dc_motor->pin_gateP_pos, LOW); //disconnect Motor-Pos from supply voltage
      analogWrite(dc_motor->pin_gateN_pos,dc_motor->speed_current); //PWM output to low-side switch on Motor-Pos
      analogWrite(dc_motor->pin_gateN_neg,0); //stop PWM output to low-side switch on Motor-Neg
    }
    else //motor stopped
    {
      digitalWrite(dc_motor->pin_gateP_pos, HIGH); //set Motor-Pos to supply voltage  <--- used for start/stop detection
      digitalWrite(dc_motor->pin_gateP_neg, LOW); //disconnect Motor-Neg from supply voltage
      analogWrite(dc_motor->pin_gateN_neg,0); //stop PWM output to low-side switch on Motor-Neg
      analogWrite(dc_motor->pin_gateN_pos,0); //stop PWM output to low-side switch on Motor-Pos
    }
  #endif

}