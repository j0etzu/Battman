#pragma once

#include <CoreFoundation/CoreFoundation.h>

#define kBattmanFirstLaunchKey CFSTR("com.torrekie.Battman.1stLaunch")
#define kBattmanLaunchCountKey CFSTR("com.torrekie.Battman.LaunchCnt")
#define kBattmanDonateShownKey CFSTR("com.torrekie.Battman.SpsrShown")

__BEGIN_DECLS

// Check if previously auto triggered this page
bool donation_shown(void);

// manually push a UINavigationController containing our DonationViewController anywhere
void show_donation(void);

// show_donation() but still obey to the conditions
void donation_prompter_request_check(void);

__END_DECLS
