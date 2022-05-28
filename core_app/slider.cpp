#include "slider.h"
#include "qevent.h"
#include "qdebug.h"

Slider::Slider(QWidget *parent) : QSlider(parent)
{
}

void Slider::mousePressEvent(QMouseEvent *e)
{
    //限定必须是鼠标左键按下
    if (e->button() != Qt::LeftButton) {
        return;
    }

    //获取鼠标的位置
    double pos, value;
    if (orientation() == Qt::Horizontal) {
        pos = e->pos().x() / (double)width();
        value = pos * (maximum() - minimum()) + minimum();
    } else {
        pos = e->pos().y() / (double)height();
        value = maximum() - pos * (maximum() - minimum()) + minimum();
    }

    setValue(value + 0.5);

    //发送自定义的鼠标单击信号
    emit clicked();
    mouseMoveEvent(e);
    QSlider::mousePressEvent(e);
}
