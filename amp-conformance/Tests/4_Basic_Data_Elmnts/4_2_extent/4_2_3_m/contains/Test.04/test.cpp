// Copyright (c) Microsoft
// All rights reserved
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
// INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.
/// <tags>P1</tags>
/// <summary>Test that contains return false for indices with greater than maximum value in one or more dimensions.</summary>

#define NOMINMAX
#include <amptest.h>
#include <amptest_main.h>
#include <amptest/coordinates.h>
#include <limits>

using namespace concurrency;
using namespace concurrency::Test;

runall_result test(int int_max) restrict(cpu,amp)
{
	// Tests for extent rank 1
	{
		extent<1> ext(10);
		if(!(  !ext.contains(index<1>(16))
			&& !ext.contains(index<1>(int_max))
			))
			return runall_fail;
	}

	// Tests for extent rank 2
	{
		extent<2> ext(11, 8);
		if(!(  !ext.contains(index<2>(23, 8))
			&& !ext.contains(index<2>(11, int_max))
			&& !ext.contains(index<2>(0, 10))
			&& !ext.contains(index<2>(int_max, int_max))
			))
			return runall_fail;
	}

	// Tests for extent rank 3
	{
		extent<3> ext(2, 2, 2);
		if(!(  !ext.contains(index<3>(5, 0, 2))
			&& !ext.contains(index<3>(0, 2, 6))
			&& !ext.contains(index<3>(2, 7, 0))
			&& !ext.contains(index<3>(1, int_max, 1))
			&& !ext.contains(index<3>(4, 0, -4))
			&& !ext.contains(index<3>(int_max, int_max,	int_max))
			))
			return runall_fail;
	}

	// Tests for extent rank N = 5
	{
		extent<5> ext = make_extent(1, 2, 3, 4, 5);
		if(!(  !ext.contains(make_index(0, 99, 0, 0, 0))
			&& !ext.contains(make_index(0, 0, 2, 99, 0))
			&& !ext.contains(make_index(0, 0, 99, 0, 2))
			&& !ext.contains(make_index(99, 1, 1, 1, 1))
			&& !ext.contains(make_index(0, 1, 2, 3, 99))
			&& !ext.contains(make_index(4, 1, 2, 1, 0))
			&& !ext.contains(make_index(int_max, int_max, int_max, int_max, int_max))
			))
			return runall_fail;
	}

    return runall_pass;
}

runall_result test_main()
{
	accelerator_view av = require_device().get_default_view();

	int int_max = std::numeric_limits<int>::max();

	runall_result cpu_result = test(int_max);
	Log() << "Test " << cpu_result << " on host\n";

	runall_result amp_result = GPU_INVOKE(av, runall_result, test, int_max);
	Log() << "Test " << amp_result << " on device\n";

	return cpu_result & amp_result;
}
