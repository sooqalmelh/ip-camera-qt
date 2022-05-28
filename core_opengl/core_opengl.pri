#不需要opengl的话屏蔽下面这行即可
DEFINES += opengl

#判断当前qt版本号
message($$QT_ARCH : $$QT_VERSION -> $$QT_MAJOR_VERSION . $$QT_MINOR_VERSION)

#Qt4.8以上才支持 QGLFunctions 下面的含义是如果版本 < 4.8
lessThan(QT_MAJOR_VERSION, 5) {
lessThan(QT_MINOR_VERSION, 8) {
DEFINES -= opengl
message($$QT_VERSION not support QGLFunctions)
}}

#Qt5.4及以上版本才有但是要5.5及以上版本才稳定 下面的含义是如果版本 >= 5.5
greaterThan(QT_MAJOR_VERSION, 4) {
greaterThan(QT_MINOR_VERSION, 4) {
DEFINES += openglnew
}}

#Qt6以上肯定支持 单独提取出来了 openglwidgets 模块
greaterThan(QT_MAJOR_VERSION, 5) {
DEFINES += openglnew
QT += openglwidgets
}

#2020-12-12 暂时限定存在qopenglwidget模块才能启用
#因为qglwidget如何绘制实时视频没有搞定需要换成回调模式
!contains(DEFINES, openglnew) {
DEFINES -= opengl
message($$QT_VERSION not support QOpenGLWidget)
}

#表示arm平台构建套件
contains(QT_ARCH, arm) || contains(QT_ARCH, arm64) {
#arm上一般没有opengl或者支持度不友好或者没有qopenglwidget
#如果都有可以自行屏蔽下面这行
DEFINES -= opengl openglnew
}

HEADERS += $$PWD/openglhead.h

contains(DEFINES, opengl) {
contains(DEFINES, openglnew) {
HEADERS += $$PWD/nv12openglwidget.h
HEADERS += $$PWD/yuvopenglwidget.h

SOURCES += $$PWD/nv12openglwidget.cpp
SOURCES += $$PWD/yuvopenglwidget.cpp
} else {
#旧版本的opengl采用编译时链接 新版本的采用动态运行链接
QT += opengl
HEADERS += $$PWD/nv12glwidget.h
HEADERS += $$PWD/yuvglwidget.h

SOURCES += $$PWD/nv12glwidget.cpp
SOURCES += $$PWD/yuvglwidget.cpp
}}
