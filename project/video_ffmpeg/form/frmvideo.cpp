/**
 * @file frmvideo.cpp
 * @author creekwater
 * @brief
 * @version 0.1
 * @date 2022-05-28
 *
 * 这个是所有TAB界面的核心
 *
 * 1、4个视频流播放器，自行填入RTSP视频流地址
 * rtsp://admin:密码@192.168.3.170:554
 *
 * 2、填入本地视频文件，可以播放本地视频
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "frmvideo.h"
#include "ui_frmvideo.h"
#include "head.h"
#include "ffmpegwidget.h"

frmVideo::frmVideo(QWidget *parent) : QWidget(parent), ui(new Ui::frmVideo)
{
    ui->setupUi(this);
    this->initForm();
    QTimer::singleShot(100, this, SLOT(initConfig()));
}

frmVideo::~frmVideo()
{
    delete ui;
}

void frmVideo::initForm()
{
    // 设置视频流默认地址，下拉菜单
    QStringList urls;
    ui->cboxUrl->addItems(AppUrl::getUrls(urls));

    QFont font;
    font.setPixelSize(qApp->font().pixelSize() + 2);
    font.setBold(true);

#ifndef Q_OS_ANDROID
    ui->btnOpen->setMaximumHeight(qApp->font().pixelSize() + qApp->font().pixelSize()/2);
    ui->btnPause->setMaximumHeight(qApp->font().pixelSize() + qApp->font().pixelSize()/2);
    ui->btnSnap->setMaximumHeight(qApp->font().pixelSize() + qApp->font().pixelSize()/2);
    ui->btnScreen->setMaximumHeight(qApp->font().pixelSize() + qApp->font().pixelSize()/2);
    ui->btnFace->setMaximumHeight(qApp->font().pixelSize() + qApp->font().pixelSize()/2);
    ui->pushButton->setMaximumHeight(qApp->font().pixelSize() + qApp->font().pixelSize()/2);
#endif

    ui->btnOSD1->setFont(font);
    ui->btnOSD2->setFont(font);
    ui->btnOSD1->setStyleSheet(QString("background:#000000;color:%1;").arg(ui->btnOSD1->text()));
    ui->btnOSD2->setStyleSheet(QString("background:#000000;color:%1;").arg(ui->btnOSD2->text()));
    ui->cboxOSD1->setCurrentIndex(ui->cboxOSD1->findText("右上角"));
    ui->cboxOSD2->setCurrentIndex(ui->cboxOSD2->findText("左下角"));

    //设置背景图片或者背景文字
    ui->playWidget->setBgText("视频监控");
    ui->playWidget->setBgImage(QImage(":/image/bg_novideo.png"));

    //设置OSD图片
    ui->playWidget->setOSD1Image(QImage(":/image/bg_novideo.png"));
    ui->playWidget->setOSD2Image(QImage(":/image/bg_novideo.png"));

    //设置边框宽度和颜色
    ui->playWidget->setBorderWidth(3);
    ui->playWidget->setBorderColor(QColor(0, 0, 0));

    //设置悬浮条可见,绑定顶部工具栏按钮单击事件
    ui->playWidget->setFlowEnable(true);
    connect(ui->playWidget, SIGNAL(fileDrag(QString)), this, SLOT(fileDrag(QString)));
    connect(ui->playWidget, SIGNAL(btnClicked(QString)), this, SLOT(btnClicked(QString)));
    ui->tabWidget->setCurrentIndex(0);

    //设置自动重连
    //ui->playWidget->setCheckLive(true);

    //绑定开始播放+停止播放+截图信号
    connect(ui->playWidget, SIGNAL(receivePlayStart()), this, SLOT(receivePlayStart()));
    connect(ui->playWidget, SIGNAL(receivePlayError()), this, SLOT(receivePlayError()));
    connect(ui->playWidget, SIGNAL(receivePlayFinsh()), this, SLOT(receivePlayFinsh()));

    //安卓屏幕比较小尽量空出位置
#ifdef Q_OS_ANDROID
    AppConfig::VideoConfig2.callback = true;

    // 跑在安卓平板端，不需要腾出位置
#if 0
    ui->ckSaveFile->setVisible(false);
    ui->ckSaveInterval->setVisible(false);
    ui->ckSaveTime->setVisible(false);
    ui->ckSaveHand->setVisible(false);
    ui->labFillImage->setVisible(false);
    ui->cboxFillImage->setVisible(false);
    ui->btnOSD1->setVisible(false);
    ui->btnOSD2->setVisible(false);
    this->layout()->setContentsMargins(0, 0, 0, 0);
#endif
#endif
}

void frmVideo::initConfig()
{
    VideoConfig videoConfig;
    QString objName = this->objectName();
    if (objName == "video1") {
        videoConfig = AppConfig::VideoConfig1;
    } else if (objName == "video2") {
        videoConfig = AppConfig::VideoConfig2;
    } else if (objName == "video3") {
        videoConfig = AppConfig::VideoConfig3;
    } else if (objName == "video4") {
        videoConfig = AppConfig::VideoConfig4;
    }

    ui->cboxUrl->lineEdit()->setText(videoConfig.rtspAddr);
    ui->cboxUrl->lineEdit()->setCursorPosition(0);
    connect(ui->cboxUrl->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(saveConfig()));

    ui->txtFileName->setText(videoConfig.fileName);
    connect(ui->txtFileName, SIGNAL(textChanged(QString)), this, SLOT(saveConfig()));

    ui->ckSaveFile->setChecked(videoConfig.saveFile);
    connect(ui->ckSaveFile, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->ckSaveInterval->setChecked(videoConfig.saveInterval);
    connect(ui->ckSaveInterval, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->cboxHardware->setCurrentIndex(ui->cboxHardware->findText(videoConfig.hardware));
    connect(ui->cboxHardware, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    ui->cboxTransport->setCurrentIndex(ui->cboxTransport->findText(videoConfig.transport));
    connect(ui->cboxTransport, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    ui->cboxFillImage->setCurrentIndex(videoConfig.fillImage ? 0 : 1);
    connect(ui->cboxFillImage, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    ui->cboxCallback->setCurrentIndex(videoConfig.callback ? 1 : 0);
    connect(ui->cboxCallback, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));

    ui->cboxImageFlag->setCurrentIndex(videoConfig.imageFlag);
    connect(ui->cboxImageFlag, SIGNAL(currentIndexChanged(int)), this, SLOT(saveConfig()));
}

void frmVideo::saveConfig()
{
    VideoConfig videoConfig;
    videoConfig.rtspAddr = ui->cboxUrl->currentText().trimmed();
    videoConfig.fileName = ui->txtFileName->text().trimmed();
    videoConfig.saveFile = ui->ckSaveFile->isChecked();
    videoConfig.saveInterval = ui->ckSaveInterval->isChecked();
    videoConfig.hardware = ui->cboxHardware->currentText();
    videoConfig.transport = ui->cboxTransport->currentText();
    videoConfig.fillImage = (ui->cboxFillImage->currentIndex() == 0);
    videoConfig.callback = (ui->cboxCallback->currentIndex() == 1);
    videoConfig.imageFlag = ui->cboxImageFlag->currentIndex();

    QString objName = this->objectName();
    if (objName == "video1") {
        AppConfig::VideoConfig1 = videoConfig;
    } else if (objName == "video2") {
        AppConfig::VideoConfig2 = videoConfig;
    } else if (objName == "video3") {
        AppConfig::VideoConfig3 = videoConfig;
    } else if (objName == "video4") {
        AppConfig::VideoConfig4 = videoConfig;
    }

    AppConfig::writeConfig();
}

void frmVideo::fileDrag(const QString &url)
{
    //设置拉伸填充
    ui->playWidget->setFillImage(ui->cboxFillImage->currentIndex() == 0);
    //设置回调
    ui->playWidget->setCallback(ui->cboxCallback->currentIndex() == 1);

    ui->cboxUrl->lineEdit()->setText(url);
    qDebug() << TIMEMS << "拖曳文件" << url;
}

void frmVideo::btnClicked(const QString &btnName)
{
    qDebug() << TIMEMS << "按下按钮" << btnName;
}

void frmVideo::receivePlayStart()
{
    ui->btnOpen->setText("关闭");
}

void frmVideo::receivePlayError()
{
    ui->btnOpen->setText("打开");
}

void frmVideo::receivePlayFinsh()
{
    ui->btnOpen->setText("打开");
}

QString frmVideo::getUrl() const
{
    return ui->cboxUrl->currentText();
}

void frmVideo::setUrl(const QString &url)
{
    ui->cboxUrl->lineEdit()->setText(url);
}

void frmVideo::setName(const QString &name)
{
    this->setObjectName(name);
    ui->playWidget->setObjectName(name + "_ffmpeg");
}

void frmVideo::setHardware(const QString &hardware)
{
    ui->cboxHardware->setCurrentIndex(ui->cboxHardware->findText(hardware));
}

void frmVideo::on_ckOSD1_stateChanged(int arg1)
{
    ui->playWidget->setOSD1Visible(arg1 != 0);
}

void frmVideo::on_ckOSD2_stateChanged(int arg1)
{
    ui->playWidget->setOSD2Visible(arg1 != 0);
}

void frmVideo::on_cboxFont1_currentIndexChanged(int index)
{
    ui->playWidget->setOSD1FontSize(ui->cboxFont1->currentText().toInt());
}

void frmVideo::on_cboxFont2_currentIndexChanged(int index)
{
    ui->playWidget->setOSD2FontSize(ui->cboxFont2->currentText().toInt());
}

void frmVideo::on_cboxFormat1_currentIndexChanged(int index)
{
    ui->playWidget->setOSD1Format((FFmpegWidget::OSDFormat)index);
}

void frmVideo::on_cboxFormat2_currentIndexChanged(int index)
{
    ui->playWidget->setOSD2Format((FFmpegWidget::OSDFormat)index);
}

void frmVideo::on_cboxOSD1_currentIndexChanged(int index)
{
    ui->playWidget->setOSD1Position((FFmpegWidget::OSDPosition)index);
}

void frmVideo::on_cboxOSD2_currentIndexChanged(int index)
{
    ui->playWidget->setOSD2Position((FFmpegWidget::OSDPosition)index);
}

void frmVideo::on_txtOSD1_textChanged(const QString &arg1)
{
    ui->playWidget->setOSD1Text(arg1);
}

void frmVideo::on_txtOSD2_textChanged(const QString &arg1)
{
    ui->playWidget->setOSD2Text(arg1);
}

void frmVideo::on_btnOSD1_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        ui->playWidget->setOSD1Color(color);
        ui->btnOSD1->setText(color.name().toUpper());
        ui->btnOSD1->setStyleSheet(QString("background:#000000;color:%1;").arg(ui->btnOSD1->text()));
    }
}

void frmVideo::on_btnOSD2_clicked()
{
    QColor color = QColorDialog::getColor();
    if (color.isValid()) {
        ui->playWidget->setOSD2Color(color);
        ui->btnOSD2->setText(color.name().toUpper());
        ui->btnOSD2->setStyleSheet(QString("background:#000000;color:%1;").arg(ui->btnOSD2->text()));
    }
}

void frmVideo::on_btnOpen_clicked()
{
    if (ui->btnOpen->text() == "打开") {
        //设置视频流地址
        ui->playWidget->setUrl(ui->cboxUrl->currentText().trimmed());
        //设置是否开启保存文件
        ui->playWidget->setSaveFile(ui->ckSaveFile->isChecked());
        //设置保存文件名称
        ui->playWidget->setFileName(ui->txtFileName->text().trimmed());
        //设置硬件加速
        ui->playWidget->setHardware(ui->cboxHardware->currentText());
        //设置tcp还是udp处理
        ui->playWidget->setTransport(ui->cboxTransport->currentText());
        //设置图片质量类型,速度优先+质量优先+均衡
        ui->playWidget->setImageFlag((FFmpegThread::ImageFlag)ui->cboxImageFlag->currentIndex());

        //设置深拷贝图像,在一些配置低的电脑上需要设置图像才不会断层
        //ui->playWidget->setCopyImage(true);

        //根据不同的选择设定不同的保存策略
        QString objName = this->objectName();
        if (ui->ckSaveInterval->isChecked()) {
            //设置按照时间段存储,单位秒钟,在当前程序目录下以类对象名称命名的文件夹
            ui->playWidget->setSavePath(AppPath + "/" + objName);
            ui->playWidget->setFileFlag(objName);
            ui->playWidget->setSaveInterval(1 * 30);
        } else if (ui->ckSaveTime->isChecked()) {
            //如果只需要存储一次文件则设置保存文件时间
            ui->playWidget->setSaveTime(QDateTime::currentDateTime().addSecs(30));
        }

        //有些地址比较特殊获取不到fps 需要手动设置
        if (ui->playWidget->getUrl().contains("ys7.com")) {
            ui->playWidget->setVideoFps(15);
        }

        //设置拉伸填充
        ui->playWidget->setFillImage(ui->cboxFillImage->currentIndex() == 0);
        //设置回调
        ui->playWidget->setCallback(ui->cboxCallback->currentIndex() == 1);

        ui->playWidget->open();
        ui->btnOpen->setText("关闭");
    } else {
        ui->playWidget->close();
        ui->btnOpen->setText("打开");
    }
}

void frmVideo::on_btnPause_clicked()
{
    if (ui->btnPause->text() == "暂停") {
        ui->playWidget->pause();
        ui->btnPause->setText("继续");
    } else {
        ui->playWidget->next();
        ui->btnPause->setText("暂停");
    }
}

void frmVideo::on_btnSnap_clicked()
{
    QString objName = this->objectName();
    int channel = objName.right(1).toInt();
    QString fileName = QString("%1/snap/Ch%2_%3.jpg").arg(AppPath).arg(channel).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz"));
    ui->playWidget->snap(fileName);
}

void frmVideo::on_btnScreen_clicked()
{
    QString objName = this->objectName();
    int channel = objName.right(1).toInt();
    QString fileName = QString("%1/snap/Ch%2_%3.jpg").arg(AppPath).arg(channel).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz"));
    QPixmap pix = ui->playWidget->getPixmap();
    if (!pix.isNull()) {
        pix.save(fileName, "jpg");
    }
}

void frmVideo::on_btnUrl_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "打开文件", AppPath);
    if (!fileName.isEmpty()) {
        ui->cboxUrl->insertItem(0, fileName);
        ui->cboxUrl->setCurrentIndex(0);
    }
}

void frmVideo::on_btnSave_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "保存文件", AppPath);
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".mp4")) {
            fileName = fileName + ".mp4";
        }
        ui->txtFileName->setText(fileName);
    }
}

void frmVideo::on_btnFace_clicked()
{
    //人脸框粗细
    ui->playWidget->setFaceBorder(3);
    //人脸边框颜色
    //ui->playWidget->setFaceColor(Qt::green);

    //如果有人脸则清空人脸 这里为了演示
    QList<QRect> faceRects;
    if (ui->playWidget->getFaceRects().count() == 0) {
        int size = 50;
        for (int i = 0; i < 3; i++) {
            int x = rand() % 300;
            int y = rand() % 200;
            faceRects << QRect(x, y, size, size);
        }
        ui->playWidget->setFaceRects(faceRects);
    } else {
        ui->playWidget->setFaceRects(faceRects);
    }
}

void frmVideo::on_ckSaveInterval_stateChanged(int arg1)
{
    if (arg1 != 0) {
        ui->ckSaveFile->setChecked(true);
        ui->ckSaveTime->setChecked(false);
        ui->ckSaveHand->setChecked(false);
    }
}

void frmVideo::on_ckSaveTime_stateChanged(int arg1)
{
    if (arg1 != 0) {
        ui->ckSaveFile->setChecked(true);
        ui->ckSaveInterval->setChecked(false);
        ui->ckSaveHand->setChecked(false);
    }
}

void frmVideo::on_ckSaveHand_stateChanged(int arg1)
{
    if (arg1 != 0) {
        ui->ckSaveFile->setChecked(true);
        ui->ckSaveInterval->setChecked(false);
        ui->ckSaveTime->setChecked(false);
    }

    if (!ui->playWidget->getIsPlaying()) {
        return;
    }

    if (arg1 != 0) {
        //每次设置不一样的文件名称,不然一样的话会覆盖
        QString savePath = QString("%1/%2").arg(AppPath).arg(objectName());
        QString fileName = QString("%1/%2.mp4").arg(savePath).arg(STRDATETIMEMS);
        ui->playWidget->setSaveFile(true);
        ui->playWidget->setSavePath(savePath);
        ui->playWidget->setFileName(fileName);
        ui->playWidget->startSave();
    } else {
        ui->playWidget->stopSave();
    }
}
