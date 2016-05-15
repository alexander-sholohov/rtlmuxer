//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <stdio.h>
#include <unistd.h>
#include <netdb.h> 
#include <assert.h>



#include "sink_list.h"

//---------------------------------------------------------------------------------------------------
void CSinkList::addSink(int socket, size_t bufferSize, bool backReadEnabled)
{
    CTcpSink item(socket, bufferSize, backReadEnabled);
    m_items.push_back(item);
}

//---------------------------------------------------------------------------------------------------
CTcpSink& CSinkList::getLastSink()
{
    // we never call this method with empty list
    assert(m_items.size() > 0);
    return m_items[m_items.size() - 1];
}

//---------------------------------------------------------------------------------------------------
void CSinkList::putData(const char* buf, size_t len)
{
    for( std::vector<CTcpSink>::iterator it=m_items.begin(); it!=m_items.end(); it++ )
    {
        CTcpSink& item = *it;
        if( !item.isNeedDelete() )
        {
        	item.putData(buf, len);
        }
    }
}


//---------------------------------------------------------------------------------------------------
void CSinkList::fillRdSet(fd_set& set) const
{
    for( std::vector<CTcpSink>::const_iterator it=m_items.begin(); it!=m_items.end(); it++ )
    {
        CTcpSink const& item = *it;
        if( !item.isNeedDelete() )
        {
        	FD_SET( item.getSocket(), &set);
        }
    }

}

//---------------------------------------------------------------------------------------------------
void CSinkList::fillWrSet(fd_set& set) const
{
    for( std::vector<CTcpSink>::const_iterator it=m_items.begin(); it!=m_items.end(); it++ )
    {
        CTcpSink const& item = *it;
        if( !item.isNeedDelete() && item.isWritePending() )
        {
            FD_SET( item.getSocket(), &set);
        }
    }

}

//---------------------------------------------------------------------------------------------------
void CSinkList::processRead(fd_set& set, CBuffer& backBuffer)
{
    for( std::vector<CTcpSink>::iterator it=m_items.begin(); it!=m_items.end(); it++ )
    {
        CTcpSink& item = *it;
        if( FD_ISSET(item.getSocket(), &set)) 
        {
            char buf[8192];
            int readed = ::read(item.getSocket(), buf, sizeof(buf));
            if( readed <= 0 )
            {
                item.markForDelete();
            }
            else
            {
                // process read
                if( item.isBackReadEnabled() )
                {
                	backBuffer.put(buf, readed);
            	}
            }

        }
        
    }

}


//---------------------------------------------------------------------------------------------------
void CSinkList::processWrite(fd_set& set)
{
    for( std::vector<CTcpSink>::iterator it=m_items.begin(); it!=m_items.end(); it++ )
    {
        CTcpSink& item = *it;
        if( FD_ISSET(item.getSocket(), &set) && !item.isNeedDelete() ) 
        {
            item.doWrite();
        }
    }
}


//---------------------------------------------------------------------------------------------------
void CSinkList::cleanupTask()
{
    for( size_t i=0; i<m_items.size();  )
    {
        CTcpSink& item = m_items[i];
        if( item.isNeedDelete() ) 
        {
            printf("will del sink %d\n", item.getSocket());
            item.close();
            m_items.erase( m_items.begin() + i );
        }
        else
        {
            i++;
        }
    }

}


//---------------------------------------------------------------------------------------------------
int CSinkList::calcMaxFd(int incomingMaxFd) const
{
    int res = incomingMaxFd;
    for( std::vector<CTcpSink>::const_iterator it=m_items.begin(); it!=m_items.end(); it++ )
    {
        CTcpSink const& item = *it;
        if( item.getSocket() > res )
        {
            res = item.getSocket();
        }
    }

    return res;
}
