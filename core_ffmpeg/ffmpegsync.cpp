#include "ffmpegsync.h"
#include "ffmpeghelper.h"
#include "ffmpegthread.h"

FFmpegSync::FFmpegSync(QObject *parent) : QThread(parent)
{
    stopped = false;
    type = 0;
    thread = NULL;
}

void FFmpegSync::run()
{
    reset();
    while (!stopped) {
        //暂停状态或者队列中没有帧则不处理
        if (!thread->isPause && packets.count() > 0) {
            mutex.lock();
            AVPacket *packet = packets.first();
            mutex.unlock();

            //h264的裸流文件同步有问题,获取不到pts和dts,暂时用最蠢的办法延时解决
            if (thread->formatName == "h264") {
                int sleepTime = (1000 / thread->videoFps) - 5;
                msleep(sleepTime);
            }

            //计算当前帧显示时间 外部时钟同步
            ptsTime = getPtsTime(thread->formatCtx, packet);
            if (!this->checkPtsTime()) {
                msleep(1);
                continue;
            }

            //显示当前的播放进度
            checkShowTime();

            //0-表示音频 1-表示视频
            if (type == 0) {
                thread->decodeAudio1(packet);
            } else if (type == 1) {
                thread->decodeVideo1(packet);
            }

            //释放资源并移除
            mutex.lock();
            thread->free(packet);
            packets.removeFirst();
            mutex.unlock();
        }

        msleep(1);
    }

    clear();
    stopped = false;
}

void FFmpegSync::stop()
{
    stopped = true;
}

void FFmpegSync::clear()
{
    mutex.lock();
    //释放还没有来得及处理的剩余的帧
    foreach (AVPacket *packet, packets) {
        thread->free(packet);
    }
    packets.clear();
    mutex.unlock();
}

void FFmpegSync::reset()
{
    //复位音频外部时钟
    showTime = 0;
    bufferTime = 0;
    offsetTime = -1;
    startTime = av_gettime();
    //考虑取主解码线程的时间?
    //startTime = thread->startTime;
}

void FFmpegSync::setType(quint8 type)
{
    this->type = type;
}

void FFmpegSync::setThread(FFmpegThread *thread)
{
    this->thread = thread;
}

void FFmpegSync::append(AVPacket *packet)
{
    mutex.lock();
    packets.append(packet);
    mutex.unlock();
}

int FFmpegSync::getPacketCount()
{
    return this->packets.count();
}

bool FFmpegSync::checkPtsTime()
{
    bool ok = false;
    if (ptsTime > 0) {
        if (ptsTime > offsetTime + 100000) {
            bufferTime = ptsTime - offsetTime + 100000;
        }

        int offset = (type == 0 ? 1000 : 5000);
        offsetTime = av_gettime() - startTime + bufferTime;
        if ((offsetTime <= ptsTime && ptsTime - offsetTime <= offset) || (offsetTime > ptsTime)) {
            ok = true;
        }
    } else {
        ok = true;
    }

    return ok;
}

void FFmpegSync::checkShowTime()
{
    //视频流或者本地USB摄像机流不用处理
    if (thread->isRtsp || thread->isUsbCamera) {
        return;
    }

    //过滤重复发送播放时间 只存在音频或者视频+音视频都存在则取视频
    bool needShowTime = false;
    bool existVideo = (thread->videoIndex >= 0);
    if (type == 0) {
        if (!existVideo) {
            needShowTime = true;
        }
    } else if (type == 1) {
        if (existVideo) {
            needShowTime = true;
        }
    }

    //需要显示时间的时候 并且差值大于200毫秒 不要太过于频繁发送
    if (needShowTime) {
        if (offsetTime - showTime > 200000) {
            showTime = offsetTime;
            emit filePositionReceive(showTime / 1000);
        }
    }
}
