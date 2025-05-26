#include "battery_info.h"
#include "../common.h"
#include "libsmc.h"
#include "accessory.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFString.h>

#ifdef _C
#undef _C
#endif
// Stub _C definition for gettext, locales are caller processed
#define _C(x) x
const char *bin_unit_strings[] = {
    _C("℃"), _C("%"), _C("mA"), _C("mAh"), _C("mV"), _C("mW"), _C("min"),
    _C("Hr") // Do not modify, thats how Texas Instruments documented
};

struct battery_info_node main_battery_template[] = {
    { _C("Gas Gauge (Basic)"), _C("All Gas Gauge metrics are dynamically retrieved from the onboard sensor array in real time. Should anomalies be detected in specific readings, this may indicate the presence of unauthorized components or require diagnostics through Apple Authorised Service Provider."), DEFINE_SECTION(10000) },
    {
     _C("Device Name"),
     _C("This indicates the name of the current Gas Gauge IC used by the installed battery."),
     },
    { _C("Health"), NULL, BIN_IS_BACKGROUND | BIN_UNIT_PERCENT },
    { _C("SoC"), NULL, BIN_IS_FLOAT | BIN_UNIT_PERCENT },
    { _C("Avg. Temperature"), NULL, BIN_IS_FLOAT | BIN_UNIT_DEGREE_C | BIN_DETAILS_SHARED },
    { _C("Charging"), NULL, BIN_IS_BOOLEAN },
    { "ASoC(Hidden)", NULL, BIN_IS_FOREGROUND | BIN_IS_HIDDEN },
    { _C("Full Charge Capacity"), NULL, BIN_UNIT_MAH | BIN_IN_DETAILS },
    { _C("Designed Capacity"), NULL, BIN_UNIT_MAH | BIN_IN_DETAILS },
    { _C("Remaining Capacity"), NULL, BIN_UNIT_MAH | BIN_IN_DETAILS },
    { _C("Battery Uptime"), _C("The length of time the Battery Management System (BMS) has been up."), BIN_UNIT_MIN | BIN_IN_DETAILS },
    { _C("Qmax"), NULL, BIN_UNIT_MAH | BIN_IN_DETAILS },
    { _C("Depth of Discharge"), _C("Current chemical depth of discharge (DOD₀). The gas gauge updates information on the DOD₀ based on open-circuit voltage (OCV) readings when in a relaxed state."), BIN_UNIT_MAH | BIN_IN_DETAILS },
    { _C("Passed Charge"), _C("The cumulative capacity of the current charging or discharging cycle. It is reset to zero with each DOD₀ update."), BIN_UNIT_MAH | BIN_IN_DETAILS },
    { _C("Voltage"), NULL, BIN_UNIT_MVOLT | BIN_IN_DETAILS },
    { _C("Avg. Current"), NULL, BIN_UNIT_MAMP | BIN_IN_DETAILS },
    { _C("Avg. Power"), NULL, BIN_UNIT_MWATT | BIN_IN_DETAILS },
    { _C("Cell Count"), NULL, BIN_IN_DETAILS },
    /* TODO: TimeToFull */
    { _C("Time To Empty"), NULL, BIN_UNIT_MIN | BIN_IN_DETAILS },
    { _C("Cycle Count"), NULL, BIN_IN_DETAILS },
    { _C("Designed Cycle Count"), NULL, BIN_IN_DETAILS },
    { _C("State Of Charge"), NULL, BIN_UNIT_PERCENT | BIN_IN_DETAILS },
    { _C("State Of Charge (UI)"), _C("The \"Battery Percentage\" displayed exactly on your status bar. This is the SoC that Apple wants to tell you."), BIN_UNIT_PERCENT | BIN_IN_DETAILS },
    { _C("Resistance Scale"), NULL, BIN_IN_DETAILS },
    { _C("Battery Serial No."), NULL, 0 },
    { _C("Chemistry ID"), _C("Chemistry unique identifier (ChemID) assigned to each battery in Texas Instruments' database. It ensures accurate calculations and predictions."), 0 },
    { _C("Flags"), NULL, 0 },
    { _C("True Remaining Capacity"), NULL, BIN_UNIT_MAH | BIN_IN_DETAILS },
    { _C("OCV Current"), NULL, BIN_UNIT_MAMP | BIN_IN_DETAILS },
    { _C("OCV Voltage"), NULL, BIN_UNIT_MVOLT | BIN_IN_DETAILS },
    { _C("Max Load Current"), NULL, BIN_UNIT_MAMP | BIN_IN_DETAILS },
    { _C("Max Load Current 2"), NULL, BIN_UNIT_MAMP | BIN_IN_DETAILS },
	/* TODO: Parse ITMiscStatus
	 * Known components: DOD0Update, FastQmaxUpdate, ITSimulation, QmaxAtEOC, QmaxDisqualified, QmaxDOD0, QmaxUpdate, RaUpdate */
	{ _C("IT Misc Status"), _C("This field refers to the miscellaneous data returned by battery Impedance Track™ Gas Gauge IC."), 0 },
    { _C("Simulation Rate"), _C("This field refers to the rate of Gas Gauge performing Impedance Track™ simulations."), BIN_UNIT_HOUR | BIN_IN_DETAILS },
    { _C("Daily Max SoC"), NULL, BIN_UNIT_PERCENT | BIN_IN_DETAILS },
    { _C("Daily Min SoC"), NULL, BIN_UNIT_PERCENT | BIN_IN_DETAILS },
    { _C("Adapter Details"), _C("All adapter information is dynamically retrieved from the hardware of the currently connected adapter (or cable if you are using Lightning ports). If any of the data is missing, it may indicate that the power source is not providing the relevant information, or there may be a hardware issue with the power source."), DEFINE_SECTION(9000) },
    { _C("Port"), _C("Port of currently connected adapter. On macOS, this is the USB port that the adapter currently attached."), BIN_IN_DETAILS },
    { _C("Port Type"), NULL, 0 },
    { _C("Compatibility"), NULL, 0 },
    { _C("Type"), _C("This field refers to the Family Code (kIOPSPowerAdapterFamilyKey) of currently connected power adapter."), 0 },
    { _C("Status"), NULL, 0 },
    { _C("Reason"), _C("If this field appears in the list, it indicates that an issue has occurred or that a condition was met, causing charging to stop."), 0 },
    { _C("Current Rating"), _C("Current rating of connected power source, this does not indicates the real-time passing current."), BIN_IN_DETAILS | BIN_UNIT_MAMP },
    { _C("Voltage Rating"), _C("Voltage rating of connected power source, this does not indicates the real-time passing voltage."), BIN_IN_DETAILS | BIN_UNIT_MVOLT },
    { _C("Charging Current"), NULL, BIN_IN_DETAILS | BIN_UNIT_MAMP },
    { _C("Charging Voltage"), NULL, BIN_IN_DETAILS | BIN_UNIT_MVOLT },
    { _C("Charger ID"), NULL, 0 },
    { _C("Model Name"), NULL, 0 },
    { _C("Manufacturer"), NULL, 0 },
    { _C("Model"), NULL, 0 },
    { _C("Firmware Version"), NULL, 0 },
    { _C("Hardware Version"), NULL, 0 },
    { _C("Description"), _C("Short description provided by Apple PMU on current power adapter Family Code. Sometimes may not set."), 0 },
    { _C("Serial No."), NULL, 0 },
    { _C("PMU Configuration"), _C("The Configuration values is the max allowed Charging Current configurations."), BIN_IN_DETAILS | BIN_UNIT_MAMP },
    { _C("Charger Configuration"), NULL, BIN_IN_DETAILS | BIN_UNIT_MAMP },
    { _C("HVC Mode"), _C("High Voltage Charging (HVC) Mode may accquired by your power adapter or system, all supported modes will be listed below."), BIN_IN_DETAILS },
    { _C("Inductive Port"), NULL, DEFINE_SECTION(8500) },
    /* FIXME: We are meeting situations that needing rows with same names but not same sections, current data structure cannot let us do this. */
    { _C("Acc. ID"), NULL, 0 },
    { _C("Allowed Features"), _C("Accessory Feature Flags, I don't know how to parse it yet."), 0 },
    { _C("Acc. Serial No."), NULL, 0 },
    { _C("Acc. Manufacturer"), NULL, 0 },
    { _C("Acc. Product ID"), NULL, 0},
    { _C("Acc. Model"), NULL, 0 },
    { _C("Acc. Name"), NULL, 0 },
    { _C("Acc. PPID"), NULL, 0 },
    { _C("Acc. Firmware Version"), NULL, 0 },
    { _C("Acc. Hardware Version"), NULL, 0 },
    { _C("Battery Pack"), _C("This indicates if an accessory is now working as a Battery Pack."), 0 },
    { _C("Power Mode"), NULL, 0 },
    { _C("Sleep Power"), NULL, 0 },
    { _C("Supervised Acc. Attached"), NULL, 0 },
    { _C("Supervised Transports Restricted"), NULL, 0 },
    { NULL }  // DO NOT DELETE
};

