#ifndef FFMPEGTHREAD_H
#define FFMPEGTHREAD_H

//把这几个头文件全部包含下懒得每次增加一个新类又来引入对应头文件
#include <QtGui>
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
#include <QtWidgets>
#endif
#ifdef multimedia
#include <QtMultimedia>
#endif

#include "ffmpeghead.h"

class FFmpegSync;
class QAudioOutput;

class FFmpegThread : public QThread
{
    Q_OBJECT
public:
    struct DecodeContext {
        AVBufferRef *hw_device_ref;
    };

    enum ImageFlag {
        ImageFlag_Fast = 0,     //速度优先
        ImageFlag_Full = 1,     //质量优先
        ImageFlag_Even = 2,     //均衡处理
        ImageFlag_Fast2 = 3     //最快速度
    };

    //解码同步线程定义成友元类,这样可以直接使用主解码线程的变量
    friend class FFmpegSync;
    explicit FFmpegThread(QObject *parent = 0);
    ~FFmpegThread();

protected:
    void run();

private:
    volatile bool stopped;      //线程停止标志位
    volatile bool isPlay;       //播放视频标志位
    volatile bool isPause;      //暂停播放标志位
    volatile bool isSnap;       //是否截图
    volatile bool isRtsp;       //是否是视频流
    volatile bool isUsbCamera;  //是否是USB摄像机
    volatile bool isInit;       //是否初始化成功
    volatile bool isSave;       //是否正在切换保存文件

    QMutex mutex;               //锁对象
    QDateTime lastTime;         //最后的消息时间
    int errorCount;             //错误次数累加
    int frameCount;             //帧数统计
    int frameFinish;            //一帧完成
    int videoWidth;             //视频宽度
    int videoHeight;            //视频高度
    int frameRate;              //视频帧率 一般指USB摄像机
    int rotate;                 //视频旋转角度 手机上拍摄的视频很可能旋转了90度

    int videoIndex;             //视频流索引
    int audioIndex;             //音频流索引
    qint64 videoFirstPts;       //首个包的pts start_time
    qint64 audioFirstPts;       //首个包的pts start_time
    double videoFps;            //视频帧率
    qint64 duration;            //视频或音频时长
    QString formatName;         //文件格式

    int interval;               //采集间隔
    int sleepTime;              //休眠时间
    int readTime;               //读取视频帧超时时间
    int checkTime;              //检测及打开视频超时时间
    bool checkConn;             //检测视频流连接
    bool tryOpen;               //是否已经尝试打开过
    bool tryRead;               //是否已经尝试读取过

    bool multiMode;             //复用模式,从其他地方直接传入视频流
    QString url;                //视频流地址
    bool callback;              //是否回调模式 这里的回调意味着不采用opengl
    QString hardware;           //硬解码解码器类型
    QString transport;          //通信协议 tcp udp
    ImageFlag imageFlag;        //图片缩放类型,速度优先或者质量优先等

    bool saveFile;              //是否保存文件
    int saveInterval;           //保存文件间隔,单位秒钟
    QString savePath;           //保存文件夹
    QString fileFlag;           //定时保存文件唯一标识符
    QString fileName;           //保存文件名称
    QTimer *timerSave;          //定时器隔段时间存储文件
    QDateTime saveTime;         //只存储单个文件的关闭时间
    QFile fileVideo;            //保存视频文件
    QFile fileAudio;            //保存音频文件

    AVPacket *packet;           //待解码临时包
    AVFrame *frameSrc;          //帧对象解码前
    AVFrame *frameDst;          //帧对象解码后
    AVFrame *videoFrame;        //帧对象视频
    AVFrame *audioFrame;        //帧对象音频

    AVFormatContext *formatCtx; //描述一个多媒体文件的构成及其基本信息
    AVCodecContext *videoCtx;   //视频解码器
    AVCodecContext *audioCtx;   //音频解码器
    SwsContext *videoSwsCtx;    //视频转换
    SwrContext *audioSwrCtx;    //音频转换

    uint8_t *videoData;         //解码后的视频数据
    uint8_t *audioData;         //解码后的音频数据
    AVCodec *videoCodec;        //视频解码
    AVCodec *audioCodec;        //音频解码
    AVDictionary *options;      //参数对象
    DecodeContext decode;       //qsv硬解码需要用到的上下文

    bool playRepeat;            //重复循环播放
    bool playAudio;             //播放音频
    bool audioDeviceOk;         //音频设备正常
#if (QT_VERSION >= QT_VERSION_CHECK(6, 2, 0))
    QAudioSink *audioOutput;    //音频播放设备
#else
    QAudioOutput *audioOutput;  //音频播放对象
#endif
    QIODevice *audioDevice;     //音频播放设备
    qint64 startTime;           //解码开始时间

    qint64 packetCount;         //计数保存到文件的包
    bool saveMp4;               //保存成MP4文件而不是裸流
    bool initSaveOk;            //初始化保存成功
    AVFormatContext *formatOut; //输出文件格式上下文

    bool useList;               //启用线程队列处理帧
    FFmpegSync *audioSync;      //音频解码同步线程
    FFmpegSync *videoSync;      //视频解码同步线程

    int volume;                 //记住音量用来切换静音使用
    qint64 position;            //定位播放位置
    QTimer *timerPositon;       //定位播放定时器

private:
    //初始化
    bool init();
    void initOption();
    bool initInput();

