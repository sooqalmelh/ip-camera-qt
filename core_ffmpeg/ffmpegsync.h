#ifndef FFMPEGSYNC_H
#define FFMPEGSYNC_H

#include <QThread>
#include <QMutex>

class FFmpegThread;
class AVPacket;

class FFmpegSync : public QThread
{
    Q_OBJECT
public:
    explicit FFmpegSync(QObject *parent = 0);

protected:
    void run();

private:
    volatile bool stopped;      //线程停止标志位
    QMutex mutex;               //数据锁
    quint8 type;                //类型 音频还是视频
    FFmpegThread *thread;       //解码主线程
    QList<AVPacket *> packets;  //帧队列

    double ptsTime;             //当前帧显示时间
    qint64 showTime;            //已播放时间
    qint64 bufferTime;          //缓冲时间
    qint64 offsetTime;          //当前时间和开始时间的差值
    qint64 startTime;           //解码开始时间    

signals:
    void filePositionReceive(qint64 position);

public:
    void stop();
    void clear();
    void reset();

    void setType(quint8 type);
    void setThread(FFmpegThread *thread);
    void append(AVPacket *packet);

    int getPacketCount();
    bool checkPtsTime();
    void checkShowTime();
};

#endif // FFMPEGSYNC_H
