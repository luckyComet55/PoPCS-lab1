#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

#define BAIL_OUT 4.0
#define MAX_ITERATIONS 1000
#define MAX_X +1.5
#define MIN_X -2.5
#define MAX_Y +2.0
#define MIN_Y -2.0

typedef struct thread_info {
    int thread_num;
    long points;
    FILE *fp;
} thread_info_t;

typedef struct complex_point {
    double x;
    double y;
} complex_point_t;

pthread_mutex_t mtx;

complex_point_t get_rand_point(struct drand48_data *drand_buffer) {
    double x_tmp, y_tmp;
    drand48_r(drand_buffer, &x_tmp);
    drand48_r(drand_buffer, &y_tmp);
    double x = (MAX_X - MIN_X) * x_tmp + MIN_X;
    double y = (MAX_Y - MIN_Y) * y_tmp + MIN_Y;
    complex_point_t p;
    p.x = x;
    p.y = y;
    return p;
}

unsigned char check_mandelbrot(complex_point_t c, complex_point_t z0, long max_iterations) {
    long current_iterations = 0;
    while (current_iterations != max_iterations) {
	double z0_x2 = z0.x * z0.x;
	double z0_y2 = z0.y * z0.y;
	z0.y = 2 * z0.x * z0.y + c.y;
        z0.x = z0_x2 - z0_y2 + c.x;
	z0_x2 = z0.x * z0.x;
	z0_y2 = z0.y * z0.y;
	if (z0_x2 + z0_y2 >= BAIL_OUT) {
	    return 0;
	}
	current_iterations++;
    }
    return 1;
}

void seed_rand(int thread_n, struct drand48_data *buffer) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    srand48_r(tv.tv_sec * thread_n + tv.tv_usec, buffer);
}

void* mandelbrot_runner(void* data) {
    thread_info_t *info = data;
    struct drand48_data drand_buffer;
    const long goal_points = info->points;
    long generated_points = 0;
    FILE *file = info->fp;
    seed_rand(info->thread_num, &drand_buffer);
    complex_point_t z0 = {0, 0};
    while (generated_points != goal_points) {
        complex_point_t c = get_rand_point(&drand_buffer);
        unsigned char check_res = check_mandelbrot(c, z0, MAX_ITERATIONS);
	if (check_res == 1) {
	    generated_points++;
	    pthread_mutex_lock(&mtx);
            fprintf(file, "%lf,%lf\n", c.x, c.y);
	    pthread_mutex_unlock(&mtx);
	}
    }
}


int main(int argc, char** argv) {
    int thread_count = strtol(argv[1], NULL, 10);
    long npoints = strtol(argv[2], NULL, 10);
    pthread_mutex_init(&mtx, NULL);
    pthread_t* thread_handlers = malloc(sizeof(pthread_t) * thread_count);
    thread_info_t *info = malloc(sizeof(thread_info_t) * thread_count);
    char *filename = "mandelbrot_res.csv";
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "X,Y\n");
    long points_per_thread = npoints / thread_count;
    long last_thread_points = points_per_thread;
    if (npoints % thread_count != 0) {
	last_thread_points += (npoints % thread_count);
    }

    for (int i = 0; i < thread_count; i++) {
	long points = points_per_thread;
	if (i == thread_count - 1) {
	    points = last_thread_points;
	}
        info[i].thread_num = i;
        info[i].points = points;
        info[i].fp = fp;
    }

    for (int i = 0; i < thread_count; i++) {
	int err = pthread_create(&thread_handlers[i], NULL, mandelbrot_runner, (void*) &info[i]);

	if (err != 0) {
	    perror("error while crearting thread\n");
	}
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(thread_handlers[i], NULL);
    }


    free(thread_handlers);
    free(info);
    fclose(fp);
    pthread_mutex_destroy(&mtx);

    return 0;
}
