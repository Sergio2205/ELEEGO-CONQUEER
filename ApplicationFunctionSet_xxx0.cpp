/*
 * @Author: ELEGOO
 * @Date: 2019-10-22 11:59:09
 * @LastEditTime: 2020-12-18 15:01:11
 * @LastEditors: Changhua
 * @Description: conqueror robot tank
 * @FilePath: 
 */
//#include <hardwareSerial.h>
#include <stdio.h>
#include <string.h>
#include "ApplicationFunctionSet_xxx0.h"
#include "DeviceDriverSet_xxx0.h"

#include "ArduinoJson-v6.11.1.h" //ArduinoJson
#include "MPU6050_getdata.h"

#define _is_print 1
#define _Test_print 0

ApplicationFunctionSet Application_FunctionSet;

/*硬件设备成员对象序列*/
MPU6050_getdata AppMPU6050getdata;
DeviceDriverSet_RBGLED AppRBG_LED;
DeviceDriverSet_Key AppKey;
DeviceDriverSet_ITR20001 AppITR20001;
DeviceDriverSet_Voltage AppVoltage;

DeviceDriverSet_Motor AppMotor;
DeviceDriverSet_ULTRASONIC AppULTRASONIC;
DeviceDriverSet_Servo AppServo;
DeviceDriverSet_IRrecv AppIRrecv;
/*f(x) int */
static boolean
function_xxx(long x, long s, long e) //f(x)
{
  if (s <= x && x <= e)
    return true;
  else
    return false;
}

/*运动方向控制序列*/
enum ConquerorCarMotionControl
{
  Forward,       //(1)
  Backward,      //(2)
  Left,          //(3)
  Right,         //(4)
  LeftForward,   //(5)
  LeftBackward,  //(6)
  RightForward,  //(7)
  RightBackward, //(8)
  stop_it        //(9)
};               //direction方向:（1）、（2）、 （3）、（4）、（5）、（6）

/*模式控制序列*/
enum ConquerorCarFunctionalModel
{
  Standby_mode,           /*空闲模式*/
  TraceBased_mode,        /*循迹模式*/
  ObstacleAvoidance_mode, /*避障模式*/
  Follow_mode,            /*跟随模式*/
  Rocker_mode,            /*摇杆模式*/
  CMD_inspect,
  CMD_Programming_mode,                   /*编程模式*/
  CMD_ClearAllFunctions_Standby_mode,     /*清除所有功能：进入空闲模式*/
  CMD_ClearAllFunctions_Programming_mode, /*清除所有功能：进入编程模式*/
  CMD_MotorControl,                       /*电机控制模式*/
  CMD_CarControl_TimeLimit,               /*小车方向控制：有时间限定模式*/
  CMD_CarControl_NoTimeLimit,             /*小车方向控制：无时间限定模式*/
  CMD_MotorControl_Speed,                 /*电机控制:控制转速模式*/
  CMD_ServoControl,                       /*舵机控制:模式*/
  CMD_LightingControl_TimeLimit,          /*灯光控制:模式*/
  CMD_LightingControl_NoTimeLimit,        /*灯光控制:模式*/

};

/*控制管理成员*/
struct Application_xxx
{
  ConquerorCarMotionControl Motion_Control;
  ConquerorCarFunctionalModel Functional_Mode;
  unsigned long CMD_CarControl_Millis;
  unsigned long CMD_LightingControl_Millis;
};
Application_xxx Application_ConquerorCarxxx0;

bool ApplicationFunctionSet_ConquerorCarLeaveTheGround(void);
void ApplicationFunctionSet_ConquerorCarLinearMotionControl(ConquerorCarMotionControl direction, uint8_t directionRecord, uint8_t speed, uint8_t Kp, uint8_t UpperLimit);
void ApplicationFunctionSet_ConquerorCarMotionControl(ConquerorCarMotionControl direction, uint8_t is_speed);

void ApplicationFunctionSet::ApplicationFunctionSet_Init(void)
{
  bool res_error = true;
  Serial.begin(9600);
  AppVoltage.DeviceDriverSet_Voltage_Init();
  AppMotor.DeviceDriverSet_Motor_Init();
  AppServo.DeviceDriverSet_Servo_Init(90);
  AppKey.DeviceDriverSet_Key_Init();
  AppRBG_LED.DeviceDriverSet_RBGLED_Init(20);
  AppIRrecv.DeviceDriverSet_IRrecv_Init();
  AppULTRASONIC.DeviceDriverSet_ULTRASONIC_Init();
  AppITR20001.DeviceDriverSet_ITR20001_Init();
  res_error = AppMPU6050getdata.MPU6050_dveInit();
  AppMPU6050getdata.MPU6050_calibration();

  while (Serial.read() >= 0)
  {
    /*清空串口缓存...*/
  }
  Application_ConquerorCarxxx0.Functional_Mode = Standby_mode;
}

