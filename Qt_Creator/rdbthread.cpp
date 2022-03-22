#include "rdbthread.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/shm.h>

#define DEFAULT_PORT 48190
#define DEFAULT_PORT_LIVE 48192
#define DEFAULT_BUFFER_SIZE 204800
#define DEFAULT_BUFFER_SIZE_Image 1555200

std::string szServer("127.0.0.1");

RDBthread::RDBthread(QObject *parent) :
    QThread(parent),
    mSocketTCP(-1),
    mSocketTCP_Image(-1),
    EgoID(0)
{
    createSocketTCP();
    createSocketTCP_Image();
}

RDBthread::~RDBthread()
{
    delete this;
}

void RDBthread::run()
{
    // createSocketTCP();
    while(true)
    {
        dummyTCrecv(mSocketTCP);
        dummyTCrecv_Image(mSocketTCP_Image);
    }
}

void RDBthread::createSocketTCP()
{

        struct sockaddr_in server;
        struct hostent *host = NULL;

        // already created?
        if (mSocketTCP >= 0)
            return;

        mSocketTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        fprintf(stderr, "Module::createComSocket: mOutSocketTCP=%d\n", mSocketTCP);

        if (mSocketTCP == -1)
        {
            fprintf(stderr, "Module::createComSocket: could not open socket\n");
            return;
        }

        int opt = 1; ///////////////////////////
        setsockopt(mSocketTCP, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)); ////////////////

        server.sin_family = AF_INET;
        server.sin_port = htons(DEFAULT_PORT);
        server.sin_addr.s_addr = inet_addr(szServer.c_str());

        if (server.sin_addr.s_addr == INADDR_NONE)
        {
            host = gethostbyname("127.0.0.1");
            if (host == NULL)
            {
                fprintf(stderr, "Unable to resolve server: %s\n", "127.0.0.1");
                return;
            }
            memcpy(&server.sin_addr, host->h_addr_list[0], host->h_length);
        }
        // wait for connection
        bool bConnected = false;

        while (!bConnected)
        {
            // this->connect(&qserver, SIGNAL(QTcpServer::newConnection()), this, SLOT(this->onNewConnection()));

            if (::connect(mSocketTCP, (struct sockaddr *)&server, sizeof(server)) == -1)
            {
                fprintf(stderr, "connect() failed: %s\n", strerror(errno));
                sleep(1);
            }
            else
                bConnected = true;

        }

        fprintf(stderr, "connected!\n");
}

void RDBthread::createSocketTCP_Image()
{
    struct sockaddr_in server;
    struct hostent *host = NULL;

    // already created?
    if (mSocketTCP_Image >= 0)
        return;

    mSocketTCP_Image = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    fprintf(stderr, "Module::createComSocket: mOutSocketTCP=%d\n", mSocketTCP_Image);

    if (mSocketTCP_Image == -1)
    {
        fprintf(stderr, "Module::createComSocket: could not open socket\n");
        return;
    }

//        int opt = 1; ///////////////////////////
//        setsockopt(mSocketTCP_Image, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)); ////////////////

    server.sin_family = AF_INET;
    server.sin_port = htons(DEFAULT_PORT_LIVE);
    server.sin_addr.s_addr = inet_addr(szServer.c_str());

    if (server.sin_addr.s_addr == INADDR_NONE)
    {
        host = gethostbyname("127.0.0.1");
        if (host == NULL)
        {
            fprintf(stderr, "Unable to resolve server: %s\n", "127.0.0.1");
            return;
        }
        memcpy(&server.sin_addr, host->h_addr_list[0], host->h_length);
    }
    // wait for connection
    bool bConnected = false;

    while (!bConnected)
    {
        // this->connect(&qserver, SIGNAL(QTcpServer::newConnection()), this, SLOT(this->onNewConnection()));

        if (::connect(mSocketTCP_Image, (struct sockaddr *)&server, sizeof(server)) == -1)
        {
            fprintf(stderr, "connect() failed: %s\n", strerror(errno));
            sleep(1);
        }
        else
            bConnected = true;

    }

    fprintf(stderr, "RDB connected Image!\n");
}





