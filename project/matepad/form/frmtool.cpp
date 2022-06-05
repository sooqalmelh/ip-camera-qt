#include "frmtool.h"
#include "ui_frmtool.h"
#include "ffmpeghead.h"
#include "ffmpegtool.h"
#include "qfiledialog.h"
#include "qtimer.h"

frmTool::frmTool(QWidget *parent) : QWidget(parent), ui(new Ui::frmTool)
{
    ui->setupUi(this);
    this->initForm();
}

frmTool::~frmTool()
{
    delete ui;
}

void frmTool::initForm()
{
    QStringList cmds;
    cmds << "获取文件信息" << "输出json格式" << "显示编解码器" << "显示格式" << "显示滤镜" << "显示协议" << "显示硬解码";
    ui->cboxCmd->addItems(cmds);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(addProgress()));
    timer->setInterval(100);

    ffmpegTool = new FFmpegTool(this);
    connect(ffmpegTool, SIGNAL(started()), this, SLOT(started()));
    connect(ffmpegTool, SIGNAL(finished()), this, SLOT(finished()));
    connect(ffmpegTool, SIGNAL(receiveData(QString)), this, SLOT(receiveData(QString)));
}

void frmTool::addProgress()
{
    int value = ui->progressBar->value();
    if (value < 95) {
        value += 3;
    } else {
        timer->stop();
    }

    ui->progressBar->setValue(value);
}

void frmTool::openFile(int type)
{
    //可以增加拓展名过滤
    QString filter = QString("所有文件 (*.*)");
    QLineEdit *txt = ui->txtH264;
    if (type == 0) {
        txt = ui->txtH264;
    } else if (type == 1) {
        txt = ui->txtAAC;
        filter = QString("音频文件 (*.aac)");
    } else if (type == 2) {
        txt = ui->txtMP4;
    }

    QString fileName;
    if (type == 0 || type == 1) {
        fileName = QFileDialog::getOpenFileName(this, "打开", qApp->applicationDirPath(), filter);
    } else if (type == 2) {
        fileName = QFileDialog::getSaveFileName(this, "打开", qApp->applicationDirPath(), filter);
    }

    if (!fileName.isEmpty()) {
        txt->setText(fileName);
    }
}

void frmTool::append(int type, const QString &data, bool clear)
{
    static int currentCount = 0;
    static int maxCount = 100;

    if (clear) {
        ui->txtMain->clear();
        currentCount = 0;
        return;
    }

    if (currentCount >= maxCount) {
        ui->txtMain->clear();
        currentCount = 0;
    }

#if 1
    ui->txtMain->append(data);
    currentCount++;
#else
    //过滤回车换行符
    QString strData = data;
    //strData.replace("\r", "");
    //strData.replace("\n", "");

    //不同类型不同颜色显示
    QString strType;
    if (type == 0) {
        strType = "发送";
        ui->txtMain->setTextColor(QColor("darkgreen"));
    } else if (type == 1) {
        strType = "接收";
        ui->txtMain->setTextColor(QColor("red"));
    } else if (type == 2) {
        strType = "解析";
        ui->txtMain->setTextColor(QColor("#22A3A9"));
    }

    strData = QString("时间[%1] %2: %3").arg(TIMEMS).arg(strType).arg(strData);
    ui->txtMain->append(strData);
    currentCount++;
#endif

    //输出数据
    //qDebug() << strData;
}

void frmTool::started()
{
    on_btnClear_clicked();
    ui->progressBar->setValue(0);
    ui->widget->setEnabled(false);
    timer->start();
}

void frmTool::finished()
{
    ui->progressBar->setValue(100);
    ui->widget->setEnabled(true);
    timer->stop();
}

void frmTool::receiveData(const QString &data)
{
    append(2, data);
}

void frmTool::on_btnOpenH264_clicked()
{
    openFile(0);
}

void frmTool::on_btnOpenAAC_clicked()
{
    openFile(1);
}

void frmTool::on_btnOpenMP4_clicked()
{
    openFile(2);
}

void frmTool::on_btnMp4Cmd_clicked()
{
    QString h264File = ui->txtH264->text().trimmed();
    QString aacFile = ui->txtAAC->text().trimmed();
    QString mp4File = ui->txtMP4->text().trimmed();
    ffmpegTool->h264ToMp4ByCmd(h264File, aacFile, mp4File);
}

void frmTool::on_btnMp4Code_clicked()
{
    QString h264File = ui->txtH264->text().trimmed();
    QString aacFile = ui->txtAAC->text().trimmed();
    QString mp4File = ui->txtMP4->text().trimmed();
    ffmpegTool->h264ToMp4ByCode(h264File, aacFile, mp4File);
}

void frmTool::on_btnCmd_clicked()
{
    QString binFile = "./ffmpeg";
#ifdef Q_OS_WIN32
    binFile = "ffmpeg.exe";
#endif
    QString mediaFile = ui->txtH264->text().trimmed();
    int index = ui->cboxCmd->currentIndex();
    if (index == 0) {
        ffmpegTool->getMediaInfo(mediaFile);
    } else if (index == 1) {
        ffmpegTool->getMediaInfo(mediaFile, true);
    } else if (index == 2) {
        //所有编码解码器:-codecs  单独解码器:-decoders
        ffmpegTool->start(binFile + " -codecs");
    } else if (index == 3) {
        ffmpegTool->start(binFile + " -formats");
    } else if (index == 4) {
        ffmpegTool->start(binFile + " -filters");
    } else if (index == 5) {
        ffmpegTool->start(binFile + " -protocols");
    } else if (index == 6) {
        ffmpegTool->start(binFile + " -hwaccels");
    }
}

void frmTool::on_btnClear_clicked()
{
    append(0, "", true);
}
