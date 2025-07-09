#ifndef _FADASIFACE_STUB_H
#define _FADASIFACE_STUB_H
#include "FadasIface.h"
#ifndef _QAIC_ENV_H
#define _QAIC_ENV_H

#include <stdio.h>

#ifdef __GNUC__
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#else
#pragma GCC diagnostic ignored "-Wpragmas"
#endif
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifndef _ATTRIBUTE_UNUSED

#ifdef _WIN32
#define _ATTRIBUTE_UNUSED
#else
#define _ATTRIBUTE_UNUSED __attribute__ ((unused))
#endif

#endif // _ATTRIBUTE_UNUSED

#ifndef __QAIC_REMOTE
#define __QAIC_REMOTE(ff) ff
#endif //__QAIC_REMOTE

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

#ifndef __QAIC_STUB
#define __QAIC_STUB(ff) ff
#endif //__QAIC_STUB

#ifndef __QAIC_STUB_EXPORT
#define __QAIC_STUB_EXPORT
#endif // __QAIC_STUB_EXPORT

#ifndef __QAIC_STUB_ATTRIBUTE
#define __QAIC_STUB_ATTRIBUTE
#endif // __QAIC_STUB_ATTRIBUTE

#ifndef __QAIC_SKEL
#define __QAIC_SKEL(ff) ff
#endif //__QAIC_SKEL__

#ifndef __QAIC_SKEL_EXPORT
#define __QAIC_SKEL_EXPORT
#endif // __QAIC_SKEL_EXPORT

#ifndef __QAIC_SKEL_ATTRIBUTE
#define __QAIC_SKEL_ATTRIBUTE
#endif // __QAIC_SKEL_ATTRIBUTE

#ifdef __QAIC_DEBUG__
   #ifndef __QAIC_DBG_PRINTF__
   #include <stdio.h>
   #define __QAIC_DBG_PRINTF__( ee ) do { printf ee ; } while(0)
   #endif
#else
   #define __QAIC_DBG_PRINTF__( ee ) (void)0
#endif


#define _OFFSET(src, sof)  ((void*)(((char*)(src)) + (sof)))

#define _COPY(dst, dof, src, sof, sz)  \
   do {\
         struct __copy { \
            char ar[sz]; \
         };\
         *(struct __copy*)_OFFSET(dst, dof) = *(struct __copy*)_OFFSET(src, sof);\
   } while (0)

#define _COPYIF(dst, dof, src, sof, sz)  \
   do {\
      if(_OFFSET(dst, dof) != _OFFSET(src, sof)) {\
         _COPY(dst, dof, src, sof, sz); \
      } \
   } while (0)

_ATTRIBUTE_UNUSED
static __inline void _qaic_memmove(void* dst, void* src, int size) {
   int i;
   for(i = 0; i < size; ++i) {
      ((char*)dst)[i] = ((char*)src)[i];
   }
}

#define _MEMMOVEIF(dst, src, sz)  \
   do {\
      if(dst != src) {\
         _qaic_memmove(dst, src, sz);\
      } \
   } while (0)


#define _ASSIGN(dst, src, sof)  \
   do {\
      dst = OFFSET(src, sof); \
   } while (0)

#define _STD_STRLEN_IF(str) (str == 0 ? 0 : strlen(str))

#include "AEEStdErr.h"

