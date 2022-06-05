QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat
android {QT += androidextras}

TARGET      = video_ffmpeg
TEMPLATE    = app

SOURCES     += main.cpp
HEADERS     += head.h

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/form
include ($$PWD/form/form.pri)

INCLUDEPATH += $$PWD/../../core_app
include ($$PWD/../../core_app/core_app.pri)

INCLUDEPATH += $$PWD/../../core_common
include ($$PWD/../../core_common/core_common.pri)

INCLUDEPATH += $$PWD/../../core_ffmpeg
include ($$PWD/../../core_ffmpeg/core_ffmpeg.pri)

INCLUDEPATH += $$PWD/../../core_opengl
include ($$PWD/../../core_opengl/core_opengl.pri)

android {
DISTFILES += android/AndroidManifest.xml
DISTFILES += android/build.gradle
DISTFILES += android/res/values/libs.xml
ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}
