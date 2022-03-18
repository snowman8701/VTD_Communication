#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "RDBHandler.hh"

#define DEFAULT_RX_PORT     48270   /* for image port it should be 48192 */
#define DEFAULT_TX_PORT     48271   
#define DEFAULT_BUFFER      204800

char                      hostname[128];        // Server to connect to

int          mPortTx    = DEFAULT_TX_PORT;
int          mSocketTx  = -1;
unsigned int mAddressTx = INADDR_BROADCAST;
int          mPortRx    = DEFAULT_RX_PORT;
int          mSocketRx  = -1;
unsigned int mAddressRx = INADDR_ANY; //inet_addr( "127.0.0.1" );

// function prototypes
void parseRDBMessage( RDB_MSG_t* msg, bool & isImage );
void parseRDBMessageEntry( const double & simTime, const unsigned int & simFrame, RDB_MSG_ENTRY_HDR_t* entryHdr );
void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_CONTACT_POINT_t & item );
void sendRoadQuery( int & sendSocket, const double & simTime, const unsigned int & simFrame );
int openRxPort();
int openTxPort();

//
// Function: usage:
//
// Description:
//    Print usage information and exit
//
void usage()
{
    printf("usage: odrGatewaySample [-r:n] [-s:n]\n\n");
    printf("       -r:n      receive port\n");
    printf("       -s:n      send port\n");
    printf("       -a:IP     IP address of partner (127.0.0.1)\n");
    exit(1);
}

//
// Function: ValidateArgs
//
// Description:
//    Parse the command line arguments, and set some global flags
//    to indicate what actions to perform
//
void ValidateArgs(int argc, char **argv)
{
    int i;
    
    strcpy( hostname, "127.0.0.1" );
 
    for(i = 1; i < argc; i++)
    {
        if ((argv[i][0] == '-') || (argv[i][0] == '/'))
        {
            switch (tolower(argv[i][1]))
            {
                case 'r':        // Remote port
                    if (strlen(argv[i]) > 3)
                        mPortRx = atoi(&argv[i][3]);
                    break;
                case 's':        // Remote port
                    if (strlen(argv[i]) > 3)
                        mPortTx = atoi(&argv[i][3]);
                    break;
                case 'a':       // Server
                    if (strlen(argv[i]) > 3)
                    {
                        strcpy(hostname, &argv[i][3]);
                    }
                    break;
                default:
                    usage();
                    break;
            }
        }
    }
    
    mAddressTx = inet_addr( hostname );
    //mAddressRx = inet_addr( szServer );
    
    fprintf( stderr, "ValidateArgs: TX port  = %d, RX port = %d, TX address = %s (0x%x)\n",
                     mPortTx, mPortRx, hostname, mAddressTx );
}

int main(int argc, char* argv[])
{
    char*              szBuffer = new char[DEFAULT_BUFFER];  // allocate on heap
    int                ret;
    struct sockaddr_in server;
    struct hostent    *host     = NULL;
    static bool        sVerbose = false;

    // Parse the command line
    //
    ValidateArgs(argc, argv);
    
    // open receive port
    if ( !openRxPort() )
    {
        fprintf( stderr, "main: FATAL: could not open receive port\n" );
        return 0;
    }
        
    // open send port
    if ( !openTxPort() )
    {
        fprintf( stderr, "main: FATAL: could not open send port\n" );
        return 0;
    }
        
    // now handle the messagess    
    unsigned int  bytesInBuffer = 0;
    size_t        bufferSize    = sizeof( RDB_MSG_HDR_t );
    unsigned int  sCount         = 0;
    unsigned char *pData        = ( unsigned char* ) calloc( 1, bufferSize );

    // Main loop, send and receive data - forever!
    for(;;)
    {
        bool bMsgComplete = false;
        
        sCount++;

        // fprintf(stderr,"sCount : %lu\n", sCount);
        
        // request the contact information
        // if ( !( sCount ) )
        //     sendRoadQuery( mSocketTx, 0.0, 0 );
        
        // read data
        ret = recv( mSocketRx, szBuffer, DEFAULT_BUFFER, 0 );
        
        // fprintf( stderr, "main: ret = %d, bytesInBuffer = %d\n", ret, bytesInBuffer );

        if ( ret > 0 )
        {
            // do we have to grow the buffer??
            if ( ( bytesInBuffer + ret ) > bufferSize )
            {
                pData      = ( unsigned char* ) realloc( pData, bytesInBuffer + ret );
                bufferSize = bytesInBuffer + ret;
            }

            memcpy( pData + bytesInBuffer, szBuffer, ret );
            bytesInBuffer += ret;

            // already complete messagae?
            if ( bytesInBuffer >= sizeof( RDB_MSG_HDR_t ) )
            {
                RDB_MSG_HDR_t* hdr = ( RDB_MSG_HDR_t* ) pData;

                // is this message containing the valid magic number?
                if ( hdr->magicNo != RDB_MAGIC_NO )
                {
                    printf( "message receiving is out of sync; discarding data" );
                    bytesInBuffer = 0;
                }

                // handle all messages in the buffer before proceeding
                while ( bytesInBuffer >= ( hdr->headerSize + hdr->dataSize ) )
                {
                    unsigned int msgSize = hdr->headerSize + hdr->dataSize;
                    bool         isImage = false;
 
                    // print the message
                    if ( sVerbose )
                        Framework::RDBHandler::printMessage( ( RDB_MSG_t* ) pData, true );
 
                    // now parse the message
                    parseRDBMessage( ( RDB_MSG_t* ) pData, isImage );

                    // remove message from queue
                    memmove( pData, pData + msgSize, bytesInBuffer - msgSize );
                    bytesInBuffer -= msgSize;
                }
            }
        }
        
        // do some other stuff before returning to network reading
        
        // don't use the processor excessively
        usleep( 10 );
    }
    ::close( mSocketTx );
    ::close( mSocketRx );

    return 0;
}

