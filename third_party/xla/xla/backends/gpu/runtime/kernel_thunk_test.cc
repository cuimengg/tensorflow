/* Copyright 2025 The OpenXLA Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "xla/backends/gpu/runtime/kernel_thunk.h"

#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include "xla/backends/gpu/runtime/thunk.h"
#include "xla/service/buffer_assignment.h"
#include "xla/service/gpu/kernel_arguments.h"
#include "xla/service/gpu/launch_dimensions.h"
#include "xla/shape_util.h"
#include "xla/stream_executor/launch_dim.h"
#include "xla/tsl/platform/test.h"

namespace xla::gpu {
namespace {

using Kind = Thunk::Kind;

TEST(KernelThunkTest, CreateAndGettersAndToString) {
  Thunk::ThunkInfo thunk_info;
  thunk_info.profile_annotation = "DotGeneral";
  thunk_info.execution_stream_id = 123;

  BufferAllocation alloc0(/*index=*/0, /*size=*/1024, /*color=*/0);
  BufferAllocation::Slice slice0(&alloc0, /*offset=*/0, /*size=*/1024);

  BufferAllocation alloc1(/*index=*/0, /*size=*/256, /*color=*/0);
  BufferAllocation::Slice slice1(&alloc1, /*offset=*/0, /*size=*/256);

  std::vector<KernelArgument> kernel_arguments = {
      KernelArgument(ShapeUtil::MakeShape(F32, {1024}), slice0,
                     /*written=*/false),
      KernelArgument(ShapeUtil::MakeShape(F32, {256}), slice1,
                     /*written=*/true)};

  LaunchDimensions launch_dimensions(se::BlockDim(32, 1, 1),
                                     se::ThreadDim(256, 1, 1));

  KernelThunk thunk(thunk_info,
                    /*kernel_name=*/"kernel123",
                    /*kernel_arguments=*/kernel_arguments,
                    /*launch_dimensions=*/launch_dimensions,
                    /*cluster_dim=*/se::ClusterDim(8, 1, 1),
                    /*shmem_bytes=*/1024,
                    /*tma_metadata=*/std::nullopt);
  EXPECT_EQ(thunk.kind(), Kind::kKernel);
  EXPECT_EQ(thunk.kernel_name(), "kernel123");
  EXPECT_EQ(thunk.arguments(),
            std::vector<BufferAllocation::Slice>({slice0, slice1}));
  EXPECT_EQ(thunk.written(), std::vector<bool>({false, true}));
  EXPECT_EQ(thunk.launch_dimensions().block_counts(), se::BlockDim(32, 1, 1));
  EXPECT_EQ(thunk.launch_dimensions().thread_counts_per_block(),
            se::ThreadDim(256, 1, 1));
  EXPECT_EQ(thunk.cluster_dim(), se::ClusterDim(8, 1, 1));
  EXPECT_EQ(thunk.shmem_bytes(), 1024);
  EXPECT_EQ(thunk.ToString(0),
            ", kernel = kernel123, launch dimensions = blocks: {32, 1, 1}, "
            "threads/block: {256, 1, 1}, cluster_dim = ClusterDim{8, 1, 1}");
}

}  // namespace
}  // namespace xla::gpu
