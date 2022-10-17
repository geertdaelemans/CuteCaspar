#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QtSql>
#include <QtMath>

#include "DatabaseManager.h"
#include "PlayListDialog.h"

#include "Models/LibraryModel.h"

#include "Timecode.h"

#include "qmidiin.h"
#include "qmidiout.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
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

    midiCon = MidiConnection::getInstance();
    m_raspberryPI = RaspberryPI::getInstance();

    // When AutoConnect is active connect immediately to server
    settings.beginGroup("Configuration");
    if (settings.value("auto_connect", true).toBool())
        connectServer();
    settings.endGroup();

    connect(this, SIGNAL(parseMessage(QString)),
            m_raspberryPI, SLOT(parseMessage(QString)));
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
    player = Player::getInstance();
    player->setDevice(device);

    connect(midiCon, SIGNAL(midiMessageReceived(unsigned int, bool)),
            player, SLOT(playNote(unsigned int, bool)));

    connect(player, SIGNAL(activeClip(int)),
            this, SLOT(setCurrentClip(int)));

    // Playlist: when next clip has started, load the following clip
    connect(this, SIGNAL(nextClip()),
            player, SLOT(loadNextClip()));

    connect(this, SIGNAL(currentTime(double, double, int)),
            player, SLOT(timecode(double, double, int)));

    connect(this, SIGNAL(currentFrame(int, int)),
            player, SLOT(currentFrame(int, int)));

    connect(this, SIGNAL(currentTime(double, double, int)),
            this, SLOT(setTimeCode(double, double, int)));

    connect(player, SIGNAL(activeClipName(QString, QString, bool)),
            this, SLOT(activeClipName(QString, QString, bool)));

    connect(this, SIGNAL(setRecording()),
            player, SLOT(setRecording()));

    connect(player, SIGNAL(playerStatus(PlayerStatus, bool)),
            this, SLOT(playerStatus(PlayerStatus, bool)));

    connect(m_raspberryPI, SIGNAL(insertPlaylist(QString)),
            player, SLOT(insertPlaylist(QString)));

    connect(player, SIGNAL(insertFinished()),
            m_raspberryPI, SLOT(insertFinished()));
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
    if (buffer.mid(0, 5) == "raspi") {
        emit parseMessage(&buffer.data()[6]);
    } else {
        listener.ProcessPacket(buffer.data(), buffer.size(), IpEndpointName(sender.toIPv4Address(), senderPort));
    }
}

/**
 * @brief Process recieved OSC messages
 * @param address - An OSC Address Pattern in StringList
 * @param values - A list of OSC values
 */
