#include "M5Unit-DDS.hpp"


Unit_DDS::DDSmode WaveFormMap[4] = {
	Unit_DDS::kSINUSMode,
	Unit_DDS::kTRIANGLEMode,
	Unit_DDS::kSQUAREMode,
	Unit_DDS::kSAWTOOTHMode
};

bool M5UnitDDS::begin(TwoWire *wire, uint8_t addr, uint8_t sda, uint8_t scl, uint32_t speed)
{
	this->_wire  = wire;
	this->_addr  = addr;
	this->_sda   = sda;
	this->_scl   = scl;
	this->_speed = speed;

	return this->start();
}

bool M5UnitDDS::start() {
	this->_wire->begin(this->_sda, this->_scl, this->_speed);
	this->_wire->setClock(this->_speed);
    delay(10);
	this->_dds.begin(this->_wire);

	return true;
}

bool M5UnitDDS::end() {
	this->_wire->end();
	return true;
}

void M5UnitDDS::setWave(uint32_t WaveForm, uint32_t Frequency, uint32_t Phase) {
	// SAWTOOTH WAVE Only support 13.6Khz
	if (WaveForm == 3)
		Frequency = 13600;

	this->_dds.quickOUT(WaveFormMap[WaveForm], Frequency, Phase);
}