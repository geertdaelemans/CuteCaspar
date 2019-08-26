#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QtSql>

#include "CasparDevice.h"
#include "DatabaseManager.h"
#include "SettingsDialog.h"
#include "Models/LibraryModel.h"

#include "Timecode.h"

#include "qmidiin.h"
#include "qmidiout.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    playPlaying = false;
    ui->setupUi(this);

    // Set-up UDP connection with device
    udp.bind(QHostAddress::Any, 6250);

    // Data is available for reading from the device
    connect(&udp, SIGNAL(readyRead()),
            this, SLOT(readyRead()));

    // CasparCG OSC listener received data
    connect(&listener, SIGNAL(messageAvailable(QStringList,QStringList)),
            this, SLOT(processOsc(QStringList,QStringList)));

    // Refresh the Media ListView when new data is available
    connect(this, SIGNAL(mediaListUpdated()),
            this, SLOT(refreshMediaList()));

    // When AutoConnect is active connect immediately to server
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    if (settings.value("auto_connect", true).toBool())
        connectServer();
    settings.endGroup();


    // Prepare MIDI connection
    QMidiIn* midiIn = new QMidiIn(this);
    midiOut = new QMidiOut(this);

    qDebug() << "MIDI inputs" << midiIn->getPorts();
    qDebug() << "MIDI outputs" << midiOut->getPorts();

    ui->inPortComboBox->addItems(midiIn->getPorts());
    ui->outPortComboBox->addItems(midiOut->getPorts());

    // Prepare notes panel
    QFile profile(":/Profiles/Notes.csv");
    if (!profile.open(QIODevice::ReadOnly)) {
        qDebug() << profile.errorString();
    }

    while (!profile.atEnd()) {
        QByteArray line = profile.readLine();
        notes.insert(line.split(',').at(0), line.split(',').at(1).toUInt());
        QPushButton *button = new QPushButton(line.split(',').at(0));
        button->setProperty("pitch", line.split(',').at(1).toInt());
        connect(button, SIGNAL(pressed()), this, SLOT(playNote()));
        connect(button, SIGNAL(released()), this, SLOT(killNote()));
        ui->theGrid->addWidget(button);
    }
}

/**
 * @brief Destructor of MainWindow
 * At destruction write all settings to the registry
 */
MainWindow::~MainWindow()
{
    disconnectServer();
    delete ui;
}

/**
 * @brief Log messages to the Log tab
 * @param message - message to be logged
 */
void MainWindow::log(QString message)
{
    ui->edtLog->moveCursor(QTextCursor::End);
    ui->edtLog->textCursor().insertText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + ": " + message + "\n");
    ui->edtLog->moveCursor(QTextCursor::End);
    statusBar()->showMessage(message);
}

/**
 * @brief Clicked on Connect Server button
 * Connect to host using the edtHost field
 */
void MainWindow::connectServer()
{
    // Get server credentials from registry
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    QString address = settings.value("host", "127.0.0.1").toString();
    quint16 port = static_cast<quint16>(settings.value("port", "5250").toInt());
    settings.endGroup();
    device = new CasparDevice(address, port);

//    emit deviceAdded(*device);
    connect(device, SIGNAL(connectionStateChanged(CasparDevice&)), this, SLOT(connectionStateChanged(CasparDevice&)));

    device->connectDevice();
}

void MainWindow::connectionStateChanged(CasparDevice& device) {
    if (device.isConnected()) {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        log("Server connected");
        listMedia();
    } else {
        ui->btnPlay->setEnabled(false);
        ui->btnStop->setEnabled(false);
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
        log("Trying to connect to server");
    };
}

/**
 * @brief Clicked on Disconnect Server button
 * Disconnects host
 */
void MainWindow::disconnectServer()
{
    on_btnStop_clicked();
    device->disconnectDevice();
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    DatabaseManager::getInstance().reset();
    refreshMediaList();
    delete device;
    log("Disconnected from server");
}

/**
 * @brief Slot called when TCP status with server has changed
 * Enables/disables the connection buttons.
 * Possible reported states: "unconnected", "host lookup", "connecting", "connected", "bound",
 * "closing" and "listening"
 * @param socketState
 */
void MainWindow::onTcpStateChanged(QAbstractSocket::SocketState socketState)
{
    QStringList states = {"unconnected", "host lookup", "connecting", "connected", "bound", "closing", "listening"};
    log("Connection status: " + states[socketState]);
    if (socketState == QAbstractSocket::UnconnectedState) {
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
        ui->btnPlay->setEnabled(false);
        ui->btnStop->setEnabled(false);
    } else {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
    };
}

/**
 * @brief MainWindow::readyRead
 * Data is available for reading from device. This data will be read and
 * forwarded to th OSC listener.
 */