void RDBthread::dummyTCrecv(int &sendSocket)
{

    unsigned int bytesInBuffer = 0;
      size_t bufferSize = sizeof(RDB_MSG_HDR_t);
      // unsigned int  count         = 0;
      unsigned char *pData = (unsigned char *)calloc(1, bufferSize);
      bool bMsgComplete = false;
      char *szBuffer = new char[DEFAULT_BUFFER_SIZE]; // allocate on heap

      // read a complete message
      while (!bMsgComplete)
      {
          //if ( sSendTrigger && !( count++ % 1000 ) )
          //  sendTrigger( sClient, 0, 0 );

          ssize_t ret = recv(sendSocket, szBuffer, DEFAULT_BUFFER_SIZE, 0);
          int tempRet = (int)ret;
          if (tempRet == -1)
          {
              printf("recv() failed: %s\n", strerror(errno));
              break;
          }

          if (tempRet != 0)
          {
              // do we have to grow the buffer??
              if ((bytesInBuffer + tempRet) > bufferSize)
              {
                  pData = (unsigned char *)realloc(pData, bytesInBuffer + tempRet);
                  bufferSize = bytesInBuffer + tempRet;
              }

              memcpy(pData + bytesInBuffer, szBuffer, tempRet);
              bytesInBuffer += tempRet;

              // already complete messagae?
              if (bytesInBuffer >= sizeof(RDB_MSG_HDR_t))
              {
                  RDB_MSG_HDR_t *hdr = (RDB_MSG_HDR_t *)pData;

                  // is this message containing the valid magic number?
                  if (hdr->magicNo != RDB_MAGIC_NO)
                  {
                      printf("message receiving is out of sync; discarding data");
                      bytesInBuffer = 0;
                  }

                  while (bytesInBuffer >= (hdr->headerSize + hdr->dataSize))
                  {

                      unsigned int msgSize = hdr->headerSize + hdr->dataSize;
                      bool         isImage = false;

                      // print the message
                      //if ( sVerbose )
                      //    Framework::RDBHandler::printMessage( ( RDB_MSG_t* ) pData, true );

                      // now parse the message
                      parseRDBMessage( ( RDB_MSG_t* ) pData, isImage );

                      // remove message from queue
                      memmove( pData, pData + msgSize, bytesInBuffer - msgSize );
                      bytesInBuffer -= msgSize;

                      bMsgComplete = true;
                  }
              }
          }
      }
}

void RDBthread::dummyTCrecv_Image(int &sendsocket)
{
    unsigned int bytesInBuffer = 0;
    size_t bufferSize = sizeof(RDB_MSG_HDR_t);
    // unsigned int  count         = 0;
    unsigned char *pData_Image = (unsigned char *)calloc(1, bufferSize);
    bool bMsgComplete_Image = false;
    char *szBuffer_Image = new char[DEFAULT_BUFFER_SIZE_Image]; // allocate on heap

    // read a complete message
    while (!bMsgComplete_Image)
    {
        //if ( sSendTrigger && !( count++ % 1000 ) )
        //  sendTrigger( sClient, 0, 0 );

        ssize_t ret = recv(sendsocket, szBuffer_Image, DEFAULT_BUFFER_SIZE_Image, 0);
        int tempRet = (int)ret;
        if (tempRet == -1)
        {
            printf("recv(image\n");
            printf("recv() failed: %s\n", strerror(errno));
            break;
        }

        if (tempRet != 0)
        {
            // do we have to grow the buffer??
            if ((bytesInBuffer + tempRet) > bufferSize)
            {
                pData_Image = (unsigned char *)realloc(pData_Image, bytesInBuffer + tempRet);
                bufferSize = bytesInBuffer + tempRet;
            }

            memcpy(pData_Image + bytesInBuffer, szBuffer_Image, tempRet);
            bytesInBuffer += tempRet;

            // already complete messagae?
            if (bytesInBuffer >= sizeof(RDB_MSG_HDR_t))
            {
                RDB_MSG_HDR_t *hdr = (RDB_MSG_HDR_t *)pData_Image;

                // is this message containing the valid magic number?
                if (hdr->magicNo != RDB_MAGIC_NO)
                {
                    printf("message receiving is out of sync; discarding data");
                    bytesInBuffer = 0;
                }

                while (bytesInBuffer >= (hdr->headerSize + hdr->dataSize))
                {

                    unsigned int msgSize = hdr->headerSize + hdr->dataSize;
                    bool         isImage = false;

                    // print the message
                    //if ( sVerbose )
                    //    Framework::RDBHandler::printMessage( ( RDB_MSG_t* ) pData, true );

                    // now parse the message
                    parseRDBMessage( ( RDB_MSG_t* ) pData_Image, isImage );

                    // remove message from queue
                    memmove( pData_Image, pData_Image + msgSize, bytesInBuffer - msgSize );
                    bytesInBuffer -= msgSize;

                    bMsgComplete_Image = true;
                }
            }
        }
        usleep(10);
    }
    // ::close(sendsocket);
}


