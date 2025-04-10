// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_CL2DCONSTANT_CLH
#define RIDEHAL_CL2DCONSTANT_CLH

KernelCode(

        __constant float coeffs[5] = { 1.163999557f, 2.017999649f, -0.390999794f, -0.812999725f,
                                       1.5959997177f };
        __constant float coeffY = 1.163999557f;
        __constant float4 coeffUV4 = ( float4 )( 2.017999649f, -0.812999725f, -0.390999794f,
                                                 1.5959997177f );

)

#endif   // RIDEHAL_CONSTANT_CLH
