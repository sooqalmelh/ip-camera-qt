/**
 * @file ffmpegwidget.cpp
 * @author creekwater
 * @brief
 *
 * FFMPEG窗体
 *
 * @version 0.1
 * @date 2022-06-05
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "ffmpegwidget.h"

#ifdef opengl
#ifdef openglnew
#include "yuvopenglwidget.h"
#include "nv12openglwidget.h"
#else
#include "yuvglwidget.h"
#include "nv12glwidget.h"
#endif
#endif

FFmpegWidget::FFmpegWidget(QWidget *parent) : QWidget(parent)
{
    //设置强焦点
    setFocusPolicy(Qt::StrongFocus);
    //设置支持拖放
    setAcceptDrops(true);

    //定时器校验视频
    timerCheck = new QTimer(this);
    timerCheck->setInterval(10 * 1000);
    connect(timerCheck, SIGNAL(timeout()), this, SLOT(checkVideo()));

    //定时器绘制OSD时间
    timerOSD = new QTimer(this);
    timerOSD->setInterval(1000);
    connect(timerOSD, SIGNAL(timeout()), this, SLOT(update()));

    image = QImage();
    copyImage = false;
    checkLive = false;
    drawImage = true;
    fillImage = true;

    flowEnable = false;
    flowBgColor = "#000000";
    flowPressColor = "#5EC7D9";

    timeout = 20;
    borderWidth = 5;
    borderColor = "#000000";
    focusColor = "#22A3A9";
    bgColor = Qt::transparent;
    bgText = "实时视频";
    bgImage = QImage();

    osd1Visible = false;
    osd1FontSize = 12;
    osd1Text = "通道名称";
    osd1Color = "#FF0000";
    osd1Image = QImage();
    osd1Format = OSDFormat_DateTime;
    osd1Position = OSDPosition_Right_Top;

    osd2Visible = false;
    osd2FontSize = 12;
    osd2Text = "通道名称";
    osd2Color = "#FF0000";
    osd2Image = QImage();
    osd2Format = OSDFormat_Text;
    osd2Position = OSDPosition_Left_Bottom;

    faceBorder = 3;
    faceColor = QColor(255, 0, 0);
    faceRects.clear();

    //初始化解码线程
    this->initThread();
    //初始化悬浮条
    this->initFlowPanel();
    //初始化悬浮条样式
    this->initFlowStyle();
}

void FFmpegWidget::initThread()
{
    //拖放的时候不用进行一些隐藏处理
    isDrag = false;

    thread = new FFmpegThread(this);
    connect(thread, SIGNAL(receivePlayStart()), this, SIGNAL(receivePlayStart()));
    connect(thread, SIGNAL(receivePlayError()), this, SIGNAL(receivePlayError()));
    connect(thread, SIGNAL(receivePlayFinsh()), this, SIGNAL(receivePlayFinsh()));
    connect(thread, SIGNAL(receivePlayStart()), this, SLOT(playStart()));
    connect(thread, SIGNAL(receivePlayFinsh()), this, SLOT(playFinsh()));

    connect(thread, SIGNAL(snapImage(QImage)), this, SLOT(snapImage(QImage)));
    connect(thread, SIGNAL(receiveImage(QImage)), this, SIGNAL(receiveImage(QImage)));
    connect(thread, SIGNAL(receiveImage(QImage)), this, SLOT(updateImage(QImage)));

    connect(thread, SIGNAL(fileLengthReceive(qint64)), this, SIGNAL(fileLengthReceive(qint64)));
    connect(thread, SIGNAL(filePositionReceive(qint64)), this, SIGNAL(filePositionReceive(qint64)));
    connect(thread, SIGNAL(fileVolumeReceive(int, bool)), this, SIGNAL(fileVolumeReceive(int, bool)));

    //选择是否是opengl绘制还是painter绘制
#ifdef opengl
    yuvWidget = new YUVWidget(this);
    nv12Widget = new NV12Widget(this);
    yuvWidget->hide();
    nv12Widget->hide();
    connect(thread, SIGNAL(receiveFrame(AVFrame *)), this, SIGNAL(receiveFrame(AVFrame *)));
    connect(thread, SIGNAL(receiveFrame(AVFrame *)), this, SLOT(updateFrame(AVFrame *)));
#endif

    osdWidget = new QWidget(this);
    osdWidget->installEventFilter(this);
}

void FFmpegWidget::initFlowPanel()
{
    //顶部工具栏,默认隐藏,鼠标移入显示移除隐藏
    flowPanel = new QWidget(this);
    flowPanel->setObjectName("flowPanel");
    flowPanel->setVisible(false);

    //用布局顶住,左侧弹簧
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setSpacing(2);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();
    flowPanel->setLayout(layout);

    //按钮集合名称,如果需要新增按钮则在这里增加即可
    QList<QString> btns;
    btns << "btnFlowVideo" << "btnFlowSnap" << "btnFlowSound" << "btnFlowAlarm" << "btnFlowClose";

    //有多种办法来设置图片,qt内置的图标+自定义的图标+图形字体
    //既可以设置图标形式,也可以直接图形字体设置文本
#if 0
    QList<QIcon> icons;
    icons << QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    icons << QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    icons << QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    icons << QApplication::style()->standardIcon(QStyle::SP_DialogOkButton);
    icons << QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton);
#else
    QList<int> icons;
    icons << 0xe68d << 0xe672 << 0xe674 << 0xea36 << 0xe74c;

    //判断图形字体是否存在,不存在则加入
    QFont iconFont;
    QFontDatabase fontDb;
    if (!fontDb.families().contains("iconfont")) {
        int fontId = fontDb.addApplicationFont(":/font/iconfont.ttf");
        QStringList fontName = fontDb.applicationFontFamilies(fontId);
        if (fontName.count() == 0) {
            //qDebug() << "load iconfont.ttf error";
        }
    }

    if (fontDb.families().contains("iconfont")) {
        iconFont = QFont("iconfont");
        iconFont.setPixelSize(17);
#if (QT_VERSION >= QT_VERSION_CHECK(4,8,0))
        iconFont.setHintingPreference(QFont::PreferNoHinting);
#endif
    }
#endif

    //循环添加顶部按钮
    for (int i = 0; i < btns.count(); i++) {
        QPushButton *btn = new QPushButton;
        //绑定按钮单击事件,用来发出信号通知
        connect(btn, SIGNAL(clicked(bool)), this, SLOT(btnClicked()));
        //设置标识,用来区别按钮
        btn->setObjectName(btns.at(i));
        //设置固定宽度
        btn->setFixedWidth(20);
        //设置拉伸策略使得填充
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        //设置焦点策略为无焦点,避免单击后焦点跑到按钮上
        btn->setFocusPolicy(Qt::NoFocus);

#if 0
        //设置图标大小和图标
        btn->setIconSize(QSize(16, 16));
        btn->setIcon(icons.at(i));
#else
        btn->setFont(iconFont);
        btn->setText((QChar)icons.at(i));
#endif

        //将按钮加到布局中
        layout->addWidget(btn);
    }
}

void FFmpegWidget::initFlowStyle()
{
    //设置样式以便区分,可以自行更改样式,也可以不用样式
    QStringList qss;
    QString rgba = QString("rgba(%1,%2,%3,150)").arg(flowBgColor.red()).arg(flowBgColor.green()).arg(flowBgColor.blue());
    qss.append(QString("#flowPanel{background:%1;border:none;}").arg(rgba));
    qss.append(QString("QPushButton{border:none;padding:0px;background:rgba(0,0,0,0);}"));
    qss.append(QString("QPushButton:pressed{color:%1;}").arg(flowPressColor.name()));
    flowPanel->setStyleSheet(qss.join(""));
}

FFmpegWidget::~FFmpegWidget()
{
    if (timerCheck->isActive()) {
        timerCheck->stop();
    }
    if (timerOSD->isActive()) {
        timerOSD->stop();
    }

    close();
}

void FFmpegWidget::resizeEvent(QResizeEvent *)
{
    //重新设置顶部工具栏的位置和宽高,可以自行设置顶部显示或者底部显示
    int height = 20;
    int offset = borderWidth;
    flowPanel->setGeometry(offset, offset, this->width() - (offset * 2), height);
    //flowPanel->setGeometry(offset, this->height() - height - offset, this->width() - (offset * 2), height);

#ifdef opengl
    offset = borderWidth * 1 + 0;
    QRect rect(offset / 2, offset / 2, this->width() - offset, this->height() - offset);
    //如果是等比例则按照图片尺寸自动调整
    if (!fillImage) {
        //通过假定一个图片来计算尺寸
        QImage img(thread->getVideoWidth(), thread->getVideoHeight(), QImage::Format_RGB888);
        img = img.scaled(this->width() - offset, this->height() - offset, Qt::KeepAspectRatio);
        int x = this->rect().center().x() - img.width() / 2;
        int y = this->rect().center().y() - img.height() / 2;
        rect = QRect(x, y, img.width(), img.height());
    }

    //先要清空再设置尺寸否则可能会遇到崩溃
    yuvWidget->clear();
    yuvWidget->setGeometry(rect);
    nv12Widget->clear();
    nv12Widget->setGeometry(rect);
#endif

    osdWidget->setGeometry(this->rect());
}

#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
void FFmpegWidget::enterEvent(QEnterEvent *)
#else
void FFmpegWidget::enterEvent(QEvent *)
#endif
{
    //这里还可以增加一个判断,是否获取了焦点的才需要显示
    //if (this->hasFocus()) {}
    if (flowEnable) {
        flowPanel->setVisible(true);
    }
}

void FFmpegWidget::leaveEvent(QEvent *)
{
    if (flowEnable) {
        flowPanel->setVisible(false);
    }
}

void FFmpegWidget::dropEvent(QDropEvent *event)
{
    //拖放完毕鼠标松开的时候执行
    //判断拖放进来的类型,取出文件,进行播放
    isDrag = true;
    QString url;
    QTimer::singleShot(1000, this, SLOT(clearDrag()));
    if (event->mimeData()->hasUrls()) {
        url = event->mimeData()->urls().first().toLocalFile();
    } else if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QTreeWidget *treeWidget = (QTreeWidget *)event->source();
        if (treeWidget != 0) {
            //过滤父节点 那个一般是NVR
            QTreeWidgetItem *item = treeWidget->currentItem();
            if (item->parent() != 0) {
                url = item->data(0, Qt::UserRole).toString();
            }
        }
    }

    if (!url.isEmpty()) {
        this->restart(url);
    }
}

void FFmpegWidget::dragEnterEvent(QDragEnterEvent *event)
{
    //拖曳进来的时候先判断下类型,非法类型则不处理
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else if (event->mimeData()->hasFormat("text/uri-list")) {
        event->setDropAction(Qt::LinkAction);
        event->accept();
    } else {
        event->ignore();
    }
}

bool FFmpegWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == osdWidget && event->type() == QEvent::Paint) {
        QPainter painter;
        painter.begin(osdWidget);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        //绘制边框
        drawBorder(&painter);
        if (thread->getIsInit()) {
            if (drawImage) {
                //绘制背景图片
                drawImg(&painter, image);
                //绘制人脸框
                drawFace(&painter);
                //绘制标签
                drawOSD(&painter, osd1Visible, osd1FontSize, osd1Text, osd1Color, osd1Image, osd1Format, osd1Position);
                drawOSD(&painter, osd2Visible, osd2FontSize, osd2Text, osd2Color, osd2Image, osd2Format, osd2Position);
            }
        } else {
            //绘制背景
            if (!isDrag) {
                drawBg(&painter);
            }
        }

        painter.end();
    }

    return QWidget::eventFilter(watched, event);
}

void FFmpegWidget::drawBorder(QPainter *painter)
{
    if (borderWidth == 0) {
        return;
    }

    painter->save();
    QPen pen;
    pen.setWidth(borderWidth);
    pen.setColor(hasFocus() ? focusColor : borderColor);
    painter->setPen(pen);
    painter->drawRect(rect());
    painter->restore();
}

void FFmpegWidget::drawBg(QPainter *painter)
{
    painter->save();
    if (bgColor != Qt::transparent) {
        painter->fillRect(rect(), bgColor);
    }

    //背景图片为空则绘制文字,否则绘制背景图片
    if (bgImage.isNull()) {
        painter->setFont(this->font());
        painter->setPen(palette().windowText().color());
        painter->drawText(rect(), Qt::AlignCenter, bgText);
    } else {
        //居中绘制
        int x = rect().center().x() - bgImage.width() / 2;
        int y = rect().center().y() - bgImage.height() / 2;
        QPoint point(x, y);
        painter->drawImage(point, bgImage);
    }

    painter->restore();
}

void FFmpegWidget::drawImg(QPainter *painter, QImage img)
{
    if (img.isNull()) {
        return;
    }

    painter->save();

    int offset = borderWidth * 1 + 0;
    img = img.scaled(width() - offset, height() - offset, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (fillImage) {
        QRect rect(offset / 2, offset / 2, width() - offset, height() - offset);
        painter->drawImage(rect, img);
    } else {
        //按照比例自动居中绘制
        int x = rect().center().x() - img.width() / 2;
        int y = rect().center().y() - img.height() / 2;
        QPoint point(x, y);
        painter->drawImage(point, img);
    }

    painter->restore();
}

void FFmpegWidget::drawFace(QPainter *painter)
{
    if (faceRects.count() == 0) {
        return;
    }

    painter->save();

    //人脸边框的颜色
    QPen pen;
    pen.setWidth(faceBorder);
    pen.setColor(faceColor);
    painter->setPen(pen);

    //逐个取出人脸框区域进行绘制
    foreach (QRect rect, faceRects) {
        painter->drawRect(rect);
    }

#if 1
    //绘制多边形
    QPainterPath path;
    //先移动到起始点
    path.moveTo(QPoint(100, 100));
    //逐个连接线条
    path.lineTo(QPoint(150, 50));
    path.lineTo(QPoint(200, 150));
    //下面两行二选一既可以选择连接到起始点也可以调用closeSubpath自动闭合
    //path.lineTo(QPoint(100, 100));
    path.closeSubpath();
    painter->drawPath(path);
#endif

    painter->restore();
}

void FFmpegWidget::drawOSD(QPainter *painter,
                           bool osdVisible,
                           int osdFontSize,
                           const QString &osdText,
                           const QColor &osdColor,
                           const QImage &osdImage,
                           const FFmpegWidget::OSDFormat &osdFormat,
                           const FFmpegWidget::OSDPosition &osdPosition)
{
    if (!osdVisible) {
        return;
    }

    painter->save();

    //标签位置尽量偏移多一点避免遮挡
    QRect osdRect(rect().x() + (borderWidth * 2), rect().y() + (borderWidth * 2), width() - (borderWidth * 5), height() - (borderWidth * 5));
    int flag = Qt::AlignLeft | Qt::AlignTop;
    QPoint point = QPoint(osdRect.x(), osdRect.y());

    if (osdPosition == OSDPosition_Left_Top) {
        flag = Qt::AlignLeft | Qt::AlignTop;
        point = QPoint(osdRect.x(), osdRect.y());
    } else if (osdPosition == OSDPosition_Left_Bottom) {
        flag = Qt::AlignLeft | Qt::AlignBottom;
        point = QPoint(osdRect.x(), osdRect.height() - osdImage.height());
    } else if (osdPosition == OSDPosition_Right_Top) {
        flag = Qt::AlignRight | Qt::AlignTop;
        point = QPoint(osdRect.width() - osdImage.width(), osdRect.y());
    } else if (osdPosition == OSDPosition_Right_Bottom) {
        flag = Qt::AlignRight | Qt::AlignBottom;
        point = QPoint(osdRect.width() - osdImage.width(), osdRect.height() - osdImage.height());
    }

    if (osdFormat == OSDFormat_Image) {
        painter->drawImage(point, osdImage);
    } else {
        QDateTime now = QDateTime::currentDateTime();
        QString text = osdText;
        if (osdFormat == OSDFormat_Date) {
            text = now.toString("yyyy-MM-dd");
        } else if (osdFormat == OSDFormat_Time) {
            text = now.toString("HH:mm:ss");
        } else if (osdFormat == OSDFormat_DateTime) {
            text = now.toString("yyyy-MM-dd HH:mm:ss");
        }

        //设置颜色及字号
        QFont font;
        font.setPixelSize(osdFontSize);
        painter->setPen(osdColor);
        painter->setFont(font);

        painter->drawText(osdRect, flag, text);
    }

    painter->restore();
}

QImage FFmpegWidget::getImage() const
{
#ifdef opengl
    thread->snap();
#endif
    return this->image;
}

QPixmap FFmpegWidget::getPixmap() const
{
    //采用截屏的形式截图,这种方式可以将 设置的OSD等信息都截图进去
#if (QT_VERSION <= QT_VERSION_CHECK(5,0,0))
    QPixmap pixmap = QPixmap::grabWindow(winId(), 0, 0, width(), height());
#else
    QScreen *screen = QApplication::primaryScreen();
    QPixmap pixmap = screen->grabWindow(winId(), 0, 0, width(), height());
#endif
    return pixmap;
}

QString FFmpegWidget::getUrl() const
{
    return thread->getUrl();
}

QDateTime FFmpegWidget::getLastTime() const
{
    return thread->getLastTime();
}

bool FFmpegWidget::getCallback() const
{
    return thread->getCallback();
}

bool FFmpegWidget::getIsPlaying() const
{
    return thread->getIsPlaying();
}

bool FFmpegWidget::getIsRtsp() const
{
    return thread->getIsRtsp();
}

bool FFmpegWidget::getIsUsbCamera() const
{
    return thread->getIsUsbCamera();
}

bool FFmpegWidget::getCopyImage() const
{
    return this->copyImage;
}

bool FFmpegWidget::getCheckLive() const
{
    return this->checkLive;
}

bool FFmpegWidget::getDrawImage() const
{
    return this->drawImage;
}

bool FFmpegWidget::getFillImage() const
{
    return this->fillImage;
}

bool FFmpegWidget::getFlowEnable() const
{
    return this->flowEnable;
}

QColor FFmpegWidget::getFlowBgColor() const
{
    return this->flowBgColor;
}

QColor FFmpegWidget::getFlowPressColor() const
{
    return this->flowPressColor;
}

int FFmpegWidget::getTimeout() const
{
    return this->timeout;
}

int FFmpegWidget::getBorderWidth() const
{
    return this->borderWidth;
}

QColor FFmpegWidget::getBorderColor() const
{
    return this->borderColor;
}

QColor FFmpegWidget::getFocusColor() const
{
    return this->focusColor;
}

QColor FFmpegWidget::getBgColor() const
{
    return this->bgColor;
}

QString FFmpegWidget::getBgText() const
{
    return this->bgText;
}

QImage FFmpegWidget::getBgImage() const
{
    return this->bgImage;
}

bool FFmpegWidget::getOSD1Visible() const
{
    return this->osd1Visible;
}

int FFmpegWidget::getOSD1FontSize() const
{
    return this->osd1FontSize;
}

QString FFmpegWidget::getOSD1Text() const
{
    return this->osd1Text;
}

QColor FFmpegWidget::getOSD1Color() const
{
    return this->osd1Color;
}

QImage FFmpegWidget::getOSD1Image() const
{
    return this->osd1Image;
}

FFmpegWidget::OSDFormat FFmpegWidget::getOSD1Format() const
{
    return this->osd1Format;
}

FFmpegWidget::OSDPosition FFmpegWidget::getOSD1Position() const
{
    return this->osd1Position;
}

bool FFmpegWidget::getOSD2Visible() const
{
    return this->osd2Visible;
}

int FFmpegWidget::getOSD2FontSize() const
{
    return this->osd2FontSize;
}

QString FFmpegWidget::getOSD2Text() const
{
    return this->osd2Text;
}

QColor FFmpegWidget::getOSD2Color() const
{
    return this->osd2Color;
}

QImage FFmpegWidget::getOSD2Image() const
{
    return this->osd2Image;
}

FFmpegWidget::OSDFormat FFmpegWidget::getOSD2Format() const
{
    return this->osd2Format;
}

FFmpegWidget::OSDPosition FFmpegWidget::getOSD2Position() const
{
    return this->osd2Position;
}

int FFmpegWidget::getFaceBorder() const
{
    return this->faceBorder;
}

QColor FFmpegWidget::getFaceColor() const
{
    return this->faceColor;
}

QList<QRect> FFmpegWidget::getFaceRects() const
{
    return this->faceRects;
}

QSize FFmpegWidget::sizeHint() const
{
    return QSize(400, 300);
}

QSize FFmpegWidget::minimumSizeHint() const
{
    return QSize(40, 30);
}

void FFmpegWidget::snapImage(const QImage &image)
{
    QString fileName = this->property("fileName").toString();
    if (!image.isNull()) {
        image.save(fileName, "jpg");
    }
}

void FFmpegWidget::updateImage(const QImage &image)
{
    //暂停或者不可见 rtsp视频流需要停止绘制
    if (drawImage && !this->property("isPause").toBool() && this->isVisible() && thread->getIsPlaying()) {
        //拷贝图片有个好处,当处理器比较差的时候,图片不会产生断层,缺点是占用时间
        //默认QImage类型是浅拷贝,可能正在绘制的时候,那边已经更改了图片的上部分数据
        this->image = copyImage ? image.copy() : image;
        this->update();
    }
}

void FFmpegWidget::updateFrame(AVFrame *frame)
{
#ifdef opengl
    //暂停或者不可见 rtsp视频流需要停止绘制
    if (drawImage && !this->property("isPause").toBool() && (yuvWidget->isVisible() || nv12Widget->isVisible()) && thread->getIsPlaying()) {
        //采用了硬件加速的直接用nv12渲染,否则采用yuv渲染
        if (thread->getHardware() == "none") {
            yuvWidget->setFrameSize(frame->width, frame->height);
            yuvWidget->updateTextures(frame->data[0], frame->data[1], frame->data[2], frame->linesize[0], frame->linesize[1], frame->linesize[2]);
        } else {
            nv12Widget->setFrameSize(frame->width, frame->height);
            nv12Widget->updateTextures(frame->data[0], frame->data[1], frame->linesize[0], frame->linesize[1]);
        }
    }
#endif
}

void FFmpegWidget::checkVideo()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime lastTime = thread->getLastTime();
    int sec = lastTime.secsTo(now);
    if (sec >= timeout) {
        restart(this->getUrl());
    }
}

void FFmpegWidget::btnClicked()
{
    QPushButton *btn = (QPushButton *)sender();
    emit btnClicked(btn->objectName());
}

void FFmpegWidget::playStart()
{
    //仅仅只有音频不需要处理
    if (thread->getOnlyAudio()) {
        return;
    }

    //回调模式采用painter自绘
    if (thread->getCallback()) {
        return;
    }

#ifdef opengl
    timerOSD->start();
    if (thread->getHardware() == "none") {
        yuvWidget->show();
    } else {
        nv12Widget->show();
    }

    if (!fillImage) {
        this->setAutoFillBackground(true);
        //填充主背景颜色,可以自己设置这个颜色
        QPalette palette = this->palette();
        palette.setBrush(QPalette::Window, Qt::black);
        this->setPalette(palette);
    }

    //重新设置尺寸
    resizeEvent(NULL);
#endif
}

void FFmpegWidget::playFinsh()
{
    //仅仅只有音频或者拖曳期间不需要处理
    if (thread->getOnlyAudio() || isDrag) {
        return;
    }

    //回调模式采用painter自绘
    if (thread->getCallback()) {
        return;
    }

    this->clear();
#ifdef opengl
    timerOSD->stop();
    if (thread->getHardware() == "none") {
        yuvWidget->clear();
        yuvWidget->hide();
    } else {
        nv12Widget->clear();
        nv12Widget->hide();
    }

    //qApp->processEvents();
    if (!fillImage) {
        this->setAutoFillBackground(false);
    }
#endif
}

void FFmpegWidget::clearDrag()
{
    isDrag = false;
}

FFmpegThread *FFmpegWidget::getThread()
{
    return this->thread;
}

qint64 FFmpegWidget::getLength()
{
    return thread->getLength();
}

qint64 FFmpegWidget::getPosition()
{
    return thread->getPosition();
}

void FFmpegWidget::setPosition(qint64 position)
{
    thread->setPosition(position);
}

float FFmpegWidget::getRate()
{
    return thread->getRate();
}

void FFmpegWidget::setRate(float rate)
{
    thread->setRate(rate);
}

bool FFmpegWidget::getMuted()
{
    return thread->getMuted();
}

void FFmpegWidget::setMuted(bool muted)
{
    thread->setMuted(muted);
}

int FFmpegWidget::getVolume()
{
    return thread->getVolume();
}

void FFmpegWidget::setVolume(int volume)
{
    thread->setVolume(volume);
}

void FFmpegWidget::setInterval(int interval)
{
    thread->setInterval(interval);
}

void FFmpegWidget::setVideoFps(int videoFps)
{
    thread->setVideoFps(videoFps);
}

void FFmpegWidget::setSleepTime(int sleepTime)
{
    thread->setSleepTime(sleepTime);
}

void FFmpegWidget::setReadTime(int readTime)
{
    thread->setReadTime(readTime);
}

void FFmpegWidget::setCheckTime(int checkTime)
{
    thread->setCheckTime(checkTime);
}

void FFmpegWidget::setCheckConn(bool checkConn)
{
    thread->setCheckConn(checkConn);
}

void FFmpegWidget::setPlayRepeat(bool playRepeat)
{
    thread->setPlayRepeat(playRepeat);
}

void FFmpegWidget::setPlayAudio(bool playAudio)
{
    thread->setPlayAudio(playAudio);
}

void FFmpegWidget::setSaveMp4(bool saveMp4)
{
    thread->setSaveMp4(saveMp4);
}

void FFmpegWidget::setMultiMode(bool multiMode, int videoWidth, int videoHeight)
{
    thread->setMultiMode(multiMode, videoWidth, videoHeight);
}

void FFmpegWidget::setUrl(const QString &url)
{
    thread->setUrl(url);
}

void FFmpegWidget::setCallback(bool callback)
{
    thread->setCallback(callback);
}

void FFmpegWidget::setHardware(const QString &hardware)
{
    thread->setHardware(hardware);
}

void FFmpegWidget::setTransport(const QString &transport)
{
    thread->setTransport(transport);
}

void FFmpegWidget::setImageFlag(int imageFlag)
{
    thread->setImageFlag((FFmpegThread::ImageFlag)imageFlag);
}

void FFmpegWidget::setOption(const char *key, const char *value)
{
    thread->setOption(key, value);
}

void FFmpegWidget::setSaveFile(bool saveFile)
{
    thread->setSaveFile(saveFile);
}

void FFmpegWidget::setSaveInterval(int saveInterval)
{
    thread->setSaveInterval(saveInterval);
}

void FFmpegWidget::setSavePath(const QString &savePath)
{
    //如果目录不存在则新建
    QDir dir(savePath);
    if (!dir.exists()) {
        dir.mkdir(savePath);
    }

    thread->setSavePath(savePath);
}

void FFmpegWidget::setFileFlag(const QString &fileFlag)
{
    thread->setFileFlag(fileFlag);
}

void FFmpegWidget::setFileName(const QString &fileName)
{
    thread->setFileName(fileName);
}

void FFmpegWidget::setSaveTime(const QDateTime &saveTime)
{
    thread->setSaveTime(saveTime);
}

void FFmpegWidget::setCopyImage(bool copyImage)
{
    this->copyImage = copyImage;
}

void FFmpegWidget::setCheckLive(bool checkLive)
{
    this->checkLive = checkLive;
}

void FFmpegWidget::setDrawImage(bool drawImage)
{
    this->drawImage = drawImage;
}

void FFmpegWidget::setFillImage(bool fillImage)
{
    this->fillImage = fillImage;
}

void FFmpegWidget::setFlowEnable(bool flowEnable)
{
    this->flowEnable = flowEnable;
}

void FFmpegWidget::setFlowBgColor(const QColor &flowBgColor)
{
    if (this->flowBgColor != flowBgColor) {
        this->flowBgColor = flowBgColor;
        this->initFlowStyle();
    }
}

void FFmpegWidget::setFlowPressColor(const QColor &flowPressColor)
{
    if (this->flowPressColor != flowPressColor) {
        this->flowPressColor = flowPressColor;
        this->initFlowStyle();
    }
}

void FFmpegWidget::setTimeout(int timeout)
{
    this->timeout = timeout;
}

void FFmpegWidget::setBorderWidth(int borderWidth)
{
    this->borderWidth = borderWidth;
    this->update();
}

void FFmpegWidget::setBorderColor(const QColor &borderColor)
{
    this->borderColor = borderColor;
    this->update();
}

void FFmpegWidget::setFocusColor(const QColor &focusColor)
{
    this->focusColor = focusColor;
    this->update();
}

void FFmpegWidget::setBgColor(const QColor &bgColor)
{
    this->bgColor = bgColor;
    this->update();
}

void FFmpegWidget::setBgText(const QString &bgText)
{
    this->bgText = bgText;
    this->update();
}

void FFmpegWidget::setBgImage(const QImage &bgImage)
{
    this->bgImage = bgImage;
    this->update();
}

void FFmpegWidget::setOSD1Visible(bool osdVisible)
{
    this->osd1Visible = osdVisible;
    this->update();
}

void FFmpegWidget::setOSD1FontSize(int osdFontSize)
{
    this->osd1FontSize = osdFontSize;
    this->update();
}

void FFmpegWidget::setOSD1Text(const QString &osdText)
{
    this->osd1Text = osdText;
    this->update();
}

void FFmpegWidget::setOSD1Color(const QColor &osdColor)
{
    this->osd1Color = osdColor;
    this->update();
}

void FFmpegWidget::setOSD1Image(const QImage &osdImage)
{
    this->osd1Image = osdImage;
}

void FFmpegWidget::setOSD1Format(const FFmpegWidget::OSDFormat &osdFormat)
{
    this->osd1Format = osdFormat;
    this->update();
}

void FFmpegWidget::setOSD1Position(const FFmpegWidget::OSDPosition &osdPosition)
{
    this->osd1Position = osdPosition;
    this->update();
}

void FFmpegWidget::setOSD2Visible(bool osdVisible)
{
    this->osd2Visible = osdVisible;
    this->update();
}

void FFmpegWidget::setOSD2FontSize(int osdFontSize)
{
    this->osd2FontSize = osdFontSize;
    this->update();
}

void FFmpegWidget::setOSD2Text(const QString &osdText)
{
    this->osd2Text = osdText;
    this->update();
}

void FFmpegWidget::setOSD2Color(const QColor &osdColor)
{
    this->osd2Color = osdColor;
    this->update();
}

void FFmpegWidget::setOSD2Image(const QImage &osdImage)
{
    this->osd2Image = osdImage;
    this->update();
}

void FFmpegWidget::setOSD2Format(const FFmpegWidget::OSDFormat &osdFormat)
{
    this->osd2Format = osdFormat;
    this->update();
}

void FFmpegWidget::setOSD2Position(const FFmpegWidget::OSDPosition &osdPosition)
{
    this->osd2Position = osdPosition;
    this->update();
}

void FFmpegWidget::setOSD1Format(quint8 osdFormat)
{
    setOSD1Format((FFmpegWidget::OSDFormat)osdFormat);
}

void FFmpegWidget::setOSD2Format(quint8 osdFormat)
{
    setOSD2Format((FFmpegWidget::OSDFormat)osdFormat);
}

void FFmpegWidget::setOSD1Position(quint8 osdPosition)
{
    setOSD1Position((FFmpegWidget::OSDPosition)osdPosition);
}

void FFmpegWidget::setOSD2Position(quint8 osdPosition)
{
    setOSD2Position((FFmpegWidget::OSDPosition)osdPosition);
}

void FFmpegWidget::setFaceBorder(int faceBorder)
{
    this->faceBorder = faceBorder;
    this->update();
}

void FFmpegWidget::setFaceColor(const QColor &faceColor)
{
    this->faceColor = faceColor;
    this->update();
}

void FFmpegWidget::setFaceRects(const QList<QRect> &faceRects)
{
    this->faceRects = faceRects;
    this->update();
}

void FFmpegWidget::open()
{
    //qDebug() << TIMEMS << "open video" << objectName();
    clear();

    //如果是图片则只显示图片就行
    image = QImage(thread->getUrl());
    if (!image.isNull()) {
        update();
        return;
    }

    thread->play();
    thread->start();

    if (checkLive) {
        timerCheck->start();
    }

    this->setProperty("isPause", false);
}

void FFmpegWidget::pause()
{
    //视频流无法暂停只能伪装暂停因为视频流是实时流
    //本地文件采用暂停解码的形式而视频流采用继续解码但是不现实的形式
    if (!this->property("isPause").toBool()) {
        thread->pause();
        this->setProperty("isPause", true);
    }
}

void FFmpegWidget::next()
{
    if (this->property("isPause").toBool()) {
        thread->next();
        this->setProperty("isPause", false);
    }
}

void FFmpegWidget::close()
{
    //qDebug() << TIMEMS << "close video" << objectName();
    if (thread->isRunning()) {
        thread->stop();
        thread->quit();
        thread->wait();
    }

    if (checkLive) {
        timerCheck->stop();
    }

    this->clear();
    //QTimer::singleShot(5, this, SLOT(clear()));
}

void FFmpegWidget::restart(const QString &url, int delayOpen)
{
    //qDebug() << TIMEMS << "restart video" << objectName();
    //关闭视频
    close();
    //重新设置播放地址
    setUrl(url);
    //如果是拖曳产生的则发送拖曳信号
    if (isDrag) {
        emit fileDrag(url);
    }

    //打开视频
    if (delayOpen > 0) {
        QTimer::singleShot(delayOpen, this, SLOT(open()));
    } else {
        open();
    }
}

void FFmpegWidget::clear()
{
    image = QImage();
    update();
}

void FFmpegWidget::snap(const QString &fileName)
{
    //回调取当前图片,句柄由线程调用函数截图
    if (thread->getCallback()) {
        if (!image.isNull()) {
            image.save(fileName, "jpg");
        }
    } else {
        //设置弱属性等待截图信号进行保存
        this->setProperty("fileName", fileName);
        thread->snap();
    }
}

void FFmpegWidget::startSave()
{
    thread->initSave();
}

void FFmpegWidget::stopSave()
{
    thread->stopSave();
}