struct iopm_property {
    const char *name;
    CFStringRef **candidates;
    int property_type; // 0 = don't care : default s_int32
    int in_detail;
    double multiplier; // when multiplier==0, no multiplier is applied
                       // which means, multiplier 0 is equivalent to 1
};

// Special types:
// 0 - signed int32 - equivalent to kCFNumberSInt32Type
// 500 - bool
// 501 - CFStringRef
// 502 - bool (set hidden)
// 502 - bool (set hidden - inverted)
// (CFStringRef) 1 - first item in array (fail if not an array, will not crash)

#define IPCandidateGroup(...) \
    (CFStringRef *[]) { __VA_ARGS__, NULL }
#define IPCandidate(...) \
    (CFStringRef[]) { __VA_ARGS__, NULL }
#define IPSingleCandidate(...) \
    (CFStringRef *[]) { (CFStringRef[]) { __VA_ARGS__, NULL }, NULL }

struct iopm_property iopm_items[]={
	{_C("Avg. Temperature"), IPSingleCandidate(CFSTR("Temperature")),kCFNumberSInt16Type,0,1.0/100.0},
	{_C("Charging"),IPSingleCandidate(CFSTR("AppleRawExternalConnected")),500,0,0},
	{_C("Full Charge Capacity"),IPSingleCandidate(CFSTR("AppleRawMaxCapacity")),kCFNumberSInt16Type,1,0},
	{_C("Designed Capacity"),IPSingleCandidate(CFSTR("DesignCapacity")),kCFNumberSInt16Type,1,0},
	{_C("Remaining Capacity"),IPSingleCandidate(CFSTR("AppleRawCurrentCapacity")),kCFNumberSInt16Type,1,0},
	{_C("Battery Uptime"),IPSingleCandidate(CFSTR("BatteryData"),CFSTR("LifetimeData"),CFSTR("TotalOperatingTime")),0,1,1.0/60.0},
	{_C("Qmax"),IPCandidateGroup(IPCandidate(CFSTR("BatteryData"),CFSTR("Qmax"),(CFStringRef)1),IPCandidate(CFSTR("BatteryData"),CFSTR("QmaxCell0"))),0,1,0},
	{_C("Depth of Discharge"),IPCandidateGroup(IPCandidate(CFSTR("BatteryData"),CFSTR("DOD0"),(CFStringRef)1),IPCandidate(CFSTR("BatteryFCCData"),CFSTR("DOD0"))),0,1,0},
	{_C("Passed Charge"),IPCandidateGroup(IPCandidate(CFSTR("BatteryData"),CFSTR("PassedCharge")),IPCandidate(CFSTR("BatteryFCCData"),CFSTR("PassedCharge"))),0,1,0},
	{_C("Voltage"),IPSingleCandidate(CFSTR("BatteryData"),CFSTR("Voltage")),0,1,0},
	{_C("Avg. Current"),IPSingleCandidate(CFSTR("InstantAmperage")),0,1,0},
	{_C("Cycle Count"),IPSingleCandidate(CFSTR("BatteryData"),CFSTR("CycleCount")),0,1,0},
	{_C("State Of Charge"),IPSingleCandidate(CFSTR("BatteryData"),CFSTR("StateOfCharge")),0,1,0},
	{_C("State Of Charge (UI)"),IPSingleCandidate(CFSTR("CurrentCapacity")),0,1,0},
	{_C("Resistance Scale"),IPSingleCandidate(CFSTR("BatteryFCCData"),CFSTR("ResScale")),0,1,0},
	{_C("Battery Serial No."),IPSingleCandidate(CFSTR("Serial")),501,1,0},
	// Chemistry ID: To be done programmatically
	// Flags: TBD Prg/ly
	{_C("True Remaining Capacity"),IPSingleCandidate(CFSTR("AbsoluteCapacity")),0,1,0},
	//{_C("IT Misc Status"),IPSingleCandidate(CFSTR("BatteryData"),CFSTR("ITMiscStatus")),0,1,0},
	{_C("Simulation Rate"),IPSingleCandidate(CFSTR("BatteryData"),CFSTR("SimRate")),0,1,0},
	{_C("Daily Max SoC"),IPSingleCandidate(CFSTR("BatteryData"),CFSTR("DailyMaxSoc")),0,1,0},
	{_C("Daily Min SoC"),IPSingleCandidate(CFSTR("BatteryData"),CFSTR("DailyMinSoc")),0,1,0},
	{_C("Adapter Details"),IPSingleCandidate(CFSTR("AppleRawExternalConnected")),503,1,0},
	// Port???
	// Port Type???
	// Type TBD programmatically
	// Status???
	{_C("Current Rating"),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("Current")),0,1,0},
	{_C("Voltage Rating"),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("Voltage")),0,1,0},
	// Charger ID prog
	{_C("Description"),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("Description")),501,1,0},
	{_C("PMU Configuration"),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("PMUConfiguration")),0,1,0},
	{_C("Model Name"),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("Name")),501,1,0},
	// Model p
	{_C("Manufacturer"),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("Manufacturer")),501,1,0},
	{_C("Firmware Version"),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("FwVersion")),501,1,0},
	{_C("Hardware Version"),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("HwVersion")),501,1,0},
	{_C("Serial No."),IPSingleCandidate(CFSTR("AdapterDetails"),CFSTR("SerialString")),501,1,0},
	{NULL}
};

