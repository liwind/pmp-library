//=============================================================================
// Copyright (C) 2017, 2018 The pmp-library developers
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// =============================================================================

#include "gtest/gtest.h"

#include <pmp/algorithms/SurfaceSmoothing.h>

using namespace pmp;

class SurfaceSmoothingTest : public ::testing::Test
{
public:
    SurfaceSmoothingTest()
    {
        EXPECT_TRUE(mesh.read("pmp-data/off/hemisphere.off"));
    }
    SurfaceMesh mesh;
};

TEST_F(SurfaceSmoothingTest, implicitSmoothing)
{
    auto bbz = mesh.bounds().max()[2];
    SurfaceSmoothing ss(mesh);
    ss.implicitSmoothing(0.01);
    ss.implicitSmoothing(0.01,true);
    auto bbs = mesh.bounds().max()[2];
    EXPECT_LT(bbs,bbz);
}

TEST_F(SurfaceSmoothingTest, explicitSmoothing)
{
    auto bbz = mesh.bounds().max()[2];
    SurfaceSmoothing ss(mesh);
    ss.explicitSmoothing(10);
    ss.explicitSmoothing(10,true);
    auto bbs = mesh.bounds().max()[2];
    EXPECT_LT(bbs,bbz);
}