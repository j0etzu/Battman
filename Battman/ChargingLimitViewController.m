#import "ChargingLimitViewController.h"
#import "SliderTableViewCell.h"
#include "common.h"
#include "intlextern.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

enum { CL_SECTION_MAIN, CL_SECTION_COUNT };

int connect_to_daemon(void) {
    struct sockaddr_un sockaddr;
    sockaddr.sun_family = AF_UNIX;
    const char *end = stpcpy(sockaddr.sun_path, "./Library/dsck");
    sockaddr.sun_len = (unsigned char)(end - sockaddr.sun_path + 1);
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_un)) == -1) {
        // char errstr[1024];
        ////memset(errstr, 0, sizeof(errstr));
        //// ^ no initialization needed for sprintf
        // sprintf(errstr, "%s\n%s: %s", _C("Failed to connect to daemon"), L_ERR, strerror(errno));
        // show_alert(L_ERR, errstr, L_OK);
        close(sock);
        // No alert bc we need to retry
        return 0;
    }
    char cmd = 2;
    write(sock, &cmd, 1);
    if (read(sock, &cmd, 1) != 1 || cmd != 2) {
        NSLog(@"Failed to ping daemon: %s", strerror(errno));
        show_alert(L_FAILED, "The daemon may not be working properly.", L_OK);
        close(sock);
        return 0;
        ;
    }
    NSLog(@"Connected to daemon and received ping!");
    return sock;
}

@implementation ChargingLimitViewController

- (NSString *)title {
	return _("Charging Limit");
}

- (instancetype)init {
    if (@available(iOS 13.0, *)) {
        self = [super initWithStyle:UITableViewStyleInsetGrouped];
    } else {
        self = [super initWithStyle:UITableViewStyleGrouped];
    }
    [self.tableView registerClass:[SliderTableViewCell class] forCellReuseIdentifier:@"clhighthr"];
    [self.tableView registerClass:[SliderTableViewCell class] forCellReuseIdentifier:@"cllowthr"];

    char buf[1024];
    char *end = stpcpy(stpcpy(buf, getenv("HOME")), "/Library/daemon");
    strcpy(end, ".run");
    int drfd = open(buf, O_RDONLY);
    if (drfd != -1) {
        int pid;
        if (read(drfd, &pid, 4) == 4) {
            if (kill(pid, 0) == 0 || errno != ESRCH) {
                daemon_pid = pid;
            }
        }
        close(drfd);
    }

    strcpy(end, "_settings");
    int fd = open(buf, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        NSLog(@"open %s: Error - %s", buf, strerror(errno));
        show_alert(L_ERR, _C("Failed to open daemon settings file"), L_OK);
        vals = NULL;
        return self;
    }
    char _vals[2];
    if (read(fd, _vals, 2) != 2) {
        NSLog(@"Writing initial values to daemon_settings");
        _vals[0] = -1;
        _vals[1] = -1;
        lseek(fd, 0, SEEK_SET);
        write(fd, _vals, 2);
    }
    lseek(fd, 0, SEEK_SET);
    vals = mmap(NULL, 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ((long long)vals == -1) {
        NSLog(@"mmap: Error - %s", strerror(errno));
        show_alert(L_ERR, _C("File mapping failed"), L_OK);
        vals = NULL;
        close(fd);
        return self;
    }
    close(fd);
    if (daemon_pid) {
        NSLog(@"Daemon likely valid, trying to connect");
        [self connectToDaemon];
    }
    return self;
}

- (void)connectToDaemon {
    if (daemon_fd)
        return;
    daemon_fd = connect_to_daemon();
}

- (NSString *)tableView:(UITableView *)tv titleForHeaderInSection:(NSInteger)section {
    if (section == 0)
        return _("Charging Limit (Experimental)");
    return nil;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section == 0)
        return _("Charging Limit uses a background service to monitor your battery's charge level and automatically adjust charging behavior. You need to restart the service after changing the configuration.");
    return nil;
}