void MainWindow::processOsc(QStringList address, QStringList values)
{
    QString adr = address.join("/");
    if (adr == "channel/1/stage/layer/2/file/frame") {  // TO DO: this only needs default video frames, must become parameter
        emit currentFrame(values[0].toInt(), values[1].toInt());
    } else if (QRegularExpression("channel/1/stage/layer/./file/time").match(adr).hasMatch()) {
        emit currentTime(values[0].toDouble(), values[1].toDouble(), address[4].toInt());
    } else if (QRegularExpression("channel/1/stage/layer/./foreground/file/time").match(adr).hasMatch()) {
        emit currentTime(values[0].toDouble(), values[1].toDouble(), address[4].toInt());
    } else {
//      qDebug() << address << values;
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

    qry->prepare("select Timecode, TypeId, Fps, Name, Midi from Playlist");
    qry->exec();

    model->setQuery(*qry);

    // Set proxy model to enable sorting columns:
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->sort(1, Qt::AscendingOrder);

    ui->tableView->setModel(proxyModel);
    ui->tableView->setColumnWidth(0, 100);
    ui->tableView->hideColumn(1);
    ui->tableView->hideColumn(2);
    ui->tableView->setColumnWidth(3, 400);
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
    emit clipNameSelected(currentClip);
}



void MainWindow::on_actionPreview_toggled(bool visible)
{
    ui->boxServer->setVisible(visible);
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

void MainWindow::setTimeCode(double time, double duration, int videoLayer)
{
    if (videoLayer == 2) {    // TO DO: this is the default video layer, should become parameter
        timecode = Timecode::fromTime(time, 29.97, false);
        ui->timeCode->setText(timecode);
        ui->lblDuration->setText(Timecode::fromTime(duration - time, 29.97, false));
        ui->progressBar->setValue(static_cast<int>(100 - (duration - time)/duration * 100));
    }
}

void MainWindow::activeClipName(QString clipName, QString upcoming, bool insert)
{
    currentClip = clipName;
    if (!insert) {
        ui->clipName->setStyleSheet("QLabel { color : white; }");
        ui->clipName->setText(clipName);
    } else {
        ui->clipName->setStyleSheet("QLabel { color : red; }");
        ui->clipName->setText("INSERT\n" + clipName);
    }
    if (upcoming == clipName) {
        ui->lblUpcomingClip->setText("---");
    } else {
        ui->lblUpcomingClip->setText(upcoming);
    }
}

void MainWindow::on_btnRecording_clicked()
{
    emit setRecording();
}

void MainWindow::on_chkRandom_stateChanged(int random)
{
    Player::getInstance()->setRandom(random);
}

void MainWindow::on_chkTriggers_stateChanged(int triggers)
{
    Player::getInstance()->setTriggersActive(triggers);
}

void MainWindow::playerStatus(PlayerStatus status, bool isRecording)
{
    switch (status) {
    case PlayerStatus::IDLE:
        ui->statusLabel->setText("IDLE");
        break;
    case PlayerStatus::READY:
        ui->statusLabel->setText("READY");
        ui->btnPlayClip->setText(tr("Play Clip"));
        ui->btnStartPlaylist->setText(tr("Start Playlist"));
        if (isRecording) {
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
        if (isRecording) {
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
        ui->statusLabel->setText("PLAYLIST PLAYING");
        ui->btnStartPlaylist->setText(tr("Pause Playlist"));
        if (isRecording) {
            ui->btnRecording->setText(tr("Recording"));
            setButtonColor(ui->btnRecording, Qt::red);
        } else {
            ui->btnRecording->setText(tr("Not Recording"));
            setButtonColor(ui->btnRecording, QColor(53,53,53));
        }
        break;
    case PlayerStatus::PLAYLIST_PAUSED:
        ui->statusLabel->setText("PLAYLIST PAUSED");
        ui->btnStartPlaylist->setText(tr("Resume Playlist"));
        break;
    case PlayerStatus::PLAYLIST_INSERT:
        ui->statusLabel->setText("PLAYLIST INSERT");
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

void MainWindow::on_pushButton_clicked()
{
    player->insertPlaylist();
}

/**************************************************************
 * PANELS
 **************************************************************/

/**
 * @brief MainWindow::on_actionSettings_triggered
 */
void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog::getInstance()->exec();
}


/**
 * @brief MainWindow::on_actionControl_Panel_triggered
 */
void MainWindow::on_actionControl_Panel_triggered()
{
    if (!m_controlDialog) {
        m_controlDialog = new ControlDialog();

        // Send selected clips to Player
        connect(m_controlDialog, SIGNAL(insertPlaylist(QString)),
                player, SLOT(insertPlaylist(QString)));
    }
    if (!m_controlDialog->isVisible()) {
        m_controlDialog->setup();
        m_controlDialog->show();
        m_controlDialog->activateWindow();
    } else {
        m_controlDialog->hide();
    }
}


/**
 * @brief MainWindow::on_actionMIDI_Editor_triggered
 */
void MainWindow::on_actionMIDI_Editor_triggered()
{
    if (!m_midiEditorDialog) {
        m_midiEditorDialog = new MidiEditorDialog();

        // Retrieve current MIDI playlist from Player
        connect(player, SIGNAL(newMidiPlaylist(QMap<QString, message>)),
                m_midiEditorDialog, SLOT(newMidiPlaylist(QMap<QString, message>)));

        // Actual note being played at the moment
        connect(player, SIGNAL(currentNote(QString, bool, unsigned int)),
                m_midiEditorDialog, SLOT(currentNote(QString, bool, unsigned int)));

        // Action when new clip is being played
        connect(player, SIGNAL(activeClipName(QString, QString, bool)),
                m_midiEditorDialog, SLOT(activeClipName(QString, QString, bool)));

        // Action when new clip is selected
        connect(this, SIGNAL(clipNameSelected(QString)),
                m_midiEditorDialog, SLOT(setClipName(QString)));

        // Players reports status
        connect(player, SIGNAL(playerStatus(PlayerStatus, bool)),
                m_midiEditorDialog, SLOT(playerStatus(PlayerStatus, bool)));
    }
    if (!m_midiEditorDialog->isVisible()) {
        m_midiEditorDialog->show();
        m_midiEditorDialog->activateWindow();
    } else {
        m_midiEditorDialog->hide();
    }
    emit clipNameSelected(currentClip);
}


/**
 * @brief MainWindow::on_actionMidi_Panel_triggered
 */
void MainWindow::on_actionMidi_Panel_triggered()
{
    if (!m_midiPanelDialog) {
        m_midiPanelDialog = new MidiPanelDialog();

        // Send notes to player
        connect(m_midiPanelDialog, SIGNAL(buttonPushed(unsigned int, bool)),
                player, SLOT(playNote(unsigned int, bool)));

        // Receive activateButton()
        connect(player, SIGNAL(activateButton(unsigned int, bool)),
                m_midiPanelDialog, SLOT(activateButton(unsigned int, bool)));
    }
    if (!m_midiPanelDialog->isVisible()) {
        m_midiPanelDialog->show();
        m_midiPanelDialog->activateWindow();
    } else {
        m_midiPanelDialog->hide();
    }
}


/**
 * @brief MainWindow::on_actionRaspberryPI_triggered
 */
void MainWindow::on_actionRaspberryPI_triggered()
{
    if (!m_raspberryPIDialog) {
        m_raspberryPIDialog = new RaspberryPIDialog();

        // Update remote button status
        connect(m_raspberryPI, SIGNAL(statusButton(QString)),
                m_raspberryPIDialog, SLOT(statusButton(QString)));

        // Update other statuses
        connect(m_raspberryPI, SIGNAL(statusUpdate(status)),
                m_raspberryPIDialog, SLOT(refreshUpdate(status)));
    }
    if (!m_raspberryPIDialog->isVisible()) {
        m_raspberryPIDialog->show();
        m_raspberryPIDialog->activateWindow();
    } else {
        m_raspberryPIDialog->hide();
    }
}

void MainWindow::on_btnNext_clicked()
{
    Player::getInstance()->nextClip();
}
