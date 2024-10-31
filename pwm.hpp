/*
 * pwm.phh 
 * Header file for PWM function
*/
#ifndef PWM_HPP
#define PWM_HPP
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "main.hpp"
#include "control.hpp"

//ESC1 ESC モータ
//ESC2 ラダー
//ESC3 エレベーター
//ESC4 エルロン

//#define DUTYMIN 1250
//#define DUTYMAX 2500
#define DUTYMIN 625
#define DUTYMAX 3000

//グローバル変数
extern uint8_t ESC_calib;

//関数の宣言
void pwm_init();
void set_duty_rudder(float duty);
void set_duty_elevator(float duty);
void set_duty_aileron(float duty);
void set_duty_throttle(float duty);
void set_duty_servo(float duty);
/*
    set_duty_fr(Rudder_duty);
    set_duty_fl(Elevator_duty);
    set_duty_rr(Aileron_duty);
    set_duty_rl(Throttle_duty);
*/
#endif
