#pragma once
#include "./cobjc.h"

DefineObjcMethod(void,UITableViewDeselectRow,deselectRowAtIndexPath:animated:,NSIndexPath*,BOOL);

DefineObjcMethod(UILabel*,UITableViewCellGetTextLabel,textLabel);