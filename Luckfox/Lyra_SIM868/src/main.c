#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

#include "module.h"


int main() {

	genericInit();
	genericNetInit();

	return 0;
}