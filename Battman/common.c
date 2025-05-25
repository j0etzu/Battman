#include <CoreGraphics/CoreGraphics.h>
#include <TargetConditionals.h>

#include <regex.h>
#include <sys/sysctl.h>

#include "battery_utils/battery_info.h"
#include "cobjc/cobjc.h"
#include "common.h"
#include "gtkextern.h"
#include "intlextern.h"

/* Consider make this a standalone header */
#define SYM_EXIST(...) check_ptr(__VA_ARGS__)

#define PTR_TYPE_NAME(x, y) \
	x##_ptr = y;            \
	DBGLOG(@"%s_ptr (%p) = %s (%p)", #x, x, #y, y);
#define PTR_TYPE(x) PTR_TYPE_NAME(x, x)
#define PTR_TYPE_NAME_DLSYM(handle, y, x) ((y##_ptr = (typeof(x) *)dlsym(handle, #x)) != NULL)
#define PTR_TYPE_DLSYM(handle, x) PTR_TYPE_NAME_DLSYM(handle, x, x)

typeof(gettext)                 *gettext_ptr;
typeof(textdomain)              *textdomain_ptr;
typeof(bindtextdomain)          *bindtextdomain_ptr;
typeof(bind_textdomain_codeset) *bind_textdomain_codeset_ptr;

typeof(gtk_dialog_get_type)     *gtk_dialog_get_type_ptr;
typeof(gtk_message_dialog_new)  *gtk_message_dialog_new_ptr;
typeof(gtk_dialog_add_button)   *gtk_dialog_add_button_ptr;
typeof(gtk_dialog_run)          *gtk_dialog_run_ptr;
typeof(gtk_widget_destroy)      *gtk_widget_destroy_ptr;

extern CFStringRef               NSFontAttributeName;
extern CFStringRef               NSForegroundColorAttributeName;
extern id                        UIGraphicsGetImageFromCurrentImageContext(void);
extern void                      UIGraphicsBeginImageContextWithOptions(CGSize, BOOL, int);
extern void                      UIGraphicsEndImageContext(void);

// CALLER FREE
static char                     *get_CFLocale() {
    CFArrayRef list = CFLocaleCopyPreferredLanguages();

    if (list == NULL || CFArrayGetCount(list) == 0)
        return NULL;

    char *lang = (char *)malloc(256);

    if (!CFStringGetCString(CFArrayGetValueAtIndex(list, 0), lang, 256, kCFStringEncodingUTF8)) {
        CFRelease(list);
        return NULL;
    }

    CFRelease(list);
    return lang;
}

// Caller free!!!
char *preferred_language(void) {
	/* Convert new-style locale names with language tags (ISO 639 and ISO 15924)
	   to Unix (ISO 639 and ISO 3166) names.  */
	typedef struct {
		const char langtag[10 + 1];
		const char unixy[12 + 1];
	} langtag_entry;
	static const langtag_entry langtag_table[] = {
		/* Mac OS X has "az-Arab", "az-Cyrl", "az-Latn".
		 The default script for az on Unix is Latin.  */
		{"az-Latn",     "az"   },
		/* Mac OS X has "bs-Cyrl", "bs-Latn".
		 The default script for bs on Unix is Latin.  */
		{ "bs-Latn",    "bs"   },
		/* Mac OS X has "ga-dots".  Does not yet exist on Unix.  */
		{ "ga-dots",    "ga"   },
		/* Mac OS X has "kk-Cyrl".
		 The default script for kk on Unix is Cyrillic.  */
		{ "kk-Cyrl",    "kk"   },
		/* Mac OS X has "mn-Cyrl", "mn-Mong".
		 The default script for mn on Unix is Cyrillic.  */
		{ "mn-Cyrl",    "mn"   },
		/* Mac OS X has "ms-Arab", "ms-Latn".
		 The default script for ms on Unix is Latin.  */
		{ "ms-Latn",    "ms"   },
		/* Mac OS X has "pa-Arab", "pa-Guru".
		 Country codes are used to distinguish these on Unix.  */
		{ "pa-Arab",    "pa_PK"},
		{ "pa-Guru",    "pa_IN"},
		/* Mac OS X has "shi-Latn", "shi-Tfng".  Does not yet exist on Unix.  */
		/* Mac OS X has "sr-Cyrl", "sr-Latn".
		 The default script for sr on Unix is Cyrillic.  */
		{ "sr-Cyrl",    "sr"   },
		/* Mac OS X has "tg-Cyrl".
		 The default script for tg on Unix is Cyrillic.  */
		{ "tg-Cyrl",    "tg"   },
		/* Mac OS X has "tk-Cyrl".
		 The default script for tk on Unix is Cyrillic.  */
		{ "tk-Cyrl",    "tk"   },
		/* Mac OS X has "tt-Cyrl".
		 The default script for tt on Unix is Cyrillic.  */
		{ "tt-Cyrl",    "tt"   },
		/* Mac OS X has "uz-Arab", "uz-Cyrl", "uz-Latn".
		 The default script for uz on Unix is Latin.  */
		{ "uz-Latn",    "uz"   },
		/* Mac OS X has "vai-Latn", "vai-Vaii".  Does not yet exist on Unix.  */
		/* Mac OS X has "yue-Hans", "yue-Hant".
		 The default script for yue on Unix is Simplified Han.  */
		{ "yue-Hans",   "yue"  },
		/* Mac OS X has "zh-Hans", "zh-Hant".
		 Country codes are used to distinguish these on Unix.  */
		{ "zh-Hans",    "zh_CN"},

		{ "zh-Hant",    "zh_TW"},
		{ "zh-Hant-HK", "zh_HK"},
	};
	/* Convert script names (ISO 15924) to Unix conventions.
	   See https://www.unicode.org/iso15924/iso15924-codes.html  */
	typedef struct {
		const char script[4 + 1];
		const char unixy[9 + 1];
	} script_entry;
	static const script_entry script_table[] = {
		{"Arab",  "arabic"   },
		{ "Cyrl", "cyrillic" },
		{ "Latn", "latin"    },
		{ "Mong", "mongolian"}
	};
	char *name = get_CFLocale();
	/* Step 2: Convert using langtag_table and script_table.  */
	if ((strlen(name) == 7 || strlen(name) == 10) && name[2] == '-') {
		unsigned int i1, i2;
		i1 = 0;
		i2 = sizeof(langtag_table) / sizeof(langtag_entry);
		while (i2 - i1 > 1) {
			/* At this point we know that if name occurs in langtag_table,
			   its index must be >= i1 and < i2.  */
			unsigned int         i = (i1 + i2) >> 1;
			const langtag_entry *p = &langtag_table[i];
			if (strcmp(name, p->langtag) < 0)
				i2 = i;
			else
				i1 = i;
		}
		if (strncmp(name, langtag_table[i1].langtag, strlen(langtag_table[i1].langtag)) == 0) {
			strcpy(name, langtag_table[i1].unixy);
			return name;
		}

		i1 = 0;
		i2 = sizeof(script_table) / sizeof(script_entry);
		while (i2 - i1 > 1) {
			/* At this point we know that if (name + 3) occurs in script_table,
			   its index must be >= i1 and < i2.  */
			unsigned int        i = (i1 + i2) >> 1;
			const script_entry *p = &script_table[i];
			if (strcmp(name + 3, p->script) < 0)
				i2 = i;
			else
				i1 = i;
		}
		if (strcmp(name + 3, script_table[i1].script) == 0) {
			name[2] = '@';
			strcpy(name + 3, script_table[i1].unixy);
			return name;
		}
	}

	/* Step 3: Convert new-style dash to Unix underscore. */
	{
		char *p;
		for (p = name; *p != '\0'; p++)
			if (*p == '-')
				*p = '_';
	}
	return name;
}

/* Conditional libintl */
bool libintl_available(void) {
	static bool  avail          = false;
	static void *libintl_handle = NULL;

	if (avail)
		return avail;

	if (PTR_TYPE_DLSYM(NULL, gettext) &&
	    PTR_TYPE_DLSYM(NULL, bindtextdomain) &&
	    PTR_TYPE_DLSYM(NULL, textdomain) &&
	    PTR_TYPE_DLSYM(NULL, bind_textdomain_codeset)) {
		avail = true;
		DBGLOG(CFSTR("Avail as direct: %p %p %p %p"), gettext_ptr, bindtextdomain_ptr, textdomain_ptr, bind_textdomain_codeset_ptr);
	} else if (PTR_TYPE_NAME_DLSYM(NULL, gettext, libintl_gettext) &&
	    PTR_TYPE_NAME_DLSYM(NULL, bindtextdomain, libintl_bindtextdomain) &&
	    PTR_TYPE_NAME_DLSYM(NULL, textdomain, libintl_textdomain) &&
	    PTR_TYPE_NAME_DLSYM(NULL, bind_textdomain_codeset, libintl_bind_textdomain_codeset)) {
		avail = true;
		DBGLOG(CFSTR("Avail as direct (libintl_*): %p %p %p %p"), gettext_ptr, bindtextdomain_ptr, textdomain_ptr, bind_textdomain_codeset_ptr);
	}

	if (!avail) {
		if (!libintl_handle) {
			int i;
			for (i = 0; libintl_paths[i] != NULL; i++) {
				libintl_handle = dlopen(libintl_paths[i], RTLD_LAZY);
				if (libintl_handle)
					break;
			}
			// DBGALT("Using libintl: %s", libintl_paths[i], "OK");
		}

		if (libintl_handle) {
			if (PTR_TYPE_DLSYM(libintl_handle, gettext) &&
			    PTR_TYPE_DLSYM(libintl_handle, bindtextdomain) &&
			    PTR_TYPE_DLSYM(libintl_handle, textdomain) &&
			    PTR_TYPE_DLSYM(libintl_handle, bind_textdomain_codeset)) {
				avail = true;
				DBGLOG(CFSTR("Avail as dlsym: %p %p %p %p"), gettext_ptr, bindtextdomain_ptr, textdomain_ptr, bind_textdomain_codeset_ptr);
			} else if (PTR_TYPE_NAME_DLSYM(libintl_handle, gettext, libintl_gettext) &&
			    PTR_TYPE_NAME_DLSYM(libintl_handle, bindtextdomain, libintl_bindtextdomain) &&
			    PTR_TYPE_NAME_DLSYM(libintl_handle, textdomain, libintl_textdomain) &&
			    PTR_TYPE_NAME_DLSYM(libintl_handle, bind_textdomain_codeset, libintl_bind_textdomain_codeset)) {
				DBGLOG(CFSTR("Avail as dlsym (libintl_*): %p %p %p %p"), gettext_ptr, bindtextdomain_ptr, textdomain_ptr, bind_textdomain_codeset_ptr);
				avail = true;
			}
		}
	}
	return avail;
}

/* Conditional libgtk */
bool gtk_available(void) {
	static bool  avail          = false;
	static void *libgtk3_handle = NULL;

	if (avail)
		return avail;

	if (PTR_TYPE_DLSYM(NULL, gtk_dialog_get_type) &&
	    PTR_TYPE_DLSYM(NULL, gtk_message_dialog_new) &&
	    PTR_TYPE_DLSYM(NULL, gtk_dialog_add_button) &&
	    PTR_TYPE_DLSYM(NULL, gtk_dialog_run) &&
	    PTR_TYPE_DLSYM(NULL, gtk_widget_destroy))
		avail = true;

	if (!avail) {
		if (!libgtk3_handle) {
			for (int i = 0; libgtk3_paths[i] != NULL; i++) {
				libgtk3_handle = dlopen(libgtk3_paths[i], RTLD_LAZY);
				if (libgtk3_handle)
					break;
			}
		}

		if (libgtk3_handle) {
			if (PTR_TYPE_DLSYM(libgtk3_handle, gtk_dialog_get_type) &&
			    PTR_TYPE_DLSYM(libgtk3_handle, gtk_message_dialog_new) &&
			    PTR_TYPE_DLSYM(libgtk3_handle, gtk_dialog_add_button) &&
			    PTR_TYPE_DLSYM(libgtk3_handle, gtk_dialog_run) &&
			    PTR_TYPE_DLSYM(libgtk3_handle, gtk_widget_destroy))
				avail = true;
		}
	}
	return avail;
}

#if TARGET_OS_IPHONE
id find_top_controller(id root) {
	if (NSObjectIsKindOfClass(root, UINavigationController)) {
		return find_top_controller(ocall(0, root, topViewController));
	} else if (objc_opt_isKindOfClass(root, oclass(UITabBarController))) {
		return find_top_controller(ocall(0, root, selectedViewController));
	} else {
		id pvc = UIViewControllerGetPresentedViewController(root);
		if (pvc)
			return find_top_controller(pvc);
	}
	return root;
}
#endif

const char *L_OK;
const char *L_FAILED;
const char *L_ERR;
const char *L_NONE;
const char *L_MA;
const char *L_MAH;
const char *L_MV;
const char *L_TRUE;
const char *L_FALSE;
/* This function is for speed up PO generation */
void init_common_text(void) {
    static bool done = false;
    if (!done) {
        L_OK     = _C("OK");
        L_FAILED = _C("Failed");
        L_ERR    = _C("Error");
        L_NONE   = _C("None"); // Consider "None" => "N/A"
        L_MA     = _C("mA");
        L_MAH    = _C("mAh");
        L_MV     = _C("mV");
        L_TRUE   = _C("True");
        L_FALSE  = _C("False");
    }
    done = true;
}

/* Alert for multiple scene */
/* TODO: Check if program running under SSH */
// It should NOT be a blocking function bc
// main thread would not be able to call it if so
void show_alert(const char *title, const char *message, const char *button) {
	show_alert_async(title, message, button, NULL);
}

void show_fatal_overlay_async(const char *title, const char *message) {
	/* FIXME: This seems completely black on Sims */
	CFURLRef    obkit_path = CFURLCreateWithFileSystemPath(NULL, CFSTR("/System/Library/PrivateFrameworks/OnBoardingKit.framework"), kCFURLPOSIXPathStyle, 1);
	CFBundleRef obkit      = CFBundleCreate(NULL, obkit_path);
	CFRelease(obkit_path);
	if (!CFBundleLoadExecutable(obkit)) {
		CFRelease(obkit);
		show_alert_async_f(title, message, L_OK, (void *)app_exit);
		return;
	}
	Class safc_cls = objc_getClass("OBSetupAssistantFinishedController");
	if (!safc_cls || !class_respondsToSelector(safc_cls, oselector(initWithTitle:detailText:))) {
		CFRelease(obkit);
		show_alert_async_f(title, message, L_OK, (void *)app_exit);
		return;
	}
	CFStringRef title_str = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
	CFStringRef msg_str   = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
	id          safc      = ocall(2, objc_alloc(safc_cls), initWithTitle:detailText:, title_str, msg_str);
	CFRelease(title_str);
	CFRelease(msg_str);
	if (!safc) {
		CFRelease(obkit);
		show_alert_async_f(title, message, L_OK, (void *)app_exit);
		return;
	}
	if (class_respondsToSelector(safc_cls, oselector(setInstructionalText:)))
		ocall(1, safc, setInstructionalText:, _("Swipe up to exit"));
	UIWindowSetRootViewController(UIApplicationGetKeyWindow(UIApplicationSharedApplication()), safc);
	NSObjectRelease(safc);
}

static void show_alert_async_f__invoke(char *block) {
	(*((void (**)(BOOL))(block + 32)))(1);
}
void show_alert_async_f(const char *title, const char *message, const char *button, void (*completion)(int)) {
	id block = NULL;
	if (completion)
		block = objc_make_block(show_alert_async_f__invoke, completion);
	show_alert_async(title, message, button, block);
	objc_release(block);
}

static void show_alert__invoke(CFStringRef *all_strings) {
	extern id          gWindow;
	UIViewController  *topController = find_top_controller(UIWindowGetRootViewController(gWindow));
	UIAlertController *alert         = UIAlertControllerCreate(all_strings[0], all_strings[1], UIAlertControllerStyleAlert);
	UIAlertControllerAddAction(alert, UIAlertActionCreate(all_strings[2], UIAlertActionStyleDefault, (id)all_strings[3]));
	NSObjectRelease((NSObject *)all_strings[3]);
	UIViewController *pvc  = topController;
	UIViewController *pvcp = UIViewControllerGetPresentedViewController(pvc);
	if (pvcp)
		pvc = pvcp;
	UIViewControllerPresentViewController(pvc, alert, 1, NULL);
	CFRelease(all_strings[0]);
	CFRelease(all_strings[1]);
	CFRelease(all_strings[2]);
	free(all_strings);
}

void show_alert_async(const char *title, const char *message, const char *button, void *completion_block) {
	DBGLOG(CFSTR("show_alert called: [%s], [%s], [%s]"), title, message, button);

	NSObjectRetain(completion_block);
	/* Alert in GTK+ if under Xfce / GNOME */
	/* this check may not accurate */
	if (gtk_available() && getenv("DISPLAY")) {
		GtkWidget *dialog = gtk_message_dialog_new_ptr(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, "%s", message);
		gtk_dialog_add_button_ptr(GTK_DIALOG(dialog), button, GTK_RESPONSE_CANCEL);
		// gtk_dialog_add_button(GTK_DIALOG(dialog), "OK", GTK_RESPONSE_ACCEPT);

		int response = gtk_dialog_run_ptr(GTK_DIALOG(dialog));
		gtk_widget_destroy_ptr(dialog);
		if (completion_block) {
			((void (*)(void *, BOOL))((char *)completion_block + 16))(completion_block, response != GTK_RESPONSE_CANCEL);
			objc_release(completion_block);
		}
	}
#if TARGET_OS_IPHONE
	/* Alert using system UIAlert */
	if (__builtin_available(iOS 9.0, *)) {
		CFStringRef *all_strings = malloc(sizeof(CFStringRef) * 4);
		all_strings[0]           = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
		all_strings[1]           = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
		all_strings[2]           = CFStringCreateWithCString(NULL, button, kCFStringEncodingUTF8);
		all_strings[3]           = completion_block;
		// Use UIAlertController if iOS 10 or later

		dispatch_async_f(dispatch_get_main_queue(), all_strings, (void (*)(void *))show_alert__invoke);
	} else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		// Use UIAlertView for iOS 9 or earlier
		/* TODO: Add a delegate */
		id          alert_ninit = objc_alloc(oclass(UIAlertView));
		CFStringRef title_str   = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
		CFStringRef msg_str     = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
		CFStringRef btn_str     = CFStringCreateWithCString(NULL, button, kCFStringEncodingUTF8);

		id          alert       = ocall(5, alert_ninit, initWithTitle:message:delegate:cancelButtonTitle:otherButtonTitles:, title_str, msg_str, NULL, btn_str, NULL);
		ocall(0, alert, show);
		// alloc_init'ed objects need to be released
		objc_release(alert);
		CFRelease(title_str);
		CFRelease(msg_str);
		CFRelease(btn_str);
		// not calling completion bc this is NON BLOCKING!!
		objc_release(completion_block);
#pragma clang diagnostic pop
	}
#elif TARGET_OS_OSX
	id          alert     = objc_alloc_init(oclass(NSAlert));
	CFStringRef title_str = CFStringCreateWithCString(NULL, title, kCFStringEncodingUTF8);
	CFStringRef msg_str   = CFStringCreateWithCString(NULL, message, kCFStringEncodingUTF8);
	CFStringRef btn_str   = CFStringCreateWithCString(NULL, button, kCFStringEncodingUTF8);

	ocall(1, alert, setMessageText : title_str);
	ocall(1, alert, setInformativeText:, msg_str);
	ocall(1, alert, addButtonWithTitle:, btn_str);
	ocall(0, alert, runModal);

	if (completion_block) {
		((void (*)(void *, BOOL))((char *)completion_block + 16))(completion_block, 1);
		objc_release(completion_block);
	}
	objc_release(alert);
#endif
}