struct battery_info_section *bi_make_section(const char *name, uint64_t context_size) {
	size_t size=0;
	struct battery_info_node *section_begin=NULL;
	for(struct battery_info_node *i=main_battery_template;i->name;i++) {
		if((i->content&BIN_SECTION)==BIN_SECTION) {
			if(section_begin)
				break;
			if(i->name==name)
				section_begin=i;
		}
		if(section_begin)
			size+=sizeof(struct battery_info_node);
	}
	if(!section_begin)
		return NULL;
	struct battery_info_section *section=malloc(size+32+context_size);
	section->next=NULL;
	section->self_ref=NULL;
	memcpy(&section->data,section_begin,size);
	*(char**)((uint64_t)section+size+24)=NULL;
	section->context=(struct battery_info_section_context*)((uint64_t)section+size+32);
	return section;
}

void bi_destroy_section(struct battery_info_section *sect) {
	if(sect->self_ref)
		battery_info_remove_section(sect);
	sect->data[0].name=NULL; // This is to tell update() that the section is being destroyed
	sect->context->update(sect); // update() will have to release resources
	free(sect);
}

void bi_node_change_content_value(struct battery_info_node *node,
    int identifier, unsigned short value)
{
    node += identifier;
    uint16_t *sects = (uint16_t *)&node->content;
    sects[1]        = value;
}

