#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define NUM_THREADS 4
#define ITERATIONS 1000000

#define ARRAY_SIZE 10000

/* =====================================================
   Part A: Race Condition Demo
   ===================================================== */

static int race_counter = 0;

int race_worker(void *arg)
{
    (void)arg;

    for (int i = 0; i < ITERATIONS; i++)
    {
        race_counter++;
    }

    return 0;
}

/* =====================================================
   Part A: Mutex-Protected Counter
   ===================================================== */

typedef struct
{
    int *counter;
    mtx_t *lock;
} CounterData;

int safe_worker(void *arg)
{
    CounterData *data = arg;

    for (int i = 0; i < ITERATIONS; i++)
    {
        if (mtx_lock(data->lock) != thrd_success)
        {
            return 1;
        }

        (*data->counter)++;

        if (mtx_unlock(data->lock) != thrd_success)
        {
            return 1;
        }
    }

    return 0;
}

/* =====================================================
   Part B: Parallel Array Sum
   ===================================================== */

typedef struct
{
    int *array;
    int start;
    int end;
    long long partial_sum;
} SumTask;

int sum_worker(void *arg)
{
    SumTask *task = arg;

    task->partial_sum = 0;

    for (int i = task->start; i < task->end; i++)
    {
        task->partial_sum += task->array[i];
    }

    return 0;
}

/* =====================================================
   Main
   ===================================================== */

int main(void)
{
    printf("========== PART A ==========\n");
    printf("Race Condition Demo\n\n");

    thrd_t race_threads[NUM_THREADS];
    int race_created[NUM_THREADS] = {0};

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (thrd_create(&race_threads[i],
                        race_worker,
                        NULL) == thrd_success)
        {
            race_created[i] = 1;
        }
        else
        {
            fprintf(stderr,
                    "Failed to create race thread %d\n",
                    i);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (race_created[i])
        {
            thrd_join(race_threads[i], NULL);
        }
    }

    printf("Expected: %d\n",
           NUM_THREADS * ITERATIONS);
    printf("Actual:   %d\n\n",
           race_counter);

    printf("Mutex-Protected Version\n\n");

    int counter = 0;
    int expected = NUM_THREADS * ITERATIONS;

    mtx_t lock;

    if (mtx_init(&lock, mtx_plain) != thrd_success)
    {
        fprintf(stderr,
                "Failed to initialize mutex\n");
        return EXIT_FAILURE;
    }

    CounterData counter_data = {
        .counter = &counter,
        .lock = &lock
    };

    thrd_t safe_threads[NUM_THREADS];
    int safe_created[NUM_THREADS] = {0};

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (thrd_create(&safe_threads[i],
                        safe_worker,
                        &counter_data) == thrd_success)
        {
            safe_created[i] = 1;
        }
        else
        {
            fprintf(stderr,
                    "Failed to create safe thread %d\n",
                    i);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        if (safe_created[i])
        {
            int result;

            if (thrd_join(safe_threads[i],
                          &result) != thrd_success)
            {
                fprintf(stderr,
                        "Failed to join safe thread %d\n",
                        i);
            }

            if (result != 0)
            {
                fprintf(stderr,
                        "Thread %d returned error %d\n",
                        i,
                        result);
            }
        }
    }

    printf("Expected: %d\n", expected);
    printf("Actual:   %d\n\n", counter);

    assert(counter == expected);

    mtx_destroy(&lock);

    /* ================================================= */

    printf("========== PART B ==========\n");
    printf("Parallel Array Sum\n\n");

    int numbers[ARRAY_SIZE];

    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        numbers[i] = i + 1;
    }

    long long sequential_total = 0;

    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        sequential_total += numbers[i];
    }

    thrd_t sum_threads[NUM_THREADS];
    SumTask tasks[NUM_THREADS];

    int chunk = ARRAY_SIZE / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        tasks[i].array = numbers;
        tasks[i].start = i * chunk;

        if (i == NUM_THREADS - 1)
        {
            tasks[i].end = ARRAY_SIZE;
        }
        else
        {
            tasks[i].end = (i + 1) * chunk;
        }

        tasks[i].partial_sum = 0;

        if (thrd_create(&sum_threads[i],
                        sum_worker,
                        &tasks[i]) != thrd_success)
        {
            fprintf(stderr,
                    "Failed to create sum thread %d\n",
                    i);
            return EXIT_FAILURE;
        }
    }

    long long parallel_total = 0;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        thrd_join(sum_threads[i], NULL);
        parallel_total += tasks[i].partial_sum;
    }

    printf("Sequential total: %lld\n",
           sequential_total);

    printf("Parallel total:   %lld\n",
           parallel_total);

    assert(parallel_total == sequential_total);

    printf("\nAll tests passed.\n");

    return EXIT_SUCCESS;
}
