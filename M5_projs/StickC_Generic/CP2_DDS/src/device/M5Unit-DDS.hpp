#pragma once

#include <Unit_DDS.h>


#define DDS_GPIO_SDA		32
#define DDS_GPIO_SCL		33

extern Unit_DDS::DDSmode WaveFormMap[4];

	class M5UnitDDS {
	public:
		uint8_t		_addr;
		TwoWire		*_wire;
		uint8_t		_scl;
		uint8_t		_sda;
		uint32_t	_speed;

		Unit_DDS	_dds;

		M5UnitDDS() {};
		// virtual ~M5UnitDDS() {}

		bool start();
		bool end();

		/**
		 * @brief Unit Joystick2 init
		 * @param wire I2C Wire number
		 * @param addr I2C address
		 * @param sda I2C SDA Pin
		 * @param scl I2C SCL Pin
		 * @param speed I2C clock
		 * @return 1 success, 0 false
		 */
		bool begin(TwoWire *wire = &Wire, uint8_t addr = DDS_UNIT_I2CADDR,
				uint8_t sda = DDS_GPIO_SDA, uint8_t scl = DDS_GPIO_SCL,
				uint32_t speed = 400000UL);

		void setWave(uint32_t WaveForm, uint32_t Frequency, uint32_t Phase);
	};