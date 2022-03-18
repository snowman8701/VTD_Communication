// odrSHMSample.cpp : Defines the entry point for the odr shm  application.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
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

unsigned int            mShmKeyRequest = 0x123A;  // key of the SHM segment that I will request road information 
void*                   mShmPtrRequest = 0;       // pointer to the SHM segment that I will request

unsigned int            mShmKeyRead    = 0x123b;  // key of the SHM segment that I will read road information 
unsigned int            mCheckMask     = 0x2;           // first mask returned by a query
void*                   mShmPtrRead    = 0;       // pointer to the SHM segment that I will read

bool                    mVerbose      = true;
bool                    mReadyToWrite = true;
bool                    mReadyToRead  = false;
RDB_SHM_HDR_t *		shmHdr_Request		= 0;
RDB_SHM_BUFFER_INFO_t* 	shmBufferInfo_Request	= 0;
RDB_MSG_HDR_t*          msgHdr_Request          = 0;
RDB_MSG_ENTRY_HDR_t*    msgEntryHdr_Request     = 0;
RDB_ROAD_QUERY_t*       roadQuery       = 0;
RDB_ROAD_QUERY_t*       FL_Wheel       = 0;
RDB_ROAD_QUERY_t*       FR_Wheel       = 0;
RDB_ROAD_QUERY_t*       RL_Wheel       = 0;
RDB_ROAD_QUERY_t*       RR_Wheel       = 0;

RDB_SHM_HDR_t *         shmHdr_Read          = 0;
RDB_SHM_BUFFER_INFO_t*  shmBufferInfo_Read   = 0;
RDB_MSG_t*              pRdbMsg              = 0;


Framework::RDBHandler mRdbHandler_Request;
double                  mSimTime        = 0.0;               // simulation time to send outcoming message
unsigned int            mSimFrame       = 0;                 // simulation frame outcoming  message
float                   deltaT          = 0.01;

// function prototypes
int initVTD();
void* openShm(unsigned int key);
int requestShm();
int readShm( unsigned int checkMask, bool verbose, bool processMessage ); 

void handleMessage( RDB_MSG_t* msg, bool verbose );

int main(int argc, char* argv[])
{
  
    if ( initVTD() == 0 )
        return 0;

    // Send request and receive data - forever!
    //
    for(;;)
    {

        // write only if SHM is ready (i.e. at first time or if previous read was ok)
        if ( mReadyToWrite )
        { 
            mSimTime  = mSimTime + deltaT;
            mSimFrame = mSimFrame +1;
        
            requestShm();
        }
            
        usleep( 10000000 * deltaT );
        
        if ( mReadyToRead )
            readShm( mCheckMask, true, true ); 
    }
    return 0;
}

int initVTD()
{
    // first: open the shared memory (try to attach without creating a new segment)
    
    fprintf( stderr, "...Attaching to shared memory for writing....\n" );
    
    while ( !mShmPtrRequest )
    {
        mShmPtrRequest = openShm( mShmKeyRequest );
        usleep( 1000 );     // do not overload the CPU
    }
    
    fprintf( stderr, "...attached! Processing now...\n" );
    fprintf( stderr, "mShmPtrRequest = %p\n", mShmPtrRequest );
    //
    // now check the SHM for the time being

    shmHdr_Request = (RDB_SHM_HDR_t *) mShmPtrRequest;
    shmBufferInfo_Request = (RDB_SHM_BUFFER_INFO_t*) ((char*) shmHdr_Request + sizeof(RDB_SHM_HDR_t));
 
    // compose a new message
    mRdbHandler_Request.initMsg();

    mRdbHandler_Request.addPackage( mSimTime, mSimFrame, RDB_PKG_ID_START_OF_FRAME );
    msgHdr_Request = mRdbHandler_Request.getMsgHdr();

    // add ROAD Query package
    roadQuery = ( RDB_ROAD_QUERY_t* ) mRdbHandler_Request.addPackage( mSimTime, mSimFrame, RDB_PKG_ID_ROAD_QUERY, 4 );
    if ( !roadQuery )
        return 0;

    FL_Wheel = roadQuery;
    FR_Wheel = ( RDB_ROAD_QUERY_t* ) ((char *) FL_Wheel + sizeof(RDB_ROAD_QUERY_t));
    RL_Wheel = ( RDB_ROAD_QUERY_t* ) ((char *) FR_Wheel + sizeof(RDB_ROAD_QUERY_t));
    RR_Wheel = ( RDB_ROAD_QUERY_t* ) ((char *) RL_Wheel + sizeof(RDB_ROAD_QUERY_t));

    // add end of frame package
    mRdbHandler_Request.addPackage( mSimTime, mSimFrame, RDB_PKG_ID_END_OF_FRAME, 1, 0, 0 );

    //set the access pointer
    mRdbHandler_Request.shmSetAddress( mShmPtrRequest );
    
    // clear flags (they might still be set from a previous run)
    mRdbHandler_Request.shmBufferSetFlags( 0, 0 );


    // second: open the shared memory to read road data (try to attach without creating a new segment)
    fprintf( stderr, "...Attaching to shared memory for reading....\n" );

    while ( !mShmPtrRead )
    {
        mShmPtrRead = openShm( mShmKeyRead );
        usleep( 1000 );     // do not overload the CPU
    }
    fprintf( stderr, "...attached! Processing now...\n" );
    fprintf( stderr, "mShmKeyRead = %x, mShmPtrRead = %p\n", mShmKeyRead, mShmPtrRead );


    // get a pointer to the shm info block
    shmHdr_Read = (RDB_SHM_HDR_t *) mShmPtrRead;

    if ( !shmHdr_Read )
        return 0;

    shmBufferInfo_Read = (RDB_SHM_BUFFER_INFO_t*) ((char*) shmHdr_Read + shmHdr_Read->headerSize);
    
    // there may actually be old content in the "read" part of the SHM due to a previous run. 
    // Make sure this is read and invalidated before proceeding.
    readShm( mCheckMask, true, false ); 
    
    return 1;
}

