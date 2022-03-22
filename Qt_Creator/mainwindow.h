#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <rdbthread.h>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <string>

#include <VtdToolkit/viSCPIcd.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    // Image Streaming
    void RDB_Camera_1(char *Buf, int width, int height);
    // Monitoring Data
    void RDB_Monitor_id(QString data);
    void RDB_Monitor_pos_x(QString data);
    void RDB_Monitor_pos_y(QString data);
    void RDB_Monitor_vehicle_sp(QString data);
    void RDB_Monitor_sp_x(QString data);
    void RDB_Monitor_sp_y(QString data);
    void RDB_Monitor_col_accel(QString data);
    void RDB_Monitor_row_accel(QString data);
    void RDB_Monitor_YawAngle(QString data);
    void RDB_Monitor_PitchAngle(QString data);
    void RDB_Monitor_RollAngle(QString data);
    void RDB_Monitor_brake(QString data);
    void RDB_Monitor_throttle(QString data);
    void RDB_Monitor_steering(QString data);
    void RDB_Monitor_curvature(QString data);
    void RDB_Monitor_timeGap(QString data);
    void RDB_Monitor_Indicator(QString data);
    void RDB_Monitor_collision(QString data);
    void RDB_Monitor_frame(QString data);

    // Simulation Control
    void playbutton_Clicked();
    void pausebutton_Clicked();
    void stopbutton_Clicked();
    void scpCommandButton_Clicked();

    // SCP Control -> Path Config
    void Scenario_dir_button_Clicked();
    void RDB_SavePath_button_Clicked();
    void Sensor_SavePath_button_Clicked();
    void Result_SavePath_button_Clicked();
    void GT_SavePath_button_Clicked();
    void Monitoring_SavePath_button_Clicked();

    // Vehicle mode
    void vehicle1SpinBox(int num);
    void vehicle2SpinBox(int num);
    void vehicle3SpinBox(int num);
    void vehicle4SpinBox(int num);
    void vehicle5SpinBox(int num);

    // Speed
    void speedMaxSpinBox(int speed);
    void minDistanceSpinBox(int distance);
    void pulkSpinBox(int traffic);

    // VTD Scenario parameter interface module -> Path Config
    void Recv_OpenPath_button_Clicked();
    void Recv_OpenPath_button2_Clicked();
    void GT_Result_OpenPath_button_Clicked();
    void GT_Result_OpenPath_button2_Clicked();

    void replaybutton_Clicked();
    void replaybutton2_Clicked();
    void replayStopbutton_Clicked();
    void replayStopbutton2_Clicked();

private:  
    // SCP Connect, SCP Message Send
    void createSocketSCP();
    void sendSCPMessage(int sClient, const char* text);
    void viewSCPMessage(int& sendSocket);

    Ui::MainWindow *ui;

    // RDB
    RDBthread* thread;

    // SCP
    int sClient;
    QString initialPath;
    QString ScenarioPath;
    int flag; // VTD Simulation Start/Pause flag

    // Image Streaming
    QImage *image;
};

#endif // MAINWINDOW_H
