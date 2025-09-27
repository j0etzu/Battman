#include "cobjc/cobjc.h"
#include "common.h"

typedef UIViewController CreditVC;

CFStringRef CreditViewControllerGetTitle(void) {
    return _("Credit");
}

CreditVC *CreditViewControllerInit(CreditVC *self) {
	UITableViewStyle style = UITableViewStyleGrouped;
	if (__builtin_available(iOS 13.0, *))
		style = UITableViewStyleInsetGrouped;
	return osupercall(CreditViewControllerNew,self, initWithStyle:, style);
}

long CreditViewControllerNumRows(void) {
	return 2;
}

long CreditViewControllerNumSects(void) {
	return 1;
}

CFStringRef CreditViewControllerTableTitle(void) {
	return _("Battman Credit");
}

CFNumberRef CVGetRef(id self,void *ref) {
	return CFAutorelease(CFNumberCreate(NULL,kCFNumberSInt64Type,&ref));
}
CFStringRef CVGetRef1(id self,SEL sel) {
	return _(sel_getName(sel));
}

void CreditViewControllerDidSelectRow(CreditVC *self, void *data, UITableView *tv, NSIndexPath *indexPath) {
	open_url(NSIndexPathGetRow(indexPath) ? "https://github.com/LNSSPsd" : "https://github.com/Torrekie");
	UITableViewDeselectRow(tv, indexPath, 1);
}

UITableViewCell *CreditViewCellForRow(CreditVC *self, void *data, UITableView *tv, NSIndexPath *indexPath) {
	UITableViewCell *cell  = NSObjectNew(UITableViewCell);
	UILabel         *label = UITableViewCellGetTextLabel(cell);
	UILabelSetText(label, NSIndexPathGetRow(indexPath) ? CFSTR("Ruphane") : CFSTR("Torrekie"));
	UILabelSetTextColor(label, UIColorLinkColor());
	return (UITableViewCell *)CFAutorelease(cell);
}

MAKE_CLASS(CreditViewControllerNew,UITableViewController,0, \
	CVGetRef1, debugGetRefC,, \
	CreditViewControllerInit, init, \
	CreditViewControllerGetTitle, title, \
	CreditViewControllerNumRows, tableView:numberOfRowsInSection:, \
	CreditViewControllerNumSects, numberOfSectionsInTableView:, \
	CreditViewControllerTableTitle, tableView:titleForHeaderInSection:, \
	CreditViewControllerDidSelectRow, tableView:didSelectRowAtIndexPath:, \
	CreditViewCellForRow, tableView:cellForRowAtIndexPath:, \
	CVGetRef, debugGetRef);
