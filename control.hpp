#ifndef CONTROL_HPP
#define CONTROL_HPP

#include <stdio.h>
#include "main.hpp"
#include "pico/stdlib.h"
#include "sensor.hpp"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "Eigen/Dense"
#include "ekf.hpp"
#include "pid.hpp"
#include <math.h>

#define AILERON_CH Chdata[0]
#define ELEVATOR_CH Chdata[1]
#define THROTTLE_CH Chdata[2]
#define RUDDER_CH Chdata[3]

#define INIT_MODE (0)
#define AVERAGE_MODE (1)
#define FLIGHT_MODE (2)
#define PARKING_MODE (3)
#define LOG_MODE (4)
#define LANDING_MODE (5)
#define PARACHUTE_DEPLOYMENT_MODE (6)

#define P_UNLOCK (0.01f)
#define P_LOCK (0.95f)
//#define M_PI (3.14159265358979323846)

using Eigen::MatrixXd;
using Eigen::MatrixXf;
using Eigen::Matrix;
using Eigen::PartialPivLU;
using namespace Eigen;

#define BATTERY_VOLTAGE (11.1)


//グローバル関数の宣言
void loop_400Hz(void);
void control_init();
void rate_control(void);
void angle_control(void);
void gyro_calibration(void);
void variable_init(void);
void log_output(void);

//グローバル変数
extern uint8_t LockMode;
extern volatile uint8_t Logoutputflag;
extern float FR_duty, FL_duty, RR_duty, RL_duty;
extern float Flight_time;
extern uint8_t Start_G_flag, Last_Start_G_flag, OverG_flag;
extern float Acc_norm, Acc_norm_raw, Acc_norm_x,Rate_norm_raw;


#if 0
class PID
{
  private:
    float m_kp;
    float m_ti;
    float m_td;
    float m_filter_time_constant;
    float m_err,m_err2,m_err3;
    float m_h;
  public:
    float m_filter_output;
    float m_integral;
    PID();
    void set_parameter(
        float kp, 
        float ti, 
        float td,
        float filter_time_constant, 
        float h);
    void reset(void);
    void i_reset(void);
    void printGain(void);
    float filter(float x);
    float update(float err);
};

class Filter
{
  private:
    float m_state;
    float m_T;
    float m_h;
  public:
    float m_out;
    Filter();
    void set_parameter(
        float T,
        float h);
    void reset(void);
    float update(float u);
};
#endif

#endif
