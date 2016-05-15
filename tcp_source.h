#pragma once

//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <string>
#include <netinet/in.h>

#include "buffer.h"

//---------------------------------------------------------------------------------------
class CTcpSource
{
public:
    CTcpSource(size_t bufferSize, std::string const& address, unsigned port);
    void connectSync();
    bool isConnected() const { return m_isConnected; }
    int getSocket() const { return m_socket; }
    bool checkConnected();
    int calcMaxFd(int incomingMaxFd) const;

    void putData(const char* buf, size_t len);
    bool isWritePending() const;
    void doWrite();
    unsigned doRead(char* buf, size_t maxLen);
    void fillWrSet(fd_set& set) const;

    std::string printableAddress() const;
    bool isIdentifierPresent() const { return m_identifierPresent; }
    void fillIdentifier( std::vector<char>& data ) const;
private:
    int m_socket;
    std::string m_address;
    unsigned m_port;
    bool m_isConnected;
    CBuffer m_identifierBuffer;
    bool m_identifierPresent;
    CBuffer m_buffer;
   

};

