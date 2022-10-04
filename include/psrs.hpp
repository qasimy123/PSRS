/**
 * @file psrs.h
 * @author Qasim Khawaja (khawaja@ualberta.ca)
 * @brief Header file for the PSRS algorithm.
 * @version 0.1
 * @date 2022-09-11
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <algorithm>
#include <iostream>
#include <vector>
// #include <mpi.h>
#include <chrono>
#include <random>

#define vi std::vector<int>

/**
 * Class for the PSRS object.
 *
 */
class PSRS {
public:
    /**
     * Constructor for the PSRS object
     *
     * @param n The number of elements in the array.
     * @param p The number of processors.
     * @param seed The seed for the random number generator.
     */
    PSRS(int n, int p, int seed);

    /**
     * @brief Perform the PSRS algorithm using p processors.
     *
     * @return std::vector<int> The sorted array.
     */
    vi sort();

private:
    int n; // The number of elements in the array.
    int p; // The number of processors.
    int seed; // The seed for the random number generator.
    vi array; // The array to sort.

    /**
     * @brief Generate a list of size n using the seed.
     *
     */
    vi generateArray();

    /**
     * @brief Generate sublist of size n/p from the array.
     * 
     * @param processor The processor number.
     */
    vi generateSublist(int processor);

    /**
     * @brief Sort the local array.
     *
     * @param start The start index of the local array.
     * @param end The end index of the local array.
     */
    void sortLocal(int start, int end);

    /**
     * @brief Find the p-1 pivots from the sample.
     *
     */
    void findPivots();

    /**
     * @brief Partition the local array into p-1 buckets and stores the partitions in the partitions array.
     *
     * @param start The start index of the local array.
     * @param end The end index of the local array.
     */
    void partitionLocal(int start, int end);

    /**
     * @brief Exchange the partitions between the processors.
     */
    void exchangePartitions();

    /**
     * @brief Merge the partitions into the global array.
     */
    void mergePartitions();
};