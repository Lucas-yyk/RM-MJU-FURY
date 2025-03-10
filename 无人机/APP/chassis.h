#ifndef __CHASSIS_H
#define __CHASSIS_H

#include "stm32f4xx.h"
#include "sys.h"

#include "chassis.h"

void chassisInit(void);
void chassisTask(void);
void chassisSend(int16_t chassisMotor1, int16_t chassisMotor2, int16_t chassisMotor3, int16_t chassisMotor4);

//M3508 速度环 PID参数以及 PID最大输出，积分输出
#define M3508_PID_KP 10.0f
#define M3508_PID_KI 0.02f
#define M3508_PID_KD 0.0f
#define M3508_PID_MAX_OUT 2500.0f
#define M3508_PID_MAX_IOUT 2000.0f


typedef struct
{
    float kp;
    float ki;
    float kd;

    float set;
    float get;
    float err;

    float max_out;
    float max_iout;

    float Pout;
    float Iout;
    float Dout;

    float out;
} Chassis_PID_t;



#endif
