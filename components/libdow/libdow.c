
#include "libdow.h"
#include "dow_devices.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <rom/ets_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *LOGTAG __attribute__((unused)) = "DOW";

bool
reset(
    dow_bus_t *bus,
    int *presence )
{
	taskDISABLE_INTERRUPTS();
	gpio_set_level(bus->dq_pin, 0);
	ets_delay_us(500);
	gpio_set_level(bus->dq_pin, 1);
	ets_delay_us(75);
    //pinMode(ONE_WIRE_PIN, INPUT);
    //delayMicroseconds(70);
    int x = gpio_get_level(bus->dq_pin);
	ets_delay_us(430);
    //digitalWrite(ONE_WIRE_PIN, HIGH);
    //pinMode(ONE_WIRE_PIN, OUTPUT);
	taskENABLE_INTERRUPTS();
    if (presence)
        *presence = x ? 0 : 1;
    return true;
}

bool
txBit(
    dow_bus_t *bus,
    unsigned char bit )
{
	taskDISABLE_INTERRUPTS();
    if (bit) {
		gpio_set_level(bus->dq_pin, 0);
		ets_delay_us(1);
		gpio_set_level(bus->dq_pin, 1);
		ets_delay_us(65);
    } else {
		gpio_set_level(bus->dq_pin, 0);
		ets_delay_us(60);
		gpio_set_level(bus->dq_pin, 1);
		ets_delay_us(5);
    }
	taskENABLE_INTERRUPTS();
    return true;
}

bool
txByte(
    dow_bus_t *bus,
    unsigned char byte )
{
    if (bus->spu_pin >= 0)
		gpio_set_level(bus->spu_pin, 1);
    for (int i = 0; i < 8; i++) {
        bool enspu = (i == 7) && bus->spu_pin >= 0;
		taskDISABLE_INTERRUPTS();
        if (byte & 1) {
			gpio_set_level(bus->dq_pin, 0);
			ets_delay_us(2);
			gpio_set_level(bus->dq_pin, 1);
            if (enspu)
				gpio_set_level(bus->spu_pin, 0);
			ets_delay_us(67);
        } else {
			gpio_set_level(bus->dq_pin, 0);
			ets_delay_us(60);
			gpio_set_level(bus->dq_pin, 1);
            if (enspu)
				gpio_set_level(bus->spu_pin, 0);
			ets_delay_us(5);
        }
		taskENABLE_INTERRUPTS();
        byte >>= 1;
    }
    return true;
}

bool
txBytes(
    dow_bus_t *bus,
    unsigned char *buf,
    int count )
{
    for (int i = 0; i < count; i++)
        txByte(bus, buf[i]);
    return true;
}

bool
rxBit(
    dow_bus_t *bus,
    unsigned char *bit )
{
	taskDISABLE_INTERRUPTS();
	gpio_set_level(bus->dq_pin, 0);
	ets_delay_us(1);
    //pinMode(ONE_WIRE_PIN, INPUT);
	gpio_set_level(bus->dq_pin, 1);
	ets_delay_us(15);
    int x = gpio_get_level(bus->dq_pin);
	ets_delay_us(50);
    //pinMode(ONE_WIRE_PIN, OUTPUT);
    //delayMicroseconds(1);
	taskENABLE_INTERRUPTS();
    *bit = x;
    return true;
}

bool
rxByte(
    dow_bus_t *bus,
    unsigned char *buf )
{
    unsigned char v = 0;
    for (int i = 0; i < 8; i++) {
		taskDISABLE_INTERRUPTS();
		gpio_set_level(bus->dq_pin, 0);
		ets_delay_us(1);
        //pinMode(ONE_WIRE_PIN, INPUT);
		gpio_set_level(bus->dq_pin, 1);
		ets_delay_us(15);
        int x = gpio_get_level(bus->dq_pin);
		ets_delay_us(50);
        //digitalWrite(ONE_WIRE_PIN, HIGH);
        //pinMode(ONE_WIRE_PIN, OUTPUT);
        //delayMicroseconds(1);
		taskENABLE_INTERRUPTS();
        v = (v >> 1) | (x ? 0x80 : 0);
    }
    *buf = v;
    return true;
}

