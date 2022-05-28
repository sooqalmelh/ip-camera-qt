#include "videoffmpeg.h"
#include "ffmpeghelper.h"

QScopedPointer<VideoFFmpeg> VideoFFmpeg::self;
VideoFFmpeg *VideoFFmpeg::Instance()
{
    if (self.isNull()) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (self.isNull()) {
            self.reset(new VideoFFmpeg);
        }
    }

    return self.data();
}

VideoFFmpeg::VideoFFmpeg(QObject *parent) : QObject(parent)
{
    timeout = 10;
    openInterval = 1000;
    checkInterval = 5;
    videoCount = 16;

    saveVideo = false;
    saveVideoInterval = 0;
    savePath = qApp->applicationDirPath();

    //打开视频定时器
    timerOpen = new QTimer(this);
    connect(timerOpen, SIGNAL(timeout()), this, SLOT(openVideo()));
    timerOpen->setInterval(openInterval);

    //重连视频定时器
    timerCheck = new QTimer(this);
    connect(timerCheck, SIGNAL(timeout()), this, SLOT(checkVideo()));
    timerCheck->setInterval(checkInterval * 1000);

    newDir("snap");
}

VideoFFmpeg::~VideoFFmpeg()
{
    this->stop();
}

void VideoFFmpeg::openVideo()
{
    if (index < videoCount) {
        //取出一个进行打开,跳过为空的立即下一个
        QString url = videoUrls.at(index);
        if (!url.isEmpty()) {
            this->open(index);
            index++;
        } else {
            index++;
            this->openVideo();
        }
    } else {
        //全部取完则关闭定时器
        timerOpen->stop();
    }
}

void VideoFFmpeg::checkVideo()
{
    //如果打开定时器还在工作则不用继续
    if (timerOpen->isActive()) {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; i < videoCount; i++) {
        //只有url不为空的才需要处理重连
        if (videoUrls.at(i).isEmpty()) {
            continue;
        }

        //如果10秒内已经处理过重连则跳过当前这个,防止多个掉线一直处理第一个掉线的
        if (lastTimes.at(i).secsTo(now) < 10) {
            continue;
        }

        //计算超时时间
        QDateTime lastTime = videoWidgets.at(i)->getLastTime();
        int sec = lastTime.secsTo(now);
        if (sec >= timeout) {
            //重连该设备
            videoWidgets.at(i)->restart(videoUrls.at(i));
            //每次重连记住最后重连时间
            lastTimes[i] = now;
            //break;
        }
    }
}

void VideoFFmpeg::setTimeout(int timeout)
{
    if (timeout >= 5 && timeout < 60) {
        this->timeout = timeout;
    }
}

void VideoFFmpeg::setOpenInterval(int openInterval)
{
    if (openInterval >= 0 && openInterval <= 2000) {
        this->openInterval = openInterval;
        timerOpen->setInterval(openInterval);
    }
}

void VideoFFmpeg::setCheckInterval(int checkInterval)
{
    if (checkInterval >= 5 && checkInterval <= 60) {
        this->checkInterval = checkInterval;
        timerCheck->setInterval(checkInterval * 1000);
    }
}

void VideoFFmpeg::setVideoCount(int videoCount)
{
    this->videoCount = videoCount;
}

void VideoFFmpeg::setSaveVideo(bool saveVideo)
{
    this->saveVideo = saveVideo;
}

void VideoFFmpeg::setSaveVideoInterval(int saveVideoInterval)
{
    this->saveVideoInterval = saveVideoInterval;
}

void VideoFFmpeg::setSavePath(const QString &savePath)
{
    this->savePath = savePath;
}

void VideoFFmpeg::setUrls(const QList<QString> &videoUrls)
{
    this->videoUrls = videoUrls;
}

void VideoFFmpeg::setNames(const QList<QString> &videoNames)
{
    this->videoNames = videoNames;
}

void VideoFFmpeg::setWidgets(QList<FFmpegWidget *> videoWidgets)
{
    this->videoWidgets = videoWidgets;
}

void VideoFFmpeg::start()
{
    if (videoWidgets.count() != videoCount) {
        return;
    }

    lastTimes.clear();
    for (int i = 0; i < videoCount; i++) {
        lastTimes.append(QDateTime::currentDateTime());
        QString url = videoUrls.at(i);
        if (!url.isEmpty()) {
            FFmpegWidget *w = videoWidgets.at(i);
            //设置文件url地址
            w->setUrl(url);

            //设置OSD信息,可见+字体大小+文字+颜色+格式+位置
            if (i < videoNames.count()) {
                w->setOSD1Visible(true);
                w->setOSD1FontSize(18);
                w->setOSD1Text(videoNames.at(i));
                w->setOSD1Color(Qt::yellow);
                w->setOSD1Format(FFmpegWidget::OSDFormat_Text);
                w->setOSD1Position(FFmpegWidget::OSDPosition_Right_Top);

                //还可以设置第二路OSD
#if 0
                w->setOSD2Visible(true);
                w->setOSD2FontSize(18);
                w->setOSD2Color(Qt::yellow);
                w->setOSD2Format(FFmpegWidget::OSDFormat_DateTime);
                w->setOSD2Position(FFmpegWidget::OSDPosition_Left_Bottom);
#endif
            }

            //设置是否存储文件
            w->setSaveFile(saveVideo);
            w->setSavePath(savePath);
            w->setSaveInterval(saveVideoInterval);
            if (saveVideo && saveVideoInterval == 0) {
                QString path = QString("%1/%2").arg(savePath).arg(QDATE);
                newDir(path);
                QString fileName = QString("%1/Ch%2_%3.mp4").arg(path).arg(i + 1).arg(STRDATETIME);
                w->setFileName(fileName);
            }

            //打开间隔 = 0 毫秒则立即打开
            if (openInterval == 0) {
                this->open(i);
            }
        }
    }

    //启动定时器挨个排队打开
    if (openInterval > 0) {
        index = 0;
        timerOpen->start();
    }

    //启动定时器排队处理重连
    QTimer::singleShot(5000, timerCheck, SLOT(start()));
}

void VideoFFmpeg::stop()
{
    if (videoWidgets.count() != videoCount) {
        return;
    }

    if (timerOpen->isActive()) {
        timerOpen->stop();
    }

    if (timerCheck->isActive()) {
        timerCheck->stop();
    }

    for (int i = 0; i < videoCount; i++) {
        this->close(i);
    }
}

void VideoFFmpeg::open(int index)
{
    if (!videoUrls.at(index).isEmpty()) {
        videoWidgets.at(index)->open();
    }
}

void VideoFFmpeg::close(int index)
{
    if (!videoUrls.at(index).isEmpty()) {
        videoWidgets.at(index)->close();
    }
}

void VideoFFmpeg::snap(int index, const QString &fileName)
{
    if (!videoUrls.at(index).isEmpty()) {
        videoWidgets.at(index)->snap(fileName);
    }
}
