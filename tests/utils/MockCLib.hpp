// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef MOCK_C_LIB_HPP
#define MOCK_C_LIB_HPP

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void MockC_MallocCtrlSize( size_t size );
void MockC_MallocCtrl( int whenToReturnNull );

void MockC_CallocCtrlSize( size_t size );
void MockC_CallocCtrl( int whenToReturnNull );

void MockC_FreadCtrlSize( size_t size );
#endif /* MOCK_C_LIB_HPP */
