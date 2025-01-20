#include "serial.h"


int openSerial(int port) {
	int serial_fd;
	char serial_port[32];

	sprintf(serial_port, "/dev/ttyS%d", port);
	serial_fd = open(serial_port, O_RDWR | O_NOCTTY);
	if (serial_fd == -1) {
		perror("Failed to open serial port\n");
	}
	return serial_fd;
}

int configSerial(int serial_fd) {
	struct termios tty;
	memset(&tty, 0, sizeof(tty));

	if (tcgetattr(serial_fd, &tty) != 0) {
		perror("Error from tcgetattr\n");
		return 1;
	}

	cfsetospeed(&tty, B1152000);
	cfsetispeed(&tty, B1152000);

	tty.c_cflag &= ~PARENB;			// 禁用奇偶校验
	tty.c_cflag &= ~CSTOPB;			// 使用1位停止位
	tty.c_cflag &= ~CSIZE;			// 清除数据位设置
	tty.c_cflag |= CS8;				// 设置8位数据位
	// tty.c_cflag &= ~CRTSCTS;		// 禁用硬件流控制
	tty.c_cflag |= CREAD | CLOCAL;	// 启用接收和本地模式

	if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
		perror("Error from tcsetattr");
		return -1;
	}

	return 0;
}

int readSerial(int serial_fd, char *rx_buffer, int bufflen) {
	int bytes_read = read(serial_fd, rx_buffer, bufflen);
	assert(bytes_read < bufflen);
	if (bytes_read > 0) {
		rx_buffer[bytes_read] = '\0';
		printf("rx_buffer: %s \n", rx_buffer);
	} else {
		printf("No data received.\n");
	}
	return bytes_read;
}

int writeSerial(int serial_fd, char *tx_buffer, int bufflen) {
	ssize_t bytes_written = write(serial_fd, tx_buffer, bufflen);
	assert(bytes_written < bufflen);
	if (bytes_written < 0) {
		perror("Error writing to serial port\n");
		// close(serial_fd);
		return -1;
	}
	return bytes_written;
}