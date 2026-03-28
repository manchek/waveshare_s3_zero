#include <stdio.h>

#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "libdow.h"

static const char *LOGTAG __attribute__((unused)) = "WSDEMO";

esp_err_t
sht40_init( int sda, int scl )
{
	i2c_config_t i2cc = {0};
	i2cc.mode = I2C_MODE_MASTER;
	i2cc.sda_io_num = sda;
	i2cc.scl_io_num = scl;
	i2cc.master.clk_speed = 100000;

	esp_err_t rc = i2c_param_config(I2C_NUM_0, &i2cc);
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "i2c config failed: %s", esp_err_to_name(rc));
		return rc;
	}

	rc = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "i2c driver install failed: %s", esp_err_to_name(rc));
		return rc;
	}
	return ESP_OK;
}

esp_err_t
sht40_read_id()
{
	uint8_t wbuf[2];
	uint8_t buf[6] = {0};

	wbuf[0] = 0x89;
	ESP_LOGI(LOGTAG, "read device id");
	esp_err_t rc = i2c_master_write_to_device(I2C_NUM_0, 0x44, wbuf, 1, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "i2c mw failed: %s", esp_err_to_name(rc));
		return rc;
	}
	vTaskDelay(pdMS_TO_TICKS(10));
	rc = i2c_master_read_from_device(I2C_NUM_0, 0x44, buf, 6, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "i2c mr failed: %s", esp_err_to_name(rc));
		return rc;
	}
	ESP_LOGI(LOGTAG, "SHT40 %02x:%02x:%02x:%02x:%02x:%02x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	return ESP_OK;
}

void app_main(void)
{
	ESP_LOGI(LOGTAG, "Hi there");
#if 0
	dow_bus_t bus;
	dowBusInit(&bus, GPIO_NUM_6, -1);
#endif
	sht40_init(4, 5);
	for (; ; ) {
		ESP_LOGI(LOGTAG, "Loop");
		sht40_read_id(4, 5);
#if 0
		dow_rom_t rom;
		dow_rom_search_t srch;
		dowRomSearchClear(&srch);
		while (dowRomSearchNext(&bus, &srch, &rom)) {
			dow_rom_string_t s;
			dowRomString(&rom, s);
			const char *q = s;
			//ESP_LOGI(LOGTAG, "DOW ROM %s", q);
			ds1820ConvertAndWait(&bus, &rom);
			float temp;
			if (ds1820ReadTemp(&bus, &rom, &temp))
				ESP_LOGI(LOGTAG, "ROM %s Temperature %.1f C %.1f F", q, (double)temp, (double)temp * 1.8 + 32);
			else
				ESP_LOGI(LOGTAG, "ROM %s Temperature read failed", q);
		}
#endif
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

