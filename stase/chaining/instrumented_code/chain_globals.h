/* === shared symbolic objects === */
extern uint8_t payload_buf[1024];
extern uint8_t *payload_ptr;            /* = payload_buf                */
extern void (*fp_slot_ptr)(void);       /* hijacked function pointer    */
extern uint8_t g_sym_payload_byte;      /* attacker-controlled byte     */

