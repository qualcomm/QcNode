// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_CL2D_PIPELINE_CONVERT_CLH
#define QC_CL2D_PIPELINE_CONVERT_CLH

KernelCode(

        __kernel void ConvertNV12ToRGB( __global const uchar *srcPtr, int srcOffset,
                                        __global uchar *dstPtr, int dstOffset, int inputStride0,
                                        int inputPlane0Size, int inputStride1, int outputStride,
                                        int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );

            int yOffset =
                    srcOffset + mad24( ( y + roiY ) << 1, inputStride0, ( ( x + roiX ) << 1 ) );
            int uOffset = srcOffset + inputPlane0Size +
                          mad24( ( y + roiY ), inputStride1, ( ( x + roiX ) << 1 ) );
            int dstOffset1 = dstOffset + mad24( y << 1, outputStride, x * 6 );
            int dstOffset2 = dstOffset1 + outputStride;

            uchar4 dst1Val4, dst2Val4;
            uchar2 dst1Val2, dst2Val2;

            float2 Y12 = convert_float2( vload2( 0, srcPtr + yOffset ) ) - 16.0f;
            float2 Y34 = convert_float2( vload2( 0, srcPtr + yOffset + inputStride0 ) ) - 16.0f;
            Y12 = max( 0, Y12 ) * coeffY;
            Y34 = max( 0, Y34 ) * coeffY;

            float2 UV = convert_float2( vload2( 0, srcPtr + uOffset ) ) - 128.0f;
            float4 UV4 = ( float4 )( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;

            float4 Y1UV = UV4 + Y12.s0;
            float4 Y2UV = UV4 + Y12.s1;
            float4 Y3UV = UV4 + Y34.s0;
            float4 Y4UV = UV4 + Y34.s1;

            dst1Val4 = convert_uchar4_sat( ( float4 )( Y1UV.s3, Y1UV.s1, Y1UV.s0, Y2UV.s3 ) );
            dst1Val2 = convert_uchar2_sat( ( float2 )( Y2UV.s1, Y2UV.s0 ) );
            dst2Val4 = convert_uchar4_sat( ( float4 )( Y3UV.s3, Y3UV.s1, Y3UV.s0, Y4UV.s3 ) );
            dst2Val2 = convert_uchar2_sat( ( float2 )( Y4UV.s1, Y4UV.s0 ) );

            vstore4( dst1Val4, 0, dstPtr + dstOffset1 );
            vstore2( dst1Val2, 2, dstPtr + dstOffset1 );
            vstore4( dst2Val4, 0, dstPtr + dstOffset2 );
            vstore2( dst2Val2, 2, dstPtr + dstOffset2 );
        }

        __kernel void ConvertUYVYToRGB( __global const uchar *srcPtr, int srcOffset,
                                        __global uchar *dstPtr, int dstOffset, int inputStride,
                                        int outputStride, int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *uSrc =
                    srcPtr + srcOffset + mad24( ( y + roiY ), inputStride, ( x + roiX ) * 4 );
            __global uchar *dst = dstPtr + dstOffset + mad24( y, outputStride, x * 6 );

            float4 UYVY = convert_float4( vload4( 0, uSrc ) );
            float2 UV = ( float2 )( UYVY.s0, UYVY.s2 ) - 128.0f;
            float2 Y1Y2 = ( float2 )( UYVY.s1, UYVY.s3 ) - 16.0f;
            Y1Y2 = max( 0, Y1Y2 ) * coeffY;
            float4 Y1UV = ( float4 )( Y1Y2.s0 );
            float4 Y2UV = ( float4 )( Y1Y2.s1 );
            float4 UV4 = ( float4 )( UV, UV );
            UV4 = mad( UV4, coeffUV4, 0.5f );
            UV4.s1 = UV4.s1 + UV4.s2 - 0.5f;

            Y1UV += UV4;
            Y2UV += UV4;
            uchar4 udst1 = convert_uchar4_sat( ( float4 )( Y1UV.s3, Y1UV.s1, Y1UV.s0, Y2UV.s3 ) );
            uchar2 udst2 = convert_uchar2_sat( ( float2 )( Y2UV.s1, Y2UV.s0 ) );
            vstore4( udst1, 0, dst );
            vstore2( udst2, 2, dst );
        }

        __kernel void ConvertUYVYToNV12( __global const uchar *srcPtr, int srcOffset,
                                         __global uchar *dstPtr, int dstOffset, int inputStride,
                                         int outputStride0, int outputPlane0Size, int outputStride1,
                                         int roiX, int roiY ) {
            int x = get_global_id( 0 );
            int y = get_global_id( 1 );
            __global const uchar *src =
                    srcPtr + srcOffset + mad24( ( y + roiY ) << 1, inputStride, ( x + roiX ) * 4 );
            __global uchar *dst1 = dstPtr + dstOffset + mad24( y << 1, outputStride0, x << 1 );
            __global uchar *dst2 =
                    dstPtr + dstOffset + outputPlane0Size + mad24( y, outputStride1, x << 1 );
            float U1 = src[0];
            float V1 = src[2];
            float U2 = src[0 + inputStride];
            float V2 = src[2 + inputStride];
            dst1[0] = src[1];
            dst1[1] = src[3];
            dst1[0 + outputStride0] = src[1 + inputStride];
            dst1[1 + outputStride0] = src[3 + inputStride];
            dst2[0] = convert_uchar_sat( ( U1 + U2 ) / 2.0f );
            dst2[1] = convert_uchar_sat( ( V1 + V2 ) / 2.0f );
        }

)

#endif   // QC_CL2D_PIPELINE_CONVERT_CLH
