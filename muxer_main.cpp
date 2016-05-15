//
// Author: Alexander Sholohov <ra9yer@yahoo.com>
//
// License: MIT
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>


#include <string>
#include <vector>
#include <cstring>
#include <errno.h>
#include <getopt.h>


#include "buffer.h"
#include "tcp_sink.h"
#include "sink_list.h"
#include "tcp_source.h"


#define BASIC_BUFFER_SIZE (128*1024)
#define OUT_BUFFER_SIZE (BASIC_BUFFER_SIZE * 16)
#define BACK_BUFFER_SIZE (8192)



//-------------------------------------------------------------------
void close_and_exit(int status)
{
    fprintf(stderr, "exit status = %d\n", status);
    exit(status);
}

//-------------------------------------------------------------------
int prepareServerSocket(std::string const& addr, int port)
{
    struct sockaddr_in local;

    int optval, server;


    /* Setup server addr & port */
    memset(&local,0,sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = (addr == "0.0.0.0")? INADDR_ANY : inet_addr(addr.c_str());


    server = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (server == -1) {
        perror("socket");
        close_and_exit(EXIT_FAILURE);
    }

    optval = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    setsockopt(server, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int));

    //struct linger ling = {1,0};
    //setsockopt(server, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
    
    /* Bind and listen */
    if (bind(server, (struct sockaddr *)&local, sizeof(local))) 
    {
        perror("bind");
        close_and_exit(EXIT_FAILURE);
    }

    if (listen(server, 0)) 
    {
        perror("listen");
        close_and_exit(EXIT_FAILURE);
    }

    return server;

}


