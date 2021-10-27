/*
 *  TCP/IP or UDP/IP networking functions
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "net_alt.h"
#include "wm_include.h"

//#include "lwip/netdb.h"

#define MSVC_INT_CAST

/*
 * Prepare for using the sockets interface
 */
static int net_prepare( void )
{
    return( 0 );
}

/*
 * Initialize a context
 */
void mbedtls_net_init( mbedtls_net_context *ctx )
{
    ctx->fd = -1;
}

/*
 * Initiate a TCP connection with host:port and the given protocol
 */
int mbedtls_net_connect( mbedtls_net_context *ctx, const char *host, const char *port, int proto )
{
    return( -1 );
}

/*
 * Create a listening socket on bind_ip:port
 */
int mbedtls_net_bind( mbedtls_net_context *ctx, const char *bind_ip, const char *port, int proto )
{
    return( -1 );
}

/*
 * Check if the requested operation would be blocking on a non-blocking socket
 * and thus 'failed' with a negative return value.
 *
 * Note: on a blocking socket this function always returns 0!
 */
static int net_would_block( const mbedtls_net_context *ctx )
{
    /*
     * Never return 'WOULD BLOCK' on a non-blocking socket
     */
//    if( ( fcntl( ctx->fd, F_GETFL ) & O_NONBLOCK ) != O_NONBLOCK )
//        return( 0 );

//    switch( errno )
//    {
//#if defined EAGAIN
//        case EAGAIN:
//#endif
//#if defined EWOULDBLOCK && EWOULDBLOCK != EAGAIN
//        case EWOULDBLOCK:
//#endif
//            return( 1 );
//    }
//    return( 0 );

    if( ( fcntl( ctx->fd, F_GETFL, 0) & O_NONBLOCK ) != O_NONBLOCK )
        return( 0 );
    return( 1 );

}

/*
 * Accept a connection from a remote client
 */
int mbedtls_net_accept( mbedtls_net_context *bind_ctx,
                        mbedtls_net_context *client_ctx,
                        void *client_ip, size_t buf_size, size_t *ip_len )
{
    return( -1 );
}

/*
 * Set the socket blocking or non-blocking
 */
/*
int mbedtls_net_set_block( mbedtls_net_context *ctx )
{
#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) &&
   !defined(EFI32)
   u_long n = 0;
   return( ioctlsocket( ctx->fd, FIONBIO, &n ) );
#else
   return( fcntl( ctx->fd, F_SETFL, fcntl( ctx->fd, F_GETFL ) & ~O_NONBLOCK ) );
#endif

}

int mbedtls_net_set_nonblock( mbedtls_net_context *ctx )
{
#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) &&
   !defined(EFI32)
   u_long n = 1;
   return( ioctlsocket( ctx->fd, FIONBIO, &n ) );
#else
   return( fcntl( ctx->fd, F_SETFL, fcntl( ctx->fd, F_GETFL ) | O_NONBLOCK ) );
#endif
}
*/

/*
 * Set the socket blocking or non-blocking
 */
int net_set_block(int fd)
{
    return (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK));
}

int net_set_nonblock(int fd)
{
    return (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK));
}

/*
 * Portable usleep helper
 */
void mbedtls_net_usleep( unsigned long usec )
{
    struct timeval tv;
    tv.tv_sec  = usec / 1000000;
    tv.tv_usec = usec % 1000000;
    select( 0, NULL, NULL, NULL, &tv );
}

/*
 * Read at most 'len' characters
 */
int mbedtls_net_recv( void *ctx, unsigned char *buf, size_t len )
{
    return( -1 );
}

/*
 * Read at most 'len' characters, blocking for at most 'timeout' ms
 */
int mbedtls_net_recv_timeout( void *ctx, unsigned char *buf, size_t len,
                      uint32_t timeout )
{
    return( -1 );
}

/*
 * Write at most 'len' characters
 */
int mbedtls_net_send( void *ctx, const unsigned char *buf, size_t len )
{
    return( -1 );
}

/*
 * Gracefully close the connection
 */
void mbedtls_net_free( mbedtls_net_context *ctx )
{
    if( ctx->fd == -1 )
        return;

    closesocket( ctx->fd );
    ctx->fd = -1;
}