bool
rxBytes(
    dow_bus_t *bus,
    unsigned char *buf,
    int count )
{
    for (int i = 0; i < count; i++)
        rxByte(bus, buf + i);
    return true;
}

unsigned char
crcBytes(
    unsigned char accum,
    unsigned char *buf,
    int count )
{
    unsigned char i;

    while (count > 0) {
        unsigned char b = *buf;

        for (int k = 0; k < 8; k++) {
            i = b ^ accum;
            b >>= 1;
            accum >>= 1;
            if (i & 1)
                accum ^= 0x8c;
        }
        ++buf;
        --count;
    }
    return accum;
}

esp_err_t
dowBusInit(
	dow_bus_t *bus,
	int dq_pin,
	int spu_pin )
{
	bus->dq_pin = dq_pin;
	bus->spu_pin = spu_pin;

	esp_err_t e;
	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
	io_conf.pin_bit_mask = BIT64(dq_pin);
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	e = gpio_config(&io_conf);
	if (e != ESP_OK)
		ESP_LOGE(LOGTAG, "Configure DQ pin: %s", esp_err_to_name(e));
	gpio_set_level(dq_pin, 1);

//    if (bus->spu_pin >= 0)
//		gpio_set_level(bus->spu_pin, 1);

	return e;
}

void
dowBusFin(
	dow_bus_t *bus )
{
}

void
dowRomString(
	dow_rom_t *rom,
    dow_rom_string_t s )
{
	const uint8_t *id = rom->id;
    for (int i = 0; i < 8; i++)
        snprintf(&s[i * 2], 3, "%02X", 0xff & id[i]);
}

void
dowRomClear(
	dow_rom_t *rom )
{
    memset(rom->id, 0, sizeof(rom->id));
}

bool
dowRomIsAllOnes(
	dow_rom_t *rom )
{
	const uint8_t *id = rom->id;
    return ((id[0] & id[1] & id[2] & id[3] & id[4] & id[5] & id[6] & id[7]) == 0xff)
           ? true : false;
}

bool
dowRomIsAllZeros(
	dow_rom_t *rom )
{
	const uint8_t *id = rom->id;
    return (id[0] | id[1] | id[2] | id[3] | id[4] | id[5] | id[6] | id[7])
           ? false : true;
}

esp_err_t
dowRomRead(
    dow_bus_t *bus,
    dow_rom_t *rom )
{
    reset(bus, (int *)0);
    txByte(bus, READ_ROM);
    rxBytes(bus, rom->id, sizeof(rom->id));
    return ESP_OK;
}

esp_err_t
dowRomMatch(
    dow_bus_t *bus,
    dow_rom_t *rom )
{
    reset(bus, (int *)0);
    txByte(bus, MATCH_ROM);
    txBytes(bus, rom->id, sizeof(rom->id));
    return ESP_OK;
}

esp_err_t
dowRomSkip(
    dow_bus_t *bus )
{
    reset(bus, (int *)0);
    txByte(bus, SKIP_ROM);
    return ESP_OK;
}

void dowRomSearchClear(
	dow_rom_search_t *srch )
{
	dowRomClear(&srch->last);
	srch->rmzt = 64;
}

/**
* reset the bus, send SEARCH ROM command and then do search protocol
* this is broken out because there used to be different hardware drivers
* that did it in different ways.
*/
static void
DoSearch(
    dow_bus_t *bus,
    unsigned char *pttn /* [8] */,   // pattern to direct search
    unsigned char *idfnd /* [8] */,  // returns ROM id found
    int *nrmzt )
{
    int bitno;
    unsigned char b0, b1, b2;
    int rmzt = 255;

    reset(bus, (int *)0);
    txByte(bus, SEARCH_ROM);
    for (bitno = 0; bitno < 64; bitno++) {
        rxBit(bus, &b0);
        rxBit(bus, &b1);
        if (!b0 && !b1) {
            b2 = (pttn[bitno / 8] >> (bitno & 7)) & 1;
            if (!b2)
                rmzt = bitno;
        } else {
            b2 = b0;
        }
        txBit(bus, b2);
        idfnd[bitno / 8] &= ~(1 << (bitno & 7));
        idfnd[bitno / 8] |= b2 << (bitno & 7);
    }
    *nrmzt = rmzt;
}

