#pragma once
#include "types.h"

/* clone */
pid_t clone(int flag, void *stack, pid_t ptid, void *tls, pid_t ctid);