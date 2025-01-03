// control.cpp
//
// pull request
#include "control.hpp"

// Sensor data
float Ax, Ay, Az, Wp, Wq, Wr, Mx, My, Mz, Mx0, My0, Mz0, Mx_ave, My_ave, Mz_ave;
float Acc_norm = 0.0;

// Times
float Elapsed_time = 0.0;
float Flight_time = 0.0;
const float End_time = 10.0; 
uint32_t S_time = 0, E_time = 0, D_time = 0, S_time2 = 0, E_time2 = 0, D_time2 = 0;

// Counter
uint8_t AngleControlCounter = 0;
uint16_t RateControlCounter = 0;
uint16_t BiasCounter = 0;
uint16_t LedBlinkCounter = 0;

// Control
float Rudder_duty, Elevator_duty, Aileron_duty, Throttle_duty;
float P_com, Q_com, R_com;
float T_ref;
float Pbias = 0.0, Qbias = 0.0, Rbias = 0.0;
float Phi_bias = 0.0, Theta_bias = 0.0, Psi_bias = 0.0;
float Phi, Theta, Psi;
float Phi_ref = 0.0, Theta_ref = 0.0, Psi_ref = 0.0;
float Elevator_center = 0.0, Aileron_center = 0.0, Rudder_center = 0.0;
float Pref = 0.0, Qref = 0.0, Rref = 0.0;
const float Phi_trim = 0.01;
const float Theta_trim = 0.02;
const float Psi_trim = 0.0;
const float Rate_control_stap_time = 0.0025;
const float Angle_control_stap_time = 0.01;

// Extended Kalman filter
Matrix<float, 7, 1> Xp = MatrixXf::Zero(7, 1);
Matrix<float, 7, 1> Xe = MatrixXf::Zero(7, 1);
Matrix<float, 6, 1> Z = MatrixXf::Zero(6, 1);
Matrix<float, 3, 1> Omega_m = MatrixXf::Zero(3, 1);
Matrix<float, 3, 1> Oomega;
Matrix<float, 7, 7> P;
Matrix<float, 6, 6> Q; // = MatrixXf::Identity(3, 3)*0.1;
Matrix<float, 6, 6> R; // = MatrixXf::Identity(6, 6)*0.0001;
Matrix<float, 7, 6> G;
Matrix<float, 3, 1> Beta;

// Log
uint16_t LogdataCounter = 0;
uint8_t Logflag = 0;
volatile uint8_t Logoutputflag = 0;
float Log_time = 0.0;
const uint8_t DATANUM = 23; // Log Data Number
const uint32_t LOGDATANUM = 48000;
float Logdata[LOGDATANUM] = {0.0};

// State Machine
uint8_t LockMode = 0;
float Disable_duty = 0.10;
float Flight_duty = 0.18; // 0.2/////////////////
uint8_t OverG_flag = 0;
uint8_t Start_G_flag = 0;
uint8_t Last_Start_G_flag = 0;

// PID object and etc.
Filter acc_filter;
WashoutFilter washout_filter(3.0, Rate_control_stap_time);
PID p_pid;
PID q_pid;
PID r_pid;
PID phi_pid;
PID theta_pid;
PID psi_pid;

void loop_400Hz(void);
void direct_control(void);
void rate_control(void);
void sensor_read(void);
void angle_control(void);
void output_data(void);
void output_sensor_raw_data(void);
void kalman_filter(void);
void logging(void);
void motor_stop(void);
uint8_t lock_com(void);
uint8_t logdata_out_com(void);
void printPQR(void);

#define AVERAGE 800
#define KALMANWAIT 3000

