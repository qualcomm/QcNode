// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CL2D_PIPELINE_LETTERBOX_CLH
#define QC_CL2D_PIPELINE_LETTERBOX_CLH

KernelCode(

        __kernel void LetterboxNV12ToRGB( __global const uchar *srcPtr, int srcOffset,
                                          __global uchar *dstPtr, int dstOffset, int inputHeight,
                                          int inputWidth, int resizeHeight, int resizeWidth,
                                          int inputStride0, int inputPlane0Size, int inputStride1,
                                          int outputStride, int roiX, int roiY, float inputRatio,
                                          float outputRatio, int paddingValue ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *ySrc = srcPtr + srcOffset;
            __global const uchar *uSrc = srcPtr + srcOffset + inputPlane0Size;
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 3 );
            uchar3 RGB;
            if ( inputRatio < outputRatio )
            {
                if ( y < ( resizeWidth * inputRatio ) )
                {
                    int xIn = round( (float) x * native_recip( (float) resizeWidth ) *
                                     (float) inputWidth ) +
                              roiX;
                    int yIn = round( (float) y * native_recip( (float) resizeWidth ) *
                                     (float) inputWidth ) +
                              roiY;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
                    float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
                    float4 UV4 = ( float4 )( UV, UV );
                    UV4 = mad( UV4, coeffUV4, 0.5f );
                    UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
                    UV4 += Y;
                    RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
                    vstore3( RGB, 0, dst );
                }
                else
                {
                    RGB.s0 = ( paddingValue >> 16 ) & 0xFF;
                    RGB.s1 = ( paddingValue >> 8 ) & 0xFF;
                    RGB.s2 = (paddingValue) &0xFF;
                    vstore3( RGB, 0, dst );
                }
            }
            else
            {
                if ( x < ( resizeHeight * native_recip( inputRatio ) ) )
                {
                    int xIn = round( (float) x * native_recip( (float) resizeHeight ) *
                                     (float) inputHeight ) +
                              roiX;
                    int yIn = round( (float) y * native_recip( (float) resizeHeight ) *
                                     (float) inputHeight ) +
                              roiY;
                    int yPtr = mad24( yIn, inputStride0, xIn );
                    int uPtr = mad24( yIn / 2, inputStride1, ( xIn / 2 ) << 1 );
                    float Y = max( 0, ySrc[yPtr] - 16 ) * coeffY;
                    float2 UV = convert_float2( vload2( 0, uSrc + uPtr ) ) - 128.0f;
                    float4 UV4 = ( float4 )( UV, UV );
                    UV4 = mad( UV4, coeffUV4, 0.5f );
                    UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;
                    UV4 += Y;
                    RGB = convert_uchar3_sat( ( float3 )( UV4.s3, UV4.s1, UV4.s0 ) );
                    vstore3( RGB, 0, dst );
                }
                else
                {
                    RGB.s0 = ( paddingValue >> 16 ) & 0xFF;
                    RGB.s1 = ( paddingValue >> 8 ) & 0xFF;
                    RGB.s2 = (paddingValue) &0xFF;
                    vstore3( RGB, 0, dst );
                }
            }
        }

)

#endif   // QC_CL2D_PIPELINE_LETTERBOX_CLH
