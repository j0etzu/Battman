//
//  iokit_connection.h
//  Battman
//
//  Created by Torrekie on 2025/2/9.
//

#ifndef iokit_connection_h
#define iokit_connection_h

#include "libsmc.h"
#include "../iokitextern.h"
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>

/* IOThunderboltController Power State */
#define kThunderboltPMSleepState 0
#define kThunderboltPMPauseState 1
#define kThunderboltPMWakeState  2

/* Known ChargerStatus */
/*
 Base: 0x8A
 0: None
 0xB0:
 0xB4: Stopped (charged?)
 0xD4: Not Connected
 0xD8: Charging
 0xB8: Stopped
 */

__BEGIN_DECLS

hvc_menu_t *convert_hvc(CFDictionaryRef dict, size_t *size, int8_t *index);
bool first_vendor_at_usagepagepairs(uint32_t *vid, uint32_t *pid, uint32_t usagePage, uint32_t usage);

__END_DECLS

#endif /* iokit_connection_h */
