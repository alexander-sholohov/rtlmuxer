//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <stdio.h>
#include <unistd.h>

#include <netdb.h> 

#include <cstring>
#include <errno.h>
#include <sstream>
#include <errno.h>

#include "tcp_source.h"


#define IDENTIFIER_SIZE (12)

//---------------------------------------------------------------------------------------
CTcpSource::CTcpSource(size_t bufferSize, std::string const& address, unsigned port)
    : m_socket(0)
    , m_address( address )
    , m_port( port )
    , m_isConnected(false)
    , m_identifierBuffer(IDENTIFIER_SIZE)
    , m_identifierPresent(false)
    , m_buffer(bufferSize)
{
}

//---------------------------------------------------------------------------------------
int CTcpSource::calcMaxFd(int incomingMaxFd) const
{
    return (m_socket > incomingMaxFd)? m_socket : incomingMaxFd;
}

//---------------------------------------------------------------------------------------
bool CTcpSource::checkConnected()
{
    int sock_result;
    socklen_t result_len = sizeof(sock_result);
    if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &sock_result, &result_len) < 0) {
        // error, fail somehow, close socket
        return false;
    }

    return sock_result == 0;
}

//---------------------------------------------------------------------------------------
void CTcpSource::connectSync()
{
    m_isConnected = false;

    m_socket = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);   
    if (m_socket < 0) 
    {
        fprintf(stderr,"ERROR opening socket\n");
        return;
    }

    int rc;

    hostent *server;
    server = gethostbyname(m_address.c_str());
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return;
    }

    struct sockaddr_in serv_addr;

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(m_port);

    // connection procedure
    // http://stackoverflow.com/questions/10187347/async-connect-and-disconnect-with-epoll-linux/10194883#10194883

    rc = connect(m_socket,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    if (rc < 0 && errno != EINPROGRESS) {
        // error, fail somehow, close socket
        fprintf(stderr,"ERROR connecting\n");
        return;
    }

    if (rc == 0) {
        // connection has succeeded immediately
        m_isConnected = true;
        return;
    }
    
    // connection attempt is in progress


    fd_set wr_set;
    FD_ZERO(&wr_set);
    FD_SET(m_socket, &wr_set);


    struct timeval       timeout;
    timeout.tv_sec  = 20;
    timeout.tv_usec = 0;    
    rc = ::select(m_socket + 1, NULL, &wr_set, NULL, &timeout);


    int sock_result;
    socklen_t result_len = sizeof(sock_result);
    if (getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &sock_result, &result_len) < 0) {
        // error, fail somehow, close socket
        close(m_socket);
        return;
    }

    if (sock_result != 0) {
        // connection failed; error code is in 'result'
        close(m_socket);
        return;
    }

    struct linger ling = {1,0};
    setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));


    m_identifierPresent = false;
    m_identifierBuffer.reset();

    m_isConnected = true;

}



//---------------------------------------------------------------------------------------
void CTcpSource::putData(const char* buf, size_t len)
{
    m_buffer.put(buf, len);
    doWrite();
}

//---------------------------------------------------------------------------------------
void CTcpSource::doWrite()
{

    printf("in src doWrite (%d)\n", m_socket);
    size_t len = m_buffer.bytesAvailable();
    if( len == 0 )
        return;


    std::vector<char> tmpBuf(len);
    m_buffer.peek(&tmpBuf[0], len);

    // TODO: remove
    printf("Data to write: ");
    for( size_t i=0; i<len; i++ )
    {
        printf("%02X ", tmpBuf[i]);
    }
    printf("\n");fflush(stdout);


    int written = ::send(m_socket, &tmpBuf[0], len, MSG_NOSIGNAL);
    if( written < 0 )
    {
        if( errno != EWOULDBLOCK )
        {
            // some error happen
            printf("src socket returns error on write. close.\n");fflush(stdout);
            close( m_socket );
            m_isConnected = false;
        }
        return;
    }

    m_buffer.consume( written );

}

//---------------------------------------------------------------------------------------
bool CTcpSource::isWritePending() const
{
    return m_buffer.bytesAvailable() > 0;
}

//---------------------------------------------------------------------------------------
unsigned CTcpSource::doRead(char* buf, size_t maxLen)
{
    int readed = ::read(m_socket, buf, maxLen );
    
    if( readed <= 0 )
    {
        printf("readed = %d, close src socket.\n", readed);
        m_isConnected = false;
        close(m_socket);
        return 0;
    }

    // read rls_tcp specific identifier
    if( !m_identifierPresent && IDENTIFIER_SIZE > 0 )
    {
        int available =  IDENTIFIER_SIZE - m_identifierBuffer.bytesAvailable();
        int len = (readed > available) ? available : readed;
        m_identifierBuffer.put( buf, len );
        if( m_identifierBuffer.bytesAvailable() == IDENTIFIER_SIZE )
            m_identifierPresent = true;
    }

    return readed;
}

//---------------------------------------------------------------------------------------
void CTcpSource::fillIdentifier( std::vector<char>& data ) const
{
    data.resize(IDENTIFIER_SIZE);
    m_identifierBuffer.peek(data, IDENTIFIER_SIZE);
}

//---------------------------------------------------------------------------------------
void CTcpSource::fillWrSet(fd_set& set) const
{
    if( isWritePending() )
    {
        FD_SET(m_socket, &set);
    }

}

//---------------------------------------------------------------------------------------
std::string CTcpSource::printableAddress() const
{
    std::ostringstream out;
    out << m_address;
    out << ":";
    out << m_port;
    return out.str();
}
