//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <stdio.h>
#include <unistd.h>

#include <netdb.h> 
#include <errno.h>

#include "tcp_sink.h"

//---------------------------------------------------------------------------------------
CTcpSink::CTcpSink(int socket, size_t bufferSize, bool backReadEnabled)
    : m_socket(socket)
    , m_buffer(bufferSize)
    , m_needDelete(false)
    , m_backReadEnabled(backReadEnabled)
    , m_errorPrinted(false)
{

}


//---------------------------------------------------------------------------------------
void CTcpSink::close()
{
    if( m_socket )
    {
        ::close(m_socket);
        m_socket = 0;
    }
}

//---------------------------------------------------------------------------------------------------
void CTcpSink::markForDelete()
{
    m_needDelete = true;
}

//---------------------------------------------------------------------------------------------------
void CTcpSink::putData(const char* buf, size_t len)
{
    m_buffer.put(buf, len);
    doWrite();
}

//---------------------------------------------------------------------------------------------------
void CTcpSink::putData(std::vector<char> data)
{
    putData( &data[0], data.size() );
}


//---------------------------------------------------------------------------------------------------
void CTcpSink::doWrite()
{
    if( m_needDelete )
    {
        //printf("in doWrite need Del signalled (%d)\n", m_socket);
        return;
    }

    size_t len = m_buffer.bytesAvailable();
    if( len == 0 )
        return;

    std::vector<char> tmpBuf(len);
    m_buffer.peek(tmpBuf, len);
    int written = ::send(m_socket, &tmpBuf[0], len, MSG_NOSIGNAL);
    if( written < 0 )
    {
        if( !m_errorPrinted )
        {
            printf("Sink socket (%d) got error: %d", m_socket, errno);fflush(stdout);
            m_errorPrinted = true;
        }

        if( errno != EWOULDBLOCK )
        {
            // some error happen
            m_needDelete = true;
        }
        return;
    }

    m_buffer.consume( written );
    m_errorPrinted = false;

}

//---------------------------------------------------------------------------------------------------
bool CTcpSink::isWritePending() const
{
    return m_buffer.bytesAvailable() > 0;
}