void RDBthread::parseRDBMessage( RDB_MSG_t* msg, bool & isImage )
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

        // fprintf(stderr,"data_size : %lu\n",sizeof(entry->dataSize));

        isImage |= ( entry->pkgId == RDB_PKG_ID_IMAGE );

        remainingBytes -= ( entry->headerSize + entry->dataSize );

        if ( remainingBytes )
        entry = ( RDB_MSG_ENTRY_HDR_t* ) ( ( ( ( char* ) entry ) + entry->headerSize + entry->dataSize ) );
    }
}

void RDBthread::parseRDBMessageEntry( const double & simTime, const unsigned int & simFrame, RDB_MSG_ENTRY_HDR_t* entryHdr )
{
    if ( !entryHdr )
         return;

     int noElements = entryHdr->elementSize ? ( entryHdr->dataSize / entryHdr->elementSize ) : 0;

     if ( !noElements )  // some elements require special treatment
     {
         switch ( entryHdr->pkgId )
         {
             case RDB_PKG_ID_START_OF_FRAME:
                 // fprintf( stderr, "void parseRDBMessageEntry: got start of frame\n" );
                 break;

             case RDB_PKG_ID_END_OF_FRAME:
                 // fprintf( stderr, "void parseRDBMessageEntry: got end of frame\n" );
                 break;

             default:
                 return;
                 break;
         }
         return;
     }

      unsigned char ident   = 6;

      char*         dataPtr = ( char* ) entryHdr;

      dataPtr += entryHdr->headerSize;

      while ( noElements-- )
      {
          // bool printedMsg = true;
          // fprintf(stderr,"noElements : %d\n",noElements);
          switch ( entryHdr->pkgId )
          {
              case RDB_PKG_ID_OBJECT_STATE:
                   handleRDBitem( simTime, simFrame, *( ( RDB_OBJECT_STATE_t* ) dataPtr ), entryHdr->flags & RDB_PKG_FLAG_EXTENDED, noElements );
                   // change();
                   break;
              case RDB_PKG_ID_DRIVER_CTRL:
                   handleDriver( *( ( RDB_DRIVER_CTRL_t* ) dataPtr ), ident );
                   break;
              case RDB_PKG_ID_LANE_INFO:
                   handleLane( *( ( RDB_LANE_INFO_t* ) dataPtr ), ident );
                   break;
              case RDB_PKG_ID_SENSOR_OBJECT:
                   handleSensor( simTime, simFrame, *( ( RDB_SENSOR_OBJECT_t* ) dataPtr ), ident , noElements );
                   break;
              case RDB_PKG_ID_IG_FRAME:
                   handleframe( simTime, simFrame, *( ( RDB_IG_FRAME_t* ) dataPtr ), ident, noElements );
                   break;
              case RDB_PKG_ID_IMAGE:
                   handleImageitem( simTime, simFrame, *( ( RDB_IMAGE_t* ) dataPtr ) );
                   break;
          }
          dataPtr += entryHdr->elementSize;
      }
}

