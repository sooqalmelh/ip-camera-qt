#ifndef FFMPEGHELPER_H
#define FFMPEGHELPER_H

#include "ffmpeghead.h"
#include "ffmpegthread.h"
#include "qmutex.h"
#include "qregexp.h"
#include "qeventloop.h"
#include "qtimer.h"
#include "qdir.h"
#include "qtcpsocket.h"
#include "qapplication.h"

//获取版本
static QString getVersion()
{
    return QString("%1").arg(FFMPEG_VERSION);
}

//打印输出各种信息 https://blog.csdn.net/xu13879531489/article/details/80703465
static void qdebuglib()
{
    //输出所有支持的解码器名称
    QStringList listCodeName;
    AVCodec *code = av_codec_next(NULL);
    while (code != NULL) {
        listCodeName << code->name;
        code = code->next;
    }

    qDebug() << TIMEMS << "code names" << listCodeName;

    //输出支持的协议
    QStringList listProtocolsIn, listProtocolsOut;
    struct URLProtocol *protocol = NULL;
    struct URLProtocol **protocol2 = &protocol;

    avio_enum_protocols((void **)protocol2, 0);
    while ((*protocol2) != NULL) {
        listProtocolsIn << avio_enum_protocols((void **)protocol2, 0);
    }

    qDebug() << TIMEMS << "protocols in" << listProtocolsIn;

    protocol = NULL;
    avio_enum_protocols((void **)protocol2, 1);
    while ((*protocol2) != NULL) {
        listProtocolsOut << avio_enum_protocols((void **)protocol2, 1);
    }

    qDebug() << TIMEMS << "protocols out" << listProtocolsOut;
}

/**
 * @brief 初始化ffmpeg库，一个软件中只需要初始化一次就行
 *
 */
static void initlib()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    static bool isInit = false;
    if (!isInit) {
#ifdef ffmpegdevice
        //注册所有设备,主要用于本地摄像机播放支持
        avdevice_register_all();
#endif
#ifdef ffmpegfilter
        //注册特效库 色调、模糊、水平翻转、裁剪、加方框、叠加文字等功能
        avfilter_register_all();
#endif
        //注册库中所有可用的文件格式和解码器
        av_register_all();
        //初始化网络流格式,使用网络流时必须先执行
        avformat_network_init();

        //设置日志级别
        //如果不想看到烦人的打印信息可以设置成 AV_LOG_QUIET 表示不打印日志
        //有时候发现使用不正常比如打开了没法播放视频则需要打开日志看下报错提示
        av_log_set_level(AV_LOG_QUIET);

        isInit = true;
        qDebug() << TIMEMS << "init ffmpeg lib ok" << getVersion();
        //qdebuglib();
    }
}

//校验url地址是否通畅
static bool checkUrl(const QString &url, int checkTime)
{
    //找出IP地址
    QRegExp reg("((?:(?:25[0-5]|2[0-4]\\d|[01]?\\d?\\d)\\.){3}(?:25[0-5]|2[0-4]\\d|[01]?\\d?\\d))");
    int start = reg.indexIn(url);
    int length = reg.matchedLength();
    QString ip = url.mid(start, length);

    //找出端口号
    int port = 554;
    QStringList list = url.split(":");
    if (list.count() > 2) {
        int index = 2;
        if (url.contains("@")) {
            index = 3;
        }

        list = list.at(index).split("/");
        QString str = list.first();
        if (!str.isEmpty()) {
            port = str.toInt();
        }
    }

    //局部的事件循环,不卡主界面
    QEventLoop eventLoop;

    //设置超时
    QTimer timer;
    QObject::connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
    timer.setSingleShot(true);
    timer.start(checkTime);

    QTcpSocket tcpSocket;
    QObject::connect(&tcpSocket, SIGNAL(connected()), &eventLoop, SLOT(quit()));
    tcpSocket.connectToHost(ip, port);
    eventLoop.exec();

    //超时没有连接上则判断该摄像机不在线
    if (tcpSocket.state() != QAbstractSocket::ConnectedState) {
        qDebug() << TIMEMS << url << "connect error";
        return false;
    }

    return true;
}

