#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>

namespace Ui {
class ProfileDialog;
}

class ProfileDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ProfileDialog(QWidget *parent = 0);
    ~ProfileDialog();
    
private slots:
    void on_buttonBox_accepted();

private:
    Ui::ProfileDialog *ui;
};

#endif // PROFILEDIALOG_H
