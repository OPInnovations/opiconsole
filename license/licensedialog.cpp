#include "licensedialog.h"
#include "ui_licensedialog.h"

LicenseDialog::LicenseDialog(bool showAccept, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LicenseDialog)
{
    ui->setupUi(this);
    if(!showAccept)
    {
        ui->label->hide();
        ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
    }
}

LicenseDialog::~LicenseDialog()
{
    delete ui;
}
