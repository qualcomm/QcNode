// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CL2D_PIPELINE_CONVERTUBWC_CLH
#define QC_CL2D_PIPELINE_CONVERTUBWC_CLH

KernelCode(

        __kernel void ConvertUBWC( __read_only image2d_t srcYPlane,
                                   __read_only image2d_t srcUVPlane, sampler_t sampler,
                                   __global uchar *dstPtr, int dstOffset, int outputStride0,
                                   int outputPlane0Size, int outputStride1, int roiX, int roiY ) {
            const int x = get_global_id( 0 );
            const int y = get_global_id( 1 );
            const int2 coord = ( int2 )( x + roiX, y + roiY );
            const float4 pixelY = read_imagef( srcYPlane, sampler, coord );
            const float4 pixelUV = read_imagef( srcUVPlane, sampler, coord );

            __global uchar *dst1 = dstPtr + dstOffset + mad24( y, outputStride0, x );
            __global uchar *dst2 = dstPtr + dstOffset + outputPlane0Size +
                                   mad24( y / 2, outputStride1, ( x / 2 ) << 1 );
            dst1[0] = convert_uchar_sat( pixelY.s0 * 255 );
            dst2[0] = convert_uchar_sat( pixelUV.s0 * 255 );
            dst2[1] = convert_uchar_sat( pixelUV.s1 * 255 );
        }

)

#endif   // QC_CL2D_PIPELINE_CONVERTUBWC_CLH
