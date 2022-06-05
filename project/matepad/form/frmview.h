#ifndef FRMVIEW_H
#define FRMVIEW_H

#include <QWidget>

class QMenu;
class VideoBox;
class FFmpegWidget;

namespace Ui {
class frmView;
}

class frmView : public QWidget
{
    Q_OBJECT

public:
    explicit frmView(QWidget *parent = 0);
    ~frmView();

protected:
    void keyReleaseEvent(QKeyEvent *key);
    void showEvent(QShowEvent *);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Ui::frmView *ui;

    bool videoMax;
    int videoCount;
    QMenu *videoMenu;
    QAction *actionFull;
    QAction *actionPoll;

    VideoBox *videoBox;
    QList<FFmpegWidget *> widgets;

private slots:
    void initForm();
    void initMenu();
    void full();
    void poll();

private slots:
    void play_video_all();
    void snapshot_video_one();
    void snapshot_video_all();

    //画面布局切换信号
    void changeVideo(int type, const QString &videoType, bool videoMax);

signals:
    void fullScreen(bool full);
};

#endif // FRMVIEW_H
