//
//  common.h
//  Battman
//
//  Created by Torrekie on 2025/1/21.
//

#ifndef common_h
#define common_h

#include <stdbool.h>
#include <dlfcn.h>
#include <TargetConditionals.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <sys/types.h>
#include <os/log.h>

#include "main.h"
#include "CompatibilityHelper.h"

#ifdef DEBUG
#define DBGLOG(...) NSLog(__VA_ARGS__)
#define DBGALT(x, y, z) show_alert(x, y, z)
#else
#define DBGLOG(...)
#define DBGALT(x, y, z)
#endif

#ifndef USE_MOBILEGESTALT
#define USE_MOBILEGESTALT 0
#endif

#define LICENSE_MIT 2
#define LICENSE_GPL 3
#define LICENSE_NONFREE 4

#ifndef LICENSE
#define LICENSE LICENSE_MIT
#endif

#if LICENSE == LICENSE_NONFREE
// Standalone packages (deb, ipa) for use in Torrekie's repo or else
#define NONFREE_TYPE_STANDALONE	0x10
// Havoc deb packages
#define NONFREE_TYPE_HAVOC		0x20
// Torrekie/Battman releases
#define NONFREE_TYPE_GITHUB		0x30

#ifndef NONFREE_TYPE
#define NONFREE_TYPE NONFREE_TYPE_STANDALONE
#endif
#endif

#if (LICENSE == LICENSE_NONFREE) && (NONFREE_TYPE == NONFREE_TYPE_HAVOC) && !__has_include("havoc-defs.h")
#error Havoc configuration is not designed for OSS Battman, please switch LICENSE to LICENSE_MIT!
#endif

#define DL_CALL(fn, ret, proto, call_args) \
	({                                     \
		static ret(*_fp) proto = NULL;     \
		if (!_fp)                          \
			_fp = (ret(*) proto)           \
		dlsym(RTLD_DEFAULT, #fn);          \
		_fp call_args;                     \
	})

#define IOS_CONTAINER_FMT "^/private/var/mobile/Containers/Data/Application/[0-9A-Fa-f\\-]{36}$"
#define MAC_CONTAINER_FMT "^/Users/[^/]+/Library/Containers/[^/]+/Data$"
#define SIM_CONTAINER_FMT "^/Users/[^/]+/Library/Developer/CoreSimulator/Devices/[0-9A-Fa-f\\-]{36}/data/Containers/Data/Application/[0-9A-Fa-f\\-]{36}$"
#define SIM_UNSANDBOX_FMT "^/Users/[^/]+/Library/Developer/CoreSimulator/Devices/[0-9A-Fa-f\\-]{36}/data$"

#define SFPRO "SFProDisplay-Regular"

__BEGIN_DECLS

#ifndef __OBJC__
void NSLog(CFStringRef fmt, ...);
#endif

extern const char *L_OK;
extern const char *L_FAILED;
extern const char *L_ERR;
extern const char *L_NONE;
extern const char *L_MA;
extern const char *L_MAH;
extern const char *L_MV;
extern const char *L_TRUE;
extern const char *L_FALSE;

extern os_log_t gLog;

void show_alert(const char *title, const char *message, const char *cancel_button_title);
#ifdef __OBJC__
void show_alert_async(const char *title, const char *message, const char *button, void (^completion)(bool result));
#else
void show_alert_async(const char *title, const char *message, const char *button, void *completion);
#endif
void show_alert_async_f(const char *title,const char *message,const char *button,void (*completion)(int));
void show_fatal_overlay_async(const char *title, const char *message);

char *preferred_language(void);
bool libintl_available(void);
bool gtk_available(void);

void init_common_text(void);

void app_exit(void);
bool is_carbon(void);
void open_url(const char *url);

bool match_regex(const char *string, const char *pattern);

int is_rosetta(void);

const char *lang_cfg_file(void);
int open_lang_override(int flags,int mode);
int preferred_language_code(void);

const char *target_type(void);

bool is_debugged(void);
bool is_platformized(void);
pid_t get_pid_for_launchd_label(const char *label);
pid_t get_pid_for_procname(const char *name);

__END_DECLS

#endif /* common_h */
