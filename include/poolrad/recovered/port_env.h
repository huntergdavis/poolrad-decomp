#ifndef POOLRAD_RECOVERED_PORT_ENV_H
#define POOLRAD_RECOVERED_PORT_ENV_H

#include <stdint.h>

/* Port environment for raw-CPU models of port-bound routines (docs/re/RES_00BA_16EC_PORT_HARNESS.md). A model calls
 * `out` for each OUT it reaches, in order, with the instruction's CS:IP, before the instruction completes; `in` (may be
 * NULL when a model has no IN) supplies the value an IN reads. A nonzero return stops the model at that instruction. In
 * audits the environment records the model's OUTs and replays recorded INs, and the sequence must equal the native
 * trace. */
typedef struct {
    void *ctx;
    int (*out)(void *ctx, uint16_t port, uint16_t value, unsigned size, uint16_t cs, uint16_t ip);
    int (*in)(void *ctx, uint16_t port, unsigned size, uint16_t cs, uint16_t ip, uint16_t *value);
} pr_port_env;

#endif
