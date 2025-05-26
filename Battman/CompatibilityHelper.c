#include <Availability.h>

#if __IPHONE_OS_VERSION_MAX_ALLOWED < 110000

#include <string.h>

// TODO: Need confirm, I actually see memcpy and memset defs in iPhoneOS9.3.sdk
/* memccpy, memcpy, mempcpy, memmove, memset, strcpy, strlcpy, stpcpy,
   strncpy, stpncpy, strcat, strlcat, and strncat */

void *memcpy(void *a, const void *b, unsigned long c) {
	return __builtin___memcpy_chk(a, b, c, c);
}

void *memset(void *a, int b, unsigned long c) {
	return __builtin___memset_chk(a, b, c, c);
}

int strcmp(const char *a, const char *b) {
	extern int _platform_strcmp(const char *, const char *);
	return _platform_strcmp(a, b);
}

int strncmp(const char *a, const char *b, unsigned long c) {
	extern int _platform_strncmp(const char *, const char *, unsigned long);
	return _platform_strncmp(a, b, c);
}

void _Unwind_Resume(void) { }

#endif
