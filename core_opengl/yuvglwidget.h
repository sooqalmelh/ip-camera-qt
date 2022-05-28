#ifndef YUVGLWIDGET_H
#define YUVGLWIDGET_H

#include <QFile>
#include <QTimer>
#include <QGLWidget>
#include <QGLFunctions>
#include <QGLShaderProgram>

class YUVWidget: public QGLWidget, protected QGLFunctions
{
    Q_OBJECT
public:
    explicit YUVWidget(QWidget *parent = 0);
    ~YUVWidget();

    //清空数据
    void clear();
    //设置图片尺寸
    void setFrameSize(int width, int height);
    //更新纹理数据
    void updateTextures(quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV);

protected:    
    void initializeGL();
    void resizeGL(int width, int height);
    void paintGL();

private:
    void initData();
    void initColor();
    void initShader();
    void initTextures();
    void initParamete();
    void deleteTextures();

private:
    //图片宽度高度
    int width, height;
    //YUV原数据
    quint8 *dataY, *dataU, *dataV;
    //YUV数据尺寸
    quint32 linesizeY, linesizeU, linesizeV;
    //顶点着色器代码+片段着色器代码
    QString shaderVert, shaderFrag;

    //着色器程序,编译链接着色器
    QGLShaderProgram program;
    //YUV纹理,用于生成纹理贴图
    GLuint textureY, textureU, textureV;

private:
    //直接读取文件
    QFile file;
    //读取文件内容定时器
    QTimer timer;

public slots:
    //播放本地文件
    void open(const QString &fileName, int interval);
    //读取文件内容
    void read();
};

#endif // YUVGLWIDGET_H
