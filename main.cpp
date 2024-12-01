// comment
#include "main.hpp"
#include "control.hpp"
#include "pid.hpp"

//グローバル変数
uint8_t Arm_flag=0;
semaphore_t sem;

int main(void)
{
  int start_wait=5;
  
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  
  //Initialize stdio for Pico
  stdio_init_all();
  
  //Initialize LSM9DS1
  imu_mag_init();
  
  //Initialize Radio
  radio_init();
  
  //Initialize Variavle
  variable_init();
  
  //Initilize Control
  control_init();
  
  //Initialize PWM
  //Start 400Hz Interval
  ESC_calib=0;
  pwm_init();

  while(start_wait)
  {
    start_wait--;
    printf("#Please wait %d[s]\r",start_wait); 
    sleep_ms(1000);
  }
  printf("\n");
 
  //マルチコア関連の設定
  sem_init(&sem, 0, 1);
  multicore_launch_core1(angle_control);  

  Arm_flag=1;
  uint8_t mode;
  float maxacc = 0.0;
  float maxacc_x = 0.0;
  float maxrate = 0.0;
  while(1) 
  {
    #if 0
    if (Arm_flag != mode) {
      maxacc = 0.0;
      maxrate = 0.0;
      maxacc_x = 0.0;
    }
    mode = Arm_flag;
    if (Acc_norm_raw > maxacc) maxacc = Acc_norm_raw;
    if (Acc_norm_x > maxacc_x) maxacc_x = Acc_norm_x;   
    if (Rate_norm_raw > maxrate) maxrate = Rate_norm_raw;

    if (Arm_flag ==FLIGHT_MODE || Arm_flag ==PARKING_MODE)
      printf("%f %d %d %d %f %f %f\n\r", Flight_time, Start_G_flag, Last_Start_G_flag, OverG_flag, maxacc, maxacc_x, maxrate);
    #endif
    //printf("Arm_flag:%d LockMode:%d\n",Arm_flag, LockMode);
    //printf("%4d %4d %4d %4d %4d\n",Chdata[0],Chdata[1],Chdata[2],Chdata[3],Chdata[4]);
    tight_loop_contents();
    while (Logoutputflag==1){
      log_output();
    }
  }

  return 0;
}
