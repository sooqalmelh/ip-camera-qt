#ifndef FFMPEGTOOL_H
#define FFMPEGTOOL_H

#include <QObject>
#include <QProcess>

class FFmpegTool : public QObject
{
    Q_OBJECT
public:
    static FFmpegTool *Instance();
    explicit FFmpegTool(QObject *parent = 0);

private:
    static QScopedPointer<FFmpegTool> self;
    QProcess process;

public slots:
    //收到执行命令的数据
    void readData();

    //通用执行命令
    void start(const QString &command);
    void start(const QString &program, const QStringList &arguments);

    //获取文件信息
    void getMediaInfo(const QString &mediaFile, bool json = false);
    //命令执行将h264+aac转成mp4
    void h264ToMp4ByCmd(const QString &h264File, const QString &aacFile, const QString &mp4File);
    //代码执行将h264+aac转成mp4
    void h264ToMp4ByCode(const QString &h264File, const QString &aacFile, const QString &mp4File);

signals:
    void started();
    void finished();
    void receiveData(const QString &data);
};

#endif // FFMPEGTOOL_H