- (void)dealloc {
    if (!vals)
        return;
    const char endconnectioncmd = 5;
    write(daemon_fd, &endconnectioncmd, 1);
    close(daemon_fd);
    munmap(vals, 2);
}

- (NSInteger)tableView:(id)tv numberOfRowsInSection:(NSInteger)sect {
    return 7;
}

- (NSInteger)numberOfSectionsInTableView:(id)tv {
    return 1;
}

- (void)tableView:(UITableView *)tv didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.row == 6) {
        if (daemon_pid) {
            NSLog(@"Daemon is likely active, requesting stop");
            if (!daemon_fd) {
                show_alert(L_ERR, _C("Unable to connect to the daemon."), L_OK);
                [tv deselectRowAtIndexPath:indexPath animated:YES];
                return;
            }
            char stop_cmd = 3;
            write(daemon_fd, &stop_cmd, 1);
            if (read(daemon_fd, &stop_cmd, 1) == 1 && stop_cmd == 3) {
                NSLog(@"Daemon returned 3 - stopped");
                daemon_pid = 0;
                close(daemon_fd);
                daemon_fd = 0;
            }
            [tv reloadData];
        } else {
            extern int battman_run_daemon(void);
            daemon_pid = battman_run_daemon();
            for (int i = 0; i < 30; i++) {
                usleep(50000);
                [self connectToDaemon];
                if (daemon_fd) {
                    break;
                } else if (i == 29) {
                    show_alert(L_FAILED, _C("Couldn't start the daemon — it isn’t responding."), L_OK);
                }
            }
            [tv reloadData];
        }
    }
    [tv deselectRowAtIndexPath:indexPath animated:YES];
}

- (void)cltypechanged:(UISegmentedControl *)segCon {
    vals[0] = segCon.selectedSegmentIndex ? 0 : -1;
    [self daemonRedecide];
    /* Refreshing the whole tableview causes animation lost */
    NSIndexPath *resumeIndexLabel = [NSIndexPath indexPathForRow:3 inSection:0];
    NSIndexPath *resumeIndexSlider = [NSIndexPath indexPathForRow:4 inSection:0];
    [self.tableView reloadRowsAtIndexPaths:@[ resumeIndexLabel, resumeIndexSlider ] withRowAnimation:UITableViewRowAnimationFade];
}