//-------------------------------------------------------------------
void prepareAcceptedSocket(int socket)
{
    struct linger ling = {1,0};
    setsockopt(socket, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
}

//-------------------------------------------------------------------
void usage(void)
{
    printf("rtlmuxer, simple rtl_tcp specific TCP stream splitter\n\n"
        "Usage:\t[--src-address= address to connect to (default: 127.0.0.1)]\n"
        "\t[--src-port= port to connect to (default: 1234)]\n"
        "\t[--sink-bind-address= sink listen address (default: 127.0.0.1)]\n"
        "\t[--sink-bind-port-a= sink listen port A (default: 7373)]\n"
        "\t[--sink-bind-port-b= sink listen port B (default: 7374)]\n"
        "\t[--help this text]\n");
    exit(1);
}



//-------------------------------------------------------------------
int main(int argc, char** argv)
{

    std::string srcAddress="127.0.0.1";
    unsigned srcPort = 1234;
    std::string sinkBindAddressA = "127.0.0.1";
    unsigned skinkBindPortA = 7373;
    std::string sinkBindAddressB = "127.0.0.1";
    unsigned skinkBindPortB = 7374;


    static struct option long_options[] = {
        {"help", 0, 0, 0},
        {"src-address", 1, 0, 0},
        {"src-port", 1, 0, 0},
        {"sink-bind-address", 1, 0, 0},
        {"sink-bind-port-a", 1, 0, 0},
        {"sink-bind-port-b", 1, 0, 0},
        {0, 0, 0, 0}
    };

    while( true ) 
    {
        int option_index = 0;

        int c = getopt_long (argc, argv, "", long_options, &option_index);

        if( c == -1 )
            break;

        if( c == 0 )
        {
            switch( option_index )
            {
                case 0:
                    usage();
                    break;
                case 1:
                    srcAddress = optarg;
                    break;
                case 2:
                    srcPort = atoi(optarg);
                    break;
                case 3:
                    sinkBindAddressA = optarg;
                    sinkBindAddressB = optarg;
                    break;
                case 4:
                    skinkBindPortA = atoi(optarg);
                    break;
                case 5:
                    skinkBindPortB = atoi(optarg);
                    break;
                default:
                    usage();
            }
        }
    }

    printf("Using params:\n");
    printf("Source connect address: %s:%d\n", srcAddress.c_str(), srcPort);
    printf("Sink port A: %s:%d\n", sinkBindAddressA.c_str(), skinkBindPortA);
    printf("Sink port B: %s:%d\n", sinkBindAddressB.c_str(), skinkBindPortB);
    printf("\n");
    fflush(stdout);


    // ignore SIGPIPE, SIGHUP
    struct sigaction sigign;
    sigign.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sigign, NULL);
    sigaction(SIGHUP, &sigign, NULL);


    int serverSocketA = prepareServerSocket(sinkBindAddressA, skinkBindPortA);
    int serverSocketB = prepareServerSocket(sinkBindAddressB, skinkBindPortB);

    CSinkList sinkList;
    CTcpSource src(BASIC_BUFFER_SIZE, srcAddress, srcPort);

    
    CBuffer backBuffer(BACK_BUFFER_SIZE); 

    while( true )
    {

        unsigned numTriesConnect = 1;
        while( !src.isConnected() )
        {
            printf("Try to connect %s (%d)\n", src.printableAddress().c_str(), numTriesConnect );fflush(stdout);
            src.connectSync();
            if( src.isConnected() )
            {
                printf("Source port connected\n");fflush(stdout);
                break;
            }

            numTriesConnect++;
            sleep(5);
        }


        //- 
        fd_set rd_set;
        FD_ZERO(&rd_set);
        FD_SET(serverSocketA, &rd_set);
        FD_SET(serverSocketB, &rd_set);
        FD_SET(src.getSocket(), &rd_set);
        sinkList.fillRdSet( rd_set );

        //-
        fd_set wr_set;
        FD_ZERO(&wr_set);
        src.fillWrSet( wr_set );
        sinkList.fillWrSet( wr_set );

        struct timeval timeout;
        timeout.tv_sec  = 10;
        timeout.tv_usec = 0;    

        int maxFd = 0;
        maxFd = src.calcMaxFd( serverSocketA );
        maxFd = src.calcMaxFd( serverSocketB );
        maxFd = sinkList.calcMaxFd( maxFd );
        ::select(maxFd+1, &rd_set, &wr_set, NULL, &timeout);

        if( FD_ISSET(serverSocketA, &rd_set) )
        {
            struct sockaddr_in remote;
            socklen_t rlen;
            int client = ::accept4(serverSocketA, (struct sockaddr *)&remote, &rlen, SOCK_NONBLOCK );
            printf("after acceptA %d\n", client);fflush(stdout);
            prepareAcceptedSocket(client);
            sinkList.addSink(client, OUT_BUFFER_SIZE, true);
            if( src.isIdentifierPresent() )
            {
                printf("identifier sent\n");fflush(stdout);
                std::vector<char> identifier;
                src.fillIdentifier( identifier );
                sinkList.getLastSink().putData( identifier );
            }
        }

        if( FD_ISSET(serverSocketB, &rd_set) )
        {
            struct sockaddr_in remote;
            socklen_t rlen;
            int client = ::accept4(serverSocketB, (struct sockaddr *)&remote, &rlen, SOCK_NONBLOCK );
            printf("after acceptB %d\n", client);fflush(stdout);
            prepareAcceptedSocket(client);
            sinkList.addSink(client, OUT_BUFFER_SIZE, false);
            // we do not send identifier to port B
        }

        if( FD_ISSET(src.getSocket(), &wr_set) )
        {
            src.doWrite();
        }
        
        if( FD_ISSET(src.getSocket(), &rd_set) )
        {
            char buffer[BASIC_BUFFER_SIZE];
            unsigned readed = src.doRead(buffer, sizeof(buffer));
            if( readed > 0 )
            {
                sinkList.putData( buffer, readed );
            }
        }


        
        backBuffer.reset();
        sinkList.processRead( rd_set, backBuffer );
        sinkList.processWrite( wr_set );
        sinkList.cleanupTask();

        // we do not expect big data here
        if( backBuffer.bytesAvailable() > 0 )
        {
            size_t len = backBuffer.bytesAvailable();
            std::vector<char> tmpBuf(len);
            backBuffer.peek( tmpBuf, len );
            src.putData( &tmpBuf[0], len );
        }

    }

    return 0;
}