void bi_node_change_content_value_float(struct battery_info_node *node,
    int identifier, float value)
{
    node += identifier;
    assert((node->content & BIN_IS_FLOAT) == BIN_IS_FLOAT);
    uint32_t *vptr = (uint32_t *)&value;
    uint32_t vr    = *vptr;
    // TODO: No magic numbers!
    node->content =
        ((vr & ((uint64_t)0b11 << 30)) | (vr & (((1 << 4) - 1) << 23)) << 3 |
            (vr & (((1 << 10) - 1) << 13)) << 3) |
        (node->content & ((1 << 16) - 1));
    // overwrite higher bits;
}

float bi_node_load_float(struct battery_info_node *node)
{
    float ret;
    uint32_t *vptr = (uint32_t *)&ret;
    uint32_t vr    = node->content;
    *vptr =
        ((vr & ((uint64_t)0b11 << 30)) | (vr & (((1 << 4) - 1) << 26)) >> 3 |
            (vr & (((1 << 10) - 1) << 16)) >> 3);
    return ret;
}

void bi_node_set_hidden(struct battery_info_node *node, int identifier,
    bool hidden)
{
    node += identifier;
    // assert((node->content & BIN_IN_DETAILS) == BIN_IN_DETAILS);
    if(!node->content)
	    return;
    if (hidden) {
        node->content |= (1 << 5);
    } else {
        node->content &= ~(1L << 5);
    }
}

#include <mach/mach.h>

char *bi_node_ensure_string(struct battery_info_node *node, int identifier,
    uint64_t length)
{
    node += identifier;
    assert(!(node->content & BIN_IS_SPECIAL));

    if (!node->content) {
        void *allocen = (void *)0x10000000;
        // ^ Preferred addr
        // Use vm_allocate to prevent possible unexpected heap allocation (it
        // crashes in current data structure)
        // TODO: get rid of hardcoded length
        int result = vm_allocate(mach_task_self(), (vm_address_t *)&allocen, 256, VM_FLAGS_ANYWHERE);
        if (result != KERN_SUCCESS) {
            // Fallback to malloc
            // allocen = malloc(length);
            allocen = nil;
        }
        node->content = (uint32_t)(((uint64_t)allocen) >> 3);
    }
    return bi_node_get_string(node);
}

char *bi_node_get_string(struct battery_info_node *node)
{
    return (char *)(((uint64_t)node->content) << 3);
}

void bi_node_free_string(struct battery_info_node *node)
{
    if (!node->content)
        return;
    vm_deallocate(mach_task_self(), (vm_address_t)bi_node_get_string(node), 256);
    node->content = 0;
}

static int _impl_set_item_find_item(struct battery_info_node **head, const char *desc)
{
    if (!desc)
        return 0;
    for (struct battery_info_node *i = *head; i->name; i++) {
        if (i->name == desc) {
            *head = i;
            return 1;
        }
    }
    for (struct battery_info_node *i = (*head) - 1; i != head[1] - 1; i--) {
        if (i->name == desc) {
            *head = i;
            return 1;
        }
    }
    return 0;
}

static char *_impl_set_item(struct battery_info_node **head, const char *desc,
    uint64_t value, float valueAsFloat, int options)
{
    if (!_impl_set_item_find_item(head, desc))
        return NULL;
    struct battery_info_node *i = *head;
    if (options == 2 || (i->content & BIN_IS_SPECIAL) == 0) {
        if (options == 1) {
            if (value && i->content) {
                bi_node_free_string(i);
            }
            return NULL;
        }
        return bi_node_ensure_string(i, 0, 256);
    } else if (options == 1) {
        bi_node_set_hidden(i, 0, (bool)value);
    } else if ((i->content & BIN_IS_FLOAT) == BIN_IS_FLOAT) {
        bi_node_change_content_value_float(i, 0, valueAsFloat);
        return NULL;
    } else {
        bi_node_change_content_value(i, 0, (uint16_t)value);
    }
    return NULL;
}

#define BI_SET_ITEM(name, value) \
    _impl_set_item(head_arr, name, (uint64_t)(value), (float)(value), 0)
#define BI_ENSURE_STR(name) _impl_set_item(head_arr, name, 0, 0, 2)
#define BI_FORMAT_ITEM(name, ...) \
    sprintf(_impl_set_item(head_arr, name, 0, 0, 2), __VA_ARGS__)
#define BI_SET_ITEM_IF(cond, name, value)        \
    if (cond) {                                  \
        BI_SET_ITEM(name, value);                \
        _impl_set_item(head_arr, name, 0, 0, 1); \
    } else {                                     \
        _impl_set_item(head_arr, name, 1, 0, 1); \
    }
#define BI_FORMAT_ITEM_IF(cond, name, ...)       \
    if (cond) {                                  \
        BI_FORMAT_ITEM(name, __VA_ARGS__);       \
    } else {                                     \
        _impl_set_item(head_arr, name, 1, 0, 1); \
    }
#define BI_SET_HIDDEN(name, value) _impl_set_item(head_arr, name, value, 0, 1)

#include "../iokitextern.h"

void battery_info_init(struct battery_info_section **ptr) {
	*ptr=NULL;
	battery_info_update(ptr);
}

