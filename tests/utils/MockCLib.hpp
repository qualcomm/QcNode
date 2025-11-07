// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef MOCK_C_LIB_HPP
#define MOCK_C_LIB_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void MockC_MallocCtrlSize( size_t size );
void MockC_MallocCtrl( int whenToReturnNull );

void MockC_CallocCtrlSize( size_t size );
void MockC_CallocCtrl( int whenToReturnNull );
#endif /* MOCK_C_LIB_HPP */
