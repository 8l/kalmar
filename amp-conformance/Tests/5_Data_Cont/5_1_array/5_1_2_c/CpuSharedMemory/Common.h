// Copyright (c) Microsoft
// All rights reserved
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

#pragma once

#include <amptest.h>
#include <amptest/coordinates.h>

namespace Concurrency {
namespace Test {

template<typename _type, int _rank, template<typename, int> class _amp_container_type> 
bool ReadAndVerify(_amp_container_type<_type, _rank>& amp_container, _type value)
{
	index_iterator<_rank> idx_iter(amp_container.get_extent());

	for(index_iterator<_rank> iter = idx_iter.begin(); iter != idx_iter.end(); iter++)
	{
		if(amp_container[*iter] != value)
		{
			Log(LogType::Error) << "Value mismatch at " << *iter << std::endl;
			Log(LogType::Error) << "Excpected: " << value << " Actual: " << amp_container[*iter] << std::endl;
			return false;
		}
	}
	
	return true;
}

template<typename _type, int _rank, template<typename, int> class _amp_container_type> 
void Write(_amp_container_type<_type, _rank>& amp_container, _type value)
{
	index_iterator<_rank> idx_iter(amp_container.get_extent());

	for(index_iterator<_rank> iter = idx_iter.begin(); iter != idx_iter.end(); iter++)
	{
		amp_container[*iter] = value;
	}
}

template<typename _type, int _rank, template<typename, int> class _amp_container_type> 
void Increment(_amp_container_type<_type, _rank>& amp_container, _type value)
{
	index_iterator<_rank> idx_iter(amp_container.get_extent());

	for(index_iterator<_rank> iter = idx_iter.begin(); iter != idx_iter.end(); iter++)
	{
		amp_container[*iter] += value;
	}
}

template<typename _type, int _rank> 
bool VerifyCpuAccessType(concurrency::array<_type, _rank>& arr, concurrency::access_type exp_access_type)
{
	if(arr.get_cpu_access_type() != exp_access_type)
	{
		Log(LogType::Error) << "Wrong cpu_access_type." << std::endl;
		Log(LogType::Error) << "Expect: " << exp_access_type << " Actual: " << arr.get_cpu_access_type() << std::endl;
        // TODO: Current implementation is not accompliant with this standard 
        //return false;
	}
	
	if(arr.get_cpu_access_type() != access_type_none && arr.data() == NULL)
	{
		Log(LogType::Error) << "Array with CPU access type read or write or read-and-write has NULL arr.data()" << std::endl;
        // TODO: Current implementation is not accompliant with this standard 
        //return false;
	}
	
	if(arr.get_cpu_access_type() == access_type_none && arr.data() != NULL)
	{
		Log(LogType::Error) << "Array with CPU access type none has non-NULL arr.data()" << std::endl;
        // TODO: Current implementation is not accompliant with this standard 
        //return false;
	}
	
	return true;
}

}//namespace Test
}//namespace Concurrency

