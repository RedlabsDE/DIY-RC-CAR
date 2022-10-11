// dc_motor.h
#ifndef DC_MOTOR_h
#define DC_MOTOR_h

#include <Arduino.h>

class DC_MOTOR{

public:
    enum DC_MOTOR_DIRECTION
    {
        DCM_STOP,
        DCM_FWD,
        DCM_RWD,
    };
    enum DRIVER_TYPE
    {
        DRIVER_HBRIGE,
        DRIVER_L9110,
    };

  private:

    bool enable;
    bool initDone; //gpio init is done
    enum DRIVER_TYPE usedDriverType;

    int speed_current; //actual motor state
    int speed_destination; //speed to be reached
    enum DC_MOTOR_DIRECTION direction_current; //actual motor state
    enum DC_MOTOR_DIRECTION direction_destination; //direction to reach (before a change of direction is done, the speed is ramped down)

//Limits
    uint8_t speed_min;
    uint8_t speed_max;
    uint8_t speed_increment;

//H-Bridge driver
    int pin_gateP_neg; //digital pin to set direction
    int pin_gateP_pos; //digital pin to set direction
    int pin_gateN_neg; //digital pin to set speed via PWM
    int pin_gateN_pos; //digital pin to set speed via PWM

    bool pmos_active; //false/low: pmos is active if GPIO (to drive gate) is low, true/high: pmos is active if GPIO is high

//L9110 driver
    int pin_dir;
    int pin_pwm;

//Driver
    void process_h_bridge();
    void process_L9110();

    void reset();

  public:
    DC_MOTOR();
    DC_MOTOR(uint8_t speedMin, uint8_t speedMax, uint8_t speedIncrement);

    //call this method frequently in your main loop. 
    bool process();

    void init_h_bridge(int pinNum_gateP_neg, int pinNum_gateP_pos, int pinNum_gateN_neg, int pinNum_gateN_pos, bool pmos_active_state);
    void init_L9110(int pinNum_dir, int pinNum_pwm);

    void set_enable(bool enabled);

    void set_direction(enum DC_MOTOR_DIRECTION dir);
    void set_speed(uint8_t speed);

};



#endif