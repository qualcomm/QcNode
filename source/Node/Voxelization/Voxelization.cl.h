// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_VOXELIZATION_CLH
#define QC_VOXELIZATION_CLH

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceVoxelization = KernelCode(

        __constant float2 ZEROVEC2 = (float2) ( 0.0f );
        __constant float4 ZEROVEC4 = (float4) ( 0.0f );
        __constant float8 ZEROVEC8 = (float8) ( 0.0f );

        __kernel void ClusterPointsFromXYZR(
                __global const float *pInPts, __global float *pOutPlrs, __global float *pOutFeature,
                __global int *coorToPlrIdx, __global int *numOfPts, const float minXRange,
                const float minYRange, const float minZRange, const float maxXRange,
                const float maxYRange, const float maxZRange, const float pillarXSize,
                const float pillarYSize, const float pillarZSize, const int gridXSize,
                const int gridYSize, const int maxNumPlrs, const int maxNumPtsPerPlr,
                const int numOutFeatureDim ) {
            int x = get_global_id( 0 );

            float4 xyzr = vload4( x, pInPts );
            float2 xyMinRange = (float2) ( minXRange, minYRange );
            int3 point = (int3) ( 0 );
            float3 plrs = (float3) ( 0.0f );
            if ( ( xyzr.s0 > minXRange ) && ( xyzr.s0 < maxXRange ) && ( xyzr.s1 > minYRange ) &&
                 ( xyzr.s1 < maxYRange ) && ( xyzr.s2 > minZRange ) && ( xyzr.s2 < maxZRange ) )
            {
                plrs.s01 = floor( ( xyzr.s01 - xyMinRange ) *
                                  native_recip( (float2) ( pillarXSize, pillarYSize ) ) );
                point = convert_int3_sat( plrs );
                int id = point.s1 * gridXSize + point.s0;
                int plrIdx = 0;
                if ( atomic_load( (atomic_int *) &numOfPts[maxNumPlrs] ) < maxNumPlrs )
                {
                    if ( atomic_cmpxchg( &coorToPlrIdx[id], -1, 0 ) == -1 )
                    {
                        coorToPlrIdx[id] = atomic_inc( &numOfPts[maxNumPlrs] );
                        plrIdx = coorToPlrIdx[id];
                        if ( plrIdx < maxNumPlrs )
                        {
                            vstore3( plrs, 0, pOutPlrs + plrIdx * 4 );
                        }
                    }
                    else
                    {
                        plrIdx = coorToPlrIdx[id];
                    }

                    if ( plrIdx < maxNumPlrs )
                    {
                        if ( atomic_load( (atomic_int *) &numOfPts[plrIdx] ) < maxNumPtsPerPlr )
                        {
                            int numPts = atomic_inc( &numOfPts[plrIdx] );
                            pOutPlrs[plrIdx * 4 + 3] = numPts + 1;
                            if ( numPts < maxNumPtsPerPlr )
                            {
                                int featureID = plrIdx * maxNumPtsPerPlr * numOutFeatureDim +
                                                numPts * numOutFeatureDim;
                                vstore4( xyzr, 0, pOutFeature + featureID );
                            }
                        }
                    }
                }
            }
        }

        __kernel void ClusterPointsFromXYZRT(
                __global const float *pInPts, __global int *pOutPlrs, __global float *pOutFeature,
                __global int *coorToPlrIdx, __global int *numOfPts, const float minXRange,
                const float minYRange, const float minZRange, const float maxXRange,
                const float maxYRange, const float maxZRange, const float pillarXSize,
                const float pillarYSize, const float pillarZSize, const int gridXSize,
                const int gridYSize, const int maxNumPlrs, const int maxNumPtsPerPlr,
                const int numOutFeatureDim ) {
            int x = get_global_id( 0 );

            float4 xyzr = vload4( 0, pInPts + x * 5 );
            float t = pInPts[x * 5 + 4];
            float2 xyMinRange = (float2) ( minXRange, minYRange );
            int2 xyCoor = (int2) ( 0 );
            if ( ( xyzr.s0 > minXRange ) && ( xyzr.s0 < maxXRange ) && ( xyzr.s1 > minYRange ) &&
                 ( xyzr.s1 < maxYRange ) && ( xyzr.s2 > minZRange ) && ( xyzr.s2 < maxZRange ) )
            {
                xyCoor = convert_int2_sat(
                        floor( ( xyzr.s01 - xyMinRange ) *
                               native_recip( (float2) ( pillarXSize, pillarYSize ) ) ) );
                int id = xyCoor.s1 * gridXSize + xyCoor.s0;
                int plrIdx = 0;
                if ( atomic_load( (atomic_int *) &numOfPts[maxNumPlrs] ) < maxNumPlrs )
                {
                    if ( atomic_cmpxchg( &coorToPlrIdx[id], -1, 0 ) == -1 )
                    {
                        coorToPlrIdx[id] = atomic_inc( &numOfPts[maxNumPlrs] );
                        plrIdx = coorToPlrIdx[id];
                        if ( plrIdx < maxNumPlrs )
                        {
                            vstore2( xyCoor, plrIdx, pOutPlrs );
                        }
                    }
                    else
                    {
                        plrIdx = coorToPlrIdx[id];
                    }

                    if ( plrIdx < maxNumPlrs )
                    {
                        if ( atomic_load( (atomic_int *) &numOfPts[plrIdx] ) < maxNumPtsPerPlr )
                        {
                            int numPts = atomic_inc( &numOfPts[plrIdx] );
                            if ( numPts < maxNumPtsPerPlr )
                            {
                                int featureID = plrIdx * maxNumPtsPerPlr * numOutFeatureDim +
                                                numPts * numOutFeatureDim;
                                vstore4( xyzr, 0, pOutFeature + featureID );
                                pOutFeature[featureID + 4] = t;
                            }
                        }
                    }
                }
            }
        }

        __kernel void FeatureGatherFromXYZR(
                __global float *pOutPlrs, __global float *pOutFeature, __global int *numOfPts,
                const float minXRange, const float minYRange, const float minZRange,
                const float pillarXSize, const float pillarYSize, const float pillarZSize,
                const int maxNumPlrs, const int maxNumPtsPerPlr, const int numOutFeatureDim,
                const int numOfPillar ) {
            int x = get_global_id( 0 );

            if ( x < numOfPillar )
            {
                float4 outPlr4 = vload4( x, pOutPlrs );
                outPlr4.s3 = min( outPlr4.s3, (float) maxNumPtsPerPlr );
                int numPts = (int) outPlr4.s3;

                float3 meanXYZ = (float3) ( 0.0f );
                float3 featXYZ = (float3) ( 0.0f );
                float3 pillarXYZ = (float3) ( 0.0f );
                float3 pillarSizeXYZ = (float3) ( pillarXSize, pillarYSize, pillarZSize );
                float3 minRangeXYZ = (float3) ( minXRange, minYRange, minZRange );
                float3 outPlrXYZ = outPlr4.s012 + 0.5f;
                for ( int i = 0; i < numPts; i++ )
                {
                    int id1 = x * maxNumPtsPerPlr * numOutFeatureDim + i * numOutFeatureDim;
                    featXYZ = vload3( 0, pOutFeature + id1 );
                    meanXYZ += featXYZ;
                }
                meanXYZ *= native_recip( outPlr4.s3 );
                pillarXYZ = mad( pillarSizeXYZ, outPlrXYZ, minRangeXYZ );

                for ( int j = 0; j < maxNumPtsPerPlr; j++ )
                {
                    int id2 = x * maxNumPtsPerPlr * numOutFeatureDim + j * numOutFeatureDim;
                    if ( j < numPts )
                    {
                        featXYZ = vload3( 0, pOutFeature + id2 );
                        outPlrXYZ = featXYZ - meanXYZ;
                        vstore3( outPlrXYZ, 0, pOutFeature + id2 + 4 );
                        outPlrXYZ = featXYZ - pillarXYZ;
                        vstore3( outPlrXYZ, 0, pOutFeature + id2 + 7 );
                    }
                    else
                    {
                        vstore8( ZEROVEC8, 0, pOutFeature + id2 );
                        vstore2( ZEROVEC2, 0, pOutFeature + id2 + 8 );
                    }
                }
            }
            else
            {
                vstore4( ZEROVEC4, x, pOutPlrs );
            }
        }

        __kernel void FeatureGatherFromXYZRT(
                __global int *pOutPlrs, __global float *pOutFeature, __global int *numOfPts,
                const float minXRange, const float minYRange, const float minZRange,
                const float pillarXSize, const float pillarYSize, const float pillarZSize,
                const int maxNumPlrs, const int maxNumPtsPerPlr, const int numOutFeatureDim,
                const int numOfPillar ) {
            int x = get_global_id( 0 );

            if ( x < numOfPillar )
            {
                float3 featXYZ = (float3) ( 0.0f );
                float3 outFeatXYZ = (float3) ( 0.0f );
                float2 outFeatRT = (float2) ( 0.0f );
                float3 meanXYZ = (float3) ( 0.0f );
                float2 outPillarXY = convert_float2( vload2( 2, pOutPlrs ) );
                float2 minRangeXY = (float2) ( minXRange, minYRange );
                float2 pillarSizeXY = (float2) ( pillarXSize, pillarYSize ) + 0.5f;
                float2 pillarXY = mad( outPillarXY, pillarSizeXY, minRangeXY );
                int numPts = min( numOfPts[x], maxNumPtsPerPlr );
                for ( int i = 0; i < numPts; i++ )
                {
                    int id1 = x * maxNumPtsPerPlr * numOutFeatureDim + i * numOutFeatureDim;
                    featXYZ = vload3( 0, pOutFeature + id1 );
                    meanXYZ += featXYZ;
                }
                meanXYZ *= native_recip( (float) numPts );

                for ( int j = 0; j < maxNumPtsPerPlr; j++ )
                {
                    int id2 = x * maxNumPtsPerPlr * numOutFeatureDim + j * numOutFeatureDim;
                    if ( j < numPts )
                    {
                        featXYZ = vload3( 0, pOutFeature + id2 );
                        outFeatXYZ = featXYZ - meanXYZ;
                        outFeatRT = featXYZ.s01 - pillarXY;
                        vstore3( outFeatXYZ, 0, pOutFeature + id2 + 5 );
                        vstore2( outFeatRT, 0, pOutFeature + id2 + 8 );
                    }
                    else
                    {
                        vstore8( ZEROVEC8, 0, pOutFeature + id2 );
                        vstore2( ZEROVEC2, 0, pOutFeature + id2 + 8 );
                    }
                }
            }
            else
            {
                int2 outPlr = convert_int2_sat( ZEROVEC2 );
                vstore2( outPlr, x, pOutPlrs );
            }
        } );

#endif   // QC_VOXELIZATION_CLH

