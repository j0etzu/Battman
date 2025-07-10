#include <assert.h>
#include <notify.h>
#include <stdio.h>
#include <string.h>

#include <libkern/OSThermalNotification.h>

static const char *notify_keys[] = {
	"com.apple.system.thermalSensorValues",
	"com.apple.system.thermalSensorValues2",
	"com.apple.system.thermalSensorValues3",
	"com.apple.system.thermalSensorValues4",
	"com.apple.system.thermalSensorValues5",
	"com.apple.system.thermalSensorValues6",
	"com.apple.system.thermalSensorValues7",
	"com.apple.system.thermalSensorValues8",
	"com.apple.system.thermalSensorValues9"
};

static inline void printLevels(void) {
	int         token;
	uint64_t    level;
	const char *name;

	for (int i = 0; i < 9; ++i) {
		token = 0;
		level = 0;
		name  = notify_keys[i];
		assert(NOTIFY_STATUS_OK == notify_register_check(name, &token));
		assert(NOTIFY_STATUS_OK == notify_get_state(token, &level));
		assert(NOTIFY_STATUS_OK == notify_cancel(token));
		printf("%d %d %d %d ", (int16_t)(level >> 0x00), (int16_t)(level >> 0x10), (int16_t)(level >> 0x20), (int16_t)(level >> 0x30));
	}
	putchar('\n');
}

int main(int argc, const char *argv[]) {
	const char *key;
	uint64_t    level;
	int         token;

	token = 0;
	level = 0;
	if (argc == 3) {
		if (!strncmp(argv[1], "pressure", 8)) {
			key = argv[2];
			if (!strncmp(key, "nominal", 7)) {
				level = kOSThermalPressureLevelNominal;
#if !(TARGET_OS_OSX || TARGET_OS_MACCATALYST)
			} else if (!strncmp(key, "light", 5)) {
				level = kOSThermalPressureLevelLight;
#endif
			} else if (!strncmp(key, "moderate", 8)) {
				level = kOSThermalPressureLevelModerate;
			} else if (!strncmp(key, "heavy", 5)) {
				level = kOSThermalPressureLevelHeavy;
			} else if (!strncmp(key, "trapping", 8)) {
				level = kOSThermalPressureLevelTrapping;
			} else if (!strncmp(key, "sleeping", 8)) {
				level = kOSThermalPressureLevelSleeping;
			} else {
				printf("usage:  %s %s {", *argv, argv[1]);
#if !(TARGET_OS_OSX || TARGET_OS_MACCATALYST)
				printf("nominal|light|moderate|heavy|trapping|sleeping");
#else
				printf("nominal|moderate|heavy|trapping|sleeping");
#endif
				puts("}");
				return EXIT_SUCCESS;
			}

			assert(NOTIFY_STATUS_OK == notify_register_check(kOSThermalNotificationPressureLevelName, &token));
			assert(NOTIFY_STATUS_OK == notify_set_state(token, level));
			assert(NOTIFY_STATUS_OK == notify_post(kOSThermalNotificationPressureLevelName));
		}
	}
	if (argc == 2) {
#if !(TARGET_OS_OSX || TARGET_OS_MACCATALYST)
		level = atoi(argv[1]);
		assert(NOTIFY_STATUS_OK == notify_register_check(kOSThermalNotificationName, &token));
		assert(NOTIFY_STATUS_OK == notify_set_state(token, level));
		assert(NOTIFY_STATUS_OK == notify_post(kOSThermalNotificationName));
#else
		// Unsupported
		return EXIT_FAILURE;
#endif
	}

	if (argc == 1) {
#if !(TARGET_OS_OSX || TARGET_OS_MACCATALYST)
		assert(NOTIFY_STATUS_OK == notify_register_check(kOSThermalNotificationName, &token));
		assert(NOTIFY_STATUS_OK == notify_get_state(token, &level));
		assert(NOTIFY_STATUS_OK == notify_cancel(token));
		printf("OSNotification Level = %d\n", (int)level);
#endif

		assert(NOTIFY_STATUS_OK == notify_register_check(kOSThermalNotificationPressureLevelName, &token));
		assert(NOTIFY_STATUS_OK == notify_get_state(token, &level));
		assert(NOTIFY_STATUS_OK == notify_cancel(token));
		switch (level) {
		case kOSThermalPressureLevelNominal:
			key = "nominal";
			break;
#if !(TARGET_OS_OSX || TARGET_OS_MACCATALYST)
		case kOSThermalPressureLevelLight:
			key = "light";
			break;
#endif
		case kOSThermalPressureLevelModerate:
			key = "moderate";
			break;
		case kOSThermalPressureLevelHeavy:
			key = "heavy";
			break;
		case kOSThermalPressureLevelTrapping:
			key = "trapping";
			break;
		case kOSThermalPressureLevelSleeping:
			key = "sleeping";
			break;
		default:
			key = "unknown";
			break;
		}
		printf("thermal pressure level = %d (%s)\n", (int)level, key);
		printf("thermal levels = ");
		printLevels();

		assert(NOTIFY_STATUS_OK == notify_register_check("com.apple.system.maxthermalsensorvalue", &token));
		assert(NOTIFY_STATUS_OK == notify_get_state(token, &level));
		assert(NOTIFY_STATUS_OK == notify_cancel(token));
		printf("max thermal level = %d\n", (int16_t)level);
	}

	return EXIT_SUCCESS;
}
