#pragma once

#include <DFRobot_GP8XXX.h>

#include "algo/waveform.hpp"



extern DFRobot_GP8XXX_IIC GP8413;


void sendToDac(void *arg);