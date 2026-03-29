#include <stdio.h>

#include <esp_log.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "libdow.h"
#include "sht40.h"

#define ENABLE_DOW

static const char *LOGTAG __attribute__((unused)) = "WSDEMO";

i2c_master_bus_handle_t bus_handle;

sht40_sensor_t sensor1;

void monitor_heap() {
    multi_heap_info_t heap_info;
    heap_caps_get_info(&heap_info, MALLOC_CAP_DEFAULT);
    ESP_LOGI(LOGTAG,
			"HeapInfo aby %d fby %d mfby %d abk %d fbk %d tbk %d lfbk %d",
			heap_info.total_allocated_bytes,
			heap_info.total_free_bytes,
			heap_info.minimum_free_bytes,
			heap_info.allocated_blocks,
			heap_info.free_blocks,
			heap_info.total_blocks,
			heap_info.largest_free_block
			);
}

esp_err_t
initialize_i2c( int sda, int scl )
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
	return ESP_OK;
}

void app_main(void)
{
	int last_heap = 0;
	ESP_LOGI(LOGTAG, "Hi there");
#ifdef ENABLE_DOW
	dow_bus_t bus;
	dowBusInit(&bus, GPIO_NUM_6, -1);
#endif
	initialize_i2c(4, 5);
	sht40_init(&bus_handle, 0x44, &sensor1);
	sht40_id_string_t id_string;
	sht40_get_id(&sensor1, id_string);
	ESP_LOGI(LOGTAG, "SHT40 sensor %s", id_string);
	for (; ; ) {
		ESP_LOGI(LOGTAG, "Loop");
		float temp, rh;
		if (sht40_convert(&sensor1, &temp, &rh)) {
			ESP_LOGI(LOGTAG, "Temp %f RH %f", (double)temp, (double)rh);
		}
#ifdef ENABLE_DOW
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
		vTaskDelay(pdMS_TO_TICKS(5000));
		int now = esp_timer_get_time() / 1000000;
		if (now - last_heap >= 60) {
			last_heap = now;
			monitor_heap();
		}
	}
}

