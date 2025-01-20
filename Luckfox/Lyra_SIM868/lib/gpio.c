#include "gpio.h"


// 导出gpio引脚
int pinExport(uint16_t pin)
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
int pinMode(uint16_t pin, GPIO_DIR_e dir) {
	char direction_path[50];
	snprintf(direction_path, sizeof(direction_path), "/sys/class/gpio/gpio%d/direction", pin);
	assert(access(direction_path, F_OK) != -1);

	const char *direction_str = "in";
	if (dir == GPIO_OUT)
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

int pinWrite(uint16_t pin, DIGI_LEV_e level) {
	char value_path[50];
	// char cat_command[100];
	snprintf(value_path, sizeof(value_path), "/sys/class/gpio/gpio%d/value", pin);
	// snprintf(cat_command, sizeof(cat_command), "cat %s", value_path);
	FILE *value_file = fopen(value_path, "w");
	if (value_file == NULL) {
		perror("Failed to open GPIO value file");
		return -1;
	}
	if (level == DIGITAL_HIGH)
		fprintf(value_file, "1");
	else
		fprintf(value_file, "0");

	fflush(value_file);

	return 0;
}