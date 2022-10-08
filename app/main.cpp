/**
 * @file main.cpp
 * @author Qasim Khawaja (khawaja@ualberta.ca)
 * @brief Implemention of the Parallel Sort by Regular Sampling algorithm.
 * @version 1.0
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
int main(int argc, char** argv)
{
    bool useUniprocessor = false;
    // Read in p=number of processors, n=number of elements, s=seed
    // std::cout << "Enter p, n, s, useUniprocesser: ";
    // std::cin >> p >> n >> s >> useUniprocessor;
    if (argc != 5) {
        std::cout << "Usage: ./psrs p n s useUniprocessor" << std::endl;
        return 0;
    }
    p = atoi(argv[1]);
    n = atoi(argv[2]);
    s = atoi(argv[3]);
    useUniprocessor = atoi(argv[4]);
    std::cout << "p: " << p << " n: " << n << " s: " << s << " useUniprocessor: " << useUniprocessor << std::endl;

    // set concurrency to p
    pthread_setconcurrency(p);
    // assert(pow(p, 3) <= n);
    // Malloc memory for the array
    // 16 2 17 24 33 28 30 1 0 27 9 25 34 23 19 18 11 7 21 13 8 35 12 29 6 3 4 14 22 15 32 10 26 31 20 5
    // long j[] = { 16, 2, 17, 24, 33, 28, 30, 1, 0, 27, 9, 25, 34, 23, 19, 18, 11, 7, 21, 13, 8, 35, 12, 29, 6, 3, 4, 14, 22, 15, 32, 10, 26, 31, 20, 5 };
    // n = 36;
    A = (long*)malloc((n + 1) * sizeof(long));
    // for (int i = 0; i < n; i++) {
    //     A[i] = j[i];
    // }
    samples = (long*)malloc((p + 1) * (p + 1) * sizeof(long));
    pivots = (long*)malloc((p + 1) * sizeof(long));
    partitionStartEnd = (struct StartEnd*)malloc((p + 1) * (p + 1) * sizeof(struct StartEnd));
    BARRIER_INIT
    // Generate the array
    srandom(s);
    for (long long i = 0; i < n; i++) {
        *(A + i) = random();
    }
    assert(std::is_sorted(A, A + n) == false);

    // std::cout << "Uniprocessor: " << useUniprocessor << std::endl;

    if (useUniprocessor) {
        auto begin = std::chrono::steady_clock::now();
        qsort(A, n, sizeof(long), compare);
        auto end = std::chrono::steady_clock::now();
        assert(std::is_sorted(A, A + n) == true);
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
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
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << std::endl;
    }

    // Clean up
    free(A);
    free(threads);
    free(tcb);
    free(samples);
    free(pivots);
    free(partitionStartEnd);
    BARRIER_DESTROY
    std::cout << "Success" << std::endl;
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
    // std::cout << "Thread " + std::to_string(myId) + " start: " + std::to_string(start) + " end: " + std::to_string(end)+"\n";
    // auto beginPhase1 = std::chrono::steady_clock::now();
    // Quick sort the local array
    long* localA = (long*)malloc((end - start + 1) * sizeof(long));
    for (int i = start; i <= end; i++) {
        localA[i - start] = A[i];
    }
    qsort(localA, end - start + 1, sizeof(long), compare);
    // auto endPhase1 = std::chrono::steady_clock::now();
    BARRIER
    // Collect samples at n/(p*p) intervals
    int w = n / (p * p);
    int startP = myId * p;
    for (int i = 0; i < p; i++) {
        samples[startP + i] = localA[i * w];
    }
    // std::cout << "Thread " + std::to_string(myId) + " Phase 1: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endPhase1 - beginPhase1).count()) + "s\n";
    BARRIER
    // auto beginPhase2 = std::chrono::steady_clock::now();
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
    // auto endPhase2 = std::chrono::steady_clock::now();
    // std::cout << "Thread " + std::to_string(myId) + " Phase 2: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endPhase2 - beginPhase2).count()) + "s\n";
    // auto beginPhase4 = std::chrono::steady_clock::now();
    // K-way merge
    std::vector<long> localSorted;
    std::vector<std::pair<long, int>> heap;

    for (int i = 0; i < p; i++) {
        int currStart = i * p + myId;
        long long currStartIndex = partitionStartEnd[currStart].start;
        long long currEndIndex = partitionStartEnd[currStart].end;
        if (i > 0 && currStartIndex == 0 && currEndIndex == 0) {
            continue;
        }
        if (currStartIndex <= currEndIndex) {
            heap.push_back(std::make_pair(A[currStartIndex], currStart));
            partitionStartEnd[currStart].start++;
        }
    }
    std::make_heap(heap.begin(), heap.end(), comparePair);
    while (heap.size() > 0) {
        std::pair<long, int> min = heap.front();
        std::pop_heap(heap.begin(), heap.end(), comparePair);
        heap.pop_back();
        localSorted.push_back(min.first);

        int currStart = min.second;
        long long currStartIndex = partitionStartEnd[currStart].start;
        long long currEndIndex = partitionStartEnd[currStart].end;
        if (currStartIndex <= currEndIndex) {
            min.first = A[currStartIndex];
            heap.push_back(min);
            std::push_heap(heap.begin(), heap.end(), comparePair);
            partitionStartEnd[currStart].start++;
        }
    }

    // auto endPhase4 = std::chrono::steady_clock::now();
    // std::cout << "Thread " + std::to_string(myId) + " Phase 4: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endPhase4 - beginPhase4).count()) + "s\n";

    // std::string sorted = "";
    // for (long unsigned int i = 0; i < localSorted.size(); i++) {
    //     sorted += std::to_string(localSorted[i]) + " ";
    // }
    // std::cout << "Thread:" + std::to_string(myId) + " " + sorted + "\n";

    BARRIER
    return NULL;
}