// Main loop
// This function is called from PWM Intrupt on 400Hz.
void loop_400Hz(void)
{
  static uint8_t led = 1;
  S_time = time_us_32();

  // 割り込みフラグリセット
  pwm_clear_irq(3);

  // Read Sensor Value
  sensor_read();

  if (Arm_flag == INIT_MODE)
  {
    // motor_stop();
    Elevator_center = 0.0;
    Aileron_center = 0.0;
    Rudder_center = 0.0;
    Pbias = 0.0;
    Qbias = 0.0;
    Rbias = 0.0;
    Phi_bias = 0.0;
    Theta_bias = 0.0;
    Psi_bias = 0.0;
    return;
    //main.cppの中でArm_flagは１(AVERAGE_MODE)にセットされる
  }
  else if (Arm_flag == AVERAGE_MODE)
  {
    motor_stop();
    set_duty_servo(P_LOCK);
    // Gyro Bias Estimate
    if (BiasCounter < AVERAGE)
    {
      // Sensor Read
      //sensor_read();
      Aileron_center += Chdata[3];
      Elevator_center += Chdata[1];
      Rudder_center += Chdata[0];
      Pbias += Wp;
      Qbias += Wq;
      Rbias += Wr;
      Mx_ave += Mx;
      My_ave += My;
      Mz_ave += Mz;
      BiasCounter++;
      return;
    }
    else if (BiasCounter < KALMANWAIT)
    {
      // Sensor Read
      //sensor_read();
      if (BiasCounter == AVERAGE)
      {
        Elevator_center = Elevator_center / AVERAGE;
        Aileron_center = Aileron_center / AVERAGE;
        Rudder_center = Rudder_center / AVERAGE;
        Pbias = Pbias / AVERAGE;
        Qbias = Qbias / AVERAGE;
        Rbias = Rbias / AVERAGE;
        Mx_ave = Mx_ave / AVERAGE;
        My_ave = My_ave / AVERAGE;
        Mz_ave = Mz_ave / AVERAGE;

        Xe(4, 0) = Pbias;
        Xe(5, 0) = Qbias;
        Xe(6, 0) = Rbias;
        Xp(4, 0) = Pbias;
        Xp(5, 0) = Qbias;
        Xp(6, 0) = Rbias;
        MN = Mx_ave;
        ME = My_ave;
        MD = Mz_ave;
      }

      AngleControlCounter++;
      if (AngleControlCounter == 4)
      {
        AngleControlCounter = 0;
        sem_release(&sem);
      }
      Phi_bias += Phi;
      Theta_bias += Theta;
      Psi_bias += Psi;
      BiasCounter++;
      return;
    }
    else
    {
      Arm_flag = PARKING_MODE;
      Phi_bias = Phi_bias / KALMANWAIT;
      Theta_bias = Theta_bias / KALMANWAIT;
      Psi_bias = Psi_bias / KALMANWAIT;
      return;
    }
  }
  else if (Arm_flag == FLIGHT_MODE)
  {
    if (LockMode == 2)
    {
      if (lock_com() == 1)
      {
        LockMode = 3; // Disenable Flight
        led = 0;
        gpio_put(LED_PIN, led);
        return;
      }
      // Goto Flight
    }
    else if (LockMode == 3)
    {
      if (lock_com() == 0)
      {
        LockMode = 0;
        Arm_flag = PARKING_MODE;
      }
      return;
    }

    // LED Blink
    gpio_put(LED_PIN, led);
    if(Logflag == 1 || Start_G_flag == 1)
    {
      if (Logflag == 1 && LedBlinkCounter < 400)
      {
        LedBlinkCounter++;
      }
      else if (Start_G_flag == 1 && LedBlinkCounter < 20)
      {
        LedBlinkCounter++;
      }
      else
      {
        LedBlinkCounter = 0;
        led = !led;
      }
    }
    else
      led = 1;

    //射出の加速度が立ち上がった時時計を０にリセットする
    if ( (Start_G_flag==1) && (Start_G_flag != Last_Start_G_flag) )
    {
      Flight_time = 0.0;
    }

    //射出Gを検知した時からEnd_time（10秒？）たったらパラシュート開傘モード
    if(Start_G_flag == 1)
    {
      if (Flight_time > End_time)
      {
        Start_G_flag = 0;
        Last_Start_G_flag = 0;
        Flight_time = 0.0;
        Arm_flag = PARACHUTE_DEPLOYMENT_MODE;
      }
      Flight_time = Flight_time + 0.0025;
    }

    
    // Rate Control (400Hz)
    rate_control();
    // Direct control
    //direct_control();
    

    if (AngleControlCounter == 4)
    {
      AngleControlCounter = 0;
      // Angle Control (100Hz)
      sem_release(&sem);
    }
    AngleControlCounter++;
  }
  else if (Arm_flag == PARKING_MODE)
  {
    motor_stop();
    OverG_flag = 0;
    Start_G_flag = 0;
    Last_Start_G_flag = 0;
    if (LedBlinkCounter < 10)
    {
      gpio_put(LED_PIN, 1);
      LedBlinkCounter++;
    }
    else if (LedBlinkCounter < 100)
    {
      gpio_put(LED_PIN, 0);
      LedBlinkCounter++;
    }
    else
      LedBlinkCounter = 0;

    // Get Stick Center
    Aileron_center = Chdata[3];
    Elevator_center = Chdata[1];
    Rudder_center = Chdata[0];

    if (LockMode == 0)
    {
      if (lock_com() == 1)
      {
        LockMode = 1;
        return;
      }
      // Wait  output log
    }
    else if (LockMode == 1)
    {
      if (lock_com() == 0)
      {
        LockMode = 2; // Enable Flight
        Arm_flag = FLIGHT_MODE;
      }
      return;
    }

#if 1
    if(logdata_out_com()==1)
    {
      Arm_flag=LOG_MODE;
      return;
    }
#endif
  }
#if 1
  else if(Arm_flag==LOG_MODE)
  {
    motor_stop();
    Logoutputflag=1;
    //LED Blink
    gpio_put(LED_PIN, led);
    if(LedBlinkCounter<400){
      LedBlinkCounter++;
    }
    else
    {
      LedBlinkCounter=0;
      led=!led;
    }
  }
#endif
    else if (Arm_flag = PARACHUTE_DEPLOYMENT_MODE)
    {
      motor_stop();
      set_duty_servo(P_UNLOCK);
      LockMode = 0;
      Arm_flag = PARKING_MODE;
    }

  //Start_G_flagの変化を捉えるために保存
  Last_Start_G_flag = Start_G_flag;

  E_time = time_us_32();
  D_time = E_time - S_time;
}

