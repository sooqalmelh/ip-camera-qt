#ifndef FRMPLAYER_H
#define FRMPLAYER_H

#include <QWidget>

namespace Ui
{
    class frmPlayer;
}

class frmPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit frmPlayer(QWidget *parent = 0);
    ~frmPlayer();

protected:
    void fullScreen();
    void keyReleaseEvent(QKeyEvent *key);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Ui::frmPlayer *ui;
    bool full;

private slots:
    void initForm();
    void initConfig();
    void saveConfig();

private slots:
    //接收到拖曳文件
    void fileDrag(const QString &url);

    //播放成功
    void receivePlayStart();
    //播放失败
    void receivePlayError();
    //播放结束
    void receivePlayFinsh();

    //总时长
    void fileLengthReceive(qint64 length);
    //当前播放时长
    void filePositionReceive(qint64 position);
    //音量大小
    void fileVolumeReceive(int volume, bool muted);    

private slots:
    void on_btnPlay_clicked();
    void on_btnStop_clicked();
    void on_btnPause_clicked();
    void on_btnNext_clicked();

    void on_btnSelect_clicked();
    void on_btnVersion_clicked();
    void on_btnAbout_clicked();
    void on_ckMuted_clicked();

    void on_sliderVolume_clicked();
    void on_sliderVolume_sliderMoved(int value);
    void on_sliderPosition_clicked();
    void on_sliderPosition_sliderMoved(int value);
    void on_cboxRate_currentIndexChanged(int index);

signals:
    void fullScreen(bool full);
};

#endif // FRMPLAYER_H
