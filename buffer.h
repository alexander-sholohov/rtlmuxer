#pragma once
//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <stdlib.h>

#include <vector>

class CBuffer
{
public:
    CBuffer(size_t bufferSize);
    void put(const char* buf, size_t len);
    size_t bytesAvailable() const;
    void peek(char* buf, size_t len) const;
    void peek(std::vector<char> & buffer, size_t len) const;
    void consume(size_t len);
    void reset();
private:
    std::vector<char> m_buffer;
    size_t m_head;
    size_t m_tail;

};
