#pragma once


	class GenericI2C {
	public:
		virtual GenericI2C();
		virtual ~GenericI2C();

        virtual bool begin() = 0;
        virtual void end() = 0;

	private:
		uint8_t		_addr;
		uint8_t		_scl;
		uint8_t		_sda;
		uint32_t	_speed;
	};