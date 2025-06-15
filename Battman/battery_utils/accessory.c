#include "accessory.h"
#include "../common.h"
//#include "intlextern.h"
#include <mach/mach.h>

io_iterator_t gAccessories;
io_service_t gAccPrimary;
static bool use_libioam = false;

const char *acc_id_0_f[] = {
	"3K: Simple dock (beep on insert)",
	"10K: FireWire card reader",
	"18K: (Reserved/unused)",
	"28K: USB device accessory",
	"39K: (Reserved/unused)",
	"52K: (Reserved/unused)",
	"68K: Line out on/speaker off (MFP-compat, A63)",
	"88K: Diagnostics dock",
	"113K: (Reserved/unused)",
	"147K: (Reserved/unused)",
	"191K: USB iAP HID alternate config",
	"255K: Battery pack (no iPod charge)",
	"360K: Line out off, echo cancel on (MFP-compat, A63)",
	"549K: 30 pin connector serial device",
	"1000K: Car charger (pause on detach)",
	"3010K: Acc detect grounded but no resistor ID)",
};

const char *acc_id_50_5e[] = {
	"USBC: USB Host only",
	"USBC: USB Device only",
	"USBC: USB Host + DP display",
	"USBC: USB Device + DP display",
	"USBC: DP display only",
	"USBC: Snk current only",
	"USBC: Src current only",
	"USBC: Debug only",
	"USBC: Plugged other",
	NULL,
	"Digital ID: not found",
	"Digital ID", // This will need IOAccessoryManagerGetDigitalID()
	"Digital ID: unsupported",
	"Digital ID: unreliable",
	"Digital ID: reversed orientation",
};

/* Sadly I didn't got the full list of accids, but we can guess */
const char *acc_id_string(int accid) {
	static _Thread_local char buf[256];

	buf[0] = '\0';
	if (accid >= 0 && accid < 16) {
		snprintf(buf, sizeof(buf), "%d\n(%s)", accid, acc_id_0_f[accid]);
	} else if (accid > 79 && accid < 95 && accid != 89) {
		int idx = accid - 80;
		snprintf(buf, sizeof(buf), "%d\n(%s)", accid, acc_id_50_5e[idx]);
	} else if (accid == 70) {
		snprintf(buf, sizeof(buf), "%d\n(Scorpius: unknown)", accid);
	} else if (accid == 71) {
		snprintf(buf, sizeof(buf), "%d\n(Scorpius: pencil)", accid);
	}
	
	if (buf[0] != '\0') {
		return buf;
	}
	// Fallback for unknown IDs
	snprintf(buf, sizeof(buf), "%d", accid);
	return buf;
}


#ifdef _C
#undef _C
#endif
#define _C(x) x
static const char *acc_powermodes[] = {
	_C("Off"),
	_C("Low"),
	_C("On"),
	_C("High Current"),
	_C("High Current (BM3)"),
	_C("Low Voltage"),
};
static const char *acc_usb_connstats[] = {
	_C("None"),
	_C("Charger"),
	_C("Host"),
	_C("Host (alternate config)"), // HostAltConfig
	_C("Device"),
	_C("Device (Auth required)"),  // DeviceAuthRequired
};
static const char *acc_port_types[] = {
	_C("Unknown"),
	_C("Virtual"),
	_C("USB-C"),
	_C("USB-A"),
	_C("Mini DP"),
	_C("FireWire 800"),
	_C("HDMI"),
	_C("Audio Jack (Mini)"),
	_C("Ethernet"),
	_C("MagSafe"),			// This is typically old MagSafe charger port on old MacBooks
	_C("MagSafe 2"),
	_C("SD Card"),
	_C("Lightning"),
	_C("30-Pin"),			// Wow, who still using this
	_C("Inductive"),		// iPhone wireless charging (or they called MagSafe)
	_C("Smart Connector"),
	_C("Display Port"),
	_C("MagSafe 3"),		// 2021 introduced MagSafe 3 for MacBooks
	// We need help on testing 'Inductive', 'Contactless' and 'Wireless'
	_C("Contactless"),
	_C("Wireless")
};
#undef _C
extern const char *cond_localize_c(const char *);
#define _C(x) cond_localize_c(x)

const char *acc_powermode_string(AccessoryPowermode powermode) {
	static char modestr[32];
	// IOAM modes are starting form 1
	if ((powermode - 1) < kIOAMPowermodeCount) {
		return _C(acc_powermodes[powermode - 1]);
	}

	snprintf(modestr, 32, "<%d>", powermode);
	return modestr;
}
const char *acc_usb_connstat_string(SInt32 usb_connstat) {
	if (usb_connstat > 5) return _C("Unknown");
	
	return _C(acc_usb_connstats[usb_connstat]);
}

