/**
 * @file psrs.cpp
 * @author Qasim Khawaja (khawaja@ualberta.ca)
 * @brief Implemention of the Parallel Sort by Regular Sampling algorithm.
 * @version 0.1
 * @date 2022-09-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>

#define MASTER if (myId == 0)
#define BARRIER pthread_barrier_wait(&barrier);
#define BARRIER_INIT pthread_barrier_init(&barrier, NULL, p);
#define BARRIER_DESTROY pthread_barrier_destroy(&barrier);

void* myPSRS(void*);

pthread_barrier_t barrier;
struct ThreadControlBlock {
    int id;
};

struct ThreadControlBlock* tcb;

void swapA(long* a, long* b)
{
    long temp = *a;
    *a = *b;
    *b = temp;
}

long* partition(long* start, long* end)
{
    long pivot = *end;
    long* i = start - 1;
    for (long* j = start; j < end; j++) {
        if (*j < pivot) {
            i++;
            swapA(i, j);
        }
    }
    swapA(i + 1, end);
    return i + 1;
}
void sort(long* start, long* end)
{
    if (start >= end)
        return;
    long* pivot = partition(start, end);
    sort(start, pivot - 1);
    sort(pivot + 1, end);
}

struct StartEnd {
    long long start;
    long long end;
};

struct SubArray {
    long* start;
    long* end;
};

// compare function for qsort
int compare(const void* a, const void* b)
{
    return (*(long*)a - *(long*)b);
}

pthread_t* threads;
long* A;
long* samples;
long* pivots;
struct StartEnd* partitionStartEnd;
int p;
long long n;
int s;
struct SubArray* subArrays;
int main()
{
    // Read in p=number of processors, n=number of elements, s=seed

    // std::cin >> p >> n >> s;
    p = 4;
    n = 9000000;
    s = 4;
    assert(pow(p, 3) <= n);
    // Malloc memory for the array
    A = (long*)malloc((n + 1) * sizeof(long));
    samples = (long*)malloc((p + 1) * (p + 1) * sizeof(long));
    pivots = (long*)malloc((p + 1) * sizeof(int));
    partitionStartEnd = (struct StartEnd*)malloc((p + 1) * (p + 1) * sizeof(struct StartEnd));
    subArrays = (struct SubArray*)malloc((p + 1) * sizeof(struct SubArray));
    BARRIER_INIT
    // Generate the array
    srand(s);
    for (long long i = 0; i < n; i++) {
        *(A + i) = random();
    }
    std::cout << "Is sorted: " << std::is_sorted(A, A + n) << std::endl;
    bool tryUniprocessor = false;
    std::cin >> tryUniprocessor;
    std::cout << "Try uniprocessor: " << tryUniprocessor << std::endl;
    // Start the timer
    auto begin = std::chrono::high_resolution_clock::now();

    if (tryUniprocessor) {
        sort(A, A + n - 1);
        std::cout << "Is sorted: " << std::is_sorted(A, A + n) << std::endl;
    } else {
        // Create the threads
        threads = (pthread_t*)malloc((p + 1) * sizeof(pthread_t));
        tcb = (struct ThreadControlBlock*)malloc((p + 1) * sizeof(struct ThreadControlBlock));

        for (int i = 1; i < p; i++) {
            tcb[i].id = i;
            pthread_create(&threads[i], NULL, myPSRS, (void*)&tcb[i]);
        }

        tcb[0].id = 0;
        myPSRS((void*)&tcb[0]);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "ns" << std::endl;
    // Clean up
    free(A);
    free(threads);
    free(tcb);
    BARRIER_DESTROY
    return 0;
}

void* myPSRS(void* arg)
{
    struct ThreadControlBlock* myTCB;
    int myId;

    myTCB = (struct ThreadControlBlock*)arg;
    myId = myTCB->id;
    BARRIER
    long long start = myId * n / p;
    long long end = (myId + 1) * n / p - 1;

    // Quick sort the local array
    sort(A + start, A + end);

    BARRIER

    // Collect samples at n/(p*p) intervals
    int w = n / (p * p);
    for (int i = 0; i < p; i++) {
        samples[myId * p + i] = A[start + i * w];
    }

    BARRIER
    // Sort the samples
    MASTER
    {
        sort(samples, samples + p * p - 1);
        // Select the pivots

        for (int i = 0; i < (p - 1); i++) {
            pivots[i] = samples[(i + 1) * p];
        }
    }
    BARRIER
    // For each pivot, find the number of elements in the partition
    long long i = start;
    int pivotIndex = 0;
    long long last = start;
    assert(pivotIndex < p);
    while (i <= end) {
        if (pivotIndex == (p - 1) || A[i] <= pivots[pivotIndex]) {
            partitionStartEnd[myId * p + pivotIndex].start = last;
            partitionStartEnd[myId * p + pivotIndex].end = i;
            i++;
        } else {
            pivotIndex++;
            last = i;
        }
    }
    BARRIER
    MASTER
    {
        long long total = 0;
        for (int i = 0; i < p; i++) {
            for (int j = 0; j < p; j++) {
                if (i > 0 && partitionStartEnd[i * p + j].start == 0 && partitionStartEnd[i * p + j].end == 0) {
                    continue;
                }
                total += partitionStartEnd[i * p + j].end - partitionStartEnd[i * p + j].start + 1;
            }
        }
    }
    BARRIER

    // K-way merge
    long long count = 0;
    for (int i = 0; i < p; i++) {
        if (i > 0 && partitionStartEnd[i * p + myId].start == 0 && partitionStartEnd[i * p + myId].end == 0) {
            continue;
        }
        count += partitionStartEnd[i * p + myId].end - partitionStartEnd[i * p + myId].start + 1;
    }
    long* localSorted = (long*)malloc((count + 1) * sizeof(long));
    long long index = 0;
    while (index < count) {
        long min = LONG_MAX;
        int minIndex = -1;
        for (int i = 0; i < p; i++) {
            if (i > 0 && partitionStartEnd[i * p + myId].start == 0 && partitionStartEnd[i * p + myId].end == 0) {
                continue;
            }
            if (partitionStartEnd[i * p + myId].start <= partitionStartEnd[i * p + myId].end) {
                if (A[partitionStartEnd[i * p + myId].start] < min) {
                    min = A[partitionStartEnd[i * p + myId].start];
                    minIndex = i;
                }
            }
        }
        localSorted[index] = min;
        partitionStartEnd[minIndex * p + myId].start++;
        index++;
    }
    subArrays[myId].start = localSorted;
    subArrays[myId].end = localSorted + count - 1;
    BARRIER
    MASTER
    {
        long long j = 0;
        for (int i = 0; i < p; i++) {
            for (long* k = subArrays[i].start; k <= subArrays[i].end; k++) {
                A[j] = *k;
                j++;
            }
        }
    }
    BARRIER

    return NULL;
}
