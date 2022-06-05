/**
 * @file frmmain.cpp
 * @author creekwater
 * @brief
 *
 * 主界面
 *
 * @version 0.1
 * @date 2022-06-03
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "frmmain.h"
#include "ui_frmmain.h"
#include "head.h"
#include "frmmonitor.h"

frmMain::frmMain(QWidget *parent) : QWidget(parent), ui(new Ui::frmMain)
{
    ui->setupUi(this);
    this->initForm();
}

frmMain::~frmMain()
{
    delete ui;
}

void frmMain::initForm()
{
#ifdef Q_OS_ANDROID
    frmMonitor *video2 = new frmMonitor;
    video2->setName("video2");
    ui->layout->addWidget(video2, 0, 0);
#else
    frmMonitor *video1 = new frmMonitor;
    video1->setName("video1");
    ui->layout->addWidget(video1, 0, 0);
#endif
}
