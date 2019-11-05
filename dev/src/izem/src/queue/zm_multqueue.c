#include "queue/zm_multqueue.h"
#include "queue/zm_glqueue.h"
#include "stdio.h"
#include "stdlib.h"
#include <pthread.h>

#define CORES 6
#define QUEUES_PER_CORE 2

int zm_multqueue_init(zm_multqueue_t *q) {
    q->threads_num = CORES; // TODO (AT): evaluate number of cores dynamically
    q->queues_per_thread = QUEUES_PER_CORE;
    q->queues_num = q->queues_per_thread * q->threads_num;

    q->locks = (pthread_mutex_t *) malloc(q->queues_num * sizeof(pthread_mutex_t));
    q->queues = (zm_glqueue_t *)
            malloc(q->queues_num * sizeof(zm_glqueue_t));
    for (int i = 0; i < q->queues_num; ++i) {
        pthread_mutex_init(&q->locks[i], NULL);
        zm_glqueue_init(&q->queues[i]);
        //printf("multqueue %d init\n", i);
    }

    return 0;
}

inline int get_random_queue_index_half(int threads_num) { return rand() % (threads_num / 2); }

inline int get_random_queue_index(int threads_num) { return rand() % threads_num; }

inline void insert(zm_multqueue_t *q, void *data) {
    int queue_index;

    pthread_id_np_t tid;
    pthread_t self;
    self = pthread_self();
    pthread_getunique_np(&self, &tid);

    pthread_mutex_t *locks = q->locks;
    do {
        if (tid < q->threads_num) {
            queue_index = get_random_queue_index_half(q->threads_num);
        } else {
            queue_index = get_random_queue_index_half(q->threads_num) + q->threads_num / 2;
        }
    } while (pthread_mutex_lock(&locks[queue_index]) != 0);
    //printf("multqueue %d enqueue\n", queue_index);
    zm_glqueue_enqueue(&q->queues[queue_index], data);
    pthread_mutex_unlock(&locks[queue_index]);
}

int zm_multqueue_enqueue(zm_multqueue_t *q, void *data) {
    insert(q, data);
    return 0;
}


inline void pop(zm_multqueue_t *q, void **data) {
    int queue_index;
    pthread_mutex_t *locks = q->locks;
    int it = 0;
    pthread_id_np_t tid;
    pthread_t self;
    self = pthread_self();
    pthread_getunique_np(&self, &tid);
    do {
        if (it == 0) {
            queue_index = tid * QUEUES_PER_CORE;
            ++it;
            secondQueueIndex = getRandomQueueIndexForHalf();
        } else if (it == 1) {
            queue_index = tid * QUEUES_PER_CORE + 1;
            ++it;
        } else {
            queue_index = get_random_queue_index_half(q->threads_num);
        }
    } while (pthread_mutex_lock(&locks[queue_index]) != 0);
    //printf("multqueue %d dequeue\n", queue_index);
    zm_glqueue_dequeue(&q->queues[queue_index], data);
    pthread_mutex_unlock(&locks[queue_index]);
}

int zm_multqueue_dequeue(zm_multqueue_t *q, void **data) {
    pop(q, data);
    return 1;
}