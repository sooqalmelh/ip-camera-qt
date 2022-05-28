#include "frmtab.h"
#include "ui_frmtab.h"
#include "head.h"
#include "frmmain.h"
#include "frmview.h"
#include "frmplayer.h"
#include "frmmulti.h"
#include "frmtool.h"

frmTab::frmTab(QWidget *parent) : QWidget(parent), ui(new Ui::frmTab)
{
    ui->setupUi(this);
    this->initForm();
    this->initConfig();
}

frmTab::~frmTab()
{
    delete ui;
}

void frmTab::closeEvent(QCloseEvent *)
{
    AppConfig::IsMax = this->isMaximized();
    AppConfig::writeConfig();
    exit(0);
}

void frmTab::initForm()
{
    frmView *view = new frmView;
    frmPlayer *player = new frmPlayer;
    connect(view, SIGNAL(fullScreen(bool)), this, SLOT(fullScreen(bool)));
    connect(player, SIGNAL(fullScreen(bool)), this, SLOT(fullScreen(bool)));

    ui->tabWidget->addTab(new frmMain, " 视频流播放器 ");
    ui->tabWidget->addTab(view, " 视频监控界面 ");
    ui->tabWidget->addTab(player, " 视频播放器 ");
#ifndef Q_OS_ANDROID
    ui->tabWidget->addTab(new frmMulti, " 视频多端复用 ");
    ui->tabWidget->addTab(new frmTool, " 其他功能测试 ");
#endif
}

void frmTab::initConfig()
{
    ui->tabWidget->setCurrentIndex(AppConfig::TabIndex);
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(saveConfig()));

    if (AppConfig::IsMax) {
        QTimer::singleShot(100, this, SLOT(showMaximized()));
    }
}

void frmTab::saveConfig()
{
    AppConfig::TabIndex = ui->tabWidget->currentIndex();
    AppConfig::writeConfig();
}

void frmTab::fullScreen(bool full)
{
    static Qt::WindowStates state;
    if (full) {
        //全屏前记住最后的窗体状态以便恢复
        state = this->windowState();
        this->showFullScreen();
    } else {
        this->setWindowState(state);        
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    ui->tabWidget->tabBar()->setHidden(full);
#endif
}
