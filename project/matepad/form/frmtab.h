#ifndef FRMTAB_H
#define FRMTAB_H

#include <QWidget>

namespace Ui {
class frmTab;
}

class frmTab : public QWidget
{
    Q_OBJECT

public:
    explicit frmTab(QWidget *parent = 0);
    ~frmTab();

protected:
    void closeEvent(QCloseEvent *);

private:
    Ui::frmTab *ui;

private slots:
    void initForm();
    void initConfig();
    void saveConfig();
    void fullScreen(bool full);
};

#endif // FRMTAB_H
