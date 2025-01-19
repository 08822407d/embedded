#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#include "gpio.h"


int SIM868_PWR_pin = 3;
int SIM868_WAKE_pin = 58;

int main() {
	// 导出sim868模组电源gpio和唤醒gpio
	if (pinExport(SIM868_PWR_pin) == -1) {
		perror("Failed to Export SIM868_PWR_pin\n");
		return -1;
	}
	if (pinExport(SIM868_WAKE_pin) == -1) {
		perror("Failed to Export SIM868_WAKE_pin\n");
		return -1;
	}
	// 设置引脚为输出
	if (pinMode(SIM868_PWR_pin, GPIO_OUT) == -1) {
		perror("Failed to set SIM868_WAKE_pin pinMode\n");
		return -1;
	}
	if (pinMode(SIM868_WAKE_pin, GPIO_OUT) == -1) {
		perror("Failed to set SIM868_WAKE_pin pinMode\n");
		return -1;
	}


	// FILE *unexport_file = fopen("/sys/class/gpio/unexport", "w");
	// if (unexport_file == NULL) {
	// 	perror("Failed to open GPIO unexport file");
	// 	return -1;
	// }
	// fprintf(unexport_file, "%d", gpio_pin);
	// fclose(unexport_file);

	return 0;
}