#include <stdio.h>

#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "libdow.h"

static const char *LOGTAG __attribute__((unused)) = "WSDEMO";

void app_main(void)
{
	ESP_LOGI(LOGTAG, "Hi there");
	dow_bus_t bus;
	dowBusInit(&bus, GPIO_NUM_6, -1);
	for (; ; ) {
		ESP_LOGI(LOGTAG, "Loop");
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
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