void app_exit(void) {
	if (is_carbon()) {
#if TARGET_OS_IOS
		extern void ios_app_exit(void);
		ios_app_exit();
#endif
#if TARGET_OS_OSX
#endif
	} else {
		// TODO: CLI & X11 logic
	}

	// Fallback to C exit
	exit(0);
}

/* UIApplicationMain/NSApplicationMain only works when App launched with NSBundle */
/* FIXME: NSBundle still exists if with Info.plist, we need a better detection */
bool is_carbon(void) {
	return (CFBundleGetMainBundle() && getenv("XPC_SERVICE_NAME"));
}

#if TARGET_OS_IPHONE
static void __open_url__invoke(CFURLRef urlRef) {
	CFDictionaryRef emptyDict = CFDictionaryCreate(NULL, NULL, NULL, 0, NULL, NULL);
	UIApplicationOpenURL(UIApplicationSharedApplication(), urlRef, emptyDict, NULL);
	CFRelease(emptyDict);
	CFRelease(urlRef);
}
#endif

void open_url(const char *url) {
	/* TODO: open url inside DE */

	CFStringRef urlStr = CFStringCreateWithCString(NULL, url, kCFStringEncodingUTF8);
	CFURLRef    urlRef = CFURLCreateWithString(NULL, urlStr, NULL);
	CFRelease(urlStr);
	if (!urlRef)
		return;
#if TARGET_OS_IPHONE
	// Ensure URL opening is done on the main thread.
	dispatch_async_f(dispatch_get_main_queue(), (void *)urlRef, (void (*)(void *))__open_url__invoke);
#endif
#if TARGET_OS_OSX
	ocall(1, ocall(0, oclass(NSWorkspace), sharedWorkspace), openURL:, urlRef);
	CFRelease(urlRef);
#endif
}

