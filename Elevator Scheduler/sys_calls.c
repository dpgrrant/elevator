#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>

int (*STUB_issue_request)(int,int,int) = NULL;
EXPORT_SYMBOL(STUB_issue_request);
asmlinkage int sys_issue_request(int current_floor, int final_floor,int pet_type) {
	if (STUB_issue_request)
		return STUB_issue_request(current_floor, final_floor,pet_type);
	else
		return -ENOSYS;
}

int (*STUB_start_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);
asmlinkage int sys_start_elevator(void) {
	if (STUB_start_elevator)
		return STUB_start_elevator();
	else
		return -ENOSYS;
}

int (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_stop_elevator);
asmlinkage int sys_stop_elevator(void) {
	if (STUB_stop_elevator)
		return STUB_stop_elevator();
	else
		return -ENOSYS;
}