#define _TRY(ee, func) \
   do { \
      if (AEE_SUCCESS != ((ee) = func)) {\
         __QAIC_DBG_PRINTF__((__FILE__ ":%d:error:%d:%s\n", __LINE__, (int)(ee),#func));\
         goto ee##bail;\
      } \
   } while (0)

#define _CATCH(exception) exception##bail: if (exception != AEE_SUCCESS)

#define _ASSERT(nErr, ff) _TRY(nErr, 0 == (ff) ? AEE_EBADPARM : AEE_SUCCESS)

#ifdef __QAIC_DEBUG__
#define _ALLOCATE(nErr, pal, size, alignment, pv) _TRY(nErr, _allocator_alloc(pal, __FILE_LINE__, size, alignment, (void**)&pv));\
                                                  _ASSERT(nErr,pv || !(size))
#else
#define _ALLOCATE(nErr, pal, size, alignment, pv) _TRY(nErr, _allocator_alloc(pal, 0, size, alignment, (void**)&pv));\
                                                  _ASSERT(nErr,pv || !(size))
#endif


#endif // _QAIC_ENV_H

#include <string.h>
#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

#include <stdlib.h>
#include <stdint.h>

typedef struct _heap _heap;
struct _heap {
   _heap* pPrev;
   const char* loc;
   uint64_t buf;
};

typedef struct _allocator {
   _heap* pheap;
   uint8_t* stack;
   uint8_t* stackEnd;
   int nSize;
} _allocator;

_ATTRIBUTE_UNUSED
static __inline int _heap_alloc(_heap** ppa, const char* loc, int size, void** ppbuf) {
   _heap* pn = 0;
   pn = malloc(size + sizeof(_heap) - sizeof(uint64_t));
   if(pn != 0) {
      pn->pPrev = *ppa;
      pn->loc = loc;
      *ppa = pn;
      *ppbuf = (void*)&(pn->buf);
      return 0;
   } else {
      return -1;
   }
}
#define _ALIGN_SIZE(x, y) (((x) + (y-1)) & ~(y-1))

_ATTRIBUTE_UNUSED
static __inline int _allocator_alloc(_allocator* me,
                                    const char* loc,
                                    int size,
                                    unsigned int al,
                                    void** ppbuf) {
   if(size < 0) {
      return -1;
   } else if (size == 0) {
      *ppbuf = 0;
      return 0;
   }
   if((_ALIGN_SIZE((uintptr_t)me->stackEnd, al) + size) < (uintptr_t)me->stack + me->nSize) {
      *ppbuf = (uint8_t*)_ALIGN_SIZE((uintptr_t)me->stackEnd, al);
      me->stackEnd = (uint8_t*)_ALIGN_SIZE((uintptr_t)me->stackEnd, al) + size;
      return 0;
   } else {
      return _heap_alloc(&me->pheap, loc, size, ppbuf);
   }
}

_ATTRIBUTE_UNUSED
static __inline void _allocator_deinit(_allocator* me) {
   _heap* pa = me->pheap;
   while(pa != 0) {
      _heap* pn = pa;
      const char* loc = pn->loc;
      (void)loc;
      pa = pn->pPrev;
      free(pn);
   }
}

_ATTRIBUTE_UNUSED
static __inline void _allocator_init(_allocator* me, uint8_t* stack, int stackSize) {
   me->stack =  stack;
   me->stackEnd =  stack + stackSize;
   me->nSize = stackSize;
   me->pheap = 0;
}


#endif // _ALLOCATOR_H

#ifndef SLIM_H
#define SLIM_H

#include <stdint.h>

//a C data structure for the idl types that can be used to implement
//static and dynamic language bindings fairly efficiently.
//
//the goal is to have a minimal ROM and RAM footprint and without
//doing too many allocations.  A good way to package these things seemed
//like the module boundary, so all the idls within  one module can share
//all the type references.


#define PARAMETER_IN       0x0
#define PARAMETER_OUT      0x1
#define PARAMETER_INOUT    0x2
#define PARAMETER_ROUT     0x3
#define PARAMETER_INROUT   0x4

//the types that we get from idl
#define TYPE_OBJECT             0x0
#define TYPE_INTERFACE          0x1
#define TYPE_PRIMITIVE          0x2
#define TYPE_ENUM               0x3
#define TYPE_STRING             0x4
#define TYPE_WSTRING            0x5
#define TYPE_STRUCTURE          0x6
#define TYPE_UNION              0x7
#define TYPE_ARRAY              0x8
#define TYPE_SEQUENCE           0x9

//these require the pack/unpack to recurse
//so it's a hint to those languages that can optimize in cases where
//recursion isn't necessary.
#define TYPE_COMPLEX_STRUCTURE  (0x10 | TYPE_STRUCTURE)
#define TYPE_COMPLEX_UNION      (0x10 | TYPE_UNION)
#define TYPE_COMPLEX_ARRAY      (0x10 | TYPE_ARRAY)
#define TYPE_COMPLEX_SEQUENCE   (0x10 | TYPE_SEQUENCE)


typedef struct Type Type;

#define INHERIT_TYPE\
   int32_t nativeSize;                /*in the simple case its the same as wire size and alignment*/\
   union {\
      struct {\
         const uintptr_t         p1;\
         const uintptr_t         p2;\
      } _cast;\
      struct {\
         uint32_t  iid;\
         uint32_t  bNotNil;\
      } object;\
      struct {\
         const Type  *arrayType;\
         int32_t      nItems;\
      } array;\
      struct {\
         const Type *seqType;\
         int32_t      nMaxLen;\
      } seqSimple; \
      struct {\
         uint32_t bFloating;\
         uint32_t bSigned;\
      } prim; \
      const SequenceType* seqComplex;\
      const UnionType  *unionType;\
      const StructType *structType;\
      int32_t         stringMaxLen;\
      uint8_t        bInterfaceNotNil;\
   } param;\
   uint8_t    type;\
   uint8_t    nativeAlignment\

typedef struct UnionType UnionType;
typedef struct StructType StructType;
typedef struct SequenceType SequenceType;
struct Type {
   INHERIT_TYPE;
};

struct SequenceType {
   const Type *         seqType;
   uint32_t               nMaxLen;
   uint32_t               inSize;
   uint32_t               routSizePrimIn;
   uint32_t               routSizePrimROut;
};

//byte offset from the start of the case values for
//this unions case value array.  it MUST be aligned
//at the alignment requrements for the descriptor
//
//if negative it means that the unions cases are
//simple enumerators, so the value read from the descriptor
//can be used directly to find the correct case
typedef union CaseValuePtr CaseValuePtr;
union CaseValuePtr {
   const uint8_t*   value8s;
   const uint16_t*  value16s;
   const uint32_t*  value32s;
   const uint64_t*  value64s;
};

//these are only used in complex cases
//so I pulled them out of the type definition as references to make
//the type smaller
struct UnionType {
   const Type           *descriptor;
   uint32_t               nCases;
   const CaseValuePtr   caseValues;
   const Type * const   *cases;
   int32_t               inSize;
   int32_t               routSizePrimIn;
   int32_t               routSizePrimROut;
   uint8_t                inAlignment;
   uint8_t                routAlignmentPrimIn;
   uint8_t                routAlignmentPrimROut;
   uint8_t                inCaseAlignment;
   uint8_t                routCaseAlignmentPrimIn;
   uint8_t                routCaseAlignmentPrimROut;
   uint8_t                nativeCaseAlignment;
   uint8_t              bDefaultCase;
};

struct StructType {
   uint32_t               nMembers;
   const Type * const   *members;
   int32_t               inSize;
   int32_t               routSizePrimIn;
   int32_t               routSizePrimROut;
   uint8_t                inAlignment;
   uint8_t                routAlignmentPrimIn;
   uint8_t                routAlignmentPrimROut;
};

typedef struct Parameter Parameter;
struct Parameter {
   INHERIT_TYPE;
   uint8_t    mode;
   uint8_t  bNotNil;
};

#define SLIM_IFPTR32(is32,is64) (sizeof(uintptr_t) == 4 ? (is32) : (is64))
#define SLIM_SCALARS_IS_DYNAMIC(u) (((u) & 0x00ffffff) == 0x00ffffff)

typedef struct Method Method;
struct Method {
   uint32_t                    uScalars;            //no method index
   int32_t                     primInSize;
   int32_t                     primROutSize;
   int                         maxArgs;
   int                         numParams;
   const Parameter * const     *params;
   uint8_t                       primInAlignment;
   uint8_t                       primROutAlignment;
};

typedef struct Interface Interface;

struct Interface {
   int                            nMethods;
   const Method  * const          *methodArray;
   int                            nIIds;
   const uint32_t                   *iids;
   const uint16_t*                  methodStringArray;
   const uint16_t*                  methodStrings;
   const char*                    strings;
};


#endif //SLIM_H


#ifndef _FADASIFACE_SLIM_H
#define _FADASIFACE_SLIM_H
#include <stdint.h>

#ifndef __QAIC_SLIM
#define __QAIC_SLIM(ff) ff
#endif
#ifndef __QAIC_SLIM_EXPORT
#define __QAIC_SLIM_EXPORT
#endif

static const Type types[10];
static const Type* const typeArrays[16] = {&(types[9]),&(types[9]),&(types[9]),&(types[9]),&(types[9]),&(types[9]),&(types[3]),&(types[3]),&(types[5]),&(types[6]),&(types[3]),&(types[6]),&(types[3]),&(types[3]),&(types[3]),&(types[3])};
static const StructType structTypes[4] = {{0x6,&(typeArrays[6]),0x30,0x0,0x30,0x4,0x1,0x4},{0x3,&(typeArrays[0]),0xc,0x0,0xc,0x4,0x1,0x4},{0x6,&(typeArrays[0]),0x18,0x0,0x18,0x4,0x1,0x4},{0x4,&(typeArrays[12]),0x10,0x0,0x10,0x4,0x1,0x4}};
static const Type types[10] = {{0x1,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x1},{0x8,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x8},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4},{0x30,{{(const uintptr_t)&(structTypes[0]),0}}, 6,0x4},{0x4,{{0,0}}, 3,0x4},{0x10,{{(const uintptr_t)&(types[3]),(const uintptr_t)0x4}}, 8,0x4},{0x10,{{(const uintptr_t)&(structTypes[3]),0}}, 6,0x4},{0xc,{{(const uintptr_t)&(structTypes[1]),0}}, 6,0x4},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4}};
static const Parameter parameters[26] = {{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)0x0,0}}, 4,SLIM_IFPTR32(0x4,0x8),0,0},{SLIM_IFPTR32(0x4,0x8),{{(const uintptr_t)0xdeadc0de,(const uintptr_t)0}}, 0,SLIM_IFPTR32(0x4,0x8),3,0},{SLIM_IFPTR32(0x4,0x8),{{(const uintptr_t)0xdeadc0de,(const uintptr_t)0}}, 0,SLIM_IFPTR32(0x4,0x8),0,0},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4,3,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[0]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),3,0},{0x8,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x8,4,0},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4,0,0},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4,0,0},{0x4,{{0,0}}, 3,0x4,0,0},{0x1,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x1,0,0},{0x8,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x8,0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[1]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[2]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[3]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[4]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),0,0},{0x30,{{(const uintptr_t)&(structTypes[0]),0}}, 6,0x4,0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[7]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[8]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),0,0},{0x4,{{0,0}}, 3,0x4,0,0},{0xc,{{(const uintptr_t)&(structTypes[1]),0}}, 6,0x4,0,0},{0x8,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x8,3,0},{0x8,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x8,0,0},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4,3,0},{0x18,{{(const uintptr_t)&(structTypes[2]),0}}, 6,0x4,0,0},{0x4,{{(const uintptr_t)0,(const uintptr_t)1}}, 2,0x4,0,0},{SLIM_IFPTR32(0x8,0x10),{{(const uintptr_t)&(types[0]),(const uintptr_t)0x0}}, 9,SLIM_IFPTR32(0x4,0x8),0,0}};
static const Parameter* const parameterArrays[86] = {(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[23])),(&(parameters[24])),(&(parameters[24])),(&(parameters[24])),(&(parameters[24])),(&(parameters[24])),(&(parameters[24])),(&(parameters[24])),(&(parameters[24])),(&(parameters[25])),(&(parameters[6])),(&(parameters[20])),(&(parameters[21])),(&(parameters[6])),(&(parameters[7])),(&(parameters[6])),(&(parameters[6])),(&(parameters[7])),(&(parameters[6])),(&(parameters[6])),(&(parameters[7])),(&(parameters[6])),(&(parameters[6])),(&(parameters[22])),(&(parameters[11])),(&(parameters[11])),(&(parameters[12])),(&(parameters[13])),(&(parameters[14])),(&(parameters[7])),(&(parameters[6])),(&(parameters[15])),(&(parameters[16])),(&(parameters[17])),(&(parameters[5])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[7])),(&(parameters[7])),(&(parameters[6])),(&(parameters[8])),(&(parameters[9])),(&(parameters[19])),(&(parameters[19])),(&(parameters[19])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[20])),(&(parameters[21])),(&(parameters[6])),(&(parameters[12])),(&(parameters[13])),(&(parameters[13])),(&(parameters[9])),(&(parameters[9])),(&(parameters[22])),(&(parameters[5])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[8])),(&(parameters[9])),(&(parameters[18])),(&(parameters[7])),(&(parameters[6])),(&(parameters[6])),(&(parameters[6])),(&(parameters[5])),(&(parameters[6])),(&(parameters[8])),(&(parameters[0])),(&(parameters[1])),(&(parameters[10])),(&(parameters[4])),(&(parameters[3])),(&(parameters[2]))};
static const Method methods[18] = {{REMOTE_SCALARS_MAKEX(0,0,0x2,0x0,0x0,0x1),0x4,0x0,2,2,(&(parameterArrays[80])),0x4,0x1},{REMOTE_SCALARS_MAKEX(0,0,0x0,0x0,0x1,0x0),0x0,0x0,1,1,(&(parameterArrays[85])),0x1,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x0,0x1,0x0,0x0),0x0,0x4,1,1,(&(parameterArrays[84])),0x1,0x4},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x1,0x0,0x0),0x4,0x0,3,1,(&(parameterArrays[83])),0x4,0x1},{REMOTE_SCALARS_MAKEX(0,0,0x0,0x0,0x0,0x0),0x0,0x0,0,0,0,0x0,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x1,0x0,0x0),0x29,0x8,10,10,(&(parameterArrays[38])),0x8,0x8},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x1,0x0,0x0),0x1d,0x8,7,7,(&(parameterArrays[65])),0x8,0x8},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x0,0x0,0x0),0x8,0x0,3,1,(&(parameterArrays[82])),0x8,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x1,0x0,0x0),0x10,0x8,3,3,(&(parameterArrays[77])),0x8,0x8},{REMOTE_SCALARS_MAKEX(0,0,0x8,0x0,0x0,0x0),0x54,0x0,17,10,(&(parameterArrays[28])),0x4,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x0,0x0,0x0),0x8,0x0,2,2,(&(parameterArrays[18])),0x4,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x0,0x0,0x0),0x14,0x0,5,5,(&(parameterArrays[72])),0x4,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x0,0x0,0x0),0x10,0x0,4,4,(&(parameterArrays[73])),0x4,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x1,0x0,0x0),0x38,0x8,9,9,(&(parameterArrays[48])),0x4,0x8},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x1,0x0,0x0),0x30,0x4,14,12,(&(parameterArrays[16])),0x8,0x4},{REMOTE_SCALARS_MAKEX(0,0,0x1,0x0,0x0,0x0),0x8,0x0,3,1,(&(parameterArrays[16])),0x8,0x0},{REMOTE_SCALARS_MAKEX(0,0,0x2,0x1,0x0,0x0),0x50,0x8,17,16,(&(parameterArrays[0])),0x4,0x8},{REMOTE_SCALARS_MAKEX(0,0,0x4,0x1,0x0,0x0),0x1a,0x4,13,8,(&(parameterArrays[57])),0x8,0x4}};
static const Method* const methodArrays[21] = {&(methods[0]),&(methods[1]),&(methods[2]),&(methods[3]),&(methods[4]),&(methods[5]),&(methods[6]),&(methods[7]),&(methods[8]),&(methods[7]),&(methods[9]),&(methods[10]),&(methods[10]),&(methods[11]),&(methods[12]),&(methods[13]),&(methods[14]),&(methods[15]),&(methods[16]),&(methods[17]),&(methods[15])};
static const char strings[1132] = "FadasRemap_CreateMapNoUndistortion\0FadasRemap_CreateMapFromMap\0FadasRemap_DestroyWorkers\0FadasRemap_CreateWorkers\0FadasRemap_DestroyMap\0ExtractBBoxDestroy\0PointPillarDestroy\0ExtractBBoxCreate\0PointPillarCreate\0outFeatureOffset\0numOutFeatureDim\0FadasRemap_RunMT\0maxNumPtsPerPlr\0numInFeatureDim\0ExtractBBoxRun\0outFeatureSize\0PointPillarRun\0bMapPtsToBBox\0outPlrsOffset\0FadasDeregBuf\0maxNumFilter\0maxNumDetOut\0fdOutFeature\0actualHeight\0FadasVersion\0bBBoxFilter\0labelSelect\0threshScore\0pNumOutPlrs\0outPlrsSize\0inPtsOffset\0maxNumInPts\0FadasRegBuf\0borderConst\0FadasDeInit\0pNumDetOut\0phPostProc\0maxCentreZ\0maxCentreY\0maxCentreX\0minCentreZ\0minCentreY\0minCentreX\0maxNumPlrs\0workerPtrs\0worker_ptr\0threshIOU\0cellSizeY\0cellSizeX\0fdOutPlrs\0inPtsSize\0phPreProc\0pMaxRange\0pMinRange\0batchSize\0bufOffset\0numPlanes\0imgFormat\0mapStride\0mapHeight\0camHeight\0FadasInit\0numClass\0pPlrSize\0dstProps\0srcProps\0nThreads\0mapWidth\0camWidth\0fdInPts\0bufType\0bufSize\0dstROIs\0offsets\0mapPtrs\0version\0numPts\0munmap\0normlz\0dstLen\0stride\0format\0height\0srcFds\0mapYFd\0mapXFd\0mapPtr\0status\0sizes\0pGrid\0bufFd\0dstFd\0width\0close\0mmap\0open\0fds\0brY\0brX\0tlY\0tlX\0add\0mul\0sub\0uri\0";
static const uint16_t methodStrings[152] = {244,664,949,1014,941,873,1073,1007,1000,993,786,419,1067,986,864,1073,1007,1000,993,786,419,933,350,153,1073,1007,979,1123,1119,1115,174,517,277,393,846,1055,1111,1107,1103,1099,706,696,469,686,642,631,620,609,598,587,457,380,576,192,855,350,153,984,756,350,153,984,746,350,153,984,517,277,653,261,227,736,323,737,965,909,505,726,716,352,493,406,210,308,481,35,1035,900,826,891,816,1028,1021,806,796,541,293,577,965,1095,941,1049,338,445,565,0,1035,900,826,891,816,796,541,529,917,1061,925,776,766,366,1061,925,776,766,89,675,882,796,972,1061,925,1085,1061,925,1090,1127,898,136,577,155,737,63,675,114,1035,432,957,836,1042,1079,898,553};
static const uint16_t methodStringsArrays[21] = {134,149,147,145,151,85,105,143,124,141,0,131,128,113,119,53,72,139,30,96,137};
__QAIC_SLIM_EXPORT const Interface __QAIC_SLIM(FadasIface_slim) = {21,&(methodArrays[0]),0,0,&(methodStringsArrays [0]),methodStrings,strings};
#endif //_FADASIFACE_SLIM_H