void RDBthread::handleRDBitem(const double & simTime, const unsigned int & simFrame, RDB_OBJECT_STATE_t & item, bool isExtended, int noElements)
{
    if (item.base.id == 1) // (uint32_t)EgoID
    {
        prevobjectlist = &item;

        vehi_speed = sqrt( (prevobjectlist->ext.speed.x * prevobjectlist->ext.speed.x) + (prevobjectlist->ext.speed.y * prevobjectlist->ext.speed.y) );
        // f.push_back(QString::number(prevobjectlist->ext.speed.x));
        emit Send_RDB_id(QString::number(prevobjectlist->base.id));
        emit Send_RDB_pos_x(QString::number(prevobjectlist->base.pos.x));
        emit Send_RDB_pos_y(QString::number(prevobjectlist->base.pos.y));
        emit Send_RDB_vehicle_sp(QString::number(sqrt(vehi_speed)));
        emit Send_RDB_sp_x(QString::number(prevobjectlist->ext.speed.x));
        emit Send_RDB_sp_y(QString::number(prevobjectlist->ext.speed.y));
        emit Send_RDB_col_accel(QString::number( ( prevobjectlist->ext.accel.x / 9.81 ) ) );
        emit Send_RDB_row_accel(QString::number( ( prevobjectlist->ext.accel.y / 9.81 ) ) );
        emit Send_RDB_YawAngle(QString::number(prevobjectlist->base.pos.h));
        emit Send_RDB_PitchAngle(QString::number(prevobjectlist->base.pos.p));
        emit Send_RDB_RollAngle(QString::number(prevobjectlist->base.pos.r));
    }
}

void RDBthread::handleDriver(RDB_DRIVER_CTRL_t & item, unsigned char ident)
{
    prevdriverlist = &item;

    if(prevobjectlist->base.id == 1)
    {
        emit Send_RDB_brake(QString::number(prevdriverlist->brakePedal)); // RDB_DRIVER_CTRL_t
        emit Send_RDB_throttle(QString::number(prevdriverlist->throttlePedal)); // RDB_DRIVER_CTRL_t
        emit Send_RDB_steering(QString::number(prevdriverlist->steeringWheel)); // RDB_DRIVER_CTRL_t

        if(prevdriverlist->flags == 1)
        {
            emit Send_RDB_Indicator("Left"); // RDB_DRIVER_CTRL_t flags
        }
        else if(prevdriverlist->flags == 2)
        {
            emit Send_RDB_Indicator("Right"); // RDB_DRIVER_CTRL_t flags
        }

        else if(prevdriverlist->flags == 512)
        {
            emit Send_RDB_collision("Collision!"); // RDB_DRIVER_CTRL_t flags
        }
        else
        {
            emit Send_RDB_Indicator("None");
        }
    }
}
void RDBthread::handleLane(RDB_LANE_INFO_t & item, unsigned char ident)
{
    prevlanelist = &item;

    // if(item.playerId == 1)
    // {
    if(prevobjectlist->base.id == 1)
    {
        emit Send_RDB_curvature(QString::number(prevlanelist->curvVert)); // RDB_LANE_INFO_t
    }
    // }
}

void RDBthread::handleSensor(const double & simTime, const unsigned int & simFrame, RDB_SENSOR_OBJECT_t & item, unsigned char ident, int noElements)
{
    prevsensorlist = &item;

    // if(item.id == 1)
    // {
    if(prevobjectlist->base.id == 1)
    {
        emit Send_RDB_timeGap(QString::number(prevsensorlist->dist)); // RDB_SENSOR_OBJECT_t / vehi_speed
    }
    // }
}

void RDBthread::handleframe(const double & simTime, const unsigned int & simFrame, RDB_IG_FRAME_t & item, unsigned char ident, int noElements)
{
    prevframeno = &item;

    // if(prevobjectlist->base.id == 1)
    // {
       emit Send_RDB_frame(QString::number(prevframeno->deltaT));
    // }
}

void RDBthread::handleImageitem(const double &simTime, const unsigned int &simFrame, RDB_IMAGE_t &item)
{
    int width = item.width * 3;
    int height = item.height;
    int pictureSize = width * height;

    char *szBuffer = new char[DEFAULT_BUFFER_SIZE];

    uint8_t colors[pictureSize];

    for( int i=0 ; i < pictureSize; ++i)
    {
        colors[i]= reinterpret_cast<uint8_t*>( &item + 1 )[i];
        // fprintf(stderr,"r: %d\n",int(colors[0]));
    }

    memcpy(szBuffer,&colors,DEFAULT_BUFFER_SIZE);

    uint8_t r = colors[0];
    uint8_t g = colors[1];
    uint8_t b = colors[2];

    emit Send_RDB_Image(reinterpret_cast<char*>( &item + 1 ), item.width, item.height);

    // memset(szBuffer, 0, item.width * item.height * 3 * sizeof(char));

    // free(szBuffer);
}

