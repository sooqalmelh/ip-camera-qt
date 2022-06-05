/**
 * @file frmtab.cpp
 * @author creekwater
 * @brief
 *
 * TAB控件，用来切换不同的视频流测试窗体，所有窗体的入口
 *
 * @version 0.1
 * @date 2022-05-29
 *
 * @copyright Copyright (c) 2022
 *
 */


#include "frmtab.h"
#include "ui_frmtab.h"
#include "head.h"
#include "frmmain.h"

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
    ui->tabWidget->addTab(new frmMain, " 视频流播放器 ");
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

/**
 * @brief 接收其他控件发来的全屏信号，会调用这个接口
 *
 * @param full
 */
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
