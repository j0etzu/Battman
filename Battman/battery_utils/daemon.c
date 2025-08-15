// Avoid libiosexec
#ifndef LIBIOSEXEC_INTERNAL
#define LIBIOSEXEC_INTERNAL 1
#endif
#ifdef posix_spawn
#undef posix_spawn
#endif
#define LIBIOSEXEC_H

#include "../common.h"
#include "../iokitextern.h"
#include "libsmc.h"
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFString.h>
#include <errno.h>
#include <fcntl.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#if __has_include(<spawn_private.h>)
#include <spawn_private.h>
#else
extern int posix_spawnattr_set_persona_np(const posix_spawnattr_t *__restrict, uid_t, uint32_t);
extern int posix_spawnattr_set_persona_uid_np(const posix_spawnattr_t *__restrict, uid_t);
extern int posix_spawnattr_set_persona_gid_np(const posix_spawnattr_t *__restrict, uid_t);
extern int posix_spawnattr_setprocesstype_np(posix_spawnattr_t *, int);
extern int posix_spawnattr_setjetsam_ext(posix_spawnattr_t *, int, int, int, int);
#endif

extern char **environ;

extern void subscribeToPowerEvents(void (*cb)(int, io_registry_entry_t, int32_t));

struct battman_daemon_settings {
	unsigned char enable_charging_at_level;
	unsigned char disable_charging_at_level;
};

static struct battman_daemon_settings *daemon_settings;
static int CH0ICache = -1;
static int CH0CCache = -1;
static int last_power_level = -1;
static char daemon_settings_path[1024];

static void update_power_level(int);

static void daemon_control_thread(int fd) {
	while (1) {
		char cmd;
		if (read(fd, &cmd, 1) <= 0) {
			NSLog(CFSTR("Daemon: Closing bc %s"), strerror(errno));
			close(fd);
			return;
		}
		NSLog(CFSTR("Daemon: READ cmd %d"), (int)cmd);
		if (cmd == 3) {
			char val = 0;
			smc_write_safe('CH0I', &val, 1);
			smc_write_safe('CH0C', &val, 1);
			write(fd, &cmd, 1);
			close(fd);
			exit(0);
		} else if (cmd == 2) {
			write(fd, &cmd, 1);
		} else if (cmd == 4) {
			update_power_level(last_power_level);
		} else if (cmd == 5) {
			close(fd);
			pthread_exit(NULL);
		} else if (cmd == 6) {
			// Redirect logs
			// Will stop responding to commands
			const char *connMsg = "Hello from daemon! Log redirection started!\n";
			write(fd, connMsg, strlen(connMsg));
			int pipefds[2];
			pipe(pipefds);
			dup2(pipefds[1], 1);
			dup2(pipefds[1], 2);
			close(pipefds[1]);
			int devnull = open("/dev/null", O_WRONLY);
			char buf[512];
			while (1) {
				ssize_t len = read(pipefds[0], buf, 512);
				if (len <= 0) {
					close(fd);
					dup2(devnull, 1);
					dup2(devnull, 2);
					close(devnull);
					close(pipefds[0]);
					pthread_exit(NULL);
				}
				if (write(fd, buf, len) <= 0) {
					dup2(devnull, 1);
					dup2(devnull, 2);
					close(devnull);
					close(pipefds[0]);
					pthread_exit(NULL);
				}
			}
		}
	}
}

static void daemon_control() {
	struct sockaddr_un sockaddr;
	sockaddr.sun_family = AF_UNIX;
	chdir(getenv("HOME"));
	strcpy(sockaddr.sun_path, "./Library/dsck");
	remove(sockaddr.sun_path);
	umask(0);
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (bind(sock, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_un)) != 0)
		abort();
	int trueVal = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &trueVal, 4);
	if (listen(sock, 2) != 0)
		abort();
	while (1) {
		int conn = accept(sock, NULL, NULL);
		if (conn == -1)
			continue;
		pthread_t ct;
		pthread_create(&ct, NULL, (void *(*)(void *))daemon_control_thread, (void *)(uint64_t)conn);
		pthread_detach(ct);
	}
}