int openRxPort()
{
    // port already open?
    if ( mSocketRx > -1 )
        return 0;
        
    struct sockaddr_in sock;
    int                opt;
    unsigned int       mAddress = mAddressRx;

    fprintf( stderr, "openRxPort: RX port  = %d, RX address = 0x%x\n", mPortRx, mAddressRx );

    mSocketRx = socket( AF_INET, SOCK_DGRAM, 0 );
    
    if ( mSocketRx == -1 )
    {
        fprintf( stderr, "openRxPort: could not open socket\n" );
        return 0;
    }

    opt = 1;
    if ( setsockopt( mSocketRx, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) ) == -1 )
    {
        fprintf( stderr, "openRxPort: could not set address to reuse\n" );
        return 0;
    }

    memset( &sock, 0, sizeof( sock ) );
    sock.sin_family 	 = AF_INET;
    sock.sin_addr.s_addr = mAddressRx;
    sock.sin_port        = htons( mPortRx );

    if ( bind( mSocketRx, (struct sockaddr *) &sock, sizeof( struct sockaddr_in ) ) == -1 )
    {
        fprintf( stderr, "openRxPort: could not bind socket\n" );
        perror( "reason: " );
        return 0;
    }

    if ( mSocketRx < 0 )
        return 0;

    opt = 1;
    ioctl( mSocketRx, FIONBIO, &opt );
    
    fprintf( stderr, "openRxPort: socketRx = %d\n", mSocketRx );
    
    return 1;
}

int openTxPort()
{
    struct sockaddr_in sock;
    int                opt;
    
    mSocketTx = socket( AF_INET, SOCK_DGRAM, 0 );

    if ( mSocketTx == -1 ) 
    { 
        fprintf( stderr, "openTxPort: could not open send socket");
        return 0;
    }

    if ( 1 )    // broadcast!
    {   
        opt = 1;
        if ( setsockopt( mSocketTx, SOL_SOCKET, SO_BROADCAST, &opt, sizeof( opt ) ) == -1 )
        {
            fprintf( stderr, "openTxPort: could not set socket to broadcast");
            return 0;
        }
    }

    opt = 1;
    if ( setsockopt( mSocketTx, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof( opt ) ) == -1 ) 
    { 
        fprintf( stderr, "openTxPort: could not set address to reuse");
        return 0;
    }
    
    sock.sin_family      = AF_INET;
    sock.sin_addr.s_addr = mAddressTx;
    sock.sin_port        = htons( mPortTx ); 

    if ( connect( mSocketTx, reinterpret_cast<struct sockaddr*>( &sock ), sizeof( struct sockaddr_in ) ) == -1 ) 
    { 
        fprintf( stderr, "openTxPort: could not connect socket");
        return 0;
    }

    return 1 ;
}

void parseRDBMessage( RDB_MSG_t* msg, bool & isImage )
{
    if ( !msg )
      return;

    if ( !msg->hdr.dataSize )
        return;
    
    RDB_MSG_ENTRY_HDR_t* entry = ( RDB_MSG_ENTRY_HDR_t* ) ( ( ( char* ) msg ) + msg->hdr.headerSize );
    uint32_t remainingBytes    = msg->hdr.dataSize;
        
    while ( remainingBytes )
    {
        parseRDBMessageEntry( msg->hdr.simTime, msg->hdr.frameNo, entry );

        isImage |= ( entry->pkgId == RDB_PKG_ID_IMAGE );

        remainingBytes -= ( entry->headerSize + entry->dataSize );
        
        if ( remainingBytes )
          entry = ( RDB_MSG_ENTRY_HDR_t* ) ( ( ( ( char* ) entry ) + entry->headerSize + entry->dataSize ) );
    }
}

void parseRDBMessageEntry( const double & simTime, const unsigned int & simFrame, RDB_MSG_ENTRY_HDR_t* entryHdr )
{
    if ( !entryHdr )
        return;
    
    int noElements = entryHdr->elementSize ? ( entryHdr->dataSize / entryHdr->elementSize ) : 0;
    
    unsigned char ident   = 6;
    char*         dataPtr = ( char* ) entryHdr;
        
    dataPtr += entryHdr->headerSize;
        
    while ( noElements-- )      // only two types of messages are handled here
    {
        switch ( entryHdr->pkgId )
        {
            case RDB_PKG_ID_CONTACT_POINT:
                handleRDBitem( simTime, simFrame, *( ( RDB_CONTACT_POINT_t* ) dataPtr ) );
                break;

            default:
                break;
        }
        dataPtr += entryHdr->elementSize;
     }
}

void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_CONTACT_POINT_t & item )
{
  fprintf( stderr, "handleRDBitem: handling contact point\n" );
  fprintf( stderr, "    simTime = %.3lf, simFrame = %ld\n", simTime, simFrame );
  fprintf( stderr, "    id = %d\n", item.id );
  fprintf( stderr, "    position = %.3lf / %.3lf / %.3lf, rotation = %.3lf / %.3lf/ %.3lf, friction = %.3lf\n", item.roadDataIn.x, item.roadDataIn.y, item.roadDataIn.z, item.roadDataIn.h*57.3f, item.roadDataIn.p*57.3f, item.roadDataIn.r*57.3f, item.friction );
}

void sendRoadQuery( int & sendSocket, const double & simTime, const unsigned int & simFrame )
{
    Framework::RDBHandler myHandler;

    // start a new message
    myHandler.initMsg();

    // add extended package for the object state
    RDB_ROAD_QUERY_t *roadQuery = ( RDB_ROAD_QUERY_t* ) myHandler.addPackage( simTime, simFrame, RDB_PKG_ID_ROAD_QUERY, 4 );

    if ( !roadQuery )
    {
        fprintf( stderr, "sendRoadQuery: could not create road query\n" );
        return;
    }
    
    // setup the four wheels
    
    roadQuery[0].id = 3111;
    roadQuery[0].x  = 2.0;
    roadQuery[0].y  = 0.7;
    roadQuery[0].flags = RDB_ROAD_QUERY_FLAG_RELATIVE_POS;

    roadQuery[1].id = 3222;
    roadQuery[1].x  = 2.0;
    roadQuery[1].y  = -0.7;
    roadQuery[1].flags = RDB_ROAD_QUERY_FLAG_RELATIVE_POS;

    roadQuery[2].id = 3333;
    roadQuery[2].x  = 0.0;
    roadQuery[2].y  = 0.7;
    roadQuery[2].flags = RDB_ROAD_QUERY_FLAG_RELATIVE_POS;

    roadQuery[3].id = 3444;
    roadQuery[3].x  = 0.0;
    roadQuery[3].y  = -0.7;
    roadQuery[3].flags = RDB_ROAD_QUERY_FLAG_RELATIVE_POS;

    // roadQuery[4].id = 4111;
    // roadQuery[4].x  = 1858.531;
    // roadQuery[4].y  = 715.527;
    // roadQuery[4].flags = 0;

    // roadQuery[5].id = 4222;
    // roadQuery[5].x  = 1859.804;
    // roadQuery[5].y  = 715.110;
    // roadQuery[5].flags = 0;

    // roadQuery[6].id = 4333;
    // roadQuery[6].x  = 1859.363;
    // roadQuery[6].y  = 713.709;
    // roadQuery[6].flags = 0;

    // roadQuery[7].id = 4444;
    // roadQuery[7].x  = 1860.637;
    // roadQuery[7].y  = 714.291;
    // roadQuery[7].flags = 0;

    // add the end-of-frame boundary        
    myHandler.addPackage( simTime, simFrame, RDB_PKG_ID_END_OF_FRAME, 0 );

    fprintf( stderr, "sendRoadQuery: sending queries, total size = %d\n",  myHandler.getMsgTotalSize() );
        

    int retVal = send( sendSocket, ( const char* ) ( myHandler.getMsg() ), myHandler.getMsgTotalSize(), 0 );

    if ( !retVal )
        fprintf( stderr, "sendRoadQuery: could not send queries\n" );
}

