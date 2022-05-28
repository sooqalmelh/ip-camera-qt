#include "frmplayer.h"
#include "ui_frmplayer.h"
#include "head.h"
#include "ffmpeghelper.h"

frmPlayer::frmPlayer(QWidget *parent) : QWidget(parent), ui(new Ui::frmPlayer)
{
    ui->setupUi(this);
    this->initForm();
    this->initConfig();
}

frmPlayer::~frmPlayer()
{
    delete ui;
}

void frmPlayer::fullScreen()
{
    full = !full;
    emit fullScreen(full);

    if (full) {
        ui->playWidget->setBorderWidth(0);
        ui->frameRight->setVisible(false);
        ui->frameBottom->setVisible(false);
        this->layout()->setContentsMargins(0, 0, 0, 0);
        this->layout()->setSpacing(0);
    } else {
        ui->playWidget->setBorderWidth(3);
        ui->frameRight->setVisible(true);
        ui->frameBottom->setVisible(true);
        this->layout()->setContentsMargins(6, 6, 6, 6);
        this->layout()->setSpacing(6);
    }
}

void frmPlayer::keyReleaseEvent(QKeyEvent *key)
{
    //当按下esc键并且当前处于全屏模式则切换到正常模式
    if (key->key() == Qt::Key_Escape && full) {
        fullScreen();
    }
}

bool frmPlayer::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->playWidget) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            fullScreen();
        }
    }

    return QWidget::eventFilter(watched, event);
}