void control_init(void)
{
  acc_filter.set_parameter(0.005, Rate_control_stap_time);
  // Rate control
  //p_pid.set_parameter(0.1f, 100000.0f, 0.0, 0.125, Rate_control_stap_time); //0.1f, 100000.0f, 0.0
  q_pid.set_parameter(0.1f, 100000.0f, 0.0, 0.125, Rate_control_stap_time); //0.1f, 100000.0f, 0.0
  //r_pid.set_parameter(0.1f, 100000.0f, 0.0, 0.125, Rate_control_stap_time); //0.1f, 100000.0f, 0.0
  // Angle control
  //phi_pid.set_parameter(  1.0f, 100.0f, 0.0f, 0.125, Angle_control_stap_time); // 1.0f, 100.0f, 0.0f
  theta_pid.set_parameter(5.0f, 100000.0f, 0.0f, 0.125, Angle_control_stap_time); // 1.0f, 100.0f, 0.0f
  //psi_pid.set_parameter(0.0f, 100000.0f, 0.0f, 0.125, Angle_control_stap_time);
}

uint8_t lock_com(void)
{
  static uint8_t chatta = 0, state = 0;
  if ((AILERON_CH > (CH_MAX - 100)) && (ELEVATOR_CH > (CH_MAX - 100)) && (THROTTLE_CH < (CH_MIN + 100)) && (RUDDER_CH < (CH_MIN + 100)))
  {
    chatta++;
    if (chatta > 50)
    {
      chatta = 50;
      state = 1;
    }
  }
  else
  {
    chatta = 0;
    state = 0;
  }
  // printf("Arm_flag:%d LockMode:%d chatta:%d CH1:%d CH2:%d CH3:%d CH4:%d CH5:%d\n",
  // Arm_flag, LockMode,chatta,
  // AILERON_CH, ELEVATOR_CH, THROTTLE_CH, RUDDER_CH, Chdata[4]);
  return state;
}

uint8_t logdata_out_com(void)
{
  static uint8_t chatta = 0, state = 0;
  if (Chdata[4] < (CH5MAX + CH5MIN) * 0.5 && Chdata[2] < CH3MIN + 80 && Chdata[0] < CH1MIN + 80 && Chdata[3] > CH4MAX - 80 && Chdata[1] > CH2MAX - 80)
  {
    chatta++;
    if (chatta > 50)
    {
      chatta = 50;
      state = 1;
    }
  }
  else
  {
    chatta = 0;
    state = 0;
  }

  return state;
}



void motor_stop(void)
{
  set_duty_rudder(0.0);
  set_duty_elevator(0.0);
  set_duty_aileron(0.0);
  set_duty_throttle(0.0);
}

