//
//  not_charging_reason.h
//  Battman
//
//  Created by Torrekie on 2025/3/4.
//

#ifndef not_charging_reason_h
#define not_charging_reason_h

// Not sure how was the original header named
// Use: if (NotChargingReason & REASON)
//      if ((NotChargingReason & FAULT_MASK) && (NotChargingReason & FAULTS))

#pragma NotChargingReasonVer0 -- NonSMC
/* Known Version 0 devices:
 * D10, D11,
 * J71, J72, J73, J81, J82, J85, J86, J87, J96, J97, J98, J99, J120, J121, J127, J128, J171, J172, J207, J208,
 * N27, N28, N61, N66, N69, N71, N74, N75, N111, N121
 */
/* NonSMC specific NCReasons */
#define NOT_CHARGING_REASON_POSM_MODE                           (uint64_t)(1 << 0x0B)
#define NOT_CHARGING_REASON_DISPLAY                             (uint64_t)(1 << 0x0C)

#define TOO_COLD                                                (uint64_t)(1 << 0x20 | 0)
#define TOO_HOT                                                 (uint64_t)(1 << 0x20 | 1)
#define DONE                                                    (uint64_t)(1 << 0x20 | 2)
#define TOO_LONG                                                (uint64_t)(1 << 0x20 | 3)
#define CHG_WD                                                  (uint64_t)(1 << 0x20 | 4)

#pragma NotChargingReasonVer1 -- Gibraltar (BNCR)
/* Known Version 1 devices:
 * J132, J137, J140, J152, J213, J214, J215, J680, J780
 */
/* Only use when BNCR present */
enum {
    NO_REASON,
    NO_AC,
    NO_BATTERY,
    BAD_BATTERY,
    BATTERY_FC,
    BATTERY_NO_CHG_REQ,
    AC_INSERT,
    G3,
    ADAPTER_DISABLED,
    ADAPTER_UNKNOWN,
    ADAPTER_NOT_ALLOW_CHARGING,
    CALIBRATION,
    B0LI_0,
    OS_NO_CHG,
    BCLM_REACHED,
    UPSTREAM_NO_CHG,
    PM_NO_CHG,
    TB0T_OVER_50,
    TB0T_OVER_45,
    TEMP_GRADIENT_TOO_HIGH,
    TEMP_NOT_ATV_VLD,
    BATTERY_TCA,
    OW_TDM_LINK_ACTIVE,
    CELL_VOLTAGE_TOO_HIGH,
    OBC_NO_CHG,
    VACTFB_NO_CHG,
    OBC_NO_INFLOW,
};

#pragma NotChargingReasonVer2 -- SMC (CHNC)
/* As of 2025, all devices not listed above are using V2 */

/* Common */
#define NOT_CHARGING_REASON_FULLY_CHARGED                       (uint64_t)(1 << 0x00)
#define NOT_CHARGING_REASON_TEMP_BELOW_MIN_STOP_CHARGING        (uint64_t)(1 << 0x01)
#define NOT_CHARGING_REASON_TEMP_ABOVE_MAX_STOP_CHARGING        (uint64_t)(1 << 0x02)
#define NOT_CHARGING_REASON_TEMP_BELOW_MIN_START_CHARGING       (uint64_t)(1 << 0x03)
#define NOT_CHARGING_REASON_TEMP_ABOVE_MAX_START_CHARGING       (uint64_t)(1 << 0x04)

/* Presense */
#define NOT_CHARGING_REASON_CHARGE_TIMER_EXPIRED                (uint64_t)(1 << 0x05) /* Charged over 15h or CHST is set */
#define NOT_CHARGING_REASON_BATTERY_NOT_PRESENT                 (uint64_t)(1 << 0x06)
#define NOT_CHARGING_REASON_VBUS_NOT_PRESENT                    (uint64_t)(1 << 0x07)

