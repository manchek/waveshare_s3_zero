#ifndef dow_devices_h
#define dow_devices_h

/*
 * basic ROM commands for all devices
 */

#define READ_ROM     0x33
#define MATCH_ROM    0x55
#define SEARCH_ROM   0xF0
#define ALARM_SEARCH 0xEC
#define SKIP_ROM     0xCC

/*
 * DS1820
 */

#define DS1820_CONVERT_TEMPERATURE 0x44
#define DS1820_COPY_SCRATCHPAD     0x48
#define DS1820_READ_POWER_SUPPLY   0xB4
#define DS1820_RECALL_E2           0xB8
#define DS1820_READ_SCRATCHPAD     0xBE
#define DS1820_WRITE_SCRATCHPAD    0x4E

#endif /*dow_devices_h*/
