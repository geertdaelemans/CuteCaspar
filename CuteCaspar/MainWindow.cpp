#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QtSql>
#include <QtMath>

#include "CasparDevice.h"
#include "DatabaseManager.h"
#include "SettingsDialog.h"
#include "PlayListDialog.h"

#include "MidiConnection.h"
#include "RaspberryPI.h"

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

    // Retrieve OSC Port Number
    unsigned short oscPort = 6250;
    QSettings settings("VRT", "CasparCGClient");
    settings.beginGroup("Configuration");
    oscPort = static_cast<unsigned short>(settings.value("osc_port", 6250).toInt());
    settings.endGroup();

    // Set-up UDP connection with device
    udp.bind(QHostAddress::Any, oscPort);

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
    settings.beginGroup("Configuration");
    if (settings.value("auto_connect", true).toBool())
        connectServer();
    settings.endGroup();

    // Prepare notes panel
    QFile profile;
    if(QFileInfo::exists("Notes.csv"))
    {
        profile.setFileName("Notes.csv");
    }
    else
    {
        profile.setFileName(":/Profiles/Notes.csv");
    }

    if (!profile.open(QIODevice::ReadOnly)) {
        qDebug() << profile.errorString();
    }
    int counter = 0;
    while (!profile.atEnd()) {
        QByteArray line = profile.readLine();
        note tempNote;
        tempNote.id = counter;
        tempNote.name = line.split(',').at(0);
        tempNote.pitch = line.split(',').at(1).toUInt();
        notes.append(tempNote);
        counter++;
    }
    // Magically calculate the number of rows
    int rows = qCeil(qSqrt(static_cast<qreal>(counter)));
    counter = 0;
    for (int i = 0; i < notes.length(); i++) {
        QPushButton* newButton = new QPushButton(notes[i].name);
        newButton->setProperty("pitch", notes[i].pitch);
        newButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(newButton, SIGNAL(pressed()),
                player, SLOT(playNote()));
        connect(newButton, SIGNAL(released()),
                player, SLOT(killNote()));
        ui->theGrid->addWidget(newButton, counter / rows, counter % rows);
        button[notes[i].pitch] = newButton;
        counter++;
    }

    MidiConnection* midiCon = MidiConnection::getInstance();

    connect(midiCon, SIGNAL(midiMessageReceived(unsigned int, bool)),
            player, SLOT(playNote(unsigned int, bool)));

    RaspberryPI::getInstance();
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

    connect(device, SIGNAL(connectionStateChanged(CasparDevice&)),
            this, SLOT(connectionStateChanged(CasparDevice&)));

    device->connectDevice();
    player = new Player(device);
    connect(player, SIGNAL(activeClip(int)),
            this, SLOT(setCurrentClip(int)));

    // Playlist: when next clip has started, load the following clip
    connect(this, SIGNAL(nextClip()),
            player, SLOT(loadNextClip()));

    connect(this, SIGNAL(currentTime(double)),
            player, SLOT(timecode(double)));

    connect(this, SIGNAL(currentTime(double)),
            this, SLOT(setTimeCode(double)));

    connect(player, SIGNAL(activeClipName(QString)),
            this, SLOT(activeClipName(QString)));

    // activateButton()
    connect(player, SIGNAL(activateButton(unsigned int)),
            this, SLOT(activateButton(unsigned int)));

    // deactivateButton()
    connect(player, SIGNAL(deactivateButton(unsigned int)),
            this, SLOT(deactivateButton(unsigned int)));

    connect(this, SIGNAL(setRecording()),
            player, SLOT(setRecording()));

    connect(this, SIGNAL(setRenew(bool)),
            player, SLOT(setRenew(bool)));

    connect(player, SIGNAL(playerStatus(PlayerStatus, bool)),
            this, SLOT(playerStatus(PlayerStatus, bool)));

}

void MainWindow::connectionStateChanged(CasparDevice& device) {
    if (device.isConnected()) {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        log("Server connected");
        listMedia();
    } else {
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
        log("Trying to connect to server");
    }
}

/**
 * @brief Clicked on Disconnect Server button
 * Disconnects host
 */
void MainWindow::disconnectServer()
{
    device->stop(1, 0);
    device->disconnectDevice();
    playPlaying = false;
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
    } else {
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
    }
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
    Q_UNUSED(values)

    QString adr = address.join("/");
//    qDebug() << address << values;
    if (adr == "channel/1/stage/layer/0/file/frame")
    {
        playCurFrame = values[0].toInt();
        playLastFrame = values[1].toInt();
        lastOsc = QDateTime::currentDateTime();
//        qDebug() << playCurFrame << "of" << playLastFrame;
    }
    if (adr == "channel/1/stage/layer/0/file/time") {
        emit currentTime(values[0].toDouble());
    }
}

void MainWindow::listMedia()
{
    device->refreshMedia();
    connect(device, SIGNAL(mediaChanged(const QList<CasparMedia>&, CasparDevice&)),
            this, SLOT(mediaChanged(const QList<CasparMedia>&, CasparDevice&)), Qt::UniqueConnection);
}

void MainWindow::mediaChanged(const QList<CasparMedia>& mediaItems, CasparDevice& device)
{
    Q_UNUSED(device)
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
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery* qry = new QSqlQuery();

    qry->prepare("select Timecode, TypeId, Fps, Name from Playlist");
    qry->exec();

    model->setQuery(*qry);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->sort(1, Qt::AscendingOrder);

    ui->tableView->setModel(proxyModel);
    ui->tableView->selectRow(0);

    currentClipIndex = ui->tableView->selectionModel()->currentIndex().row();
    currentClip = ui->tableView->selectionModel()->currentIndex().siblingAtColumn(3).data(Qt::DisplayRole).toString();

    player->loadPlayList();
}

