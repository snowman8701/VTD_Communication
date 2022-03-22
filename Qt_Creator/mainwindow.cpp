#include "mainwindow.h"
#include "ui_mainwindow.h"

#define DEFAULT_PORT_SCP 48179
#define DEFAULT_BUFFER_SIZE 204800

int iPort = DEFAULT_PORT_SCP;
const std::string szServer("127.0.0.1");

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sClient(-1),
    flag(0)
{
    // CameraUI 3EA, LidarUI 3EA, replayUI 2EA
    // recvIP 1EA, sendIP 1EA, Random button 2EA, replay Monitor 2EA be will ready..

    // ui change : Monitoring => tableView -> tableWidget
    //           : scenarioPath.. => TextBrowser -> lineEdit

    ui->setupUi(this);
    this->setWindowTitle("VTD SCP Controller");

    createSocketSCP();

    // RDB Output data list define
    QStringList EgoIDList, RDBList;
    int i = 1;
    // int j = 2;
    EgoIDList.push_back("EgoID = " + QString::number(i)); // if SCPElement function solved, completed.
    EgoIDList.push_back("TUV ID = 1");
    EgoIDList.push_back("TUV ID = 2");
    EgoIDList.push_back("TUV ID = 3");
    // EgoIDList.push_back("EgoID = " + QString::number(j));
    ui->Monitoring->setColumnCount(EgoIDList.size());
    ui->Monitoring->setHorizontalHeaderLabels(EgoIDList);

    RDBList << "id" << "pos x" << "pos y" << "vehi sp" << "sp x" << "sp y" << "accel x_g" << "accel y_g"
            << "heading" << "pitch" << "roll" << "brake" << "throttle" << "steering" << "curvature" << "dist/vehi sp" << "indicator" << "collision";
    ui->Monitoring->setRowCount(RDBList.size());
    ui->Monitoring->setVerticalHeaderLabels(RDBList);
    ui->Monitoring->verticalHeader()->setVisible(true);

    // RDB Monitor
    thread = new RDBthread(this);
    thread->start();
    connect(thread, SIGNAL(Send_RDB_Image(char*,int,int)), this, SLOT(RDB_Camera_1(char*,int,int)));

    connect(thread, SIGNAL(Send_RDB_id(QString)), this, SLOT(RDB_Monitor_id(QString)));
    connect(thread, SIGNAL(Send_RDB_pos_x(QString)), this, SLOT(RDB_Monitor_pos_x(QString)));
    connect(thread, SIGNAL(Send_RDB_pos_y(QString)), this, SLOT(RDB_Monitor_pos_y(QString)));
    connect(thread, SIGNAL(Send_RDB_vehicle_sp(QString)), this, SLOT(RDB_Monitor_vehicle_sp(QString)));
    connect(thread, SIGNAL(Send_RDB_sp_x(QString)), this, SLOT(RDB_Monitor_sp_x(QString)));
    connect(thread, SIGNAL(Send_RDB_sp_y(QString)), this, SLOT(RDB_Monitor_sp_y(QString)));
    connect(thread, SIGNAL(Send_RDB_col_accel(QString)), this, SLOT(RDB_Monitor_col_accel(QString)));
    connect(thread, SIGNAL(Send_RDB_row_accel(QString)), this, SLOT(RDB_Monitor_row_accel(QString)));
    connect(thread, SIGNAL(Send_RDB_YawAngle(QString)), this, SLOT(RDB_Monitor_YawAngle(QString)));
    connect(thread, SIGNAL(Send_RDB_PitchAngle(QString)), this, SLOT(RDB_Monitor_PitchAngle(QString)));
    connect(thread, SIGNAL(Send_RDB_RollAngle(QString)), this, SLOT(RDB_Monitor_RollAngle(QString)));
    connect(thread, SIGNAL(Send_RDB_brake(QString)), this, SLOT(RDB_Monitor_brake(QString)));
    connect(thread, SIGNAL(Send_RDB_throttle(QString)), this, SLOT(RDB_Monitor_throttle(QString)));
    connect(thread, SIGNAL(Send_RDB_steering(QString)), this, SLOT(RDB_Monitor_steering(QString)));
    connect(thread, SIGNAL(Send_RDB_curvature(QString)), this, SLOT(RDB_Monitor_curvature(QString)));
    connect(thread, SIGNAL(Send_RDB_timeGap(QString)), this, SLOT(RDB_Monitor_timeGap(QString)));
    connect(thread, SIGNAL(Send_RDB_Indicator(QString)), this, SLOT(RDB_Monitor_Indicator(QString)));
    connect(thread, SIGNAL(Send_RDB_collision(QString)), this, SLOT(RDB_Monitor_collision(QString)));
    connect(thread, SIGNAL(Send_RDB_frame(QString)), this, SLOT(RDB_Monitor_frame(QString)));


    // SCP Control
    connect(ui->scenarioPathButton, SIGNAL(clicked()), this, SLOT(Scenario_dir_button_Clicked()));
    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(playbutton_Clicked()));
    connect(ui->pauseButton, SIGNAL(clicked()), this, SLOT(pausebutton_Clicked()));
    connect(ui->stopButton, SIGNAL(clicked()), this, SLOT(stopbutton_Clicked()));
    connect(ui->scpCommandButton, SIGNAL(clicked()), this, SLOT(scpCommandButton_Clicked()));

    // Image Streaming


    // Active Communication (On/Off)
    if(ui->activeComOn->isChecked())
    {

    }

    if(ui->activeComOff->isChecked())
    {

    }

    // Time
    if(ui->dayButton->isChecked())
    {

    }
    if(ui->nightButton->isChecked())
    {

    }

    // Weather
    if(ui->sunnyButton->isChecked())
    {

    }

    if(ui->fogButton->isChecked())
    {

    }

    if(ui->rainyButton->isChecked())
    {

    }

    // if valueChanged, SLOT is sendSCPMessage
    connect(ui->vehicle1SpinBox, SIGNAL(valueChanged(int)), this, SLOT(vehicle1SpinBox(int)));
    connect(ui->vehicle2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(vehicle2SpinBox(int)));
    connect(ui->vehicle3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(vehicle3SpinBox(int)));
    connect(ui->vehicle4SpinBox, SIGNAL(valueChanged(int)), this, SLOT(vehicle4SpinBox(int)));
    connect(ui->vehicle5SpinBox, SIGNAL(valueChanged(int)), this, SLOT(vehicle5SpinBox(int)));

    connect(ui->speedMaxSpinBox, SIGNAL(valueChanged(int)), this, SLOT(speedMaxSpinBox(int)));
    connect(ui->minDistanceSpinBox, SIGNAL(valueChanged(int)), this, SLOT(minDistanceSpinBox(int)));
    connect(ui->pulkSpinBox, SIGNAL(valueChanged(int)), this, SLOT(pulkSpinBox(int)));


    // SCP Control UI -> Path Settings
    connect(ui->rdbPathButton, SIGNAL(clicked()), this, SLOT(RDB_SavePath_button_Clicked()));
    connect(ui->sensorPathButton, SIGNAL(clicked()), this, SLOT(Sensor_SavePath_button_Clicked()));
    connect(ui->recvPathButton, SIGNAL(clicked()), this, SLOT(Result_SavePath_button_Clicked()));
    connect(ui->gtdataPathButton, SIGNAL(clicked()), this, SLOT(GT_SavePath_button_Clicked()));
    connect(ui->monitorDataPathButton, SIGNAL(clicked()), this, SLOT(Monitoring_SavePath_button_Clicked()));

    // VTD Scenario parameter interface module -> Path Settings
    connect(ui->recvOpenPathButton,SIGNAL(clicked()), this, SLOT(Recv_OpenPath_button_Clicked()));
    connect(ui->gtTPathButton, SIGNAL(clicked()), this, SLOT(GT_Result_OpenPath_button_Clicked()));
    connect(ui->recvOpenPathButton_3, SIGNAL(clicked()), this, SLOT(Recv_OpenPath_button2_Clicked()));
    connect(ui->gtTPathButton_3, SIGNAL(clicked()), this, SLOT(GT_Result_OpenPath_button2_Clicked()));

    connect(ui->replayButton, SIGNAL(clicked()), this, SLOT(replaybutton_Clicked()));
    connect(ui->replayButton_3, SIGNAL(clicked()), this, SLOT(replaybutton2_Clicked()));
    connect(ui->replayStopButton, SIGNAL(clicked()), this, SLOT(replayStopbutton_Clicked()));
    connect(ui->replayStopButton_3, SIGNAL(clicked()), this, SLOT(replayStopbutton2_Clicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
/////////////////// VIRES VTD Image Streaming ///////////////////
void MainWindow::RDB_Camera_1(char *Buf, int width, int height)
{
    image = new QImage(reinterpret_cast<uchar const*>(Buf), width, height, QImage::Format_RGB888);
    *image = image->mirrored(false, true);
    *image = image->scaled(341, 201, Qt::KeepAspectRatio);

    ui->camera1->setPixmap(QPixmap::fromImage(*image));

    delete image;
}

/////////////////// VIRES VTD SCP Control UI ///////////////////

void MainWindow::RDB_Monitor_id(QString data)
{
    ui->Monitoring->setItem(0, 0, new QTableWidgetItem(data)); // Ego 1
    /*
    ui->Monitoring->setItem(0, 0, new QTableWidgetItem(data)); // Ego 1
    ui->Monitoring->setItem(0, 1, new QTableWidgetItem(data)); // Ego 2
    ui->Monitoring->setItem(0, 2, new QTableWidgetItem(data)); // Ego 1
    ui->Monitoring->setItem(0, 3, new QTableWidgetItem(data)); // Ego 2
    ui->Monitoring->setItem(0, 4, new QTableWidgetItem(data)); // Ego 1
    ui->Monitoring->setItem(0, 5, new QTableWidgetItem(data)); // Ego 2
    ui->Monitoring->setItem(0, 6, new QTableWidgetItem(data)); // Ego 1
    ui->Monitoring->setItem(0, 7, new QTableWidgetItem(data)); // Ego 2
    ui->Monitoring->setItem(0, 8, new QTableWidgetItem(data)); // Ego 1
    ui->Monitoring->setItem(0, 9, new QTableWidgetItem(data)); // Ego 2
    ui->Monitoring->setItem(0, 10, new QTableWidgetItem(data)); // Ego 1
    ui->Monitoring->setItem(0, 11, new QTableWidgetItem(data)); // Ego 2
    ui->Monitoring->setItem(0, 12, new QTableWidgetItem(data)); // Ego 1
    ui->Monitoring->setItem(0, 13, new QTableWidgetItem(data)); // Ego 2
    */
}

void MainWindow::RDB_Monitor_pos_x(QString data)
{
    ui->Monitoring->setItem(0, 4, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_pos_y(QString data)
{
    ui->Monitoring->setItem(0, 8, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_vehicle_sp(QString data)
{
    ui->Monitoring->setItem(0, 12, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_sp_x(QString data)
{
    ui->Monitoring->setItem(0, 16, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_sp_y(QString data)
{
    ui->Monitoring->setItem(0, 20, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_col_accel(QString data)
{
    ui->Monitoring->setItem(0, 24, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_row_accel(QString data)
{
    ui->Monitoring->setItem(0, 28, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_YawAngle(QString data)
{
    ui->Monitoring->setItem(0, 32, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_PitchAngle(QString data)
{
    ui->Monitoring->setItem(0, 36, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_RollAngle(QString data)
{
    ui->Monitoring->setItem(0, 40, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_brake(QString data)
{
    ui->Monitoring->setItem(0, 44, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_throttle(QString data)
{
    ui->Monitoring->setItem(0, 48, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_steering(QString data)
{
    ui->Monitoring->setItem(0, 52, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_curvature(QString data)
{
    ui->Monitoring->setItem(0, 56, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_timeGap(QString data)
{
    ui->Monitoring->setItem(0, 60, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_Indicator(QString data)
{
    ui->Monitoring->setItem(0, 64, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_collision(QString data)
{
    ui->Monitoring->setItem(0, 68, new QTableWidgetItem(data)); // Ego 1
}

void MainWindow::RDB_Monitor_frame(QString data)
{
    ui->frameEdit->setText(data);
}


void MainWindow::Scenario_dir_button_Clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File Name"), ui->scenarioPath->text(), tr("xml Files(*.xml)"));

    if(!fileName.isEmpty())
        ui->scenarioPath->setText(fileName);

    ScenarioPath = fileName;
}

void MainWindow::RDB_SavePath_button_Clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File Name"), ui->scenarioPath->text(), tr("txt Files(*.txt)"));

    if(!fileName.isEmpty())
        ui->scenarioPath->setText(fileName);
}

void MainWindow::Sensor_SavePath_button_Clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File Name"), ui->scenarioPath->text(), tr("txt Files(*.txt)"));

    if(!fileName.isEmpty())
        ui->scenarioPath->setText(fileName);
}

void MainWindow::Result_SavePath_button_Clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File Name"), ui->scenarioPath->text(), tr("txt Files(*.txt)"));

    if(!fileName.isEmpty())
        ui->scenarioPath->setText(fileName);
}

void MainWindow::GT_SavePath_button_Clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File Name"), ui->scenarioPath->text(), tr("txt Files(*.txt)"));

    if(!fileName.isEmpty())
        ui->scenarioPath->setText(fileName);
}

void MainWindow::Monitoring_SavePath_button_Clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File Name"), ui->scenarioPath->text(), tr("txt Files(*.txt)"));

    if(!fileName.isEmpty())
        ui->scenarioPath->setText(fileName);
}

void MainWindow::playbutton_Clicked()
{
    if(ScenarioPath.isNull())
    {
        QMessageBox::warning(this, "Error", "Scenario path isn't existed. Please apply Scenario path.");
        return;
    }

    std::string path = ((("<SimCtrl><LoadScenario filename=\"")+ScenarioPath.toStdString())+"\"/></SimCtrl>");

    if(initialPath != ScenarioPath)
    {
        flag = 0;
        // sendSCPMessage(sClient, "<EgoCtrl player=\"\"><Speed value=\"0.000000\" /></EgoCtrl>");
        // sendSCPMessage(sClient, "<EgoCtrl player=\"Ego\"><Speed value=\"0.000000\" /></EgoCtrl>");
        // sendSCPMessage(sClient, "<EgoCtrl player=\"Ego\"><Speed value=\"0.000000\" /></EgoCtrl>");

        // ui->camera1->update();
    }

    if(flag == 0)
    {
        sendSCPMessage(sClient, "<SimCtrl><Stop /></SimCtrl>");
        sendSCPMessage(sClient, path.c_str());
        /*
        sendSCPMessage(sClient, "<Environment><Friction value=\"1.000000\" /><Date day=\"1\" month=\"6\" year=\"2008\" /><TimeOfDay headlights=\"auto\" timezone=\"0\" value=\"50400\" /><Sky cloudState=\"0/8\" visibility=\"100000.000000\" /><Precipitation intensity=\"0.000000\" type=\"none\" /><Road effectScale=\"0.500000\" state=\"dry\" /></Environment>");
        sendSCPMessage(sClient, "<Camera name=\"VIEW_CAMERA\" showOwner=\"false\"><Frustum far=\"1500.000000\" fovHor=\"60.000000\" fovVert=\"33.750000\" near=\"1.000000\" offsetHor=\"0.000000\" offsetVert=\"0.000000\" /><PosEyepoint /><ViewRelative dh=\"0.000000\" dp=\"0.000000\" dr=\"0.000000\" /><Set /></Camera>");
        sendSCPMessage(sClient, "<Display><Database enable=\"true\" streetLamps=\"false\" /><VistaOverlay enable=\"false\" /></Display>");
        sendSCPMessage(sClient, "<VIL><Imu dbElevation=\"true\" /></VIL>");
        sendSCPMessage(sClient, "<VIL><EyepointOffset hDeg=\"0.000000\" pDeg=\"0.000000\" rDeg=\"0.000000\" x=\"0.000000\" y=\"0.000000\" z=\"0.000000\" /></VIL>");
        */
        sendSCPMessage(sClient, "<SimCtrl><Start /></SimCtrl>");

        flag = 1;
    }

    else
    {
        sendSCPMessage(sClient, "<SimCtrl><Start mode=\"operation\" /></SimCtrl>");
        return;
    }

    initialPath = ScenarioPath;
}

void MainWindow::pausebutton_Clicked()
{
    if(ScenarioPath.isNull())
    {
        QMessageBox::warning(this, "Error", "Scenario path isn't existed. Please apply Scenario path.");
        return;
    }

    sendSCPMessage(sClient, "<SimCtrl><Pause/></SimCtrl>");
}

void MainWindow::stopbutton_Clicked()
{
    if(ScenarioPath.isNull())
    {
        QMessageBox::warning(this, "Error", "Scenario path isn't existed. Please apply Scenario path.");
        return;
    }

    sendSCPMessage(sClient, "<SimCtrl><Stop/></SimCtrl>");
}

void MainWindow::scpCommandButton_Clicked()
{
    QString command = ui->scpControl->toPlainText();
    sendSCPMessage(sClient, command.toStdString().c_str());
}


void MainWindow::vehicle1SpinBox(int num)
{

}

void MainWindow::vehicle2SpinBox(int num)
{

}
void MainWindow::vehicle3SpinBox(int num)
{

}
void MainWindow::vehicle4SpinBox(int num)
{

}
void MainWindow::vehicle5SpinBox(int num)
{

}

void MainWindow::speedMaxSpinBox(int speed)
{

}

void MainWindow::minDistanceSpinBox(int distance)
{

}

void MainWindow::pulkSpinBox(int traffic)
{

}

/////////////////// VIRES VTD Scenario parameter interface module ///////////////////

void MainWindow::Recv_OpenPath_button_Clicked()
{

}

void MainWindow::Recv_OpenPath_button2_Clicked()
{

}

void MainWindow::GT_Result_OpenPath_button_Clicked()
{

}

void MainWindow::GT_Result_OpenPath_button2_Clicked()
{

}

void MainWindow::replaybutton_Clicked()
{

}

void MainWindow::replaybutton2_Clicked()
{

}

void MainWindow::replayStopbutton_Clicked()
{

}

void MainWindow::replayStopbutton2_Clicked()
{

}

/////////////////// SCP Socket create ///////////////////


void MainWindow::createSocketSCP()
{
    char* szBuffer = new char[DEFAULT_BUFFER_SIZE];  // allocate on heap
    int ret;

    struct sockaddr_in server;
    struct hostent    *host = NULL;

    static bool sVerbose = true;

    // Parse the command line

    //
    // Create the socket, and attempt to connect to the server
    //
    sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( sClient == -1 )
    {
        fprintf(stderr, "socket() failed: %s\n", strerror( errno ) );
        return;
    }

    int opt = 1;
    setsockopt ( sClient, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof( opt ) );

    server.sin_family = AF_INET;
    server.sin_port = htons(iPort);
    server.sin_addr.s_addr = inet_addr(szServer.c_str());

    //
    // If the supplied server address wasn't in the form
    // "aaa.bbb.ccc.ddd" it's a hostname, so try to resolve it
    //
    if (server.sin_addr.s_addr == INADDR_NONE)
    {
        host = gethostbyname("127.0.0.1");
        if (host == NULL)
        {
            fprintf(stderr, "Unable to resolve server: %s\n", szServer);
            return;
        }
        memcpy(&server.sin_addr, host->h_addr_list[0], host->h_length);
    }
    // wait for connection
    bool bConnected = false;

    while ( !bConnected )
    {
        if (::connect(sClient, (struct sockaddr *)&server, sizeof(server)) == -1)
        {
            fprintf(stderr,"connect() failed: %s\n", strerror( errno ) );
            sleep(1);
        }
        else
            bConnected = true;
    }

    fprintf(stderr, "SCP connected!\n" );
}

void MainWindow::sendSCPMessage( int sClient, const char* text )
{
    if ( !text )
        return;

    if ( !strlen( text ) )
        return;

    size_t         totalSize = sizeof( SCP_MSG_HDR_t ) + strlen( text );
    SCP_MSG_HDR_t* msg       = ( SCP_MSG_HDR_t* ) new char[ totalSize ];

    // target pointer for actual SCP message data
    char* pData = ( char* ) msg;
    pData += sizeof( SCP_MSG_HDR_t );

    // fill the header information
    msg->magicNo  = SCP_MAGIC_NO;
    msg->version  = SCP_VERSION;
    msg->dataSize = strlen( text );
    sprintf( msg->sender, "ExampleConsole" );
    sprintf( msg->receiver, "any" );

    // fill the actual data section (no trailing \0 required)

    memcpy( pData, text, strlen( text ) );

    int retVal = ::send( sClient, msg, totalSize, 0 );

    if ( !retVal )
        fprintf( stderr, "sendSCPMessage: could not send message.\n" );

    fprintf( stderr, "sendSCPMessage: sent %d characters in a message of %d bytes: ***%s***\n",
                      msg->dataSize, totalSize, text );

    // free allocated buffer space
    delete msg;
}

void MainWindow::viewSCPMessage(int& sendSocket)
{
    // Send and receive data

    char* szBuffer = new char[DEFAULT_BUFFER_SIZE];
    int ret;
    static bool sVerbose = true;

    unsigned int  bytesInBuffer = 0;
    size_t        bufferSize    = sizeof( SCP_MSG_HDR_t );
    size_t        headerSize    = sizeof( SCP_MSG_HDR_t );
    unsigned char *pData        = ( unsigned char* ) calloc( 1, bufferSize );
    unsigned int  count         = 0;

    for(;;)
    {
        bool bMsgComplete = false;

        // read a complete message
        while ( !bMsgComplete )
        {
            ret = recv(sendSocket, szBuffer, DEFAULT_BUFFER_SIZE, 0);

            if (ret == -1)
            {
                printf( "recv() failed: %s\n", strerror( errno ) );
                break;
            }

            if ( ret != 0 )
            {
                // do we have to grow the buffer??
                if ( ( bytesInBuffer + ret ) > bufferSize )
                    pData = ( unsigned char* ) realloc( pData, bytesInBuffer + ret );

                memcpy( pData + bytesInBuffer, szBuffer, ret );
                bytesInBuffer += ret;

                // already complete messagae?
                if ( bytesInBuffer >= headerSize )
                {
                    SCP_MSG_HDR_t* hdr = ( SCP_MSG_HDR_t* ) pData;

                    // is this message containing the valid magic number?
                    if ( hdr->magicNo != SCP_MAGIC_NO )
                    {
                        printf( "message receiving is out of sync; discarding data" );
                        bytesInBuffer = 0;
                    }

                    // Do I have a complete message_
                    while ( bytesInBuffer >= ( headerSize + hdr->dataSize ) )
                    {
                        unsigned int msgSize = headerSize + hdr->dataSize;

                        fprintf( stderr, "received SCP message with %d characters\n", hdr->dataSize );

                        char* pText = ( ( char* ) pData ) + headerSize;

                        // copy the string into a new buffer and null-terminate it

                        char* scpText = new char[ hdr->dataSize + 1 ];
                        memset( scpText, 0, hdr->dataSize + 1 );
                        memcpy( scpText, pText, hdr->dataSize );

                        // print the message
                        if ( sVerbose )
                            // fprintf( stderr, "message: ---%s---\n", scpText );
                            ui->scpControl->append(QString::fromUtf8(scpText));

                        // remove message from queue
                        memmove( pData, pData + msgSize, bytesInBuffer - msgSize );
                        bytesInBuffer -= msgSize;

                        bMsgComplete = true;
                    }
                }
            }
        }
        // all right I got a message
        count++;
    }
    ::close(sendSocket);
}
