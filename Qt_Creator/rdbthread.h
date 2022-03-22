#ifndef RDBTHREAD_H
#define RDBTHREAD_H

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <viRDBIcd.h>
#include <QThread>

class RDBthread : public QThread
{
    Q_OBJECT

public:
    explicit RDBthread(QObject *parent=0);
    ~RDBthread();
private:
    // Thread run
    void run();

    // RDB Message Parsing
    void createSocketTCP();
    void dummyTCrecv(int &sendsocket);
    void parseRDBMessage( RDB_MSG_t* msg, bool & isImage );
    void parseRDBMessageEntry( const double & simTime, const unsigned int & simFrame, RDB_MSG_ENTRY_HDR_t* entryHdr );
    void handleRDBitem(const double & simTime, const unsigned int & simFrame, RDB_OBJECT_STATE_t & item, bool isExtended, int noElements);
    void handleDriver(RDB_DRIVER_CTRL_t & item, unsigned char ident);
    void handleLane(RDB_LANE_INFO_t & item, unsigned char ident);
    void handleSensor(const double & simTime, const unsigned int & simFrame, RDB_SENSOR_OBJECT_t & item, unsigned char ident, int noElements);
    void handleframe(const double & simTime, const unsigned int & simFrame, RDB_IG_FRAME_t & item, unsigned char ident, int noElements);

    // RDB
    int mSocketTCP;
    int EgoID;

    RDB_OBJECT_STATE_t *prevobjectlist = (RDB_OBJECT_STATE_t *)malloc(sizeof(RDB_OBJECT_STATE_t));
    RDB_DRIVER_CTRL_t *prevdriverlist = (RDB_DRIVER_CTRL_t *)malloc(sizeof(RDB_DRIVER_CTRL_t));
    RDB_LANE_INFO_t *prevlanelist = (RDB_LANE_INFO_t *)malloc(sizeof(RDB_LANE_INFO_t));
    RDB_SENSOR_OBJECT_t *prevsensorlist = (RDB_SENSOR_OBJECT_t *)malloc(sizeof(RDB_SENSOR_OBJECT_t));

    RDB_IG_FRAME_t *prevframeno = (RDB_IG_FRAME_t *)malloc(sizeof(RDB_IG_FRAME_t));

    double vehi_speed;

    // RDB Image Data Parsing
    void createSocketTCP_Image();
    void dummyTCrecv_Image(int &sendsocket);
    void handleImageitem(const double & simTime, const unsigned int & simFrame, RDB_IMAGE_t & item);

    int mSocketTCP_Image;

signals:
    // RDB Message Signal
    void Send_RDB_id(QString data);
    void Send_RDB_pos_x(QString data);
    void Send_RDB_pos_y(QString data);
    void Send_RDB_vehicle_sp(QString data);
    void Send_RDB_sp_x(QString data);
    void Send_RDB_sp_y(QString data);
    void Send_RDB_col_accel(QString data);
    void Send_RDB_row_accel(QString data);
    void Send_RDB_YawAngle(QString data);
    void Send_RDB_PitchAngle(QString data);
    void Send_RDB_RollAngle(QString data);

    void Send_RDB_brake(QString data); // RDB_DRIVER_CTRL_t
    void Send_RDB_throttle(QString data); // RDB_DRIVER_CTRL_t
    void Send_RDB_steering(QString data); // RDB_DRIVER_CTRL_t

    void Send_RDB_curvature(QString data); // RDB_LANE_INFO_t
    void Send_RDB_timeGap(QString data); // RDB_SENSOR_OBJECT_t
    void Send_RDB_Indicator(QString data); // RDB_DRIVER_CTRL_t flags
    void Send_RDB_collision(QString data); // RDB_DRIVER_CTRL_t flags

    void Send_RDB_frame(QString data); // RDB_IG_FRAME_t

    void Send_RDB_Image(char* szBuffer, int width, int height); // RDB_IMAGE_t
};

#endif // RDBTHREAD_H