/*ITR20001 检测小车是否离开地面*/
static bool ApplicationFunctionSet_ConquerorCarLeaveTheGround(void)
{
  if (AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_R() > Application_FunctionSet.TrackingDetection_V &&
      AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_M() > Application_FunctionSet.TrackingDetection_V &&
      AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_L() > Application_FunctionSet.TrackingDetection_V)
  {
    Application_FunctionSet.Car_LeaveTheGround = false;
    return false;
  }
  else
  {
    Application_FunctionSet.Car_LeaveTheGround = true;
    return true;
  }
}
/*
  直线运动控制：
  direction：方向选择 前/后
  directionRecord：方向记录（作用于首次进入该函数时更新方向位置数据，即:yaw偏航）
  speed：输入速度 （0--255）
  Kp：位置误差放大比例常数项（提高位置回复状态的反映，输入时根据不同的运动工作模式进行修改）
  UpperLimit：最大输出控制量上限
*/
static void ApplicationFunctionSet_ConquerorCarLinearMotionControl(ConquerorCarMotionControl direction, uint8_t directionRecord, uint8_t speed, uint8_t Kp, uint8_t UpperLimit)
{
  static float Yaw; //偏航
  static float yaw_So = 0;
  static uint8_t en = 110;
  static unsigned long is_time;
  if (en != directionRecord || millis() - is_time > 10)
  {
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_void, /*speed_A*/ 0,
                                           /*direction_B*/ direction_void, /*speed_B*/ 0, /*controlED*/ control_enable); //Motor control
    AppMPU6050getdata.MPU6050_dveGetEulerAngles(&Yaw);
    is_time = millis();
  }
  //if (en != directionRecord)
  if (en != directionRecord || Application_FunctionSet.Car_LeaveTheGround == false)
  {
    en = directionRecord;
    yaw_So = Yaw;
  }
  //加入比例常数Kp
  int R = (Yaw - yaw_So) * Kp + speed;
  if (R > UpperLimit)
  {
    R = UpperLimit;
  }
  else if (R < 10)
  {
    R = 10;
  }
  int L = (yaw_So - Yaw) * Kp + speed;
  if (L > UpperLimit)
  {
    L = UpperLimit;
  }
  else if (L < 10)
  {
    L = 10;
  }
  if (direction == Forward) //前进
  {
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ R,
                                           /*direction_B*/ direction_just, /*speed_B*/ L, /*controlED*/ control_enable);
  }
  else if (direction == Backward) //后退
  {
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ L,
                                           /*direction_B*/ direction_back, /*speed_B*/ R, /*controlED*/ control_enable);
  }
}
/*
  运动控制:
  1# direction方向:前行（1）、后退（2）、 左前（3）、右前（4）、后左（5）、后右（6）
  2# speed速度(0--255)
*/
static void ApplicationFunctionSet_ConquerorCarMotionControl(ConquerorCarMotionControl direction, uint8_t is_speed)
{
  //ApplicationFunctionSet Application_FunctionSet;
  static uint8_t directionRecord = 0;
  uint8_t Kp, UpperLimit;
  uint8_t speed = is_speed;
  //需要进行直线运动调整的控制模式（在以下工作运动模式小车前后方向运动时容易产生位置偏移，运动达不到相对直线方向的效果，因此需要加入控制调节）
  switch (Application_ConquerorCarxxx0.Functional_Mode)
  {
  case Rocker_mode:
    Kp = 10;
    UpperLimit = 255;
    break;
  case ObstacleAvoidance_mode:
    Kp = 2;
    UpperLimit = 180;
    break;
  case Follow_mode:
    Kp = 2;
    UpperLimit = 180;
    break;
  case CMD_CarControl_TimeLimit:
    Kp = 2;
    UpperLimit = 180;
    break;
  case CMD_CarControl_NoTimeLimit:
    Kp = 2;
    UpperLimit = 180;
    break;
  default:
    Kp = 10;
    UpperLimit = 255;
    break;
  }
  switch (direction)
  {
  case /* constant-expression */
      Forward:
    /* code */
    if (Application_ConquerorCarxxx0.Functional_Mode == TraceBased_mode)
    {
      AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ speed,
                                             /*direction_B*/ direction_just, /*speed_B*/ speed, /*controlED*/ control_enable); //Motor control
    }
    else
    { //前进时进入方向位置逼近控制环处理
      ApplicationFunctionSet_ConquerorCarLinearMotionControl(Forward, directionRecord, speed, Kp, UpperLimit);
      directionRecord = 1;
    }

    break;
  case /* constant-expression */ Backward:
    /* code */
    if (Application_ConquerorCarxxx0.Functional_Mode == TraceBased_mode)
    {
      AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ speed,
                                             /*direction_B*/ direction_back, /*speed_B*/ speed, /*controlED*/ control_enable); //Motor control
    }
    else
    { //后退时进入方向位置逼近控制环处理
      ApplicationFunctionSet_ConquerorCarLinearMotionControl(Backward, directionRecord, speed, Kp, UpperLimit);
      directionRecord = 2;
    }

    break;
  case /* constant-expression */ Left:
    /* code */
    directionRecord = 3;
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ speed,
                                           /*direction_B*/ direction_back, /*speed_B*/ speed, /*controlED*/ control_enable); //Motor control
    break;
  case /* constant-expression */ Right:
    /* code */
    directionRecord = 4;
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ speed,
                                           /*direction_B*/ direction_just, /*speed_B*/ speed, /*controlED*/ control_enable); //Motor control
    break;
  case /* constant-expression */ LeftForward:
    /* code */
    directionRecord = 5;
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ speed,
                                           /*direction_B*/ direction_just, /*speed_B*/ speed / 2, /*controlED*/ control_enable); //Motor control
    break;
  case /* constant-expression */ LeftBackward:
    /* code */
    directionRecord = 6;
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ speed,
                                           /*direction_B*/ direction_back, /*speed_B*/ speed / 2, /*controlED*/ control_enable); //Motor control
    break;
  case /* constant-expression */ RightForward:
    /* code */
    directionRecord = 7;
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ speed / 2,
                                           /*direction_B*/ direction_just, /*speed_B*/ speed, /*controlED*/ control_enable); //Motor control
    break;
  case /* constant-expression */ RightBackward:
    /* code */
    directionRecord = 8;
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ speed / 2,
                                           /*direction_B*/ direction_back, /*speed_B*/ speed, /*controlED*/ control_enable); //Motor control
    break;
  case /* constant-expression */ stop_it:
    /* code */
    directionRecord = 9;
    AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_void, /*speed_A*/ 0,
                                           /*direction_B*/ direction_void, /*speed_B*/ 0, /*controlED*/ control_enable); //Motor control
    break;
  default:
    directionRecord = 10;
    break;
  }
}
/*
 传感器数据更新:局部更新(选择性更新)
*/
void ApplicationFunctionSet::ApplicationFunctionSet_SensorDataUpdate(void)
{
  { /*电压状态更新*/
    static unsigned long VoltageData_time = 0;
    static uint8_t VoltageData_number = 1;
    if (millis() - VoltageData_time > 10) //10ms 采集并更新一次
    {
      VoltageData_time = millis();
      VoltageData_V = AppVoltage.DeviceDriverSet_Voltage_getAnalogue();
      if (VoltageData_V < VoltageDetection)
      {
        VoltageData_number++;
        if (VoltageData_number == 250) //连续性多次判断最新的电压值...
        {
          VoltageDetectionStatus = true;
          VoltageData_number = 0;
        }
      }
      else
      {
        VoltageDetectionStatus = false;
      }
    }
  }

  // { /*避障状态更新*/
  //   AppULTRASONIC.DeviceDriverSet_ULTRASONIC_Get(&UltrasoundData_cm /*out*/);
  //   UltrasoundDetectionStatus = function_xxx(UltrasoundData_cm, 0, ObstacleDetection);
  // }

  { /*R循迹状态更新*/
    TrackingData_R = AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_R();
    TrackingDetectionStatus_R = function_xxx(TrackingData_R, TrackingDetection_S, TrackingDetection_E);
    TrackingData_M = AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_M();
    TrackingDetectionStatus_M = function_xxx(TrackingData_M, TrackingDetection_S, TrackingDetection_E);
    TrackingData_L = AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_L();
    TrackingDetectionStatus_L = function_xxx(TrackingData_L, TrackingDetection_S, TrackingDetection_E);
    //ITR20001 检测小车是否离开地面
    ApplicationFunctionSet_ConquerorCarLeaveTheGround();
  }

  // //获取时间戳 timestamp
  // static unsigned long Test_time;
  // if (millis() - Test_time > 500)
  // {
  //   Test_time = millis();
  // }
}
/*
  开机动作需求：
*/
void ApplicationFunctionSet::ApplicationFunctionSet_Bootup(void)
{
  Application_ConquerorCarxxx0.Functional_Mode = Standby_mode;
}

static void CMD_Lighting(uint8_t is_LightingSequence, int8_t is_LightingColorValue_R, uint8_t is_LightingColorValue_G, uint8_t is_LightingColorValue_B)
{
  switch (is_LightingSequence)
  {
  case 0:
    AppRBG_LED.DeviceDriverSet_RBGLED_Color(NUM_LEDS, is_LightingColorValue_R, is_LightingColorValue_G, is_LightingColorValue_B);
    break;
  case 1: /*左*/
    AppRBG_LED.DeviceDriverSet_RBGLED_Color(3, is_LightingColorValue_R, is_LightingColorValue_G, is_LightingColorValue_B);
    break;
  case 2: /*前*/
    AppRBG_LED.DeviceDriverSet_RBGLED_Color(2, is_LightingColorValue_R, is_LightingColorValue_G, is_LightingColorValue_B);
    break;
  case 3: /*右*/
    AppRBG_LED.DeviceDriverSet_RBGLED_Color(1, is_LightingColorValue_R, is_LightingColorValue_G, is_LightingColorValue_B);
    break;
  case 4: /*后*/
    AppRBG_LED.DeviceDriverSet_RBGLED_Color(0, is_LightingColorValue_R, is_LightingColorValue_G, is_LightingColorValue_B);
    break;
  case 5: /*中*/
    AppRBG_LED.DeviceDriverSet_RBGLED_Color(4, is_LightingColorValue_R, is_LightingColorValue_G, is_LightingColorValue_B);
    break;
  default:
    break;
  }
}