- (UITableViewCell *)tableView:(UITableView *)tv cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [UITableViewCell new];
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    if (indexPath.row == 0) {
        cell.textLabel.text = _("When limit is reached");
        NSArray *items;
        if (@available(iOS 13.0, *)) {
			// XXX: Consider use dynamic check
			if (@available(iOS 14.0, *)) {
				items = @[ [UIImage systemImageNamed:@"pause.rectangle"], [UIImage systemImageNamed:@"arrow.rectanglepath"] ];
			} else {
				// Sadly arrow.rectanglepath is iOS 14+
				items = @[ [UIImage systemImageNamed:@"pause.circle"], [UIImage systemImageNamed:@"arrow.triangle.2.circlepath"] ];
			}
        } else {
            // pause.rectangle U+10029B
            // arrow.rectanglepath U+1008C1
            items = @[ @"􀊛", @"􀣁" ];
        }
        UISegmentedControl *segCon = [[UISegmentedControl alloc] initWithItems:items];
        if (@available(iOS 13.0, *)) {
            // Handle something?
        } else {
            [segCon setTitleTextAttributes:[NSDictionary dictionaryWithObjectsAndKeys:[UIFont fontWithName:@SFPRO size:12.0], NSFontAttributeName, nil] forState:UIControlStateNormal];
        }

        if (vals[0] == -1) {
            segCon.selectedSegmentIndex = 0;
        } else {
            segCon.selectedSegmentIndex = 1;
        }
        [segCon addTarget:self action:@selector(cltypechanged:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView = segCon;
        return cell;
    } else if (indexPath.row == 1) {
        cell.textLabel.text = _("Limit charging at (%)");
        return cell;
    } else if (indexPath.row == 2) {
        SliderTableViewCell *scell = [tv dequeueReusableCellWithIdentifier:@"clhighthr" forIndexPath:indexPath];
        if (!scell) {
            scell = [[SliderTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"clhighthr"];
        }
        scell.slider.minimumValue = 0;
        scell.slider.maximumValue = 100;
        scell.slider.enabled = 1;
        scell.textField.enabled = 1;
        scell.delegate = (id)self;
        if (vals[1] == -1) {
            scell.slider.value = 100;
            scell.textField.text = @"100";
        } else {
            scell.slider.value = (float)vals[1];
            scell.textField.text = [NSString stringWithFormat:@"%d", (int)vals[1]];
        }
        return scell;
    } else if (indexPath.row == 3) {
        if (vals[0] == -1)
            cell.textLabel.enabled = NO;
        cell.textLabel.text = _("Resume charging at (%)");
        return cell;
    } else if (indexPath.row == 4) {
        SliderTableViewCell *scell = [tv dequeueReusableCellWithIdentifier:@"cllowthr" forIndexPath:indexPath];
        if (!scell) {
            scell = [[SliderTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"cllowthr"];
        }
        scell.slider.minimumValue = 0;
        scell.slider.maximumValue = 100;
        scell.delegate = (id)self;
        if (vals[0] == -1) {
            scell.slider.enabled = NO;
            scell.slider.userInteractionEnabled = NO;
            scell.slider.value = 0;
            scell.textField.enabled = 0;
            scell.textField.userInteractionEnabled = 0;
            scell.textField.text = @"0";
        } else {
            scell.slider.enabled = 1;
            scell.slider.userInteractionEnabled = 1;
            scell.slider.value = (float)vals[0];
            scell.textField.enabled = 1;
            scell.textField.userInteractionEnabled = 1;
            scell.textField.text = [NSString stringWithFormat:@"%d", (int)vals[0]];
        }
        return scell;
    } else if (indexPath.row == 5) {
        if (daemon_pid) {
            cell.textLabel.text = [NSString stringWithFormat:@"%@ (%@: %d)", _("Daemon is active"), _("PID"), daemon_pid];
        } else {
            cell.textLabel.text = _("Daemon is inactive");
        }
        return cell;
    } else if (indexPath.row == 6) {
        cell.selectionStyle = UITableViewCellSelectionStyleDefault;
        if (daemon_pid) {
            cell.textLabel.text = _("Stop Daemon (Disable Charging Limit)");
        } else {
            cell.textLabel.text = _("Start Daemon (Enforce Charging Limit)");
        }
        if (@available(iOS 13.0, *)) {
            cell.textLabel.textColor = [UIColor linkColor];
        } else {
            cell.textLabel.textColor = [UIColor colorWithRed:0 green:(122.0f / 255) blue:1 alpha:1];
        }
    }
    return cell;
}

- (void)sliderTableViewCell:(SliderTableViewCell *)cell didChangeValue:(float)value {
    BOOL isHighThr = [cell.reuseIdentifier isEqualToString:@"clhighthr"];
    // TODO: Consider do a auto slider adjusting instead of this
    if (isHighThr && value < vals[0]) {
        show_alert(_C("Invalid Setup"), _C("Limit Value should be bigger than Resume Value"), L_OK);
        [self.tableView reloadData];
        return;
    } else if (!isHighThr && value > vals[1]) {
        show_alert(_C("Invalid Setup"), _C("Resume Value should be smaller than Limit Value"), L_OK);
        [self.tableView reloadData];
        return;
    }
    vals[isHighThr] = (char)value;
    [self daemonRedecide];
}

- (void)daemonRedecide {
    const char redecidecmd = 4;
    write(daemon_fd, &redecidecmd, 1);
}

@end
