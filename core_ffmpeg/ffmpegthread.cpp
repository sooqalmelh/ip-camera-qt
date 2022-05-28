#include "ffmpegthread.h"
#include "ffmpegsync.h"
#include "ffmpeghelper.h"
#include "qapplication.h"

FFmpegThread::FFmpegThread(QObject *parent) : QThread(parent)
{
    setObjectName("FFmpegThread");
    stopped = false;
    isPlay = false;
    isPause = false;
    isSnap = false;
    isRtsp = false;
    isUsbCamera = false;
    isInit = false;
    isSave = false;

    lastTime = QDateTime::currentDateTime();
    errorCount = 0;
    frameCount = 0;
    frameFinish = 0;
    videoWidth = 640;
    videoHeight = 480;
    frameRate = 0;
    rotate = 0;

    videoIndex = -1;
    audioIndex = -1;
    videoFps = 0;
    duration = -1;

    interval = 1;
    sleepTime = 0;
    readTime = 8000;
    checkTime = 3000;
    checkConn = false;

    multiMode = false;
    url = "";
    callback = false;
    hardware = "none";
    transport = "tcp";
    imageFlag = ImageFlag_Fast;

    saveFile = false;
    saveInterval = 0;
    savePath = qApp->applicationDirPath();
    fileFlag = "Ch1";
    fileName = QString("%1/%2_%3.mp4").arg(savePath).arg(fileFlag).arg(STRDATETIME);
    saveTime = QDateTime::fromString("1970-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");

    //定时器用于隔一段时间保存文件,文件名按照 yyyy-MM-dd-HH-mm-ss 格式
    timerSave = new QTimer(this);
    timerSave->setInterval(60 * 1000);
    connect(timerSave, SIGNAL(timeout()), this, SLOT(saveVideo()));

    packet = NULL;
    frameSrc = NULL;
    frameDst = NULL;
    videoFrame = NULL;
    audioFrame = NULL;

    formatCtx = NULL;
    videoCtx = NULL;
    audioCtx = NULL;
    videoSwsCtx = NULL;
    audioSwrCtx = NULL;

    videoData = NULL;
    audioData = NULL;
    videoCodec = NULL;
    audioCodec = NULL;
    options = NULL;

    playRepeat = false;
    playAudio = true;
    audioDeviceOk = false;
    audioDevice = 0;
    audioOutput = 0;

    packetCount = 0;
    saveMp4 = true;
    initSaveOk = false;
    formatOut = NULL;

    //音频dts头部数据
    int profile = 2;
    int freqIdx = 4;
    int chanCfg = 2;
    dtsData = (char *)malloc(sizeof(char) * 7);
    dtsData[0] = (char)0xFF;
    dtsData[1] = (char)0xF1;
    dtsData[2] = (char)(((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    dtsData[6] = (char)0xFC;

    //初始化注册,一个软件中只注册一次即可
    initlib();

    //启用线程队列处理帧,会音视频同步
    useList = true;

    //初始化音频解码同步线程
    audioSync = new FFmpegSync;
    connect(this, SIGNAL(receivePlayStart()), audioSync, SLOT(start()));
    connect(audioSync, SIGNAL(filePositionReceive(qint64)), this, SIGNAL(filePositionReceive(qint64)));
    audioSync->setType(0);
    audioSync->setThread(this);

    //初始化视频解码同步线程
    videoSync = new FFmpegSync;
    connect(this, SIGNAL(receivePlayStart()), videoSync, SLOT(start()));
    connect(videoSync, SIGNAL(filePositionReceive(qint64)), this, SIGNAL(filePositionReceive(qint64)));
    videoSync->setType(1);
    videoSync->setThread(this);

    //采用定时器来延时处理定位播放
    volume = 0;
    position = 0;
    timerPositon = new QTimer(this);
    connect(timerPositon, SIGNAL(timeout()), this, SLOT(setPosition()));
    timerPositon->setInterval(100);
}

FFmpegThread::~FFmpegThread()
{
    if (timerSave->isActive()) {
        timerSave->stop();
    }
    if (timerPositon->isActive()) {
        timerPositon->stop();
    }
}

bool FFmpegThread::init()
{
    if (url.isEmpty()) {
        return false;
    }

    //判断该摄像机是否能联通
    if (checkConn && isRtsp) {
        if (!checkUrl(url, checkTime)) {
            return false;
        }
    }

    //启动计时
    QElapsedTimer time;
    time.start();

    //初始化参数
    this->initOption();
    //初始化输入
    if (!initInput()) {
        return false;
    }
    //初始化视频
    if (!initVideo()) {
        return false;
    }
    //初始化音频
    if (!initAudio()) {
        return false;
    }
    //初始化其他
    this->initOther();

    QString useTime = QString::number((float)time.elapsed() / 1000, 'f', 3);
    qDebug() << TIMEMS << fileFlag << QString("初始化完 -> 用时: %1 秒  地址: %2").arg(useTime).arg(url);
    return true;
}

void FFmpegThread::initOption()
{
    //在打开码流前指定各种参数比如:探测时间/超时时间/最大延时等
    //设置缓存大小,1080p可将值调大,现在很多摄像机是2k可能需要调大,一般2k是1080p的四倍
    av_dict_set(&options, "buffer_size", "8192000", 0);
    //通信协议采用tcp还是udp,通过参数传入,不设置默认是udp
    //udp优点是无连接,在网线拔掉以后十几秒钟重新插上还能继续接收,缺点是网络不好的情况下会丢包花屏
    QByteArray data = transport.toUtf8();
    const char *rtsp_transport = data.constData();
    av_dict_set(&options, "rtsp_transport", rtsp_transport, 0);
    //设置超时断开连接时间,单位微秒,3000000表示3秒
    av_dict_set(&options, "stimeout", "3000000", 0);
    //设置最大时延,单位微秒,1000000表示1秒
    av_dict_set(&options, "max_delay", "1000000", 0);
    //自动开启线程数
    av_dict_set(&options, "threads", "auto", 0);
    //开启无缓存 RTMP等视频流不建议开启
    //av_dict_set(&options, "fflags", "nobuffer", 0);

    //增加rtp sdp支持 后面发现不要加
    if (url.endsWith(".sdp")) {
        //av_dict_set(&options, "protocol_whitelist", "file,rtp,udp", 0);
    }

    //单独对USB摄像机设置参数
    if (isUsbCamera) {
        //设置分辨率
        if (videoWidth > 0 && videoHeight > 0) {
            QString size = QString("%1x%2").arg(videoWidth).arg(videoHeight);
            av_dict_set(&options, "video_size", size.toUtf8().constData(), 0);
        }

        //设置帧率 如果是笔记本自带的摄像机建议不要设置
        if (frameRate != 0) {
            av_dict_set(&options, "framerate", QString::number(frameRate).toUtf8().constData(), 0);
        }

        //设置输入格式
        //av_dict_set(&options, "input_format", "mjpeg", 0);
        //可以打开下面这行用来打印设备列表,需要先开启ffmpeg的打印输出
        //av_dict_set(&options, "list_devices", "true", 0);
    }

    if (isUsbCamera) {
        //部分USB摄像机用现有的yuv绘制后有发蓝现象,部分USB摄像机采集后无法正常绘制,所以强制改成回调模式
        callback = true;
        //本地USB摄像机不需要硬解码,无论是否设置都强制改成无硬解码
        //视频流编码类型为 AV_CODEC_ID_RAWVIDEO 像素格式为 AV_PIX_FMT_YUYV422 不经过解码操作直接就可显示
        hardware = "none";
    }

    //没有启用opengl则强制改为回调
#ifndef opengl
    callback = true;
#endif

    //rtmp视频流强制改成存储成h264裸流,目前存储成mp4还有问题
    if (url.startsWith("rtmp", Qt::CaseInsensitive)) {
        saveMp4 = false;
    }
}

bool FFmpegThread::initInput()
{
    //实例化格式处理上下文
    formatCtx = avformat_alloc_context();
    //设置超时回调,有些不存在的地址或者网络不好的情况下要卡很久
    formatCtx->interrupt_callback.callback = AVInterruptCallBackFun;
    formatCtx->interrupt_callback.opaque = this;

    //必须要有tryOpen标志位来控制超时回调,由他来控制是否继续阻塞
    tryOpen = false;
    tryRead = true;

    //先判断是否是本地设备(video=设备名字符串),打开的方式不一样
    QByteArray urlData = url.toUtf8();
    AVInputFormat *ifmt = NULL;
    if (isUsbCamera) {
#if defined(Q_OS_WIN)
        ifmt = av_find_input_format("dshow");
#elif defined(Q_OS_LINUX)
        //ifmt = av_find_input_format("v4l2");
        ifmt = av_find_input_format("video4linux2");
#elif defined(Q_OS_MAC)
        ifmt = av_find_input_format("avfoundation");
#endif
    }

    //设置 avformat_open_input 非阻塞默认阻塞 不推荐这样设置推荐采用回调
    //formatCtx->flags |= AVFMT_FLAG_NONBLOCK;
    int result = avformat_open_input(&formatCtx, urlData.data(), ifmt, &options);
    tryOpen = true;
    if (result < 0) {
        qDebug() << TIMEMS << fileFlag << "open input error" << getError(result) << url;
        return false;
    }

    //释放设置参数
    if (options != NULL) {
        av_dict_free(&options);
    }

    //根据自己项目需要开启下面部分代码加快视频流打开速度
#if 0
    //接口内部读取的最大数据量,从源文件中读取的最大字节数
    //默认值5000000导致这里卡很久最耗时,可以调小来加快打开速度
    formatCtx->probesize = 50000;
    //从文件中读取的最大时长,单位为 AV_TIME_BASE units
    formatCtx->max_analyze_duration = 5 * AV_TIME_BASE;
    //内部读取的数据包不放入缓冲区
    //formatCtx->flags |= AVFMT_FLAG_NOBUFFER;
#endif

    //获取流信息
    result = avformat_find_stream_info(formatCtx, NULL);
    if (result < 0) {
        qDebug() << TIMEMS << fileFlag << "find stream info error" << getError(result);
        return false;
    }

    return true;
}

bool FFmpegThread::initVideo()
{
    //有些没有视频流,所以这里不用返回
    videoIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);
    if (videoIndex < 0) {
        qDebug() << TIMEMS << fileFlag << "find video stream index error";
    } else {
        //获取视频流
        AVStream *videoStream = formatCtx->streams[videoIndex];
        //如果选择了硬解码则根据硬解码的类型处理
        if (hardware == "none") {
            //获取视频流解码器,或者指定解码器
            videoCtx = videoStream->codec;
            videoCodec = avcodec_find_decoder(videoCtx->codec_id);
            //videoCodec = avcodec_find_decoder_by_name("h264");//h264_qsv
            if (videoCodec == NULL) {
                qDebug() << TIMEMS << fileFlag << "video decoder not found";
                return false;
            }
        } else if (hardware == "qsv") {
            if (!initHWDeviceQsv()) {
                return false;
            }
        } else {
            if (!initHWDeviceOther()) {
                return false;
            }
        }

        //设置加速解码 设置lowres=max_lowres的话很可能画面采用最小的分辨率
        //videoCtx->lowres = videoCodec->max_lowres;
        videoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        videoCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
        videoCtx->flags2 |= AV_CODEC_FLAG2_FAST;

        //打开视频解码器
        int result = avcodec_open2(videoCtx, videoCodec, NULL);
        if (result < 0) {
            qDebug() << TIMEMS << fileFlag << "open video codec error" << getError(result);
            return false;
        }

        //获取分辨率大小
        videoWidth = videoStream->codec->width;
        videoHeight = videoStream->codec->height;

        //如果没有获取到宽高则返回
        if (videoWidth == 0 || videoHeight == 0) {
            qDebug() << TIMEMS << fileFlag << "find width height error";
            return false;
        }

        videoFirstPts = videoStream->start_time;
        QString otherInfo = QString("其他信息 -> start_time: %1  r_frame_rate: %2  avg_frame_rate: %3")
                            .arg(videoStream->start_time).arg((int)av_q2d(videoStream->r_frame_rate)).arg((int)av_q2d(videoStream->avg_frame_rate));
        qDebug() << TIMEMS << fileFlag << otherInfo;

#if 0
        //获取视频流的帧率 fps,要对0进行过滤,除数不能为0,有些时候获取到的是0
        double num = videoStream->avg_frame_rate.num;
        double den = videoStream->avg_frame_rate.den;
        double videoFps = 0;
        //下面这个方法会产生 nan 结果不好判断
        //double videoFps = av_q2d(videoStream->avg_frame_rate);
        if (num != 0 && den != 0) {
            videoFps = num / den;
        }

        //网络流如果帧数为0则赋值 有些网络流获取不到fps
        if (videoFps == 0) {
            videoFps = 25;
            qDebug() << TIMEMS << fileFlag << "videoFps=0" << videoFps << url;
        }
#else
        double videoFps = av_q2d(videoStream->r_frame_rate);
#endif
        //如果没有主动设置fps则取当前读取到的fps
        //一般主动设置fps的情况是用来存储MP4文件使用
        //比如有时候视频流是25而存储需要改成15来存储时间才正确
        if (this->videoFps == 0) {
            this->videoFps = videoFps;
        }

        //分配视频数据内存
        quint64 byte = avpicture_get_size(AV_PIX_FMT_RGB32, videoWidth, videoHeight);
        videoData = (uint8_t *)av_malloc(byte * sizeof(uint8_t));
        if (!videoData) {
            av_free(videoData);
            return false;
        }

        QString videoInfo = QString("视频信息 -> 索引: %1  解码: %2  fps: %3  分辨率: %4*%5")
                            .arg(videoIndex).arg(videoCodec->name).arg(videoFps).arg(videoWidth).arg(videoHeight);
        qDebug() << TIMEMS << fileFlag << videoInfo;
    }

    return true;
}

bool FFmpegThread::initHWDeviceQsv()
{
#ifdef hardwarespeed
    //创建硬解码设备
    int result = av_hwdevice_ctx_create(&decode.hw_device_ref, AV_HWDEVICE_TYPE_QSV, "auto", NULL, 0);
    if (result < 0) {
        qDebug() << TIMEMS << fileFlag << "open the hardware device error" << getError(result);
        return false;
    }

    //英特尔处理器是h264_qsv,英伟达处理器是h264_cuvid
    videoCodec = avcodec_find_decoder_by_name("h264_qsv");
    if (videoCodec == NULL) {
        qDebug() << TIMEMS << fileFlag << "video decoder not found";
        return false;
    }

    AVStream *videoStream = formatCtx->streams[videoIndex];
    videoCtx = avcodec_alloc_context3(videoCodec);
    if (!videoCtx) {
        qDebug() << TIMEMS << fileFlag << "avcodec_alloc_context3 error";
        return false;
    }

    //貌似只支持264不支持265,不知道是不是电脑的硬件原因
    videoCtx->codec_id = AV_CODEC_ID_H264;
    if (videoStream->codecpar->extradata_size) {
        videoCtx->extradata = (uint8_t *)av_mallocz(videoStream->codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!videoCtx->extradata) {
            return false;
        }

        memcpy(videoCtx->extradata, videoStream->codecpar->extradata, videoStream->codecpar->extradata_size);
        videoCtx->extradata_size = videoStream->codecpar->extradata_size;
    }

    videoCtx->refcounted_frames = 1;
    videoCtx->opaque = &decode;
    videoCtx->get_format = get_qsv_format;
#endif
    return true;
}

bool FFmpegThread::initHWDeviceOther()
{
#ifdef hardwarespeed
    //根据名称自动寻找硬解码
    QByteArray hardwareData = hardware.toUtf8();
    enum AVHWDeviceType type = av_hwdevice_find_type_by_name(hardwareData.data());
    //qDebug() << TIMEMS << fileFlag << "AVHWDeviceType" << type << hardwareData;

    //找到对应的硬解码格式
    hw_pix_fmt = find_fmt_by_hw_type(type);
    if (hw_pix_fmt == -1) {
        qDebug() << TIMEMS << fileFlag << "cannot support hardware";
        return false;
    }

    AVStream *videoStream = formatCtx->streams[videoIndex];
    videoCtx = avcodec_alloc_context3(videoCodec);
    if (!videoCtx) {
        qDebug() << TIMEMS << fileFlag << "avcodec_alloc_context3 error";
        return false;
    }

    int result = avcodec_parameters_to_context(videoCtx, videoStream->codecpar);
    if (result < 0) {
        qDebug() << TIMEMS << fileFlag << "avcodec_parameters_to_context error";
        return false;
    }

    //解码器格式赋值为硬解码
    videoCtx->get_format = get_hw_format;
    //av_opt_set_int(videoCtx, "refcounted_frames", 1, 0);

    //创建硬解码设备
    AVBufferRef *hw_device_ref;
    result = av_hwdevice_ctx_create(&hw_device_ref, type, NULL, NULL, 0);
    if (result < 0) {
        qDebug() << TIMEMS << fileFlag << "open the hardware device error" << getError(result);
        return false;
    }

    videoCtx->hw_device_ctx = av_buffer_ref(hw_device_ref);
    av_buffer_unref(&hw_device_ref);
#endif
    return true;
}

bool FFmpegThread::initAudio()
{
    //有些没有音频流,所以这里不用返回
    audioIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);
    if (audioIndex < 0) {
        qDebug() << TIMEMS << fileFlag << "find audio stream index error";
    } else {
        //获取音频流
        AVStream *audioStream = formatCtx->streams[audioIndex];
        audioCtx = audioStream->codec;

        //获取音频流解码器,或者指定解码器
        audioCodec = avcodec_find_decoder(audioCtx->codec_id);
        //audioCodec = avcodec_find_decoder_by_name("aac");
        if (audioCodec == NULL) {
            qDebug() << TIMEMS << fileFlag << "audio codec not found";
            return false;
        }

        //打开音频解码器
        int result = avcodec_open2(audioCtx, audioCodec, NULL);
        if (result < 0) {
            qDebug() << TIMEMS << fileFlag << "open audio codec error" << getError(result);
            return false;
        }

        audioFirstPts = audioStream->start_time;

        //初始化音频设备
        int sampleRate = audioCtx->sample_rate;
        int sampleSize = 2;//av_get_bytes_per_sample(*audioCtx->sample_fmts) / 2;
        int channelCount = 2;//audioCtx->channels;//发现有些地址居然有6个声道
        QString codecName = audioCodec->name;//long_name

        audioDeviceOk = false;
        if (playAudio) {
            initAudioDevice(sampleRate, sampleSize, channelCount);
            if (audioDeviceOk) {
                //转换音频格式
                audioSwrCtx = swr_alloc();
                int64_t channelOut = AV_CH_LAYOUT_STEREO;
                int64_t channelIn = av_get_default_channel_layout(audioCtx->channels);
                audioSwrCtx = swr_alloc_set_opts(audioSwrCtx, channelOut, AV_SAMPLE_FMT_S16, sampleRate, channelIn, audioCtx->sample_fmt, sampleRate, 0, 0);
                audioDeviceOk = (swr_init(audioSwrCtx) >= 0);

                //分配音频数据内存 192000 这个值是看ffplay代码中的
                if (audioDeviceOk) {
                    quint64 byte = (192000 * 3) / 2;
                    audioData = (uint8_t *)av_malloc(byte * sizeof(uint8_t));
                    if (!audioData) {
                        audioDeviceOk = false;
                        av_free(audioData);
                        return false;
                    }
                }
            }
        }

        QString audioInfo = QString("音频信息 -> 索引: %1  解码: %2  比特率: %3  声道数: %4  采样: %5")
                            .arg(audioIndex).arg(codecName).arg(formatCtx->bit_rate).arg(channelCount).arg(sampleRate);
        qDebug() << TIMEMS << fileFlag << audioInfo;
    }

    return true;
}

void FFmpegThread::initAudioDevice(int sampleRate, int sampleSize, int channelCount)
{
    //6.2.0 6.2.1 的多媒体模块暂时不支持mingw编译器
    //6.2.2开始采用的mingw9.0版本已经可以支持多媒体模块
#ifdef Q_CC_GNU
#if (QT_VERSION >= QT_VERSION_CHECK(6,2,0)) && (QT_VERSION <= QT_VERSION_CHECK(6,2,1))
    return;
#endif
#endif

#ifdef multimedia
    QAudioFormat format;
    //为什么这里限定的是6.2.0呢因为msvc从6.2.0就支持
#if (QT_VERSION >= QT_VERSION_CHECK(6,2,0))
    QAudioDevice defaultDeviceInfo = QMediaDevices::defaultAudioOutput();
    format = defaultDeviceInfo.preferredFormat();
    format.setSampleRate(sampleRate);
    format.setChannelCount(channelCount);
#else
    QAudioDeviceInfo defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    format = defaultDeviceInfo.preferredFormat();
    format.setCodec("audio/pcm");
    format.setSampleRate(sampleRate);
    format.setSampleSize(sampleSize * 8);
    format.setChannelCount(channelCount);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
#endif

    //测试下来发现直接用默认设备的默认format直接支持各种视频文件但是不支持视频流的声音
    //format = defaultDeviceInfo.preferredFormat();
    audioDeviceOk = defaultDeviceInfo.isFormatSupported(format);
    if (audioDeviceOk) {
#if (QT_VERSION >= QT_VERSION_CHECK(6,2,0))
        audioOutput = new QAudioSink(defaultDeviceInfo, format);
#else
        audioOutput = new QAudioOutput(defaultDeviceInfo, format);
#endif

        //设置下缓存不然部分文件播放音频一卡卡 也不能太大 太大可能会导致崩溃分配内存失败
        //windows需要而linux不需要 linux加了会声音延迟 不知道什么原因
#ifdef Q_OS_WIN
        audioOutput->setBufferSize(655360);
#endif
        audioOutput->setVolume(1.0);
        audioDevice = audioOutput->start();
        //发送音量静音信号 其实这里可以省略 默认就是全音量+非静音
        emit fileVolumeReceive(getVolume(), getMuted());
    } else {
        qDebug() << TIMEMS << fileFlag << "Raw audio format not supported by backend, cannot play audio.";
    }
#endif
}

void FFmpegThread::freeAudioDevice()
{
    if (audioDeviceOk) {
#ifdef multimedia
        audioDevice->close();
        audioDevice->deleteLater();
        audioDeviceOk = false;
        //audioOutput->setBufferSize(0);
        //audioOutput->stop();
        //audioOutput->deleteLater();
#endif
    }
}

void FFmpegThread::initMp3()
{
    //读取MP3文件信息
    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(formatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        qDebug() << TIMEMS << fileFlag << tag->key << tag->value;
    }

    //读取封面
    if (formatCtx->iformat->read_header(formatCtx) >= 0) {
        for (int i = 0; i < formatCtx->nb_streams; i++) {
            if (formatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                isInit = true;
                AVPacket pkt = formatCtx->streams[i]->attached_pic;
                QImage image = QImage::fromData((uchar *)pkt.data, pkt.size);
                emit receiveImage(image);
                break;
            }
        }
    }
}

void FFmpegThread::initOther()
{
#ifndef gcc45
    packet = av_packet_alloc();
#else
    packet = new AVPacket;
#endif

    if (audioIndex >= 0) {
        audioFrame = av_frame_alloc();
    }

    //视频流索引存在才需要设置视频解码格式等
    if (videoIndex >= 0) {
        frameSrc = av_frame_alloc();
        frameDst = av_frame_alloc();
        videoFrame = av_frame_alloc();

        //定义像素格式
        AVPixelFormat srcFormat = AV_PIX_FMT_YUV420P;
        AVPixelFormat dstFormat = AV_PIX_FMT_RGB32;

        //重新设置源图片格式
        if (hardware == "none") {
            //通过解码器获取解码格式
            srcFormat = videoCtx->pix_fmt;
        } else {
            //硬件加速需要指定格式为 AV_PIX_FMT_NV12
            srcFormat = AV_PIX_FMT_NV12;
        }

        //默认最快速度的解码采用的SWS_FAST_BILINEAR参数,可能会丢失部分图片数据,可以自行更改成其他参数
        int flags = SWS_FAST_BILINEAR;
        if (imageFlag == ImageFlag_Fast) {
            flags = SWS_FAST_BILINEAR;
        } else if (imageFlag == ImageFlag_Full) {
            flags = SWS_BICUBIC;
        } else if (imageFlag == ImageFlag_Even) {
            flags = SWS_BILINEAR;
        }

        //开辟缓存存储一帧数据
        av_image_fill_arrays(frameDst->data, frameDst->linesize, videoData, dstFormat, videoWidth, videoHeight, 1);
        //图像转换
        videoSwsCtx = sws_getContext(videoWidth, videoHeight, srcFormat, videoWidth, videoHeight, dstFormat, flags, NULL, NULL, NULL);

        QString formatInfo = QString("格式信息 -> 源头格式: %1  目标格式: %2").arg(srcFormat).arg(dstFormat);
        qDebug() << TIMEMS << fileFlag << formatInfo;
    }

    //解码格式
    formatName = formatCtx->iformat->name;
    //如果格式是rtsp视频流则不采用队列处理直接本线程解码,不需要同步响应速度快
    if (formatName == "rtsp" || url.endsWith(".sdp")) {
        useList = false;
    }

    //设置了最快速度则不启用排队处理
    if (imageFlag == ImageFlag_Fast2) {
        useList = false;
    }

    //有些格式不支持硬解码
    if (formatName == "rm") {
        hardware = "none";
    }

    //发送文件时长信号
    duration = formatCtx->duration / AV_TIME_BASE;
    //时长正常则改成非rtsp 比如http的MP4就是类似正常文件
    if (!isUsbCamera) {
        isRtsp = (duration <= 0);
    }

    if (!isRtsp && !isUsbCamera) {
        qint64 length = duration * 1000;
        emit fileLengthReceive(length);
    }

    QString fileInfo = QString("文件信息 -> 格式: %1  时长: %2 秒  解码: %3").arg(formatName).arg(duration).arg(hardware);
    qDebug() << TIMEMS << fileFlag << fileInfo;

    //还有一种情况获取到了MP3文件的封面也在这里
    if (formatName == "mp3") {
        videoIndex = -1;
        this->initMp3();
    }

    //这里过滤就算开启了视频存储但是当前视频流不符合存储要求
    //比如限定只有视频流才需要存储
    if (videoIndex < 0) {
        saveFile = false;
    } else if (!isRtsp) {
        //后面发现 http形式的MP4也可以保存
        if (!(url.startsWith("http") && url.endsWith("mp4"))) {
            saveFile = false;
        }
    }

#if 0
    for (uint i = 0; i < formatCtx->nb_streams; i++) {
        AVMediaType type = formatCtx->streams[i]->codec->codec_type;
        qDebug() << TIMEMS << fileFlag << QString("流信息 -> 类型: %1  索引: %2").arg(type).arg(i);;
    }

    //输出视频信息
    QByteArray urlData = url.toUtf8();
    av_dump_format(formatCtx, 0, urlData.data(), 0);
#endif
}

void FFmpegThread::free()
{
    //停止解码同步线程
    if (audioSync->isRunning()) {
        audioSync->stop();
        audioSync->quit();
        audioSync->wait();
    }

    if (videoSync->isRunning()) {
        videoSync->stop();
        videoSync->quit();
        videoSync->wait();
    }

    //关闭文件
    if (fileVideo.isOpen()) {
        fileVideo.close();
    }

    if (fileAudio.isOpen()) {
        fileAudio.close();
    }

    if (videoSwsCtx != NULL) {
        sws_freeContext(videoSwsCtx);
        videoSwsCtx = NULL;
    }

    if (audioSwrCtx != NULL) {
        swr_free(&audioSwrCtx);
        audioSwrCtx = NULL;
    }

    if (packet != NULL) {
        av_packet_unref(packet);
        packet = NULL;
    }

    if (frameSrc != NULL) {
        av_frame_free(&frameSrc);
        frameSrc = NULL;
    }

    if (frameDst != NULL) {
        av_frame_free(&frameDst);
        frameDst = NULL;
    }

    if (videoFrame != NULL) {
        av_frame_free(&videoFrame);
        videoFrame = NULL;
    }

    if (audioFrame != NULL) {
        av_frame_free(&audioFrame);
        audioFrame = NULL;
    }

    if (videoData != NULL) {
        av_free(videoData);
        videoData = NULL;
    }

    if (audioData != NULL) {
        av_free(audioData);
        audioData = NULL;
    }

    if (videoCtx != NULL) {
        avcodec_close(videoCtx);
        videoCtx = NULL;
    }

    if (audioCtx != NULL) {
        avcodec_close(audioCtx);
        audioCtx = NULL;
    }

    if (formatCtx != NULL) {
        avformat_close_input(&formatCtx);
        formatCtx = NULL;
    }

    av_dict_free(&options);

    stopped = false;
    isPlay = false;
    isPause = false;
    isInit = false;
    isSave = false;
    initSaveOk = false;
    useList = true;

    errorCount = 0;
    frameCount = 0;
    packetCount = 0;
    videoWidth = 640;
    videoHeight = 480;
    frameRate = 0;
    rotate = 0;
}

void FFmpegThread::free(AVFrame *frame)
{
    if (frame != NULL) {
        av_frame_free(&frame);
        frame = NULL;
    }
}

void FFmpegThread::free(AVPacket *packet)
{
    //av_packet_free(&packet);
    av_packet_unref(packet);
    //av_freep(packet);
}

void FFmpegThread::saveFileMp4(AVPacket *packet)
{
    //定时保存文件需要重新计算 pts dts
    if (saveFile && initSaveOk && !isSave) {
        packetCount++;
        AVStream *streamOut = formatOut->streams[videoIndex];
        packet->pts = (packetCount * streamOut->time_base.den) / (streamOut->time_base.num * videoFps);
        packet->dts = packet->pts;
        av_write_frame(formatOut, packet);
        //av_interleaved_write_frame(formatOut, packet);
        //qDebug() << TIMEMS << fileFlag << packetCount << videoFps << packet->pts << streamOut->time_base.num << streamOut->time_base.den;
    }
}

void FFmpegThread::saveFileH264(AVPacket *packet)
{
    if (fileVideo.isOpen() && !isSave) {
        int packetSize = packet->size;
        av_bsf_filter(filter, packet, formatCtx->streams[videoIndex]->codecpar);
        fileVideo.write((const char *)packet->data, packetSize);
    }
}

void FFmpegThread::saveFileAac(AVPacket *packet)
{
    if (fileAudio.isOpen() && !isSave) {
        //先写入dts头,再写入音频流数据
        int packetSize = packet->size;
        dtsData[3] = (char)(((2 & 3) << 6) + ((7 + packetSize) >> 11));
        dtsData[4] = (char)(((7 + packetSize) & 0x7FF) >> 3);
        dtsData[5] = (char)((((7 + packetSize) & 7) << 5) + 0x1F);
        fileAudio.write((const char *)dtsData, 7);
        fileAudio.write((const char *)packet->data, packetSize);
    }
}

void FFmpegThread::decodeVideo(AVPacket *packet)
{
    //有些监控视频保存的MP4文件首帧开始的时间不是0 需要减去
    if (videoFirstPts > AV_TIME_BASE) {
        packet->pts -= videoFirstPts;
        packet->dts = packet->pts;
    }

    if (useList) {
        //加入到队列交给解码同步线程处理
        videoSync->append(getNewPacket(packet));
    } else {
        //直接当前线程解码
        decodeVideo1(packet);
        if (imageFlag != ImageFlag_Fast2) {
            delayTime(formatCtx, packet, startTime);
        }
    }
}

void FFmpegThread::decodeVideo1(AVPacket *packet)
{
    if (hardware == "none") {
#ifndef gcc45
        frameFinish = avcodec_send_packet(videoCtx, packet);
        if (frameFinish >= 0) {
            //理论上需要循环取出帧 大部分时候其实只有一帧
            frameFinish = avcodec_receive_frame(videoCtx, videoFrame);
            decodeVideo2(packet);
        }
#else
        avcodec_decode_video2(videoCtx, videoFrame, &frameFinish, packet);
        decodeVideo2(packet);
#endif
    } else {
        frameFinish = decode_packet(videoCtx, packet, frameSrc, videoFrame);
        decodeVideo2(packet);
    }
}

void FFmpegThread::decodeVideo2(AVPacket *packet)
{
    //可能会遇到解码失败的包
    if (frameFinish < 0) {
        free(packet);
        msleep(1);
        return;
    }

    //计数,只有到了设定的帧率才继续
    frameCount++;
    if (frameCount != interval) {
        free(packet);
        msleep(1);
        return;
    } else {
        frameCount = 0;
    }

    //保存视频流数据到文件
    if (saveMp4) {
        saveFileMp4(packet);
    } else {
        saveFileH264(packet);
    }

    //判断是否设置了存储单个视频文件的保存时间
    if (saveFile && saveInterval == 0 && saveTime.date().year() != 1970) {
        qint64 offset = QDateTime::currentDateTime().secsTo(saveTime);
        if (offset < 0) {
            saveTime = QDateTime::fromString("1970-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");
            QMetaObject::invokeMethod(this, "stopSave");
        }
    }

    //截图+回调都需要采用原图的机制而不是交给opengl绘制
    //如果采用的不是回调先要判断是否启用了opengl启用的话则用opengl绘制
    if (isSnap || callback) {
        decodeImage();
    } else {
        emit receiveFrame(videoFrame);
    }
}

void FFmpegThread::decodeImage()
{
    //将数据转成一张图片
    int result = sws_scale(videoSwsCtx, (const uint8_t *const *)videoFrame->data,
                           videoFrame->linesize, 0, videoHeight, frameDst->data, frameDst->linesize);
    if (result >= 0) {
        QImage image((uchar *)videoData, videoWidth, videoHeight, QImage::Format_RGB32);
        if (!image.isNull()) {
            //增加旋转角度判断
            if (getRotate() > 0) {
                QTransform matrix;
                matrix.rotate(rotate);
                image = image.transformed(matrix, Qt::SmoothTransformation);
            }

            if (isSnap) {
                emit snapImage(image);
            } else {
                emit receiveImage(image);
            }
        }
    }

    isSnap = false;
}

void FFmpegThread::decodeAudio(AVPacket *packet)
{
    if (audioFirstPts > AV_TIME_BASE) {
        packet->pts -= audioFirstPts;
        packet->dts = packet->pts;
    }

    if (useList) {
        //加入到队列交给解码同步线程处理
        audioSync->append(getNewPacket(packet));
    } else {
        //直接当前线程解码
        decodeAudio1(packet);
        if (imageFlag != ImageFlag_Fast2) {
            delayTime(formatCtx, packet, startTime);
        }
    }
}

void FFmpegThread::decodeAudio1(AVPacket *packet)
{
    //没有启用解码音频
    if (!playAudio) {
        return;
    }

    //保存音频流数据到文件
    saveFileAac(packet);

    //设备不正常则不解码
    if (!audioDeviceOk) {
        return;
    }

    //解码音频流
#ifndef gcc45
    frameFinish = avcodec_send_packet(audioCtx, packet);
    if (frameFinish >= 0) {
        frameFinish = avcodec_receive_frame(audioCtx, audioFrame);
        decodeAudio2(packet);
    }
#else
    avcodec_decode_audio4(audioCtx, audioFrame, &frameFinish, packet);
    decodeAudio2(packet);
#endif
}

void FFmpegThread::decodeAudio2(AVPacket *packet)
{
    if (frameFinish >= 0) {
        //qDebug() << TIMEMS << fileFlag << audioOutput->bytesFree() << audioOutput->periodSize();
        int outChannel = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
        int outSize = av_samples_get_buffer_size(NULL, outChannel, audioFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
        int result = swr_convert(audioSwrCtx, &audioData, outSize, (const uint8_t **)audioFrame->data, audioFrame->nb_samples);
        if (result >= 0) {
            audioDevice->write((char *)audioData, outSize);
        }
    }
}

void FFmpegThread::run()
{
    //记住开始解码的时间用于用视频同步
    startTime = av_gettime();
    while (!stopped) {
        //根据标志位执行初始化操作
        if (isPlay) {
            if (init()) {
                //这里也需要更新下最后的时间
                lastTime = QDateTime::currentDateTime();
                initSave();
                //初始化完成变量放在这里,绘制那边判断这个变量是否完成才需要开始绘制
                if (videoIndex >= 0) {
                    isInit = true;
                }
                emit receivePlayStart();
            } else {
                emit receivePlayError();
                break;
            }

            isPlay = false;
            continue;
        }

        //处理暂停 本地文件才会执行到这里 视频流的暂停在其他地方处理
        if (isPause) {
            //这里需要假设正常,暂停期间继续更新时间
            lastTime = QDateTime::currentDateTime();
            msleep(1);
            continue;
        }

        //QMutexLocker locker(&mutex);
        //解码队列中帧数过多暂停读取 下面这两个值可以自行调整 表示缓存的大小
        if (videoSync->getPacketCount() >= 100 || audioSync->getPacketCount() >= 100) {
            msleep(1);
            continue;
        }

        //必须要有tryRead标志位来控制超时回调,由他来控制是否继续阻塞
        tryRead = false;

        //下面还有个可以改进的地方就是如果是视频流暂停情况下只要保证 av_read_frame 一直读取就行无需解码处理
        frameFinish = av_read_frame(formatCtx, packet);
        //qDebug() << TIMEMS << fileFlag << "av_read_frame" << frameFinish;
        if (frameFinish >= 0) {
            tryRead = true;
            //更新最后的解码时间 错误计数清零
            errorCount = 0;
            lastTime = QDateTime::currentDateTime();
            //判断当前包是视频还是音频
            int index = packet->stream_index;
            if (index == videoIndex) {
                //qDebug() << TIMEMS << fileFlag << "videoPts" << qint64(getPtsTime(formatCtx, packet) / 1000) << packet->pts << packet->dts;
                decodeVideo(packet);
            } else if (index == audioIndex) {
                //qDebug() << TIMEMS << fileFlag << "audioPts" << qint64(getPtsTime(formatCtx, packet) / 1000) << packet->pts << packet->dts;
                decodeAudio(packet);
            }
        } else if (!isRtsp) {
            //如果不是视频流则说明是视频文件播放完毕
            if (frameFinish == AVERROR_EOF) {
                //当同步队列中的数量为0才需要跳出 表示解码处理完成
                if (videoSync->getPacketCount() == 0 && audioSync->getPacketCount() == 0) {
                    //循环播放则重新设置播放位置,在这里执行的代码可以做到无缝切换循环播放
                    if (playRepeat) {
                        this->position = 0;
                        videoSync->reset();
                        audioSync->reset();
                        videoSync->start();
                        audioSync->start();
                        QMetaObject::invokeMethod(this, "setPosition", Q_ARG(qint64, position));
                        qDebug() << TIMEMS << fileFlag << "repeat" << url;
                    } else {
                        break;
                    }
                }
            }
        } else {
            //下面这种情况在摄像机掉线后出现,如果想要快速识别这里直接break即可
            //一般3秒钟才会执行一次错误累加
            errorCount++;
            //qDebug() << TIMEMS << fileFlag << "errorCount" << errorCount << url;
            if (errorCount >= 3) {
                errorCount = 0;
                break;
            }
        }

        free(packet);
        msleep(1);
    }

    QMetaObject::invokeMethod(this, "stopSave");

    //线程结束后释放资源
    msleep(100);
    free();
    freeAudioDevice();
    emit receivePlayFinsh();
    //qDebug() << TIMEMS << fileFlag << "stop ffmpeg thread" << url;
}

void FFmpegThread::initSaveFileName()
{
    QString dirName = QString("%1/%2").arg(savePath).arg(QDATE);
    newDir(dirName);
    fileName = QString("%1/%2_%3.mp4").arg(dirName).arg(fileFlag).arg(STRDATETIME);
}

void FFmpegThread::initSave()
{
    if (!saveFile) {
        return;
    }

    //如果存储间隔大于0说明需要定时存储
    if (saveInterval > 0) {
        initSaveFileName();
        QMetaObject::invokeMethod(this, "startSave");
    }

    if (saveMp4) {
        saveVideoMp4(fileName);
    } else {
        saveVideoH264(fileName);
    }
}

void FFmpegThread::startSave()
{
    timerSave->start(saveInterval * 1000);
}

void FFmpegThread::stopSave()
{
    //停止存储文件以及存储定时器
    closeVideo();
    if (timerSave->isActive()) {
        timerSave->stop();
    }
}

void FFmpegThread::saveVideo()
{
    QMutexLocker locker(&mutex);
    isSave = true;

    //重新设置文件名称
    initSaveFileName();

    if (saveMp4) {
        saveVideoMp4(fileName);
    } else {
        saveVideoH264(fileName);
    }

    isSave = false;
}

void FFmpegThread::saveVideoMp4(const QString &fileName)
{
    //先关闭视频如果打开了的话
    closeVideo();

    //转换文件字符串
    QByteArray fileData = fileName.toUtf8();
    const char *filename = fileData.data();
    //开辟一个格式上下文用来处理视频流输出
    avformat_alloc_output_context2(&formatOut, NULL, "mp4", filename);

    //开辟一个视频流用来输出mp4文件
    AVStream *streamIn = formatCtx->streams[videoIndex];
    AVStream *streamOut = avformat_new_stream(formatOut, NULL);

    AVCodecContext *codecIn = streamIn->codec;
    AVCodecContext *codecOut = streamOut->codec;
    qDebug() << TIMEMS << fileFlag << "保存视频" << fileName;
    qDebug() << TIMEMS << fileFlag << "编码参数" << videoFps << streamIn->time_base.num << streamIn->time_base.den << codecIn->time_base.num << codecIn->time_base.den;

    //有部分视频参数不正确保存不了 http://tv.netxt.cc:1998/live/y.flv
    if (codecIn->time_base.num == 0) {
        return;
    }

    //重新设置输出视频流的各种参数
    if (url.startsWith("http") && url.endsWith("mp4")) {
#ifndef gcc45
        avcodec_parameters_to_context(codecOut, streamIn->codecpar);
#endif
    } else {
        codecOut->bit_rate = 400000;
        codecOut->has_b_frames = 0;
        codecOut->time_base.num = streamIn->time_base.num;
        codecOut->time_base.den = streamIn->time_base.den;
        codecOut->codec_id = codecIn->codec_id;
        codecOut->codec_type = codecIn->codec_type;
        codecOut->width = codecIn->width;
        codecOut->height = codecIn->height;
        codecOut->pix_fmt = codecIn->pix_fmt;
        codecOut->me_range = codecIn->me_range;
        codecOut->max_qdiff = codecIn->max_qdiff;
        codecOut->qmin = codecIn->qmin;
        codecOut->qmax = codecIn->qmax;
        codecOut->qcompress = codecIn->qcompress;
        codecOut->flags = codecIn->flags;
        codecOut->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //打开输出文件并写入头部标识
    if (avio_open2(&formatOut->pb, filename, AVIO_FLAG_WRITE, NULL, NULL) >= 0) {
        if (avformat_write_header(formatOut, NULL) >= 0) {
            initSaveOk = true;
        }
    }
}

void FFmpegThread::saveVideoH264(const QString &fileName)
{
    //先关闭视频如果打开了的话
    closeVideo();

    initSaveOk = true;
    if (videoIndex >= 0) {
        fileVideo.setFileName(fileName);
        fileVideo.open(QFile::WriteOnly);
    }

    //存在音频文件则同时保存音频文件
    if (audioIndex >= 0 && playAudio) {
        QString audioName = fileName;
        audioName.replace(QFileInfo(audioName).suffix(), "aac");
        fileAudio.setFileName(audioName);
        fileAudio.open(QFile::WriteOnly);
    }
}

void FFmpegThread::closeVideo()
{
    if (saveMp4) {
        closeVideoMp4();
    } else {
        closeVideoH264();
    }

    initSaveOk = false;
}

void FFmpegThread::closeVideoMp4()
{
    if (formatOut != NULL) {
        //写入结束标识
        packetCount = 0;
        av_write_trailer(formatOut);
        avcodec_close(formatOut->streams[0]->codec);
        av_freep(&formatOut->streams[0]->codec);
        av_freep(&formatOut->streams[0]);
        avio_close(formatOut->pb);
        av_free(formatOut);
        formatOut = NULL;
    }
}

void FFmpegThread::closeVideoH264()
{
    if (fileVideo.isOpen()) {
        fileVideo.close();
    }

    if (fileAudio.isOpen()) {
        fileAudio.close();
    }
}

qint64 FFmpegThread::getStartTime()
{
    return this->startTime;
}

int FFmpegThread::getCheckTime()
{
    return this->checkTime;
}

int FFmpegThread::getReadTime()
{
    return this->readTime;
}

bool FFmpegThread::getTryOpen()
{
    return this->tryOpen;
}

bool FFmpegThread::getTryRead()
{
    return this->tryRead;
}

bool FFmpegThread::getTryStop()
{
    return this->stopped;
}

QDateTime FFmpegThread::getLastTime()
{
    return this->lastTime;
}

QString FFmpegThread::getUrl()
{
    return this->url;
}

bool FFmpegThread::getCallback()
{
    return this->callback;
}

QString FFmpegThread::getHardware()
{
    return this->hardware;
}

bool FFmpegThread::getIsRtsp()
{
    return this->isRtsp;
}

bool FFmpegThread::getIsUsbCamera()
{
    return this->isUsbCamera;
}

int FFmpegThread::getVideoWidth()
{
    return this->videoWidth;
}

int FFmpegThread::getVideoHeight()
{
    return this->videoHeight;
}

bool FFmpegThread::getOnlyAudio()
{
    return (multiMode ? false : videoIndex < 0);
}

bool FFmpegThread::getIsInit()
{
    return (multiMode ? true : isInit);
}

bool FFmpegThread::getIsPlaying()
{
    return (multiMode ? true : isRunning());
}

qint64 FFmpegThread::getLength()
{
    return duration * 1000;
}

qint64 FFmpegThread::getPosition()
{
    return 0;
}

float FFmpegThread::getRate()
{
    return 1.0;
}

void FFmpegThread::setRate(float rate)
{

}

void FFmpegThread::setPosition()
{
    if (this->isRunning() && !isRtsp && !isUsbCamera) {
        timerPositon->stop();
        audioSync->clear();
        videoSync->clear();

        //发过来的是毫秒而参数需要微秒
        int64_t timestamp = position * 1000;
        //AVSEEK_FLAG_BACKWARD-找到附近画面清晰的帧 AVSEEK_FLAG_ANY-直接定位到指定帧不管画面是否清晰
        av_seek_frame(formatCtx, -1, timestamp, AVSEEK_FLAG_BACKWARD);

        //继续播放并修改暂停标志位
        this->next();
        QWidget *w = (QWidget *)this->parent();
        if (w != 0) {
            w->setProperty("isPause", false);
        }
    }
}

void FFmpegThread::setPosition(qint64 position)
{
    if (this->isRunning() && !isRtsp && !isUsbCamera) {
        timerPositon->stop();
        timerPositon->start();
        this->position = position;

        //先暂停播放等待定时器去启动重新设置
        //用定时器设置有个好处避免重复频繁设置进度
        this->pause();
        QWidget *w = (QWidget *)this->parent();
        if (w != 0) {
            w->setProperty("isPause", true);
        }
    }
}

bool FFmpegThread::getMuted()
{
    return (getVolume() == 0);
}

void FFmpegThread::setMuted(bool muted)
{
    //先记住之前的音量以便重新设置
    if (muted) {
        volume = getVolume();
        setVolume(0);
    } else {
        setVolume(volume);
    }
}

int FFmpegThread::getVolume()
{
    int volume = 0;
    if (this->isRunning() && audioDeviceOk) {
#ifdef multimedia
        volume = audioOutput->volume() * 100;
#endif
    }
    return volume;
}

void FFmpegThread::setVolume(int volume)
{
    if (this->isRunning() && audioDeviceOk) {
#ifdef multimedia
        //查阅手册说范围值是 0.0 - 1.0
        audioOutput->setVolume((float)volume / 100.0);
#endif
    }
}

int FFmpegThread::getRotate()
{
    //说明已经获取过旋转角度不用再去获取
    if (rotate > 0) {
        return rotate;
    }

    if (videoIndex >= 0) {
        AVDictionaryEntry *tag = NULL;
        AVStream *streamIn = formatCtx->streams[videoIndex];
        tag = av_dict_get(streamIn->metadata, "rotate", tag, 0);
        if (tag != NULL) {
            rotate = atoi(tag->value);
        }
    }

    return rotate;
}

void FFmpegThread::setInterval(int interval)
{
    if (interval > 0) {
        this->interval = interval;
        this->frameCount = 0;
    }
}

void FFmpegThread::setVideoFps(double videoFps)
{
    if (videoFps > 0) {
        this->videoFps = videoFps;
    }
}

void FFmpegThread::setSleepTime(int sleepTime)
{
    if (sleepTime > 0) {
        this->sleepTime = sleepTime;
    }
}

void FFmpegThread::setReadTime(int readTime)
{
    this->readTime = readTime;
}

void FFmpegThread::setCheckTime(int checkTime)
{
    this->checkTime = checkTime;
}

void FFmpegThread::setCheckConn(bool checkConn)
{
    this->checkConn = checkConn;
}

void FFmpegThread::setPlayRepeat(bool playRepeat)
{
    this->playRepeat = playRepeat;
}

void FFmpegThread::setPlayAudio(bool playAudio)
{
    this->playAudio = playAudio;
}

void FFmpegThread::setSaveMp4(bool saveMp4)
{
    this->saveMp4 = saveMp4;
}

void FFmpegThread::setMultiMode(bool multiMode, int videoWidth, int videoHeight)
{
    this->multiMode = multiMode;
    this->videoWidth = videoWidth;
    this->videoHeight = videoHeight;
}

void FFmpegThread::setUrl(const QString &url)
{
    this->url = url;
    if (url.isEmpty()) {
        return;
    }

    //可以自行增加判断
    isRtsp = (this->url.startsWith("rtsp", Qt::CaseInsensitive) ||
              this->url.startsWith("rtmp", Qt::CaseInsensitive) ||
              this->url.startsWith("http", Qt::CaseInsensitive));
    isUsbCamera = (this->url.startsWith("video") || this->url.startsWith("/dev/"));

    //如果是USB摄像头检查后面是否带了宽度高度,带了则自动设置宽高
    //video=USB2.0 PC CAMERA|1920x1080|25
    if (isUsbCamera && url.contains("|") && url.contains("x")) {
        QStringList list = url.split("|");
        int count = list.count();
        if (count >= 2) {
            //重新赋值地址
            this->url = list.at(0);
            QString size = list.at(1);
            QStringList temp = size.split("x");
            videoWidth = temp.at(0).toInt();
            videoHeight = temp.at(1).toInt();
            qDebug() << TIMEMS << fileFlag << "调整尺寸" << url << size;

            //如果还有第三个参数则为帧率
            if (count >= 3) {
                frameRate = list.at(2).toInt();
                qDebug() << TIMEMS << fileFlag << "调整帧率" << url << frameRate;
            }
        }
    }
}

void FFmpegThread::setCallback(bool callback)
{
    this->callback = callback;
}

void FFmpegThread::setHardware(const QString &hardware)
{
    //启用了硬件加速才需要设置,没启用设置无效
#ifdef hardwarespeed
    this->hardware = hardware;
#endif
}

void FFmpegThread::setTransport(const QString &transport)
{
    this->transport = transport;
}

void FFmpegThread::setImageFlag(const FFmpegThread::ImageFlag &imageFlag)
{
    this->imageFlag = imageFlag;
}

void FFmpegThread::setOption(const char *key, const char *value)
{
    //设置参数,可以覆盖
    av_dict_set(&options, key, value, 0);
#if 0
    //可以用如下方法打印设置的参数
    AVDictionaryEntry *entry = NULL;
    entry = av_dict_get(options, "rtsp_transport", NULL, AV_DICT_IGNORE_SUFFIX);
    qDebug() << TIMEMS << fileFlag << "rtsp_transport" << entry->value;
#endif
}

void FFmpegThread::setSaveFile(bool saveFile)
{
    this->saveFile = saveFile;
}

void FFmpegThread::setSaveInterval(int saveInterval)
{
    this->saveInterval = saveInterval;
}

void FFmpegThread::setSavePath(const QString &savePath)
{
    this->savePath = savePath;
}

void FFmpegThread::setFileFlag(const QString &fileFlag)
{
    this->fileFlag = fileFlag;
}

void FFmpegThread::setFileName(const QString &fileName)
{
    this->fileName = fileName;
}

void FFmpegThread::setSaveTime(const QDateTime &saveTime)
{
    this->saveTime = saveTime;
}

void FFmpegThread::play()
{
    //通过标志位让线程执行初始化
    isPlay = true;
    isPause = false;
}

void FFmpegThread::pause()
{
    //只对本地文件起作用
    if (!isRtsp && !isUsbCamera && !isPause) {
        isPause = true;
    }
}

void FFmpegThread::next()
{
    //只对本地文件起作用
    if (!isRtsp && !isUsbCamera && isPause) {
        isPause = false;
        audioSync->reset();
        videoSync->reset();
    }
}

void FFmpegThread::stop()
{
    //通过标志位让线程停止
    stopped = true;
}

void FFmpegThread::snap()
{
    //通过标志位来截图 句柄模式才需要
    if (!callback) {
        isSnap = true;
    }
}
