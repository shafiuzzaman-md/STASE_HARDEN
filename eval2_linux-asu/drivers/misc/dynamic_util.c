#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/pgtable.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/sched/task_stack.h>
#include <asm/stacktrace.h>

void make_dynamic_area(void) {
    struct task_struct *task = current;
    unsigned long stack_start = (unsigned long)task_stack_page(task);
    unsigned long addy =((unsigned long int)stack_start + (PAGE_SIZE *3));

    pte_t* pte = virt_to_kpte(addy);
    if (!pte) goto out;

    if (!pte_exec(*pte)) {
        set_pte_at(current->mm, addy, pte, pte_mkexec(*pte));
    }

    out:
}

static int __init dynamic_util_driver_init(void) {
    return 0;
}

core_initcall(dynamic_util_driver_init);

EXPORT_SYMBOL(make_dynamic_area); 