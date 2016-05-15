#pragma once

//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <stdlib.h>
#include <vector>

#include "tcp_sink.h"


//---------------------------------------------------------------------------------------
class CSinkList
{
public:
    CSinkList() {}
    void addSink(int socket, size_t bufferSize, bool backReadEnabled);
    void putData(const char* buf, size_t len);
    void fillRdSet(fd_set& set) const;
    void fillWrSet(fd_set& set) const;
    void processRead(fd_set& set, CBuffer& backBuffer);
    void processWrite(fd_set& set);
    void cleanupTask();
    int calcMaxFd(int incomingMaxFd) const;
    CTcpSink& getLastSink();

private:
    std::vector<CTcpSink> m_items;

};