//新建目录
static void newDir(const QString &dirName)
{
    //如果路径中包含斜杠字符则说明是绝对路径
    //linux系统路径字符带有 /  windows系统 路径字符带有 :/
    QString strDir = dirName;
    if (!strDir.startsWith("/") && !strDir.contains(":/")) {
        strDir = QString("%1/%2").arg(qApp->applicationDirPath()).arg(strDir);
    }

    QDir dir(strDir);
    if (!dir.exists()) {
        dir.mkpath(strDir);
    }
}

//获取错误代码错误信息
static QString getError(int errnum)
{
    char errbuf[256] = { 0 };
    av_strerror(errnum, errbuf, sizeof(errbuf));
    return errbuf;
}

//超时回调 返回1表示不继续等待处理
static QMutex mutex;
static int AVInterruptCallBackFun(void *ctx)
{
    QMutexLocker locker(&mutex);
    FFmpegThread *thread = (FFmpegThread *)ctx;

    //2021-9-29 增加先判断是否尝试停止线程,有时候不存在的地址反复打开关闭会卡主导致崩溃
    //多了这个判断可以立即停止
    if (thread->getTryStop()) {
        return 1;
    }

    //打开地址超时判定+读取数据超时判定
    if (!thread->getTryOpen()) {
        //时间差值=当前时间-开始解码的时间 单位微秒
        qint64 offsetTime = av_gettime() - thread->getStartTime();
        int timeout = thread->getCheckTime() * 1000;
        //qDebug() << TIMEMS << "getTryOpen" << offsetTime << timeout;
        if (offsetTime > timeout) {
            return 1;
        }
    } else if (!thread->getTryRead()) {
        //时间差值=当前时间-最后一次读取的时间 单位毫秒
        QDateTime now = QDateTime::currentDateTime();
        qint64 offsetTime = thread->getLastTime().msecsTo(now);
        int timeout = thread->getReadTime();
        //qDebug() << TIMEMS << "getTryRead" << offsetTime << timeout;
        if (offsetTime > timeout) {
            return 1;
        }
    }

    return 0;
}

static AVPacket *getNewPacket(AVPacket *packet)
{
    AVPacket *pkt;
#ifndef gcc45
    //新方法 推荐使用
    pkt = av_packet_clone(packet);
#else
    //旧方法 废弃使用
    pkt = new AVPacket;
    av_init_packet(pkt);
    av_copy_packet(pkt, packet);
#endif
    return pkt;
}

//获取流的索引 以下两种方法都可以
static int getStreamIndex(AVFormatContext *formatCtx, AVMediaType mediaType, AVCodec *codec)
{
    int streamIndex = -1;
#if 1
    streamIndex = av_find_best_stream(formatCtx, mediaType, -1, -1, &codec, 0);
#else
    for (uint i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codec->codec_type == mediaType) {
            streamIndex = i;
            break;
        }
    }
#endif
    return streamIndex;
}

//获取pts值 带矫正
static int64_t getPts(AVPacket *packet)
{
    //有些文件(比如asf文件)取不到pts需要矫正
    int64_t pts = 0;
    if (packet->dts == AV_NOPTS_VALUE && packet->pts && packet->pts != AV_NOPTS_VALUE) {
        pts = packet->pts;
    } else if (packet->dts != AV_NOPTS_VALUE) {
        pts = packet->dts;
    }
    return pts;
}

//播放时刻值(单位秒)
static double getPtsTime(AVFormatContext *formatCtx, AVPacket *packet)
{
    AVStream *stream = formatCtx->streams[packet->stream_index];
    int64_t pts = getPts(packet);
    //qDebug() << TIMEMS << pts << packet->pos << packet->duration;
    //double time = pts * av_q2d(stream->time_base) * 1000;
    double time = pts * 1.0 * av_q2d(stream->time_base) * AV_TIME_BASE;
    //double time = pts * 1.0 * stream->time_base.num / stream->time_base.den * AV_TIME_BASE;
    return time;
}

//播放时长值(单位秒)
static double getDurationTime(AVFormatContext *formatCtx, AVPacket *packet)
{
    AVStream *stream = formatCtx->streams[packet->stream_index];
    double time = packet->duration * av_q2d(stream->time_base);
    return time;
}

