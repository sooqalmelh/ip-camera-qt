/**
 * @file appconfig.cpp
 * @author creekwater
 * @brief
 *
 * 初始化APP的配置
 *
 * @version 0.1
 * @date 2022-06-05
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "appconfig.h"
#include "quihelper.h"

QString AppConfig::ConfigFile = "config.ini";


// 获取默认配置，初始化appconfig中的videoconfig成员
VideoConfig AppConfig::VideoConfig0 = VideoConfig();
VideoConfig AppConfig::VideoConfig1 = VideoConfig();
VideoConfig AppConfig::VideoConfig2 = VideoConfig();        // 用于安卓
VideoConfig AppConfig::VideoConfig3 = VideoConfig();
VideoConfig AppConfig::VideoConfig4 = VideoConfig();
VideoConfig AppConfig::VideoConfig5 = VideoConfig();
VideoConfig AppConfig::VideoConfig6 = VideoConfig();

// 初始化appconfig中的静态成员
int AppConfig::TabIndex = 0;
int AppConfig::TabIndex1 = 0;
int AppConfig::TabIndex2 = 0;
bool AppConfig::IsMax = false;
bool AppConfig::IsFull = false;
QString AppConfig::VideoType = "1_16";
int AppConfig::BufferWidth = 1280;
int AppConfig::BufferHeight = 720;

// 初始化appconfig中的RTSP地址成员
QString AppConfig::RtspAddr_1 = "rtsp://admin:root123456@192.168.3.173:554";
QString AppConfig::RtspAddr_2 = "rtsp://admin:root123456@192.168.3.173:554";
QString AppConfig::RtspAddr_3 = "rtsp://admin:root123456@192.168.3.173:554";
QString AppConfig::RtspAddr_4 = "rtsp://admin:root123456@192.168.3.173:554";

void AppConfig::readConfig()
{
    //文件不存在默认值
    if (!QFile(ConfigFile).exists()) {
        VideoConfig0.rtspAddr = "f:/mp4/1.mp4";
        VideoConfig1.rtspAddr = "rtsp://admin:root123456@192.168.3.173:554";
        VideoConfig1.fileName = "d:/1.mp4";
        VideoConfig2.rtspAddr = "rtsp://admin:root123456@192.168.3.173:554";
        VideoConfig2.fileName = "d:/2.mp4";
        VideoConfig3.rtspAddr = "rtsp://admin:root123456@192.168.3.173:554";
        VideoConfig3.fileName = "d:/3.mp4";
        VideoConfig4.rtspAddr = "rtsp://admin:root123456@192.168.3.173:554";
        VideoConfig4.fileName = "d:/4.mp4";
        VideoConfig5.fillImage = false;
        VideoConfig6.fillImage = false;
        return;
    }

    QSettings set(ConfigFile, QSettings::IniFormat);

    set.beginGroup("VideoConfig");
    VideoConfig0 = set.value("VideoConfig0").value<VideoConfig>();
    VideoConfig1 = set.value("VideoConfig1").value<VideoConfig>();
    VideoConfig2 = set.value("VideoConfig2").value<VideoConfig>();
    VideoConfig3 = set.value("VideoConfig3").value<VideoConfig>();
    VideoConfig4 = set.value("VideoConfig4").value<VideoConfig>();
    VideoConfig5 = set.value("VideoConfig5").value<VideoConfig>();
    VideoConfig6 = set.value("VideoConfig6").value<VideoConfig>();
    set.endGroup();

    set.beginGroup("BaseConfig");
    TabIndex = set.value("TabIndex", TabIndex).toInt();
    TabIndex1 = set.value("TabIndex1", TabIndex1).toInt();
    TabIndex2 = set.value("TabIndex2", TabIndex2).toInt();
    IsMax = set.value("IsMax", IsMax).toBool();
    IsFull = set.value("IsFull", IsFull).toBool();
    VideoType = set.value("VideoType", VideoType).toString();
    BufferWidth = set.value("BufferWidth", BufferWidth).toInt();
    BufferHeight = set.value("BufferHeight", BufferHeight).toInt();
    set.endGroup();

    set.beginGroup("RtspConfig");
    RtspAddr_1 = set.value("RtspAddr_1", RtspAddr_1).toString();
    RtspAddr_2 = set.value("RtspAddr_2", RtspAddr_2).toString();
    RtspAddr_3 = set.value("RtspAddr_3", RtspAddr_3).toString();
    RtspAddr_4 = set.value("RtspAddr_4", RtspAddr_4).toString();
    set.endGroup();
}

void AppConfig::writeConfig()
{
    QSettings set(ConfigFile, QSettings::IniFormat);

    set.beginGroup("VideoConfig");
    set.setValue("VideoConfig0", QVariant::fromValue(VideoConfig0));
    set.setValue("VideoConfig1", QVariant::fromValue(VideoConfig1));
    set.setValue("VideoConfig2", QVariant::fromValue(VideoConfig2));
    set.setValue("VideoConfig3", QVariant::fromValue(VideoConfig3));
    set.setValue("VideoConfig4", QVariant::fromValue(VideoConfig4));
    set.setValue("VideoConfig5", QVariant::fromValue(VideoConfig5));
    set.setValue("VideoConfig6", QVariant::fromValue(VideoConfig6));
    set.endGroup();

    set.beginGroup("BaseConfig");
    set.setValue("TabIndex", TabIndex);
    set.setValue("TabIndex1", TabIndex1);
    set.setValue("TabIndex2", TabIndex2);
    set.setValue("IsMax", IsMax);
    set.setValue("IsFull", IsFull);
    set.setValue("VideoType", VideoType);
    set.setValue("BufferWidth", BufferWidth);
    set.setValue("BufferHeight", BufferHeight);
    set.endGroup();

    set.beginGroup("RtspConfig");
    set.setValue("RtspAddr_1", RtspAddr_1);
    set.setValue("RtspAddr_2", RtspAddr_2);
    set.setValue("RtspAddr_3", RtspAddr_3);
    set.setValue("RtspAddr_4", RtspAddr_4);
    set.endGroup();
}
