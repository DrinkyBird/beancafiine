#ifndef BEANCAFIINE_BEANDEF_H
#define BEANCAFIINE_BEANDEF_H

#include <stdint.h>

typedef union {
    struct {
        uint32_t type, id;
    };

    uint64_t value;
} titleid_t;

void socket_perror(const char *msg);

#endif //BEANCAFIINE_BEANDEF_H