const char *acc_powermode_string_supported(accessory_powermode_t mode) {
	if (mode.supported_cnt == 0) return NULL;

	static char buffer[1024];
	sprintf(buffer, "%s: ", _C("Supported List"));
	for (size_t i = 0; i < mode.supported_cnt; i++) {
		sprintf(buffer, "%s%s%s<%lu %s>", buffer, (i == 0) ? "" : "\n", acc_powermode_string(mode.supported[i]), mode.supported_lim[i], L_MA);
	}

	return buffer;
}

const char *acc_usb_ilim_string_multiline(accessory_usb_ilim_t ilim) {
	if (ilim.limit == 0) return _C("Detached");

	// XXX: This is terrible, consider migrate to a better UI
	static char buffer[1024];
	sprintf(buffer, "%s: %lu %s", _C("Limit"), (unsigned long)ilim.limit, L_MA);
	if (ilim.base || ilim.offset || ilim.max) {
		sprintf(buffer, "%s\n(", buffer);
		if (ilim.base)
			sprintf(buffer, "%s%s: %lu", buffer, _C("Base"), (unsigned long)ilim.base);
		if (ilim.offset)
			sprintf(buffer, "%s%s%s: %lu", buffer, (ilim.base) ? " " : "", _C("Offset"), (unsigned long)ilim.offset);
		if (ilim.max)
			sprintf(buffer, "%s%s%s: %lu", buffer, (ilim.base || ilim.offset) ? " " : "", _C("Max"), (unsigned long)ilim.max);
		
		sprintf(buffer, "%s)", buffer);
	}

	return buffer;
}

const char *acc_port_type_string(SInt32 pt) {
	if (pt > 19) {
		return _C("Undefined");
	}
	return _C(acc_port_types[pt]);
}

const char *manf_id_string(SInt32 manf) {
	switch (manf) {
		// retrieve from online db? or just common vids?
		case VID_APPLE: return "Apple Inc.";
		case VID_UGREEN: return "Ugreen Group Limited";
		default: break;
	}
	return NULL;
}

const char *ugreen_prod_id_string(SInt32 prod) {
	switch (prod) {
		case 0xC5DC: return "MagSafe Charger (MFi Module)";	// 219693601610 A2463
		default:
			break;
	}
	return NULL;
}

const char *apple_prod_id_string(SInt32 prod) {
	switch (prod) {
		case 0x0500: return "MagSafe Charger";					// A2140
		case 0x0501: return "MagSafe Charger (MFi Module)";		// A2463
		case 0x0502: return "MagSafe Duo Charger";				// A2458
		case 0x0503: return "WatchPuck";						// A2515
		case 0x0504: return "WatchPuck (MFi Module)";			// A2755
		case 0x0505: return "WatchPuck (MFi Module)";			// A2675
		case 0x0506: return "MagSafe Charger";					// A2580

		case 0x1395: return "Smart Battery Case [iPhone 6]";
		case 0x1398: return "Smart Battery Case";
		case 0x1399: return "MagSafe Battery Pack";				// A2384
		case 0x139B: return "MagSafe Charger (MFi Module)";		// A2728

		case 0x7002:											// A2166
		case 0x7016:											// A2518
		case 0x7019:											// A2452
		case 0x701A:											// A2571
		case 0x701B: return "Power Adapter";					// A2676

		case 0x7800: return "MagSafe Cable";					// A2363
		case 0x7803: return "MagSafe Charger";					// A2781
		default: break;
	}
	return NULL;
}

/* Private func of IOAM */
static IOReturn checkIDBusAvailable(io_registry_entry_t entry, bool accessory_mode) {
	IOReturn result = kIOReturnSuccess;
	UInt8 bytes[6];
	
	memset(bytes, 0xAA, 6);

	result = get_acc_digitalid(entry, bytes);
	if (result == kIOReturnSuccess) {
		result = kIOReturnNotReadable;
		if (bytes[0] <= 0x3F) {
			if (accessory_mode && (bytes[1] & 3) == 0)
				return kIOReturnUnsupportedMode;
			else
				return kIOReturnNotFound;
		}
	}
	return result;
}

/* Make sure logics are not directly called in the UI */
#pragma mark - IOAccessoryMananger

