#include "readmmdialog.h"
#include "ui_readmmdialog.h"
#include "../profile/profiledialog.h"

ReadMMDialog::ReadMMDialog(QString *outfileNamep, qint32 progBarMin,
                           qint32 progBarMax, QString *tagfileNamep,
                           bool *useProfileFlagp,
                           QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReadMMDialog)
{
    ui->setupUi(this);
    ui->outFileLE->setText(*outfileNamep);
    ui->readMMProgBar->hide();
    ui->readMMProgBar->setRange(progBarMin, progBarMax);
    ui->readMMProgBar->setValue(progBarMin);
    ui->statusLA->hide();
    ui->tagStatLA->hide();
    abortedFlag = false;
    mytagfileNamep = tagfileNamep;
    myoutFileNamep = outfileNamep;
    myuseProfileFlagp = useProfileFlagp;
}


ReadMMDialog::~ReadMMDialog()
{
    delete ui;
}


void ReadMMDialog::setProgBarValue(qint32 value)
{
    ui->readMMProgBar->setValue(value);
}


void ReadMMDialog::setStatusText(QString statstr)
{
    ui->statusLA->setText(statstr);
}


void ReadMMDialog::setTagStatText(QString statstr)
{
    ui->tagStatLA->setText(statstr);
    ui->tagStatLA->setEnabled(false);
}


void ReadMMDialog::endState()
{
    ui->readMMProgBar->setValue(ui->readMMProgBar->maximum());
    ui->readMMProgBar->setEnabled(false);
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
    *mytagfileNamep = ui->tagFileLE->text();
}


void ReadMMDialog::on_buttonBox_accepted()
{
    *mytagfileNamep = ui->tagFileLE->text();
    *myoutFileNamep = ui->outDirLE->text().append(ui->outFileLE->text());
    ui->tagFileLA->setEnabled(false);
    ui->tagFileLE->setEnabled(false);
    ui->tagFileBrwsPB->setEnabled(false);
    ui->useProfileCB->setEnabled(false);
    ui->editProfilePB->setEnabled(false);
    ui->readMMProgBar->show();
    ui->statusLA->show();
    ui->tagStatLA->show();
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Abort);
    *myuseProfileFlagp = ui->useProfileCB->isChecked();
}


void ReadMMDialog::on_tagFileBrwsPB_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath());

    if(!fileName.isEmpty())
        ui->tagFileLE->setText(fileName);
}


void ReadMMDialog::on_editProfilePB_clicked()
{
    ProfileDialog profD;
    profD.exec();
}


void ReadMMDialog::on_outDirBrwsPB_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Out Directory"), QDir::currentPath());

    if(!dirName.isEmpty())
        ui->outDirLE->setText(dirName.append("\\"));
}
