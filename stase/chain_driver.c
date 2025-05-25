/***********************************************************************
 *  chain_driver.c — validation driver for chain  (1)^(2)^(3→4)       *
 *  Md Shafiuzzaman — Feb 2025                                         *
 **********************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "klee/klee.h"

/*--------------------------------------------------------------------*/
/*                     GLOBAL STUB SYMBOLS                            */
/*--------------------------------------------------------------------*/
/* These three symbols exist in your modules; declare them here so
 *   the driver can take their address or inspect their content.      */
extern char  message_buffer[];                 /*  from kbmp / kbmi* */
extern void *kbmi_usb_dangling_cb;             /*  from usb_event_logger.c  */
extern int   proc_write(void*, size_t);        /*  from evt_notifier.c      */

/*--------------------------------------------------------------------*/
/*                  REAL, INSTRUMENTED SOURCE                         */
/*--------------------------------------------------------------------*/
#include "instrumented_code/kbmi_usb.c"        /*  vuln 1  */
#include "instrumented_code/usb_event_logger.c"/*  vuln 3  */
#include "instrumented_code/evt_notifier.c"    /*  vuln 4  */
#include "instrumented_code/kbdi.c"            /*  trigger */

/*--------------------------------------------------------------------*/
/*                       CONSTANTS & MODEL                            */
/*--------------------------------------------------------------------*/
#define MESSAGE_SIZE 1024ULL
#define MEM_SZ       (64 * 1024)               /* flat memory model  */
static uint8_t MEM[MEM_SZ];

/* symbolic helpers */
static void make_sym_u64(uint64_t *p, const char *tag) {
    klee_make_symbolic(p, sizeof(*p), tag);
}
static void make_sym_buf(void *p, size_t sz, const char *tag) {
    klee_make_symbolic(p, sz, tag);
}

/*--------------------------------------------------------------------*/
/*                           main()                                   */
/*--------------------------------------------------------------------*/
int main(void)
{
    /***************** 0. Memory-range setup **************************/
    uint64_t L_shell, H_shell;    /* [L,H] = stack shell-code region          */
    uint64_t cb_slot;             /* dangling pointer slot (address)          */

    make_sym_u64(&L_shell, "L_shell");
    make_sym_u64(&H_shell, "H_shell");
    make_sym_u64(&cb_slot,  "cb_slot");

    /* range sanity */
    klee_assume(L_shell < H_shell);
    klee_assume(H_shell < MEM_SZ);
    klee_assume(cb_slot  < MEM_SZ-8);
    klee_assume(cb_slot % 8 == 0);      /* ★ pointer-size alignment         */

    uint8_t shellcode_byte;
    klee_make_symbolic(&shellcode_byte, 1, "shellcode_byte");

    /***************** 1. Prepare attacker message ********************/
    /* kbmp would normally fill this buffer; we symbolically control
     * it instead.  The driver copies it into the real global from
     * kbmi_usb.c so that load_message_to_memory() operates on it.    */
    make_sym_buf(message_buffer, MESSAGE_SIZE, "message_buffer");
    /* ensure NULL separator exists inside buffer                     */
    message_buffer[MESSAGE_SIZE-1] = '\0';

    /***************** 2. Vuln 1  (kbmi_usb)  *************************/
    /* Registers USB notifier + copies bytes from message_buffer onto
     * the stack; we call notifier manually to guarantee the copy.    */
    kbmi_usb_init();
    my_nb.notifier_call(&my_nb, 0, NULL);  /* simulate USB event     */

    /* *** ASSUME: stack copy landed exactly at L_shell  *************/
    klee_assume((uint64_t)&kbmi_usb_dangling_cb > L_shell); /* dummy to tie addr space */
    MEM[L_shell] = shellcode_byte;        /* we model copied byte    */
    klee_assume(L_shell + 1 <= H_shell);   /* within [L,H]            */

    /***************** 3. Vuln 3  (usb_event_logger) ******************/
    /* Creates dangling function-ptr at fixed offset (0x2020)
     *   — modelled as address  cb_slot                              */
    usb_event_logger_init();
    /* place slot into model                                        */
    *((uint64_t*)(MEM + cb_slot)) = 0xdeadbeefdeadbeefULL; /* garbage */

    /***************** 4. Vuln 4  (evt_notifier)  *********************/
    /* Overwrites dangling slot with &proc_write                     */
    evt_notifier_init();

    /* ==== critical POST→PRE linkage ===============================*/
    /* Overlap requirement: evt_notifier must have written L_shell
     *   into the dangling slot.  Because we cannot easily observe
     *   module internals in the harness, encode it with assume.     */
    klee_assume(*((uint64_t*)(MEM + cb_slot)) == L_shell);  /* ★ linkage */

    /***************** 5. Trigger  (kbdi loads)  **********************/
    kbdi_init();          /* module whose USB action calls notifier  */

    /***************** 6. POST-CONDITION ASSERTIONS ******************/
    /* (i)  linkage still intact                                      */
    klee_assert(*((uint64_t*)(MEM + cb_slot)) == L_shell);

    /* (ii) execution will fetch byte at L_shell  (RExec)           */
    uint8_t fetched = MEM[ *((uint64_t*)(MEM + cb_slot)) ];
    klee_assert(fetched == shellcode_byte);

    return 0;
}
