#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include "timer.h"

#define RADIUS 1.0

struct thread_info {
    int thread_num;
    long attempts;
};

long successful_results = 0;
pthread_mutex_t mtx;

void seed_rand(int thread_n, struct drand48_data *buffer) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    srand48_r(tv.tv_sec * thread_n + tv.tv_usec, buffer);
}

void* mkm_runner(void* t_info) {
    struct thread_info *info = t_info;
    struct drand48_data drand_buffer;
    long local_attempts = info->attempts;
    long local_succeeds = 0;
    seed_rand(info->thread_num, &drand_buffer);
    for (long i = 0; i < local_attempts; i++) {
        double x_coord;
        double y_coord;
	drand48_r(&drand_buffer, &x_coord);
	drand48_r(&drand_buffer, &y_coord);
        if (x_coord * x_coord + y_coord * y_coord < RADIUS) {
            local_succeeds++;
        }
    }

    pthread_mutex_lock(&mtx);
    successful_results += local_succeeds;
    pthread_mutex_unlock(&mtx);
}

int main(int argc, char** argv) {

    srand(time(NULL));
    int thread_count = strtol(argv[1], NULL, 10);
    long total_attempts = strtol(argv[2], NULL, 10);
    pthread_mutex_init(&mtx, NULL);

    long attempts_per_thread = total_attempts / thread_count;
    long last_thread_attempts = attempts_per_thread;

    if (total_attempts % thread_count != 0) {
        last_thread_attempts += (total_attempts % thread_count);
    }

    struct thread_info *info = malloc(sizeof(struct thread_info) * thread_count);
    for (int i = 0; i < thread_count; i++) {
	info[i].thread_num = i;
        if (i == thread_count - 1) {
	    info[i].attempts = last_thread_attempts;
	} else {
	    info[i].attempts = attempts_per_thread;
	}
    }

    pthread_t* thread_handlers = malloc(sizeof(pthread_t) * thread_count);

    double time_start = 0.0;
    GET_TIME(time_start);

    for (int i = 0; i < thread_count; i++) {
        int err = pthread_create(&thread_handlers[i], NULL, mkm_runner, (void*) &info[i]);

        if (err != 0) {
            perror("error while crearting thread\n");
        }
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(thread_handlers[i], NULL);
    }

    double time_end = 0.0;
    GET_TIME(time_end);

    double pi_apprx = 4.0 * (double)successful_results / ((double)total_attempts);

    printf("approximal PI=%lf\n", pi_apprx);
    printf("calculated for %lf seconds\n", time_end - time_start);

    pthread_mutex_destroy(&mtx);
    free(thread_handlers);
    free(info);
    
    return 0;
}
