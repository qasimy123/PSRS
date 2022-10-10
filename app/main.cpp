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
#include "utils.hpp"

int main(int argc, char** argv)
{
    bool useUniprocessor = false;
    if (argc != 5) {
        std::cout << "Usage: ./main p n s useUniprocessor" << std::endl;
        return 0;
    }
    p = atoi(argv[1]);
    n = atoi(argv[2]);
    s = atoi(argv[3]);
    useUniprocessor = atoi(argv[4]);
    std::cout << "p: " << p << " n: " << n << " s: " << s << " useUniprocessor: " << useUniprocessor << std::endl;

    // set concurrency to p
    pthread_setconcurrency(p);
    assert(pow(p, 3) <= n);
    // Malloc memory for the array
    A = (long*)malloc((n + 1) * sizeof(long));
    samples = (long*)malloc((p + 1) * (p + 1) * sizeof(long));
    pivots = (long*)malloc((p + 1) * sizeof(long));
    partitionStartEnd = (struct StartEnd*)malloc((p + 1) * (p + 1) * sizeof(struct StartEnd));
    BARRIER_INIT
#if NORMAL
    // Generate a random array with normal distribution
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> d(500, 100);
    for (int i = 0; i < n; i++) {
        A[i] = d(gen);
    }
#else
#if UNIFORM
    // Generate a random array with uniform distribution
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> d(400, 600);
    for (int i = 0; i < n; i++) {
        A[i] = d(gen);
    }
#else
    // Generate the array with stand random numbers
    srandom(s);
    for (long long i = 0; i < n; i++) {
        *(A + i) = random();
    }
#endif
#endif
    assert(std::is_sorted(A, A + n) == false);
#if DEBUG
    std::cout << "Unsorted Array: " << std::endl;
    for (int i = 0; i < n; i++) {
        std::cout << A[i] << " ";
    }
    std::cout << std::endl;
#endif
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
    START_TIMER;
    // Quick sort the local array
    qsort(A + start, end - start + 1, sizeof(long), compare);
    END_TIMER;
    PRINT_TIME(myId, 1);
    BARRIER

    // Collect samples
#if RANDOM_SAMPLING
    // Collect samples at random intervals
    int startP = myId * p;
    for (int i = 0; i < p; i++) {
        samples[startP + i] = A[random() % (end - start + 1) + start];
    }
#else
    // Collect samples at n/(p*p) intervals
    int w = n / (p * p);
    int startP = myId * p;
    for (int i = 0; i < p; i++) {
        samples[startP + i] = A[i * w + start];
    }
#endif

    BARRIER
    START_TIMER;
    // Sort the samples
    MASTER
    {
        qsort(samples, p * p, sizeof(long), compare);
        // Select the pivots

        for (int i = 0; i < (p - 1); i++) {
            pivots[i] = samples[(i + 1) * p];
        }
    }
    END_TIMER;
    PRINT_TIME(myId, 2);
    BARRIER
    START_TIMER;
    // Use binary search to find the partition boundaries
    partitionStartEnd[startP].start = start;
    partitionStartEnd[startP + p - 1].end = end;
    for (int i = 0; i < p - 1; i++) {
        int index = startP + i;
        partitionStartEnd[index].end = std::lower_bound(A + partitionStartEnd[index].start, A + end + 1, pivots[i]) - A - 1;
        partitionStartEnd[index + 1].start = partitionStartEnd[index].end + 1;
    }
    END_TIMER;
    PRINT_TIME(myId, 3);
    BARRIER

    START_TIMER;
    // K-way merge
    std::vector<long> localSorted;
    std::vector<std::pair<long, int>> heap;
#if VALIDATE_SORT
    long long minIndex = LONG_MAX;
    long long maxIndex = 0;
#endif

    for (int i = 0; i < p; i++) {
        int currStart = i * p + myId;
        long long currStartIndex = partitionStartEnd[currStart].start;
        long long currEndIndex = partitionStartEnd[currStart].end;
#if VALIDATE_SORT
        minIndex = std::min(minIndex, currStartIndex);
        maxIndex = std::max(maxIndex, currEndIndex);
#endif
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

    END_TIMER;
    PRINT_TIME(myId, 4);

    BARRIER
    DEBUG_PRINT_SORTED(localSorted);
#if VALIDATE_SORT

    assert(std::is_sorted(localSorted.begin(), localSorted.end()) == true);
    MinMaxCount minMaxCount = { localSorted.front(), localSorted.back(), localSorted.size() };
    pthread_mutex_lock(&mutex);
    allMinMaxCount.push_back(minMaxCount);
    pthread_mutex_unlock(&mutex);
    BARRIER
    MASTER
    {
        std::cout << "Validating sort" << std::endl;
        std::sort(allMinMaxCount.begin(), allMinMaxCount.end(), compareMinMaxCount);
        // Ensure that the sorted array in each thread is disjoint
        long long prevMax = allMinMaxCount[0].max;
        long long total = allMinMaxCount[0].count;
        for (size_t i = 1; i < allMinMaxCount.size(); i++) {
            assert(prevMax <= allMinMaxCount[i].min);
            prevMax = allMinMaxCount[i].max;
            total += allMinMaxCount[i].count;
        }
        assert(total == n); // Check if all elements are present
    }

#endif
    BARRIER
    return NULL;
}