io_iterator_t IOAccessoryManagerGetServices(void) {
	io_iterator_t services = MACH_PORT_NULL;
	IOReturn kr = kIOReturnSuccess;

	kr = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("IOAccessoryManager"), &services);
	if (kr != kIOReturnSuccess) {
		DBGLOG(CFSTR("Cannot open IOAccessoryManager (0x%X)"), kr);
	}

	return services;
}

/* Different from AppleSMC calls, we are going to get services quite often */
__attribute__((constructor))
void check_ioam_existance(void) {
	void *lib = dlopen("/usr/lib/libIOAccessoryManager.dylib", RTLD_LAZY);
	if (lib) {
		use_libioam = true;
	}
}

io_service_t acc_open_with_port(int port) {
	if (use_libioam)
		return IOAccessoryManagerGetServiceWithPrimaryPort(port);

	CFMutableDictionaryRef service;
	CFDictionaryRef matching;
	void *values;
	void *keys;
	int valuePtr;
	
	valuePtr = port;
	service = IOServiceMatching("IOAccessoryManager");
	keys = (void *)CFSTR("IOAccessoryPrimaryDevicePort");
	values = (void *)CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &valuePtr);
	matching = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&keys, (const void **)&values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(service, CFSTR("IOPropertyMatch"), matching);
	CFRelease(matching);
	CFRelease(keys);
	CFRelease(values);
	return IOServiceGetMatchingService(kIOMasterPortDefault, service);
}

static IOReturn copyDeviceInfo(io_connect_t connect, AccessoryInfo infoID, CFTypeRef *buf) {
	IOReturn kr = kIOReturnSuccess;
	if (use_libioam) {
		kr = IOAccessoryManagerCopyDeviceInfo(connect, infoID, buf);
	} else {
		bool accessory_mode = true;
		CFStringRef key = NULL;
		switch (infoID) {
			case kIOAMInterfaceDeviceSerialNumber:
				accessory_mode = false;
				key = CFSTR("IOAccessoryInterfaceDeviceSerialNumber");
				break;
			case kIOAMInterfaceModuleSerialNumber:
				accessory_mode = false;
				key = CFSTR("IOAccessoryInterfaceModuleSerialNumber");
				break;
			case kIOAMAccessorySerialNumber:    key = CFSTR("IOAccessoryAccessorySerialNumber"); break;
			case kIOAMAccessoryManufacturer:    key = CFSTR("IOAccessoryAccessoryManufacturer"); break;
			case kIOAMAccessoryName:            key = CFSTR("IOAccessoryAccessoryName"); break;
			case kIOAMAccessoryModelNumber:     key = CFSTR("IOAccessoryAccessoryModelNumber"); break;
			case kIOAMAccessoryFirmwareVersion: key = CFSTR("IOAccessoryAccessoryFirmwareVersion"); break;
			case kIOAMAccessoryHardwareVersion: key = CFSTR("IOAccessoryAccessoryHardwareVersion"); break;
			case kIOAMAccessoryPPID:            key = CFSTR("IOAccessoryAccessoryPPID"); break;
			default: kr = kIOReturnBadArgument;
		}
		if (kr == kIOReturnSuccess) {
			*buf = IORegistryEntryCreateCFProperty(connect, key, kCFAllocatorDefault, kNilOptions);
			if (buf) {
				kr = kIOReturnSuccess;
			} else {
				kr = checkIDBusAvailable(connect, accessory_mode);
			}
		}
	}
	return kr;
}

SInt32 get_accid(io_connect_t connect) {
	SInt32 accid = -1;
	if (use_libioam) {
		accid = IOAccessoryManagerGetAccessoryID(connect);
	} else {
		CFNumberRef number;
		number = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryID"), kCFAllocatorDefault, kNilOptions);
		if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &accid)) {
			accid = -1;
		}
		if (number) CFRelease(number);
	}
	return accid;
}

bool get_acc_battery_pack_mode(io_connect_t connect) {
	if (use_libioam)
		return IOAccessoryManagerGetBatteryPackMode(connect);

	CFBooleanRef boolean = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryBatteryPack"), kCFAllocatorDefault, kNilOptions);

	bool result = (boolean == kCFBooleanTrue);
	if (boolean) CFRelease(boolean);

	return result;
}

