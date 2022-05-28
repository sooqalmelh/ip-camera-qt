#include "qcoreapplication.h"
#include "yuvopenglwidget.h"
#include "openglhead.h"
#include "qdebug.h"

YUVWidget::YUVWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    QStringList list;
    list << "attribute vec4 vertexIn;";
    list << "attribute vec2 textureIn;";
    list << "varying vec2 textureOut;";

    list << "void main(void)";
    list << "{";
    list << "  gl_Position = vertexIn;";
    list << "  textureOut = textureIn;";
    list << "}";
    shaderVert = list.join("");

    list.clear();
    //opengles的float、int等要手动指定精度
#ifdef __arm__
    bool useOpenGLES = true;
#else
    bool useOpenGLES = QCoreApplication::testAttribute(Qt::AA_UseOpenGLES);
#endif
    if (useOpenGLES) {
        list << "precision mediump int;";
        list << "precision mediump float;";
    }

    list << "varying mediump vec2 textureOut;";
    list << "uniform sampler2D textureY;";
    list << "uniform sampler2D textureU;";
    list << "uniform sampler2D textureV;";

    list << "void main(void)";
    list << "{";
    list << "  vec3 yuv;";
    list << "  vec3 rgb;";
    //可以自行注释xyz以及调整差值看效果,把yz注释画面变成黑白
    list << "  yuv.x = texture2D(textureY, textureOut).r - 0.0625;";
    list << "  yuv.y = texture2D(textureU, textureOut).r - 0.5;";
    list << "  yuv.z = texture2D(textureV, textureOut).r - 0.5;";
    list << "  rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.39465, 2.03211, 1.13983, -0.58060, 0.0) * yuv;";
    list << "  gl_FragColor = vec4(rgb, 1.0);";
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
    //qDebug() << TIMEMS << linesizeY << linesizeU << linesizeV;
}

void YUVWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);

    //传递顶点和纹理坐标
    static const GLfloat ver[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
    static const GLfloat tex[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};

    //设置顶点,纹理数组并启用
    glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, ver);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, tex);
    glEnableVertexAttribArray(1);

    //初始化shader
    this->initShader();
    //初始化textures
    this->initTextures();
    //初始化颜色
    this->initColor();
}

void YUVWidget::paintGL()
{
    if (!dataY || !dataU || !dataV || width == 0 || height == 0) {
        this->initColor();
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureY);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, linesizeY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, dataY);
    glUniform1i(textureUniformY, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureU);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, linesizeU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width >> 1, height >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, dataU);
    glUniform1i(textureUniformU, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureV);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, linesizeV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width >> 1, height >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, dataV);
    glUniform1i(textureUniformV, 2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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
    QColor color = palette().window().color();
    //设置背景清理色
    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    //清理颜色背景
    glClear(GL_COLOR_BUFFER_BIT);
}

void YUVWidget::initShader()
{
    //加载顶点和片元脚本
    program.addShaderFromSourceCode(QOpenGLShader::Vertex, shaderVert);
    program.addShaderFromSourceCode(QOpenGLShader::Fragment, shaderFrag);

    //设置顶点位置
    program.bindAttributeLocation("vertexIn", 0);
    //设置纹理位置
    program.bindAttributeLocation("textureIn", 1);
    //编译shader
    program.link();
    program.bind();

    //从shader获取地址
    textureUniformY = program.uniformLocation("textureY");
    textureUniformU = program.uniformLocation("textureU");
    textureUniformV = program.uniformLocation("textureV");
}

void YUVWidget::initTextures()
{
    //创建纹理
    glGenTextures(1, &textureY);
    glBindTexture(GL_TEXTURE_2D, textureY);
    initParamete();

    glGenTextures(1, &textureU);
    glBindTexture(GL_TEXTURE_2D, textureU);
    initParamete();

    glGenTextures(1, &textureV);
    glBindTexture(GL_TEXTURE_2D, textureV);
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