/*RBG_LED 集合*/
void ApplicationFunctionSet::ApplicationFunctionSet_RGB(void)
{
  static unsigned long getAnalogue_time = 0;
  FastLED.clear(true);
  if (true == VoltageDetectionStatus) //低电压？
  {
    if ((millis() - getAnalogue_time) > 3000)
    {
      getAnalogue_time = millis();
    }
  }
  unsigned long temp = millis() - getAnalogue_time;
  if (function_xxx((temp), 0, 500) && VoltageDetectionStatus == true)
  {
    switch (temp)
    {
    case /* constant-expression */ 0 ... 49:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Red);
      break;
    case /* constant-expression */ 50 ... 99:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Black);
      break;
    case /* constant-expression */ 100 ... 149:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Red);
      break;
    case /* constant-expression */ 150 ... 199:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Black);
      break;
    case /* constant-expression */ 200 ... 249:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Red);
      break;
    case /* constant-expression */ 250 ... 299:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Red);
      break;
    case /* constant-expression */ 300 ... 349:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Black);
      break;
    case /* constant-expression */ 350 ... 399:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Red);
      break;
    case /* constant-expression */ 400 ... 449:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Black);
      break;
    case /* constant-expression */ 450 ... 499:
      /* code */
      AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Red);
      break;
    default:
      break;
    }
  }
  else if (((function_xxx((temp), 500, 3000)) && VoltageDetectionStatus == true) || VoltageDetectionStatus == false)
  {
    switch (Application_ConquerorCarxxx0.Functional_Mode) //Act on 模式控制序列
    {
    case /* constant-expression */ Standby_mode:
      /* code */
      {
        if (VoltageDetectionStatus == true)
        {
          AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Red);
          delay(30);
          AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Black);
          delay(30);
        }
        else
        {
          static uint8_t setBrightness = 0;
          static boolean et = false;
          static unsigned long time = 0;

          if ((millis() - time) > 30)
          {
            time = millis();
            if (et == false)
            {
              setBrightness += 1;
              if (setBrightness == 80)
                et = true;
            }
            else if (et == true)
            {
              setBrightness -= 1;
              if (setBrightness == 1)
                et = false;
            }
          }

          // AppRBG_LED.leds[1] = CRGB::Blue;
          AppRBG_LED.leds[0] = CRGB::Violet;
          FastLED.setBrightness(setBrightness);
          FastLED.show();
        }
      }
      break;
    case /* constant-expression */ CMD_Programming_mode:
      /* code */
      {
      }
      break;
    case /* constant-expression */ TraceBased_mode:
      /* code */
      {
        AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Green);
      }
      break;
    case /* constant-expression */ ObstacleAvoidance_mode:
      /* code */
      {
        AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Yellow);
      }
      break;
    case /* constant-expression */ Follow_mode:
      /* code */
      {
        AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Blue);
      }
      break;
    case /* constant-expression */ Rocker_mode:
      /* code */
      {
        AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, 2 /*Traversal_Number*/, CRGB::Violet);
      }
      break;
    default:
      break;
    }
  }
}

/*摇杆*/
void ApplicationFunctionSet::ApplicationFunctionSet_Rocker(void)
{
  if (Application_ConquerorCarxxx0.Functional_Mode == Rocker_mode)
  {
    ApplicationFunctionSet_ConquerorCarMotionControl(Application_ConquerorCarxxx0.Motion_Control /*direction*/, Rocker_CarSpeed /*speed*/);
  }
}

/*循迹*/
void ApplicationFunctionSet::ApplicationFunctionSet_Tracking(void)
{
  static boolean timestamp = true;
  static boolean BlindDetection = true;
  static unsigned long MotorRL_time = 0;
  if (Application_ConquerorCarxxx0.Functional_Mode == TraceBased_mode)
  {
    if (Car_LeaveTheGround == false) //车子离开地面了？
    {
      ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
      return;
    }
    int getAnaloguexxx_L = AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_L();
    int getAnaloguexxx_M = AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_M();
    int getAnaloguexxx_R = AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_R();
#if _Test_print
    static unsigned long print_time = 0;
    if (millis() - print_time > 500)
    {
      print_time = millis();
      Serial.print("ITR20001_getAnaloguexxx_L=");
      Serial.println(getAnaloguexxx_L);
      Serial.print("ITR20001_getAnaloguexxx_M=");
      Serial.println(getAnaloguexxx_M);
      Serial.print("ITR20001_getAnaloguexxx_R=");
      Serial.println(getAnaloguexxx_R);
    }
#endif
    if (function_xxx(getAnaloguexxx_M, TrackingDetection_S, TrackingDetection_E))
    {
      /*控制左右电机转动：实现匀速直行*/
      ApplicationFunctionSet_ConquerorCarMotionControl(Forward, 200);
      timestamp = true;
      BlindDetection = true;
    }
    else if (function_xxx(getAnaloguexxx_R, TrackingDetection_S, TrackingDetection_E))
    {
      /*控制左右电机转动：前右*/
      ApplicationFunctionSet_ConquerorCarMotionControl(Right, 200);
      timestamp = true;
      BlindDetection = true;
    }
    else if (function_xxx(getAnaloguexxx_L, TrackingDetection_S, TrackingDetection_E))
    {
      /*控制左右电机转动：前左*/
      ApplicationFunctionSet_ConquerorCarMotionControl(Left, 200);
      timestamp = true;
      BlindDetection = true;
    }
    else //不在黑线上的时候。。。
    {
      if (timestamp == true) //获取时间戳 timestamp
      {
        timestamp = false;
        MotorRL_time = millis();
        ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
      }
      /*Blind Detection*/
      if ((function_xxx((millis() - MotorRL_time), 0, 200) || function_xxx((millis() - MotorRL_time), 1600, 2000)) && BlindDetection == true)
      {
        ApplicationFunctionSet_ConquerorCarMotionControl(Right, 250);
      }
      else if (((function_xxx((millis() - MotorRL_time), 200, 1600))) && BlindDetection == true)
      {
        ApplicationFunctionSet_ConquerorCarMotionControl(Left, 250);
      }
      else if ((function_xxx((millis() - MotorRL_time), 3000, 3500))) // Blind Detection ...s ?
      {
        BlindDetection = false;
        ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
      }
    }
  }
  else if (false == timestamp)
  {
    BlindDetection = true;
    timestamp = true;
    MotorRL_time = 0;
  }
}

