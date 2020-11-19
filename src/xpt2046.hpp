#pragma once

#include <stdint.h>
#include <modm/ui/display/graphic_display.hpp>
#include <modm/architecture/interface/delay.hpp>

template <typename Spi, typename Cs, typename Int>
class Xpt2046
{
public:
	static void
	initialize();

	/**
	 * Get the smoothed (x,y) position.
	 *
	 * @param	point
	 * 		(x,y) position on the pressed touchscreen
	 *
	 * @return	`true` if the touchscreen is pressed and the value is
	 * 			stable enough to provide a reading, otherwise `false`.
	 */
	static bool
	read(modm::glcd::Point * point, uint16_t pressure_threshold=defaultThreshold);

	static inline uint16_t
	readX()
	{
		return readData(CHX);
	}

	static inline uint16_t
	readY()
	{
		return readData(CHY);
	}

	static inline uint16_t
	readZ1()
	{
		return readData(CHZ1);
	}

	static inline uint16_t
	readZ2()
	{
		return readData(CHZ2);
	}

	static inline uint16_t
	readPressure()
	{
		uint16_t z1 = readZ1();
		uint16_t z2 = readZ2();
		return z1 + 4095 - z2;
	}

private:
	static const uint8_t CHX = 0x90;
	static const uint8_t CHY = 0xd0;
	static const uint8_t CHZ1 = 0xb0;
	static const uint8_t CHZ2 = 0xc0;

	static const uint16_t defaultThreshold = 72;

	static uint16_t
	getBestTwo(uint16_t *temp);

	static uint16_t
	readData(uint8_t command);
};

template <typename Spi, typename Cs, typename Int>
void
Xpt2046<Spi, Cs, Int>::initialize()
{

}

template <typename Spi, typename Cs, typename Int>
uint16_t
Xpt2046<Spi, Cs, Int>::getBestTwo(uint16_t *buf)
{
	uint16_t value;
	uint16_t m0 = abs(buf[0] - buf[1]);
	uint16_t m1 = abs(buf[1] - buf[2]);
	uint16_t m2 = abs(buf[2] - buf[0]);

	if (m0 < m1)
	{
		if (m2 < m0) {
			value = (buf[0] + buf[2]) / 2;
		}
		else {
			value = (buf[0] + buf[1]) / 2;
		}
	}
	else if (m2 < m1) {
		value = (buf[0] + buf[2]) / 2;
	}
	else {
		value = (buf[1] + buf[2]) / 2;
	}

	return value;
}

template <typename Spi, typename Cs, typename Int>
bool
Xpt2046<Spi, Cs, Int>::read(modm::glcd::Point * point, uint16_t pressure_threshold)
{
	uint16_t z = readPressure();
	uint16_t xbuf[3];
	uint16_t ybuf[3];

	if(z > pressure_threshold) {
		xbuf[0] = readData(CHX);
		ybuf[0] = readData(CHY);
		xbuf[1] = readData(CHX);
		ybuf[1] = readData(CHY);
		xbuf[2] = readData(CHX);
		ybuf[2] = readData(CHY);

		point->x = (int16_t)getBestTwo(xbuf);
		point->y = (int16_t)getBestTwo(ybuf);
		return true;
	} else {
		return false;
	}
}

template <typename Spi, typename Cs, typename Int>
uint16_t
Xpt2046<Spi, Cs, Int>::readData(uint8_t command)
{
	Cs::reset();
	modm::delay_us(1);
	Spi::transferBlocking(command);
	modm::delay_us(1);

	uint16_t temp = Spi::transferBlocking(0x00);
	temp <<= 8;
	modm::delay_us(1);

	temp |= Spi::transferBlocking(0x00);
	temp >>= 3;

	Cs::set();

	return (temp & 0xfff);
}