    bool initVideo();
    bool initHWDeviceQsv();
    bool initHWDeviceOther();

    bool initAudio();
    void initAudioDevice(int sampleRate, int sampleSize, int channelCount);
    void freeAudioDevice();

    void initMp3();
    void initOther();

    //释放对象
    void free();
    void free(AVFrame *frame);
    void free(AVPacket *packet);

    //保存文件到 mp4 h264 aac
    void saveFileMp4(AVPacket *packet);
    void saveFileH264(AVPacket *packet);
    void saveFileAac(AVPacket *packet);

    //解码视频
    void decodeVideo(AVPacket *packet);
    void decodeVideo1(AVPacket *packet);
    void decodeVideo2(AVPacket *packet);
    void decodeImage();

    //解码音频
    void decodeAudio(AVPacket *packet);
    void decodeAudio1(AVPacket *packet);
    void decodeAudio2(AVPacket *packet);

public slots:
    //启动存储文件
    void refresh_savefile_name();
    void initSave();
    void startSave();
    void stopSave();

    //执行存储文件
    void saveVideo();
    void saveVideoMp4(const QString &fileName);
    void saveVideoH264(const QString &fileName);

    //关闭存储文件
    void closeVideo();
    void closeVideoMp4();
    void closeVideoH264();

    //定时器执行切换播放位置
    void setPosition();
    //设置播放位置
    void setPosition(qint64 position);

public:
    //获取初始化的时间
    qint64 getStartTime();
    //获取打开超时时间
    int getCheckTime();
    //获取读取超时时间
    int getReadTime();
    //获取是否已经尝试打开过
    bool getTryOpen();
    //获取是否已经尝试读取过
    bool getTryRead();
    //获取是否尝试停止线程
    bool getTryStop();

    //获取最后的活动时间
    QDateTime getLastTime();
    //获取url地址
    QString getUrl();
    //获取是否采用回调
    bool getCallback();
    //获取硬件加速选项
    QString getHardware();

    //获取是否是视频流
    bool getIsRtsp();
    //获取是否是本地USB摄像机
    bool getIsUsbCamera();

    //获取视频宽度
    int getVideoWidth();
    //获取视频高度
    int getVideoHeight();

    //是否仅仅只有音频
    bool getOnlyAudio();
    //是否初始化成功
    bool getIsInit();
    //是否播放状态
    bool getIsPlaying();

    //获取长度
    qint64 getLength();
    //获取当前播放位置
    qint64 getPosition();

    //获取播放速度
    float getRate();
    //设置播放速度
    void setRate(float rate);

    //获取静音状态
    bool getMuted();
    //设置静音
    void setMuted(bool muted);

    //获取音量
    int getVolume();
    //设置音量
    void setVolume(int volume);

    //获取视频旋转角度
    int getRotate();

signals:
    //播放成功
    void receivePlayStart();
    //播放失败
    void receivePlayError();
    //播放结束
    void receivePlayFinsh();

    //截图信号
    void snapImage(const QImage &image);
    //收到图片信号
    void receiveImage(const QImage &image);
    //收到一帧信号
    void receiveFrame(AVFrame *frame);

    //总时长
    void fileLengthReceive(qint64 length);
    //当前播放时长
    void filePositionReceive(qint64 position);
    //音量大小
    void fileVolumeReceive(int volume, bool muted);

public slots:
    //设置显示间隔
    void setInterval(int interval);
    //设置帧数,用于播放自身存储的视频流文件,控制播放速度
    void setVideoFps(double videoFps);
    //设置休眠时间
    void setSleepTime(int sleepTime);
    //设置读取视频帧超时时间
    void setReadTime(int readTime);
    //设置检测连接及打开视频超时时间
    void setCheckTime(int checkTime);
    //设置是否检测连接
    void setCheckConn(bool checkConn);

    //设置是否循环播放
    void setPlayRepeat(bool playRepeat);
    //设置是否播放音频
    void setPlayAudio(bool playAudio);
    //设置是否存储MP4文件
    void setSaveMp4(bool saveMp4);

    //设置复用模式
    void setMultiMode(bool multiMode, int videoWidth, int videoHeight);
    //设置视频流地址
    void setUrl(const QString &url);
    //设置是否采用回调
    void setCallback(bool callback);
    //设置硬件解码器名称
    void setHardware(const QString &hardware);
    //设置通信协议
    void setTransport(const QString &transport);
    //设置图片质量类型
    void setImageFlag(const FFmpegThread::ImageFlag &imageFlag);
    //设置参数
    void setOption(const char *key, const char *value);

    //设置是否保存文件
    void setSaveFile(bool saveFile);
    //设置保存间隔
    void setSaveInterval(int saveInterval);
    //设置保存路径
    void setSavePath(const QString &savePath);
    //设置定时保存文件唯一标识符
    void setFileFlag(const QString &fileFlag);
    //设置保存文件名称
    void setFileName(const QString &fileName);
    //设置只存储单个文件的保存时间
    void setSaveTime(const QDateTime &saveTime);

    //开始播放
    void play();
    //暂停播放
    void pause();
    //继续播放
    void next();
    //停止播放
    void stop();
    //截图
    void snap();
};

#endif // FFMPEGTHREAD_H
