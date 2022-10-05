/**
 * @file main.cpp
 * @author Qasim Khawaja (khawaja@ualberta.ca)
 * @brief Implemention of the Parallel Sort by Regular Sampling algorithm.
 * @version 0.1
 * @date 2022-09-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "psrs.hpp"
int main()
{
    // Read in p=number of processors, n=number of elements, s=seed
    std::cout << "Enter p, n, s: ";
    std::cin >> p >> n >> s;

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
    assert(std::is_sorted(A, A + n) == false);
    bool tryUniprocessor = false;
    std::cout << "Try uniprocessor? (1/0): ";
    std::cin >> tryUniprocessor;
    std::cout << "Uniprocessor: " << tryUniprocessor << std::endl;
    // Start the timer

    if (tryUniprocessor) {
        auto begin = std::chrono::steady_clock::now();
        qsort(A, n, sizeof(long), compare);
        auto end = std::chrono::steady_clock::now();
        assert(std::is_sorted(A, A + n) == true);
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "ns" << std::endl;

    } else {
        // Create the threads
        threads = (pthread_t*)malloc((p + 1) * sizeof(pthread_t));
        tcb = (struct ThreadControlBlock*)malloc((p + 1) * sizeof(struct ThreadControlBlock));

        for (int i = 1; i < p; i++) {
            tcb[i].id = i;
            pthread_create(&threads[i], NULL, myPSRS, (void*)&tcb[i]);
        }

        tcb[0].id = 0;
        auto begin = std::chrono::steady_clock::now();
        myPSRS((void*)&tcb[0]);
        auto end = std::chrono::steady_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "ns" << std::endl;
    }

    // Clean up
    free(A);
    free(threads);
    free(tcb);
    free(samples);
    free(pivots);
    free(partitionStartEnd);
    free(subArrays);
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
    qsort(A + start, end - start + 1, sizeof(long), compare);

    // Collect samples at n/(p*p) intervals
    int w = n / (p * p);
    for (int i = 0; i < p; i++) {
        samples[myId * p + i] = A[start + i * w];
    }

    BARRIER
    // Sort the samples
    MASTER
    {
        qsort(samples, p * p, sizeof(long), compare);
        // Select the pivots

        for (int i = 0; i < (p - 1); i++) {
            pivots[i] = samples[(i + 1) * p];
        }
    }
    BARRIER
    // Use binary search to find the partition boundaries
    for (int i = 0; i < p; i++) {
        partitionStartEnd[myId * p + i].start = std::lower_bound(A + start, A + end + 1, pivots[i]) - A;
        partitionStartEnd[myId * p + i].end = std::upper_bound(A + start, A + end + 1, pivots[i]) - A - 1;
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
    BARRIER
    assert(std::is_sorted(localSorted, localSorted + count) == true);
    return NULL;
}
