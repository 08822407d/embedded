#include "serial.h"


int openSerial(int port) {
	int serial_port_num;
	int serial_fd;
	char serial_port[32];

	sprintf(serial_port, "/dev/ttyS%d", serial_port_num);
	if (serial_fd == -1) {
		perror("Failed to open serial port");
	}
	return serial_fd;
}

int configSerial(int serial_fd) {
	struct termios tty;
	memset(&tty, 0, sizeof(tty));

	if (tcgetattr(serial_fd, &tty) != 0) {
		perror("Error from tcgetattr");
		return 1;
	}

	cfsetospeed(&tty, B9600);
	cfsetispeed(&tty, B9600);

	tty.c_cflag &= ~PARENB;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;

	if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
		perror("Error from tcsetattr");
		return -1;
	}

	return 0;
}