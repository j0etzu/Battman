//
//  scprefs.h
//  Battman
//
//  Created by Torrekie on 2025/7/18.
//

#if !defined(scprefs_h) && !defined(_SCPREFERENCES_H)
#define scprefs_h
#define _SCPREFERENCES_H

#include <CoreFoundation/CoreFoundation.h>

CF_IMPLICIT_BRIDGING_ENABLED
CF_ASSUME_NONNULL_BEGIN

__BEGIN_DECLS

typedef const struct CF_BRIDGED_TYPE(id) __SCPreferences *SCPreferencesRef;

SCPreferencesRef __nullable SCPreferencesCreate(CFAllocatorRef __nullable allocator, CFStringRef name, CFStringRef __nullable prefsID);

CFPropertyListRef __nullable SCPreferencesGetValue(SCPreferencesRef prefs, CFStringRef key);

__END_DECLS

CF_ASSUME_NONNULL_END
CF_IMPLICIT_BRIDGING_DISABLED

#endif /* scprefs_h */
