#include "gpio.h"

#include <assert.h>


// 导出gpio引脚
int pinExport(int pin)
{
	char direction_path[50];
	snprintf(direction_path, sizeof(direction_path), "/sys/class/gpio/gpio%d/direction", pin);
	// 确认导出gpio引脚
	if (access(direction_path, F_OK) == -1) {
		FILE *export_file = fopen("/sys/class/gpio/export", "w");
		if (export_file == NULL) {
			perror("Failed to open GPIO export file");
			return -1;
		}
		fprintf(export_file, "%d", pin);
		fclose(export_file);
	}

	return 0;
}

// 设置输入/输出方向
int pinMode(int pin, int direction) {
	char direction_path[50];
	snprintf(direction_path, sizeof(direction_path), "/sys/class/gpio/gpio%d/direction", pin);
	assert(access(direction_path, F_OK) != -1);

	const char *direction_str = "in";
	if (direction == GPIO_OUT)
		direction_str = "out";
	FILE *direction_file = fopen(direction_path, "w");
	if (direction_file == NULL) {
		perror("Failed to open GPIO direction file");
		return -1;
	}
	fputs(direction_str, direction_file);
	fflush(direction_file);
	fclose(direction_file);

	return 0;
}