static void update_power_level(int val) {
	if (val == -1)
		return;
	last_power_level = val;
	if (daemon_settings->enable_charging_at_level != 255) {
		if (val <= daemon_settings->enable_charging_at_level) {
			val = 0;
			if (val != CH0ICache) {
				CH0ICache = val;
				smc_write_safe('CH0I', &val, 1);
			}
			return;
		}
		NSLog(CFSTR("Going to disable at %d"), (int)daemon_settings->disable_charging_at_level);
		if (val >= daemon_settings->disable_charging_at_level) {
			val = 1;
			if (val != CH0ICache) {
				CH0ICache = val;
				NSLog(CFSTR("Disabling"));
				smc_write_safe('CH0I', &val, 1);
			}
			return;
		}
		return;
	}
	if (daemon_settings->disable_charging_at_level != 255) {
		if (val >= daemon_settings->disable_charging_at_level) {
			val = 1;
		} else {
			val = 0;
		}
		if (val != CH0CCache) {
			CH0CCache = val;
			smc_write_safe('CH0C', &val, 1);
		}
		return;
	}
}

static void powerevent_listener(int a, io_registry_entry_t b, int32_t c) {
	if (c != -536723200)
		return;
	if (access(daemon_settings_path, F_OK) == -1) {
		// Quit when app removed or daemon no longer needed
		exit(0);
	}
	CFNumberRef capacity = IORegistryEntryCreateCFProperty(b, CFSTR("CurrentCapacity"), 0, 0);
	int val;
	CFNumberGetValue(capacity, kCFNumberIntType, &val);
	NSLog(CFSTR("Daemon: Value=%d"), val);
	CFRelease(capacity);
	update_power_level(val);
}

void daemon_main(void) {
	signal(SIGPIPE, SIG_IGN);
	char *end = stpcpy(stpcpy(daemon_settings_path, getenv("HOME")), "/Library/daemon");
	strcpy(end, ".run");
	int runfd = open(daemon_settings_path, O_RDWR | O_CREAT, 0666);
	pid_t pid = getpid();
	write(runfd, &pid, 4);
	close(runfd);
	strcpy(end, "_settings");
	int settingsfd = open(daemon_settings_path, O_RDONLY);
	if (settingsfd == -1)
		exit(0); // Not enabled
	daemon_settings = mmap(NULL, sizeof(struct battman_daemon_settings), PROT_READ, MAP_SHARED, settingsfd, 0);
	if (!daemon_settings)
		exit(0);
	close(settingsfd);
	smc_open();
	subscribeToPowerEvents(powerevent_listener);
	pthread_t tmp;
	pthread_create(&tmp, NULL, (void *(*)(void *))daemon_control, NULL);
	pthread_join(tmp, NULL);
}

int battman_run_daemon(void) {
	posix_spawnattr_t sattr;
	posix_spawnattr_init(&sattr);
	posix_spawnattr_set_persona_np(&sattr, 99, 1);
	posix_spawnattr_set_persona_uid_np(&sattr, 0);
	posix_spawnattr_set_persona_gid_np(&sattr, 0);
	posix_spawnattr_setprocesstype_np(&sattr, 0x300); // daemon standard
	posix_spawnattr_setjetsam_ext(&sattr, 0, 3, 80, 80);
#ifndef POSIX_SPAWN_SETSID
#define POSIX_SPAWN_SETSID 0x400
#endif
	posix_spawnattr_setflags(&sattr, POSIX_SPAWN_SETSID);
	char executable[1024];
	uint32_t size = 1024;
	_NSGetExecutablePath(executable, &size);
	char *newargv[] = {executable, "--daemon", NULL};
	pid_t dpid;
	int err = posix_spawn(&dpid, executable, NULL, &sattr, (char **)newargv, environ);
	if (err != 0) {
		show_alert(L_ERR, strerror(err), L_OK);
	}
	posix_spawnattr_destroy(&sattr);
	return dpid;
}