/*
  避障功能
*/
void ApplicationFunctionSet::ApplicationFunctionSet_Obstacle(void)
{
  if (Application_ConquerorCarxxx0.Functional_Mode == ObstacleAvoidance_mode)
  {
    static unsigned long timestamp = 0;
    static bool en = false;
    static uint8_t en_ULTRASONIC_Get = 0;
    static uint8_t is_ULTRASONIC_Get = 1;

    if (Car_LeaveTheGround == false) //检测小车是否离开地面
    {
      ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
      en = false;
      return;
    }
    else
    {
      if (is_ULTRASONIC_Get != en_ULTRASONIC_Get || en == false)
      {
        if (en != false)
        { //为了保证超声波数据采集的准确性、运动装置需保持静止（即：测距操作时停止对小车左右运动控制）
          ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
        }
        AppULTRASONIC.DeviceDriverSet_ULTRASONIC_Get(&UltrasoundData_cm /*out*/);
        en_ULTRASONIC_Get = is_ULTRASONIC_Get;
      }
#if _Test_print
      Serial.print("UltrasoundData_cm=");
      Serial.println(UltrasoundData_cm);
#endif
      {
        if (function_xxx(UltrasoundData_cm, 0, ObstacleDetection)) //前方 ObstacleDetection cm内有障碍物？
        {
          //获取时间戳 timestamp
          if (en == false || millis() - timestamp > 3100)
          {
            en = true;
            timestamp = millis();
          }
          if (function_xxx((millis() - timestamp), 0, 500)) //-
          {
            is_ULTRASONIC_Get = 1;
            ApplicationFunctionSet_ConquerorCarMotionControl(Right, 250);
          }
          else if (function_xxx((millis() - timestamp), 500, 600))
          {
            is_ULTRASONIC_Get = 2;
          }
          else if (function_xxx((millis() - timestamp), 600, 1600)) //+
          {
            ApplicationFunctionSet_ConquerorCarMotionControl(Left, 250);
          }
          else if (function_xxx((millis() - timestamp), 1600, 1700))
          {
            is_ULTRASONIC_Get = 3;
          }
          else if (function_xxx((millis() - timestamp), 1700, 2200)) //0
          {
            ApplicationFunctionSet_ConquerorCarMotionControl(Right, 250);
          }
          else if (function_xxx((millis() - timestamp), 2200, 2700)) //
          {
            ApplicationFunctionSet_ConquerorCarMotionControl(Backward, 250);
          }
          else if (function_xxx((millis() - timestamp), 2700, 3000)) //0
          {
            ApplicationFunctionSet_ConquerorCarMotionControl(Left, 250);
          }
          else if (function_xxx((millis() - timestamp), 3000, 3100)) //0
          {
            is_ULTRASONIC_Get = 4;
          }
        }
        else //只有前方有效控制范围内无障碍，则永远保持向前直行运动
        {
          ApplicationFunctionSet_ConquerorCarMotionControl(Rocker_CarSpeed, 250);
          en = false;
        }
      }
    }
  }
}

/*
  跟随模式：
*/
void ApplicationFunctionSet::ApplicationFunctionSet_Follow(void)
{

  if (Application_ConquerorCarxxx0.Functional_Mode == Follow_mode)
  {
    static bool en = false;
    static unsigned long timestamp = 0;
    if (Car_LeaveTheGround == false)
    {
      ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
      return;
    }
    else
    {
      AppULTRASONIC.DeviceDriverSet_ULTRASONIC_Get(&UltrasoundData_cm /*out*/);
#if _Test_print
      Serial.print("UltrasoundData_cm=");
      Serial.println(UltrasoundData_cm);
#endif

      if (function_xxx(UltrasoundData_cm, 0, ObstacleDetection)) //前方 ObstacleDetection cm内有接触？ （等同于避障模式障碍物）
      {
        //保持向前直行运动
        ApplicationFunctionSet_ConquerorCarMotionControl(Forward, 250);
        en = false;
      }
      else
      {
        //获取时间戳 timestamp
        if (en == false)
        {
          en = true;
          timestamp = millis();
        }
        if (function_xxx((millis() - timestamp), 0, 500) || function_xxx((millis() - timestamp), 1000, 1500) || function_xxx((millis() - timestamp), 2000, 2500) || function_xxx((millis() - timestamp), 3000, 3500)) //
        {
          ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
        }
        else if (function_xxx((millis() - timestamp), 500, 1000)) //-
        {
          ApplicationFunctionSet_ConquerorCarMotionControl(Right, 255);
        }
        else if (function_xxx((millis() - timestamp), 1500, 2000) || function_xxx((millis() - timestamp), 2500, 3000)) //+
        {
          ApplicationFunctionSet_ConquerorCarMotionControl(Left, 255);
        }
        else if (function_xxx((millis() - timestamp), 3500, 4000)) //+
        {
          ApplicationFunctionSet_ConquerorCarMotionControl(Right, 255);
        }
        else if (function_xxx((millis() - timestamp), 4000, 4500)) //+
        {
          ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
        }
      }
    }
  }
}
/*舵机控制*/
void ApplicationFunctionSet::ApplicationFunctionSet_Servo(uint8_t Set_Servo)
{
  static int z_angle = 90;
  static int y_angle = 90;
  uint8_t is_Servo = Set_Servo; //防止被优化

  switch (is_Servo)
  {
  case 1 ... 2:
  {
    if (1 == is_Servo)
    {
      y_angle -= 10;
    }
    else if (2 == is_Servo)
    {
      y_angle += 10;
    }
    if (y_angle <= 30) //下限控制
    {
      y_angle = 30;
    }
    if (y_angle >= 110) //上下限控制
    {
      y_angle = 110;
    }
    AppServo.DeviceDriverSet_Servo_controls(/*uint8_t Servo--y*/ 2, /*unsigned int Position_angle*/ y_angle);
  }
  break;

  case 3 ... 4:
  {
    if (3 == is_Servo)
    {
      z_angle += 10;
    }
    else if (4 == is_Servo)
    {
      z_angle -= 10;
    }

    if (z_angle <= 0) //下限控制
    {
      z_angle = 0;
    }
    if (z_angle >= 180) //上下限控制
    {
      z_angle = 180;
    }
    AppServo.DeviceDriverSet_Servo_controls(/*uint8_t Servo--z*/ 1, /*unsigned int Position_angle*/ z_angle);
  }
  break;
  case 5:
    AppServo.DeviceDriverSet_Servo_controls(/*uint8_t Servo--y*/ 2, /*unsigned int Position_angle*/ 90);
    AppServo.DeviceDriverSet_Servo_controls(/*uint8_t Servo--z*/ 1, /*unsigned int Position_angle*/ 90);
    break;
  default:
    break;
  }
}
/*待机*/
void ApplicationFunctionSet::ApplicationFunctionSet_Standby(void)
{
  static bool is_ED = true;
  static uint8_t cout = 0;
  if (Application_ConquerorCarxxx0.Functional_Mode == Standby_mode)
  {
    ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
    if (true == is_ED) //作用于偏航原始数据调零(确认小车放置在静止平面！)
    {
      static unsigned long timestamp; //获取时间戳 timestamp
      if (millis() - timestamp > 20)
      {
        timestamp = millis();
        if (ApplicationFunctionSet_ConquerorCarLeaveTheGround() /* condition */)
        {
          cout += 1;
        }
        else
        {
          cout = 0;
        }
        if (cout > 10)
        {
          is_ED = false;
          AppMPU6050getdata.MPU6050_calibration();
        }
      }
    }
  }
}

/* 
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * Begin:CMD
 * Graphical programming and command control module
 $ Elegoo & Conqueror & 2020-06
*/