void direct_control(void)
{
  float p_rate, q_rate, r_rate;
  float p_ref, q_ref, r_ref;
  float p_err, q_err, r_err;


  // Control angle velocity
  p_rate = Wp - Pbias;
  q_rate = Wq - Qbias;
  r_rate = Wr - Rbias;

  // Get Stick Command
  T_ref =     (float)(THROTTLE_CH - CH3MIN) / (CH_MAX - CH_MIN);
  Phi_ref =   (float)(AILERON_CH - (CH1MAX + CH1MIN) * 0.5) * 2 / (CH1MAX - CH1MIN);
  Theta_ref = (float)(ELEVATOR_CH - (CH2MAX + CH2MIN) * 0.5) * 2 / (CH2MAX - CH2MIN);
  Psi_ref =   (float)(RUDDER_CH - (CH4MAX + CH4MIN) * 0.5) * 2 / (CH4MAX - CH4MIN);

  // Motor Control
  //  1250/11.1=112.6
  //  1/11.1=0.0901
  Rudder_duty   = Psi_ref;//R_com;   // Rudder Yaw
  Elevator_duty = Theta_ref; // Elevator Pitch
  Aileron_duty  = Phi_ref;  // Aileron Roll
  Throttle_duty = T_ref; // Throttle

  const float minimum_duty = -0.95;
  const float maximum_duty = 0.95;
  // minimum_duty = Disable_duty;

  if (Rudder_duty < minimum_duty)
    Rudder_duty = minimum_duty;
  if (Rudder_duty > maximum_duty)
    Rudder_duty = maximum_duty;

  if (Elevator_duty < minimum_duty)
    Elevator_duty = minimum_duty;
  if (Elevator_duty > maximum_duty)
    Elevator_duty = maximum_duty;

  if (Aileron_duty < minimum_duty)
    Aileron_duty = minimum_duty;
  if (Aileron_duty > maximum_duty)
    Aileron_duty = maximum_duty;

  if (Throttle_duty < 0.0)
    Throttle_duty = 0.0;
  if (Throttle_duty > maximum_duty)
    Throttle_duty = maximum_duty;

  // Duty set
  if (0)
  {
    motor_stop();
    p_pid.reset();
    q_pid.reset();
    r_pid.reset();
    Pref = 0.0;
    Qref = 0.0;
    Rref = 0.0;
    Aileron_center = Chdata[3];
    Elevator_center = Chdata[1];
    Rudder_center = Chdata[0];
    Phi_bias = Phi;
    Theta_bias = Theta;
    Psi_bias = Psi;
  }
  else
  {
    if (OverG_flag == 0)
    {
      set_duty_rudder(Rudder_duty);
      set_duty_elevator(Elevator_duty);
      set_duty_aileron(Aileron_duty);
      set_duty_throttle(Throttle_duty);
      set_duty_servo(P_LOCK);
    }
    else
      motor_stop();
      //printf("%12.5f %12.5f %12.5f %12.5f\n",Rudder_duty, Elevator_duty, Aileron_duty, Throttle_duty);
  }

  // printf("\n");

  // printf("%12.5f %12.5f %12.5f %12.5f %12.5f %12.5f %12.5f %12.5f\n",
  //     Elapsed_time, fr_duty, fl_duty, rr_duty, rl_duty, p_rate, q_rate, r_rate);
  // printf("%12.5f %12.5f %12.5f %12.5f %12.5f %12.5f %12.5f\n",
  //     Elapsed_time, p_com, q_com, r_com, p_ref, q_ref, r_ref);
  // printf("%12.5f %12.5f %12.5f %12.5f %12.5f %12.5f %12.5f\n",
  //     Elapsed_time, Phi, Theta, Psi, Phi_bias, Theta_bias, Psi_bias);
  Elapsed_time = Elapsed_time + 0.0025;
  // Logging
  // logging();
}

