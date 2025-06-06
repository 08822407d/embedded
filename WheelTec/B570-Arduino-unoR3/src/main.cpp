// /****************************************************************************
//    作者：平衡小车之家
//    产品名称：Minibalance For Arduino
// ****************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <MsTimer2.h>
#include <MPU6050_tockn.h>
#include <PinChangeInterrupt.h>
#include <EEPROM.h>


#define KEY				3	//按键引脚
#define IN1				12	//TB6612FNG驱动模块控制信号 共6个
#define IN2				13
#define IN3				7
#define IN4				6
#define PWMA			10
#define PWMB			9
#define ENCODER_L		2	//编码器采集引脚 每路2个 共4个
#define DIRECTION_L		5
#define ENCODER_R		4
#define DIRECTION_R		8
#define ZHONGZHI		2	//小车的机械中值  DIFFERENCE
#define DIFFERENCE		2



MPU6050		Mpu6050(Wire);				//实例化一个 MPU6050 对象，对象名称为 Mpu6050
int16_t		ax, ay, az, gx, gy, gz;		//MPU6050的三轴加速度和三轴陀螺仪数据
float		AngleX, AngleY, AngleZ;		//MPU6050的三轴角度
// int Balance_Pwm, Velocity_Pwm, Turn_Pwm;   //直立 速度 转向环的PWM
int			Motor1, Motor2;				//电机叠加之后的PWM
// float Battery_Voltage;   //电池电压 单位是V
volatile long	Velocity_L	= 0,
				Velocity_R	= 0;		//左右轮编码器数据
int			Velocity_Left	= 0,
			Velocity_Right	= 0;		//左右轮速度
int			Flag_Qian, Flag_Hou, Flag_Left, Flag_Right; //遥控相关变量
// int		Angle,
// 		Show_Data, PID_Send;	//用于显示的角度和临时变量
unsigned char Flag_Stop = 1,Send_Count,Flash_Send;  //停止标志位和上位机相关变量
float Balance_Kp=15,Balance_Kd=0.4,Velocity_Kp=2,Velocity_Ki=0.01;
// //***************下面是卡尔曼滤波相关变量***************//
// float	K1			= 0.05;		// 对加速度计取值的权重
// float	Q_angle		= 0.001,
// 		Q_gyro		= 0.005;
// float	R_angle		= 0.5,
// 		C_0			= 1;
// float	dt			= 0.005;	//注意：dt的取值为滤波器采样时间 5ms
// int		addr		= 0;


extern void READ_ENCODER_L();
extern void READ_ENCODER_R();


