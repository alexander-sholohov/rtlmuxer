//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//
#include <memory.h>

#include "buffer.h"

//---------------------------------------------------------------------------------------------------
CBuffer::CBuffer(size_t bufferSize)
{
    m_buffer.resize(bufferSize + 16); 
    m_head = 0;
    m_tail = 0;
}

//---------------------------------------------------------------------------------------------------
void CBuffer::consume(size_t len)
{
    if( bytesAvailable() < len )
    {
        // never happen but for some reason
        m_head = 0;
        m_tail = 0;
        return;
    }

    m_tail += len;
    if( m_tail >= m_buffer.size() )
    {
        m_tail -= m_buffer.size();
    }
}

//---------------------------------------------------------------------------------------------------
void CBuffer::peek(char* buf, size_t len) const
{

    if( m_tail + len < m_buffer.size() )
    {
        memcpy(buf, &m_buffer[m_tail], len);
    }
    else
    {
        size_t len1 = m_buffer.size() - m_tail;
        size_t len2 = len - len1;
        memcpy(buf, &m_buffer[m_tail], len1);
        memcpy(buf+len1, &m_buffer[0], len2);
    }
    

}

//---------------------------------------------------------------------------------------------------
void CBuffer::peek(std::vector<char> & buffer, size_t len) const
{
    size_t len2 = (len > buffer.size())? buffer.size() : len; // limit length for some reason
    peek(&buffer[0], len2);
}

//---------------------------------------------------------------------------------------------------
void CBuffer::put(const char* buf, size_t len)
{
    // ignore huge data (should never happen)
    if( len >= m_buffer.size())
        return;

    size_t bytesAvailableBefore = bytesAvailable();

    if( m_head + len < m_buffer.size() )
    {
        memcpy(&m_buffer[m_head], buf, len);
        m_head += len;
    }
    else
    {
        size_t len1 = m_buffer.size() - m_head;
        size_t len2 = len - len1;
        memcpy(&m_buffer[m_head], buf, len1);
        memcpy(&m_buffer[0], buf+len1, len2);
        m_head = len2;
    }


    // check buffer overflow
    if( bytesAvailableBefore + len >= m_buffer.size() - 1 )
    {
        m_tail = m_head + 1;
        if( m_tail == m_buffer.size() )
            m_tail = 0;
    }

}


//---------------------------------------------------------------------------------------------------
size_t CBuffer::bytesAvailable() const
{
    int s = m_head - m_tail; 
    if( s < 0 )
    {
        s += m_buffer.size();
    }
    return s;
}

//---------------------------------------------------------------------------------------------------
void CBuffer::reset()
{
    m_head = 0;
    m_tail = 0;
}
