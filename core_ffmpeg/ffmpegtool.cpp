#include "ffmpegtool.h"
#include "ffmpeghead.h"
#include "qapplication.h"
#include "qmutex.h"
#include "qfile.h"
#include "qdebug.h"

QScopedPointer<FFmpegTool> FFmpegTool::self;
FFmpegTool *FFmpegTool::Instance()
{
    if (self.isNull()) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (self.isNull()) {
            self.reset(new FFmpegTool);
        }
    }

    return self.data();
}

FFmpegTool::FFmpegTool(QObject *parent) : QObject(parent)
{
    //绑定信号槽
    connect(&process, SIGNAL(started()), this, SIGNAL(started()));
    connect(&process, SIGNAL(finished(int)), this, SIGNAL(finished()));
    connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(readData()));
    process.setProcessChannelMode(QProcess::MergedChannels);
}

void FFmpegTool::readData()
{
    QString data = process.readAllStandardOutput();
    emit receiveData(data);
}

void FFmpegTool::start(const QString &command)
{
    process.start(command);
}

void FFmpegTool::start(const QString &program, const QStringList &arguments)
{
    process.start(program, arguments);
}

void FFmpegTool::getMediaInfo(const QString &mediaFile, bool json)
{
    //ffprobe -print_format json -show_streams d:/out.mp4
    //不同平台可执行文件路径改成自己的
    QString jsonArg = "-print_format json -show_streams";
    QString binFile = qApp->applicationDirPath() + "/ffprobe.exe";
    QString cmd = QString("%1 %2 %3").arg(binFile).arg(json ? jsonArg : "").arg(mediaFile);
    start(cmd);
}

void FFmpegTool::h264ToMp4ByCmd(const QString &h264File, const QString &aacFile, const QString &mp4File)
{
    if (!QFile(h264File).exists() || mp4File.isEmpty()) {
        return;
    }

    //具体参数可以参考 https://www.cnblogs.com/renhui/p/9223969.html
    //ffmpeg.exe -y -i d:/1.aac -i d:/1.mp4 -map 0:0 -map 1:0 d:/out.mp4
    //-y参数表示默认yes覆盖文件
    //不同平台可执行文件路径改成自己的
    QString binFile = qApp->applicationDirPath() + "/ffmpeg.exe";

    //下面两种方法都可以,怎么方便怎么来
#if 0
    QString cmd = QString("%1 -y -i %2 -i %3 -map 0:0 -map 1:0 %4").arg(binFile).arg(h264File).arg(aacFile).arg(mp4File);
    start(cmd);
#else
    QStringList args;
    args << "-y";
    args << "-i" << h264File;

    //如果存在音频文件则添加
    if (QFile(aacFile).exists()) {
        args << "-i" << aacFile;
    }

    //args << "-map" << "0:0";
    //args << "-map" << "1:0";
    args << mp4File;
    start(binFile, args);
#endif
}