// ptr=&head;
void battery_info_insert_section(struct battery_info_section *sect, struct battery_info_section **ptr) {
	if(!ptr)
		return;
	if(!*ptr) {
		*ptr=sect;
		sect->next=NULL;
		sect->self_ref=ptr;
		return;
	}
	if(SECTION_PRIORITY(sect)<=SECTION_PRIORITY(*ptr)) {
		for(;;ptr=&(*ptr)->next) {
			if(!*ptr||SECTION_PRIORITY(sect)>SECTION_PRIORITY(*ptr)) {
				sect->next=*ptr;
				*ptr=sect;
				sect->self_ref=ptr;
				return;
			}
		}
	}else{
		sect->next=*ptr;
		*ptr=sect;
		sect->self_ref=ptr;
	}
}

void battery_info_remove_section(struct battery_info_section *sect) {
	*(sect->self_ref)=sect->next;
	sect->self_ref=NULL;
	sect->next=NULL;
}

static int battery_info_has(struct battery_info_section *head, uint64_t identifier) {
	for(struct battery_info_section *i=head;i;i=i->next) {
		if(i->context->custom_identifier==identifier)
			return 1;
	}
	return 0;
}

void adapter_info_update_smc(struct battery_info_section *section);
void battery_info_update_smc(struct battery_info_section *section);

void battery_info_poll(struct battery_info_section **head) {
	if(hasSMC) {
		if(!battery_info_has(*head,BI_GAS_GAUGE_SECTION_ID)) {
			struct battery_info_section *smcSect=bi_make_section(_C("Gas Gauge (Basic)"),sizeof(struct battery_info_section_context));
			smcSect->context->custom_identifier=BI_GAS_GAUGE_SECTION_ID;
			smcSect->context->update=battery_info_update_smc;
			battery_info_insert_section(smcSect,head);
		}
		charging_state_t charging_stat = is_charging(NULL, NULL);
		if(charging_stat>0&&!battery_info_has(*head,BI_ADAPTER_SECTION_ID)) {
			struct battery_info_section *smcAdpSect=bi_make_section(_C("Adapter Details"),sizeof(struct battery_info_section_context));
			smcAdpSect->context->custom_identifier=BI_ADAPTER_SECTION_ID;
			smcAdpSect->context->update=adapter_info_update_smc;
			battery_info_insert_section(smcAdpSect,head);
		}
	}
}

// Recursive call is used to support section removal while updating
static void _battery_info_update(struct battery_info_section *node) {
	if(node->next)
		_battery_info_update(node->next);
	node->context->update(node);
}

void battery_info_update(struct battery_info_section **head) {
	battery_info_poll(head);
	if(*head)
		_battery_info_update(*head);
}

struct battery_info_section *battery_info_get_section(struct battery_info_section *head,int n) {
	for(int i=0;i<n;i++)
		head=head->next;
	return head;
}

int battery_info_get_section_count(struct battery_info_section *head) {
	int ret=0;
	for(struct battery_info_section *i=head;i;i=i->next)
		ret++;
	return ret;
}

