#ifndef libdow_h
#define libdow_h

/*
 * libdow - simple Dallas one-wire library for ESP32.
 *
 * taken mainly from my ancient (1998) libdow C library,
 * but simplified and optimized for directly driving I/O pins.
 */

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  state of a one-wire bus, passed to most operations.
 *  should be initialized with dowBusInit() before use and closed
 *  with dowBusFin() when no longer needed.
 */
typedef struct dow_bus {
	int dq_pin;
	int spu_pin;
} dow_bus_t;

/** representation of a ROM id */
typedef struct dow_rom {
	uint8_t id[8];
} dow_rom_t;

/** ASCII representation of a ROM id */
typedef char dow_rom_string_t[17];

/** state vector used for ROM search */
typedef struct dow_rom_search {
    dow_rom_t last;
    int rmzt;
} dow_rom_search_t;

/***********************************************************************
 *  Basic-level functions to read and write the bus
 */

esp_err_t
dowBusInit(
	dow_bus_t *bus,             /* bus to initialize */
	int dq_pin,                 /* GPIO pin of data line */
	int spu_pin );              /* GPIO pin of active low strong pullup enable (-1 if none) */

void
dowBusFin( dow_bus_t *bus );

/***********************************************************************
 *  ROM-level functions that apply to all devices
 */

/** get human-readable value for ROM ID */
void
dowRomString(
	dow_rom_t *rom,             /* rom to convert */
	dow_rom_string_t s );       /* output string value */

/** clear ROM ID */
void
dowRomClear( dow_rom_t *rom );

/**
 *  send READ_ROM command on bus and read value.
 *  assumes there is only a single device on the bus.
 */
esp_err_t
dowRomRead(
	dow_bus_t *bus,
	dow_rom_t *rom );

/**
 *  send MATCH_ROM command for a particular device.
 */
esp_err_t
dowRomMatch(
	dow_bus_t *bus,
	dow_rom_t *rom );

/**
 *  send SKIP_ROM
 */
esp_err_t
dowRomSkip( dow_bus_t *bus );

/**
 *  initialize a ROM search
 */
void
dowRomSearchClear( dow_rom_search_t *srch );

/**
 *  find and return the next ROM on the bus.
 *  state should be initialized by dowRomSearchClear() before starting.
 *  returns true as long as next device found.
 */
bool
dowRomSearchNext(
	dow_bus_t *bus,
	dow_rom_search_t *state,
	dow_rom_t *rom );

/***********************************************************************
 *  DS1820 device functions
 */

/**
 *  send convert command to DS1820, enable strong pullup if configured
 *  and wait out conversion time.
 */
bool
ds1820ConvertAndWait(
	dow_bus_t *bus,
	dow_rom_t *rom );           /* device ROM or NULL for all devices */

bool
ds1820ReadScratchpad(
	dow_bus_t *bus,
	dow_rom_t *rom,
	unsigned char *buf );

/**
 *  read temperature C from device, after conversion is done
 *  returns true if success.
 */
bool
ds1820ReadTemp(
	dow_bus_t *bus,
	dow_rom_t *rom,
	float *temp );

#ifdef __cplusplus
}
#endif

#endif /*libdow_h*/