SInt32 get_acc_allowed_features(io_connect_t connect) {
	SInt32 buffer = -1;
	CFNumberRef AllowedFeatures;

	AllowedFeatures = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryManagerAllowedFeatures"), kCFAllocatorDefault, kNilOptions);
	if (AllowedFeatures) {
		if (!CFNumberGetValue(AllowedFeatures, kCFNumberSInt32Type, &buffer)) {
			DBGLOG(CFSTR("get_acc_allowed_features: Invalid"));
		}
	} else {
		DBGLOG(CFSTR("get_acc_allowed_features: None"));
	}
	if (AllowedFeatures) CFRelease(AllowedFeatures);

	return buffer;
}

SInt32 get_acc_port_type(io_connect_t connect) {
	SInt32 buffer = -1;
	CFNumberRef PortType;
	
	PortType = IORegistryEntryCreateCFProperty(connect, CFSTR("PortType"), kCFAllocatorDefault, kNilOptions);
	if (PortType) {
		if (!CFNumberGetValue(PortType, kCFNumberSInt32Type, &buffer)) {
			DBGLOG(CFSTR("get_acc_port_type: Invalid"));
		}
	} else {
		DBGLOG(CFSTR("get_acc_port_type: None"));
	}
	if (PortType) CFRelease(PortType);
	
	return buffer;
}

typedef struct {
	AccessoryInfo   key;
	void           *dest;
	size_t          len;
	
} AccQuery;
accessory_info_t get_acc_info(io_connect_t connect) {
	IOReturn     kr = kIOReturnSuccess;
	CFTypeRef    buffer = NULL;
	accessory_info_t info;
	
	memset(&info, 0, sizeof(info));

	AccQuery queries[] = {
		{kIOAMAccessorySerialNumber,    info.serial, sizeof(info.serial)},
		{kIOAMAccessoryManufacturer,    info.vendor, sizeof(info.vendor)},
		{kIOAMAccessoryName,            info.name,   sizeof(info.name)},
		{kIOAMAccessoryModelNumber,     info.model,  sizeof(info.model)},
		{kIOAMAccessoryFirmwareVersion, info.fwVer,  sizeof(info.fwVer)},
		{kIOAMAccessoryHardwareVersion, info.hwVer,  sizeof(info.hwVer)},
		{kIOAMAccessoryPPID,            info.PPID,   sizeof(info.PPID)},
	};
	
	for (size_t i = 0; i < sizeof(queries) / sizeof(queries[0]); i++) {
		kr = copyDeviceInfo(connect, queries[i].key, &buffer);
		if (kr != kIOReturnSuccess) {
			NSLog(CFSTR("get_acc_info(%d): %s"), queries[i].key, mach_error_string(kr));
			continue;
		}

		memset(queries[i].dest, 0, queries[i].len);

		/* We only handle Accessories, so they are all strings */
		if (!CFStringGetCString((CFStringRef)buffer, queries[i].dest, queries[i].len, kCFStringEncodingUTF8)) {
			NSLog(CFSTR("get_acc_info(%d): CF Error"), queries[i].key);
			continue;
		}
		DBGLOG(CFSTR("get_acc_info(%d): got %s"), queries[i].key, (char *)queries[i].dest);
		if (buffer) CFRelease(buffer);
	}

	return info;
}

accessory_powermode_t get_acc_powermode(io_connect_t connect) {
	accessory_powermode_t mode;
	CFArrayRef supported;

	memset(&mode, 0, sizeof(mode));

	if (use_libioam) {
		mode.mode = IOAccessoryManagerGetPowerMode(connect);
		mode.active = IOAccessoryManagerGetActivePowerMode(connect);
	} else {
		CFNumberRef number;
		number = (CFNumberRef)IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryPowerMode"), kCFAllocatorDefault, kNilOptions);
		if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &mode.mode)) {
			mode.mode = 0;
		}
		number = (CFNumberRef)IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryActivePowerMode"), kCFAllocatorDefault, kNilOptions);
		if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &mode.active)) {
			mode.active = 0;
		}
		if (number) CFRelease(number);
	}

	supported = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessorySupportedPowerModes"), kCFAllocatorDefault, kNilOptions);
	if (supported) {
#if DEBUG
		CFShow(supported);
#endif
		mode.supported_cnt = CFArrayGetCount(supported);
		for (int i = 0; i < mode.supported_cnt; i++) {
			CFNumberRef value = CFArrayGetValueAtIndex(supported, i);
			if (CFNumberGetValue(value, kCFNumberSInt32Type, &mode.supported[i])) {
				if (use_libioam)
					mode.supported_lim[i] = IOAccessoryManagerPowerModeCurrentLimit(connect, mode.supported[i]);
				else {
					CFArrayRef array = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryPowerCurrentLimits"), kCFAllocatorDefault, kNilOptions);
					if (array) {
						if (mode.supported[i]) {
							CFIndex modeIndex = mode.supported[i] - 1;
							if (CFArrayGetCount(array) > i) {
								CFNumberRef number = CFArrayGetValueAtIndex(array, modeIndex);
								if (number)
									CFNumberGetValue(number, kCFNumberSInt32Type, &mode.supported_lim[i]);
							}
						}
						CFRelease(array);
					}
				}
			}
			if (value) CFRelease(value);
		}
	}
	if (supported) CFRelease(supported);

	return mode;
}

