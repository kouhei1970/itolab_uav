/*
 * pwm.cpp
*/
#include "pwm.hpp"

//グローバル変数
uint8_t ESC_calib=0;

//ファイル内グローバル変数
const uint Slice_num_rear  = 1;
const uint Slice_num_front = 7;
const uint Slice_num_servo = 3;

uint16_t time2warp(uint16_t usec);

void pwm_init()
{
    // PWMの設定
    // Tell GPIO 2-6 they are allocated to the PWM for Motor & Servo Control
    gpio_set_function(2, GPIO_FUNC_PWM);//
    gpio_set_function(3, GPIO_FUNC_PWM);//
    gpio_set_function(14,GPIO_FUNC_PWM);//
    gpio_set_function(15,GPIO_FUNC_PWM);//
    gpio_set_function(6, GPIO_FUNC_PWM);//Servo PWM



    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
    // Set period T
    // T=(wrap+1)*clkdiv/sysclock
    // T=(24999+1)*100/125e6=25000e2/125e6=200e-4=0.02s(=50Hz)
    // T=(3124+1)*100/125e6=3125e2/125e6=0.0025s(400Hz)
    pwm_set_wrap(Slice_num_front, 3124);
    pwm_set_wrap(Slice_num_rear,  3124);
    pwm_set_wrap(Slice_num_servo, 3124);

    pwm_set_clkdiv(Slice_num_front, 100.0);
    pwm_set_clkdiv(Slice_num_rear,  100.0);
    pwm_set_clkdiv(Slice_num_servo, 100.0);

    pwm_clear_irq(Slice_num_servo);
    pwm_set_irq_enabled(Slice_num_servo, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, MAINLOOP);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    if(ESC_calib==1)
    {
      // ESC calibration
      // Set Duty
      // DutyA=clkdiv*PWM_CHAN_A/sysclock
      pwm_set_chan_level(Slice_num_front, PWM_CHAN_A, time2warp(1450));//elevator
      pwm_set_chan_level(Slice_num_rear,  PWM_CHAN_A, time2warp(2000));//throttle
      pwm_set_chan_level(Slice_num_front, PWM_CHAN_B, time2warp(1450));//aileron
      pwm_set_chan_level(Slice_num_rear,  PWM_CHAN_B, time2warp(1450));//rudder
    } 
    else
    {
      pwm_set_chan_level(Slice_num_front, PWM_CHAN_A, time2warp(1450));
      pwm_set_chan_level(Slice_num_rear,  PWM_CHAN_A, time2warp(1000));
      pwm_set_chan_level(Slice_num_front, PWM_CHAN_B, time2warp(1450));
      pwm_set_chan_level(Slice_num_rear,  PWM_CHAN_B, time2warp(1450));
    }

    // Set the PWM running
    //sleep_ms(500);
    pwm_set_enabled(Slice_num_front, true);
    pwm_set_enabled(Slice_num_rear,  true);
    pwm_set_enabled(Slice_num_servo, true);

    sleep_ms(3000);
    
    pwm_set_chan_level(Slice_num_front, PWM_CHAN_A, time2warp(1450));//elevator
    pwm_set_chan_level(Slice_num_rear,  PWM_CHAN_A, time2warp(1000));//throttle
    pwm_set_chan_level(Slice_num_front, PWM_CHAN_B, time2warp(1450));//aileron
    pwm_set_chan_level(Slice_num_rear,  PWM_CHAN_B, time2warp(1450));//rudder
    pwm_set_chan_level(Slice_num_servo, PWM_CHAN_A, time2warp(1450));

    sleep_ms(1000);
}

uint16_t time2warp(uint16_t usec)
{
    return (uint16_t)(usec*125/100 - 1);
}

void set_duty_aileron(float duty)
{
    if (duty >  0.95)duty = 0.95;  
    if (duty < -0.95)duty =-0.95;
    duty = 1187.5*duty + 1813.0;
    if (duty>3000.0)duty=3000.0;
    if (duty<625.0) duty=625.0;
    pwm_set_chan_level(Slice_num_front, PWM_CHAN_B, duty);
    //printf("%4.0f ", duty);
}

void set_duty_elevator(float duty)
{
    if (duty >  0.95)duty = 0.95;  
    if (duty < -0.95)duty =-0.95;
    duty = 1187.5*duty + 1813.0;
    if (duty>3000.0)duty=3000.0;
    if (duty<625.0) duty=625.0;
    pwm_set_chan_level(Slice_num_front, PWM_CHAN_A, duty);
    //printf("%4.0f ", duty);
}

void set_duty_rudder(float duty)
{
    if (duty >  0.95)duty = 0.95;  
    if (duty < -0.95)duty =-0.95;
    duty = 1187.5*duty + 1813.0;
    if (duty>3000.0)duty=3000.0;
    if (duty<625.0) duty=625.0;
    pwm_set_chan_level(Slice_num_rear, PWM_CHAN_B, duty);
    //printf("%4.0f ", duty);
}

void set_duty_throttle(float duty)
{
    uint16_t usec;
    if (duty >  0.95)duty = 0.95;  
    if (duty <  0.0)duty =0.0;
    usec = (uint16_t)(duty*1000.0) + 1000;
    pwm_set_chan_level(Slice_num_rear, PWM_CHAN_A, time2warp(usec));
    //printf("%4.2f\n", duty);
}

void set_duty_servo(float duty)
{
    uint16_t usec;
    if (duty >  0.95)duty = 0.95;  
    if (duty <  0.0)duty =0.0;
    usec = (uint16_t)(duty*1000.0) + 1000;
    pwm_set_chan_level(Slice_num_servo, PWM_CHAN_A, time2warp(usec));
    //printf("%4.2f\n", duty);
}
