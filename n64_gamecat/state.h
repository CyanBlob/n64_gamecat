#ifndef N64_GAMECAT_STATE_H
#define N64_GAMECAT_STATE_H

typedef enum {
    IDLE,
    ADDR_H,
    ADDR_L,
    READ_H,
    READ_IDLE,
    READ_L,
    WRITE_H,
    WRITE_L
} State;

#endif //N64_GAMECAT_STATE_H
