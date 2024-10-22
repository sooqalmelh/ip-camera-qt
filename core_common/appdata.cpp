﻿/**
 * @file appdata.cpp
 * @author creekwater
 * @brief
 *
 * 设置整个APP软件，顶层配置
 *
 * @version 0.1
 * @date 2022-06-05
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "appdata.h"
#include "quihelper.h"

QString AppData::Version = "2022-06-01";
QString AppData::TitleFlag = "powered by creekwater<灵溪驿站>";

int AppData::RowHeight = 25;        // 行高
int AppData::RightWidth = 180;      // 右侧宽度
int AppData::FormWidth = 1200;      // 窗体宽度
int AppData::FormHeight = 750;      // 窗体高度

void AppData::checkRatio()
{
    //根据分辨率设定宽高
    int width = QUIHelper::deskWidth();
    if (width >= 1440) {
        RowHeight = RowHeight < 25 ? 25 : RowHeight;
        RightWidth = RightWidth < 220 ? 220 : RightWidth;
        FormWidth = FormWidth < 1200 ? 1200 : FormWidth;
        FormHeight = FormHeight < 800 ? 800 : FormHeight;
    }
}
