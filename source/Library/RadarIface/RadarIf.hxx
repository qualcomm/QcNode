// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_RADAR_EXT_IFACE_HPP
#define QC_RADAR_EXT_IFACE_HPP

#include <devctl.h>
#include <fcntl.h>
#include <iostream>
#include <sys/iofunc.h>
#include <sys/neutrino.h>
#include <unistd.h>

// For debug
#include <cstring>

namespace q
{
namespace interface
{

constexpr int DCMD_EXECUTE_CMD_IOV = __DIOT( _DCMD_MISC, 2, 0xFFu );

class Radar
{
public:
    Radar( const char *device ) { fd = open( device, O_RDWR ); }

    bool is_open() const { return fd != -1; }

    ~Radar() { close( fd ); }

    int execute( uint8_t *pinput, size_t input_size, uint8_t *poutput, size_t output_size )
    {
        iov_t tx;
        iov_t rx;
        SETIOV( &tx, pinput, input_size );
        SETIOV( &rx, poutput, output_size );
        int status = devctlv( fd, DCMD_EXECUTE_CMD_IOV, 1, 1, &tx, &rx, NULL );
        if ( status != EOK )
        {
            return status;
        }
        else
        {
            return EOK;
        }
    }

private:
    int fd;
};


}   // namespace interface
}   // namespace q

#endif   // QC_RADAR_EXT_IFACE_HPP