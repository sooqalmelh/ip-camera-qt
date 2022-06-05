#include "frmmulti.h"
#include "ui_frmmulti.h"
#include "head.h"

frmMulti::frmMulti(QWidget *parent) : QWidget(parent), ui(new Ui::frmMulti)
{
    ui->setupUi(this);
    this->initForm();
    this->initConfig();
}

frmMulti::~frmMulti()
{
    delete ui;
}

void frmMulti::showEvent(QShowEvent *)
{
    if (!ui->playWidget1->getIsPlaying()) {
        ui->playWidget1->open();
    }
}

void frmMulti::initForm()
{
    QList<FFmpegWidget *> widgets;
    widgets << ui->playWidget2 << ui->playWidget3 << ui->playWidget4;
    foreach (FFmpegWidget *widget, widgets) {
        connect(ui->playWidget1, SIGNAL(receiveImage(QImage)), widget, SLOT(updateImage(QImage)));
        connect(ui->playWidget1, SIGNAL(receiveFrame(AVFrame *)), widget, SLOT(updateFrame(AVFrame *)));
    }

    connect(ui->playWidget1, SIGNAL(receivePlayStart()), this, SLOT(playStart()));
    connect(ui->playWidget1, SIGNAL(receivePlayFinsh()), this, SLOT(playFinsh()));
}

void frmMulti::initConfig()
{
    //AppConfig::RtspAddr6 = "f:/mp5/1.mp4";
    //AppConfig::Hardware6 = "dxva2";
    QList<FFmpegWidget *> widgets;
    widgets << ui->playWidget1 << ui->playWidget2 << ui->playWidget3 << ui->playWidget4;
    foreach (FFmpegWidget *widget, widgets) {
        widget->setUrl(AppConfig::VideoConfig6.rtspAddr);
        widget->setHardware(AppConfig::VideoConfig6.hardware);
        widget->setFillImage(AppConfig::VideoConfig6.fillImage);
        widget->setCallback(AppConfig::VideoConfig6.callback);
    }
}

void frmMulti::playStart()
{
    //需要将视频宽高传给其他几个控件
    FFmpegThread *thread = ui->playWidget1->getThread();
    int width = thread->getVideoWidth();
    int height = thread->getVideoHeight();

    QList<FFmpegWidget *> widgets;
    widgets << ui->playWidget2 << ui->playWidget3 << ui->playWidget4;
    foreach (FFmpegWidget *widget, widgets) {
        widget->setMultiMode(true, width, height);
        QMetaObject::invokeMethod(widget, "playStart");
    }
}

void frmMulti::playFinsh()
{
    QList<FFmpegWidget *> widgets;
    widgets << ui->playWidget2 << ui->playWidget3 << ui->playWidget4;
    foreach (FFmpegWidget *widget, widgets) {
        QMetaObject::invokeMethod(widget, "playFinsh");
        widget->setMultiMode(false, 0, 0);
    }
}
