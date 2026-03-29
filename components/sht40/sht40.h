#ifndef libsht40_h
#define libsht40_h

/*
 *  Sensirion SHT40 remperature/RH sensor with I2C interface
 */

#include <driver/i2c_master.h>

#ifdef __cplusplus
extern "C" {
#endif

/** human readable representation of sensor id */
typedef char sht40_id_string_t[13];

typedef struct sht40_sensor {
	i2c_master_dev_handle_t dev_handle;
	uint8_t id[6];
	bool present;
} sht40_sensor_t;

bool sht40_init( i2c_master_bus_handle_t *bus, uint8_t address, sht40_sensor_t *sensor );

bool sht40_get_id( sht40_sensor_t *sensor, sht40_id_string_t id_string );

bool sht40_convert( sht40_sensor_t *sensor, float *temp, float *rh );

#ifdef __cplusplus
}
#endif

#endif /*libsht40_h*/
