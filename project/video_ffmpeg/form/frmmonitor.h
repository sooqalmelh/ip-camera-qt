#ifndef FRMMONITOR_H
#define FRMMONITOR_H

#include <QWidget>
#include <QUdpSocket>

namespace Ui {
class frmMonitor;
}

class frmMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit frmMonitor(QWidget *parent = 0);
    ~frmMonitor();

private:
    enum
    {
        CMD_OUT_MUSIC,
        CMD_INTER_MUSIC,
        CMD_CYCLE_MUSIC,
        CMD_NEXT_MUSIC,
        CMD_PRE_MUSIC,
        CMD_OPEN_MUSIC,
        CMD_CLOSE_MUSIC,
        CMD_OPEN_CAM,
        CMD_CLOSE_CAM,
        CMD_OPEN_LIGHT,
        CMD_CLOSE_LIGHT,
        CMD_OPEN_TABLE,
        CMD_CLOSE_TABLE,
        CMD_ROCKER_ANGLE,
        CMD_ROCKER_LEN,
    }CMD_E;

    typedef struct
    {
        union
        {
            char send[7];
            struct
            {
                quint8 START;
                quint8 AISLE;
                quint16 CMD;
                quint16 DATA;
                quint8 CHECKSUM;
            };
        };
    }cmd_t;


    quint8 aisle_select;

    Ui::frmMonitor *ui;
    QString ip;
    QUdpSocket *udpSocket;
    QString getLocalIP();

    cmd_t send_cmd;
    void send_command(quint16 CMD, quint16 DATA);
    quint8 msg[7];



private slots:
    void initForm();
    void initConfig();
    void saveConfig();

    //接收到拖曳文件
    void fileDrag(const QString &url);
    //工具栏单击
    void btnClicked(const QString &btnName);

    //播放成功
    void receivePlayStart();
    //播放失败
    void receivePlayError();
    //播放结束
    void receivePlayFinsh();

public slots:
    QString getUrl()const;
    void setUrl(const QString &url);
    void setName(const QString &name);
    void setHardware(const QString &hardware);

private slots:
    void on_ckOSD1_stateChanged(int arg1);
    void on_ckOSD2_stateChanged(int arg1);
    void on_cboxFont1_currentIndexChanged(int index);
    void on_cboxFont2_currentIndexChanged(int index);
    void on_cboxFormat1_currentIndexChanged(int index);
    void on_cboxFormat2_currentIndexChanged(int index);
    void on_cboxOSD1_currentIndexChanged(int index);
    void on_cboxOSD2_currentIndexChanged(int index);
    void on_txtOSD1_textChanged(const QString &arg1);
    void on_txtOSD2_textChanged(const QString &arg1);
    void on_btnOSD1_clicked();
    void on_btnOSD2_clicked();

    void on_btnOpen_clicked();
    void on_btnPause_clicked();
    void on_btnSnap_clicked();
    void on_btnScreen_clicked();
    void on_btnUrl_clicked();
    void on_btnSave_clicked();
    void on_btnFace_clicked();

    void on_ckSaveInterval_stateChanged(int arg1);
    void on_ckSaveTime_stateChanged(int arg1);
    void on_ckSaveHand_stateChanged(int arg1);

    void moveAngle(double angle, double distance);
    void on_btnCloseLight_clicked();
    void on_btnCloseMusic_clicked();
    void on_btnCloseTable_clicked();
    void on_btnInterMusic_clicked();
    void on_btnMusicCycle_clicked();
    void on_btnNextMusic_clicked();
    void on_btnOpenCam_clicked();
    void on_btnOpenLight_clicked();
    void on_btnOpenTable_clicked();
    void on_btnOutMusic_clicked();
    void on_btnPlayMusic_clicked();
    void on_btnPreMusic_clicked();
    void on_btnShutdownMusic_clicked();
    void on_btnUdpOpen_clicked();
};

#endif // FRMMONITOR_H
