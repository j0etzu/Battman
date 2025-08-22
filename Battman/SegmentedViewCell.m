#import "SegmentedViewCell.h"

/* TODO: Use WLSegmentedControls instead of UberSegmentedControl for iOS 12 or earlier */
@implementation SegmentedHVCViewCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        // This must exist for our custom labels to align
        self.textLabel.text = @"TITLE";
        // Hide
        self.textLabel.hidden = YES;
        self.accessoryView.hidden = YES;

        // Alternative title
        self.titleLabel = [[UILabel alloc] init];
        self.titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
        self.titleLabel.font = self.textLabel.font;
        [self.contentView addSubview:self.titleLabel];

        // Alternative detail
        _detailTextLabel = [[UILabel alloc] init];
        self.detailTextLabel.translatesAutoresizingMaskIntoConstraints = NO;
        // For use as our custom labels' template
        UITableViewCell *cell;
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:nil];
        cell.hidden = YES;
        [self.contentView addSubview:cell];
        cell.detailTextLabel.text = @"DETAIL";
        self.detailTextLabel.font = cell.detailTextLabel.font;
        self.detailTextLabel.textColor = cell.detailTextLabel.textColor;
        self.detailTextLabel.textAlignment = cell.detailTextLabel.textAlignment;
        [self.contentView addSubview:self.detailTextLabel];
        
        // Initialize segmented control with sample segment
        self.segmentedControl = [[UISegmentedControl alloc] initWithItems:@[@"0"]];
        self.segmentedControl.translatesAutoresizingMaskIntoConstraints = NO;
        [self.contentView addSubview:self.segmentedControl];

        // Controlled content
        self.subTitleLabel = [[UILabel alloc] init];
        self.subTitleLabel.translatesAutoresizingMaskIntoConstraints = NO;
        self.subTitleLabel.font = self.textLabel.font;
        [self.contentView addSubview:self.subTitleLabel];
        self.subDetailLabel = [[UILabel alloc] init];
        self.subDetailLabel.translatesAutoresizingMaskIntoConstraints = NO;
        self.subDetailLabel.font = cell.detailTextLabel.font;
        self.subDetailLabel.textAlignment = cell.detailTextLabel.textAlignment;
        [self.contentView addSubview:self.subDetailLabel];

        self.subDetailLabel.text = @"0 mA";
        self.subTitleLabel.text = @"0 mV";
        
        // Setup Auto Layout constraints
        [NSLayoutConstraint activateConstraints:@[
            // Title label constraints
            [self.titleLabel.topAnchor constraintEqualToAnchor:self.textLabel.topAnchor constant:(self.frame.size.height - self.textLabel.font.pointSize) / 2],
            [self.titleLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
            [self.titleLabel.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
            // Detail label
            [self.detailTextLabel.topAnchor constraintEqualToAnchor:self.textLabel.topAnchor constant:(self.frame.size.height - self.textLabel.font.pointSize) / 2],
            [self.detailTextLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
            [self.detailTextLabel.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
            
            // Segmented control constraints
            [self.segmentedControl.topAnchor constraintEqualToAnchor:self.titleLabel.bottomAnchor constant:8],
            [self.segmentedControl.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],
            [self.segmentedControl.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
            //[self.segmentedControl.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-8],
            
            // Controlled text
            [self.subTitleLabel.topAnchor constraintEqualToAnchor:self.segmentedControl.bottomAnchor constant:8],
            [self.subTitleLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
            [self.subTitleLabel.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
            [self.subDetailLabel.topAnchor constraintEqualToAnchor:self.segmentedControl.bottomAnchor constant:8],
            [self.subDetailLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
            [self.subDetailLabel.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
            // Bottom
            [self.subDetailLabel.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-12],
        ]];
    }
    return self;
}

- (id)detailTextLabel {return _detailTextLabel;}

@end

#include "battery_utils/libsmc.h"
#include "battery_utils/not_charging_reason.h"

@implementation SegmentedFlagViewCell
#define USE_TEXT_FLAGS
#ifndef USE_TEXT_FLAGS
#define INIT    @"􀃲" // U+1000F2 checkmark.square
#define RSVD    @" "
#define BATHI   @"􀛨" // U+1006E8 battery.100
#define BATLOW  @"􀛩" // U+1006E9 battery.25
#define CHG_INH @"􀋫" // U+1002EB bolt.slash.circle
#define FC      @"􀢋" // U+10088B battery.100.bolt
#define FD      @"􀛪" // U+1006EA battery.0
#define CHG_SUS @"􀬘" // U+100B18 (missing after SF Symbols 2, consider do by ourself)
#define IMAX    @"􀂤" // U+1000A4 i.square
#define CHG     @"􀥤" // U+100964 poweron
#define DSG     @"􀥥" // U+100965 poweroff
#define SOC1    @"􀃊" // U+1000CA 1.square
#define SOCF    @"􀂞" // U+10009E f.square
#define RTA     @"􀐫" // U+10042B clock
#define OTA     @"􀙬" // U+10066C flame
#define TDA     @"􀛶" // U+100299 play.rectangle
#define TCA     @"􀊛" // U+10029B pause.rectangle
#define OCA     @"􀏇" // U+1003C7 plus.rectangle
#define RCA     @"􀖇" // U+100587 hourglass
#define TDD     @"􀋩" // U+1002E9 bolt.slash
#define ISD     @"􀈀" // U+100200 drop.triangle
#define BAT_DET @"􀈧" // U+100227 tray.and.arrow.down
#define CFGUPMODE @"􀯛" // U+100BDB clock.arrow.circlepath
#define ITPOR   @"􀊯" // U+1002AF arrow.triangle.2.circlepath
#define OCVTAKEN @"􀍾" // U+10037E speedometer
#define OT      @"􀇪" // U+1001EA thermometer.sun
#define UT      @"􀇫" // U+1001EB thermometer.snowflake
#define OTC     @"􀇘" // U+1001D8 cloud.sun.bolt
#define OTD     @"􀇔" // U+1001D4 cloud.sun
#define EEFAIL  @"􀩑" // U+100A51 externaldrive.badge.xmark
#define DODCorrect @"􀋉" // U+1002C9 flag
#define HW1     @"􀔪" // U+10052A 01.square
#define HW0     @"􀔩" // U+100529 00.square
#define EC3     @"􀃎" // U+1000CE 3.square
#define EC2     @"􀃌" // U+1000CC 2.square
#define EC1     @"􀃊" // U+1000CA 1.square
#define EC0     @"􀃈" // U+1000C8 0.square
#else
#define INIT    @"INIT"
#define RSVD    @"RSVD"
#define BATHI   @"BATHI"
#define BATLOW  @"BATLOW"
#define CHG_INH @"CHG_INH"
#define FC      @"FC"
#define FD      @"FD"
#define CHG_SUS @"CHG_SUS"
#define IMAX    @"IMAX"
#define CHG     @"CHG"
#define DSG     @"DSG"
#define SOC1    @"SOC1"
#define SOCF    @"SOCF"
#define RTA     @"RTA"
#define OTA     @"OTA"
#define TDA     @"TDA"
#define TCA     @"TCA"
#define OCA     @"OCA"
#define RCA     @"RCA"
#define TDD     @"TDD"
#define ISD     @"ISD"
#define BAT_DET @"BAT_DET"
#define CFGUPMODE @"CFGUPMODE"
#define ITPOR   @"ITPOR"
#define OCVTAKEN @"OCVTAKEN"
#define OT      @"OT"
#define UT      @"UT"
#define OTC     @"OTC"
#define OTD     @"OTD"
#define EEFAIL  @"EEFAIL"
#define DODCorrect @"DODC"
#define HW1     @"HW1"
#define HW0     @"HW0"
#define EC3     @"EC3"
#define EC2     @"EC2"
#define EC1     @"EC1"
#define EC0     @"EC0"
#endif


- (void)setBitSetByGuess {
    if (gGauge.FullChargeCapacity == 0) {
        NSLog(@"setBitSetByGuess: called too early!");
        return;
    }

    charger_data_t charger;
    get_charger_data(&charger);

    BOOL charging = (is_charging(NULL, NULL) > 0);
    BOOL charged = charger.NotChargingReason & NOT_CHARGING_REASON_FULLY_CHARGED;

    BOOL hb5, hb4, hb2fc, lb0dsg, lb7;
    hb5 = gGauge.Flags & 0x2000;
    hb4 = gGauge.Flags & 0x1000;
    hb2fc = (gGauge.Flags & 0x0200) && charging && charged;
    lb0dsg = (gGauge.Flags & 1) && !charging;
    lb7 = gGauge.Flags & 0x0080;
    
    /* We already know High Bit 2 is FC, the matching one in current code is bq274/bq275 */
    if ((hb5 ^ hb4)) {
        return [self setBitSetByModel:@"bq275"];
    }
    /* To check if hb3 is CHG_INH, we have to check battery temperature */
    
    /* Fallback to bq274 */
    return [self setBitSetByModel:@"bq274"];
}

/* This should not be used when DeviceName exists */
- (void)setBitSetByTargetName {
    const board_info_t *board = get_board_info();
    NSString *rplt_nsstr = [NSString stringWithUTF8String:board->TargetName];

    // Not really bq27545, but most similar
    /* As I observed on iPhone 12, most possibly bq275 which has BATHI/BATLOW
     * Confirmed Shape:
     * ________ __ BATHI BATLOW ???? ???? FC   CHG
     * OCVTAKEN __ _____ ______ ____ SOC1 SOCF DSG */
    NSArray *bq27545 = @[@"d52", @"d53", @"d54"]; // iPhone 12 Series
    for (int i = 0; i < bq27545.count; i++) {
        if ([rplt_nsstr rangeOfString:bq27545[i] options:NSCaseInsensitiveSearch].location != NSNotFound)
            return [self setBitSetByModel:@"bq27545"];
    }

    /* iPhone X series without iPhone X */
    NSArray *bq27546 = @[@"n84", @"star", @"lisbon", @"hangzhou", @"d32", @"d33"];
    for (int i = 0; i < bq27546.count; i++) {
        if ([rplt_nsstr rangeOfString:bq27546[i] options:NSCaseInsensitiveSearch].location != NSNotFound)
            return [self setBitSetByModel:@"bq27546"];
    }

    return [self setBitSetByGuess];
}

- (void)setBitSetByModel:(NSString * _Nonnull)name {
    /* What we knows now:
     - Newer Embedded devices does not give IC name
     - bq27546 is used on iPhone XR/XS/XS Max (But I can't test)
     - High Bytes Bit2=FC, at least iPhone 12 is
     - Low Bytes Bit7=OCVTAKEN Bit0=DSG, at least iPhone 12 is
     - So we assue Embedded is using bq274 series for now
     */
    if ([name containsString:@"bq274"]) {
        NSString *hb5, *hb4, *hb3, *hb2;
        NSString *lb6;
        /* RSVD = Reserved. */
        hb5 = hb4 = hb3 = hb2 = lb6 = RSVD;

        /*
         OT   = Over-Temperature condition is detected. [OT] is set when Temperature() ≥ Over Temp (default = 55°C). [OT] is cleared when Temperature() < Over Temp – Temp Hys.
         UT   = Under-Temperature condition is detected. [UT] is set when Temperature() ≤ Under Temp (default = 0°C). [UT] is cleared when Temperature() > Under Temp + Temp Hys.
         FC   = Full charge is detected. If the FC Set% is a positive threshold, [FC] is set when SOC ≥ FC Set % and is cleared when SOC ≤ FC Clear % (default = 98%). By default, FC Set% = –1, therefore [FC] is set when the fuel gauge has detected charge termination.
         CHG  = Fast charging allowed. If SOC changes from 98% to 99% during charging, the [CHG] bit is cleared. The [CHG] bit will become set again when SOC ≤ 95%.
         */
        if ([name containsString:@"bq27425"]) {
            /* EEFAIL = EEPROM Write Fail. True when set. This bit is set after a single EEPROM write failure. All subsequent EEPROM writes are disabled. A power-on reset or RESET subcommand is required to clear the bit to re-enable EEPROM writes. */
            hb2 = EEFAIL;
        }
        [self setHighBitSet:@[OT, UT, hb5, hb4, hb3, hb2, FC, CHG]];
        /*
         OCVTAKEN  = Cleared on entry to relax mode and set to 1 when OCV measurement is performed in relax mode.
         ITPOR     = Indicates a POR or RESET subcommand has occurred. If set, this bit generally indicates that the RAM configuration registers have been reset to default values and the host should reload the configuration parameters using the CONFIG UPDATE mode. This bit is cleared after the SOFT_RESET subcommand is received.
         CFGUPMODE = Fuel gauge is in CONFIG UPDATE mode. True when set. Default is 0.
         BAT_DET   = Battery insertion detected. True when set. When OpConfig [BIE] is set, [BAT_DET] is set by detecting a logic high-to-low transition at the BIN pin. When OpConfig [BIE] is low, [BAT_DET] is set when host issues the BAT_INSERT subcommand and is cleared when host issues the BAT_REMOVE subcommand. Gauge predictions are not valid unless [BAT_DET] is set.
         SOC1      = If set, StateOfCharge() ≤ SOC1 Set Threshold. The [SOC1] bit will remain set until StateOfCharge() ≥ SOC1 Clear Threshold.
         SOCF      = If set, StateOfCharge() ≤ SOCF Set Threshold. The [SOCF] bit will remain set until StateOfCharge() ≥ SOCF Clear Threshold.
         DSG       = Discharging detected. True when set.
         */
        if ([name containsString:@"bq27421"] || [name containsString:@"bq27426"] || [name containsString:@"bq27427"]) {
            /* DOD Correct = This indicates that DOD correction is being applied. */
            lb6 = DODCorrect;
        }
        [self setLowBitSet:@[OCVTAKEN, lb6, ITPOR, CFGUPMODE, BAT_DET, SOC1, SOCF, DSG]];
    }
    /* I am sure this is not used on iPhone 12 series
     * Low Bit 7 is OCVTAKEN on iPhone 12, but bq27546 Low Bit 7 is CHG_SUS */
    else if ([name containsString:@"bq275"]) {
        NSString *hb7, *hb6, *hb2, *hb0;
        NSString *lb7, *lb6, *lb5, *lb4, *lb3;
        hb7 = hb6 = hb2 = hb0 = lb6 = lb5 = RSVD;
        lb7 = CHG_SUS;
        lb4 = IMAX;
        lb3 = CHG;
        /*
         BATHI   = Battery High bit indicating a high battery voltage condition.
         BATLOW  = Battery Low bit indicating a low battery voltage condition.
         CHG_INH = Charge Inhibit indicates that temperature is < T1 Temp or > T4 Temp while charging is not active. True when set.
         FC      = Full-charged state is detected. FC is set when charge termination is reached and FC Set % = –1 or State of Charge is larger than FC Set % and FC Set % is not –1. True when set.
         */
        if ([name containsString:@"bq27545"]) {
            /*
             OTC = Over-Temperature in Charge condition is detected.
             OTD = Over-Temperature in Discharge condition is detected.
             CHG = (Fast) charging allowed.
             */
            hb7 = OTC;
            hb6 = OTD;
            hb0 = CHG;
            /*
             OCVTAKEN = Cleared on entry to RELAX mode and set to 1 when OCV measurement is performed in RELAX.
             ISD      = Internal Short is detected.
             TDD      = Tab Disconnect is detected.
             HW[1:0]  = Device Identification. Default is 1/0
             */
            lb7 = OCVTAKEN;
            lb6 = ISD;
            lb5 = TDD;
            lb4 = HW1;
            lb3 = HW0;
        }
        [self setHighBitSet:@[hb7, hb6, BATHI, BATLOW, CHG_INH, hb2, FC, hb0]];
        /*
         CHG_SUS = Charge Suspend indicates that temperature is < T1 Temp or > T5 Temp while charging is active. True when set.
         IMAX    = Indicates that the computed Imax() value has changed enough to signal an interrupt. True when set.
         CHG     = (Fast) charging allowed. True when set.
         SOC1    = State-of-Charge Threshold 1 (SOC1 Set) reached. True when set.
         SOCF    = State-of-Charge Threshold Final (SOCF Set %) reached. True when set.
         DSG     = Discharging detected. True when set.
         */
        [self setLowBitSet:@[lb7, lb6, lb5, lb4, lb3, SOC1, SOCF, DSG]];
    }
    /* Typically MacBook Pro 2020 M1 */
    /* For bq20z series, the Flags is BatteryStatus (0x16) */
    else if ([name containsString:@"bq20z"]) {
        NSString *hb5, *hb2;
        hb5 = hb2 = RSVD;
        /*
         OCA = Over Charged Alarm
         TCA = Terminate Charge Alarm
         OTA = Over Temperature Alarm
         TDA = Terminate Discharge Alarm
         RCA = Remaining Capacity Alarm
         RTA = Remaining Time Alarm
         */
        [self setHighBitSet:@[OCA, TCA, hb5, OTA, TDA, hb2, RCA, RTA]];
        /*
         INIT = Initialization. The INIT flag is always set in normal operation.
         DSG  = Discharging
                0 = charging mode
                1 = discharging mode or relaxation mode, or valid charge termination has occurred.
         FC   = Fully Charged
         FD   = Fully Disharged
         EC3, EC2, EC1, EC0— Error Code, returns status of processed SBS function
                0,0,0,0 =             OK - processed the function code with no errors detected.
                0,0,0,1 =           BUSY - unable to process the function code at this time.
                0,0,1,0 =       Reserved - detected an attempt to read or write to a function code reserved by this version of the specification, or detected an attempt to access an unsupported optional manufacturer function code.
                0,0,1,1 =    Unsupported - does not support this function code as defined in this version of the specification.
                0,1,0,0 =   AccessDenied - detected an attempt to write to a read-only function code.
                0,1,0,1 = Over/Underflow - detected a data overflow or underflow.
                0,1,1,0 =        BadSize - detected an attempt to write to a function code with an incorrect data block.
                0,1,1,1 =   UnknownError - detected an unidentifiable error.
         */
        [self setLowBitSet:@[INIT, DSG, FC, FD, EC3, EC2, EC1, EC0]];
    }
    else {
        /* Stub */
        [self setHighBitSet:@[@"H7", @"H6", @"H5", @"H4", @"H3", @"H2", @"H1", @"H0"]];
        [self setLowBitSet:@[@"L7", @"L6", @"L5", @"L4", @"L3", @"L2", @"L1", @"L0"]];
    }

    for (int i = 0; i < [self highBitSet].count; i++) {
        [self.highByte setTitle:(NSString *)[self highBitSet][i] forSegmentAtIndex:i];
    }
    for (int i = 0; i < [self lowBitSet].count; i++) {
        [self.lowByte setTitle:(NSString *)[self lowBitSet][i] forSegmentAtIndex:i];
    }
}

- (void)selectByFlags:(UInt32)flags {
    if (!_highByte) {
        NSLog(@"selectByFlags: too early to call!");
        return;
    }
    [self setNeedsDisplay];
    [self layoutIfNeeded];

    NSMutableIndexSet *high_set = [[NSMutableIndexSet alloc] init];
    uint16_t high_bits = (flags & (0xFF << 8)) >> 8;
    for (int i = 0; i < 8; i++) {
        if (high_bits & (1 << i)) {
            [high_set addIndex:7 - i];
        }
    }
    [self.highByte setSelectedSegmentIndexes:high_set];

    NSMutableIndexSet *low_set = [[NSMutableIndexSet alloc] init];
    uint16_t low_bits = flags & 0xFF;
    for (int i = 0; i < 8; i++) {
        if (low_bits & (1 << i)) {
            [low_set addIndex:7 - i];
        }
    }
    [self.lowByte setSelectedSegmentIndexes:low_set];
    return;
}

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        // This must exist for our custom labels to align
        self.textLabel.text = @"TITLE";
        // Hide
        self.textLabel.hidden = YES;
        self.accessoryView.hidden = YES;

        // Alternative title
        self.titleLabel = [[UILabel alloc] init];
        self.titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
        self.titleLabel.font = self.textLabel.font;
        [self.contentView addSubview:self.titleLabel];

        // Alternative detail
        _detailTextLabel = [[UILabel alloc] init];
        self.detailTextLabel.translatesAutoresizingMaskIntoConstraints = NO;
        // For use as our custom labels' template
        UITableViewCell *cell;
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:nil];
        cell.hidden = YES;
        [self.contentView addSubview:cell];
        cell.detailTextLabel.text = @"DETAIL";
        self.detailTextLabel.font = cell.detailTextLabel.font;
        self.detailTextLabel.textColor = cell.detailTextLabel.textColor;
        self.detailTextLabel.textAlignment = cell.detailTextLabel.textAlignment;
        [self.contentView addSubview:self.detailTextLabel];

        // Initialize segmented control with sample segment
        UIFont *font;
#ifdef USE_TEXT_FLAGS
        font = [UIFont systemFontOfSize:(UIFont.smallSystemFontSize + 1) * 0.7 weight:UIFontWeightRegular];
#else
        font = [UIFont fontWithName:@SFPRO size:(UIFont.smallSystemFontSize + 1) * 0.7];
#endif
        UberSegmentedControlConfig *conf = [[UberSegmentedControlConfig alloc] initWithFont:font tintColor:nil allowsMultipleSelection:YES];

        [self setBitSetByModel:@"STUB"]; // Stub first, adjust in ref
        self.highByte = [[UberSegmentedControl alloc] initWithItems:[self highBitSet] config:conf];
        self.highByte.translatesAutoresizingMaskIntoConstraints = NO;
        self.highByte.userInteractionEnabled = NO; // Only use to diplay
        [self.contentView addSubview:self.highByte];
        self.lowByte = [[UberSegmentedControl alloc] initWithItems:[self lowBitSet] config:conf];
        self.lowByte.translatesAutoresizingMaskIntoConstraints = NO;
        self.lowByte.userInteractionEnabled = NO; // Only use to diplay
        [self.contentView addSubview:self.lowByte];
        
        // Setup Auto Layout constraints
        [NSLayoutConstraint activateConstraints:@[
            // Title label constraints
            [self.titleLabel.topAnchor constraintEqualToAnchor:self.textLabel.topAnchor constant:(self.frame.size.height - self.textLabel.font.pointSize) / 2],
            [self.titleLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
            [self.titleLabel.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
            // Detail label
            [self.detailTextLabel.topAnchor constraintEqualToAnchor:self.textLabel.topAnchor constant:(self.frame.size.height - self.textLabel.font.pointSize) / 2],
            [self.detailTextLabel.leadingAnchor constraintEqualToAnchor:self.textLabel.leadingAnchor],
            [self.detailTextLabel.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],
            
            // Segmented control constraints
            [self.highByte.topAnchor constraintEqualToAnchor:self.titleLabel.bottomAnchor constant:8],
            [self.highByte.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],
            [self.highByte.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],

            [self.lowByte.topAnchor constraintEqualToAnchor:self.highByte.bottomAnchor constant:4],
            [self.lowByte.leadingAnchor constraintEqualToAnchor:self.contentView.leadingAnchor constant:16],
            [self.lowByte.trailingAnchor constraintEqualToAnchor:self.contentView.trailingAnchor constant:-16],

            //[self.segmentedControl.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-8],

            // Bottom
            [self.lowByte.bottomAnchor constraintEqualToAnchor:self.contentView.bottomAnchor constant:-12],
        ]];
    }
    return self;
}

- (id)detailTextLabel {return _detailTextLabel;}

@end
