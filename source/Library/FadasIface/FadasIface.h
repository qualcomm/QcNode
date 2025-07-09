#ifndef _FADASIFACE_H
#define _FADASIFACE_H
#include <AEEStdDef.h>
#include <remote.h>


#ifndef __QAIC_HEADER
#define __QAIC_HEADER(ff) ff
#endif //__QAIC_HEADER

#ifndef __QAIC_HEADER_EXPORT
#define __QAIC_HEADER_EXPORT
#endif // __QAIC_HEADER_EXPORT

#ifndef __QAIC_HEADER_ATTRIBUTE
#define __QAIC_HEADER_ATTRIBUTE
#endif // __QAIC_HEADER_ATTRIBUTE

#ifndef __QAIC_IMPL
#define __QAIC_IMPL(ff) ff
#endif //__QAIC_IMPL

#ifndef __QAIC_IMPL_EXPORT
#define __QAIC_IMPL_EXPORT
#endif // __QAIC_IMPL_EXPORT

#ifndef __QAIC_IMPL_ATTRIBUTE
#define __QAIC_IMPL_ATTRIBUTE
#endif // __QAIC_IMPL_ATTRIBUTE
#ifdef __cplusplus
extern "C" {
#endif
/**
    * Opens the handle in the specified domain.  If this is the first
    * handle, this creates the session.  Typically this means opening
    * the device, aka open("/dev/adsprpc-smd"), then calling ioctl
    * device APIs to create a PD on the DSP to execute our code in,
    * then asking that PD to dlopen the .so and dlsym the skel function.
    *
    * @param uri, <interface>_URI"&_dom=aDSP"
    *    <interface>_URI is a QAIC generated uri, or
    *    "file:///<sofilename>?<interface>_skel_handle_invoke&_modver=1.0"
    *    If the _dom parameter is not present, _dom=DEFAULT is assumed
    *    but not forwarded.
    *    Reserved uri keys:
    *      [0]: first unamed argument is the skel invoke function
    *      _dom: execution domain name, _dom=mDSP/aDSP/DEFAULT
    *      _modver: module version, _modver=1.0
    *      _*: any other key name starting with an _ is reserved
    *    Unknown uri keys/values are forwarded as is.
    * @param h, resulting handle
    * @retval, 0 on success
    */
__QAIC_HEADER_EXPORT int __QAIC_HEADER(FadasIface_open)(const char* uri, remote_handle64* h) __QAIC_HEADER_ATTRIBUTE;
/** 
    * Closes a handle.  If this is the last handle to close, the session
    * is closed as well, releasing all the allocated resources.

    * @param h, the handle to close
    * @retval, 0 on success, should always succeed
    */
__QAIC_HEADER_EXPORT int __QAIC_HEADER(FadasIface_close)(remote_handle64 h) __QAIC_HEADER_ATTRIBUTE;
enum FadasIface_FadasRemapPipeline_e {
   FADAS_REMAP_PIPELINE_1C8_NSP, ///< 1 8-bit channel unconverted
   FADAS_REMAP_PIPELINE_1C8_ROISCALE_NSP, ///< 8-bit channel unconverted + ROI scaling
   FADAS_REMAP_PIPELINE_3C888_NSP, ///< 3 8-bit channels unconverted
   FADAS_REMAP_PIPELINE_3C888_ROISCALE_NSP, ///< 3 8-bit channels unconverted + ROI scaling
   FADAS_REMAP_PIPELINE_YUV888_TO_RGB888_NSP, ///< Convert YUV888 to RGB888 [see FadasCvtYUV_UYVYtoRGB()]
   FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NSP, ///< Convert UYVY to RGB [see FadasCvtYUV_UYVYtoRGB()]
   FADAS_REMAP_PIPELINE_VYUY_TO_RGB888_NSP, ///< Convert VYUY to RGB
   FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_ROISCALE_NSP, ///< Convert UYVY to RGB [see FadasCvtYUV_UYVYtoRGB()] + ROI scaling
   FADAS_REMAP_PIPELINE_VYUY_TO_RGB888_ROISCALE_NSP, ///< Convert VYUY to RGB + ROI scaling
   FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMI8_NSP, ///< Convert UYVY to RGB [see FadasCvtYUV_UYVYtoRGB()] + Renormalization
   FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8_NSP, ///< Convert UYVY to RGB [see FadasCvtYUV_UYVYtoRGB()] + Renormalization with uint8 output
   FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NSP,
   FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888_NSP,
   FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMI8_NSP,
   FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8_NSP,
   FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_ROISCALE_NSP,
   FADAS_REMAP_PIPELINE_UYVY_TO_BGR888_NSP, ///< Convert UYVY to BGR
   FADAS_REMAP_PIPELINE_MAX_NSP, ///< \b WARNING: must be last};
   _32BIT_PLACEHOLDER_FadasIface_FadasRemapPipeline_e = 0x7fffffff
};
typedef enum FadasIface_FadasRemapPipeline_e FadasIface_FadasRemapPipeline_e;
typedef struct FadasIface_FadasCameraProps_t FadasIface_FadasCameraProps_t;
struct FadasIface_FadasCameraProps_t {
   double focalLengthX;
   double focalLengthY;
   double principalPointX;
   double principalPointY;
};
typedef struct FadasIface_FadasDistCoeffs_t FadasIface_FadasDistCoeffs_t;
struct FadasIface_FadasDistCoeffs_t {
   double k1;
   double k2;
   double k3;
   double k4;
};
enum FadasIface_FadasImageFormat_e {
   FADAS_IMAGE_FORMAT_UNKNOWN_NSP, ///<  Not specified
   FADAS_IMAGE_FORMAT_Y_NSP, ///<  1-channel, 8-bit pixels
   FADAS_IMAGE_FORMAT_Y12_NSP, ///<  1-channel, 12-bit pixels in 16b containers
   FADAS_IMAGE_FORMAT_UYVY_NSP, ///<  8-bit UYUY
   FADAS_IMAGE_FORMAT_UYVY10_NSP, ///<  10-bit UYUY
   FADAS_IMAGE_FORMAT_VYUY_NSP, ///<  8-bit VYUY
   FADAS_IMAGE_FORMAT_YUV888_NSP, ///<  3-channel, 24-bit pixels
   FADAS_IMAGE_FORMAT_RGB888_NSP, ///<  3-channel, 24-bit pixels
   FADAS_IMAGE_FORMAT_Y10UV10_NSP, ///<  10 bit YUV420 semi-planar
                                   ///<  10 bits in 16-bit container
   FADAS_IMAGE_FORMAT_Y8UV8_NSP, ///<  8 bit luma, 8 bit chroma
                                 ///<  YUV420 semi-planar
   FADAS_IMAGE_FORMAT_COUNT_NSP, ///<  last color format
   _32BIT_PLACEHOLDER_FadasIface_FadasImageFormat_e = 0x7fffffff
};
typedef enum FadasIface_FadasImageFormat_e FadasIface_FadasImageFormat_e;
enum FadasIface_FadasCvtYUVPipeline_e {
   FADAS_CVTYUV_PIPELINE_DownscaleUYVYAndRGB888_NSP, ///< UYVY to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
   FADAS_CVTYUV_PIPELINE_DownscaleUYVYBy2AndRGB888_NSP, ///< UYVY to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
   FADAS_CVTYUV_PIPELINE_DownscaleYUV888AndRGB888_NSP, ///< 3 8-bit channel YUV to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
   FADAS_CVTYUV_PIPELINE_DownscaleYUV888By2AndRGB888_NSP, ///< 3 8-bit channel YUV to RGB using parameters of FadasCvtYUV_UYVYtoRGB()
   FADAS_CVTYUV_PIPELINE_DownscaleY8UV8by2_NSP, ///< Multi planar image (Y,UV) with 8 bit data in 8 bit carriers
   FADAS_CVTYUV_PIPELINE_DownscaleY10UV10by2_NSP, ///< Multi planar image (Y,UV) with 10 bit data in 16 bit carriers
   FADAS_CVTYUV_PIPELINE_COUNT_NSP,
   _32BIT_PLACEHOLDER_FadasIface_FadasCvtYUVPipeline_e = 0x7fffffff
};
typedef enum FadasIface_FadasCvtYUVPipeline_e FadasIface_FadasCvtYUVPipeline_e;
enum FadasIface_FadasBufType_e {
   FADAS_BUF_TYPE_IN_NSP,
   FADAS_BUF_TYPE_OUT_NSP,
   FADAS_BUF_TYPE_INOUT_NSP,
   FADAS_BUF_TYPE_MAX_NSP,
   _32BIT_PLACEHOLDER_FadasIface_FadasBufType_e = 0x7fffffff
};
typedef enum FadasIface_FadasBufType_e FadasIface_FadasBufType_e;
typedef struct FadasIface_FadasImgProps_t FadasIface_FadasImgProps_t;
struct FadasIface_FadasImgProps_t {
   uint32_t width;
   uint32_t height;
   FadasIface_FadasImageFormat_e format;
   uint32_t stride[4];
   uint32_t numPlanes;
   uint32_t actualHeight[4];
};
typedef struct FadasIface_FadasROI_t FadasIface_FadasROI_t;
struct FadasIface_FadasROI_t {
   uint32_t x;
   uint32_t y;
   uint32_t width;
   uint32_t height;
};
typedef struct FadasIface_FadasNormlzParams_t FadasIface_FadasNormlzParams_t;
struct FadasIface_FadasNormlzParams_t {
   float sub;
   float mul;
   float add;
};
typedef struct FadasIface_Pt3D_t FadasIface_Pt3D_t;
struct FadasIface_Pt3D_t {
   float x;
   float y;
   float z;
};
typedef struct FadasIface_Grid2D_t FadasIface_Grid2D_t;
struct FadasIface_Grid2D_t {
   float tlX;
   float tlY;
   float brX;
   float brY;
   float cellSizeX;
   float cellSizeY;
};
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasInit)(remote_handle64 _h, int32_t* status) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasVersion)(remote_handle64 _h, uint8_t* version, int versionLen) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasDeInit)(remote_handle64 _h) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasRemap_CreateMapFromMap)(remote_handle64 _h, uint64* mapPtr, uint32_t camWidth, uint32_t camHeight, uint32_t mapWidth, uint32_t mapHeight, int32_t mapXFd, int32_t mapYFd, uint32_t mapStride, FadasIface_FadasRemapPipeline_e imgFormat, uint8_t borderConst) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasRemap_CreateMapNoUndistortion)(remote_handle64 _h, uint64* mapPtr, uint32_t camWidth, uint32_t camHeight, uint32_t mapWidth, uint32_t mapHeight, FadasIface_FadasRemapPipeline_e imgFormat, uint8_t borderConst) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasRemap_DestroyMap)(remote_handle64 _h, uint64 mapPtr) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasRemap_CreateWorkers)(remote_handle64 _h, uint64* worker_ptr, uint32_t nThreads, FadasIface_FadasRemapPipeline_e imgFormat) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasRemap_DestroyWorkers)(remote_handle64 _h, uint64 worker_ptr) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasRemap_RunMT)(remote_handle64 _h, const uint64* workerPtrs, int workerPtrsLen, const uint64* mapPtrs, int mapPtrsLen, const int32_t* srcFds, int srcFdsLen, const uint32_t* offsets, int offsetsLen, const FadasIface_FadasImgProps_t* srcProps, int srcPropsLen, int32_t dstFd, uint32_t dstLen, const FadasIface_FadasImgProps_t* dstProps, const FadasIface_FadasROI_t* dstROIs, int dstROIsLen, const FadasIface_FadasNormlzParams_t* normlz, int normlzLen) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_mmap)(remote_handle64 _h, int32_t bufFd, uint32_t bufSize) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_munmap)(remote_handle64 _h, int32_t bufFd, uint32_t bufSize) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasRegBuf)(remote_handle64 _h, FadasIface_FadasBufType_e bufType, int32_t bufFd, uint32_t bufSize, uint32_t bufOffset, uint32_t batchSize) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_FadasDeregBuf)(remote_handle64 _h, int32_t bufFd, uint32_t bufSize, uint32_t bufOffset, uint32_t batchSize) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_PointPillarCreate)(remote_handle64 _h, const FadasIface_Pt3D_t* pPlrSize, const FadasIface_Pt3D_t* pMinRange, const FadasIface_Pt3D_t* pMaxRange, uint32_t maxNumInPts, uint32_t numInFeatureDim, uint32_t maxNumPlrs, uint32_t maxNumPtsPerPlr, uint32_t numOutFeatureDim, uint64_t* phPreProc) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_PointPillarRun)(remote_handle64 _h, uint64_t hPreProc, uint32_t numPts, int32_t fdInPts, uint32_t inPtsOffset, uint32_t inPtsSize, int32_t fdOutPlrs, uint32_t outPlrsOffset, uint32_t outPlrsSize, int32_t fdOutFeature, uint32_t outFeatureOffset, uint32_t outFeatureSize, uint32_t* pNumOutPlrs) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_PointPillarDestroy)(remote_handle64 _h, uint64_t hPreProc) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_ExtractBBoxCreate)(remote_handle64 _h, uint32_t maxNumInPts, uint32_t numInFeatureDim, uint32_t maxNumDetOut, uint32_t numClass, const FadasIface_Grid2D_t* pGrid, float threshScore, float threshIOU, float minCentreX, float minCentreY, float minCentreZ, float maxCentreX, float maxCentreY, float maxCentreZ, const uint8_t* labelSelect, int labelSelectLen, uint32_t maxNumFilter, uint64_t* phPostProc) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_ExtractBBoxRun)(remote_handle64 _h, uint64_t hPostProc, uint32_t numPts, const int32_t* fds, int fdsLen, const uint32_t* offsets, int offsetsLen, const uint32_t* sizes, int sizesLen, uint8_t bMapPtsToBBox, uint8_t bBBoxFilter, uint32_t* pNumDetOut) __QAIC_HEADER_ATTRIBUTE;
__QAIC_HEADER_EXPORT AEEResult __QAIC_HEADER(FadasIface_ExtractBBoxDestroy)(remote_handle64 _h, uint64_t hPostProc) __QAIC_HEADER_ATTRIBUTE;
#ifndef FadasIface_URI
#define FadasIface_URI "file:///libFadasIface_skel.so?FadasIface_skel_handle_invoke&_modver=1.0"
#endif /*FadasIface_URI*/
#ifdef __cplusplus
}
#endif
#endif //_FADASIFACE_H