//延时时间值(单位微秒)
static qint64 getDelayTime(AVFormatContext *formatCtx, AVPacket *packet, qint64 startTime)
{
    AVRational time_base = formatCtx->streams[packet->stream_index]->time_base;
    AVRational time_base_q = {1, AV_TIME_BASE};//AV_TIME_BASE_Q
    int64_t pts = getPts(packet);
    int64_t pts_time = av_rescale_q(pts, time_base, time_base_q);
    int64_t now_time = av_gettime() - startTime;
    int64_t offset_time = pts_time - now_time;
    return offset_time;
}

//根据时间差延时
static void delayTime(AVFormatContext *formatCtx, AVPacket *packet, qint64 startTime)
{
    qint64 offset_time = getDelayTime(formatCtx, packet, startTime);
    //qDebug() << TIMEMS << offset_time << packet->pts << packet->dts;
    if (offset_time > 0 && offset_time < 1 * 1000 * 1000) { //
        av_usleep(offset_time);
    }
}

//yuv视频数据旋转
static void yuv420Rotate90(uchar *dst, const uchar *src, int srcWidth, int srcHeight)
{
    static int nWidth = 0, nHeight = 0;
    static int wh = 0;
    static int uvHeight = 0;
    if (srcWidth != nWidth || srcHeight != nHeight) {
        nWidth = srcWidth;
        nHeight = srcHeight;
        wh = srcWidth * srcHeight;
        uvHeight = srcHeight >> 1;
    }

    //旋转Y
    int k = 0;
    for (int i = 0; i < srcWidth; i++) {
        int nPos = 0;
        for (int j = 0; j < srcHeight; j++) {
            dst[k] = src[nPos + i];
            k++;
            nPos += srcWidth;
        }
    }

    for (int i = 0; i < srcWidth; i += 2) {
        int nPos = wh;
        for (int j = 0; j < uvHeight; j++) {
            dst[k] = src[nPos + i];
            dst[k + 1] = src[nPos + i + 1];
            k += 2;
            nPos += srcWidth;
        }
    }
    return;
}

//音频dts头部数据
static char *dtsData;

//rtmp视频流需要添加pps sps
#ifdef gcc45
static const AVBitStreamFilter *filter;
#else
static const AVBitStreamFilter *filter = av_bsf_get_by_name("h264_mp4toannexb");
#endif
static int av_bsf_filter(const AVBitStreamFilter *filter, AVPacket *packet, const AVCodecParameters *src)
{
#ifndef gcc45
    int ret;
    AVBSFContext *ctx = NULL;
    if (!filter) {
        return 0;
    }

    ret = av_bsf_alloc(filter, &ctx);
    if (ret < 0) {
        return ret;
    }

    ret = avcodec_parameters_copy(ctx->par_in, src);
    if (ret < 0) {
        return ret;
    }

    ret = av_bsf_init(ctx);
    if (ret < 0) {
        return ret;
    }

    AVPacket pkt = {0};
    pkt.data = packet->data;
    pkt.size = packet->size;

    ret = av_bsf_send_packet(ctx, &pkt);
    if (ret < 0) {
        return ret;
    }

    ret = av_bsf_receive_packet(ctx, &pkt);
    if (pkt.data == packet->data) {
        uint8_t *poutbuf = (uint8_t *)av_malloc(pkt.size);
        if (!poutbuf) {
            av_packet_unref(&pkt);
            av_free(poutbuf);
            return -1;
        }

        memcpy(poutbuf, pkt.data, pkt.size);
        av_packet_unref(packet);
        packet->data = poutbuf;
        packet->size = pkt.size;
        av_packet_unref(&pkt);
        av_bsf_free(&ctx);
        av_free(poutbuf);
        return 1;
    }

    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return 0;
    } else if (ret < 0) {
        return ret;
    }

    uint8_t *poutbuf = (uint8_t *)av_malloc(pkt.size + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!poutbuf) {
        av_packet_unref(&pkt);
        av_free(poutbuf);
        return AVERROR(ENOMEM);
    }

    int poutbuf_size = pkt.size;
    memcpy(poutbuf, pkt.data, pkt.size);
    packet->data = poutbuf;
    packet->size = poutbuf_size;
    av_packet_unref(&pkt);

    while (ret >= 0) {
        ret = av_bsf_receive_packet(ctx, &pkt);
        av_packet_unref(&pkt);
    }

    av_packet_unref(&pkt);
    av_bsf_free(&ctx);
    av_free(poutbuf);
