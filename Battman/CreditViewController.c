#include "cobjc/cobjc.h"
#include "common.h"

typedef UIViewController CreditVC;

CFStringRef CreditViewControllerGetTitle(void) {
    return _("Credit");
}

CreditVC *CreditViewControllerInit(CreditVC *self) {
	return osupercall(self, initWithStyle:, UITableViewStyleGrouped);
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

void CreditViewControllerDidSelectRow(CreditVC *self, SEL s, UITableView *tv, NSIndexPath *indexPath) {
	open_url(NSIndexPathGetRow(indexPath) ? "https://github.com/LNSSPsd" : "https://github.com/Torrekie");
	UITableViewDeselectRow(tv, indexPath, 1);
}

UITableViewCell *CreditViewCellForRow(CreditVC *self, SEL s, UITableView *tv, NSIndexPath *indexPath) {
	UITableViewCell *cell  = NSObjectNew(UITableViewCell);
	UILabel         *label = UITableViewCellGetTextLabel(cell);
	UILabelSetText(label, NSIndexPathGetRow(indexPath) ? CFSTR("Ruphane") : CFSTR("Torrekie"));
	UILabelSetTextColor(label, UIColorLinkColor());
	return (UITableViewCell *)CFAutorelease(cell);
}

DEFINE_CLASS(CreditViewControllerNew, UITableViewController);
DEFINE_CLASS_METHODS(CreditViewControllerNew, 0);
DEFINE_INSTANCE_METHODS(CreditViewControllerNew, 7);
ADD_METHOD(CreditViewControllerInit, init);
ADD_METHOD(CreditViewControllerGetTitle, title);
ADD_METHOD(CreditViewControllerNumRows, tableView:numberOfRowsInSection:);
ADD_METHOD(CreditViewControllerNumSects, numberOfSectionsInTableView:);
ADD_METHOD(CreditViewControllerTableTitle, tableView:titleForHeaderInSection:);
ADD_METHOD(CreditViewControllerDidSelectRow, tableView:didSelectRowAtIndexPath:);
ADD_METHOD(CreditViewCellForRow, tableView:cellForRowAtIndexPath:);