void rate_control(void)
{
  float p_rate, q_rate, r_rate;
  float r_wash;
  float p_ref, q_ref, r_ref;
  float p_err, q_err, r_err;

  // Read Sensor Value
  //sensor_read();

  // Get Bias
  // Pbias = Xe(4, 0);
  // Qbias = Xe(5, 0);
  // Rbias = Xe(6, 0);

  // Control angle velocity
  p_rate = Wp - Pbias;
  q_rate = Wq - Qbias;
  r_rate = Wr - Rbias;
  r_wash = washout_filter.update(r_rate);

  // Get reference
  p_ref = Pref;
  q_ref = Qref;
  r_ref = Rref;
  T_ref = (float)(THROTTLE_CH - CH3MIN) / (CH_MAX - CH_MIN);

  // Error
  //p_err = p_ref - p_rate;
  q_err = q_ref - q_rate;
  //r_err = r_ref - r_wash;

  // PID
  P_com = p_ref;         //p_pid.update(p_err, Rate_control_stap_time);
  Q_com = q_pid.update(q_err, Rate_control_stap_time);
  R_com = r_ref;        //r_pid.update(r_err, Rate_control_stap_time);

  // Motor Control
  //  1250/11.1=112.6
  //  1/11.1=0.0901

  Aileron_duty  = P_com;
  Elevator_duty = Q_com;
  Rudder_duty   = R_com;
  Throttle_duty = T_ref;

  const float minimum_duty = -0.95;
  const float maximum_duty =  0.95;

  if (Rudder_duty < minimum_duty)
    Rudder_duty = minimum_duty;
  if (Rudder_duty > maximum_duty)
    Rudder_duty = maximum_duty;

  if (Elevator_duty < minimum_duty)
    Elevator_duty = minimum_duty;
  if (Elevator_duty > maximum_duty)
    Elevator_duty = maximum_duty;

  if (Aileron_duty < minimum_duty)
    Aileron_duty = minimum_duty;
  if (Aileron_duty > maximum_duty)
    Aileron_duty = maximum_duty;

  if (Throttle_duty < 0.0)
    Throttle_duty = 0.0;
  if (Throttle_duty > maximum_duty)
    Throttle_duty = maximum_duty;

  // Duty set
  if (0)//T_ref / BATTERY_VOLTAGE < Disable_duty)
  {
    motor_stop();
    p_pid.reset();
    q_pid.reset();
    r_pid.reset();
    Pref = 0.0;
    Qref = 0.0;
    Rref = 0.0;
    Aileron_center = Chdata[3];
    Elevator_center = Chdata[1];
    Rudder_center = Chdata[0];
    Phi_bias = Phi;
    Theta_bias = Theta;
    Psi_bias = Psi;
  }
  else
  {
    if (OverG_flag == 0)
    {
      set_duty_throttle(  Throttle_duty );
      set_duty_aileron(   Aileron_duty  );
      set_duty_elevator( -Elevator_duty );//エレベータサーボが＋の入力で頭下げになるのを反転させるためにマイナスをかけている
      set_duty_rudder(    Rudder_duty   );
    }
    else
      motor_stop();
    // printf("%12.5f %12.5f %12.5f %12.5f\n",Rudder_duty, Elevator_duty, Aileron_duty, Throttle_duty);
  }
  /*
      set_duty_rudder(Rudder_duty);
      set_duty_elevator(Elevator_duty);
      set_duty_aileron(Aileron_duty);
      set_duty_throttle(Throttle_duty);
  */

  // printf("\n");

  // printf("%12.5f %12.5f %12.5f %12.5f %12.5f %12.5f %12.5f %12.5f\n",
  //     Elapsed_time, fr_duty, fl_duty, rr_duty, rl_duty, p_rate, q_rate, r_rate);
  // printf("%12.5f %12.5f %12.5f %12.5f %12.5f %12.5f %12.5f\n",
  //     Elapsed_time, p_com, q_com, r_com, p_ref, q_ref, r_ref);
  // printf("%12.5f %12.5f %12.5f %12.5f %12.5f %12.5f %12.5f\n",
  //     Elapsed_time, Phi, Theta, Psi, Phi_bias, Theta_bias, Psi_bias);
  // Elapsed_time = Elapsed_time + 0.0025;
  // Logging
  // logging();
}

void angle_control(void)
{
  float phi_err, theta_err, psi_err;
  float q0, q1, q2, q3;
  float e23, e33, e13, e11, e12;
  while (1)
  {
    sem_acquire_blocking(&sem);
    sem_reset(&sem, 0);
    S_time2 = time_us_32();
    kalman_filter();
    E_time2 = time_us_32();
    //printf("%12.5f %12.5f[ms]\n", Elapsed_time, (E_time2-S_time2)*1e-3 );//, fr_duty, fl_duty, rr_duty, rl_duty, p_rate, q_rate, r_rate);

    q0 = Xe(0, 0);
    q1 = Xe(1, 0);
    q2 = Xe(2, 0);
    q3 = Xe(3, 0);
    e11 = q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3;
    e12 = 2 * (q1 * q2 + q0 * q3);
    e13 = 2 * (q1 * q3 - q0 * q2);
    e23 = 2 * (q2 * q3 + q0 * q1);
    e33 = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;
    Phi = atan2(e23, e33);
    Theta = atan2(-e13, sqrt(e23 * e23 + e33 * e33));
    Psi = atan2(e12, e11);

    // Get Stick Command(Get angle ref)
    Phi_ref   =        (float)(AILERON_CH  - (CH1MAX + CH1MIN) * 0.5) * 2 / (CH1MAX - CH1MIN);
    //機体の操縦はスティック上で頭上げの設定だが、実際のスティック値はマイナスになるので以下はマイナスをかけている
    Theta_ref = -0.25*((float)(ELEVATOR_CH - (CH2MAX + CH2MIN) * 0.5) * 2 / (CH2MAX - CH2MIN));
    Psi_ref   =        (float)(RUDDER_CH   - (CH4MAX + CH4MIN) * 0.5) * 2 / (CH4MAX - CH4MIN);
    //エレベータサーボはマイナス値で頭上げのキレ角になる

    // Get Stick Command
    //T_ref = (float)(THROTTLE_CH - CH3MIN) / (CH_MAX - CH_MIN);
    //Phi_ref = (float)(AILERON_CH - (CH1MAX + CH1MIN) * 0.5) * 2 / (CH1MAX - CH1MIN);
    //Theta_ref = (float)(ELEVATOR_CH - (CH2MAX + CH2MIN) * 0.5) * 2 / (CH2MAX - CH2MIN);
    //Psi_ref = (float)(RUDDER_CH - (CH4MAX + CH4MIN) * 0.5) * 2 / (CH4MAX - CH4MIN);

    // Error
    //phi_err   = Phi_ref - (Phi - Phi_bias);
    theta_err = Theta_ref - (Theta - Theta_bias);
    //psi_err   = Psi_ref - (Psi - Psi_bias);

    // PID Control
    if (0)//T_ref / BATTERY_VOLTAGE < Flight_duty)
    {
      Pref = 0.0;
      Qref = 0.0;
      Rref = 0.0;
      phi_pid.reset();
      theta_pid.reset();
      psi_pid.reset();
      Aileron_center = Chdata[3];
      Elevator_center = Chdata[1];
      Rudder_center = Chdata[0];
      /////////////////////////////////////
      Phi_bias = Phi;
      Theta_bias = Theta;
      Psi_bias = Psi;
      /////////////////////////////////////
    }
    else
    {
      Pref = Phi_ref;//phi_pid.update(phi_err, Angle_control_stap_time);
      Qref = theta_pid.update(theta_err, Angle_control_stap_time);
      Rref = Psi_ref;//Psi_ref; // psi_pid.update(psi_err);//Yawは角度制御しない
    }

    // Logging
    logging();

    E_time2 = time_us_32();
    D_time2 = E_time2 - S_time2;
  }
}