#ifdef __cplusplus
extern "C" {
#endif
extern int remote_register_dma_handle(int, uint32_t);
__QAIC_STUB_EXPORT int __QAIC_STUB(FadasIface_open)(const char* uri, remote_handle64* h) __QAIC_STUB_ATTRIBUTE {
   return __QAIC_REMOTE(remote_handle64_open)(uri, h);
}
__QAIC_STUB_EXPORT int __QAIC_STUB(FadasIface_close)(remote_handle64 h) __QAIC_STUB_ATTRIBUTE {
   return __QAIC_REMOTE(remote_handle64_close)(h);
}
static __inline int _stub_method(remote_handle64 _handle, uint32_t _mid, uint32_t _rout0[1]) {
   int _numIn[1];
   remote_arg _pra[1];
   uint32_t _primROut[1];
   int _nErr = 0;
   _numIn[0] = 0;
   _pra[(_numIn[0] + 0)].buf.pv = (void*)_primROut;
   _pra[(_numIn[0] + 0)].buf.nLen = sizeof(_primROut);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 0, 1, 0, 0), _pra));
   _COPY(_rout0, 0, _primROut, 0, 4);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasInit)(remote_handle64 _handle, int32_t* status) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 2;
   return _stub_method(_handle, _mid, (uint32_t*)status);
}
static __inline int _stub_method_1(remote_handle64 _handle, uint32_t _mid, char* _rout0[1], uint32_t _rout0Len[1]) {
   int _numIn[1];
   remote_arg _pra[2];
   uint32_t _primIn[1];
   remote_arg* _praIn;
   remote_arg* _praROut;
   int _nErr = 0;
   _numIn[0] = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _COPY(_primIn, 0, _rout0Len, 0, 4);
   _praIn = (_pra + 1);
   _praROut = (_praIn + _numIn[0] + 0);
   _praROut[0].buf.pv = _rout0[0];
   _praROut[0].buf.nLen = (1 * _rout0Len[0]);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 1, 0, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasVersion)(remote_handle64 _handle, uint8_t* version, int versionLen) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 3;
   return _stub_method_1(_handle, _mid, (char**)&version, (uint32_t*)&versionLen);
}
static __inline int _stub_method_2(remote_handle64 _handle, uint32_t _mid) {
   remote_arg* _pra = 0;
   int _nErr = 0;
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 0, 0, 0, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasDeInit)(remote_handle64 _handle) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 4;
   return _stub_method_2(_handle, _mid);
}
static __inline int _stub_method_3(remote_handle64 _handle, uint32_t _mid, uint64_t _in0[1], uint64_t _rout0[1], uint32_t _in1[1], uint32_t _in2[1], uint32_t _in3[1], uint32_t _in4[1], uint32_t _in5[1], uint32_t _in6[1], uint32_t _in7[1], uint32_t _in8[1], uint8_t _in9[1]) {
   int _numIn[1];
   remote_arg _pra[2];
   uint64_t _primIn[6];
   uint64_t _primROut[1];
   int _nErr = 0;
   _numIn[0] = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _pra[(_numIn[0] + 1)].buf.pv = (void*)_primROut;
   _pra[(_numIn[0] + 1)].buf.nLen = sizeof(_primROut);
   _COPY(_primIn, 0, _in0, 0, 8);
   _COPY(_primIn, 8, _in1, 0, 4);
   _COPY(_primIn, 12, _in2, 0, 4);
   _COPY(_primIn, 16, _in3, 0, 4);
   _COPY(_primIn, 20, _in4, 0, 4);
   _COPY(_primIn, 24, _in5, 0, 4);
   _COPY(_primIn, 28, _in6, 0, 4);
   _COPY(_primIn, 32, _in7, 0, 4);
   _COPY(_primIn, 36, _in8, 0, 4);
   _COPY(_primIn, 40, _in9, 0, 1);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 1, 0, 0), _pra));
   _COPY(_rout0, 0, _primROut, 0, 8);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasRemap_CreateMapFromMap)(remote_handle64 _handle, uint64* mapPtr, uint32_t camWidth, uint32_t camHeight, uint32_t mapWidth, uint32_t mapHeight, int32_t mapXFd, int32_t mapYFd, uint32_t mapStride, FadasIface_FadasRemapPipeline_e imgFormat, uint8_t borderConst) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 5;
   return _stub_method_3(_handle, _mid, (uint64_t*)mapPtr, (uint64_t*)mapPtr, (uint32_t*)&camWidth, (uint32_t*)&camHeight, (uint32_t*)&mapWidth, (uint32_t*)&mapHeight, (uint32_t*)&mapXFd, (uint32_t*)&mapYFd, (uint32_t*)&mapStride, (uint32_t*)&imgFormat, (uint8_t*)&borderConst);
}
static __inline int _stub_method_4(remote_handle64 _handle, uint32_t _mid, uint64_t _in0[1], uint64_t _rout0[1], uint32_t _in1[1], uint32_t _in2[1], uint32_t _in3[1], uint32_t _in4[1], uint32_t _in5[1], uint8_t _in6[1]) {
   int _numIn[1];
   remote_arg _pra[2];
   uint64_t _primIn[4];
   uint64_t _primROut[1];
   int _nErr = 0;
   _numIn[0] = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _pra[(_numIn[0] + 1)].buf.pv = (void*)_primROut;
   _pra[(_numIn[0] + 1)].buf.nLen = sizeof(_primROut);
   _COPY(_primIn, 0, _in0, 0, 8);
   _COPY(_primIn, 8, _in1, 0, 4);
   _COPY(_primIn, 12, _in2, 0, 4);
   _COPY(_primIn, 16, _in3, 0, 4);
   _COPY(_primIn, 20, _in4, 0, 4);
   _COPY(_primIn, 24, _in5, 0, 4);
   _COPY(_primIn, 28, _in6, 0, 1);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 1, 0, 0), _pra));
   _COPY(_rout0, 0, _primROut, 0, 8);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasRemap_CreateMapNoUndistortion)(remote_handle64 _handle, uint64* mapPtr, uint32_t camWidth, uint32_t camHeight, uint32_t mapWidth, uint32_t mapHeight, FadasIface_FadasRemapPipeline_e imgFormat, uint8_t borderConst) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 6;
   return _stub_method_4(_handle, _mid, (uint64_t*)mapPtr, (uint64_t*)mapPtr, (uint32_t*)&camWidth, (uint32_t*)&camHeight, (uint32_t*)&mapWidth, (uint32_t*)&mapHeight, (uint32_t*)&imgFormat, (uint8_t*)&borderConst);
}
static __inline int _stub_method_5(remote_handle64 _handle, uint32_t _mid, uint64_t _in0[1]) {
   remote_arg _pra[1];
   uint64_t _primIn[1];
   int _nErr = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _COPY(_primIn, 0, _in0, 0, 8);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 0, 0, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasRemap_DestroyMap)(remote_handle64 _handle, uint64 mapPtr) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 7;
   return _stub_method_5(_handle, _mid, (uint64_t*)&mapPtr);
}
static __inline int _stub_method_6(remote_handle64 _handle, uint32_t _mid, uint64_t _in0[1], uint64_t _rout0[1], uint32_t _in1[1], uint32_t _in2[1]) {
   int _numIn[1];
   remote_arg _pra[2];
   uint64_t _primIn[2];
   uint64_t _primROut[1];
   int _nErr = 0;
   _numIn[0] = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _pra[(_numIn[0] + 1)].buf.pv = (void*)_primROut;
   _pra[(_numIn[0] + 1)].buf.nLen = sizeof(_primROut);
   _COPY(_primIn, 0, _in0, 0, 8);
   _COPY(_primIn, 8, _in1, 0, 4);
   _COPY(_primIn, 12, _in2, 0, 4);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 1, 0, 0), _pra));
   _COPY(_rout0, 0, _primROut, 0, 8);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasRemap_CreateWorkers)(remote_handle64 _handle, uint64* worker_ptr, uint32_t nThreads, FadasIface_FadasRemapPipeline_e imgFormat) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 8;
   return _stub_method_6(_handle, _mid, (uint64_t*)worker_ptr, (uint64_t*)worker_ptr, (uint32_t*)&nThreads, (uint32_t*)&imgFormat);
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasRemap_DestroyWorkers)(remote_handle64 _handle, uint64 worker_ptr) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 9;
   return _stub_method_5(_handle, _mid, (uint64_t*)&worker_ptr);
}
static __inline int _stub_method_7(remote_handle64 _handle, uint32_t _mid, char* _in0[1], uint32_t _in0Len[1], char* _in1[1], uint32_t _in1Len[1], char* _in2[1], uint32_t _in2Len[1], char* _in3[1], uint32_t _in3Len[1], char* _in4[1], uint32_t _in4Len[1], uint32_t _in5[1], uint32_t _in6[1], uint32_t _in7[12], char* _in8[1], uint32_t _in8Len[1], char* _in9[1], uint32_t _in9Len[1]) {
   remote_arg _pra[8];
   uint32_t _primIn[21];
   remote_arg* _praIn;
   int _nErr = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _COPY(_primIn, 0, _in0Len, 0, 4);
   _praIn = (_pra + 1);
   _praIn[0].buf.pv = (void*) _in0[0];
   _praIn[0].buf.nLen = (8 * _in0Len[0]);
   _COPY(_primIn, 4, _in1Len, 0, 4);
   _praIn[1].buf.pv = (void*) _in1[0];
   _praIn[1].buf.nLen = (8 * _in1Len[0]);
   _COPY(_primIn, 8, _in2Len, 0, 4);
   _praIn[2].buf.pv = (void*) _in2[0];
   _praIn[2].buf.nLen = (4 * _in2Len[0]);
   _COPY(_primIn, 12, _in3Len, 0, 4);
   _praIn[3].buf.pv = (void*) _in3[0];
   _praIn[3].buf.nLen = (4 * _in3Len[0]);
   _COPY(_primIn, 16, _in4Len, 0, 4);
   _praIn[4].buf.pv = (void*) _in4[0];
   _praIn[4].buf.nLen = (48 * _in4Len[0]);
   _COPY(_primIn, 20, _in5, 0, 4);
   _COPY(_primIn, 24, _in6, 0, 4);
   _COPY(_primIn, 28, _in7, 0, 48);
   _COPY(_primIn, 76, _in8Len, 0, 4);
   _praIn[5].buf.pv = (void*) _in8[0];
   _praIn[5].buf.nLen = (16 * _in8Len[0]);
   _COPY(_primIn, 80, _in9Len, 0, 4);
   _praIn[6].buf.pv = (void*) _in9[0];
   _praIn[6].buf.nLen = (12 * _in9Len[0]);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 8, 0, 0, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasRemap_RunMT)(remote_handle64 _handle, const uint64* workerPtrs, int workerPtrsLen, const uint64* mapPtrs, int mapPtrsLen, const int32_t* srcFds, int srcFdsLen, const uint32_t* offsets, int offsetsLen, const FadasIface_FadasImgProps_t* srcProps, int srcPropsLen, int32_t dstFd, uint32_t dstLen, const FadasIface_FadasImgProps_t* dstProps, const FadasIface_FadasROI_t* dstROIs, int dstROIsLen, const FadasIface_FadasNormlzParams_t* normlz, int normlzLen) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 10;
   return _stub_method_7(_handle, _mid, (char**)&workerPtrs, (uint32_t*)&workerPtrsLen, (char**)&mapPtrs, (uint32_t*)&mapPtrsLen, (char**)&srcFds, (uint32_t*)&srcFdsLen, (char**)&offsets, (uint32_t*)&offsetsLen, (char**)&srcProps, (uint32_t*)&srcPropsLen, (uint32_t*)&dstFd, (uint32_t*)&dstLen, (uint32_t*)dstProps, (char**)&dstROIs, (uint32_t*)&dstROIsLen, (char**)&normlz, (uint32_t*)&normlzLen);
}
static __inline int _stub_method_8(remote_handle64 _handle, uint32_t _mid, uint32_t _in0[1], uint32_t _in1[1]) {
   remote_arg _pra[1];
   uint32_t _primIn[2];
   int _nErr = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _COPY(_primIn, 0, _in0, 0, 4);
   _COPY(_primIn, 4, _in1, 0, 4);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 0, 0, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_mmap)(remote_handle64 _handle, int32_t bufFd, uint32_t bufSize) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 11;
   return _stub_method_8(_handle, _mid, (uint32_t*)&bufFd, (uint32_t*)&bufSize);
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_munmap)(remote_handle64 _handle, int32_t bufFd, uint32_t bufSize) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 12;
   return _stub_method_8(_handle, _mid, (uint32_t*)&bufFd, (uint32_t*)&bufSize);
}
static __inline int _stub_method_9(remote_handle64 _handle, uint32_t _mid, uint32_t _in0[1], uint32_t _in1[1], uint32_t _in2[1], uint32_t _in3[1], uint32_t _in4[1]) {
   remote_arg _pra[1];
   uint32_t _primIn[5];
   int _nErr = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _COPY(_primIn, 0, _in0, 0, 4);
   _COPY(_primIn, 4, _in1, 0, 4);
   _COPY(_primIn, 8, _in2, 0, 4);
   _COPY(_primIn, 12, _in3, 0, 4);
   _COPY(_primIn, 16, _in4, 0, 4);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 0, 0, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasRegBuf)(remote_handle64 _handle, FadasIface_FadasBufType_e bufType, int32_t bufFd, uint32_t bufSize, uint32_t bufOffset, uint32_t batchSize) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 13;
   return _stub_method_9(_handle, _mid, (uint32_t*)&bufType, (uint32_t*)&bufFd, (uint32_t*)&bufSize, (uint32_t*)&bufOffset, (uint32_t*)&batchSize);
}
static __inline int _stub_method_10(remote_handle64 _handle, uint32_t _mid, uint32_t _in0[1], uint32_t _in1[1], uint32_t _in2[1], uint32_t _in3[1]) {
   remote_arg _pra[1];
   uint32_t _primIn[4];
   int _nErr = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _COPY(_primIn, 0, _in0, 0, 4);
   _COPY(_primIn, 4, _in1, 0, 4);
   _COPY(_primIn, 8, _in2, 0, 4);
   _COPY(_primIn, 12, _in3, 0, 4);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 0, 0, 0), _pra));
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_FadasDeregBuf)(remote_handle64 _handle, int32_t bufFd, uint32_t bufSize, uint32_t bufOffset, uint32_t batchSize) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 14;
   return _stub_method_10(_handle, _mid, (uint32_t*)&bufFd, (uint32_t*)&bufSize, (uint32_t*)&bufOffset, (uint32_t*)&batchSize);
}
static __inline int _stub_method_11(remote_handle64 _handle, uint32_t _mid, uint32_t _in0[3], uint32_t _in1[3], uint32_t _in2[3], uint32_t _in3[1], uint32_t _in4[1], uint32_t _in5[1], uint32_t _in6[1], uint32_t _in7[1], uint64_t _rout8[1]) {
   int _numIn[1];
   remote_arg _pra[2];
   uint32_t _primIn[14];
   uint64_t _primROut[1];
   int _nErr = 0;
   _numIn[0] = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _pra[(_numIn[0] + 1)].buf.pv = (void*)_primROut;
   _pra[(_numIn[0] + 1)].buf.nLen = sizeof(_primROut);
   _COPY(_primIn, 0, _in0, 0, 12);
   _COPY(_primIn, 12, _in1, 0, 12);
   _COPY(_primIn, 24, _in2, 0, 12);
   _COPY(_primIn, 36, _in3, 0, 4);
   _COPY(_primIn, 40, _in4, 0, 4);
   _COPY(_primIn, 44, _in5, 0, 4);
   _COPY(_primIn, 48, _in6, 0, 4);
   _COPY(_primIn, 52, _in7, 0, 4);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 1, 0, 0), _pra));
   _COPY(_rout8, 0, _primROut, 0, 8);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_PointPillarCreate)(remote_handle64 _handle, const FadasIface_Pt3D_t* pPlrSize, const FadasIface_Pt3D_t* pMinRange, const FadasIface_Pt3D_t* pMaxRange, uint32_t maxNumInPts, uint32_t numInFeatureDim, uint32_t maxNumPlrs, uint32_t maxNumPtsPerPlr, uint32_t numOutFeatureDim, uint64_t* phPreProc) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 15;
   return _stub_method_11(_handle, _mid, (uint32_t*)pPlrSize, (uint32_t*)pMinRange, (uint32_t*)pMaxRange, (uint32_t*)&maxNumInPts, (uint32_t*)&numInFeatureDim, (uint32_t*)&maxNumPlrs, (uint32_t*)&maxNumPtsPerPlr, (uint32_t*)&numOutFeatureDim, (uint64_t*)phPreProc);
}
static __inline int _stub_method_12(remote_handle64 _handle, uint32_t _mid, uint64_t _in0[1], uint32_t _in1[1], uint32_t _in2[1], uint32_t _in3[1], uint32_t _in4[1], uint32_t _in5[1], uint32_t _in6[1], uint32_t _in7[1], uint32_t _in8[1], uint32_t _in9[1], uint32_t _in10[1], uint32_t _rout11[1]) {
   int _numIn[1];
   remote_arg _pra[2];
   uint64_t _primIn[6];
   uint32_t _primROut[1];
   int _nErr = 0;
   _numIn[0] = 0;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _pra[(_numIn[0] + 1)].buf.pv = (void*)_primROut;
   _pra[(_numIn[0] + 1)].buf.nLen = sizeof(_primROut);
   _COPY(_primIn, 0, _in0, 0, 8);
   _COPY(_primIn, 8, _in1, 0, 4);
   _COPY(_primIn, 12, _in2, 0, 4);
   _COPY(_primIn, 16, _in3, 0, 4);
   _COPY(_primIn, 20, _in4, 0, 4);
   _COPY(_primIn, 24, _in5, 0, 4);
   _COPY(_primIn, 28, _in6, 0, 4);
   _COPY(_primIn, 32, _in7, 0, 4);
   _COPY(_primIn, 36, _in8, 0, 4);
   _COPY(_primIn, 40, _in9, 0, 4);
   _COPY(_primIn, 44, _in10, 0, 4);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 1, 1, 0, 0), _pra));
   _COPY(_rout11, 0, _primROut, 0, 4);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_PointPillarRun)(remote_handle64 _handle, uint64_t hPreProc, uint32_t numPts, int32_t fdInPts, uint32_t inPtsOffset, uint32_t inPtsSize, int32_t fdOutPlrs, uint32_t outPlrsOffset, uint32_t outPlrsSize, int32_t fdOutFeature, uint32_t outFeatureOffset, uint32_t outFeatureSize, uint32_t* pNumOutPlrs) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 16;
   return _stub_method_12(_handle, _mid, (uint64_t*)&hPreProc, (uint32_t*)&numPts, (uint32_t*)&fdInPts, (uint32_t*)&inPtsOffset, (uint32_t*)&inPtsSize, (uint32_t*)&fdOutPlrs, (uint32_t*)&outPlrsOffset, (uint32_t*)&outPlrsSize, (uint32_t*)&fdOutFeature, (uint32_t*)&outFeatureOffset, (uint32_t*)&outFeatureSize, (uint32_t*)pNumOutPlrs);
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_PointPillarDestroy)(remote_handle64 _handle, uint64_t hPreProc) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 17;
   return _stub_method_5(_handle, _mid, (uint64_t*)&hPreProc);
}
static __inline int _stub_method_13(remote_handle64 _handle, uint32_t _mid, uint32_t _in0[1], uint32_t _in1[1], uint32_t _in2[1], uint32_t _in3[1], uint32_t _in4[6], float _in5[1], float _in6[1], float _in7[1], float _in8[1], float _in9[1], float _in10[1], float _in11[1], float _in12[1], char* _in13[1], uint32_t _in13Len[1], uint32_t _in14[1], uint64_t _rout15[1]) {
   int _numIn[1];
   remote_arg _pra[3];
   uint32_t _primIn[20];
   uint64_t _primROut[1];
   remote_arg* _praIn;
   int _nErr = 0;
   _numIn[0] = 1;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _pra[(_numIn[0] + 1)].buf.pv = (void*)_primROut;
   _pra[(_numIn[0] + 1)].buf.nLen = sizeof(_primROut);
   _COPY(_primIn, 0, _in0, 0, 4);
   _COPY(_primIn, 4, _in1, 0, 4);
   _COPY(_primIn, 8, _in2, 0, 4);
   _COPY(_primIn, 12, _in3, 0, 4);
   _COPY(_primIn, 16, _in4, 0, 24);
   _COPY(_primIn, 40, _in5, 0, 4);
   _COPY(_primIn, 44, _in6, 0, 4);
   _COPY(_primIn, 48, _in7, 0, 4);
   _COPY(_primIn, 52, _in8, 0, 4);
   _COPY(_primIn, 56, _in9, 0, 4);
   _COPY(_primIn, 60, _in10, 0, 4);
   _COPY(_primIn, 64, _in11, 0, 4);
   _COPY(_primIn, 68, _in12, 0, 4);
   _COPY(_primIn, 72, _in13Len, 0, 4);
   _praIn = (_pra + 1);
   _praIn[0].buf.pv = (void*) _in13[0];
   _praIn[0].buf.nLen = (1 * _in13Len[0]);
   _COPY(_primIn, 76, _in14, 0, 4);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 2, 1, 0, 0), _pra));
   _COPY(_rout15, 0, _primROut, 0, 8);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_ExtractBBoxCreate)(remote_handle64 _handle, uint32_t maxNumInPts, uint32_t numInFeatureDim, uint32_t maxNumDetOut, uint32_t numClass, const FadasIface_Grid2D_t* pGrid, float threshScore, float threshIOU, float minCentreX, float minCentreY, float minCentreZ, float maxCentreX, float maxCentreY, float maxCentreZ, const uint8_t* labelSelect, int labelSelectLen, uint32_t maxNumFilter, uint64_t* phPostProc) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 18;
   return _stub_method_13(_handle, _mid, (uint32_t*)&maxNumInPts, (uint32_t*)&numInFeatureDim, (uint32_t*)&maxNumDetOut, (uint32_t*)&numClass, (uint32_t*)pGrid, (float*)&threshScore, (float*)&threshIOU, (float*)&minCentreX, (float*)&minCentreY, (float*)&minCentreZ, (float*)&maxCentreX, (float*)&maxCentreY, (float*)&maxCentreZ, (char**)&labelSelect, (uint32_t*)&labelSelectLen, (uint32_t*)&maxNumFilter, (uint64_t*)phPostProc);
}
static __inline int _stub_method_14(remote_handle64 _handle, uint32_t _mid, uint64_t _in0[1], uint32_t _in1[1], char* _in2[1], uint32_t _in2Len[1], char* _in3[1], uint32_t _in3Len[1], char* _in4[1], uint32_t _in4Len[1], uint8_t _in5[1], uint8_t _in6[1], uint32_t _rout7[1]) {
   int _numIn[1];
   remote_arg _pra[5];
   uint64_t _primIn[4];
   uint32_t _primROut[1];
   remote_arg* _praIn;
   int _nErr = 0;
   _numIn[0] = 3;
   _pra[0].buf.pv = (void*)_primIn;
   _pra[0].buf.nLen = sizeof(_primIn);
   _pra[(_numIn[0] + 1)].buf.pv = (void*)_primROut;
   _pra[(_numIn[0] + 1)].buf.nLen = sizeof(_primROut);
   _COPY(_primIn, 0, _in0, 0, 8);
   _COPY(_primIn, 8, _in1, 0, 4);
   _COPY(_primIn, 12, _in2Len, 0, 4);
   _praIn = (_pra + 1);
   _praIn[0].buf.pv = (void*) _in2[0];
   _praIn[0].buf.nLen = (4 * _in2Len[0]);
   _COPY(_primIn, 16, _in3Len, 0, 4);
   _praIn[1].buf.pv = (void*) _in3[0];
   _praIn[1].buf.nLen = (4 * _in3Len[0]);
   _COPY(_primIn, 20, _in4Len, 0, 4);
   _praIn[2].buf.pv = (void*) _in4[0];
   _praIn[2].buf.nLen = (4 * _in4Len[0]);
   _COPY(_primIn, 24, _in5, 0, 1);
   _COPY(_primIn, 25, _in6, 0, 1);
   _TRY(_nErr, __QAIC_REMOTE(remote_handle64_invoke)(_handle, REMOTE_SCALARS_MAKEX(0, _mid, 4, 1, 0, 0), _pra));
   _COPY(_rout7, 0, _primROut, 0, 4);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_ExtractBBoxRun)(remote_handle64 _handle, uint64_t hPostProc, uint32_t numPts, const int32_t* fds, int fdsLen, const uint32_t* offsets, int offsetsLen, const uint32_t* sizes, int sizesLen, uint8_t bMapPtsToBBox, uint8_t bBBoxFilter, uint32_t* pNumDetOut) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 19;
   return _stub_method_14(_handle, _mid, (uint64_t*)&hPostProc, (uint32_t*)&numPts, (char**)&fds, (uint32_t*)&fdsLen, (char**)&offsets, (uint32_t*)&offsetsLen, (char**)&sizes, (uint32_t*)&sizesLen, (uint8_t*)&bMapPtsToBBox, (uint8_t*)&bBBoxFilter, (uint32_t*)pNumDetOut);
}
__QAIC_STUB_EXPORT AEEResult __QAIC_STUB(FadasIface_ExtractBBoxDestroy)(remote_handle64 _handle, uint64_t hPostProc) __QAIC_STUB_ATTRIBUTE {
   uint32_t _mid = 20;
   return _stub_method_5(_handle, _mid, (uint64_t*)&hPostProc);
}
#ifdef __cplusplus
}
#endif
#endif //_FADASIFACE_STUB_H