void ApplicationFunctionSet::CMD_inspect_xxx0(void)
{
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_inspect)
  {
    Serial.println("CMD_inspect");
    delay(100);
  }
}
/*
  N1:指令
  CMD模式：运动模式 <电机控制> 接收并根据 APP端控制命令   执行对电机的单方向驱动
  输入：uint8_t is_MotorSelection,  电机选择   1左  2右  0全部
        uint8_t is_MotorDirection, 电机转向  1正  2反  0停止
        uint8_t is_MotorSpeed,     电机速度   0-250
        无时间限定
*/
void ApplicationFunctionSet::CMD_MotorControl_xxx0(uint8_t is_MotorSelection, uint8_t is_MotorDirection, uint8_t is_MotorSpeed)
{
  static boolean MotorControl = false;
  static uint8_t is_MotorSpeed_A = 0;
  static uint8_t is_MotorSpeed_B = 0;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_MotorControl)
  {
    MotorControl = true;
    if (0 == is_MotorDirection)
    {
      ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
    }
    else
    {
      switch (is_MotorSelection) //电机选择
      {
      case 0:
      {
        is_MotorSpeed_A = is_MotorSpeed;
        is_MotorSpeed_B = is_MotorSpeed;
        if (1 == is_MotorDirection)
        { //正转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_just, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else if (2 == is_MotorDirection)
        { //反转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_back, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else
        {
          return;
        }
      }
      break;
      case 1:
      {
        is_MotorSpeed_A = is_MotorSpeed;
        if (1 == is_MotorDirection)
        { //正转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_void, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else if (2 == is_MotorDirection)
        { //反转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_void, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else
        {
          return;
        }
      }
      break;
      case 2:
      {
        is_MotorSpeed_B = is_MotorSpeed;
        if (1 == is_MotorDirection)
        { //正转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_void, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_just, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else if (2 == is_MotorDirection)
        { //反转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_void, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_back, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else
        {
          return;
        }
      }
      break;
      default:
        break;
      }
    }
  }
  else
  {
    if (MotorControl == true)
    {
      MotorControl = false;
      is_MotorSpeed_A = 0;
      is_MotorSpeed_B = 0;
    }
  }
}
void ApplicationFunctionSet::CMD_MotorControl_xxx0(void)
{
  static boolean MotorControl = false;
  static uint8_t is_MotorSpeed_A = 0;
  static uint8_t is_MotorSpeed_B = 0;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_MotorControl)
  {
    MotorControl = true;
    if (0 == CMD_is_MotorDirection)
    {
      ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
    }
    else
    {
      switch (CMD_is_MotorSelection) //电机选择
      {
      case 0:
      {
        is_MotorSpeed_A = CMD_is_MotorSpeed;
        is_MotorSpeed_B = CMD_is_MotorSpeed;
        if (1 == CMD_is_MotorDirection)
        { //正转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_just, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else if (2 == CMD_is_MotorDirection)
        { //反转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_back, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else
        {
          return;
        }
      }
      break;
      case 1:
      {
        is_MotorSpeed_A = CMD_is_MotorSpeed;
        if (1 == CMD_is_MotorDirection)
        { //正转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_void, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else if (2 == CMD_is_MotorDirection)
        { //反转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_back, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_void, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else
        {
          return;
        }
      }
      break;
      case 2:
      {
        is_MotorSpeed_B = CMD_is_MotorSpeed;
        if (1 == CMD_is_MotorDirection)
        { //正转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_void, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_just, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else if (2 == CMD_is_MotorDirection)
        { //反转
          AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_void, /*speed_A*/ is_MotorSpeed_A,
                                                 /*direction_B*/ direction_back, /*speed_B*/ is_MotorSpeed_B,
                                                 /*controlED*/ control_enable); //Motor control
        }
        else
        {
          return;
        }
      }
      break;
      default:
        break;
      }
    }
  }
  else
  {
    if (MotorControl == true)
    {
      MotorControl = false;
      is_MotorSpeed_A = 0;
      is_MotorSpeed_B = 0;
    }
  }
}

static void CMD_CarControl(uint8_t is_CarDirection, uint8_t is_CarSpeed)
{
  switch (is_CarDirection)
  {
  case 1: /*运动模式 左前*/
    ApplicationFunctionSet_ConquerorCarMotionControl(Left, is_CarSpeed);
    break;
  case 2: /*运动模式 右前*/
    ApplicationFunctionSet_ConquerorCarMotionControl(Right, is_CarSpeed);
    break;
  case 3: /*运动模式 前进*/
    ApplicationFunctionSet_ConquerorCarMotionControl(Forward, is_CarSpeed);
    break;
  case 4: /*运动模式 后退*/
    ApplicationFunctionSet_ConquerorCarMotionControl(Backward, is_CarSpeed);
    break;
  default:
    break;
  }
}
/*
  N2：指令
  CMD模式：<车子控制> 接收并根据 APP端控制命令   执行对车的单方向驱动
  有时间限定
*/
void ApplicationFunctionSet::CMD_CarControlTimeLimit_xxx0(uint8_t is_CarDirection, uint8_t is_CarSpeed, uint32_t is_Timer)
{
  static boolean CarControl = false;
  static boolean CarControl_TE = false; //还有时间标志
  static boolean CarControl_return = false;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_CarControl_TimeLimit) //进入车子有时间限定控制模式
  {
    CarControl = true;
    if (is_Timer != 0) //#1设定时间不为..时 (空)
    {
      if ((millis() - Application_ConquerorCarxxx0.CMD_CarControl_Millis) > (is_Timer)) //判断时间戳
      {
        CarControl_TE = true;
        ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);

        Application_ConquerorCarxxx0.Functional_Mode = CMD_Programming_mode; /*进入编程模式提示符<等待下一组控制命令的到来>*/
        if (CarControl_return == false)
        {

#if _is_print
          Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
          CarControl_return = true;
        }
      }
      else
      {
        CarControl_TE = false; //还有时间
        CarControl_return = false;
      }
    }
    if (CarControl_TE == false)
    {
      CMD_CarControl(is_CarDirection, is_CarSpeed);
    }
  }
  else
  {
    if (CarControl == true)
    {
      CarControl_return = false;
      CarControl = false;
      Application_ConquerorCarxxx0.CMD_CarControl_Millis = 0;
    }
  }
}

void ApplicationFunctionSet::CMD_CarControlTimeLimit_xxx0(void)
{
  static boolean CarControl = false;
  static boolean CarControl_TE = false; //还有时间标志
  static boolean CarControl_return = false;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_CarControl_TimeLimit) //进入车子有时间限定控制模式
  {
    CarControl = true;
    if (CMD_is_CarTimer != 0) //#1设定时间不为..时 (空)
    {
      if ((millis() - Application_ConquerorCarxxx0.CMD_CarControl_Millis) > (CMD_is_CarTimer)) //判断时间戳
      {
        CarControl_TE = true;
        ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);

        Application_ConquerorCarxxx0.Functional_Mode = CMD_Programming_mode; /*进入编程模式提示符<等待下一组控制命令的到来>*/
        if (CarControl_return == false)
        {

#if _is_print
          Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
          CarControl_return = true;
        }
      }
      else
      {
        CarControl_TE = false; //还有时间
        CarControl_return = false;
      }
    }
    if (CarControl_TE == false)
    {
      CMD_CarControl(CMD_is_CarDirection, CMD_is_CarSpeed);
    }
  }
  else
  {
    if (CarControl == true)
    {
      CarControl_return = false;
      CarControl = false;
      Application_ConquerorCarxxx0.CMD_CarControl_Millis = 0;
    }
  }
}
/*
  N3：指令
  CMD模式：<车子控制> 接收并根据 APP端控制命令   执行对车的单方向驱动
  无时间限定
*/
void ApplicationFunctionSet::CMD_CarControlNoTimeLimit_xxx0(uint8_t is_CarDirection, uint8_t is_CarSpeed)
{
  static boolean CarControl = false;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_CarControl_NoTimeLimit) //进入小车无时间限定控制模式
  {
    CarControl = true;
    CMD_CarControl(is_CarDirection, is_CarSpeed);
  }
  else
  {
    if (CarControl == true)
    {
      CarControl = false;
    }
  }
}
void ApplicationFunctionSet::CMD_CarControlNoTimeLimit_xxx0(void)
{
  static boolean CarControl = false;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_CarControl_NoTimeLimit) //进入小车无时间限定控制模式
  {
    CarControl = true;
    CMD_CarControl(CMD_is_CarDirection, CMD_is_CarSpeed);
  }
  else
  {
    if (CarControl == true)
    {
      CarControl = false;
    }
  }
}

/*
  N4 : 指令
  CMD模式：运动模式<电机控制>
  接收并根据 APP端控制命令 执行对左右电机转速的控制
*/
void ApplicationFunctionSet::CMD_MotorControlSpeed_xxx0(uint8_t is_Speed_L, uint8_t is_Speed_R)
{
  static boolean MotorControl = false;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_MotorControl_Speed)
  {
    MotorControl = true;
    if (is_Speed_L == 0 && is_Speed_R == 0)
    {
      ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
    }
    else
    {
      AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ is_Speed_L,
                                             /*direction_B*/ direction_just, /*speed_B*/ is_Speed_R,
                                             /*controlED*/ control_enable); //Motor control
    }
  }
  else
  {
    if (MotorControl == true)
    {
      MotorControl = false;
    }
  }
}
void ApplicationFunctionSet::CMD_MotorControlSpeed_xxx0(void)
{
  static boolean MotorControl = false;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_MotorControl_Speed)
  {
    MotorControl = true;
    if (CMD_is_MotorSpeed_L == 0 && CMD_is_MotorSpeed_R == 0)
    {
      ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
    }
    else
    {
      AppMotor.DeviceDriverSet_Motor_control(/*direction_A*/ direction_just, /*speed_A*/ CMD_is_MotorSpeed_L,
                                             /*direction_B*/ direction_just, /*speed_B*/ CMD_is_MotorSpeed_R,
                                             /*controlED*/ control_enable); //Motor control
    }
  }
  else
  {
    if (MotorControl == true)
    {
      MotorControl = false;
    }
  }
}