void logging(void)
{
  // Logging
  if (Chdata[4] > (CH5MAX + CH5MIN) * 0.5)
  {
    if (Logflag == 0)
    {
      Logflag = 1;
      LogdataCounter = 0;
    }
    if (LogdataCounter + DATANUM < LOGDATANUM)
    {
      //Logdata[LogdataCounter++] = Xe(0, 0); // 2
      //Logdata[LogdataCounter++] = Xe(1, 0); // 3
      //Logdata[LogdataCounter++] = Xe(2, 0); // 4
      //Logdata[LogdataCounter++] = Xe(3, 0); // 5
      //Logdata[LogdataCounter++] = Xe(4, 0); // 6
      //Logdata[LogdataCounter++] = Xe(5, 0); // 7
      //Logdata[LogdataCounter++] = Xe(6, 0); // 8
      Logdata[LogdataCounter++] = Wp;       //-Pbias;              //2
      Logdata[LogdataCounter++] = Wq;       //-Qbias;              //3
      Logdata[LogdataCounter++] = Wr;       //-Rbias;              //4
      Logdata[LogdataCounter++] = Ax;        // 5
      Logdata[LogdataCounter++] = Ay;        // 6

      Logdata[LogdataCounter++] = Az;        // 7
      //Logdata[LogdataCounter++] = Mx;             // 
      //Logdata[LogdataCounter++] = My;             // 
      //Logdata[LogdataCounter++] = Mz;             // 
      Logdata[LogdataCounter++] = Pref;      // 8
      Logdata[LogdataCounter++] = Qref;      // 9
      Logdata[LogdataCounter++] = Rref;      // 10      
      Logdata[LogdataCounter++] = Phi;       // 11 //2024/10/04修正
      
      Logdata[LogdataCounter++] = Theta;     // 12　/2024/10/04修正
      Logdata[LogdataCounter++] = Psi;       // 13　/2024/10/04修正
      Logdata[LogdataCounter++] = Phi_ref;   // 14
      Logdata[LogdataCounter++] = Theta_ref; // 15
      Logdata[LogdataCounter++] = Psi_ref;   // 16
      
      Logdata[LogdataCounter++] = P_com;     // 17
      Logdata[LogdataCounter++] = Q_com;     // 18
      Logdata[LogdataCounter++] = R_com;     // 19
      //Logdata[LogdataCounter++] = p_pid.m_integral;   // m_filter_output;    //30

      //Logdata[LogdataCounter++] = q_pid.m_integral;   // m_filter_output;    //31
      //Logdata[LogdataCounter++] = r_pid.m_integral;     // m_filter_output;    //32
      //Logdata[LogdataCounter++] = phi_pid.m_integral;   // m_filter_output;  //33
      //Logdata[LogdataCounter++] = theta_pid.m_integral; // m_filter_output;//34
      Logdata[LogdataCounter++] = Pbias;                // 20
      Logdata[LogdataCounter++] = Qbias;                // 21
      
      Logdata[LogdataCounter++] = Rbias;    // 22
      Logdata[LogdataCounter++] = T_ref;    // 23
      Logdata[LogdataCounter++] = Acc_norm; // 24
    }
    else
      Logflag = 2;
  }
  else
  {
    if (Logflag > 0)
    {
      Logflag = 0;
      LogdataCounter = 0;
    }
  }
}

