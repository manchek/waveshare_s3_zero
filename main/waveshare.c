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
#define ENABLE_SHT40
//#define ENABLE_SI7021

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

#ifdef ENABLE_SI7021

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

typedef struct si7021 {
	i2c_master_dev_handle_t dev_handle;
} si7021_t;

void
si7021_init( i2c_master_bus_handle_t *bus, int addr, si7021_t *sensor )
{
	i2c_device_config_t dev_config = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = addr,
		.scl_speed_hz = 100000,
	};
	esp_err_t rc = i2c_master_bus_add_device(*bus, &dev_config, &sensor->dev_handle);
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "Can't add i2c device: %s", esp_err_to_name(rc));
	}
}

void
si7021_write_cmd( si7021_t *dev, uint8_t value )
{
	esp_err_t rc = i2c_master_transmit(dev->dev_handle, &value, 1, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "si7021_write_reg: %s", esp_err_to_name(rc));
	}
}

void
si7021_read( si7021_t *dev, int len, uint8_t *buffer )
{
	esp_err_t rc = i2c_master_receive(dev->dev_handle, buffer, len, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "si7021_read: %s", esp_err_to_name(rc));
	}
}

void
si7021_check( si7021_t *dev )
{
	uint8_t wbuf[2];
	uint8_t buf[8];
	wbuf[0] = 0xfa;
	wbuf[1] = 0x0f;
	esp_err_t rc = i2c_master_transmit_receive(dev->dev_handle, wbuf, 2, buf, 8, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "si7021_check: %s", esp_err_to_name(rc));
	}
	uint8_t accum = 0;
	crc_byte(&accum, buf[0]);
	crc_byte(&accum, buf[2]);
	crc_byte(&accum, buf[4]);
	crc_byte(&accum, buf[6]);
	crc_byte(&accum, buf[7]);
	ESP_LOGI(LOGTAG, "Si7021: 0xfa0f: %02x %02x %02x %02x %02x %02x %02x %02x : %02x",
			buf[0], buf[1], buf[2], buf[3],
			buf[4], buf[5], buf[6], buf[7], accum);
	wbuf[0] = 0xfc;
	wbuf[1] = 0xc9;
	rc = i2c_master_transmit_receive(dev->dev_handle, wbuf, 2, buf, 6, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "si7021_check: %s", esp_err_to_name(rc));
	}
	accum = 0;
	crc_byte(&accum, buf[0]);
	crc_byte(&accum, buf[1]);
	crc_byte(&accum, buf[3]);
	crc_byte(&accum, buf[4]);
	crc_byte(&accum, buf[5]);
	ESP_LOGI(LOGTAG, "Si7021: 0xfcc9: %02x %02x %02x %02x %02x %02x : %02x",
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], accum);

/*
	wbuf[0] = 0xf3;
	rc = i2c_master_transmit_receive(dev->dev_handle, wbuf, 1, buf, 3, pdMS_TO_TICKS(500));
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "si7021_check: %s", esp_err_to_name(rc));
	}
	ESP_LOGI(LOGTAG, "Si7021: %02x %02x %02x", buf[0], buf[1], buf[2]);
*/
/*
	si7021_write_cmd(dev, 0xf5);
	vTaskDelay(pdMS_TO_TICKS(10));
	si7021_read(dev, 3, buf);
	ESP_LOGI(LOGTAG, "Si7021: %02x %02x %02x",
			buf[0], buf[1], buf[2]);
*/
}
#endif /*ENABLE_SI7021*/

typedef struct hdc2080 {
	i2c_master_dev_handle_t dev_handle;
} hdc2080_t;

void
hdc2080_init( i2c_master_bus_handle_t *bus, int addr, hdc2080_t *sensor )
{
	i2c_device_config_t dev_config = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = addr,
		.scl_speed_hz = 100000,
	};
	esp_err_t rc = i2c_master_bus_add_device(*bus, &dev_config, &sensor->dev_handle);
	if (rc != ESP_OK) {
		ESP_LOGE(LOGTAG, "Can't add i2c device: %s", esp_err_to_name(rc));
	}
}