/*
  N5:指令
  CMD模式：<舵机控制>
*/
void ApplicationFunctionSet::CMD_ServoControl_xxx0(void)
{
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_ServoControl)
  {
    AppServo.DeviceDriverSet_Servo_controls(/*uint8_t Servo*/ CMD_is_Servo, /*unsigned int Position_angle*/ CMD_is_Servo_angle);
    Application_ConquerorCarxxx0.Functional_Mode = CMD_Programming_mode; /*进入编程模式提示符<等待下一组控制命令的到来>*/
  }
}
/*
  N7:指令
   CMD模式：<灯光控制>
   有时间限定：时间结束后进入编程模式
*/
void ApplicationFunctionSet::CMD_LightingControlTimeLimit_xxx0(uint8_t is_LightingSequence, uint8_t is_LightingColorValue_R, uint8_t is_LightingColorValue_G, uint8_t is_LightingColorValue_B,
                                                               uint32_t is_LightingTimer)
{
  static boolean LightingControl = false;
  static boolean LightingControl_TE = false; //还有时间标志
  static boolean LightingControl_return = false;

  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_LightingControl_TimeLimit) //进入灯光有时间限定控制模式
  {
    LightingControl = true;
    if (is_LightingTimer != 0) //#1设定时间不为..时 (空)
    {
      if ((millis() - Application_ConquerorCarxxx0.CMD_LightingControl_Millis) > (is_LightingTimer)) //判断时间戳
      {
        LightingControl_TE = true;
        FastLED.clear(true);
        Application_ConquerorCarxxx0.Functional_Mode = CMD_Programming_mode; /*进入编程模式提示符<等待下一组控制命令的到来>*/
        if (LightingControl_return == false)
        {

#if _is_print
          Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
          LightingControl_return = true;
        }
      }
      else
      {
        LightingControl_TE = false; //还有时间
        LightingControl_return = false;
      }
    }
    if (LightingControl_TE == false)
    {
      CMD_Lighting(is_LightingSequence, is_LightingColorValue_R, is_LightingColorValue_G, is_LightingColorValue_B);
    }
  }
  else
  {
    if (LightingControl == true)
    {
      LightingControl_return = false;
      LightingControl = false;
      Application_ConquerorCarxxx0.CMD_LightingControl_Millis = 0;
    }
  }
}
void ApplicationFunctionSet::CMD_LightingControlTimeLimit_xxx0(void)
{
  static boolean LightingControl = false;
  static boolean LightingControl_TE = false; //还有时间标志
  static boolean LightingControl_return = false;

  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_LightingControl_TimeLimit) //进入灯光有时间限定控制模式
  {
    LightingControl = true;
    if (CMD_is_LightingTimer != 0) //#1设定时间不为..时 (空)
    {
      if ((millis() - Application_ConquerorCarxxx0.CMD_LightingControl_Millis) > (CMD_is_LightingTimer)) //判断时间戳
      {
        LightingControl_TE = true;
        FastLED.clear(true);
        Application_ConquerorCarxxx0.Functional_Mode = CMD_Programming_mode; /*进入编程模式提示符<等待下一组控制命令的到来>*/
        if (LightingControl_return == false)
        {

#if _is_print
          Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
          LightingControl_return = true;
        }
      }
      else
      {
        LightingControl_TE = false; //还有时间
        LightingControl_return = false;
      }
    }
    if (LightingControl_TE == false)
    {
      CMD_Lighting(CMD_is_LightingSequence, CMD_is_LightingColorValue_R, CMD_is_LightingColorValue_G, CMD_is_LightingColorValue_B);
    }
  }
  else
  {
    if (LightingControl == true)
    {
      LightingControl_return = false;
      LightingControl = false;
      Application_ConquerorCarxxx0.CMD_LightingControl_Millis = 0;
    }
  }
}
/*
  N8:指令
   CMD模式：<灯光控制>
   无时间限定
*/
void ApplicationFunctionSet::CMD_LightingControlNoTimeLimit_xxx0(uint8_t is_LightingSequence, uint8_t is_LightingColorValue_R, uint8_t is_LightingColorValue_G, uint8_t is_LightingColorValue_B)
{
  static boolean LightingControl = false;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_LightingControl_NoTimeLimit) //进入灯光无时间限定控制模式
  {
    LightingControl = true;
    CMD_Lighting(is_LightingSequence, is_LightingColorValue_R, is_LightingColorValue_G, is_LightingColorValue_B);
  }
  else
  {
    if (LightingControl == true)
    {
      LightingControl = false;
    }
  }
}
void ApplicationFunctionSet::CMD_LightingControlNoTimeLimit_xxx0(void)
{
  static boolean LightingControl = false;
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_LightingControl_NoTimeLimit) //进入灯光无时间限定控制模式
  {
    LightingControl = true;
    CMD_Lighting(CMD_is_LightingSequence, CMD_is_LightingColorValue_R, CMD_is_LightingColorValue_G, CMD_is_LightingColorValue_B);
  }
  else
  {
    if (LightingControl == true)
    {
      LightingControl = false;
    }
  }
}

/*
  N100/N110:指令
  CMD模式：清除所有功能
*/
void ApplicationFunctionSet::CMD_ClearAllFunctions_xxx0(void)
{
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_ClearAllFunctions_Standby_mode) //清除所有功能：进入空闲模式    N100:指令
  {
    ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
    FastLED.clear(true);
    AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, NUM_LEDS /*Traversal_Number*/, CRGB::Black);
    Application_ConquerorCarxxx0.Motion_Control = stop_it;
    Application_ConquerorCarxxx0.Functional_Mode = Standby_mode;
  }
  if (Application_ConquerorCarxxx0.Functional_Mode == CMD_ClearAllFunctions_Programming_mode) //清除所有功能：进入编程模式     N110:指令
  {
    ApplicationFunctionSet_ConquerorCarMotionControl(stop_it, 0);
    FastLED.clear(true);
    AppRBG_LED.DeviceDriverSet_RBGLED_xxx(0 /*Duration*/, NUM_LEDS /*Traversal_Number*/, CRGB::Black);
    Application_ConquerorCarxxx0.Motion_Control = stop_it;
    Application_ConquerorCarxxx0.Functional_Mode = CMD_Programming_mode;
  }
}