// TODO: Re-implement
#if 0
void battery_info_update_iokit_with_data(struct battery_info_node *head, const void *info, bool inDetail) {
	struct battery_info_node *head_arr[2] = {head, head};
	uint16_t remain_cap,full_cap,design_cap;
	uint16_t temperature;
	CFNumberRef designCapacityNum=CFDictionaryGetValue(info, CFSTR("DesignCapacity"));
	CFNumberRef fullCapacityNum=CFDictionaryGetValue(info, CFSTR("AppleRawMaxCapacity"));
	CFNumberRef remainingCapacityNum=CFDictionaryGetValue(info, CFSTR("AppleRawCurrentCapacity"));
	if(!designCapacityNum||!fullCapacityNum||!remainingCapacityNum) {
		// Basic info required
		fprintf(stderr, "battery_info_update_iokit: Basic info required not present\n");
		CFRelease(info);
		return;
	}
	CFNumberGetValue(designCapacityNum, kCFNumberSInt16Type, (void *)&design_cap);
	CFNumberGetValue(fullCapacityNum, kCFNumberSInt16Type, (void *)&full_cap);
	CFNumberGetValue(remainingCapacityNum, kCFNumberSInt16Type, (void *)&remain_cap);
	BI_SET_ITEM_IF(1,_C("Health"), 100.0f * (float)full_cap / (float)design_cap);
	BI_SET_ITEM_IF(1,_C("SoC"), 100.0f * (float)remain_cap / (float)full_cap);
	BI_SET_ITEM("ASoC(Hidden)", 100.0f * (float)remain_cap / (float)design_cap);
	if (inDetail) {
		CFDictionaryRef batteryData = CFDictionaryGetValue(info, CFSTR("BatteryData"));
		if (batteryData) {
			int val = 0;
			CFNumberRef ChemIDNum = CFDictionaryGetValue(batteryData, CFSTR("ChemID"));
			if (ChemIDNum)
				CFNumberGetValue(ChemIDNum, kCFNumberSInt32Type, (void *)&val);
			BI_FORMAT_ITEM_IF(ChemIDNum, _C("Chemistry ID"), "0x%.8X", val);
			CFNumberRef FlagsNum = CFDictionaryGetValue(batteryData, CFSTR("Flags"));
			if (FlagsNum)
				CFNumberGetValue(FlagsNum, kCFNumberSInt32Type, (void *)&val);
			BI_FORMAT_ITEM_IF(FlagsNum, _C("Flags"), "0x%.4X", val);
			CFNumberRef ITMiscNum = CFDictionaryGetValue(batteryData, CFSTR("ITMiscStatus"));
			if (ITMiscNum)
				CFNumberGetValue(ITMiscNum, kCFNumberSInt32Type,(void *)&val);
			BI_FORMAT_ITEM_IF(ITMiscNum, _C("IT Misc Status"), "0x%.4X", val);
		}
	}
	CFTypeRef lastItem;
	for(struct iopm_property *i=iopm_items;i->name;i++) {
		int succ=0;
		for(CFStringRef **ppath=i->candidates;*ppath;ppath++) {
			lastItem=info;
			for(CFStringRef *elem=*ppath;*elem;elem++) {
				if(*elem==(CFStringRef)1) {
					if(CFGetTypeID(lastItem)!=CFArrayGetTypeID()) {
						lastItem=NULL;
						break;
					}
					lastItem=CFArrayGetValueAtIndex(lastItem,0);
				}else{
					if(CFGetTypeID(lastItem)!=CFDictionaryGetTypeID()) {
						lastItem=NULL;
						break;
					}
					lastItem=CFDictionaryGetValue(lastItem,*elem);
				}
				if(!lastItem)
					break;
			}
			if(!lastItem)
				continue;
			int val=0;
			if(i->property_type==500) {
				if(CFGetTypeID(lastItem)!=CFBooleanGetTypeID())
					continue;
				val=CFBooleanGetValue(lastItem);
			}else if(i->property_type==501) {
				if(CFGetTypeID(lastItem)!=CFStringGetTypeID())
					continue;
				CFStringGetCString(lastItem,BI_ENSURE_STR(i->name),256,kCFStringEncodingUTF8);
				succ=1;
				break;
			}else if(i->property_type==502) {
				if(CFGetTypeID(lastItem)!=CFBooleanGetTypeID())
					continue;
				succ=!CFBooleanGetValue(lastItem);
				BI_SET_HIDDEN(i->name,!succ);
				break;
			}else if(i->property_type==503) {
				if(CFGetTypeID(lastItem)!=CFBooleanGetTypeID())
					continue;
				succ=CFBooleanGetValue(lastItem);
				BI_SET_HIDDEN(i->name,!succ);
				break;
			}else{
				if(CFGetTypeID(lastItem)!=CFNumberGetTypeID())
					continue;
				CFNumberGetValue(lastItem,i->property_type?i->property_type:kCFNumberSInt32Type,(void*)&val);
				if(i->property_type==kCFNumberSInt16Type)
					val=(int)(short)val;
			}
			if(i->multiplier) {
				BI_SET_ITEM(i->name,(double)val * i->multiplier);
			}else{
				BI_SET_ITEM(i->name,val);
			}
			succ=1;
			break;
		}
		if(succ)
			BI_SET_HIDDEN(i->name,0);
	}
}

void battery_info_update_iokit(struct battery_info_node *head, bool inDetail) {
	io_service_t service = IOServiceGetMatchingService(0,IOServiceMatching("IOPMPowerSource"));
	CFMutableDictionaryRef info;
	int ret = IORegistryEntryCreateCFProperties(service, &info, 0, 0);
	if (ret != 0) {
		fprintf(stderr, "battery_info_update_iokit: Failed to get info from IOPMPowerSource\n");
		return;
	}
	battery_info_update_iokit_with_data(head, info, inDetail);
	CFRelease(info);
}
#endif

extern const char *cond_localize_c(const char *);

void adapter_info_update_smc(struct battery_info_section *section) {
	if(!section->data[0].name)
		return; // no data need to be freed
	struct battery_info_node *head=section->data;
	struct battery_info_node *head_arr[2] = { head, head };
	mach_port_t adapter_family;
	device_info_t adapter_info;
	charging_state_t charging_stat = is_charging(&adapter_family, &adapter_info);
	if (charging_stat <= 0) {
		bi_destroy_section(section);
		return;
	}
	charger_data_t adapter_data;
	get_charger_data(&adapter_data);
	//BI_SET_HIDDEN(_C("Adapter Details"), 0);
	BI_SET_ITEM(_C("Port"), adapter_info.port);
	/* FIXME: no direct use of cond_localize_c(), do locales like names */
	BI_FORMAT_ITEM(_C("Compatibility"), "%s: %s\n%s: %s", cond_localize_c("External Connected"), adapter_data.ChargerExist ? L_TRUE : L_FALSE, cond_localize_c("Charger Capable"), adapter_data.ChargerCapable ? L_TRUE : L_FALSE);
	BI_FORMAT_ITEM(_C("Type"), "%s (%.8X)", cond_localize_c(get_adapter_family_desc(adapter_family)), adapter_family);
	BI_FORMAT_ITEM(_C("Status"), "%s", (charging_stat == kIsPausing || adapter_data.NotChargingReason != 0) ? cond_localize_c("Not Charging") : cond_localize_c("Charging"));
	BI_SET_ITEM(_C("Current Rating"), adapter_info.current);
	BI_SET_ITEM(_C("Voltage Rating"), adapter_info.voltage);
	BI_SET_ITEM(_C("Charging Current"), adapter_data.ChargingCurrent);
	BI_SET_ITEM(_C("Charging Voltage"), adapter_data.ChargingVoltage);
	BI_FORMAT_ITEM(_C("Charger ID"), "0x%.4X", adapter_data.ChargerId);
	BI_FORMAT_ITEM_IF(*adapter_info.name, _C("Model Name"), "%s", adapter_info.name);
	BI_FORMAT_ITEM_IF(*adapter_info.vendor, _C("Manufacturer"), "%s", adapter_info.vendor);
	BI_FORMAT_ITEM_IF(*adapter_info.adapter, _C("Model"), "%s", adapter_info.adapter);
	BI_FORMAT_ITEM_IF(*adapter_info.firmware, _C("Firmware Version"), "%s", adapter_info.firmware);
	BI_FORMAT_ITEM_IF(*adapter_info.hardware, _C("Hardware Version"), "%s", adapter_info.hardware);
	BI_FORMAT_ITEM_IF(*adapter_info.description, _C("Description"), "%s", adapter_info.description);
	BI_FORMAT_ITEM_IF(*adapter_info.serial, _C("Serial No."), "%s", adapter_info.serial);
	BI_SET_ITEM(_C("PMU Configuration"), adapter_info.PMUConfiguration);
	BI_SET_ITEM(_C("Charger Configuration"), adapter_data.ChargerConfiguration);
	BI_FORMAT_ITEM_IF(adapter_data.NotChargingReason != 0, _C("Reason"), "%s", not_charging_reason_str(adapter_data.NotChargingReason));
	BI_FORMAT_ITEM_IF(adapter_info.port_type != 0, _C("Port Type"), "%s", cond_localize_c(port_type_str(adapter_info.port_type)));
}

