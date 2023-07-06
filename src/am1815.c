#include <am1815.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

uint8_t am1815_read_register(struct am1815 *rtc, uint8_t addr)
{
	uint32_t buffer;
	spi_read(rtc->spi, addr, &buffer, 1);
	return (uint8_t)buffer;
}

void am1815_write_register(struct am1815 *rtc, uint8_t addr, uint8_t data)
{
	uint32_t buffer = data;
	spi_write(rtc->spi, addr, &buffer, 1);
}

static uint8_t from_bcd(uint8_t bcd)
{
	return (bcd & 0xF) + ((bcd >> 4) * 10);
}

struct timeval am1815_read_time(struct am1815 *rtc)
{
	struct spi *spi = rtc->spi;
	uint32_t buffer[2];
	uint8_t *data = (uint8_t*)buffer;
	spi_read(spi, 0x0, buffer, 8);
	memcpy(data, buffer, 8);

	struct tm date = {
		.tm_year = from_bcd(data[6]) + 100,
		.tm_mon = from_bcd(data[5] & 0x1F) - 1,
		.tm_mday = from_bcd(data[4] & 0x3F),
		.tm_hour = from_bcd(data[3] & 0x3F),
		.tm_min = from_bcd(data[2] & 0x7F),
		.tm_sec = from_bcd(data[1] & 0x7F),
		.tm_wday = from_bcd(data[7] & 0x07),
	};

	time_t time = mktime(&date);

	struct timeval result = {
		.tv_sec = time,
		.tv_usec = from_bcd(data[0]) * 10000,
	};
	return result;
}

void am1815_enable_trickle(struct am1815 *rtc)
{
	am1815_write_register(rtc, 0x1F, 0x9D);
	am1815_write_register(rtc, 0x20, 0xA5);
}

void am1815_disable_trickle(struct am1815 *rtc)
{
	am1815_write_register(rtc, 0x1F, 0x9D);
	am1815_write_register(rtc, 0x20, 0x00);
}

void am1815_init(struct am1815 *rtc, struct spi *spi)
{
	rtc->spi = spi;
	//am1815_enable_trickle(rtc);
}