/* Charging Inhibit when SoC >= CC0S or Temp >= CC0T */
#define NOT_CHARGING_REASON_HIGH_SOC_HIGH_TEMP_STOP_CHARGING    (uint64_t)(1 << 0x08)
#define NOT_CHARGING_REASON_CSM_COMMUNICATION_FAILED            (uint64_t)(1 << 0x09)

/* Modes */
#define NOT_CHARGING_REASON_IOAM                                (uint64_t)(1 << 0x0A)
#define NOT_CHARGING_REASON_KIOSK_MODE                          (uint64_t)(1 << 0x0B)
#define NOT_CHARGING_REASON_COREMOTION                          (uint64_t)(1 << 0x0C)
#define NOT_CHARGING_REASON_USBPD                               (uint64_t)(1 << 0x0D)

/* Inhibit Charging */
#define NOT_CHARGING_REASON_SETBATT                             (uint64_t)(1 << 0x0E)
#define NOT_CHARGING_REASON_PREDICTIVECHARGING                  (uint64_t)(1 << 0x0F)
#define NOT_CHARGING_REASON_INDUCTIVE                           (uint64_t)(1 << 0x10) /* Refer to WADS */
#define NOT_CHARGING_REASON_GG_FW_UPDATE                        (uint64_t)(1 << 0x11)
#define NOT_CHARGING_REASON_INHIBIT_INFLOW_BATTERY_NOT_PRESENT  (uint64_t)(1 << 0x12) /* CH0R & 0x40 */

#define NOT_CHARGING_REASON_PCTM                                (uint64_t)(1 << 0x13)
#define NOT_CHARGING_REASON_INHIBIT_CLIENT_ADAPTER              (uint64_t)(1 << 0x14)
#define NOT_CHARGING_REASON_CELL_VOLTAGE_TOO_HIGH               (uint64_t)(1 << 0x15)
#define NOT_CHARGING_REASON_BATTERY_NO_CHG_REQ                  (uint64_t)(1 << 0x16)
#define NOT_CHARGING_REASON_WOMBAT                              (uint64_t)(1 << 0x17) /* macOS 13 / iOS 16 */

/* System controls */
#define NOT_CHARGING_REASON_VACTFB                              (uint64_t)0x01 << (0x30 | 0x04)
#define NOT_CHARGING_REASON_FIELDDIAGS                          (uint64_t)0x01 << (0x30 | 0x05)
#define NOT_CHARGING_REASON_INHIBIT_INFLOW                      (uint64_t)0x01 << (0x30 | 0x06)
#define NOT_CHARGING_REASON_CARRIER_TEST                        (uint64_t)0x01 << (0x30 | 0x07)

/* Faults */
#define NOT_CHARGING_REASON_PERMANENT_FAULT_MASK                (uint64_t)0xFF << (0x30 | 0x08)
#define NOT_CHARGING_REASON_BATT_CHARGED_TOO_LONG               (uint64_t)0x08 << (0x30 | 0x08)
#define NOT_CHARGING_REASON_VBAT_VFAULT                         (uint64_t)0x10 << (0x30 | 0x08)
#define NOT_CHARGING_REASON_IBAT_MINFAULT                       (uint64_t)0x20 << (0x30 | 0x08)
#define NOT_CHARGING_REASON_CHARGER_COMMUNICATION_FAILED        (uint64_t)0x40 << (0x30 | 0x08)
#define NOT_CHARGING_REASON_CELL_CHECK_FAULT                    (uint64_t)0x80 << (0x30 | 0x08)

#define BATTERY_CHARGING_ENABLE_EVENT                           0
#define BATTERY_CHARGING_KIOSK_MODE_ENABLE_INHIBIT_INFLOW_EVENT 1
#define BATTERY_CHARGING_KIOSK_MODE_ENABLE_ALLOW_INFLOW_EVENT   2
#define BATTERY_CHARGING_KIOSK_MODE_DISABLE_EVENT               3

#define DEVICE_IS_CHARGING                                      0

#endif /* not_charging_reason_h */
