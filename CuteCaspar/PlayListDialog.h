#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QDialog>

namespace Ui {
class PlayList;
}

class PlayList : public QDialog
{
    Q_OBJECT

public:
    explicit PlayList(QWidget *parent = nullptr);
    ~PlayList();
    void refreshMediaList();
    void refreshPlayList();

private slots:
    void on_clipList_doubleClicked(const QModelIndex &index);
    void on_clearPlaylistButton_clicked();
    void on_clipList_clicked(const QModelIndex &index);
    void on_cmbPlaylists_currentIndexChanged(int index);

signals:
    void playlistChanged();

private:
    Ui::PlayList *ui;
    QString m_playlist = "Playlist";
};

#endif // PLAYLIST_H