#endif
    return 1;
}

static int decode_packet(AVCodecContext *avctx, AVPacket *packet, AVFrame *frameSrc, AVFrame *videoFrame)
{
#ifndef gcc45
    int ret = 0;
    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        qDebug() << TIMEMS << "Error during decoding";
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(avctx, frameSrc);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            qDebug() << TIMEMS << "Error during decoding";
            break;
        }

        ret = av_hwframe_transfer_data(videoFrame, frameSrc, 0);
        if (ret < 0) {
            qDebug() << TIMEMS << "Error transferring the data to system memory";
            av_frame_unref(videoFrame);
            av_frame_unref(frameSrc);
            return ret;
        }
    }
#endif
    return 1;
}

static AVPixelFormat get_qsv_format(AVCodecContext *avctx, const enum AVPixelFormat *pix_fmts)
{
#ifndef gcc45
    while (*pix_fmts != AV_PIX_FMT_NONE) {
        if (*pix_fmts == AV_PIX_FMT_QSV) {
            struct DecodeContext {
                AVBufferRef *hw_device_ref;
            };

            DecodeContext *decode = (DecodeContext *)avctx->opaque;
            avctx->hw_frames_ctx = av_hwframe_ctx_alloc(decode->hw_device_ref);
            av_buffer_unref(&decode->hw_device_ref);

            if (!avctx->hw_frames_ctx) {
                return AV_PIX_FMT_NONE;
            }

            AVHWFramesContext *frames_ctx = (AVHWFramesContext *)avctx->hw_frames_ctx->data;
            AVQSVFramesContext *frames_hwctx = (AVQSVFramesContext *)frames_ctx->hwctx;

            frames_ctx->format = AV_PIX_FMT_QSV;
            frames_ctx->sw_format = avctx->sw_pix_fmt;
            frames_ctx->width = FFALIGN(avctx->coded_width, 32);
            frames_ctx->height = FFALIGN(avctx->coded_height, 32);
            frames_ctx->initial_pool_size = 32;
            frames_hwctx->frame_type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;

            int ret = av_hwframe_ctx_init(avctx->hw_frames_ctx);
            if (ret < 0) {
                return AV_PIX_FMT_NONE;
            }

            //qDebug() << TIMEMS << "get_qsv_format ok";
            return AV_PIX_FMT_QSV;
        }

        pix_fmts++;
    }
#endif

    qDebug() << TIMEMS << "The QSV pixel format not offered in get_format()";
    return AV_PIX_FMT_NONE;
}

//根据硬解码类型找到对应的硬解码格式
#ifdef hardwarespeed
static enum AVPixelFormat find_fmt_by_hw_type(const enum AVHWDeviceType type)
{
    enum AVPixelFormat fmt;
    switch (type) {
        case AV_HWDEVICE_TYPE_VAAPI:
            fmt = AV_PIX_FMT_VAAPI;
            break;
        case AV_HWDEVICE_TYPE_DXVA2:
            fmt = AV_PIX_FMT_DXVA2_VLD;
            break;
        case AV_HWDEVICE_TYPE_D3D11VA:
            fmt = AV_PIX_FMT_D3D11;
            break;
        case AV_HWDEVICE_TYPE_VDPAU:
            fmt = AV_PIX_FMT_VDPAU;
            break;
        case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
            fmt = AV_PIX_FMT_VIDEOTOOLBOX;
            break;
        default:
            fmt = AV_PIX_FMT_NONE;
            break;
    }

    return fmt;
}

static enum AVPixelFormat hw_pix_fmt;
static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt) {
            //qDebug() << TIMEMS << "get_hw_format ok";
            return *p;
        }
    }

    qDebug() << TIMEMS << "Failed to get HW surface format";
    return AV_PIX_FMT_NONE;
}
#endif
#endif // FFMPEGHELPER_H
