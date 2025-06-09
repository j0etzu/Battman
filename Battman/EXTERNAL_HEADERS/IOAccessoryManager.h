#ifndef _IOACCESSORYMANAGER_H
#define _IOACCESSORYMANAGER_H

#include "../iokitextern.h"

/* The name of defs are guessed, since Apple has not exposed anything */

__BEGIN_DECLS

#define kIOAccessoryPortIDSerial	1
#define kIOAccessoryPortIDUSB		2
// Scorpius 0x100, SmartConnector 0x101
// Serial2 0x102
#define kIOAccessoryPortID0Pin		0x200

#define kIOAMUSBModeEnableHostMode	2

#define kIOAMFeatureHighCurrentPower	0xFFFF

#define kIOAMDetectOverrideAll		(uint64_t)-1

typedef CF_ENUM(UInt64, IOAMPinID) {
	kIOAMPinDP1,
	kIOAMPinDN1,
	kIOAMPinDP2,
	kIOAMPinDN2,

	kIOAMPinNone = 6,
};
typedef CF_ENUM(UInt64, IOAMMeas) {
	kIOAMBrickID,
	kIOAMAuxID,
};

typedef CF_ENUM(UInt8, IOAMIndex) {
	kIOAMConfigurePower = 0,			// uint64_t
	kIOAMConfigureUSBMode,				// uint64_t
	kIOAMConfigureUSBConnectActive,			// void
	kIOAMConfigureUSBCurrentLimitSetBase,		// uint64_t
	kIOAMConfigureUSBCurrentLimitRestoreBase,	// void
	kIOAMConfigureUSBCurrentLimitSetOffset = 5,	// uint64_t
	kIOAMConfigureUSBCurrentLimitSetMaximum,	// uint64_t
	kIOAMConfigureUSBCurrentLimitClearMaximum,	// void
	kIOAMConfigurePowerDuringSleep,			// uint64_t
	kIOAMConfigureBatteryPack,			// uint64_t
	kIOAMConfigureDetectOverride = 10,		// uint64_t[3]{1(?), YES/NO, acc_id}
	kIOAMConfigureDigitalIDOverride,		// uint64_t
	kIOAMConfigureAttachStyle,			// uint64_t
	kIOAMConfigureAllowedFeatures,			// uint64_t[2]{Allow, Revoke}
	kIOAMConfigureDPDNConfigOverride,		// uint64_t[4]{dpdn1B, dpdn2B, dpdn1T, dpdn2T}
	kIOAMConfigureUSBPowerIgnore = 15,		// uint64_t
	
	kIOAMCheckPinVoltage,				// uint64_t[2]{PinID, Meas} -> uint64_t
	
	kIOAMConfigurePowerLowVoltageSelect,		// uint64_t
	
	kIOAMRequestAccessoryPowerOff,			// void
	
	kIOAMSetGoldbond,				// uint64_t
	kIOAMConfigureGoldbondMode = 20,		// uint64_t[2]{setMask, clearMask}
	
	kIOAMRequestLdcmMeasurement,			// uint64_t
	kIOAMGetLdcmParams,				// void -> uint64_t[126]
	
	kIOAMSetVoltageDetected,			// uint64_t
	kIOAMChallengeCryptoDock9Pin,
	kIOAMChallengeCryptoDock = 25,
	kIOAMConfigureUSBPreventSourceDetection,	// uint64_t
	kIOAMSelectEisPin,				// uint64_t{Enable, PinID} -> uint64_t
	kIOAMGetFreePinMask,				// void -> uint64_t
	
	kIOAMLDCMGetAvailablePins,			// void -> uint64_t[2]
	kIOAMSetLDCM = 30,				// uint64_t[2] -> uint64_t[2]
	kIOAMLDCMSetLiquidDetected,			// uint64_t
	kIOAMLDCMEnableMitigations,			// uint64_t
	kIOAMLDCMEnableUserOverride,			// uint64_t
	kIOAMLDCMGetMeasurementStatus,			// void -> uint64_t
	kIOAMLDCMSetState,				// uint64_t
};

