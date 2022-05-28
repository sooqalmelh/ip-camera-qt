#include "frmview.h"
#include "ui_frmview.h"
#include "head.h"
#include "ffmpegwidget.h"
#include "videoffmpeg.h"
#include "videobox.h"

frmView::frmView(QWidget *parent) : QWidget(parent), ui(new Ui::frmView)
{
    ui->setupUi(this);
    this->initForm();
    this->initMenu();
}

frmView::~frmView()
{
    delete ui;
}

void frmView::keyReleaseEvent(QKeyEvent *key)
{
    //当按下esc键并且当前处于全屏模式则切换到正常模式
    if (key->key() == Qt::Key_Escape && actionFull->text() == "切换正常模式") {
        actionFull->trigger();
    }
}

void frmView::showEvent(QShowEvent *)
{
    static bool isLoad = false;
    if (!isLoad) {
        isLoad = true;
        QTimer::singleShot(1000, this, SLOT(play_video_all()));
    }
}

bool frmView::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        FFmpegWidget *widget = (FFmpegWidget *) watched;
        if (!videoMax) {
            videoMax = true;
            videoBox->hide_video_all();
            ui->gridLayout->addWidget(widget, 0, 0);
            widget->setVisible(true);
        } else {
            videoMax = false;
            videoBox->show_video_all();
        }

        widget->setFocus();
    } else if (event->type() == QEvent::MouseButtonPress) {
        if (qApp->mouseButtons() == Qt::RightButton) {
            videoMenu->exec(QCursor::pos());
        }
    }

    return QWidget::eventFilter(watched, event);
}

void frmView::initForm()
{
    ui->frame->setStyleSheet("border:2px solid #000000;");

    videoMax = false;
    videoCount = 64;

    QStringList urls;
    for (int i = 0; i < videoCount; i++) {
        FFmpegWidget *widget = new FFmpegWidget;
        widget->installEventFilter(this);
        widget->setBorderWidth(3);
        widget->setBgText(QString("通道 %1").arg(i + 1));
        widget->setBgImage(QImage(":/image/bg_novideo.png"));
        widget->setObjectName(QString("video%1").arg(i + 1));

        //widget->setCopyImage(true);
        widget->setFillImage(AppConfig::VideoConfig0.fillImage);
        widget->setCallback(AppConfig::VideoConfig0.callback);
        widget->setHardware(AppConfig::VideoConfig0.hardware);
        widget->setTransport(AppConfig::VideoConfig0.transport);
        widget->setImageFlag((FFmpegThread::ImageFlag)AppConfig::VideoConfig0.imageFlag);
        widget->setCheckTime(AppConfig::VideoConfig0.checkTime);

        //设置是否循环播放
        //widget->setPlayRepeat(true);
        //设置是否播放声音
        widget->setPlayAudio(false);

        urls.append("");
        widgets.append(widget);
    }

    AppUrl::getUrls(urls);
    VideoFFmpeg::Instance()->setUrls(urls);
    VideoFFmpeg::Instance()->setWidgets(widgets);
    VideoFFmpeg::Instance()->setVideoCount(videoCount);

    //还可以设置超时时间+打开间隔+重连间隔
    VideoFFmpeg::Instance()->setTimeout(10);
    VideoFFmpeg::Instance()->setOpenInterval(200);
    VideoFFmpeg::Instance()->setCheckInterval(5);

#ifdef Q_OS_ANDROID
    //安卓上写死四通道地址测试用
    AppConfig::VideoType = "1_4";
    //写死默认回调
    foreach (FFmpegWidget *widget, widgets) {
        widget->setCallback(true);
    }

    widgets.at(0)->setPlayAudio(true);
    urls[0] = AppConfig::RtspAddr_1;
    urls[1] = AppConfig::RtspAddr_2;
    urls[2] = AppConfig::RtspAddr_3;
    urls[3] = AppConfig::RtspAddr_4;

    VideoFFmpeg::Instance()->setOpenInterval(2000);
#endif
}

void frmView::initMenu()
{
    videoMenu = new QMenu(this);

    actionFull = new QAction("切换全屏模式", videoMenu);
    connect(actionFull, SIGNAL(triggered(bool)), this, SLOT(full()));
    actionPoll = new QAction("启动轮询视频", videoMenu);
    connect(actionPoll, SIGNAL(triggered(bool)), this, SLOT(poll()));

    videoMenu->addAction(actionFull);
    videoMenu->addAction(actionPoll);
    videoMenu->addSeparator();

    videoMenu->addAction("截图当前视频", this, SLOT(snapshot_video_one()));
    videoMenu->addAction("截图所有视频", this, SLOT(snapshot_video_all()));
    videoMenu->addSeparator();

    //实例化通道布局类
    videoBox = new VideoBox(this);
    connect(videoBox, SIGNAL(changeVideo(int, QString, bool)), this, SLOT(changeVideo(int, QString, bool)));

    QList<bool> enable;
    enable << true << true << true << true << true << true << true << true << true;
    videoBox->initMenu(videoMenu, enable);
    videoBox->setVideoType(AppConfig::VideoType);
    videoBox->setLayout(ui->gridLayout);

    //重新转换类型
    QWidgetList widgets;
    foreach (FFmpegWidget *widget, this->widgets) {
        widgets << widget;
    }
    videoBox->setWidgets(widgets);
    videoBox->show_video_all();
}

void frmView::full()
{
    if (actionFull->text() == "切换全屏模式") {
        emit fullScreen(true);
        actionFull->setText("切换正常模式");
        this->layout()->setContentsMargins(0, 0, 0, 0);
    } else {
        emit fullScreen(false);
        actionFull->setText("切换全屏模式");
        this->layout()->setContentsMargins(6, 6, 6, 6);
    }
}

void frmView::poll()
{
    if (actionPoll->text() == "启动轮询视频") {
        actionPoll->setText("停止轮询视频");
    } else {
        actionPoll->setText("启动轮询视频");
    }
}

void frmView::play_video_all()
{
    VideoFFmpeg::Instance()->start();
}

void frmView::snapshot_video_one()
{
    for (int i = 0; i < videoCount; i++) {
        if (widgets.at(i)->hasFocus()) {
            QString fileName = QString("%1/snap/Ch%2_%3.jpg").arg(AppPath).arg(i + 1).arg(STRDATETIME);
            VideoFFmpeg::Instance()->snap(i, fileName);
            break;
        }
    }
}

void frmView::snapshot_video_all()
{
    for (int i = 0; i < videoCount; i++) {
        QString fileName = QString("%1/snap/Ch%2_%3.jpg").arg(AppPath).arg(i + 1).arg(STRDATETIME);
        VideoFFmpeg::Instance()->snap(i, fileName);
    }
}

void frmView::changeVideo(int type, const QString &videoType, bool videoMax)
{
    AppConfig::VideoType = videoType;
    AppConfig::writeConfig();
}