bool
dowRomSearchNext(
    dow_bus_t *bus,
    dow_rom_search_t *state,
    dow_rom_t *rom )
{
again:
    /* if last time yielded no 0-decisions, we're finished */
    if (state->rmzt == 255)
        return false;

    int bitno;
#ifdef DEBUG_ROM_SEARCH
    int i;
#endif
    int v;

    /*
    * construct new search pattern:
    * we start with the last-found ROM id, or 0000000000000000.
    * if there was a 0-decision taken, now set that bit, and
    * clear all to the right of it.
    */
    for (bitno = 0; bitno < 64; bitno++) {
        if (bitno < state->rmzt)
            v = (state->last.id[bitno / 8] >> (bitno & 7)) & 1;
        else if (bitno == state->rmzt)
            v = 1;
        else
            v = 0;
        state->last.id[bitno / 8] &= ~(1 << (bitno & 7));
        state->last.id[bitno / 8] |= v << (bitno & 7);
    }
#ifdef DEBUG_ROM_SEARCH
    printf("ptn=");
    for (i = 0; i < 64; i++)
            printf("%d%s", (context->last.id[i / 8] & (1 << (i & 7))) ? 1 : 0,
                            (i & 7) == 7 ? " " : "");
    printf("\nrmzt=%d\n", context->rmzt);
#endif

    DoSearch(bus, state->last.id, state->last.id, &state->rmzt);

#ifdef DEBUG_ROM_SEARCH
    printf("fnd=");
    for (i = 0; i < 64; i++)
            printf("%d%s", (context->last.id[i / 8] & (1 << (i & 7))) ? 1 : 0,
                            (i & 7) == 7 ? " " : "");
    printf("\nid=");
    for (i = 0; i < 8; i++)
            printf("%02x ", context->last.id[i]);
    printf(" rmzt=%d\n", context->rmzt);
#endif

    if (dowRomIsAllOnes(&state->last))
        return false;

    if (dowRomIsAllZeros(&state->last)) {
        //bus.setError(DOW_E_SHORTED, "One-wire bus shorted");
        return false;
    }

    /* sanity check that id found checksums. if not, go around again. */
    if (crcBytes(0, state->last.id, sizeof(state->last.id)))
        goto again;

    /* copy out id found, if requested */
    *rom = state->last;
    return true;
}

bool
ds1820ConvertAndWait(
    dow_bus_t *bus,
    dow_rom_t *rom )
{
    if (rom != NULL)
        dowRomMatch(bus, rom);
    else
        dowRomSkip(bus);
    //enablePullup(bus);
    txByte(bus, DS1820_CONVERT_TEMPERATURE);
    vTaskDelay(pdMS_TO_TICKS(900));
    //disablePullup(bus);
    return true;
}

bool
ds1820ReadScratchpad(
    dow_bus_t *bus,
    dow_rom_t *rom,
    unsigned char *buf )
{
    for (int tries = 0; tries < 3; ++tries) {
        if (rom != NULL)
            dowRomMatch(bus, rom);
        else
            dowRomSkip(bus);
        txByte(bus, DS1820_READ_SCRATCHPAD);
        rxBytes(bus, buf, 9);
        if (crcBytes(0, buf, 9) == 0)
            return true;
    }
    //bus.setError(DOW_E_CRC, "Read scratchpad: Bad CRC");
	ESP_LOGE(LOGTAG, "Read scratchpad: Bad CRC");
    return false;
}

bool
ds1820ReadTemp(
    dow_bus_t *bus,
    dow_rom_t *rom,
    float *temp )
{
    unsigned char buf[9];
    int traw, cperc, cremain;

    if (!ds1820ReadScratchpad(bus, rom, buf))
        return false;
    traw = ((buf[1] & 1) ? ~0x7f : 0) | (0x7f & (buf[0] >> 1));
    cremain = buf[6];
    cperc = buf[7];
    if (!cperc) {
        //bus.setError(DOW_E_CRC, "Bad read from sensor");
        ESP_LOGE(LOGTAG, "Bad read from sensor");
        return false;
    }
    if (buf[0] == 0xaa && buf[1] == 0) {
        //bus.setError(DOW_E_DS1820, "Temp poweron value");
        ESP_LOGE(LOGTAG, "Temp poweron value");
        return false;
    }
    *temp = traw - 0.25 + (float)(cperc - cremain) / cperc;
    return true;
}