void battery_info_update_smc(struct battery_info_section *section) {
	if(!hasSMC) {
		abort(); // this section should not have been added
		/*for(struct battery_info_node *i=head+1;i->name;i++) {
			bi_node_set_hidden(i,0,1);
		}
		battery_info_update_iokit(head,inDetail);*/
		return;
	}
	if(!section->data[0].name)
		return; // no data need to be freed
	struct battery_info_node *head=section->data;
    uint16_t remain_cap, full_cap, design_cap;
    get_capacity(&remain_cap, &full_cap, &design_cap);

    struct battery_info_node *head_arr[2] = { head, head };
    /* Health = 100.0f * FullChargeCapacity (mAh) / DesignCapacity (mAh) */
    BI_SET_ITEM(_C("Health"), 100.0f * (float)full_cap / (float)design_cap);
    /* SoC = 100.0f * RemainCapacity (mAh) / FullChargeCapacity (mAh) */
    BI_SET_ITEM(_C("SoC"), 100.0f * (float)remain_cap / (float)full_cap);
    // No Imperial units here
    BI_SET_ITEM(_C("Avg. Temperature"), get_temperature());
    // // TODO: Charging Type Display {"Battery Power", "AC Power", "UPS Power"}
    charging_state_t charging_stat = is_charging(NULL,NULL);
    BI_SET_ITEM(_C("Charging"), (charging_stat == kIsCharging));
    /* ASoC = 100.0f * RemainCapacity (mAh) / DesignCapacity (mAh) */
    BI_SET_ITEM("ASoC(Hidden)", 100.0f * remain_cap / design_cap);
    //if (inDetail) {
        get_gas_gauge(&gGauge);
        BI_FORMAT_ITEM_IF(strlen(gGauge.DeviceName), _C("Device Name"), "%s", gGauge.DeviceName);
        BI_SET_ITEM(_C("Full Charge Capacity"), full_cap);
        BI_SET_ITEM(_C("Designed Capacity"), design_cap);
        BI_SET_ITEM(_C("Remaining Capacity"), remain_cap);
        BI_SET_ITEM(_C("Battery Uptime"), gGauge.bmsUpTime / 60);
        BI_SET_ITEM(_C("Qmax"), gGauge.Qmax * batt_cell_num());
        BI_SET_ITEM(_C("Depth of Discharge"), gGauge.DOD0);
        BI_SET_ITEM(_C("Passed Charge"), gGauge.PassedCharge);
        BI_SET_ITEM(_C("Voltage"), gGauge.Voltage);
        BI_SET_ITEM(_C("Avg. Current"), gGauge.AverageCurrent);
        BI_SET_ITEM(_C("Avg. Power"), gGauge.AveragePower);
        BI_SET_ITEM(_C("Cell Count"), batt_cell_num());
        /* FIXME: TTE shall display "Never" when -1 */
        int timeToEmpty = get_time_to_empty();
        BI_SET_ITEM_IF(timeToEmpty > 0, _C("Time To Empty"), timeToEmpty);
        BI_SET_ITEM(_C("Cycle Count"), gGauge.CycleCount);
        BI_SET_ITEM_IF(gGauge.DesignCycleCount, _C("Designed Cycle Count"), gGauge.DesignCycleCount)
        BI_SET_ITEM(_C("State Of Charge"), gGauge.StateOfCharge);
        BI_SET_ITEM(_C("State Of Charge (UI)"), gGauge.UISoC);
        BI_SET_ITEM_IF(gGauge.ResScale, _C("Resistance Scale"), gGauge.ResScale);
        if (!battery_serial(BI_ENSURE_STR(_C("Battery Serial No.")))) {
            BI_FORMAT_ITEM(_C("Battery Serial No."), "%s", L_NONE);
        }
        BI_FORMAT_ITEM("Chemistry ID", "0x%.8X", gGauge.ChemID);

        /* Confirmed Flags format */
        /* bq20z45*: Battery Status (0x16):
         * https://www.ti.com/lit/er/sluu313a/sluu313a.pdf */
        BI_FORMAT_ITEM(_C("Flags"), "0x%.4X", gGauge.Flags);
        BI_SET_ITEM_IF(gGauge.TrueRemainingCapacity,
            _C("True Remaining Capacity"),
            gGauge.TrueRemainingCapacity);
        BI_SET_ITEM_IF(gGauge.OCV_Current, _C("OCV Current"), gGauge.OCV_Current);
        BI_SET_ITEM_IF(gGauge.OCV_Voltage, _C("OCV Voltage"), gGauge.OCV_Voltage);
        BI_SET_ITEM_IF(gGauge.IMAX, _C("Max Load Current"), gGauge.IMAX);
        BI_SET_ITEM_IF(gGauge.IMAX2, _C("Max Load Current 2"), gGauge.IMAX2);
        BI_FORMAT_ITEM_IF(gGauge.ITMiscStatus, _C("IT Misc Status"), "0x%.4X", gGauge.ITMiscStatus);
        BI_SET_ITEM_IF(gGauge.SimRate, _C("Simulation Rate"), gGauge.SimRate);
        BI_SET_ITEM_IF(gGauge.DailyMaxSoc, _C("Daily Max SoC"), gGauge.DailyMaxSoc);
        BI_SET_ITEM_IF(gGauge.DailyMinSoc, _C("Daily Min SoC"), gGauge.DailyMinSoc);
#if 0
// TODO: FIXME: Put accessory connection in section context! Do not throw away
            /* Inductive Port Section */
			/* 1: internal, 512: inductive */
			io_connect_t connect = acc_open_with_port(512);
			SInt32 acc_id = get_accid(connect);
			/* 100: No device connected */
			/* TODO: On simulators, fake an accessory to test UI */
            if (acc_id != 100 && acc_id != -1 && connect != MACH_PORT_NULL) {
                BI_SET_HIDDEN(_C("Inductive Port"), 0);
				BI_FORMAT_ITEM(_C("Acc. ID"), "%s", acc_id_string(acc_id));
				SInt32 features = get_acc_allowed_features(connect);
                BI_FORMAT_ITEM_IF(features != -1, _C("Allowed Features"), "0x%.8X", features);
				accessory_info_t accinfo = get_acc_info(connect);
                BI_FORMAT_ITEM_IF(*accinfo.serial, _C("Acc. Serial No."), "%s", accinfo.serial);
                BI_FORMAT_ITEM_IF(*accinfo.vendor, _C("Acc. Manufacturer"), "%s", accinfo.vendor);
				/* TODO: VID/PID from IOHIDDevice */
                //BI_FORMAT_ITEM(_C("Acc. Product ID"), "0x%0.4X", 0x1399);
                BI_FORMAT_ITEM_IF(*accinfo.model, _C("Acc. Model"), "%s", accinfo.model);
                BI_FORMAT_ITEM_IF(*accinfo.name, _C("Acc. Name"), "%s", accinfo.name);
                BI_FORMAT_ITEM_IF(*accinfo.PPID, _C("Acc. PPID"), "%s", accinfo.PPID);
                BI_FORMAT_ITEM_IF(*accinfo.fwVer, _C("Acc. Firmware Version"), "%s", accinfo.fwVer);
                BI_FORMAT_ITEM_IF(*accinfo.hwVer, _C("Acc. Hardware Version"), "%s", accinfo.hwVer);
				BI_FORMAT_ITEM(_C("Battery Pack"), "%s", get_acc_battery_pack_mode(connect) ? L_TRUE : L_FALSE);
				accessory_powermode_t mode = get_acc_powermode(connect);
                BI_FORMAT_ITEM(_C("Power Mode"), "%s: %s\n%s: %s\n%s", cond_localize_c("Configured Mode"), acc_powermode_string(mode.mode), cond_localize_c("Active Mode"), acc_powermode_string(mode.active), acc_powermode_string_supported(mode));
				accessory_sleeppower_t sleep = get_acc_sleeppower(connect);
				if (sleep.supported) {
					BI_FORMAT_ITEM(_C("Sleep Power"), "%s\n%s: %d", sleep.enabled ? cond_localize_c("Enabled") : cond_localize_c("Disabled"), cond_localize_c("Limit"), sleep.limit);
				} else {
					BI_FORMAT_ITEM(_C("Sleep Power"), "%s", cond_localize_c("Unsupported"));
				}
				BI_FORMAT_ITEM(_C("Supervised Acc. Attached"), "%s", get_acc_supervised(connect) ? L_TRUE : L_FALSE);
				BI_FORMAT_ITEM(_C("Supervised Transports Restricted"), "%s", get_acc_supervised_transport_restricted(connect) ? L_TRUE : L_FALSE);
                if (1 /* accessory.transport_type == Inductive_InBand */) {
                    
                }
            } else {
                BI_SET_HIDDEN(_C("Inductive Port"), 1);
            }
#endif
}
