#ifndef LICENSEDIALOG_H
#define LICENSEDIALOG_H

#include <QDialog>

namespace Ui {
class LicenseDialog;
}

class LicenseDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit LicenseDialog(bool showAccept = false, QWidget *parent = 0);
    ~LicenseDialog();
    
private:
    Ui::LicenseDialog *ui;
};

#endif // LICENSEDIALOG_H