void frmPlayer::initForm()
{
    full = false;
    ui->playWidget->installEventFilter(this);

    QStringList urls;
    ui->cboxUrl->addItems(AppUrl::getUrls(urls));

    ui->cboxRate->addItem("0.5倍速度", "0.5");
    ui->cboxRate->addItem("1.0倍速度", "1.0");
    ui->cboxRate->addItem("2.0倍速度", "2.0");
    ui->cboxRate->addItem("2.5倍速度", "2.5");
    ui->cboxRate->addItem("5.0倍速度", "5.0");
    ui->cboxRate->addItem("6.5倍速度", "6.5");
    ui->cboxRate->setCurrentIndex(1);

    //以下方法可以设置背景图片或者背景文字
    ui->playWidget->setBgText("视频监控");
    ui->playWidget->setBgImage(QImage(":/image/bg_novideo.png"));

    //以下方法可以设置OSD为图片
    ui->playWidget->setOSD1Image(QImage(":/image/bg_novideo.png"));
    ui->playWidget->setOSD1Format(FFmpegWidget::OSDFormat_Image);
    //ui->playWidget->setOSD1Visible(true);

    //设置边框宽度和颜色
    ui->playWidget->setBorderWidth(3);
    ui->playWidget->setBorderColor(QColor(0, 0, 0));

    //设置是否播放声音 默认真
    //ui->playWidget->setPlayAudio(false);

    connect(ui->playWidget, SIGNAL(fileDrag(QString)), this, SLOT(fileDrag(QString)));
    connect(ui->playWidget, SIGNAL(receivePlayStart()), this, SLOT(receivePlayStart()));
    connect(ui->playWidget, SIGNAL(receivePlayError()), this, SLOT(receivePlayError()));
    connect(ui->playWidget, SIGNAL(receivePlayFinsh()), this, SLOT(receivePlayFinsh()));
    connect(ui->playWidget, SIGNAL(fileLengthReceive(qint64)), this, SLOT(fileLengthReceive(qint64)));
    connect(ui->playWidget, SIGNAL(filePositionReceive(qint64)), this, SLOT(filePositionReceive(qint64)));
    connect(ui->playWidget, SIGNAL(fileVolumeReceive(int, bool)), this, SLOT(fileVolumeReceive(int, bool)));

    //安卓屏幕比较小尽量空出位置
#ifdef Q_OS_ANDROID
    AppConfig::Callback5 = true;
    ui->btnVersion->setVisible(false);
    ui->btnAbout->setVisible(false);
    ui->verticalSpacer->changeSize(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored);

    QList<QPushButton *> btns = ui->frameRight->findChildren<QPushButton *>();
    foreach (QPushButton *btn, btns) {
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
#endif
}

void frmPlayer::initConfig()
{
    ui->playWidget->setCheckTime(30 * 1000);
    ui->playWidget->setHardware(AppConfig::VideoConfig5.hardware);

    ui->cboxUrl->lineEdit()->setText(AppConfig::VideoConfig5.rtspAddr);
    ui->cboxUrl->lineEdit()->setCursorPosition(0);
    connect(ui->cboxUrl->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(saveConfig()));

    ui->ckCallback->setChecked(AppConfig::VideoConfig5.callback);
    connect(ui->ckCallback, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->ckFillImage->setChecked(AppConfig::VideoConfig5.fillImage);
    connect(ui->ckFillImage, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));

    ui->ckPlayRepeat->setChecked(AppConfig::VideoConfig5.playRepeat);
    connect(ui->ckPlayRepeat, SIGNAL(stateChanged(int)), this, SLOT(saveConfig()));
}

void frmPlayer::saveConfig()
{
    AppConfig::VideoConfig5.rtspAddr = ui->cboxUrl->lineEdit()->text();
    AppConfig::VideoConfig5.callback = ui->ckCallback->isChecked();
    AppConfig::VideoConfig5.fillImage = ui->ckFillImage->isChecked();
    AppConfig::VideoConfig5.playRepeat = ui->ckPlayRepeat->isChecked();
    AppConfig::writeConfig();
}

void frmPlayer::fileDrag(const QString &url)
{
    //设置回调
    ui->playWidget->setCallback(ui->ckCallback->isChecked());
    //设置拉伸填充
    ui->playWidget->setFillImage(ui->ckFillImage->isChecked());
    //设置重复播放
    ui->playWidget->setPlayRepeat(ui->ckPlayRepeat->isChecked());

    ui->cboxUrl->lineEdit()->setText(url);
    qDebug() << TIMEMS << "拖曳文件" << url;
}

void frmPlayer::receivePlayStart()
{
    qDebug() << TIMEMS << "播放开始";
}

void frmPlayer::receivePlayError()
{
    qDebug() << TIMEMS << "播放出错";
}

void frmPlayer::receivePlayFinsh()
{
    ui->labTimePlay->setText("00:00");
    ui->labTimeAll->setText("00:00");
    ui->sliderPosition->setValue(0);
    qDebug() << TIMEMS << "播放结束";
}

void frmPlayer::fileLengthReceive(qint64 length)
{
    //设置进度条最大进度以及总时长,自动在总时长前补零
    ui->sliderPosition->setMaximum(length);
    ui->sliderPosition->setValue(0);

    length = length / 1000;
    QString min = QString("%1").arg(length / 60, 2, 10, QChar('0'));
    QString sec = QString("%2").arg(length % 60, 2, 10, QChar('0'));
    ui->labTimeAll->setText(QString("%1:%2").arg(min).arg(sec));
}

void frmPlayer::filePositionReceive(qint64 position)
{
    //设置当前进度及已播放时长,自动在已播放时长前补零
    ui->sliderPosition->setValue(position);

    position = position / 1000;
    QString min = QString("%1").arg(position / 60, 2, 10, QChar('0'));
    QString sec = QString("%2").arg(position % 60, 2, 10, QChar('0'));
    ui->labTimePlay->setText(QString("%1:%2").arg(min).arg(sec));
}

void frmPlayer::fileVolumeReceive(int volume, bool muted)
{
    ui->sliderVolume->setValue(volume);
    ui->ckMuted->setChecked(muted);
}

void frmPlayer::on_btnPlay_clicked()
{
    if (!ui->playWidget->getIsPlaying()) {
        ui->playWidget->setUrl(ui->cboxUrl->currentText());
        ui->playWidget->setCallback(ui->ckCallback->isChecked());
        ui->playWidget->setFillImage(ui->ckFillImage->isChecked());
        ui->playWidget->setPlayRepeat(ui->ckPlayRepeat->isChecked());
        ui->playWidget->open();
    }
}

void frmPlayer::on_btnStop_clicked()
{
    ui->playWidget->close();
}

void frmPlayer::on_btnPause_clicked()
{
    ui->playWidget->pause();
}

void frmPlayer::on_btnNext_clicked()
{
    ui->playWidget->next();
}

void frmPlayer::on_btnSelect_clicked()
{
    QString file = QFileDialog::getOpenFileName(this);
    if (!file.isEmpty()) {
        ui->cboxUrl->lineEdit()->setText(file);
        on_btnStop_clicked();
        on_btnPlay_clicked();
    }
}

void frmPlayer::on_btnVersion_clicked()
{
    QMessageBox::information(this, "提示", QString("当前内核版本: %1").arg(getVersion()));
}

void frmPlayer::on_btnAbout_clicked()
{
    qApp->aboutQt();
}

void frmPlayer::on_ckMuted_clicked()
{
    ui->playWidget->setMuted(ui->ckMuted->isChecked());
}

void frmPlayer::on_sliderVolume_clicked()
{
    ui->playWidget->setVolume(ui->sliderVolume->value());
}

void frmPlayer::on_sliderVolume_sliderMoved(int value)
{
    ui->playWidget->setVolume(value);
}

void frmPlayer::on_sliderPosition_clicked()
{
    ui->playWidget->setPosition(ui->sliderPosition->value());
}

void frmPlayer::on_sliderPosition_sliderMoved(int value)
{
    ui->playWidget->setPosition(value);
}

void frmPlayer::on_cboxRate_currentIndexChanged(int index)
{
    float rate = ui->cboxRate->itemData(index).toFloat();
    ui->playWidget->setRate(rate);
}