void MainWindow::on_tableView_clicked(const QModelIndex &index)
{
    const QModelIndex name = index.sibling(index.row(), 3);
    currentClip = name.data(Qt::DisplayRole).toString();
    currentClipIndex = index.row();
    ui->statusLabel->setText("Video selected...");
}

/**
 * PANELS
 */

void MainWindow::on_actionSettings_triggered()
{
    m_settingsDialog = new SettingsDialog();
    m_settingsDialog->exec();
}

void MainWindow::on_actionMidi_Panel_triggered()
{
    m_midiPanelDialog = new MidiPanelDialog();
    m_midiPanelDialog->show();

    // Send notes to player
    connect(m_midiPanelDialog, SIGNAL(buttonPushed(unsigned int, bool)),
            player, SLOT(playNote(unsigned int, bool)));

    // Receive activateButton()
    connect(player, SIGNAL(activateButton(unsigned int)),
            m_midiPanelDialog, SLOT(activateButton(unsigned int)));

    // Receive deactivateButton()
    connect(player, SIGNAL(deactivateButton(unsigned int)),
            m_midiPanelDialog, SLOT(deactivateButton(unsigned int)));
}


void MainWindow::on_actionPreview_toggled(bool visible)
{
    ui->boxServer->setVisible(visible);
}

void MainWindow::activateButton(unsigned int pitch)
{
    if (button.contains(pitch)) {
        button[pitch]->setDown(true);
    }
}

void MainWindow::deactivateButton(unsigned int pitch)
{
    if (button.contains(pitch)) {
        button[pitch]->setDown(false);
    }
}

void MainWindow::on_actionPlaylist_triggered()
{
    PlayList* playlistDialog = new PlayList(this);
    playlistDialog->exec();
    refreshMediaList();
}


void MainWindow::on_btnStartPlaylist_clicked()
{
    if (player->getStatus() == PlayerStatus::READY) {
        player->startPlayList(currentClipIndex);
    } else if (player->getStatus() == PlayerStatus::PLAYLIST_PLAYING) {
        player->pausePlayList();
    } else if (player->getStatus() == PlayerStatus::PLAYLIST_PAUSED) {
        player->resumePlayList();
    }
}


void MainWindow::on_btnStopPlaylist_clicked()
{
    player->stopPlayList();
}


void MainWindow::on_btnPlayClip_clicked()
{
    if (player->getStatus() == PlayerStatus::READY) {
        player->playClip(currentClipIndex);
    } else if (player->getStatus() == PlayerStatus::CLIP_PLAYING) {
        player->pausePlayList();
    } else if (player->getStatus() == PlayerStatus::CLIP_PAUSED) {
        player->resumePlayList();
    }
}


void MainWindow::setCurrentClip(int index)
{
    ui->tableView->selectRow(index);
}

void MainWindow::setTimeCode(double time)
{
    timecode = Timecode::fromTime(time, 29.97, false);
    ui->timeCode->setText(timecode);
}

void MainWindow::activeClipName(QString clipName)
{
    currentClip = clipName;
    qDebug() << "Current clip:" << clipName;
    ui->clipName->setText(clipName);
}

void MainWindow::on_btnRecording_clicked()
{
    emit setRecording();
}

void MainWindow::on_renewCheckBox_stateChanged(int arg1)
{
    emit setRenew(arg1);
}

void MainWindow::playerStatus(PlayerStatus status, bool recording)
{
    switch (status) {
    case PlayerStatus::IDLE:
        ui->statusLabel->setText("IDLE");
        break;
    case PlayerStatus::READY:
        ui->statusLabel->setText("READY");
        ui->btnPlayClip->setText(tr("Play Clip"));
        ui->btnStartPlaylist->setText(tr("Start Playlist"));
        if (recording) {
            ui->btnRecording->setText(tr("Ready"));
            setButtonColor(ui->btnRecording, Qt::darkGreen);
        } else {
            ui->btnRecording->setText(tr("Not Recording"));
            setButtonColor(ui->btnRecording, QColor(53,53,53));
        }
        break;
    case PlayerStatus::CLIP_PLAYING:
        ui->statusLabel->setText("CLIP PLAYING");
        ui->btnPlayClip->setText(tr("Pause Clip"));
        if (recording) {
            ui->btnRecording->setText(tr("Recording"));
            setButtonColor(ui->btnRecording, Qt::red);
        } else {
            ui->btnRecording->setText(tr("Not Recording"));
            setButtonColor(ui->btnRecording, QColor(53,53,53));
        }
        break;
    case PlayerStatus::CLIP_PAUSED:
        ui->statusLabel->setText("CLIP PAUSED");
        ui->btnPlayClip->setText(tr("Resume Clip"));
        break;
    case PlayerStatus::PLAYLIST_PLAYING:
        ui->statusLabel->setText("PLAYLIST LOADED");
        ui->btnStartPlaylist->setText(tr("Pause Playlist"));
        if (recording) {
            ui->btnRecording->setText(tr("Recording"));
            setButtonColor(ui->btnRecording, Qt::red);
        } else {
            ui->btnRecording->setText(tr("Not Recording"));
            setButtonColor(ui->btnRecording, QColor(53,53,53));
        }
        break;
    case PlayerStatus::PLAYLIST_PAUSED:
        ui->statusLabel->setText("PLAYLIST LOADED");
        ui->btnStartPlaylist->setText(tr("Resume Playlist"));
        break;
    }
}

void MainWindow::setButtonColor(QPushButton* button, QColor color)
{
    QPalette pal = button->palette();
    pal.setColor(QPalette::Button, color);
    button->setPalette(pal);
    button->update();
}
