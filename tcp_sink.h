#pragma once

//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <vector>
#include "buffer.h"

//---------------------------------------------------------------------------------------
class CTcpSink
{
public:
    CTcpSink(int socket, size_t bufferSize, bool backReadEnabled);
    void putData(const char* buf, size_t len);
    void putData(std::vector<char> data);
    void doWrite();
    int getSocket() const { return m_socket; }
    bool isWritePending() const;
    void markForDelete();
    bool isNeedDelete() const { return m_needDelete;}
    bool isBackReadEnabled() const { return m_backReadEnabled; }

    void close();

private:
    int m_socket;    
    CBuffer m_buffer;
    bool m_needDelete;
    bool m_backReadEnabled;
    bool m_errorPrinted;


};