accessory_sleeppower_t get_acc_sleeppower(io_connect_t connect) {
	accessory_sleeppower_t sleep;

	memset(&sleep, 0, sizeof(sleep));

	if (use_libioam) {
		sleep.supported = IOAccessoryManagerPowerDuringSleepIsSupported(connect);
		sleep.enabled = IOAccessoryManagerGetPowerDuringSleep(connect);
		sleep.limit = IOAccessoryManagerGetSleepPowerCurrentLimit(connect);
	} else {
		CFBooleanRef result;
		result = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryManagerSleepPower"), kCFAllocatorDefault, kNilOptions);
		sleep.supported = (result != NULL);
		sleep.enabled = (result == kCFBooleanTrue);
		if (result) CFRelease(result);

		CFNumberRef number;
		number = (CFNumberRef)IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessorySleepPowerCurrentLimit"), kCFAllocatorDefault, kNilOptions);
		if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &sleep.limit)) {
			sleep.limit = 0;
		}
		if (number) CFRelease(number);
	}

	return sleep;
}

bool get_acc_supervised(io_connect_t connect) {
	CFBooleanRef supervised;

	supervised = IORegistryEntryCreateCFProperty(connect, CFSTR("SupervisedAccessoryAttached"), kCFAllocatorDefault, kNilOptions);

	bool ret = (supervised == kCFBooleanTrue);
	if (supervised) CFRelease(supervised);

	return ret;
}

bool get_acc_supervised_transport_restricted(io_connect_t connect) {
	CFBooleanRef restricted;
	
	restricted = IORegistryEntryCreateCFProperty(connect, CFSTR("SupervisedTransportsRestricted"), kCFAllocatorDefault, kNilOptions);
	
	bool ret = (restricted == kCFBooleanTrue);
	if (restricted) CFRelease(restricted);
	
	return ret;
}

SInt32 get_acc_type(io_connect_t connect) {
	if (use_libioam)
		return IOAccessoryManagerGetType(connect);

	CFNumberRef number;
	SInt32 type = 0;
	number = (CFNumberRef)IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryManagerType"), kCFAllocatorDefault, kNilOptions);
	if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &type)) {
		 type = 0;
	}
	if (number) CFRelease(number);

	return type;
}

IOReturn get_acc_digitalid(io_connect_t connect, void *digitalID) {
	IOReturn kr = kIOReturnSuccess;
	if (use_libioam)
		return IOAccessoryManagerGetDigitalID(connect, digitalID);
		
	CFDataRef data = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryDigitalID"), kCFAllocatorDefault, kNilOptions);

	if (data) {
		CFDataGetBytes(data, CFRangeMake(0, 6), digitalID);
		CFRelease(data);
		return kIOReturnSuccess;
	} else {
		kr = kIOReturnUnsupported;
		if ((get_acc_type(connect) & 0xF) != 0 ) {
			CFBooleanRef detect = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryDetect"), kCFAllocatorDefault, kNilOptions);
			kr = (detect == kCFBooleanTrue) ? kIOReturnNotReady : kIOReturnNotAttached;

			if (detect) CFRelease(detect);
		}
	}
	return kr;
}

