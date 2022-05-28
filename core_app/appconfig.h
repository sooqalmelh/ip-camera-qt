#ifndef APPCONFIG_H
#define APPCONFIG_H

#include "head.h"

class AppConfig
{
public:
    static QString ConfigFile;      //配置文件文件路径及名称

    //视频配置
    static VideoConfig VideoConfig0;//视频监控界面
    static VideoConfig VideoConfig1;//视频播放控件1
    static VideoConfig VideoConfig2;//视频播放控件2
    static VideoConfig VideoConfig3;//视频播放控件3
    static VideoConfig VideoConfig4;//视频播放控件4
    static VideoConfig VideoConfig5;//视频播放器
    static VideoConfig VideoConfig6;//视频多端复用

    //基础配置
    static int TabIndex;            //页面索引
    static int TabIndex1;           //页面索引1
    static int TabIndex2;           //页面索引2    
    static bool IsMax;              //最大化显示
    static bool IsFull;             //全屏显示
    static QString VideoType;       //画面分割类型
    static int BufferWidth;         //VLC回调模式分辨率宽度
    static int BufferHeight;        //VLC回调模式分辨率高度

    //播放地址
    static QString RtspAddr_1;      //视频流地址1
    static QString RtspAddr_2;      //视频流地址2
    static QString RtspAddr_3;      //视频流地址3
    static QString RtspAddr_4;      //视频流地址4

    static void readConfig();       //读取配置文件,在main函数最开始加载程序载入
    static void writeConfig();      //写入配置文件,在更改配置文件程序关闭时调用
};

#endif // APPCONFIG_H
