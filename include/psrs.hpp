/**
 * @file psrs.hpp
 * @author Qasim Khawaja (khawaja@ualberta.ca)
 * @brief Header file for the PSRS algorithm.
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

/**
 * @brief myPSRS is the main function for the PSRS algorithm.
 * @param arg is a pointer to the thread control block.
 *
 */
void* myPSRS(void* arg);

pthread_barrier_t barrier;
struct ThreadControlBlock {
    int id;
};

struct ThreadControlBlock* tcb;

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