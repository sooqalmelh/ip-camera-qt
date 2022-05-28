greaterThan(QT_MAJOR_VERSION, 4) {
qtHaveModule(multimedia) {
DEFINES += multimedia
QT += multimedia
}}

HEADERS += $$PWD/ffmpeghead.h
HEADERS += $$PWD/ffmpeghelper.h
HEADERS += $$PWD/ffmpegconvert.h

HEADERS += $$PWD/ffmpegthread.h
HEADERS += $$PWD/ffmpegsync.h
HEADERS += $$PWD/ffmpegwidget.h
HEADERS += $$PWD/ffmpegtool.h
HEADERS += $$PWD/videoffmpeg.h

SOURCES += $$PWD/ffmpegthread.cpp
SOURCES += $$PWD/ffmpegsync.cpp
SOURCES += $$PWD/ffmpegwidget.cpp
SOURCES += $$PWD/ffmpegtool.cpp
SOURCES += $$PWD/videoffmpeg.cpp

#如果用的是ffmpeg4内核请将ffmpeg3改成ffmpeg4,两种内核不兼容,头文件也不一样
#如果要支持xp则必须使用ffmpeg3 ffmpeg4开始放弃了xp的支持
!contains(DEFINES, ffmpeg3) {
!contains(DEFINES, ffmpeg4) {
DEFINES += ffmpeg4
}}

win32 {
#开启USB设备查找 一般是在win上
DEFINES += ffmpegdevice
#硬件加速 x86 linux系统(需要真机环境,虚拟机环境不支持)也支持硬件加速,可以自行打开
DEFINES += hardwarespeed
}

#ffmpeg4则使用ffmpeg4的目录
contains(DEFINES, ffmpeg4) {
strPath = ffmpeg4
} else {
strPath = ffmpeg3
}

#表示64位的构建套件
contains(QT_ARCH, x86_64) {
strLib = winlib64
strInclude = include64
} else {
#由于Qt4不支持QT_ARCH所以会执行下面的
#如果用的64位的Qt4则需要自行修改
strLib = winlib
strInclude = include
}

#加入头文件文件夹
!android {
INCLUDEPATH += $$PWD/$$strPath/$$strInclude
}

#window系统
win32 {
LIBS += -L$$PWD/$$strPath/$$strLib/ -lavformat -lavcodec -lavfilter -lswscale -lavutil -lswresample -lavdevice
}

#linux系统
#在某些环境下可能如果报错的话可能还需要主动链接一些库 -lm -lz -lrt -ld -llzma -lvdpau -lX11 -lx264 等
#具体报错提示自行搜索可以找到答案,增加需要链接的库即可
#-lavformat -lswscale -lswresample -lavdevice -lavfilter -lavcodec -lavutil
!android {
unix:!macx {
contains(DEFINES, rk3399) {
LIBS += -L$$PWD/rk3399lib/ -lavformat -lavcodec -lavfilter -lswscale -lavutil -lswresample -lavdevice
} else {
contains(DEFINES, gcc47) {
LIBS += -L$$PWD/armlib/ -lavformat -lavcodec -lavfilter -lswscale -lavutil -lswresample -lavdevice
} else {
contains(DEFINES, gcc45) {
LIBS += -L$$PWD/armlib4.5/ -lavformat -lavcodec -lavfilter -lswscale -lavutil -lswresample -lavdevice -lpostproc
} else {
#如果是UOS系统可以使用自带的ffmpeg 很多linux系统如果安装有ffmpeg则可以用系统自带的
#可以用 find / -name ffmpeg 找到位置 然后用 ldd ffmpeg 查看依赖库就知道在哪里
#LIBS += -L/lib/x86_64-linux-gnu/
#如果是系统路径可以自动找到的可以直接
#LIBS += -L/ -lavformat -lavcodec -lavfilter -lswscale -lavutil -lswresample -lavdevice
LIBS += -L$$PWD/linuxlib/ -lavformat -lavcodec -lavfilter -lswscale -lavutil -lswresample -lavdevice -lpthread -lm -lz -lrt -ldl
}}}}}

#mac系统
macx {
LIBS += -L$$PWD/maclib/ -lavformat.58 -lavcodec.58 -lavfilter.7 -lswscale.5 -lavutil.56 -lswresample.3 -lavdevice.58
}

#android系统
android {
INCLUDEPATH += $$PWD/androidlib/include
LIBS += -L$$PWD/androidlib/ -lavcodec -lavfilter -lavformat -lswscale -lavutil -lswresample
#将动态库文件一起打包
ANDROID_EXTRA_LIBS += $$PWD/androidlib/libavcodec.so
ANDROID_EXTRA_LIBS += $$PWD/androidlib/libavfilter.so
ANDROID_EXTRA_LIBS += $$PWD/androidlib/libavformat.so
ANDROID_EXTRA_LIBS += $$PWD/androidlib/libavutil.so
ANDROID_EXTRA_LIBS += $$PWD/androidlib/libswresample.so
ANDROID_EXTRA_LIBS += $$PWD/androidlib/libswscale.so
}
