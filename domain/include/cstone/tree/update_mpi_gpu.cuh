/*
 * MIT License
 *
 * Copyright (c) 2021 CSCS, ETH Zurich
 *               2021 University of Basel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*! @file
 * @brief  MPI extension for calculating distributed cornerstone octrees
 *
 * @author Sebastian Keller <sebastian.f.keller@gmail.com>
 */

#pragma once

#include <mpi.h>

#include "cstone/primitives/mpi_wrappers.hpp"
#include "cstone/tree/octree_gpu.h"
#include "cstone/tree/octree_internal.hpp"

namespace cstone
{

/*! @brief global update step of an octree, including regeneration of the internal node structure
 *
 * @tparam        KeyType     unsigned 32- or 64-bit integer
 * @param[in]     keyStart    first particle key, on device
 * @param[in]     keyEnd      last particle key, on device
 * @param[in]     bucketSize  max number of particles per leaf
 * @param[inout]  tree        a fully linked octree
 * @param[inout]  counts      leaf node particle counts
 * @param[in]     numRanks    number of MPI ranks
 * @return                    true if tree was not changed
 */
template<class KeyType, class DevKeyVec, class DevCountVec>
bool updateOctreeGlobalGpu(const KeyType* keyStart,
                           const KeyType* keyEnd,
                           unsigned bucketSize,
                           Octree<KeyType>& tree,
                           DevKeyVec& d_csTree,
                           std::vector<unsigned>& counts,
                           DevCountVec& d_counts,
                           int numRanks)
{
    // to prevent 32-bit overflow we limit the maximum count to 2^32-1, divided by numRanks due to MPI_Allreduce
    unsigned maxCount = std::numeric_limits<unsigned>::max() / numRanks;
    bool converged    = tree.rebalance(bucketSize, counts);

    counts.resize(tree.numLeafNodes());
    reallocateDevice(d_csTree, tree.numLeafNodes() + 1, 1.01);
    reallocateDevice(d_counts, tree.numLeafNodes(), 1.01);

    thrust::copy_n(tree.treeLeaves().data(), d_csTree.size(), d_csTree.begin());
    computeNodeCountsGpu(rawPtr(d_csTree), rawPtr(d_counts), tree.numLeafNodes(), keyStart, keyEnd, maxCount, true);
    thrust::copy(d_counts.begin(), d_counts.end(), counts.begin());

    MPI_Allreduce(MPI_IN_PLACE, counts.data(), counts.size(), MPI_UNSIGNED, MPI_SUM, MPI_COMM_WORLD);

    return converged;
}

} // namespace cstone
