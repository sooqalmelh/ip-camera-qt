﻿#include "yuvglwidget.h"
#include "openglhead.h"
#include "qdatetime.h"
#include "qdebug.h"

//QGLFormat(QGL::SampleBuffers),
YUVWidget::YUVWidget(QWidget *parent) : QGLWidget(parent)
{
    QStringList list;
    list << "uniform sampler2D textureY;";
    list << "uniform sampler2D textureU;";
    list << "uniform sampler2D textureV;";

    list << "void main(void)";
    list << "{";
    list << "  float y, u, v, red, green, blue;";
    list << "  y = texture2D(textureY, gl_TexCoord[0].st).r;";
    list << "  y = 1.1643 * (y - 0.0625);";
    list << "  u = texture2D(textureU, gl_TexCoord[0].st).r - 0.5;";
    list << "  v = texture2D(textureV, gl_TexCoord[0].st).r - 0.5;";
    list << "  red = y + 1.5958 * v;";
    list << "  green = y - 0.39173 * u - 0.81290 * v;";
    list << "  blue = y + 2.017 * u;";
    list << "  gl_FragColor = vec4(red, green, blue, 1.0);";
    list << "}";
    shaderFrag = list.join("");

    this->initData();

    //关联定时器读取文件
    connect(&timer, SIGNAL(timeout()), this, SLOT(read()));
}

YUVWidget::~YUVWidget()
{
    makeCurrent();
    doneCurrent();
}

void YUVWidget::clear()
{
    this->initData();
    this->deleteTextures();
    this->update();
}

void YUVWidget::setFrameSize(int width, int height)
{
    this->width = width;
    this->height = height;
}

void YUVWidget::updateTextures(quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV)
{
    this->dataY = dataY;
    this->dataU = dataU;
    this->dataV = dataV;
    this->linesizeY = linesizeY;
    this->linesizeU = linesizeU;
    this->linesizeV = linesizeV;
    this->update();
}

void YUVWidget::initializeGL()
{
    initializeGLFunctions();
    glDisable(GL_DEPTH_TEST);

    //初始化shader
    this->initShader();
    //初始化textures
    this->initTextures();
    //初始化颜色
    this->initColor();    
}

void YUVWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void YUVWidget::paintGL()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataY);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureU);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width >> 1, height >> 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataU);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureV);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width >> 1, height >> 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataV);

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    //左上角
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 0.0f);
    //右上角
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f);
    //右下角
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    //左下角
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);

    glEnd();
    glFlush();
    glDisable(GL_TEXTURE_2D);
}

void YUVWidget::initData()
{
    width = height = 0;
    dataY = dataU = dataV = 0;
    linesizeY = linesizeU = linesizeV = 0;
}

void YUVWidget::initColor()
{
    //取画板背景颜色
    QColor color = palette().background().color();
    //设置背景清理色
    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    //清理颜色背景
    glClear(GL_COLOR_BUFFER_BIT);
}

void YUVWidget::initShader()
{
    program.addShaderFromSourceCode(QGLShader::Fragment, shaderFrag);
    program.link();
    program.bind();

    program.setUniformValue("textureY", 0);
    program.setUniformValue("textureU", 1);
    program.setUniformValue("textureV", 2);
}

void YUVWidget::initTextures()
{
    glGenTextures(1, &textureY);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataY);
    initParamete();

    glGenTextures(1, &textureU);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width >> 1, height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataU);
    initParamete();

    glGenTextures(1, &textureV);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width >> 1, height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, dataV);
    initParamete();
}

void YUVWidget::initParamete()
{
    //具体啥意思 https://blog.csdn.net/d04421024/article/details/5089641

    //纹理过滤
    //GL_TEXTURE_MAG_FILTER: 放大过滤
    //GL_TEXTURE_MIN_FILTER: 缩小过滤
    //GL_LINEAR: 线性插值过滤,获取坐标点附近4个像素的加权平均值
    //GL_NEAREST: 最临近过滤,获得最靠近纹理坐标点的像素
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    //纹理贴图
    //GL_TEXTURE_2D: 操作2D纹理
    //GL_TEXTURE_WRAP_S: S方向上的贴图模式
    //GL_TEXTURE_WRAP_T: T方向上的贴图模式
    //GL_CLAMP: 将纹理坐标限制在0.0,1.0的范围之内,如果超出了会如何呢,不会错误,只是会边缘拉伸填充
    //GL_CLAMP_TO_EDGE: 超出纹理范围的坐标被截取成0和1,形成纹理边缘延伸的效果
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
}

void YUVWidget::deleteTextures()
{
    glDeleteTextures(1, &textureY);
    glDeleteTextures(1, &textureU);
    glDeleteTextures(1, &textureV);
}

void YUVWidget::open(const QString &fileName, int interval)
{
    timer.stop();
    file.setFileName(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    //初始化对应数据指针位置
    dataY = new quint8[(width * height * 3) >> 1];
    dataU = dataY + (width * height);
    dataV = dataU + ((width * height) >> 2);

    //启动定时器读取文件数据
    timer.start(interval);
}

void YUVWidget::read()
{
    qint64 len = (width * height * 3) >> 1;
    if (file.read((char *)dataY, len)) {
        this->update();
    } else {
        timer.stop();
        this->clear();
    }
}