bool match_regex(const char *string, const char *pattern) {
	regex_t regex;
	if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
		return 0;
	int result = regexec(&regex, string, 0, NULL, 0);
	regfree(&regex);
	return result == 0;
}

// For iOS 12 or ealier, we generate image directly from 'SF-Pro-Display-Regular.otf'
id imageForSFProGlyph(CFStringRef glyph, CFStringRef fontName, CGFloat fontSize, id tintColor) {
	// CGFloat (double) can NOT be treated as void* bc it uses a different register
	id              font  = ((id(*)(Class, SEL, CFStringRef, CGFloat))objc_msgSend)(oclass(UIFont), oselector(fontWithName:size:), fontName, fontSize)
	                  ?: ((id(*)(Class, SEL, CGFloat))objc_msgSend)(oclass(UIFont), oselector(systemFontOfSize:), fontSize);

	CFDictionaryRef attrs = CFDictionaryCreate(NULL, (const void *[]) { NSFontAttributeName, NSForegroundColorAttributeName }, (const void *[]) { font, tintColor }, 2, NULL, NULL);
	/*NSDictionary *attrs = @{
	    NSFontAttributeName: font,
	    NSForegroundColorAttributeName: tintColor
	};*/

	CGSize          sz    = ((CGSize(*)(id, SEL, CFDictionaryRef))objc_msgSend)((id)glyph, oselector(sizeWithAttributes), attrs);
	UIGraphicsBeginImageContextWithOptions(sz, NO, 0);
	((void (*)(id, SEL, CGPoint, CFDictionaryRef))objc_msgSend)((id)glyph, oselector(drawAtPoint:withAttributes:), CGPointZero, attrs);
	CFRelease(attrs);
	id img = objc_retain((id)UIGraphicsGetImageFromCurrentImageContext());
	UIGraphicsEndImageContext();

	// template so tintColor still applies if you change it later
	id ret = ocall(1, img, imageWithRenderingMode:, /*UIImageRenderingModeAlwaysTemplate*/ 2);
	objc_release(img);

	return ret;
}