void* openShm( unsigned int key )
{
    void* shmPtr = 0;
    
    int shmid = 0; 

    if ( ( shmid = shmget( key, 0, 0 ) ) < 0 )
        return 0;

    if ( ( shmPtr = (char *)shmat( shmid, (char *)0, 0 ) ) == (char *) -1 )
    {
        perror("openShm: shmat()");
        shmPtr = 0;
    }

    if ( shmPtr )
    {
        struct shmid_ds sInfo;

        if ( ( shmid = shmctl( shmid, IPC_STAT, &sInfo ) ) < 0 )
        {
            perror( "openShm: shmctl()" );
            return 0;
        }
        else
            fprintf( stderr, "shared memory with key 0x%05x has size of %d bytes\n", key, sInfo.shm_segsz );
    }
    
    return shmPtr;
}


int requestShm()
{
    msgHdr_Request->frameNo = mSimFrame;
    msgHdr_Request->simTime = mSimTime;

    FL_Wheel->id = 3111;
    FL_Wheel->x = 1706.0;
    FL_Wheel->y = 842.0;

    FR_Wheel->id = 3222;
    FR_Wheel->x = 1706.0;
    FR_Wheel->y = 842.0;

    RL_Wheel->id = 3333;
    RL_Wheel->x = 1706.0;
    RL_Wheel->y = 842.0;

    RR_Wheel->id = 3444;
    RR_Wheel->x = 1706.0;
    RR_Wheel->y = 842.0;

    // wait for a buffer to become ready for writing
    if ( mVerbose )
        fprintf( stderr, "xxxxxxxxxxxxxxxxxxxxxxxxxx shmBufferGet flags = 0x%x\n", mRdbHandler_Request.shmBufferGetFlags( 0 ) );
        
    // wait for the buffer to be free for writing (i.e. no flags set)
    while( mRdbHandler_Request.shmBufferGetFlags( 0 ) );
    {
        usleep( 10 );   // do not consume more CPU power than necessary
    }
    
    if ( mVerbose )
        fprintf( stderr, "requestShm: wrting message for simTime = %.3lf, frame = %d\n", msgHdr_Request->simTime, msgHdr_Request->frameNo );

    // lock the buffer before writing to it
    mRdbHandler_Request.shmBufferLock( 0 );
    
    // now write the message to the current RDB buffer in SHM
    mRdbHandler_Request.mapMsgToShm( 0, false ); 
     
    // add the odr flag to the buffer
    mRdbHandler_Request.shmBufferAddFlags( 0, RDB_SHM_BUFFER_FLAG_TC );     // odrGateway is acting in place of the TC

    // finally, release the buffer
    mRdbHandler_Request.shmBufferRelease( 0 );

    fprintf(stderr,"buffer access flags = %d\n", shmBufferInfo_Request->flags);
    fprintf( stderr, "yyyyyyyyyyyyyyyyyyyyy  shmBufferGet flags = 0x%x\n", mRdbHandler_Request.shmBufferGetFlags( 0 ) );
    
    // all right, data has been written and answer should be read
    mReadyToRead  = true;
    mReadyToWrite = false;
    
    return 1;
}    