void MainWindow::readyRead()
{
    QByteArray buffer;
    buffer.resize(static_cast<int>(udp.pendingDatagramSize()));
    QHostAddress sender;
    quint16 senderPort;
    udp.readDatagram(buffer.data(), buffer.size(), &sender, &senderPort);
    listener.ProcessPacket(buffer.data(), buffer.size(), IpEndpointName(sender.toIPv4Address(), senderPort));
}

/**
 * @brief Process recieved OSC messages
 * @param address - An OSC Address Pattern in StringList
 * @param values - A list of OSC values
 */
void MainWindow::processOsc(QStringList address, QStringList values)
{
    Q_UNUSED(values);
    QString adr = address.join("/");
//    qDebug() << address << values;
    if (adr == "channel/1/stage/layer/0/file/frame")
    {
        playCurFrame = values[0].toInt();
        playLastFrame = values[1].toInt();
        lastOsc = QDateTime::currentDateTime();
//        qDebug() << playCurFrame << "of" << playLastFrame;
    }
    if (adr == "channel/1/stage/layer/0/file/time")
    {
//        qDebug() << values[0].toDouble() << "of" << values[1].toDouble();
        double time = values[0].toDouble();
        QString timecode = Timecode::fromTime(time, 29.97, false);
        ui->timeCode->setText(timecode);
        //qDebug() << timecode;
    }
    if (adr == "channel/1/output/file/frame")
//        ui->lcdFrame->display(values[0].toInt());
        qDebug() << "Test" << values[0].toInt();
//    if (adr == "channel/2/output/file/fps")
//    {
//        if (recordFps != values[0].toInt())
//        {
//            recordFps = values[0].toInt();
//            ui->lblRecordFps->setText(QString::number(recordRealFps) + "fps/" + QString::number(recordFps) + "fps");
//        }
//    }
//    else if (adr == "channel/" + ui->edtChannel->text() + "/stage/layer/" + ui->edtLayer->text() + "/file/path")
//        playPath = values[0];
//    else if (adr == "channel/" + ui->edtChannel->text() + "/stage/layer/" + ui->edtLayer->text() + "/file/speed")
//    {
//        playSpeed = values[0].toFloat();
//        if (!sliderPressed && playSpeed != ui->vslSpeed->value()/10)
//            ui->vslSpeed->setValue(playSpeed*10);
//    }
//    else if (adr == "channel/" + ui->edtChannel->text() + "/stage/layer/" + ui->edtLayer->text() + "/file/frame")
//    {
//        playCurFrame = values[0].toInt();
//        playLastFrame = values[1].toInt();
//        lastOsc = QDateTime::currentDateTime();
//    }
//    else if (adr == "channel/" + ui->edtChannel->text() + "/stage/layer/" + ui->edtLayer->text() + "/file/vframe")
//    {
//        playCurVFrame = values[0].toInt();
//        playLastVFrame = values[1].toInt();
//    }
//    else if (adr == "channel/" + ui->edtChannel->text() + "/stage/layer/" + ui->edtLayer->text() + "/file/fps")
    //        playFps = values[0].toInt();
}

void MainWindow::listMedia()
{
    device->refreshMedia();
    connect(device, SIGNAL(mediaChanged(const QList<CasparMedia>&, CasparDevice&)), this, SLOT(mediaChanged(const QList<CasparMedia>&, CasparDevice&)), Qt::UniqueConnection);
}

void MainWindow::mediaChanged(const QList<CasparMedia>& mediaItems, CasparDevice& device)
{
    Q_UNUSED(device);
    QTime time;
    time.start();

//    if(mediaItems.size()) {
//        for(int i = 0; i < mediaItems.size(); i++) {
//            ui->listMedia->addItem(mediaItems[i].getName());
//            mediaItems[i].getType();
//            mediaItems[i].getTimecode();
//            DatabaseManager::getInstance().updateLibraryMedia(mediaItems[i].getName());
//        }
//    }

    QList<LibraryModel> insertModels;
    QList<LibraryModel> deleteModels;
//    QList<LibraryModel> libraryModels = DatabaseManager::getInstance().getLibraryMediaByDeviceAddress(device.getAddress());

//    DatabaseManager::getInstance

//    // Find library items to delete.
//    foreach (const LibraryModel& libraryModel, libraryModels)
//    {
//        bool found = false;
//        foreach (CasparMedia mediaItem, mediaItems)
//        {
//            if (mediaItem.getName() == libraryModel.getName())
//                found = true;
//            qDebug() << mediaItem.getName();
//        }

//        if (!found)
//            deleteModels.push_back(libraryModel);
//    }

    // Find library items to insert.
    foreach (CasparMedia mediaItem, mediaItems)
    {
        bool found = false;
//        foreach (const LibraryModel& libraryModel, libraryModels)
//        {
//            if (libraryModel.getName() == mediaItem.getName())
//                found = true;
//        }

        if (!found)
            insertModels.push_back(LibraryModel(0, mediaItem.getName(), mediaItem.getName(), "", mediaItem.getType(), 0, mediaItem.getTimecode(), mediaItem.getFPS()));
    }

    if (deleteModels.count() > 0 || insertModels.count() > 0)
    {
//        DatabaseManager::getInstance().updateLibraryMedia(device.getAddress(), deleteModels, insertModels);
        DatabaseManager::getInstance().updateLibraryMedia(insertModels);

//        EventManager::getInstance().fireMediaChangedEvent(MediaChangedEvent());
        emit mediaListUpdated();
    }

        qDebug("LibraryManager::mediaChanged %d msec", time.elapsed());
}

