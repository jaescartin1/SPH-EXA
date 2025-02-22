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
 * @brief Unit tests for ParticlesData
 *
 * @author Sebastian Keller <sebastian.f.keller@gmail.com>
 */

#include <iostream>

#include "gtest/gtest.h"

#include "cstone/fields/particles_get.hpp"
#include "sph/particles_data.hpp"

using namespace sphexa;

TEST(ParticlesData, resize)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    d.setConserved("x", "y", "z");
    d.setDependent("du");

    size_t size = 10;
    d.resize(10);

    EXPECT_EQ(d.x.size(), size);
    EXPECT_EQ(d.y.size(), size);
    EXPECT_EQ(d.z.size(), size);
    EXPECT_EQ(d.du.size(), size);
}

TEST(ParticlesData, releaseAcquire)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    d.setConserved("x", "y", "z");
    d.setDependent("du", "ax", "p");

    size_t size = 10;
    d.resize(size);

    d.release("ax");
    d.acquire("c");

    EXPECT_EQ(d.ax.size(), 0);
    EXPECT_EQ(d.c.size(), size);
}

TEST(ParticlesData, releaseAcquireInt)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    d.setDependent("x");

    int indexOfX = std::find(d.fieldNames.begin(), d.fieldNames.end(), "x") - d.fieldNames.begin();

    size_t size = 10;
    d.resize(size);

    EXPECT_EQ(d.x.size(), size);
    EXPECT_EQ(d.y.size(), 0);

    d.release(indexOfX);
    d.acquire("y");

    EXPECT_EQ(d.x.size(), 0);
    EXPECT_EQ(d.y.size(), size);
}

TEST(ParticlesData, releaseAcquireMultiple)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    d.setDependent("ax", "ay", "p");

    size_t size = 10;
    d.resize(size);

    EXPECT_EQ(d.ax.size(), size);
    EXPECT_EQ(d.ay.size(), size);
    EXPECT_EQ(d.p.size(), size);

    d.release("ax", "ay", "p");
    d.acquire("c11", "c12", "c13");

    EXPECT_EQ(d.ax.size(), 0);
    EXPECT_EQ(d.ay.size(), 0);
    EXPECT_EQ(d.p.size(), 0);
    EXPECT_EQ(d.c11.size(), size);
    EXPECT_EQ(d.c12.size(), size);
    EXPECT_EQ(d.c13.size(), size);
}

TEST(ParticlesData, aquireThrow)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    d.setConserved("x", "y", "z");
    d.setDependent("du", "ax", "p");

    size_t size = 10;
    d.resize(size);

    // cannot acquire "c": no released field available
    EXPECT_ANY_THROW(d.acquire("c"));
}

TEST(ParticlesData, conservedThrow)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    d.setConserved("x", "y", "z");

    // cannot release "x" which is conserved
    EXPECT_ANY_THROW(d.release("x"));
}

TEST(ParticlesData, typeMismatch)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    d.setDependent("ax");
    d.resize(10);

    d.release("ax");

    // cannot acquire "nc" from released "x": types do not match
    EXPECT_ANY_THROW(d.acquire("nc"));
}

TEST(ParticlesData, get)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    constexpr std::array conservedFields{"x", "y", "rho"};
    std::apply([&d](auto&... f) { d.setConserved(f...); }, conservedFields);

    d.resize(1);
    d.rho[0] = 1;

    auto acc = get<"x", "y", "rho">(d);

    std::get<2>(acc)[0] = 2;
    EXPECT_EQ(d.rho[0], 2);

    auto acc2 = get<"x", "y", "rho">(d);
    EXPECT_EQ(std::get<2>(acc2)[0], 2);
}

TEST(ParticlesData, getFieldList)
{
    ParticlesData<double, unsigned, cstone::CpuTag> d;

    constexpr std::array conservedFields{"x", "y", "rho"};
    std::apply([&d](auto&... f) { d.setConserved(f...); }, conservedFields);

    d.resize(1);
    d.rho[0] = 1;

    using Fields = cstone::FieldList<"x", "y", "rho">;

    auto acc = get<Fields>(d);

    std::get<2>(acc)[0] = 2;
    EXPECT_EQ(d.rho[0], 2);

    auto acc2 = get<Fields>(d);
    EXPECT_EQ(std::get<2>(acc2)[0], 2);

    constexpr auto tup = make_tuple(Fields{});
    EXPECT_EQ(d.x.data(), get<std::get<0>(tup)>(d).data());
    auto& xRef = get<"x">(d);
    EXPECT_EQ(d.x.data(), xRef.data());
}
