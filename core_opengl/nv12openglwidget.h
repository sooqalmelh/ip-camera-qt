#ifndef NV12OPENGLWIDGET_H
#define NV12OPENGLWIDGET_H

#include <QFile>
#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class NV12Widget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit NV12Widget(QWidget *parent = 0);
    ~NV12Widget();

    //清空数据
    void clear();
    //设置图片尺寸
    void setFrameSize(int width, int height);
    //更新纹理数据
    void updateTextures(quint8 *dataY, quint8 *dataUV, quint32 linesizeY, quint32 linesizeUV);

protected:
    void initializeGL();
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
    quint8 *dataY, *dataUV;
    //YUV数据尺寸
    quint32 linesizeY, linesizeUV;    
    //顶点着色器代码+片段着色器代码
    QString shaderVert, shaderFrag;

    //顶点缓冲对象
    QOpenGLBuffer vbo;
    //着色器程序,编译链接着色器
    QOpenGLShaderProgram program;
    //YUV纹理,用于生成纹理贴图
    GLuint textureY, textureUV;

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

#endif // NV12OPENGLWIDGET_H
