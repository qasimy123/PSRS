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
#include <vector>
bool comparePair(const std::pair<long, int>& a, const std::pair<long, int>& b)
{
    return a.first > b.first;
}
int main()
{
    // Read in p=number of processors, n=number of elements, s=seed
    std::cout << "Enter p, n, s: ";
    std::cin >> p >> n >> s;
    // set concurrency to p
    pthread_setconcurrency(p);
    // assert(pow(p, 3) <= n);
    // Malloc memory for the array
    A = (long*)malloc((n + 1) * sizeof(long));
    samples = (long*)malloc((p + 1) * (p + 1) * sizeof(long));
    pivots = (long*)malloc((p + 1) * sizeof(int));
    partitionStartEnd = (struct StartEnd*)malloc((p + 1) * (p + 1) * sizeof(struct StartEnd));
    subArrays = (struct SubArray*)malloc((p + 1) * sizeof(struct SubArray));
    BARRIER_INIT
    // Generate the array
    srandom(s);
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
        std::cout << "Uniprocessor" << std::endl;
        auto begin = std::chrono::steady_clock::now();
        qsort(A, n, sizeof(long), compare);
        auto end = std::chrono::steady_clock::now();
        assert(std::is_sorted(A, A + n) == true);
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1000000000.0 << "s" << std::endl;

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
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1000000000.0 << "s" << std::endl;
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
    auto beginPhase1 = std::chrono::steady_clock::now();
    // Quick sort the local array
    qsort(&A[start], end - start + 1, sizeof(long), compare);
    auto endPhase1 = std::chrono::steady_clock::now();
    BARRIER
    // Collect samples at n/(p*p) intervals
    int w = n / (p * p);
    int startP = myId * p;
    for (int i = 0; i < p; i++) {
        samples[startP + i] = A[start + i * w];
    }
    std::cout << "Thread " + std::to_string(myId) + " Phase 1: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endPhase1 - beginPhase1).count()) + "s\n";
    BARRIER
    auto beginPhase2 = std::chrono::steady_clock::now();
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
    partitionStartEnd[startP].start = start;
    partitionStartEnd[startP + p - 1].end = end;
    for (int i = 0; i < p - 1; i++) {
        int index = startP + i;
        partitionStartEnd[index].end = std::lower_bound(A + partitionStartEnd[index].start, A + end + 1, pivots[i]) - A - 1;
        partitionStartEnd[index + 1].start = partitionStartEnd[index].end + 1;
    }
    BARRIER
    auto endPhase2 = std::chrono::steady_clock::now();
    std::cout << "Thread " + std::to_string(myId) + " Phase 2: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endPhase2 - beginPhase2).count()) + "s\n";
    auto beginPhase4 = std::chrono::steady_clock::now();
    // K-way merge
    std::vector<long> localSorted;
    while (true) {
        long min = LONG_MAX;
        int minIndex = -1;
        for (int i = 0; i < p; i++) {
            int currStart = i * p + myId;
            if (i > 0 && partitionStartEnd[currStart].start == 0 && partitionStartEnd[currStart].end == 0) {
                continue;
            }
            if (partitionStartEnd[currStart].start <= partitionStartEnd[currStart].end) {
                if (A[partitionStartEnd[currStart].start] < min) {
                    min = A[partitionStartEnd[currStart].start];
                    minIndex = i;
                }
            }
        }
        if (minIndex == -1) {
            break;
        }
        localSorted.push_back(min);
        partitionStartEnd[minIndex * p + myId].start++;
    }
    auto endPhase4 = std::chrono::steady_clock::now();
    std::cout << "Thread " + std::to_string(myId) + " Phase 4: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endPhase4 - beginPhase4).count()) + "s\n";

    // std::string sorted = "";
    // for (long unsigned int i = 0; i < localSorted.size(); i++) {
    //     sorted += std::to_string(localSorted[i]) + " ";
    // }
    // std::cout << "Thread:" + std::to_string(myId) + " " + sorted + "\n";

    BARRIER
    return NULL;
}
