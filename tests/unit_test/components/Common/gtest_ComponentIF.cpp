// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "gtest/gtest.h"
#include <stdio.h>

#include "QC/component/ComponentIF.hpp"

using namespace QC;
using namespace QC::component;

typedef struct
{
    int a;
} ComponentIFTest_Config_t;

class ComponentIFTest : public ComponentIF
{
public:
    ComponentIFTest() {}
    ~ComponentIFTest() {}
    QCStatus_e Init( const char *pName, const ComponentIFTest_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR )
    {
        QCStatus_e ret = QC_STATUS_OK;

        ret = ComponentIF::Init( pName, level );
        if ( QC_STATUS_OK == ret )
        {
            // DO real initialize using pConfig.
        }

        if ( QC_STATUS_OK == ret )
        {
            m_state = QC_OBJECT_STATE_READY;
        }

        return ret;
    }

    QCStatus_e Start()
    {
        QCStatus_e ret = QC_STATUS_OK;

        if ( QC_OBJECT_STATE_READY != m_state )
        {
            ret = QC_STATUS_BAD_STATE;
        }

        if ( QC_STATUS_OK == ret )
        {
            // DO start
        }

        if ( QC_STATUS_OK == ret )
        {
            m_state = QC_OBJECT_STATE_RUNNING;
        }

        return ret;
    }


    QCStatus_e Stop()
    {
        QCStatus_e ret = QC_STATUS_OK;

        if ( QC_OBJECT_STATE_RUNNING != m_state )
        {
            ret = QC_STATUS_BAD_STATE;
        }

        if ( QC_STATUS_OK == ret )
        {
            // DO stop
        }

        if ( QC_STATUS_OK == ret )
        {
            m_state = QC_OBJECT_STATE_READY;
        }

        return ret;
    }

    QCStatus_e Deinit()
    {
        QCStatus_e ret = QC_STATUS_OK;

        if ( QC_OBJECT_STATE_READY != m_state )
        {
            ret = QC_STATUS_BAD_STATE;
        }

        if ( QC_STATUS_OK == ret )
        {
            // DO deinit
        }

        if ( QC_STATUS_OK == ret )
        {
            ret = ComponentIF::Deinit();
        }

        return ret;
    }

    QCStatus_e DeinitIF() { return ComponentIF::Deinit(); }
};

TEST( ComponentIF, SANITY_ComponentIF )
{
    ComponentIFTest cifTest;
    ComponentIFTest_Config_t config = { 10 };
    QCStatus_e ret;

    for ( int i = 0; i < 3; i++ )
    {
        ASSERT_EQ( QC_OBJECT_STATE_INITIAL, cifTest.GetState() );

        ret = cifTest.Init( "TEST", &config );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ASSERT_EQ( QC_OBJECT_STATE_READY, cifTest.GetState() );
        ASSERT_EQ( std::string( "TEST" ), std::string( cifTest.GetName() ) );

        ret = cifTest.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ASSERT_EQ( QC_OBJECT_STATE_RUNNING, cifTest.GetState() );

        ret = cifTest.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ASSERT_EQ( QC_OBJECT_STATE_READY, cifTest.GetState() );

        ret = cifTest.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ASSERT_EQ( QC_OBJECT_STATE_INITIAL, cifTest.GetState() );
    }
}

TEST( ComponentIF, L2_ComponentIF )
{
    {
        ComponentIFTest cifTest;
        ComponentIFTest_Config_t config = { 10 };
        QCStatus_e ret;

        ret = cifTest.Init( "TEST", &config );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = cifTest.Init( "TEST", &config );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = cifTest.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = cifTest.DeinitIF();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    {
        ComponentIFTest cifTest;
        ComponentIFTest_Config_t config = { 10 };
        QCStatus_e ret;

        ret = cifTest.Init( nullptr, &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    }

    {
        ComponentIFTest cifTest;
        ComponentIFTest_Config_t config = { 10 };
        QCStatus_e ret;

        ret = cifTest.Init( "TEST_ERROR", &config, LOGGER_LEVEL_MAX );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = cifTest.DeinitIF();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    {
        ComponentIFTest cifTest;
        ComponentIFTest cifTest1;
        ComponentIFTest_Config_t config = { 10 };
        QCStatus_e ret;

        /* generally OK that 2 component has the same name but it was not suggested */
        ret = cifTest.Init( "TEST", &config );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = cifTest1.Init( "TEST", &config );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif