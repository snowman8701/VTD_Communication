// ExampleVehDynInteg.cpp : (c) 2014 by VIRES Simulationstechnologie GmbH
//

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
#include "RDBHandler.hh"
#include <math.h>

#define DEFAULT_PORT        48192   /* for image port it should be 48192 */
#define DEFAULT_BUFFER      1555200

char                      szServer[128];        // Server to connect to
int                       iPort = DEFAULT_PORT; // Port on server to connect to
static int                sClient = -1;         // client socket

// function prototypes
void parseRDBMessage( RDB_MSG_t* msg, bool & isImage );
void parseRDBMessageEntry( const double & simTime, const unsigned int & simFrame, RDB_MSG_ENTRY_HDR_t* entryHdr );
void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_DRIVER_CTRL_t & item );
void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_OBJECT_STATE_t & item, bool isExtended );
void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_IMAGE_t & item);
void sendOwnObjectState( int & sendSocket, const double & simTime, const unsigned int & simFrame );
int getNoReadyRead( int descriptor );



int main(int argc, char* argv[])
{
    strcpy( szServer, "127.0.0.1" );
    char*              szBuffer = new char[DEFAULT_BUFFER];  // allocate on heap
    int                ret;
    struct sockaddr_in server;
    struct hostent    *host     = NULL;
    static bool        sVerbose = false;

    sClient = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    
    if ( sClient == -1 )
    {
        fprintf( stderr, "socket() failed: %s\n", strerror( errno ) );
        return 1;
    }
    
    // int opt = 1;
    // setsockopt ( sClient, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof( opt ) ); // do not use Nagle Algorithm

    server.sin_family      = AF_INET;
    server.sin_port        = htons(iPort);
    server.sin_addr.s_addr = inet_addr(szServer);
    
    //
    // If the supplied server address wasn't in the form
    // "aaa.bbb.ccc.ddd" it's a hostname, so try to resolve it
    //
    if ( server.sin_addr.s_addr == INADDR_NONE )
    {
        host = gethostbyname(szServer);
        if ( host == NULL )
        {
            fprintf( stderr, "Unable to resolve server: %s\n", szServer );
            return 1;
        }
        memcpy( &server.sin_addr, host->h_addr_list[0], host->h_length );
    }
	// wait for connection
	bool bConnected = false;

    while ( !bConnected )
    {
        if ( connect( sClient, (struct sockaddr *)&server, sizeof( server ) ) == -1 )
        {
            fprintf( stderr, "connect() failed: %s\n", strerror( errno ) );
            sleep( 1 );
        }
        else
            bConnected = true;
    }

    fprintf( stderr, "connected!\n" );
    
    unsigned int  bytesInBuffer = 0;
    size_t        bufferSize    = sizeof( RDB_MSG_HDR_t );
    unsigned int  count         = 0;
    unsigned char *pData        = ( unsigned char* ) calloc( 1, bufferSize );

    // Main loop, send and receive data - forever!
    //
    for(;;)
    {
        bool bMsgComplete = false;
        
        // make sure this is non-bloekcing

        int nrReady = getNoReadyRead( sClient ); // MUST BE ZERO in order to be non-blocking!!!
    
        if ( nrReady < 0 )
        {
            printf( "recv() failed: %s\n", strerror( errno ) );
            break;
        }
        
        //fprintf( stderr, "nrReady = %d\n", nrReady );
        
        if ( nrReady > 0 )
        {
            // read data
            ret = recv( sClient, szBuffer, DEFAULT_BUFFER, 0 );

            if ( ret != 0 )
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
                        // if ( sVerbose )
                        //     Framework::RDBHandler::printMessage( ( RDB_MSG_t* ) pData, true );
 
                        // now parse the message
                        parseRDBMessage( ( RDB_MSG_t* ) pData, isImage );

                        // remove message from queue
                        memmove( pData, pData + msgSize, bytesInBuffer - msgSize );
                        bytesInBuffer -= msgSize;
                    }
                }
            }
        }
        // do some other stuff before returning to network reading
        
        // don't use the processor excessively
        usleep( 10 );
    }
    ::close(sClient);

    return 0;
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
            case RDB_PKG_ID_IMAGE:
                fprintf( stderr, "Package type RDB_PKG_ID_IMAGE\n");
                handleRDBitem( simTime, simFrame, *( ( RDB_IMAGE_t* ) dataPtr ) );
                break;            
            default:
                break;
        }
        dataPtr += entryHdr->elementSize;
     }
}

void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_OBJECT_STATE_t & item, bool isExtended )
{
  fprintf( stderr, "handleRDBitem: handling object state\n" );
  fprintf( stderr, "    simTime = %.3lf, simFrame = %ld\n", simTime, simFrame );
  fprintf( stderr, "    object = %s, id = %d\n", item.base.name, item.base.id );
  fprintf( stderr, "    position = %.3lf / %.3lf / %.3lf\n", item.base.pos.x, item.base.pos.y, item.base.pos.z );

  if ( isExtended )
    fprintf( stderr, "    speed = %.3lf / %.3lf / %.3lf\n", item.ext.speed.x, item.ext.speed.y, item.ext.speed.z );
}

void handleRDBitem( const double & simTime, const unsigned int & simFrame, RDB_IMAGE_t & item)
{
    fprintf(stderr,"id : %d\n", item.id);

    int width = item.width * 3;
    int height = item.height;
    int pictureSize = width * height;

    uint8_t colors[pictureSize];

    for( int i=0 ; i < pictureSize; ++i)
    {
        colors[i]= reinterpret_cast<uint8_t*>( &item + 1 )[i];
        // fprintf(stderr,"r: %d\n",int(colors[0]));

    }

    fprintf(stderr,"r_uint8_t : %d\n",colors[0]);
    fprintf(stderr,"g_uint8_t : %d\n",colors[1]);
    fprintf(stderr,"b_uint8_t : %d\n",colors[2]);

    // if (item.base.id == (uint32_t)EgoID)
    // {

    //     prevobjectlist = &item;

    //     fprintf(stderr,"id : %d\n", item.base.id);
    //     fprintf(stderr,"geo_dimX : %f\n", prevobjectlist->base.geo.dimX);
    //     fprintf(stderr,"geo_dimY : %f\n", prevobjectlist->base.geo.dimY);
    //     fprintf(stderr,"geo_offX : %f\n", prevobjectlist->base.geo.offX);
    //     fprintf(stderr,"geo_offY : %f\n", prevobjectlist->base.geo.offY);
    //     fprintf(stderr,"pos_X : %f\n", prevobjectlist->base.pos.x);
    //     fprintf(stderr,"pos_Y : %f\n", prevobjectlist->base.pos.y);
    //     fprintf(stderr,"pos_Z : %f\n", prevobjectlist->base.pos.z);
    //     double vel;
    //     vel = sqrt(prevobjectlist->ext.speed.x*prevobjectlist->ext.speed.x+prevobjectlist->ext.speed.y*prevobjectlist->ext.speed.y);
    //     fprintf(stderr,"Velocity : %lf\n", vel);
    // }
}

int getNoReadyRead( int descriptor )
{
    fd_set         fs;
    struct timeval time;
    
    // still no valid descriptor?
    if ( descriptor < 0 )
        return 0;

    FD_ZERO( &fs );
    FD_SET( descriptor, &fs );
    
    memset( &time, 0, sizeof( struct timeval ) );

    int retVal = 0;
    
    retVal = select( descriptor + 1, &fs, (fd_set*) 0, (fd_set*) 0, &time);
    
    if ( retVal > 0 )
        return FD_ISSET( descriptor, &fs );
    
    return retVal;
}
