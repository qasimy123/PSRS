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
#include <random>
#include <stdio.h>
#include <vector>

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

pthread_t* threads;
long* A;
long* samples;
long* pivots;
struct StartEnd* partitionStartEnd;
int p;
long long n;
int s;

// Add macros for enabling/disabling specific outputs

#if TIMING
std::chrono::_V2::steady_clock::time_point beginPhaseTime;
std::chrono::_V2::steady_clock::time_point endPhaseTime;
#define START_TIMER beginPhaseTime = std::chrono::steady_clock::now();
#define END_TIMER endPhaseTime = std::chrono::steady_clock::now();
#define PRINT_TIME(threadId, phase) \
    std::cout << "Thread: " + std::to_string(threadId) + " Phase: " + std::to_string(phase) + " time: " + std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(endPhaseTime - beginPhaseTime).count()) + "\n";
#else
#define START_TIMER
#define END_TIMER
#define PRINT_TIME(threadId, phase)
#endif

#if DEBUG
#define DEBUG_PRINT_SORTED(x)                          \
    std::string sorted = "";                           \
    for (long unsigned int i = 0; i < x.size(); i++) { \
        sorted += std::to_string(x[i]) + " ";          \
    }                                                  \
    std::cout << "Thread:" + std::to_string(myId) + " " + sorted + "\n";
#else
#define DEBUG_PRINT_SORTED(x)
#endif

#if VALIDATE_SORT
struct MinMaxCount {
    long min;
    long max;
    size_t count;
};
std::vector<struct MinMaxCount> allMinMaxCount;
int compareMinMaxCount(const MinMaxCount& a, const MinMaxCount& b)
{
    return a.min < b.min;
}
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif