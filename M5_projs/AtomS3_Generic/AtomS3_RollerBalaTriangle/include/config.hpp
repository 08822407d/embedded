#pragma once

#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_


	#define IMU_SAMPLE_FREQ		200
	#define IMU_SAMPLE_PERIOD	(1000 / (IMU_SAMPLE_FREQ))
	#define STACK_SIZE			4096	// 任务栈大小 (字数, 每字约4字节)

#endif /* _CONFIG_HPP_ */