#include "dc_motor.h"

DC_MOTOR::DC_MOTOR()
{
    speed_min = 5;
    speed_max = 255;
    speed_increment = 2;

    reset();
}

DC_MOTOR::DC_MOTOR(uint8_t speedMin, uint8_t speedMax, uint8_t speedIncrement)
{
    speed_min = speedMin;
    speed_max = speedMax;
    speed_increment = speedIncrement;

    reset();
}

void DC_MOTOR::reset()
{
    speed_current = 0;
    direction_current = DCM_STOP;

    speed_destination = 0;
    direction_destination = DCM_STOP;

    enable = false;
    initDone = false;
}

//for use of HBridge "ZXMHC6A07T8" use init_h_bridge(G4, G1, G3, G2)
void DC_MOTOR::init_h_bridge(int pinNum_gateP_neg, int pinNum_gateP_pos, int pinNum_gateN_neg, int pinNum_gateN_pos, bool pmos_active_state)
{
    pin_gateP_neg = pinNum_gateP_neg;
    pin_gateP_pos = pinNum_gateP_pos;
    pin_gateN_neg = pinNum_gateN_neg;
    pin_gateN_pos = pinNum_gateN_pos;

    pmos_active = pmos_active_state;
    bool PMOS_INACTIVE = pmos_active;

    pinMode(pin_gateP_neg, OUTPUT);
    pinMode(pin_gateP_pos, OUTPUT);
    pinMode(pin_gateN_pos, OUTPUT); //PWM
    pinMode(pin_gateN_neg, OUTPUT); //PWM

    digitalWrite(pin_gateP_neg, PMOS_INACTIVE);
    digitalWrite(pin_gateP_pos, PMOS_INACTIVE);
    digitalWrite(pin_gateN_pos, LOW);
    digitalWrite(pin_gateN_neg, LOW);

    usedDriverType = DRIVER_HBRIGE;
    initDone = true;
}

//not tested
void DC_MOTOR::init_L9110(int pinNum_dir, int pinNum_pwm)
{
    pin_dir = pinNum_dir;
    pin_pwm = pinNum_pwm;

    pinMode(pin_dir, OUTPUT);
    pinMode(pin_pwm, OUTPUT); //PWM

    digitalWrite(pin_dir, LOW);
    digitalWrite(pin_pwm, LOW);

    usedDriverType = DRIVER_L9110;
    initDone = true;
}

void DC_MOTOR::set_enable(bool motorEnable)
{
    enable = motorEnable;
}

void DC_MOTOR::set_direction(enum DC_MOTOR_DIRECTION dir)
{
  direction_destination = dir;
}

void DC_MOTOR::set_speed(uint8_t speed)
{
  //TODO add minimum speed (absolut min is step size in motor ramp up/down)
  if(speed < speed_min)
  {
    speed = speed_min;
  }
  else if (speed > speed_max)
  {
    speed = speed_max;
  }
  speed_destination = speed;
}

//set motor speed / direction. Return false if motor driver is not initialized.
bool DC_MOTOR::process()
{
    if(direction_current != direction_destination) //ramp speed to 0, change direction, ramp speed up
    {
        //change of direction from FWD or RWD
        if((direction_current==DCM_FWD)||(direction_current==DCM_RWD))
        {
            speed_current -= speed_increment; //ramp down speed
            #if DEBUG
            Serial.print("x");
            #endif

            if(speed_current <= speed_min) //if speed is low enough to stop, change direction
            {
                direction_current = direction_destination;
                #if DEBUG
                Serial.print("c");
                #endif
            }

        }
        else //change from stop to other direction
        {
            direction_current = direction_destination;
        }
    }
    else if(speed_current != speed_destination) //change in motor speed, without change of direction
    {
        if(speed_current > speed_destination) //if actual speed is higher than new speed
        {
            speed_current -= speed_increment; //ramp down speed

            #if DEBUG
            Serial.print("<");
            #endif
        }
        else if(speed_current < speed_destination) //if actual speed is lower than new speed
        {
            speed_current += speed_increment; //ramp up speed
            #if DEBUG
            Serial.print(">");
            #endif
        }
    }
    else //no change
    {
        speed_current = speed_destination; //just set speed again
    }

    //limit speed
    if(speed_current > speed_max)
    {
        speed_current = speed_max;
    }
    else if(speed_current< speed_min)
    {
        speed_current = speed_min;
    }

    if(enable == false)
    {
        speed_current = 0;
        direction_current = DCM_STOP;
        #if DEBUG
        Serial.println("enable=false");
        #endif
    }
    if(initDone == false)
    {
        #if DEBUG
        Serial.println("initDone=false");
        #endif
        return false;
    }

    if(usedDriverType == DRIVER_HBRIGE)
    {
        process_h_bridge();
    }
    else if(usedDriverType == DRIVER_L9110)
    {
        process_L9110();
    }

    return true;
}



//output speed and direction to H-Bridge (use two GPIOs and two PWMs)
void DC_MOTOR::process_h_bridge()
{
    //here, the P-MOSFETs are driven directly by a GPIO (pin low = active). If inverting transistors are used (if V_motor > V_uC), switch following settings.
    bool PMOS_ACTIVE = pmos_active;
    bool PMOS_INACTIVE = !pmos_active;

    if(direction_current == DCM_FWD)
    {
      digitalWrite(pin_gateP_pos, PMOS_ACTIVE); //G1, set Motor-Pos to supply voltage
      digitalWrite(pin_gateP_neg, PMOS_INACTIVE); //G4, disconnect Motor-Neg from supply voltage
      analogWrite(pin_gateN_neg,speed_current); //G3, PWM output to low-side switch on Motor-Neg
      analogWrite(pin_gateN_pos,0); //G2, stop PWM output to low-side switch on Motor-Pos
    }
    else if (direction_current == DCM_RWD)
    {
      digitalWrite(pin_gateP_neg, PMOS_ACTIVE); //G4, set Motor-Neg to supply voltage
      digitalWrite(pin_gateP_pos, PMOS_INACTIVE); //G1, disconnect Motor-Pos from supply voltage
      analogWrite(pin_gateN_pos,speed_current); //G2, PWM output to low-side switch on Motor-Pos
      analogWrite(pin_gateN_neg,0); //G3, stop PWM output to low-side switch on Motor-Neg
    }
    else //motor stopped
    {
      digitalWrite(pin_gateP_pos, PMOS_INACTIVE); //G1, disconnect Motor-Pos from supply voltage
      digitalWrite(pin_gateP_neg, PMOS_INACTIVE); //G4, disconnect Motor-Neg from supply voltage
      analogWrite(pin_gateN_neg,0); //G3, stop PWM output to low-side switch on Motor-Neg
      analogWrite(pin_gateN_pos,0); //G2, stop PWM output to low-side switch on Motor-Pos
    }
}

//not tested
void DC_MOTOR::process_L9110()
{
  //output speed and direction to motor driver (use one dirction GPIO and one for PWM)
    if(direction_current == DCM_FWD)
    {
      digitalWrite(pin_dir, LOW);
      analogWrite(pin_pwm,speed_current); //PWM output to motor driver
    }
    else if (direction_current == DCM_RWD)
    {
      digitalWrite(pin_dir, HIGH);
      analogWrite(pin_pwm,255-speed_current); //PWM inverse output to motor driver
    }
    else
    {
      digitalWrite(pin_dir, LOW);
      analogWrite(pin_pwm,0); //PWM output to motor driver
    }
}