#ifndef P2_H_
#define P2_H_

typedef struct train_info{
    int train_number;
    char direction;
    int loading_time;
    int crossing_time;
    pthread_cond_t *train_convar;
} t;

#endif
