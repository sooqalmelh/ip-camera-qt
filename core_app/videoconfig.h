#ifndef VIDEOCONFIG_H
#define VIDEOCONFIG_H

#include <QObject>
#include <QDebug>
#include <QDataStream>

//视频配置参数结构体
struct VideoConfig {
    QString rtspAddr;       //视频地址
    QString fileName;       //文件名称
    bool saveFile;          //保存文件
    bool saveInterval;      //定时保存
    QString hardware;       //解码名称
    QString transport;      //传输协议
    bool fillImage;         //拉伸填充
    bool callback;          //视频回调
    int caching;            //缓存时间
    int imageFlag;          //图片质量
    int checkTime;          //打开超时
    bool playRepeat;        //循环播放

    //默认构造函数
    VideoConfig() {
        rtspAddr = "http://vfx.mtime.cn/Video/2019/02/04/mp4/190204084208765161.mp4";
        fileName = "d:/1.mp4";
        saveFile = false;
        saveInterval = false;
        hardware = "none";
        transport = "tcp";
        fillImage = true;
        callback = false;
        caching = 500;
        imageFlag = 0;
        checkTime = 3000;
        playRepeat = false;
    }

    //重载打印输出格式
    friend QDebug operator << (QDebug debug, const VideoConfig &videoConfig) {
        QStringList list;
        list << QString("视频地址: %1").arg(videoConfig.rtspAddr);
        list << QString("文件名称: %1").arg(videoConfig.fileName);
        list << QString("保存文件: %1").arg(videoConfig.saveFile);
        list << QString("定时保存: %1").arg(videoConfig.saveInterval);
        list << QString("解码名称: %1").arg(videoConfig.hardware);
        list << QString("传输协议: %1").arg(videoConfig.transport);
        list << QString("拉伸填充: %1").arg(videoConfig.fillImage);
        list << QString("视频回调: %1").arg(videoConfig.callback);
        list << QString("缓存时间: %1").arg(videoConfig.caching);
        list << QString("图片质量: %1").arg(videoConfig.imageFlag);
        list << QString("打开超时: %1").arg(videoConfig.checkTime);
        list << QString("循环播放: %1").arg(videoConfig.playRepeat);

#if (QT_VERSION >= QT_VERSION_CHECK(5,4,0))
        debug.noquote() << list.join("\n");
#else
        debug << list.join("\n");
#endif
        return debug;
    }

    //重载数据流输出
    friend QDataStream &operator << (QDataStream &out, const VideoConfig &videoConfig) {
        out << videoConfig.rtspAddr;
        out << videoConfig.fileName;
        out << videoConfig.saveFile;
        out << videoConfig.saveInterval;
        out << videoConfig.hardware;
        out << videoConfig.transport;
        out << videoConfig.fillImage;
        out << videoConfig.callback;
        out << videoConfig.caching;
        out << videoConfig.imageFlag;
        out << videoConfig.checkTime;
        out << videoConfig.playRepeat;
        return out;
    }

    //重载数据流输入
    friend QDataStream &operator >> (QDataStream &in, VideoConfig &videoConfig) {
        in >> videoConfig.rtspAddr;
        in >> videoConfig.fileName;
        in >> videoConfig.saveFile;
        in >> videoConfig.saveInterval;
        in >> videoConfig.hardware;
        in >> videoConfig.transport;
        in >> videoConfig.fillImage;
        in >> videoConfig.callback;
        in >> videoConfig.caching;
        in >> videoConfig.imageFlag;
        in >> videoConfig.checkTime;
        in >> videoConfig.playRepeat;
        return in;
    }
};
//必须加上下面这句用来注册元数据类型,不然报错
//error: static assertion failed: Type is not registered, please use the Q_DECLARE_METATYPE macro to make it known to Qt's meta-object system
Q_DECLARE_METATYPE(VideoConfig)

#endif // VIDEOCONFIG_H
