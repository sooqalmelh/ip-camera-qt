#ifndef FRMVIDEO_H
#define FRMVIDEO_H

#include <QWidget>

namespace Ui {
class frmVideo;
}

class frmVideo : public QWidget
{
    Q_OBJECT

public:
    explicit frmVideo(QWidget *parent = 0);
    ~frmVideo();

private:
    Ui::frmVideo *ui;

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
};

#endif // FRMVIDEO_H