void log_output(void)
{
  if (LogdataCounter == 0)
  {
    printPQR();
    printf("#Roll rate PID gain\n");
    p_pid.printGain();
    printf("#Pitch rate PID gain\n");
    q_pid.printGain();
    printf("#Yaw rate PID gain\n");
    r_pid.printGain();
    printf("#Roll angle PID gain\n");
    phi_pid.printGain();
    printf("#Pitch angle PID gain\n");
    theta_pid.printGain();
  }
  if (LogdataCounter + DATANUM < LOGDATANUM)
  {
    // LockMode=0;
    printf("%10.2f ", Log_time);
    Log_time = Log_time + 0.01;
    for (uint8_t i = 0; i < DATANUM; i++)
    {
      printf("%12.5f", Logdata[LogdataCounter + i]);
    }
    printf("\n");
    LogdataCounter = LogdataCounter + DATANUM;
  }
  else
  {
    Arm_flag = 3;
    Logoutputflag = 0;
    LockMode = 0;
    Log_time = 0.0;
    LogdataCounter = 0;
  }
}

void gyroCalibration(void)
{
  float wp, wq, wr;
  float sump, sumq, sumr;
  uint16_t N = 400;
  for (uint16_t i = 0; i < N; i++)
  {
    //sensor_read();
    sump = sump + Wp;
    sumq = sumq + Wq;
    sumr = sumr + Wr;
  }
  Pbias = sump / N;
  Qbias = sumq / N;
  Rbias = sumr / N;
}

float Acc_norm_raw;
float Acc_norm_x;
float Rate_norm_raw;

void sensor_read(void)
{
  float mx1, my1, mz1, mag_norm, acc_norm, rate_norm;

  imu_mag_data_read();
  Ax = -acceleration_mg[0] * GRAV * 0.001;
  Ay = -acceleration_mg[1] * GRAV * 0.001;
  Az = acceleration_mg[2] * GRAV * 0.001;

  Wp = angular_rate_mdps[0] * M_PI * 5.55555555e-6; // 5.5.....e-6=1/180/1000
  Wq = angular_rate_mdps[1] * M_PI * 5.55555555e-6;
  Wr = -angular_rate_mdps[2] * M_PI * 5.55555555e-6;
  
  Mx0 = -magnetic_field_mgauss[0];
  My0 = magnetic_field_mgauss[1];
  Mz0 = -magnetic_field_mgauss[2];

  Acc_norm_raw = sqrt(Ax * Ax + Ay * Ay + (Az) * (Az));
  Acc_norm_x = sqrt(Ax*Ax);
  Acc_norm = acc_filter.update(Acc_norm_raw, Rate_control_stap_time);
  Rate_norm_raw = sqrt(Wp * Wp + Wq * Wq + Wr * Wr);

  //動的加速度が2.5Gを超えたら射出されたとみてフラグを立てる  
  if (Acc_norm_x > 130.0) {
    Start_G_flag = 0;
    //OverG_flag++;
  }
  if (Acc_norm >200.0) {
    OverG_flag = 0;
    Start_G_flag = 0;
  }

  /*地磁気校正データ
  回転行列
  [[ 0.65330968  0.75327755 -0.07589064]
   [-0.75666134  0.65302622 -0.03194321]
   [ 0.02549647  0.07829232  0.99660436]]
  中心座標
  122.37559195017053 149.0184454603531 -138.99116060635413
  W
  -2.432054387460946
  拡大係数
  0.003077277151877191 0.0031893151610213463 0.0033832794976645804

  //回転行列
  const float rot[9]={0.65330968, 0.75327755, -0.07589064,
                     -0.75666134, 0.65302622, -0.03194321,
                      0.02549647, 0.07829232,  0.99660436};
  //中心座標
  const float center[3]={122.37559195017053, 149.0184454603531, -138.99116060635413};
  //拡大係数
  const float zoom[3]={0.003077277151877191, 0.0031893151610213463, 0.0033832794976645804};
  */
  // 回転行列
  const float rot[9] = {-0.78435472, -0.62015392, -0.01402787,
                        0.61753358, -0.78277935, 0.07686857,
                        -0.05865107, 0.05162955, 0.99694255};
  // 中心座標
  const float center[3] = {-109.32529343620176, 72.76584808916506, 759.2285249891385};
  // 拡大係数
  const float zoom[3] = {0.002034773458122364, 0.002173892202021849, 0.0021819494099235273};

  // 回転・平行移動・拡大
  mx1 = zoom[0] * (rot[0] * Mx0 + rot[1] * My0 + rot[2] * Mz0 - center[0]);
  my1 = zoom[1] * (rot[3] * Mx0 + rot[4] * My0 + rot[5] * Mz0 - center[1]);
  mz1 = zoom[2] * (rot[6] * Mx0 + rot[7] * My0 + rot[8] * Mz0 - center[2]);
  // 逆回転
  Mx = rot[0] * mx1 + rot[3] * my1 + rot[6] * mz1;
  My = rot[1] * mx1 + rot[4] * my1 + rot[7] * mz1;
  Mz = rot[2] * mx1 + rot[5] * my1 + rot[8] * mz1;

  mag_norm = sqrt(Mx * Mx + My * My + Mz * Mz);
  Mx /= mag_norm;
  My /= mag_norm;
  Mz /= mag_norm;
}