void FFmpegTool::h264ToMp4ByCode(const QString &h264File, const QString &aacFile, const QString &mp4File)
{
    if (!QFile(h264File).exists() || mp4File.isEmpty()) {
        return;
    }

    AVOutputFormat *fmt_out = NULL;
    AVFormatContext *fmt_h264 = NULL;
    AVFormatContext *fmt_aac = NULL;
    AVFormatContext *fmt_mp4 = NULL;

    int frame_index = 0;
    int64_t ts_h264 = 0;
    int64_t ts_aac = 0;
    AVPacket pkt;

    AVFormatContext *ifmt_ctx;
    AVStream *stream_in;
    AVStream *stream_out;
    AVBitStreamFilterContext *filter_h264;
    AVBitStreamFilterContext *filter_aac;

    int ret = -1;
    int videoindex_h264 = -1;
    int videoindex_mp4 = -1;
    int audioindex_aac = -1;
    int audioindex_mp4 = -1;
    int streamindex_mp4 = 0;

    const char *filename_h264 = h264File.toUtf8().constData();
    const char *filename_aac = aacFile.toUtf8().constData();
    const char *filename_mp4 = mp4File.toUtf8().constData();

    //在ffmpeg的类中已经初始化过了
    //avcodec_register_all();
    //av_register_all();

    if ((ret = avformat_open_input(&fmt_h264, filename_h264, NULL, NULL)) < 0) {
        qDebug() << TIMEMS << "avformat_open_input h264 error";
        goto end;
    }

    if ((ret = avformat_find_stream_info(fmt_h264, 0)) < 0) {
        qDebug() << TIMEMS << "avformat_find_stream_info h264 error";
        goto end;
    }

    if (QFile(aacFile).exists()) {
        if ((ret = avformat_open_input(&fmt_aac, filename_aac, NULL, NULL)) < 0) {
            qDebug() << TIMEMS << "avformat_open_input aac error";
            goto end;
        }

        if ((ret = avformat_find_stream_info(fmt_aac, 0)) < 0) {
            qDebug() << TIMEMS << "avformat_find_stream_info aac error";
            goto end;
        }
    }

    //创建输出文件
    if ((ret = avformat_alloc_output_context2(&fmt_mp4, NULL, NULL, filename_mp4)) < 0) {
        qDebug() << TIMEMS << "avformat_alloc_output_context2 mp4 error";
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    //视频部分
    for (int i = 0; i < fmt_h264->nb_streams; i++) {
        if (fmt_h264->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVStream *stream_in = fmt_h264->streams[i];
            AVStream *stream_out = avformat_new_stream(fmt_mp4, stream_in->codec->codec);
            if (!stream_out) {
                qDebug() << TIMEMS << "avformat_new_stream h264 error";
                ret = AVERROR_UNKNOWN;
                goto end;
            }

            videoindex_h264 = i;
            videoindex_mp4 = stream_out->index;
            //设置旋转角度
            //av_dict_set(&stream_out->metadata, "rotate", "90", 0);

            if (avcodec_copy_context(stream_out->codec, stream_in->codec) < 0) {
                qDebug() << TIMEMS << "avcodec_copy_context h264 error";
                goto end;
            }

            stream_out->codec->codec_tag = 0;
            if (fmt_mp4->oformat->flags & AVFMT_GLOBALHEADER) {
                stream_out->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            break;
        }
    }

    //音频部分
    if (!aacFile.isEmpty()) {
        for (int i = 0; i < fmt_aac->nb_streams; i++) {
            if (fmt_aac->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                AVStream *stream_in = fmt_aac->streams[i];
                AVStream *stream_out = avformat_new_stream(fmt_mp4, stream_in->codec->codec);

                if (!stream_out) {
                    qDebug() << TIMEMS << "avformat_new_stream aac error";
                    ret = AVERROR_UNKNOWN;
                    goto end;
                }

                audioindex_aac = i;
                audioindex_mp4 = stream_out->index;
                if (avcodec_copy_context(stream_out->codec, stream_in->codec) < 0) {
                    qDebug() << TIMEMS << "avcodec_copy_context aac error";
                    goto end;
                }

                stream_out->codec->codec_tag = 0;
                if (fmt_mp4->oformat->flags & AVFMT_GLOBALHEADER) {
                    stream_out->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                }

                break;
            }
        }
    }

    //打印文件信息
    //av_dump_format(fmt_h264, 0, filename_h264, 0);
    //av_dump_format(fmt_aac, 0, filename_aac, 0);
    //av_dump_format(fmt_mp4, 0, filename_mp4, 1);

    fmt_out = fmt_mp4->oformat;
    if (!(fmt_out->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmt_mp4->pb, filename_mp4, AVIO_FLAG_WRITE) < 0) {
            qDebug() << TIMEMS << "avio_open mp4 error";
            goto end;
        }
    }

    //写入头部标记
    if (avformat_write_header(fmt_mp4, NULL) < 0) {
        qDebug() << TIMEMS << "avformat_write_header mp4 error";
        goto end;
    }

    filter_h264 = av_bitstream_filter_init("h264_mp4toannexb");
    filter_aac = av_bitstream_filter_init("aac_adtstoasc");

    while (1) {
        //比较一帧
        if (av_compare_ts(ts_h264, fmt_h264->streams[videoindex_h264]->time_base, ts_aac, fmt_aac->streams[audioindex_aac]->time_base) <= 0) {
            //读取一帧视频写入
            ifmt_ctx = fmt_h264;
            streamindex_mp4 = videoindex_mp4;
            if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
                do {
                    stream_in  = ifmt_ctx->streams[pkt.stream_index];
                    stream_out = fmt_mp4->streams[streamindex_mp4];
                    if (pkt.stream_index == videoindex_h264) {
                        if (pkt.pts == AV_NOPTS_VALUE) {
                            AVRational time_base1 = stream_in->time_base;
                            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(stream_in->r_frame_rate);
                            pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
                            pkt.dts = pkt.pts;
                            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
                            frame_index++;
                        }
                        ts_h264 = pkt.pts;
                        break;
                    }
                } while (av_read_frame(ifmt_ctx, &pkt) >= 0);
            } else {
                break;
            }
        } else {
            //读取一帧音频写入
            ifmt_ctx = fmt_aac;
            streamindex_mp4 = audioindex_mp4;
            if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
                do {
                    stream_in = ifmt_ctx->streams[pkt.stream_index];
                    stream_out = fmt_mp4->streams[streamindex_mp4];
                    if (pkt.stream_index == audioindex_aac) {
                        if (pkt.pts == AV_NOPTS_VALUE) {
                            AVRational time_base1 = stream_in->time_base;
                            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(stream_in->r_frame_rate);
                            pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
                            pkt.dts = pkt.pts;
                            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
                            frame_index++;
                        }
                        ts_aac = pkt.pts;
                        break;
                    }
                } while (av_read_frame(ifmt_ctx, &pkt) >= 0);
            } else {
                break;
            }
        }

        av_bitstream_filter_filter(filter_h264, stream_in->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
        if (!aacFile.isEmpty()) {
            av_bitstream_filter_filter(filter_aac, stream_out->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
        }

        //转换 PTS/DTS
        enum AVRounding rnd = (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.pts = av_rescale_q_rnd(pkt.pts, stream_in->time_base, stream_out->time_base, rnd);
        pkt.dts = av_rescale_q_rnd(pkt.dts, stream_in->time_base, stream_out->time_base, rnd);
        pkt.duration = av_rescale_q(pkt.duration, stream_in->time_base, stream_out->time_base);
        pkt.stream_index = streamindex_mp4;
        pkt.pos = -1;

        qDebug() << TIMEMS << QString("正在写入包 size: %1  pts: %2").arg(pkt.size).arg(pkt.pts);
        if (av_interleaved_write_frame(fmt_mp4, &pkt) < 0) {
            qDebug() << TIMEMS << "写入包数据出错";
            break;
        }

        av_free_packet(&pkt);
    }

    //写文件结尾
    av_write_trailer(fmt_mp4);
    av_bitstream_filter_close(filter_h264);
    if (!aacFile.isEmpty()) {
        av_bitstream_filter_close(filter_aac);
    }

end:
    qDebug() << TIMEMS << "error";
    avformat_close_input(&fmt_h264);
    avformat_close_input(&fmt_aac);
    if (fmt_mp4 && !(fmt_out->flags & AVFMT_NOFILE)) {
        avio_close(fmt_mp4->pb);
    }

    avformat_free_context(fmt_mp4);
}
