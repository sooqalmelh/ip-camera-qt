#ifndef FRMTOOL_H
#define FRMTOOL_H

#include <QWidget>
class FFmpegTool;

namespace Ui {
class frmTool;
}

class frmTool : public QWidget
{
    Q_OBJECT

public:
    explicit frmTool(QWidget *parent = 0);
    ~frmTool();

private:
    Ui::frmTool *ui;
    QTimer *timer;
    FFmpegTool *ffmpegTool;

private slots:
    void initForm();
    void addProgress();
    void openFile(int type);
    void append(int type, const QString &data, bool clear = false);

private slots:
    void started();
    void finished();
    void receiveData(const QString &data);

private slots:
    void on_btnOpenH264_clicked();
    void on_btnOpenAAC_clicked();
    void on_btnOpenMP4_clicked();
    void on_btnMp4Cmd_clicked();
    void on_btnMp4Code_clicked();
    void on_btnCmd_clicked();
    void on_btnClear_clicked();
};

#endif // FRMTOOL_H
