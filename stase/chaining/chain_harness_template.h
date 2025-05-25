/* chain_harness_template.h */
#ifndef CHAIN_HARNESS_TEMPLATE_H
#define CHAIN_HARNESS_TEMPLATE_H

/* step kinds used by every chain */
typedef enum { STEP_CALL, STEP_ASSUME_LINK } stepkind_t;

/* one entry in the ordered STEP table */
typedef struct { stepkind_t k; int a, b; void (*fn)(void); } step_t;

#endif /* CHAIN_HARNESS_TEMPLATE_H */
