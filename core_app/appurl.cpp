#include "appurl.h"
#include "quihelper.h"

QStringList AppUrl::getUrls(QStringList &newurls)
{
    //读取配置文件地址列表
    static QStringList urls;
    //已经读取过一次就不用在读取
    int count = newurls.count();
    if (count == 0 && urls.count() > 0) {
        return urls;
    }

    QFile file(AppPath + "/url.txt");
    if (file.open(QFile::ReadOnly)) {
        while (!file.atEnd()) {
            QString line = file.readLine();
            line = line.trimmed();
            line.replace("\r", "");
            line.replace("\n", "");
            if (!line.isEmpty() && !line.startsWith("#")) {
                QStringList list = line.split(":");
                int ch = list.at(0).toInt();
                if (ch > 0) {
                    QString url = line.mid(line.indexOf(":") + 1, line.length());
                    //urls << url;
                    //替换掉新的url
                    if (count > ch - 1) {
                        newurls[ch - 1] = url;
                    }
                }
            }
        }

        file.close();
    }

#if 0
    //由于视频源不稳定而且很可能打不开暂时屏蔽
    //cctv 格式可以自行增加后缀 cctc1 cctv2 依次下去
    appendUrl(urls, "rtmp://58.200.131.2:1935/livetv/cctv1");
    appendUrl(urls, "rtmp://58.200.131.2:1935/livetv/cctv6");
    appendUrl(urls, "rtmp://58.200.131.2:1935/livetv/hunantv");
    appendUrl(urls, "http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8");
    appendUrl(urls, "http://ivi.bupt.edu.cn/hls/cctv6hd.m3u8");
    appendUrl(urls, "http://112.17.40.140/PLTV/88888888/224/3221226557/index.m3u8");
    appendUrl(urls, "http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear2/prog_index.m3u8");
#endif

#if 0
    //网络电视台地址
    appendUrl(urls, "rtmp://218.3.205.46/live/ggpd_sd");
    appendUrl(urls, "rtmp://hls.hsrtv.cn/hls/hstv1");
    appendUrl(urls, "rtmp://hls.hsrtv.cn/hls/hstv2");
    appendUrl(urls, "rtmp://222.173.22.119:1935/live/jnyd_sd");
    appendUrl(urls, "rtmp://222.173.22.119:1935/live/xwhd_hd");
    appendUrl(urls, "rtmp://livetv.dhtv.cn:1935/live/peoples");
    appendUrl(urls, "rtmp://livetv.dhtv.cn:1935/live/citylife");
    appendUrl(urls, "rtmp://livetv.dhtv.cn:1935/live/financial");
    appendUrl(urls, "rtmp://livetv.dhtv.cn:1935/live/news");
#endif

#if 0
    appendUrl(urls, "https://hls01open.ys7.com/openlive/6e0b2be040a943489ef0b9bb344b96b8.hd.m3u8");
    appendUrl(urls, "http://vts.simba-cn.com:280/gb28181/21100000001320000002.m3u8");
    //https://blog.csdn.net/qq_28073073/article/details/103399279
    appendUrl(urls, "http://vfx.mtime.cn/Video/2019/02/04/mp4/190204084208765161.mp4");
    appendUrl(urls, "http://vfx.mtime.cn/Video/2019/03/18/mp4/190318231014076505.mp4");
    appendUrl(urls, "http://vfx.mtime.cn/Video/2019/03/19/mp4/190319212559089721.mp4");
    appendUrl(urls, "http://vfx.mtime.cn/Video/2019/03/17/mp4/190317150237409904.mp4");
    appendUrl(urls, "http://vfx.mtime.cn/Video/2019/03/14/mp4/190314223540373995.mp4");

    appendUrl(urls, "rtsp://admin:Admin123456@192.168.0.15:554/media/video1");
    appendUrl(urls, "rtsp://admin:Admin123456@192.168.0.15:554/media/video2");
    appendUrl(urls, "rtsp://admin:Admin123456@192.168.0.64:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_2");
    appendUrl(urls, "rtsp://admin:Admin123456@192.168.0.64:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_2");
    appendUrl(urls, "rtsp://admin:Admin123456@192.168.0.107:554/cam/realmonitor?channel=1&subtype=0&unicast=true&proto=Onvif");
    appendUrl(urls, "rtsp://admin:Admin123456@192.168.0.107:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif");
    appendUrl(urls, "rtsp://admin:Admin123456@192.168.0.160:554/stream0?username=admin&password=E10ADC3949BA59ABBE56E057F20F883E");
    appendUrl(urls, "rtsp://admin:Admin123456@192.168.0.160:554/stream1?username=admin&password=E10ADC3949BA59ABBE56E057F20F883E");

    appendUrl(urls, "rtmp://livetv.dhtv.cn:1935/live/peoples");
    appendUrl(urls, "rtmp://livetv.dhtv.cn:1935/live/financial");
    appendUrl(urls, "http://192.168.0.101:8083");

    //格式 搜索到的 https://music.163.com/#/song?id=179768
    appendUrl(urls, "http://music.163.com/song/media/outer/url?id=179768.mp3");
    appendUrl(urls, "http://music.163.com/song/media/outer/url?id=281951.mp3");
    appendUrl(urls, "http://music.163.com/song/media/outer/url?id=447925558.mp3");
#endif

    appendUrl(urls, "rtsp://admin:root123456@192.168.3.173:554");


    //后面的都是win系统上的路径和格式 其他系统无意义
#ifndef Q_OS_WIN
    appendUrl(urls, "/dev/video0");
    appendUrl(urls, "/dev/video1");
    return urls;
#endif

    appendUrl(urls, "f:/mp3/1.mp3");
    appendUrl(urls, "f:/mp3/1.wav");
    appendUrl(urls, "f:/mp3/1.wma");
    appendUrl(urls, "f:/mp3/1.mid");

    appendUrl(urls, "f:/mp4/1.mp4");
    appendUrl(urls, "f:/mp4/1000.mkv");
    appendUrl(urls, "f:/mp4/1001.wmv");
    appendUrl(urls, "f:/mp4/1002.mov");
    appendUrl(urls, "f:/mp4/1003.mp4");
    appendUrl(urls, "f:/mp4/1080.mp4");

    appendUrl(urls, "f:/mp5/1.avi");
    appendUrl(urls, "f:/mp5/1.ts");
    appendUrl(urls, "f:/mp5/1.asf");
    appendUrl(urls, "f:/mp5/1.mp4");
    appendUrl(urls, "f:/mp5/5.mp4");
    appendUrl(urls, "f:/mp5/1.rmvb");
    appendUrl(urls, "f:/mp5/4k-001.mp4");
    appendUrl(urls, "f:/mp5/h264.aac");
    appendUrl(urls, "f:/mp5/h264.mp4");
    appendUrl(urls, "f:/mp5/haikang.mp4");
    appendUrl(urls, "f:/mp4/新建文件夹/1.mp4");

    appendUrl(urls, "dshow://:dshow-vdev=USB2.0 PC CAMERA");
    appendUrl(urls, "dshow://:dshow-vdev=USB Video Device:dshow-size=1280*720");
    appendUrl(urls, "video=LIHAPPE8-316A");
    appendUrl(urls, "video=USB2.0 PC CAMERA");
    appendUrl(urls, "video=Logitech HD Webcam C270");
    appendUrl(urls, "video=USB Video Device|1280x720|30");
    return urls;
}

void AppUrl::appendUrl(QStringList &urls, const QString &url)
{
    if (!urls.contains(url)) {
        urls << url;
    }
}

