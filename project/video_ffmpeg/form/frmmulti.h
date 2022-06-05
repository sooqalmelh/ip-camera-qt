#ifndef FRMMULTI_H
#define FRMMULTI_H

#include <QWidget>

namespace Ui {
class frmMulti;
}

class frmMulti : public QWidget
{
    Q_OBJECT

public:
    explicit frmMulti(QWidget *parent = 0);
    ~frmMulti();

protected:
    void showEvent(QShowEvent *);

private:
    Ui::frmMulti *ui;

private slots:
    void initForm();
    void initConfig();
    void playStart();
    void playFinsh();
};

#endif // FRMMULTI_H
