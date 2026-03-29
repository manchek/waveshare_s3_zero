
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "sht40.h"

#define READ_SERIAL            0x89
#define SOFT_RESET             0x94
#define MEASURE_HIGH_PRECISION 0xFD

static const char *LOGTAG __attribute__((unused)) = "SHT40";

#define CRCPOLY 0x31

static void
crc_byte(
	uint8_t *accum,
	uint8_t b )
{
	uint8_t i;
	uint8_t k;

	for (k = 0; k < 8; k++) {
		i = *accum ^ b;
		*accum <<= 1;
		if (i & 0x80)
			*accum ^= CRCPOLY;
		b <<= 1;
	}
}

static bool
check_crc(
	uint8_t *buf /*6*/ )
{
	uint8_t accum = 0xff;
	crc_byte(&accum, buf[0]);
	crc_byte(&accum, buf[1]);
	crc_byte(&accum, buf[2]);
	if (accum != 0)
		return false;
	accum = 0xff;
	crc_byte(&accum, buf[3]);
	crc_byte(&accum, buf[4]);
	crc_byte(&accum, buf[5]);
	if (accum != 0)
		return false;
	return true;
}

static bool
send_recv( i2c_master_dev_handle_t *dev, uint8_t cmd, uint8_t *result /*6*/ )
{
	esp_err_t rc = i2c_master_transmit(*dev, &cmd, 1, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "i2c mw failed: %s", esp_err_to_name(rc));
		return rc;
	}
	vTaskDelay(pdMS_TO_TICKS(10));
	rc = i2c_master_receive(*dev, result, 6, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "i2c mr failed: %s", esp_err_to_name(rc));
		return rc;
	}
	if (!check_crc(result)) {
		ESP_LOGE(LOGTAG, "i2c read CRC error");
		return ESP_FAIL;
	}
	return ESP_OK;
}

bool
sht40_get_id( sht40_sensor_t *sensor, sht40_id_string_t id_string )
{
	if (sensor->present) {
		snprintf(id_string, 13, "%02x%02x%02x%02x%02x%02x",
				sensor->id[0], sensor->id[1], sensor->id[2],
				sensor->id[3], sensor->id[4], sensor->id[5]);
		return true;
	}
	id_string[0] = 0;
	return false;
}

bool
sht40_init( i2c_master_bus_handle_t *bus, uint8_t address, sht40_sensor_t *sensor )
{
	i2c_device_config_t dev_config = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = address,
		.scl_speed_hz = 100000,
	};
	esp_err_t rc = i2c_master_bus_add_device(*bus, &dev_config, &sensor->dev_handle);
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "Can't add i2c device: %s", esp_err_to_name(rc));
		return false;
	}

	rc = send_recv(&sensor->dev_handle, READ_SERIAL, sensor->id);
	if (rc != ESP_OK) {
		ESP_LOGW(LOGTAG, "Failed to read sensor id: %s", esp_err_to_name(rc));
		return false;
	}
	
	sensor->present = true;
	return true;
}

bool
sht40_convert( sht40_sensor_t *sensor, float *temp, float *rh )
{
	if (!sensor->present)
		return false;
	uint8_t cmd = SOFT_RESET;
	esp_err_t rc = i2c_master_transmit(sensor->dev_handle, &cmd, 1, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "Sensor reset failed: %s", esp_err_to_name(rc));
		return false;
	}
	vTaskDelay(pdMS_TO_TICKS(10));
	uint8_t rxbuf[6];
	rc = send_recv(&sensor->dev_handle, MEASURE_HIGH_PRECISION, rxbuf);
	if (rc != ESP_OK) {
		ESP_LOGW(LOGTAG, "Failed to read sensor: %s", esp_err_to_name(rc));
		return false;
	}
	int tt = rxbuf[0] * 256 + rxbuf[1];
	int rht = rxbuf[3] * 256 + rxbuf[4];
	*temp = (175 * tt) / 65535. - 45;
	*rh = (125 * rht) / 65535. - 6;
	return true;
}