/*
  N21:指令
  CMD模式：超声波模块处理 接收并根据 APP端控制命令   反馈超声波状态及数据
  输入：
*/
void ApplicationFunctionSet::CMD_UltrasoundModuleStatus_xxx0(uint8_t is_get)
{
  AppULTRASONIC.DeviceDriverSet_ULTRASONIC_Get(&UltrasoundData_cm /*out*/); //超声波数据
  UltrasoundDetectionStatus = function_xxx(UltrasoundData_cm, 0, ObstacleDetection);
  if (1 == is_get) //超声波  is_get Start     true：有障碍物 / false:无障碍物
  {
    if (true == UltrasoundDetectionStatus)
    {
#if _is_print
      Serial.print('{' + CommandSerialNumber + "_true}");
#endif
    }
    else
    {
#if _is_print
      Serial.print('{' + CommandSerialNumber + "_false}");
#endif
    }
  }
  else if (2 == is_get) //超声波 is_get data
  {
    char toString[10];
    sprintf(toString, "%d", UltrasoundData_cm);
#if _is_print
    Serial.print('{' + CommandSerialNumber + '_' + toString + '}');
#endif
  }
}
/*
  N22:指令
  CMD模式：循迹模块 接收并根据 APP端控制命令   反馈循迹状态及数据
  输入：
*/
void ApplicationFunctionSet::CMD_TraceModuleStatus_xxx0(uint8_t is_get)
{
  char toString[10];
  if (0 == is_get) /*循迹状态获取左边*/
  {
    sprintf(toString, "%d", TrackingData_L);
#if _is_print
    Serial.print('{' + CommandSerialNumber + '_' + toString + '}');
#endif
    /*
    if (true == TrackingDetectionStatus_L)
    {
#if _is_print
      Serial.print('{' + CommandSerialNumber + "_true}");
#endif
    }
    else
    {
#if _is_print
      Serial.print('{' + CommandSerialNumber + "_false}");
#endif
    }*/
  }
  else if (1 == is_get) /*循迹状态获取中间*/
  {
    sprintf(toString, "%d", TrackingData_M);
#if _is_print
    Serial.print('{' + CommandSerialNumber + '_' + toString + '}');
#endif
    /*    if (true == TrackingDetectionStatus_M)
    {
#if _is_print
      Serial.print('{' + CommandSerialNumber + "_true}");
#endif
    }
    else
    {
#if _is_print
      Serial.print('{' + CommandSerialNumber + "_false}");
#endif
    }*/
  }
  else if (2 == is_get) /*循迹状态获取右边*/
  {
    sprintf(toString, "%d", TrackingData_R);
#if _is_print
    Serial.print('{' + CommandSerialNumber + '_' + toString + '}');
#endif

    /*  if (true == TrackingDetectionStatus_R)
    {
#if _is_print
      Serial.print('{' + CommandSerialNumber + "_true}");
#endif
    }
    else
    {
#if _is_print
      Serial.print('{' + CommandSerialNumber + "_false}");
#endif
    }
    */
  }
  Application_ConquerorCarxxx0.Functional_Mode = CMD_Programming_mode; /*进入编程模式提示符<等待下一组控制命令的到来>*/
}

