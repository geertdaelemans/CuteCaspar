#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QDialog>
#include <QSqlQueryModel>

namespace Ui {
class PlayList;
}

class PlayList : public QDialog
{
    Q_OBJECT

public:
    explicit PlayList(QWidget *parent = nullptr);
    ~PlayList();
    void refreshLibraryList();
    void refreshPlayList();

private slots:
    void on_clipList_doubleClicked(const QModelIndex &index);
    void on_clearPlaylistButton_clicked();
    void on_clipList_clicked(const QModelIndex &index);
    void on_cmbPlaylists_currentIndexChanged(int index);
    void on_btnUp_clicked();
    void on_btnDown_clicked();
    void on_btnDelete_clicked();
    void on_btnAddToList_clicked();

signals:
    void playlistChanged();

private:
    Ui::PlayList *ui;
    QString m_playlist = "Playlist";
    QSqlQueryModel * modelPlayList = nullptr;
    void deleteRow(int row);
    bool isMidiPresent(QString clipName);
};

#endif // PLAYLIST_H
