#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "../klee/klee.h"

#include "chain_harness_template.h"
#include "chain_cfg_eval2.h"   /* auto-generated per chain */


/*  SYMBOLIC MEMORY MODEL */
#define MEM_SZ (64 * 1024)
static uint8_t MEM[MEM_SZ];

typedef struct { uint64_t lo, hi; } range_t;
static range_t RANGE[RANGE_CNT];

// helper
static void call_and_ignore_ret(int (*f)(void)) { if (f) f(); }

/*  MAIN  */
int main(void)
{
    /* 1. allocate symbolic ranges */
    for (int i = 0; i < RANGE_CNT; ++i) {
        uint64_t lo_tmp, hi_tmp;

        klee_make_symbolic(&lo_tmp, sizeof lo_tmp, RANGE_NAME(i));
        klee_make_symbolic(&hi_tmp, sizeof hi_tmp, RANGE_NAME(i));

        klee_assume(lo_tmp < hi_tmp);
        klee_assume(hi_tmp < MEM_SZ);
        klee_assume(lo_tmp + sizeof(uint64_t) <= MEM_SZ);  //safe deref

        RANGE[i].lo = lo_tmp;
        RANGE[i].hi = hi_tmp;
    }
     PRE_CHECK(MEM, RANGE);
    /* 2. execute ordered STEP table */
    for (int i = 0; i < STEP_CNT; ++i) {
        const step_t *s = &STEP_TAB[i];
        switch (s->k) {

        case STEP_CALL:
            call_and_ignore_ret((int(*)(void))s->fn);
            break;

        case STEP_ASSUME_LINK: {
            uint64_t addr = RANGE[s->a].lo;
            klee_assume(addr + sizeof(uint64_t) <= MEM_SZ);  // protect MEM + addr deref
            klee_assume(*(uint64_t*)(MEM + addr) == RANGE[s->b].lo);
            break;
        }
        }
    }
    
   
    POST_CHECK(MEM, RANGE);

    return 0;
}
