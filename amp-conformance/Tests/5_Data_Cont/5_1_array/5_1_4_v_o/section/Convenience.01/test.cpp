// Copyright (c) Microsoft
// All rights reserved
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED,
// INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.
/// <tags>P1</tags>
/// <summary>Create a section of a rank 1 array using the convenience APIs on CPU</summary>

#include <amptest/array_test.h>
#include <amptest.h>
#include <amptest_main.h>

using namespace Concurrency;
using namespace Concurrency::Test;

runall_result scalar_values()
{
	ArrayTest<int, 1> original(extent<1>(10));
    ArrayViewTest<int, 1> section = original.section(original.arr().section(2, 5), index<1>(2));
    
    original.set_value(index<1>(2), 13);
    
    section.view()(3) = 15;
    section.set_known_value(index<1>(3), 15);
    
    return (gpu_read(original.arr(),index<1>(5)) == 15 && section.view()(0) == 13) ? original.pass() : original.fail();
}

runall_result only_index()
{
	ArrayTest<int, 1> original(extent<1>(10));
    ArrayViewTest<int, 1> section = original.section(original.arr().section(index<1>(2)), index<1>(2));
    
    original.set_value(index<1>(2), 13);
    
    section.view()(3) = 15;
    section.set_known_value(index<1>(3), 15);
    
    return (gpu_read(original.arr(),index<1>(5)) == 15 && section.view()(0) == 13) ? original.pass() : original.fail();
}

runall_result only_extent()
{
	ArrayTest<int, 1> original(extent<1>(10));
    ArrayViewTest<int, 1> section = original.section(original.arr().section(extent<1>(5)), index<1>(0));
    
    original.set_value(index<1>(2), 13);
    
    section.view()(3) = 15;
    section.set_known_value(index<1>(3), 15);
    
    return (gpu_read(original.arr(),index<1>(3)) == 15 && section.view()(2) == 13) ? original.pass() : original.fail();
}

runall_result test_main()
{
    runall_result res;
	
	res &= REPORT_RESULT(scalar_values());
	res &= REPORT_RESULT(only_index());
	res &= REPORT_RESULT(only_extent());
	
	return res;
}

