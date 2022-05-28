#include "quihelper.h"
#include "frmtab.h"

//动态设置权限
bool checkPermission(const QString &permission)
{
#ifdef Q_OS_ANDROID
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
    QtAndroid::PermissionResult result = QtAndroid::checkPermission(permission);
    if (result == QtAndroid::PermissionResult::Denied) {
        QtAndroid::requestPermissionsSync(QStringList() << permission);
        result = QtAndroid::checkPermission(permission);
        if (result == QtAndroid::PermissionResult::Denied) {
            return false;
        }
    }
#endif
#endif
    return true;
}

void initStyle()
{
    //复选框单选框滑块等指示器大小
    QStringList list;
    int rbtnWidth = 20;
    int ckWidth = 18;
    list.append(QString("QRadioButton::indicator{width:%1px;height:%1px;}").arg(rbtnWidth));
    list.append(QString("QCheckBox::indicator,QGroupBox::indicator,QTreeWidget::indicator,QListWidget::indicator{width:%1px;height:%1px;}").arg(ckWidth));

    QString normalColor = "#E8EDF2";
    QString grooveColor = "#1ABC9C";
    QString handleColor = "#1ABC9C";
    int sliderHeight = 12;
    int sliderRadius = sliderHeight / 2;
    int handleWidth = (sliderHeight * 3) / 2 + (sliderHeight / 5);
    int handleRadius = handleWidth / 2;
    int handleOffset = handleRadius / 2;

    list << QString("QSlider::horizontal{min-height:%1px;}").arg(sliderHeight * 2);
    list << QString("QSlider::groove:horizontal{background:%1;height:%2px;border-radius:%3px;}")
         .arg(normalColor).arg(sliderHeight).arg(sliderRadius);
    list << QString("QSlider::add-page:horizontal{background:%1;height:%2px;border-radius:%3px;}")
         .arg(normalColor).arg(sliderHeight).arg(sliderRadius);
    list << QString("QSlider::sub-page:horizontal{background:%1;height:%2px;border-radius:%3px;}")
         .arg(grooveColor).arg(sliderHeight).arg(sliderRadius);
    list << QString("QSlider::handle:horizontal{width:%2px;margin-top:-%3px;margin-bottom:-%3px;border-radius:%4px;"
                    "background:qradialgradient(spread:pad,cx:0.5,cy:0.5,radius:0.5,fx:0.5,fy:0.5,stop:0.6 #FFFFFF,stop:0.8 %1);}")
         .arg(handleColor).arg(handleWidth).arg(handleOffset).arg(handleRadius);

    //偏移一个像素
    handleWidth = handleWidth + 1;
    list << QString("QSlider::vertical{min-width:%1px;}").arg(sliderHeight * 2);
    list << QString("QSlider::groove:vertical{background:%1;width:%2px;border-radius:%3px;}")
         .arg(normalColor).arg(sliderHeight).arg(sliderRadius);
    list << QString("QSlider::add-page:vertical{background:%1;width:%2px;border-radius:%3px;}")
         .arg(grooveColor).arg(sliderHeight).arg(sliderRadius);
    list << QString("QSlider::sub-page:vertical{background:%1;width:%2px;border-radius:%3px;}")
         .arg(normalColor).arg(sliderHeight).arg(sliderRadius);
    list << QString("QSlider::handle:vertical{height:%2px;margin-left:-%3px;margin-right:-%3px;border-radius:%4px;"
                    "background:qradialgradient(spread:pad,cx:0.5,cy:0.5,radius:0.5,fx:0.5,fy:0.5,stop:0.6 #FFFFFF,stop:0.8 %1);}")
         .arg(handleColor).arg(handleWidth).arg(handleOffset).arg(handleRadius);

    qApp->setStyleSheet(list.join(""));
}

void test()
{
    //打印各种信息
#if 0
    qDebug() << TIMEMS << "GL_VERSION" << GL_VERSION;
    qDebug() << TIMEMS << "QSurfaceFormat version old" << QSurfaceFormat::defaultFormat().version();
    QSurfaceFormat format;
    format.setVersion(2, 0);
    //format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    qDebug() << TIMEMS << "QSurfaceFormat version new" << QSurfaceFormat::defaultFormat().version();
#endif
}

int main(int argc, char *argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    QApplication::setAttribute(Qt::AA_Use96Dpi);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5,4,0))
    //设置opengl模式 AA_UseDesktopOpenGL(默认) AA_UseOpenGLES AA_UseSoftwareOpenGL
    //在一些很旧的设备上或者对opengl支持很低的设备上需要使用AA_UseOpenGLES表示禁用硬件加速
    //如果开启的是AA_UseOpenGLES则无法使用硬件加速比如ffmpeg的dxva2
    //QApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif

    QApplication a(argc, argv);
    QUIHelper::initAll(false);

    //注册自定义数据类型,不注册可能提示 QVariant::load: unknown user type with name
    qRegisterMetaType<VideoConfig>("VideoConfig");
    //不注册执行对应类型,输入输出流提示 QVariant::save: unable to save type
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
    qRegisterMetaTypeStreamOperators<VideoConfig>("VideoConfig");
#endif

#ifdef Q_OS_ANDROID
    //为了兼容安卓方便直接修改配置文件特意将拓展名改成了 txt
    AppConfig::ConfigFile = AppPath + "/video_ffmpeg.txt";
#else
    AppConfig::ConfigFile = AppPath + "/video_ffmpeg.ini";
#endif
    AppConfig::readConfig();
    AppData::checkRatio();

    frmTab w;
#ifdef Q_OS_ANDROID
    //请求权限
    checkPermission("android.permission.READ_EXTERNAL_STORAGE");
    checkPermission("android.permission.WRITE_EXTERNAL_STORAGE");

    QString strDir = AppPath + "/snap";
    QDir dir(strDir);
    if (!dir.exists()) {
        dir.mkpath(strDir);
    }

    initStyle();
    w.showMaximized();
#else
    w.resize(AppData::FormWidth, AppData::FormHeight);
    w.setWindowTitle(QString("wifi-camera (version:%1)          %2").arg(AppData::Version).arg(AppData::TitleFlag));
    QUIHelper::showForm(&w);
#endif

    return a.exec();
}