int readShm( unsigned int checkMask, bool verbose, bool processMessage )
{
    if ( verbose )
    {
        fprintf( stderr, "readShm: entered: shmBufferInfo_Read = %p\n", shmBufferInfo_Read );
    }

    if ( !shmBufferInfo_Read )
        return 0;

    // remember the flags that are set for buffer
    unsigned int flags = shmBufferInfo_Read->flags;

    pRdbMsg = ( RDB_MSG_t* ) ( ( ( char* ) shmHdr_Read ) + shmBufferInfo_Read->offset );
    // check whether any buffer is ready for reading (checkMask is set (or 0) and buffer is NOT locked)
    
    bool readyForRead = ( ( flags & mCheckMask ) || !checkMask ) && !( flags & RDB_SHM_BUFFER_FLAG_LOCK );

    if ( mVerbose )
    {
        fprintf( stderr, "readShm: before processing SHM\n" );
        fprintf( stderr, "readShm: Buffer : frameNo = %06d, flags = 0x%x, locked = <%s>, lock mask set = <%s>, readyForRead = <%s>\n", 
                         pRdbMsg->hdr.frameNo, 
                         flags,
                         ( flags & RDB_SHM_BUFFER_FLAG_LOCK ) ? "true" : "false",
                         ( flags & checkMask ) ? "true" : "false",
                         readyForRead ?  "true" : "false" );
    }
    
    if ( !readyForRead )
        return 0;
        
    // just clear the message?
    if ( !processMessage )
    {
        shmBufferInfo_Read->flags = 0;
        return 0;
    }

    // lock the buffer that will be processed now (by this, no other process will alter the contents)
    if ( readyForRead && shmBufferInfo_Read )
        shmBufferInfo_Read->flags |= RDB_SHM_BUFFER_FLAG_LOCK;

    // handle all messages in the buffer
    if ( !pRdbMsg->hdr.dataSize )
    {
        fprintf( stderr, "checkShm: zero message data size, error????????????.\n" );
        return 0;
    }
    
    while ( 1 )
    {
        // handle the message that is contained in the buffer; this method should be provided by the user (i.e. YOU!)
        handleMessage( pRdbMsg, verbose );
        
        // go to the next message (if available); there may be more than one message in an SHM buffer!
        pRdbMsg = ( RDB_MSG_t* ) ( ( ( char* ) pRdbMsg ) + pRdbMsg->hdr.dataSize + pRdbMsg->hdr.headerSize );
        
        if ( !pRdbMsg )
            break;
            
        if ( pRdbMsg->hdr.magicNo != RDB_MAGIC_NO )
            break;
    }
    
    // release after reading
    shmBufferInfo_Read->flags &= ~checkMask;                   // remove the check mask
    shmBufferInfo_Read->flags &= ~RDB_SHM_BUFFER_FLAG_LOCK;     // remove the lock mask

    if ( mVerbose )
    {
        unsigned int flags = shmBufferInfo_Read->flags;

        fprintf( stderr, "odrShmReader::checkShm: after processing SHM\n" );
        fprintf( stderr, "odrShmReader::checkShm: Buffer : frameNo = %06d, flags = 0x%x, locked = <%s>, lock mask set = <%s>\n", 
                         pRdbMsg->hdr.frameNo, 
                         flags,
                         ( flags & RDB_SHM_BUFFER_FLAG_LOCK ) ? "true" : "false",
                         ( flags & checkMask ) ? "true" : "false" );
    }
    
    // data has been processed, so I may write again
    mReadyToRead  = false;
    mReadyToWrite = true;

    return 1;
}    



void handleMessage( RDB_MSG_t* msg, bool verbose )
{

    // just print the message
    //Framework::RDBHandler::printMessage( msg );
    double v =0.0; 
    if ( !msg )
        return;
        
    // remember incoming simulaton time
    mSimTime  = msg->hdr.simTime;
    mSimFrame = msg->hdr.frameNo;
    

    int i = 0, id, flags, playerId, spare1;
    float friction;
    RDB_COORD_t roadDataIn;

    RDB_CONTACT_POINT_t*  contactPoint;



    if ( mVerbose )
        fprintf( stderr, "handleMessage: simTime = %.3lf, simFrame = %d, dataSize = %d\n", mSimTime, mSimFrame, msg->hdr.dataSize );

    unsigned int noElements = 0;
    
    switch(msg->entryHdr.pkgId) 
    {

        case RDB_PKG_ID_CONTACT_POINT:

            contactPoint = ( RDB_CONTACT_POINT_t* ) Framework::RDBHandler::getFirstEntry( msg, RDB_PKG_ID_CONTACT_POINT, noElements, false );
    
            if ( mVerbose )
                fprintf( stderr, "handleMessage: noElements = %d\n", noElements );

            while ( noElements )
            {
                if ( mVerbose || verbose )
                {   
		    id = contactPoint->id;
		    flags = contactPoint->flags;
		    roadDataIn.x = contactPoint->roadDataIn.x;
		    roadDataIn.y = contactPoint->roadDataIn.y;
		    roadDataIn.z = contactPoint->roadDataIn.z;
		    roadDataIn.h = contactPoint->roadDataIn.h;
		    roadDataIn.p = contactPoint->roadDataIn.p;
		    roadDataIn.r = contactPoint->roadDataIn.r;
		    roadDataIn.flags = contactPoint->roadDataIn.flags;
		    roadDataIn.type = contactPoint->roadDataIn.type;
		    roadDataIn.system = contactPoint->roadDataIn.system;
		    friction = contactPoint->friction;
		    playerId = contactPoint->playerId;
		    printf("id = %d, flags = %d, x = %f, y = %f, z = %f, h = %f, p = %f, r = %f, roadDataIn.flags = %d, type = %d, system = %d, friction = %f, playerId = %d \n", id, flags, roadDataIn.x, roadDataIn.y, roadDataIn.z, roadDataIn.h, roadDataIn.p, roadDataIn.r, roadDataIn.flags, roadDataIn.type, roadDataIn.system, friction, playerId);
		}
                noElements--;
        
                if ( noElements )
                    contactPoint++;
            }

            break;
        default:
            break;
    }
}
