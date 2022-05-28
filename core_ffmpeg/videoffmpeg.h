#ifndef VIDEOFFMPEG_H
#define VIDEOFFMPEG_H

#include "ffmpegwidget.h"

class VideoFFmpeg : public QObject
{
    Q_OBJECT
public:
    static VideoFFmpeg *Instance();
    explicit VideoFFmpeg(QObject *parent = 0);
    ~VideoFFmpeg();

private:
    static QScopedPointer<VideoFFmpeg> self;

    //视频流地址链表
    QList<QString> videoUrls;
    //视频名称链表
    QList<QString> videoNames;
    //视频播放窗体链表
    QList<FFmpegWidget *> videoWidgets;
    //最后的重连时间
    QList<QDateTime> lastTimes;

    //超时时间 单位秒
    int timeout;
    //打开视频的间隔 单位毫秒
    int openInterval;
    //重连视频的间隔 单位秒
    int checkInterval;
    //视频数量
    int videoCount;

    //是否存储视频文件
    bool saveVideo;
    //存储视频间隔
    int saveVideoInterval;
    //存储路径
    QString savePath;

    //定时器排队打开视频
    int index;
    QTimer *timerOpen;
    //定时器重连
    QTimer *timerCheck;

private slots:
    //排队打开视频
    void openVideo();
    //处理重连
    void checkVideo();

public slots:
    //设置超时时间
    void setTimeout(int timeout);
    //设置打开视频的间隔
    void setOpenInterval(int openInterval);
    //设置重连视频的间隔
    void setCheckInterval(int checkInterval);
    //设置视频通道数
    void setVideoCount(int videoCount);

    //设置是否存储视频
    void setSaveVideo(bool saveVideo);
    //设置视频存储间隔
    void setSaveVideoInterval(int saveVideoInterval);
    //设置存储文件夹
    void setSavePath(const QString &savePath);

    //设置地址集合
    void setUrls(const QList<QString> &videoUrls);
    //设置名称集合
    void setNames(const QList<QString> &videoNames);
    //设置对象集合
    void setWidgets(QList<FFmpegWidget *> videoWidgets);

    //启动
    void start();
    //停止
    void stop();

    //打开
    void open(int index);
    //关闭
    void close(int index);
    //快照
    void snap(int index, const QString &fileName);
};

#endif // VIDEOFFMPEG_H
