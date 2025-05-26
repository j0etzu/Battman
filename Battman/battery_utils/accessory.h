//
//  accessory.h
//  Battman
//
//  Created by Torrekie on 2025/5/17.
//

#ifndef accessory_h
#define accessory_h

#include <stdio.h>
#include "IOAccessoryManager.h"

#define VID_APPLE 0x05ac

typedef struct accessory_info {
	char serial[32];
	char vendor[256];
	char name[256];
	char model[256];
	char fwVer[256];
	char hwVer[256];
	char PPID[256];
} accessory_info_t;

typedef struct accessory_powermode {
	AccessoryPowermode mode;
	AccessoryPowermode active;
	size_t supported_cnt;
	AccessoryPowermode supported[kIOAMPowermodeCount];
	unsigned long supported_lim[kIOAMPowermodeCount];
} accessory_powermode_t;

typedef struct accessory_sleeppower {
	bool supported;
	bool enabled;
	SInt32 limit;
} accessory_sleeppower_t;

__BEGIN_DECLS

const char *acc_id_string(SInt32 accid);
const char *manf_id_string(SInt32 manf);
const char *apple_prod_id_string(SInt32 prod);
const char *acc_powermode_string(AccessoryPowermode powermode);
const char *acc_powermode_string_supported(accessory_powermode_t mode);

io_connect_t acc_open_with_port(int port);

SInt32 get_accid(io_connect_t connect);
bool get_acc_battery_pack_mode(io_connect_t connect);
SInt32 get_acc_allowed_features(io_connect_t connect);
accessory_info_t get_acc_info(io_connect_t connect);
accessory_powermode_t get_acc_powermode(io_connect_t connect);
accessory_sleeppower_t get_acc_sleeppower(io_connect_t connect);
bool get_acc_supervised(io_connect_t connect);
bool get_acc_supervised_transport_restricted(io_connect_t connect);
SInt32 get_acc_type(io_connect_t connect);
IOReturn get_acc_digitalid(io_connect_t connect, UInt8 *digitalID);

__END_DECLS

#endif /* accessory_h */