/**************************************************************************
函数功能：直立PD控制  作者：平衡小车之家
入口参数：角度、角速度
返回  值：直立控制PWM
**************************************************************************/
int balance(float Angle, float Gyro)
{
	float Bias;
	int balance;
	Bias = Angle - 0;   //===求出平衡的角度中值 和机械相关
	balance = Balance_Kp * Bias + Gyro * Balance_Kd; //===计算平衡控制的电机PWM  PD控制   kp是P系数 kd是D系数
	return balance;
}
/**************************************************************************
函数功能：速度PI控制 作者：平衡小车之家
入口参数：左轮编码器、右轮编码器
返回  值：速度控制PWM
**************************************************************************/
int velocity(int encoder_left, int encoder_right)
{
	static float Velocity, Encoder_Least, Encoder, Movement;
	static float Encoder_Integral, Target_Velocity;
	float kp = 2, ki = kp / 200;    //PI参数
	if       ( Flag_Qian == 1)Movement = 600;
	else   if ( Flag_Hou == 1)Movement = -600;
	else    //这里是停止的时候反转，让小车尽快停下来
	{
		Movement = 0;
		if (Encoder_Integral > 300)   Encoder_Integral -= 200;
		if (Encoder_Integral < -300)  Encoder_Integral += 200;
	}
	//=============速度PI控制器=======================//
	Encoder_Least = (encoder_left + encoder_right) - 0;               //===获取最新速度偏差==测量速度（左右编码器之和）-目标速度（此处为零）
	Encoder *= 0.7;                                                   //===一阶低通滤波器
	Encoder += Encoder_Least * 0.3;                                   //===一阶低通滤波器
	Encoder_Integral += Encoder;                                      //===积分出位移 积分时间：40ms
	Encoder_Integral = Encoder_Integral - Movement;                   //===接收遥控器数据，控制前进后退
	if (Encoder_Integral > 21000)    Encoder_Integral = 21000;        //===积分限幅
	if (Encoder_Integral < -21000) Encoder_Integral = -21000;         //===积分限幅
	Velocity = Encoder * Velocity_Kp + Encoder_Integral * Velocity_Ki;                  //===速度控制
	// if (Turn_Off(KalFilter.angle, Battery_Voltage) == 1 || Flag_Stop == 1)   
	// 	Encoder_Integral = 0;//小车停止的时候积分清零
	return Velocity;
}
/**************************************************************************
函数功能：转向控制 作者：平衡小车之家
入口参数：Z轴陀螺仪
返回  值：转向控制PWM
**************************************************************************/
int turn(float gyro)//转向控制
{
	static float Turn_Target, Turn, Turn_Convert = 3;
	float Turn_Amplitude = 80, Kp = 2, Kd = 0.001;  //PD参数
	if (1 == Flag_Left)             Turn_Target += Turn_Convert;  //根据遥控指令改变转向偏差
	else if (1 == Flag_Right)       Turn_Target -= Turn_Convert;//根据遥控指令改变转向偏差
	else Turn_Target = 0;
	if (Turn_Target > Turn_Amplitude)  Turn_Target = Turn_Amplitude; //===转向速度限幅
	if (Turn_Target < -Turn_Amplitude) Turn_Target = -Turn_Amplitude;
	Turn = -Turn_Target * Kp + gyro * Kd;         //===结合Z轴陀螺仪进行PD控制
	return Turn;
}
/**************************************************************************
函数功能：赋值给PWM寄存器 作者：平衡小车之家
入口参数：左轮PWM、右轮PWM
返回  值：无
**************************************************************************/
void Set_Pwm(int moto1, int moto2)
{
	if (moto1 > 0)     digitalWrite(IN1, HIGH),      digitalWrite(IN2, LOW);  //TB6612的电平控制
	else             digitalWrite(IN1, LOW),       digitalWrite(IN2, HIGH); //TB6612的电平控制
	analogWrite(PWMA, abs(moto1)); //赋值给PWM寄存器
	if (moto2 < 0) digitalWrite(IN3, HIGH),     digitalWrite(IN4, LOW); //TB6612的电平控制
	else        digitalWrite(IN3, LOW),      digitalWrite(IN4, HIGH); //TB6612的电平控制
	analogWrite(PWMB, abs(moto2));//赋值给PWM寄存器
}

/**************************************************************************
函数功能：限制PWM赋值  作者：平衡小车之家
入口参数：无
返回  值：无
**************************************************************************/
void Xianfu_Pwm(void)
{
	int Amplitude = 250;  //===PWM满幅是255 限制在250
	if(Flag_Qian==1)  Motor2-=DIFFERENCE;  //DIFFERENCE是一个衡量平衡小车电机和机械安装差异的一个变量。直接作用于输出，让小车具有更好的一致性。
	if(Flag_Hou==1)   Motor2-=DIFFERENCE-2;
	if (Motor1 < -Amplitude) Motor1 = -Amplitude;
	if (Motor1 > Amplitude)  Motor1 = Amplitude;
	if (Motor2 < -Amplitude) Motor2 = -Amplitude;
	if (Motor2 > Amplitude)  Motor2 = Amplitude;
}

/**************************************************************************
函数功能：外部中断读取编码器数据，具有二倍频功能 注意外部中断是跳变沿触发
入口参数：无
返回  值：无
**************************************************************************/
void READ_ENCODER_L() {
	if (digitalRead(ENCODER_L) == LOW) {	//如果是下降沿触发的中断
		if (digitalRead(DIRECTION_L) == LOW)
			Velocity_L--;	//根据另外一相电平判定方向
		else
			Velocity_L++;
	} else {								//如果是上升沿触发的中断
		if (digitalRead(DIRECTION_L) == LOW)
			Velocity_L++;	//根据另外一相电平判定方向
		else
			Velocity_L--;
	}
}

