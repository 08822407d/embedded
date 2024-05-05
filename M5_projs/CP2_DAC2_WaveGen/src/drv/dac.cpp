#include "dac.hpp"


DFRobot_GP8XXX_IIC GP8413(RESOLUTION_15_BIT, 0x59, &Wire);

void sendToDac(void *arg) {
	WaveForm *wave = (WaveForm *)arg;
	GP8413.setDACOutVoltage(wave->GetDacVal(), 0);
}