esp_err_t
hdc2080_read_regs( hdc2080_t *dev, int addr, int len, uint8_t *buffer )
{
	uint8_t wbuf = addr;
	esp_err_t rc = i2c_master_transmit_receive(dev->dev_handle, &wbuf, 1, buffer, len, pdMS_TO_TICKS(500));
	if (rc != ESP_OK)
		ESP_LOGE(LOGTAG, "hdc2080_read_regs %02x[%d]: %s", addr, len, esp_err_to_name(rc));
	return rc;
}

esp_err_t
hdc2080_write_reg( hdc2080_t *dev, int addr, uint8_t value )
{
	uint8_t wbuf[2];
	wbuf[0] = addr;
	wbuf[1] = value;
	esp_err_t rc = i2c_master_transmit(dev->dev_handle, wbuf, 2, pdMS_TO_TICKS(500));
	if (rc != ESP_OK)
		ESP_LOGE(LOGTAG, "hdc2080_write_reg %02x=%02x: %s", addr, value, esp_err_to_name(rc));
	return rc;
}

void
hdc2080_check( hdc2080_t *dev, bool heat )
{
	uint8_t buf[8];
	esp_err_t rc;
	if (hdc2080_read_regs(dev, 0xfc, 4, buf) == ESP_OK)
		ESP_LOGI(LOGTAG, "HDC2080: 0xfc ID: %02x %02x %02x %02x", buf[0], buf[1], buf[2], buf[3]);
	hdc2080_write_reg(dev, 0x0e, heat ? 0x08 : 0x80);
	vTaskDelay(pdMS_TO_TICKS(100));
	//while ((rc = hdc2080_read_regs(dev, 0x0e, 1, buf)) != ESP_OK) {
		//vTaskDelay(pdMS_TO_TICKS(100));
	//}
	//ESP_LOGI(LOGTAG, "HDC2080: 0e=%02x", buf[0]);
	while ((rc = hdc2080_write_reg(dev, 0x0f, 0x01)) != ESP_OK) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	vTaskDelay(pdMS_TO_TICKS(10));
	hdc2080_read_regs(dev, 0x00, 4, buf);
	double temp = ((int)buf[1] << 8 | buf[0]) * 165. / 65536. - 40.5;
	double rh = ((int)buf[3] << 8 | buf[2]) * 100. / 65536.;
	ESP_LOGI(LOGTAG, "HDC2080: %02x %02x %02x %02x T %.2f RH %.2f",
			buf[0], buf[1], buf[2], buf[3], temp, rh);
}

void app_main(void)
{
	int last_heap = 0;
	ESP_LOGI(LOGTAG, "Hi there");

	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = BIT64(3);
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	esp_err_t rc = gpio_config(&io_conf);
	if (rc != ESP_OK)
		ESP_LOGE(LOGTAG, "Configure button pin: %s", esp_err_to_name(rc));

#ifdef ENABLE_DOW
	dow_bus_t bus;
	dowBusInit(&bus, GPIO_NUM_6, -1);
#endif
	initialize_i2c(4, 5);
	hdc2080_t hdc;
	hdc2080_init(&bus_handle, 0x41, &hdc);
#ifdef ENABLE_SI7021
	si7021_t si;
	si7021_init(&bus_handle, 0x40, &si);
#endif /*ENABLE_SI7021*/

#ifdef ENABLE_SHT40
	sht40_init(&bus_handle, 0x44, &sensor1);
	sht40_id_string_t id_string;
	sht40_get_id(&sensor1, id_string);
	ESP_LOGI(LOGTAG, "SHT40 sensor %s", id_string);
#endif
	for (; ; ) {
		ESP_LOGI(LOGTAG, "Loop");
        int x = gpio_get_level(3);
		ESP_LOGI(LOGTAG, "Button %d", x);

		hdc2080_check(&hdc, x == 0 ? true : false);
#ifdef ENABLE_SI7021
		si7021_check(&si);
#endif /*ENABLE_SI7021*/
#ifdef ENABLE_SHT40
		float temp, rh;
		if (sht40_convert(&sensor1, &temp, &rh)) {
			ESP_LOGI(LOGTAG, "SHT40   %s T %.2f RH %.2f", id_string, (double)temp, (double)rh);
		}
#endif
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
				ESP_LOGI(LOGTAG, "ROM %s T %.2f C %.2f F", q, (double)temp, (double)temp * 1.8 + 32);
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