/**************************************************************************
函数功能：外部中断读取编码器数据，具有二倍频功能 注意外部中断是跳变沿触发
入口参数：无
返回  值：无
**************************************************************************/
// ISR(PCINT2_vect) {
void READ_ENCODER_R() {
	Serial.println("R");
	if (digitalRead(ENCODER_R) == LOW) {	//如果是下降沿触发的中断
		if (digitalRead(DIRECTION_R) == LOW)
			Velocity_R--;	//根据另外一相电平判定方向
		else
			Velocity_R++;
	} else {								//如果是上升沿触发的中断
		if (digitalRead(DIRECTION_R) == LOW)
			Velocity_R++;	//根据另外一相电平判定方向
		else
			Velocity_R--;
	}
}
/**************************************************************************
函数功能：5ms控制函数 核心代码 作者：平衡小车之家
入口参数：无
返回  值：无
**************************************************************************/
void control()
{
	sei();//全局中断开启
	// Serial.print(".");

	Mpu6050.update();				//更新MPU6050数据
	ax = Mpu6050.getAccX();			//获取MPU6050的加速度计数据
	ay = Mpu6050.getAccY();
	az = Mpu6050.getAccZ();			//获取MPU6050的加速度计数据
	gx = Mpu6050.getGyroX();		//获取MPU6050的陀螺仪数据
	gy = Mpu6050.getGyroY();
	gz = Mpu6050.getGyroZ();		//获取MPU6050的陀螺仪数据
	AngleX = Mpu6050.getAngleX();
	AngleY = Mpu6050.getAngleY();
	AngleZ = Mpu6050.getAngleZ();

	// KalFilter.Angletest(ax, ay, az, gx, gy, gz, dt, Q_angle, Q_gyro, R_angle, C_0, K1);          //通过卡尔曼滤波获取角度
	// Angle = KalFilter.angle;//Angle是一个用于显示的整形变量
	// Balance_Pwm = balance(KalFilter.angle + ZHONGZHI , KalFilter.Gyro_x);//直立PD控制 控制周期5ms
}

/**************************************************************************
函数功能：初始化 相当于STM32里面的Main函数 作者：平衡小车之家
入口参数：无
返回  值：无
**************************************************************************/
void setup() {
	Serial.begin(9600);				//开启串口，设置波特率为 9600

	Serial.println("Initiating Pins ...");
	pinMode(IN1, OUTPUT);		//TB6612控制引脚，控制电机1的方向，01为正转，10为反转
	pinMode(IN2, OUTPUT);		//TB6612控制引脚，
	pinMode(IN3, OUTPUT);		//TB6612控制引脚，控制电机2的方向，01为正转，10为反转
	pinMode(IN4, OUTPUT);		//TB6612控制引脚，
	pinMode(PWMA, OUTPUT);		//TB6612控制引脚，电机PWM
	pinMode(PWMB, OUTPUT);		//TB6612控制引脚，电机PWM
	digitalWrite(IN1, 0);		//TB6612控制引脚拉低
	digitalWrite(IN2, 0);		//TB6612控制引脚拉低
	digitalWrite(IN3, 0);		//TB6612控制引脚拉低
	digitalWrite(IN4, 0);		//TB6612控制引脚拉低
	analogWrite(PWMA, 0);		//TB6612控制引脚拉低
	analogWrite(PWMB, 0);		//TB6612控制引脚拉低
	pinMode(ENCODER_L, INPUT);			//编码器引脚
	pinMode(ENCODER_R, INPUT_PULLUP);	//编码器引脚
	pinMode(DIRECTION_L, INPUT);		//编码器引脚
	pinMode(DIRECTION_R, INPUT_PULLUP);	//编码器引脚
	pinMode(KEY, INPUT);		//按键引脚

	Serial.println("Initiating I2C ...");
	Wire.begin();             //加入 IIC 总线
	delay(400);              //延时等待初始化完成

	// Serial.println("Initiating IMU ...");
	// Mpu6050.begin();					//初始化MPU6050
	// Mpu6050.calcGyroOffsets(false);		// 自动校准陀螺仪偏移量
	// delay(200); 

	Serial.println("Starting timer Intr ...");
	MsTimer2::set(5, control);  //使用Timer2设置5ms定时中断
	MsTimer2::start();          //使用中断使能
	delay(200);              //延时等待初始化完成

	Serial.println("Starting Encoder Intr ...");
	attachInterrupt(0, READ_ENCODER_L, CHANGE);           //开启外部中断 编码器接口1
	attachPinChangeInterrupt(ENCODER_R, READ_ENCODER_R, CHANGE);  //开启外部中断 编码器接口2
	// PCICR |= (1 << PCIE2);		// 启用端口D（引脚0-7）的中断
	// PCMSK2 |= (1 << PCINT20);	// 启用引脚4（PD4）的
	delay(200);				//延时等待初始化完成

	Serial.println("Initiation Finished.");
}
/**************************************************************************
函数功能：主循环程序体
入口参数：无
返回  值：无
**************************************************************************/
void loop() {
	// Serial.println("Angle: " + String(KalFilter.angle) + "  Gyro: " + String(KalFilter.Gyro_x) + "  Voltage: " + String(Battery_Voltage));
	// Serial.println("Velocity Left: " + String(Velocity_Left) + "  Velocity Right: " + String(Velocity_Right));
	Serial.println("AngleX: " + String(AngleX) + "; Velocity_L: " +
		String(Velocity_L) + "; Velocity_R: " + String(Velocity_R) + ";");
	// Serial.print("*");
	delay(50);
}