// HTML operations broken on Rosetta Sims pre iOS 14
int is_rosetta(void) {
	int    ret  = 0;
	size_t size = sizeof(ret);
	if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1) {
		if (errno == ENOENT)
			return 0;
		return -1;
	}
	return ret;
}

/* Consider use NSDefaults instead of file */
const char *lang_cfg_file(void) {
	char *home = getenv("HOME");
	if (match_regex(home, IOS_CONTAINER_FMT) || match_regex(home, MAC_CONTAINER_FMT)) {
		/* iOS/macOS sandboxed */
	} else if (match_regex(home, SIM_CONTAINER_FMT)) {
		/* Simulator sandboxed */
	} else {
		/* Unknown/Unsandboxed */
		return "/.config/Battman_LANG";
	}
	return "/Library/_LANG";
}

int open_lang_override(int flags, int mode) {
	const char *homedir = getenv("HOME");
	if (!homedir)
		return 0;
	char *langoverride_fn = malloc(strlen(homedir) + 20);
	stpcpy(stpcpy(langoverride_fn, homedir), lang_cfg_file());
	int fd = open(langoverride_fn, flags, mode);
	free(langoverride_fn);
	return fd;
}

static int _preferred_language_code = -1;

void preferred_language_code_clear(void) {
    _preferred_language_code = -1;
}

int preferred_language_code() {
	if (_preferred_language_code != -1)
		return _preferred_language_code;
	int lfd = open_lang_override(O_RDONLY, 0);
	if (lfd == -1) {
		char *lang = preferred_language();
		if (*(uint16_t *)lang == 0x6e7a) {
			_preferred_language_code = 1;
		} else {
			_preferred_language_code = 0;
		}
		free(lang);
		return _preferred_language_code;
	}
	int ret;
	if (read(lfd, &ret, 4) != 4) {
		close(lfd);
		_preferred_language_code = 0;
		return 0;
	}
	close(lfd);
	_preferred_language_code = ret;
	return ret;
}

const char *target_type(void) {
	static char *buf = NULL;

	if (buf != NULL)
		return buf;

	const char *name = "hw.targettype";
	size_t      len  = 0;

	if (sysctlbyname(name, NULL, &len, NULL, 0) != 0 || len == 0)
		return NULL;

	buf = malloc(len + 1);
	if (!buf)
		return NULL;

	if (sysctlbyname(name, buf, &len, NULL, 0) != 0) {
		free(buf);
		buf = NULL;
		return buf;
	}

	buf[len] = '\0';
	return buf;
}
