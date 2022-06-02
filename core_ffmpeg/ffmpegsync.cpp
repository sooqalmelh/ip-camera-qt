/**
 * @file ffmpegsync.cpp
 * @author creekwater
 * @brief
 *
 * 音视频解码同步线程
 *
 * @version 0.1
 * @date 2022-06-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "ffmpegsync.h"
#include "ffmpeghelper.h"
#include "ffmpegthread.h"

FFmpegSync::FFmpegSync(QObject *parent) : QThread(parent)
{
    stopped = false; // 线程停止
    type = 0;        // 类型：音频还是视频
    thread = NULL;   // 用于解码的主线程
}

/**
 * @brief 同步时钟线程
 *
 * 实时更新ptstime
 *
 */
void FFmpegSync::run()
{
    reset(); // 复位相关变量

    while (!stopped)
    {
        // 没有挂起，且帧队列中有数据
        if (!thread->isPause && packets.count() > 0)
        {
            mutex.lock();
            AVPacket *packet = packets.first(); // 获取队列中第一个数据帧
            mutex.unlock();

            // h264的裸流文件同步有问题,获取不到pts和dts,暂时用最蠢的办法延时解决
            if (thread->formatName == "h264")
            {
                int sleepTime = (1000 / thread->videoFps) - 5;
                msleep(sleepTime);
            }

            // 计算当前帧显示时间 外部时钟同步
            ptsTime = getPtsTime(thread->formatCtx, packet);
            if (!this->checkPtsTime())
            {
                msleep(1);
                continue;
            }

            //显示当前的播放进度
            checkShowTime();

            // 0-表示音频 1-表示视频
            if (type == 0)
            {
                thread->decodeAudio1(packet);   // 解码音频
            }
            else if (type == 1)
            {
                thread->decodeVideo1(packet);   // 解码视频
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

/**
 * @brief 停止同步时钟
 *
 */
void FFmpegSync::stop()
{
    stopped = true;
}

void FFmpegSync::clear()
{
    mutex.lock();
    //释放还没有来得及处理的剩余的帧
    foreach (AVPacket *packet, packets)
    {
        thread->free(packet);
    }
    packets.clear();
    mutex.unlock();
}

/**
 * @brief 复位同步时钟
 *
 */
void FFmpegSync::reset()
{
    //复位音频外部时钟
    showTime = 0;
    bufferTime = 0;
    offsetTime = -1;
    startTime = av_gettime();
    //考虑取主解码线程的时间?
    // startTime = thread->startTime;
}

/**
 * @brief 设置同步线程类型
 *
 * @param type 0：音频同步线程   1：视频同步线程
 */
void FFmpegSync::setType(quint8 type)
{
    this->type = type;
}

/**
 * @brief 设置同步线程
 *
 * @param thread
 */
void FFmpegSync::setThread(FFmpegThread *thread)
{
    this->thread = thread;
}

/**
 * @brief 队列中添加一个数据包
 *
 * @param packet
 */
void FFmpegSync::append(AVPacket *packet)
{
    mutex.lock();
    packets.append(packet);
    mutex.unlock();
}

/**
 * @brief 获取队列中数据包的个数
 *
 * @return int
 */
int FFmpegSync::getPacketCount()
{
    return this->packets.count();
}

/**
 * @brief 查询当前显示的时间（Presentation Time Stamp）
 *
 * 调用该函数之前需要先更新ptsTime，函数内部更新offsetTime
 *
 * @return true
 * @return false
 */
bool FFmpegSync::checkPtsTime()
{
    bool ok = false;

    // 确保时钟同步线程已经已经启动
    if (ptsTime > 0)
    {
        if (ptsTime > offsetTime + 100000)
        {
            bufferTime = ptsTime - offsetTime + 100000;
        }

        int offset = (type == 0 ? 1000 : 5000);
        offsetTime = av_gettime() - startTime + bufferTime;
        if ((offsetTime <= ptsTime && ptsTime - offsetTime <= offset) || (offsetTime > ptsTime))
        {
            ok = true;
        }
    }
    else
    {
        ok = true;
    }

    return ok;
}

/**
 * @brief 检查是否需要更新并显示时间，需要的话则发送一个信号
 *
 */
void FFmpegSync::checkShowTime()
{
    //视频流或者本地USB摄像机流不用处理
    if (thread->isRtsp || thread->isUsbCamera)
    {
        return;
    }

    //过滤重复发送播放时间 只存在音频或者视频+音视频都存在则取视频
    bool needShowTime = false;
    bool existVideo = (thread->videoIndex >= 0); // 存在视频

    // 音频，不能有视频
    if (type == 0)
    {
        if (!existVideo)
        {
            needShowTime = true; // 音频不存在视频，需要显示time
        }
    }
    // 视频
    else if (type == 1)
    {
        if (existVideo)
        {
            needShowTime = true; // 存在视频，需要显示time
        }
    }

    // 需要显示时间的时候 并且差值大于200毫秒 不要太过于频繁发送
    if (needShowTime)
    {
        if (offsetTime - showTime > 200000)
        {
            showTime = offsetTime;
            emit filePositionReceive(showTime / 1000);
        }
    }
}