/*
enum {
    kIOAMConnectionTypeVirtual,
    kIOAMConnectionTypeLightning,
    kIOAMConnectionTypeBT,
    kIOAMConnectionTypeOrion,
    kIOAMConnectionTypeC26,
    kIOAMConnectionTypeIP,
    kIOAMConnectionTypeUSB,
    kIOAMConnectionTypeBuiltInHW,
    kIOAMConnectionTypeScorpius,
    kIOAMConnectionTypeInductive,
    kIOAMConnectionTypeNFC,
};

typedef CF_ENUM(SInt32, IOAMTransportType) {
    kIOAMTransportTypeUSBD,
    kIOAMTransportTypeMikeyBus,
    kIOAMTransportTypeBT,
    kIOAMTransportTypeBLE,
    kIOAMTransportTypeAirPlay,
    kIOAMTransportTypeGeneric,
    kIOAMTransportTypeUART,
    kIOAMTransportTypeAID,
    kIOAMTransportTypeUSBH,
    kIOAMTransportTypeScorpius,
    kIOAMTransportTypeBattery,
    kIOAMTransportTypeTouchController,
    kIOAMTransportTypeInductive,
    kIOAMTransportTypeNFC,
};
*/
typedef enum {
    kIOAMInterfaceDeviceSerialNumber = 1,   // CFNumberRef SInt64
    kIOAMInterfaceModuleSerialNumber,       // CFStringRef char[32]
    kIOAMAccessorySerialNumber,             // CFStringRef char[32]
    kIOAMAccessoryManufacturer,             // CFStringRef char[256]
    kIOAMAccessoryName,                     // CFStringRef char[256]
    kIOAMAccessoryModelNumber,              // CFStringRef char[256]
    kIOAMAccessoryFirmwareVersion,          // CFStringRef char[256]
    kIOAMAccessoryHardwareVersion,			// CFStringRef char[256]
    kIOAMAccessoryPPID                      // CFStringRef char[256]
} AccessoryInfo;

typedef enum {
	kIOAMPowermodeOff = 1,
	kIOAMPowermodeLow,
	kIOAMPowermodeOn,
	kIOAMPowermodeHighCurrent,
	kIOAMPowermodeHighCurrentBM3,
	kIOAMPowermodeLowVoltage,

	kIOAMPowermodeCount = kIOAMPowermodeLowVoltage,
} AccessoryPowermode;

SInt32 IOAccessoryPortGetPort(io_service_t service);
SInt32 IOAccessoryPortGetManagerPrimaryPort(io_service_t service);
SInt32 IOAccessoryPortGetTransportType(io_service_t service);
SInt32 IOAccessoryPortGetStreamType(io_service_t service);

io_service_t IOAccessoryManagerGetServiceWithPrimaryPort(SInt32 port);
SInt32 IOAccessoryManagerGetPrimaryPort(io_service_t service);
SInt32 IOAccessoryManagerGetType(io_service_t service);
bool IOAccessoryManagerGetBatteryPackMode(io_service_t service);
io_registry_entry_t IOAccessoryManagerGetUpstreamService(io_service_t service);
SInt32 IOAccessoryManagerGetUpstreamPrimaryPort(io_service_t service);
SInt32 IOAccessoryManagerGetAccessoryID(io_service_t service);
CFNumberRef IOAccessoryManagerCopyAccessoryDeviceUID(io_service_t service);
IOReturn IOAccessoryManagerGetDigitalID(io_service_t service, UInt8 *bytes);
IOReturn IOAccesoryManagerGetDigitalIDSInt32erfaceDeviceInfo(io_service_t service, UInt8 *bytes);
IOReturn IOAccesoryManagerGetDigitalIDAccessoryVersionInfo(io_service_t service, UInt8 *bytes);
IOReturn IOAccessoryManagerCopyDeviceInfo(io_service_t service, AccessoryInfo infoID, CFTypeRef *a3);

SInt32 IOAccessoryManagerGetPowerMode(io_service_t service);
SInt32 IOAccessoryManagerGetActivePowerMode(io_service_t service);
unsigned long IOAccessoryManagerPowerModeCurrentLimit(io_service_t service, AccessoryPowermode mode);

bool IOAccessoryManagerPowerDuringSleepIsSupported(io_service_t service);
bool IOAccessoryManagerGetPowerDuringSleep(io_service_t service);
SInt32 IOAccessoryManagerGetSleepPowerCurrentLimit(io_service_t service);

IOReturn IOAccessoryManagerGetUSBConnectType(io_service_t service, SInt32 *type, bool *active);
IOReturn IOAccessoryManagerGetUSBConnectTypePublished(io_service_t service, SInt32 *published, bool *active);
IOReturn IOAccessoryManagerGetUSBChargingVoltage(io_service_t service, SInt32 *voltage);
IOReturn IOAccessoryManagerGetUSBCurrentLimit(io_service_t service, SInt32 *ilim);
IOReturn IOAccessoryManagerSetUSBCurrentLimitBase(io_service_t service, uint64_t input);
IOReturn IOAccessoryManagerRestoreUSBCurrentLimitBase(io_service_t service);
IOReturn IOAccessoryManagerGetUSBCurrentLimitBase(io_service_t service, SInt32 *ilimBase);
IOReturn IOAccessoryManagerSetUSBCurrentOffset(io_service_t service, SInt32 offset);
IOReturn IOAccessoryManagerGetUSBCurrentLimitOffset(io_service_t service, SInt32 *offset);
IOReturn IOAccessoryManagerSetUSBCurrentLimitMaximum(io_service_t service, uint64_t ilimMax);
IOReturn IOAccessoryManagerGetUSBCurrentLimitMaximum(io_service_t service, SInt32 *ilimMax);
IOReturn IOAccessoryManagerClearUSBCurrentLimitMaximum(io_service_t service);

__END_DECLS

#endif