void MainWindow::on_btnPlay_clicked()
{
    if (playPlaying) {
        device->pause(1, 0);
        ui->btnPlay->setText("Play");
        playPlaying = false;
    } else {
        device->play(1, 0);
        ui->btnStop->setEnabled(true);
        ui->btnPlay->setText("Pause");
        playPlaying = true;
        if (newClipLoaded)
            newClipLoaded = false;
    }
}

void MainWindow::on_btnStop_clicked()
{
    device->stop(1, 0);
    ui->btnPlay->setText("Play");
    ui->btnPlay->setEnabled(false);
    ui->btnStop->setEnabled(false);
    ui->txtClip->setText("<none>");
    playPlaying = false;
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::exit();
}

void MainWindow::on_actionConnect_triggered()
{
    connectServer();
}

void MainWindow::on_actionDisconnect_triggered()
{
    disconnectServer();
}

void MainWindow::refreshMediaList()
{
    QSqlQueryModel * model = new QSqlQueryModel();

    QSqlQuery* qry = new QSqlQuery();

    qry->prepare("select Timecode, TypeId, Fps, Name from Library");
    qry->exec();

    model->setQuery(*qry);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->sort(1, Qt::AscendingOrder);

    ui->tableView->setModel(proxyModel);
}

void MainWindow::on_tableView_clicked(const QModelIndex &index)
{
    const QModelIndex name = index.sibling(index.row(), 3);
    QString clip = name.data(Qt::DisplayRole).toString();
    device->loadMovie(1, 0, clip, "", 0, "", "", 0, 0, false, false, true);
    ui->txtClip->setText(clip);
    ui->btnPlay->setText("Pause");
    ui->btnPlay->setEnabled(true);
    ui->btnStop->setEnabled(true);
    if (playPlaying) {
        newClipLoaded = true;
    }
    else {
        playPlaying = true;
    }
    log(QString("Loaded clip: %1").arg(clip));
}

void MainWindow::on_actionSettings_triggered()
{
    m_settingsDialog = new SettingsDialog();
    m_settingsDialog->exec();
}

void MainWindow::on_actionPreview_toggled(bool visible)
{
    ui->boxServer->setVisible(visible);
}

void MainWindow::on_outPortComboBox_currentIndexChanged(int index)
{
    qDebug() << "Opening MIDI output" << ui->outPortComboBox->itemText(index);
    if(midiOut->isPortOpen()) {
        midiOut->closePort();
        qDebug() << "MIDI port already open";
    }
    midiOut->openPort(static_cast<unsigned int>(index));
}

void MainWindow::playNote(unsigned int pitch)
{
    if (pitch == 128) {
        auto button = qobject_cast<QPushButton *>(sender());
        if (button && button->property("pitch").isValid()) {
            pitch = button->property("pitch").toUInt();
        }
        else {
            pitch = 60;
        }
    }

    QMidiMessage *message = new QMidiMessage(this);
    message->setChannel(1);
    message->setStatus(MIDI_NOTE_ON);
    message->setPitch(pitch);
    message->setVelocity(60);

    qDebug() << "ON: pitch" << message->getPitch() << "velocity" << message->getVelocity();

    midiOut->sendMessage(message);
}

void MainWindow::killNote(unsigned int pitch)
{
    if (pitch == 128) {
        auto button = qobject_cast<QPushButton *>(sender());
        if (button && button->property("pitch").isValid()) {
            pitch = button->property("pitch").toUInt();
        }
        else {
            pitch = 60;
        }
    }

    QMidiMessage *message = new QMidiMessage(this);
    message->setChannel(1);
    message->setStatus(MIDI_NOTE_OFF);
    message->setPitch(pitch);
    message->setVelocity(0);

    qDebug() << "OFF: pitch" << message->getPitch();

    midiOut->sendMessage(message);
}