/* 
 * End:CMD
 * Graphical programming and command control module
 $ Elegoo & Conqueror & 2020-06
 --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


/*按键命令*/
void ApplicationFunctionSet::ApplicationFunctionSet_KeyCommand(void)
{
  uint8_t get_keyValue;
  static uint8_t temp_keyValue = keyValue_Max;
  AppKey.DeviceDriverSet_key_Get(&get_keyValue);

  if (temp_keyValue != get_keyValue)
  {
    temp_keyValue = get_keyValue;
    switch (get_keyValue)
    {
    case /* constant-expression */ 1:
      /* code */
      Application_ConquerorCarxxx0.Functional_Mode = TraceBased_mode;
      break;
    case /* constant-expression */ 2:
      /* code */
      Application_ConquerorCarxxx0.Functional_Mode = ObstacleAvoidance_mode;
      break;
    case /* constant-expression */ 3:
      /* code */
      Application_ConquerorCarxxx0.Functional_Mode = Follow_mode;
      break;
    case /* constant-expression */ 4:
      /* code */
      Application_ConquerorCarxxx0.Functional_Mode = Standby_mode;
      break;
    default:

      break;
    }
  }
}
/*红外遥控*/
void ApplicationFunctionSet::ApplicationFunctionSet_IRrecv(void)
{
  uint8_t IRrecv_button;
  static bool IRrecv_en = false;
  if (AppIRrecv.DeviceDriverSet_IRrecv_Get(&IRrecv_button /*out*/))
  {
    IRrecv_en = true;
    //Serial.println(IRrecv_button);
  }
  if (true == IRrecv_en)
  {
    switch (IRrecv_button)
    {
    case /* constant-expression */ 1:
      /* code */
      Application_ConquerorCarxxx0.Motion_Control = Forward;
      break;
    case /* constant-expression */ 2:
      /* code */
      Application_ConquerorCarxxx0.Motion_Control = Backward;
      break;
    case /* constant-expression */ 3:
      /* code */
      Application_ConquerorCarxxx0.Motion_Control = Left;
      break;
    case /* constant-expression */ 4:
      /* code */
      Application_ConquerorCarxxx0.Motion_Control = Right;
      break;
    case /* constant-expression */ 5:
      /* code */
      //Application_ConquerorCarxxx0.Motion_Control = stop_it;
      Application_ConquerorCarxxx0.Functional_Mode = Standby_mode;
      break;

    case /* constant-expression */ 6:
      /* code */ Application_ConquerorCarxxx0.Functional_Mode = TraceBased_mode;
      break;
    case /* constant-expression */ 7:
      /* code */ Application_ConquerorCarxxx0.Functional_Mode = ObstacleAvoidance_mode;
      break;
    case /* constant-expression */ 8:
      /* code */ Application_ConquerorCarxxx0.Functional_Mode = Follow_mode;
      break;
    case /* constant-expression */ 9:
      /* code */ if (Application_ConquerorCarxxx0.Functional_Mode == TraceBased_mode) //调节适配循迹模块敏感数据段响应
      {
        TrackingDetection_S += 10;
        if (TrackingDetection_S > 600)
        {
          TrackingDetection_S = 600;
        }
        else if (TrackingDetection_S < 30)
        {
          TrackingDetection_S = 30;
        }
      }

      break;
    case /* constant-expression */ 10:
      /* code */ if (Application_ConquerorCarxxx0.Functional_Mode == TraceBased_mode)
      {
        TrackingDetection_S = 250;
      }
      break;
    case /* constant-expression */ 11:
      /* code */ if (Application_ConquerorCarxxx0.Functional_Mode == TraceBased_mode)
      {
        TrackingDetection_S -= 10;
        if (TrackingDetection_S > 600)
        {
          TrackingDetection_S = 600;
        }
        else if (TrackingDetection_S < 30)
        {
          TrackingDetection_S = 30;
        }
      }
      break;

    default:
      Application_ConquerorCarxxx0.Functional_Mode = Standby_mode;
      break;
    }
    /*方向控制部分实现时长约束控制*/
    if (IRrecv_button < 5)
    {
      Application_ConquerorCarxxx0.Functional_Mode = Rocker_mode;
      if (millis() - AppIRrecv.IR_PreMillis > 300)
      {
        IRrecv_en = false;
        Application_ConquerorCarxxx0.Functional_Mode = Standby_mode;
        AppIRrecv.IR_PreMillis = millis();
      }
    }
    else
    {
      IRrecv_en = false;
      AppIRrecv.IR_PreMillis = millis();
    }
  }
}
/*串口数据解析*/
void ApplicationFunctionSet::ApplicationFunctionSet_SerialPortDataAnalysis(void)
{
  static String SerialPortData = "";
  uint8_t c = "";
  if (Serial.available() > 0)
  {
    while (c != '}' && Serial.available() > 0)
    {
      // while (Serial.available() == 0)//强行等待一帧数据完成接收
      //   ;
      c = Serial.read();
      SerialPortData += (char)c;
    }
  }
  if (c == '}') //数据帧尾部校验
  {
#if _Test_print
    Serial.println(SerialPortData);
#endif

    // if (true == SerialPortData.equals("{f}") || true == SerialPortData.equals("{b}") || true == SerialPortData.equals("{l}") || true == SerialPortData.equals("{r}"))
    // {
    //   Serial.print(SerialPortData);
    //   SerialPortData = "";
    //   return;
    // }
    // if (true == SerialPortData.equals("{Factory}") || true == SerialPortData.equals("{WA_NO}") || true == SerialPortData.equals("{WA_OK}")) //避让测试架
    // {
    //   SerialPortData = "";
    //   return;
    // }
    StaticJsonDocument<200> doc;                                       //声明一个JsonDocument对象
    DeserializationError error = deserializeJson(doc, SerialPortData); //反序列化JSON数据
    SerialPortData = "";
    if (error)
    {
      //Serial.println("error:deserializeJson");
      return;
    }
    else if (!error) //检查反序列化是否成功
    {
      int control_mode_N = doc["N"];
      char *temp = doc["H"];
      CommandSerialNumber = temp; //获取新命令的序号

      /*以下代码块请结合通讯协议V.docx 查看*/
      switch (control_mode_N)
      {
      case 1: /*<命令：N 1> 电机控制模式 */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_MotorControl;
        CMD_is_MotorSelection = doc["D1"];
        CMD_is_MotorSpeed = doc["D2"];
        CMD_is_MotorDirection = doc["D3"];

#if _is_print
        Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 2:                                                                    /*<命令：N 2> */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_CarControl_TimeLimit; /*小车方向控制：有时间限定模式*/
        CMD_is_CarDirection = doc["D1"];
        CMD_is_CarSpeed = doc["D2"];
        CMD_is_CarTimer = doc["T"];
        Application_ConquerorCarxxx0.CMD_CarControl_Millis = millis();
#if _is_print
        //Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 3:                                                                      /*<命令：N 3> */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_CarControl_NoTimeLimit; /*小车方向控制：无时间限定模式*/
        CMD_is_CarDirection = doc["D1"];
        CMD_is_CarSpeed = doc["D2"];
#if _is_print
        Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 4:                                                                  /*<命令：N 4> */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_MotorControl_Speed; /*电机控制:控制转速模式*/
        CMD_is_MotorSpeed_L = doc["D1"];
        CMD_is_MotorSpeed_R = doc["D2"];
#if _is_print
        Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 5:                                                            /*<命令：N 5> */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_ServoControl; /*编程控制舵机*/
        CMD_is_Servo = doc["D1"];
        CMD_is_Servo_angle = doc["D2"];
#if _is_print
        Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 7:                                                                         /*<命令：N 7> */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_LightingControl_TimeLimit; /*灯光控制:有时间限定模式*/

        CMD_is_LightingSequence = doc["D1"]; //Lighting (Left, front, right, back and center)
        CMD_is_LightingColorValue_R = doc["D2"];
        CMD_is_LightingColorValue_G = doc["D3"];
        CMD_is_LightingColorValue_B = doc["D4"];
        CMD_is_LightingTimer = doc["T"];
        Application_ConquerorCarxxx0.CMD_LightingControl_Millis = millis();
#if _is_print
        //Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 8:                                                                           /*<命令：N 8> */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_LightingControl_NoTimeLimit; /*灯光控制:无时间限定模式*/

        CMD_is_LightingSequence = doc["D1"]; //Lighting (Left, front, right, back and center)
        CMD_is_LightingColorValue_R = doc["D2"];
        CMD_is_LightingColorValue_G = doc["D3"];
        CMD_is_LightingColorValue_B = doc["D4"];
#if _is_print
        Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 21: /*<命令：N 21>：超声波模块:测距 */
        CMD_UltrasoundModuleStatus_xxx0(doc["D1"]);
#if _is_print
        //Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 22: /*<命令：N 22>：红外模块：寻迹 */
        CMD_TraceModuleStatus_xxx0(doc["D1"]);
#if _is_print
        //Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 23: /*<命令：N 23>：是否离开地面 */
        if (true == Car_LeaveTheGround)
        {
#if _is_print
          Serial.print('{' + CommandSerialNumber + "_false}");
#endif
        }
        else if (false == Car_LeaveTheGround)
        {
#if _is_print
          Serial.print('{' + CommandSerialNumber + "_true}");
#endif
        }
        break;

      case 110:                                                                                /*<命令：N 110> */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_ClearAllFunctions_Programming_mode; /*清除功能:进入编程模式*/

#if _is_print
        Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;
      case 100:                                                                            /*<命令：N 100> */
        Application_ConquerorCarxxx0.Functional_Mode = CMD_ClearAllFunctions_Standby_mode; /*清除功能：进入空闲模式*/
#if _is_print
        Serial.print("{ok}");
        //Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      case 101: /*<命令：N 101> :遥控切换命令*/
        if (1 == doc["D1"])
        {
          Application_ConquerorCarxxx0.Functional_Mode = TraceBased_mode;
        }
        else if (2 == doc["D1"])
        {
          Application_ConquerorCarxxx0.Functional_Mode = ObstacleAvoidance_mode;
        }
        else if (3 == doc["D1"])
        {
          Application_ConquerorCarxxx0.Functional_Mode = Follow_mode;
        }

#if _is_print
        // Serial.print('{' + CommandSerialNumber + "_ok}");
        Serial.print("{ok}");
#endif
        break;

      case 105: /*<命令：N 105> :FastLED亮度调节控制命令*/
        if (1 == doc["D1"] && (CMD_is_FastLED_setBrightness < 250))
        {
          CMD_is_FastLED_setBrightness += 5;
        }
        else if (2 == doc["D1"] && (CMD_is_FastLED_setBrightness > 0))
        {
          CMD_is_FastLED_setBrightness -= 5;
        }
        FastLED.setBrightness(CMD_is_FastLED_setBrightness);

#if _Test_print
        //Serial.print('{' + CommandSerialNumber + "_ok}");
        Serial.print("{ok}");
#endif
        break;

      case 106: /*<命令：N 106> :/*摇杆控制舵机*/
      {
        uint8_t temp_Set_Servo = doc["D1"];
        if (temp_Set_Servo > 5 || temp_Set_Servo < 1)
          return;
        ApplicationFunctionSet_Servo(temp_Set_Servo);
      }

#if _is_print
      //Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
      break;
      case 102: /*<命令：N 102> :摇杆控制命令*/
        Application_ConquerorCarxxx0.Functional_Mode = Rocker_mode;
        Rocker_temp = doc["D1"];
        Rocker_CarSpeed = doc["D2"];
        switch (Rocker_temp)
        {
        case 1:
          Application_ConquerorCarxxx0.Motion_Control = Forward;
          break;
        case 2:
          Application_ConquerorCarxxx0.Motion_Control = Backward;
          break;
        case 3:
          Application_ConquerorCarxxx0.Motion_Control = Left;
          break;
        case 4:
          Application_ConquerorCarxxx0.Motion_Control = Right;
          break;
        case 5:
          Application_ConquerorCarxxx0.Motion_Control = LeftForward;
          break;
        case 6:
          Application_ConquerorCarxxx0.Motion_Control = LeftBackward;
          break;
        case 7:
          Application_ConquerorCarxxx0.Motion_Control = RightForward;
          break;
        case 8:
          Application_ConquerorCarxxx0.Motion_Control = RightBackward;
          break;
        case 9:
          Application_ConquerorCarxxx0.Motion_Control = stop_it;
          break;
        default:
          Application_ConquerorCarxxx0.Motion_Control = stop_it;
          break;
        }
#if _is_print
        // Serial.print('{' + CommandSerialNumber + "_ok}");
#endif
        break;

      default:
        break;
      }
    }
  }
}