IOReturn get_acc_usb_connstat(io_connect_t connect, accessory_usb_connstat_t *connstat) {
	IOReturn kr = kIOReturnSuccess;
	memset(connstat, 0, sizeof(accessory_usb_connstat_t));

	if (use_libioam) {
		SInt32 buf;
		bool active;
		kr = IOAccessoryManagerGetUSBConnectType(connect, &buf, &active);
		if (kr == kIOReturnSuccess) {
			connstat->active = active;
			connstat->type = buf;
		}

		kr = IOAccessoryManagerGetUSBConnectTypePublished(connect, &buf, &active);
		if (kr == kIOReturnSuccess) {
			connstat->active = active;
			connstat->published_type = buf;
		}
	} else {
		CFMutableDictionaryRef properties;
		kr = IORegistryEntryCreateCFProperties(connect, &properties, kCFAllocatorDefault, kNilOptions);
		if (kr == kIOReturnSuccess) {
			CFNumberRef number = CFDictionaryGetValue(properties, CFSTR("IOAccessoryUSBConnectType"));
			if (number) {
				if (!CFNumberGetValue(number, kCFNumberSInt32Type, &(connstat->type)))
					connstat->type = -1;
			}
			number = CFDictionaryGetValue(properties, CFSTR("IOAccessoryUSBConnectTypePublished"));
			if (number) {
				if (!CFNumberGetValue(number, kCFNumberSInt32Type, &(connstat->published_type)))
					connstat->published_type = -1;
			}
			connstat->active = (CFDictionaryGetValue(properties, CFSTR("IOAccessoryUSBActive")) == kCFBooleanTrue);

			CFRelease(properties);
		}
	}
	return kr;
}

IOReturn get_acc_usb_voltage(io_connect_t connect, SInt32 *voltage) {
	IOReturn kr = kIOReturnSuccess;
	if (use_libioam)
		return IOAccessoryManagerGetUSBChargingVoltage(connect, voltage);
	
	CFNumberRef number;
	number = (CFNumberRef)IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryUSBChargingVoltage"), kCFAllocatorDefault, kNilOptions);
	if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, voltage)) {
		*voltage = 0;
		kr = kIOReturnNotAttached;
	}
	if (number) CFRelease(number);
	
	return kr;
}

IOReturn get_acc_usb_ilim(io_connect_t connect, accessory_usb_ilim_t *ilim) {
	IOReturn kr = kIOReturnSuccess;
	IOReturn lim = 0, base = 0, offset = 0, max = 0;

	memset(ilim, 0, sizeof(accessory_usb_ilim_t));
	if (use_libioam) {
		lim = IOAccessoryManagerGetUSBCurrentLimit(connect, &(ilim->limit));
		base = IOAccessoryManagerGetUSBCurrentLimitBase(connect, &(ilim->base));
		offset = IOAccessoryManagerGetUSBCurrentLimitOffset(connect, &(ilim->offset));
		max = IOAccessoryManagerGetUSBCurrentLimitMaximum(connect, &(ilim->max));
	} else {
		CFNumberRef number;

		number = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryUSBCurrentLimit"), kCFAllocatorDefault, kNilOptions);
		if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &(ilim->limit))) {
			lim = kIOReturnNotAttached;
		}
		CFRelease(number);

		number = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryUSBCurrentLimitBase"), kCFAllocatorDefault, kNilOptions);
		if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &(ilim->base))) {
			base = kIOReturnNotAttached;
		}
		CFRelease(number);

		number = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryUSBCurrentLimitOffset"), kCFAllocatorDefault, kNilOptions);
		if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &(ilim->offset))) {
			offset = kIOReturnNotFound;
		}
		CFRelease(number);

		number = IORegistryEntryCreateCFProperty(connect, CFSTR("IOAccessoryUSBCurrentLimitMaximum"), kCFAllocatorDefault, kNilOptions);
		if (!number || !CFNumberGetValue(number, kCFNumberSInt32Type, &(ilim->max))) {
			offset = kIOReturnNotFound;
		}
		CFRelease(number);
	}

	if ((lim == kIOReturnNotAttached) && (base == kIOReturnNotAttached) && (offset == kIOReturnNotFound) && (max == kIOReturnNotFound)) {
		kr = kIOReturnNotAttached;
	}
	
	return kr;
}

IOReturn get_acc_idsn(io_connect_t connect, SInt64 *buf) {
	IOReturn kr;
	CFTypeRef number;
	kr = copyDeviceInfo(connect, kIOAMInterfaceDeviceSerialNumber, &number);
	if (kr == kIOReturnSuccess) {
		if (!CFNumberGetValue((CFNumberRef)number, kCFNumberSInt64Type, buf)) {
			kr = kIOReturnError;
		}
		CFRelease(number);
	}
	return kr;
}

IOReturn get_acc_msn(io_connect_t connect, void *buf) {
	IOReturn kr;
	CFTypeRef sn;
	kr = copyDeviceInfo(connect, kIOAMInterfaceModuleSerialNumber, &sn);
	if (kr == kIOReturnSuccess) {
		if (!CFStringGetCString(sn, buf, 32, kCFStringEncodingUTF8)) {
			kr = kIOReturnError;
		}
		CFRelease(sn);
	}
	return kr;
}
