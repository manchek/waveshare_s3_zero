#include <stdio.h>

#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "libdow.h"

static const char *LOGTAG __attribute__((unused)) = "WSDEMO";

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

esp_err_t
sht40_init( int sda, int scl )
{
	i2c_master_bus_config_t bus_config = {
		.i2c_port = I2C_NUM_0,
		.sda_io_num = sda,
		.scl_io_num = scl,
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
	};
	ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

	i2c_device_config_t dev_config = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = 0x44,
		.scl_speed_hz = 100000,
	};
	ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

	return ESP_OK;
}

esp_err_t
sht40_read_id()
{
	uint8_t wbuf[2];
	uint8_t buf[6] = {0};

	wbuf[0] = 0x89;
	ESP_LOGI(LOGTAG, "read device id");
	esp_err_t rc = i2c_master_transmit(dev_handle, wbuf, 1, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "i2c mw failed: %s", esp_err_to_name(rc));
		return rc;
	}
	vTaskDelay(pdMS_TO_TICKS(10));
	rc = i2c_master_receive(dev_handle, buf, 6, pdMS_TO_TICKS(500));
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

