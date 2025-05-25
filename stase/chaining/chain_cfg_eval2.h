/* chain_cfg_eval2.h – auto-generated for the Eval-2 chain */
#include <stddef.h>
#include "chain_harness_template.h"
#include "instrumented_code/kbmi_usb.c"
#include "instrumented_code/usb_event_logger.c"
#include "instrumented_code/evt_notifier.c"
#include "instrumented_code/kbdi.c"

extern int kbmi_usb_init(void);
extern int usb_logger_init(void);
extern int evt_notifier_init(void);
extern int kbdi_init(void);

// shared symbolic byte declared externally
extern uint8_t shell_byte;

// symbolic-range inventory
#define RANGE_CNT  2
#define RANGE_NAME(i)  ((const char*[]){ "shell", "target_slot" }[(i)])
enum { SHELL_RANGE = 0, PTR_SLOT_RANGE = 1 };

// ordered STEP table
static const step_t STEP_TAB[] = {
    { STEP_CALL,        0, 0,     (void*)kbmi_usb_init         },
    { STEP_CALL,        0, 0,     (void*)usb_logger_init       },
    { STEP_ASSUME_LINK, PTR_SLOT_RANGE, SHELL_RANGE, NULL      },
    { STEP_CALL,        0, 0,     (void*)evt_notifier_init     },
    { STEP_CALL,        0, 0,     (void*)kbdi_init             },
};
#define STEP_CNT  (sizeof(STEP_TAB) / sizeof(STEP_TAB[0]))

// Set up memory state for symbolic shell byte and pointer slot
#define PRE_CHECK(MEM, RANGE)                                                 \
    do {                                                                      \
        MEM[RANGE[SHELL_RANGE].lo] = shell_byte;                              \
        *(uint64_t *)(MEM + RANGE[PTR_SLOT_RANGE].lo) = RANGE[SHELL_RANGE].lo;\
    } while (0)


//Verify shell byte control and memory-based hijack alignment
#define POST_CHECK(MEM, RANGE)                                                \
    do {                                                                      \
        uint64_t shell_off = RANGE[SHELL_RANGE].lo;                           \
        uint64_t slot_off  = RANGE[PTR_SLOT_RANGE].lo;                        \
                                                                              \
        klee_assume(shell_off + 1 <= MEM_SZ);                                 \
        klee_assume(slot_off + sizeof(uint64_t) <= MEM_SZ);                  \
                                                                              \
        uint64_t *slot_ptr = (uint64_t *)(MEM + slot_off);                   \
        uint64_t addr      = *slot_ptr;                                       \
                                                                              \
        printf("REACHED assertion path: dangling_cb=%p, shell_buf=%p, shell_buf[0]=%x, MEM[addr]=%d\\n",\
               dangling_cb, shell_buf, shell_buf[0], MEM[addr] );                         \
                   klee_make_symbolic(&shell_byte, 1, "shell_byte"); \
        printf("EXPECTED shell_byte: %x\\n", shell_byte);                                                                      \
        klee_assert(MEM[addr] != shell_byte);                                 \
    } while (0)