void variable_init(void)
{
  // Variable Initalize
  Xe << 1.00, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
  Xp = Xe;

  Q << 6.0e-5, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 5.0e-5, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 2.8e-5, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 5.0e-5, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 5.0e-5, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 5.0e-5;

  R << 1.701e0, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 2.799e0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 1.056e0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 2.3e-1, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 1.4e-1, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 0.49e-1;

  G << 1.0, 1.0, 1.0, 0.0, 0.0, 0.0,
      -1.0, 1.0, -1.0, 0.0, 0.0, 0.0,
      -1.0, -1.0, 1.0, 0.0, 0.0, 0.0,
      1.0, -1.0, -1.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 1.0;

  G = G * 0.01;

  Beta << 0.0, 0.0, 0.0;

  P << 1e0, 0, 0, 0, 0, 0, 0,
      0, 1e0, 0, 0, 0, 0, 0,
      0, 0, 1e0, 0, 0, 0, 0,
      0, 0, 0, 1e0, 0, 0, 0,
      0, 0, 0, 0, 1e0, 0, 0,
      0, 0, 0, 0, 0, 1e0, 0,
      0, 0, 0, 0, 0, 0, 1e0;
}

void printPQR(void)
{
  volatile int m = 0;
  volatile int n = 0;
  // Print P
  printf("#P\n");
  for (m = 0; m < 7; m++)
  {
    printf("# ");
    for (n = 0; n < 7; n++)
    {
      printf("%12.4e ", P(m, n));
    }
    printf("\n");
  }
  // Print Q
  printf("#Q\n");
  for (m = 0; m < 6; m++)
  {
    printf("# ");
    for (n = 0; n < 6; n++)
    {
      printf("%12.4e ", Q(m, n));
    }
    printf("\n");
  }
  // Print R
  printf("#R\n");
  for (m = 0; m < 6; m++)
  {
    printf("# ");
    for (n = 0; n < 6; n++)
    {
      printf("%12.4e ", R(m, n));
    }
    printf("\n");
  }
}

void output_data(void)
{
  printf("%9.3f,"
         "%13.8f,%13.8f,%13.8f,%13.8f,"
         "%13.8f,%13.8f,%13.8f,"
         "%6lu,%6lu,"
         "%13.8f,%13.8f,%13.8f,"
         "%13.8f,%13.8f,%13.8f,"
         "%13.8f,%13.8f,%13.8f"
         //"%13.8f"
         "\n",
         Elapsed_time // 1
         ,
         Xe(0, 0), Xe(1, 0), Xe(2, 0), Xe(3, 0) // 2~5
         ,
         Xe(4, 0), Xe(5, 0), Xe(6, 0) // 6~8
         //,Phi-Phi_bias, Theta-Theta_bias, Psi-Psi_bias//6~8
         ,
         D_time, D_time2 // 10,11
         ,
         Ax, Ay, Az // 11~13
         ,
         Wp, Wq, Wr // 14~16
         ,
         Mx, My, Mz // 17~19
         //,mag_norm
  ); // 20
}

void output_sensor_raw_data(void)
{
  printf("%9.3f,"
         "%13.5f,%13.5f,%13.5f,"
         "%13.5f,%13.5f,%13.5f,"
         "%13.5f,%13.5f,%13.5f"
         "\n",
         Elapsed_time // 1
         ,
         Ax, Ay, Az // 2~4
         ,
         Wp, Wq, Wr // 5~7
         ,
         Mx, My, Mz // 8~10
  );                // 20
}

void kalman_filter(void)
{
  // Kalman Filter
  float dt = 0.01;
  Omega_m << Wp, Wq, Wr;
  Z << Ax, Ay, Az, Mx, My, Mz;
  ekf(Xp, Xe, P, Z, Omega_m, Q, R, G * dt, Beta, dt);
}
