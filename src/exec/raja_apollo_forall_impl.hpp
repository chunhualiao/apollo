
// Copyright (c) 2019, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
//
// This file is part of Apollo.
// OCEC-17-092
// All rights reserved.
//
// Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
// of many collaborators.
//
// Apollo was originally created by David Beckingsale, david@llnl.gov
//
// For details, see https://github.com/LLNL/apollo.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing RAJA index set and segment iteration
 *          template methods for Apollo-guided execution.
 *
 *          These methods should work on any platform.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-18, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-689114
//
// All rights reserved.
//
// This file is part of RAJA.
//
// For details about use and distribution, please read RAJA/LICENSE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_forall_apollo_HPP
#define RAJA_forall_apollo_HPP

#include <string>
#include <sstream>
#include <functional>
#include <unordered_set>

#include "RAJA/config.hpp"
#include "RAJA/util/types.hpp"
#include "RAJA/policy/apollo/policy.hpp"
#include "RAJA/internal/fault_tolerance.hpp"

#include "apollo/Apollo.h"
#include "apollo/Region.h"

namespace RAJA
{
namespace policy
{
namespace apollo
{

    //printf("region->name == \"%s\"  apolloExecCount == %d\n",
    //    apolloRegion->name,
    //    apolloExecCount);

    //switch(policyIndex) {
    //    case 0:
    //    case 1: printf("apolloPolicySeq{}\n"); break;
    //    case 2: printf("apolloPolicySIMD{}\n"); break;
    //    case 3: printf("apolloPolicyLoopExec{}\n"); break;
    //    case 4: printf("apolloPolicyOpenMP{}\n"); break;
    //    case 5: printf("apolloPolicyCUDA{}\n"); break;
    //    default:
    //            printf("UNKNOWN POLICY ---\n"); break;
    //}
    //fflush(stdout);


using apolloPolicySeq      = RAJA::seq_exec;
using apolloPolicySIMD     = RAJA::simd_exec;
using apolloPolicyLoopExec = RAJA::loop_exec;
#if defined(RAJA_ENABLE_OPENMP)
  using apolloPolicyOpenMP   = RAJA::omp_parallel_for_exec;
  #if defined(RAJA_ENABLE_CUDA)
    using apolloPolicyCUDA     = RAJA::cuda_exec<CUDA_BLOCK_SIZE>;
    const int POLICY_COUNT = 5;
  #else
    const int POLICY_COUNT = 4;
  #endif
#else
  #if defined(RAJA_ENABLE_CUDA)
    using apolloPolicyCUDA     = RAJA::cuda_exec<CUDA_BLOCK_SIZE>;
    const int POLICY_COUNT = 4;
  #else
    const int POLICY_COUNT = 3;
  #endif
#endif



//
//////////////////////////////////////////////////////////////////////
//
// The following function template switches between various RAJA
// execution policies based on feedback from the Apollo system.
//
//////////////////////////////////////////////////////////////////////
//

template <typename BODY>
RAJA_INLINE void apolloPolicySwitcher(int choice, BODY body) {
    switch (choice) {
    case 1: body(apolloPolicySeq{});      break;
    case 2: body(apolloPolicySIMD{});     break;
    case 3: body(apolloPolicyLoopExec{}); break;
#if defined(RAJA_ENABLE_OPENMP)
    case 4: body(apolloPolicyOpenMP{});   break;
    #if defined(RAJA_ENABLE_CUDA)
    case 5: body(apolloPolicyCUDA{});     break;
    #endif
#else
    #if defined(RAJA_ENABLE_CUDA)
    case 4: body(apolloPolicyCUDA{});     break;
    #endif
#endif
    case 0:
    default: body(apolloPolicySeq{});     break;
    }
}

template <typename Iterable, typename Func>
RAJA_INLINE void forall_impl(const apollo_exec &, Iterable &&iter, Func &&body)
{
    static Apollo::Region *apolloRegion      = nullptr;
    static int             apolloExecCount   = 0;
    static int             policyIndex       = 0;
    if (apolloRegion == nullptr) {
        // ----------
        // NOTE: This section runs *once* the first time the
        //       region is encountered
        std::stringstream ss_location;
        ss_location << (const void *) &body;
        apolloRegion = new Apollo::Region(
            Apollo::instance(),
            ss_location.str().c_str(),
            RAJA::policy::apollo::POLICY_COUNT);
        // ----------
    }


    apolloRegion->begin(apolloExecCount++);

    policyIndex = apolloRegion->getPolicyIndex();

    apolloPolicySwitcher(policyIndex , [=] (auto pol) {
        forall_impl(pol, iter, body); });

    apolloRegion->end();

}

//////////
}  // closing brace for apollo namespace
}  // closing brace for policy namespace
}  // closing brace for RAJA namespace

#endif  // closing endif for header file include guard
