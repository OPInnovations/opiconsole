#include "profiledialog.h"
#include "ui_profiledialog.h"
#include "opi_helper.h"

ProfileDialog::ProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileDialog)
{
    QString readName, readDOB, readSex, readAlpha, readSigma;
    QString readSleep_DeltaTH2, readSleep_DeltaTH1, readSleep_ThetaTH, readSleep_SigmaTH, readSleep_BetaTH, readSleep_G1TH;
    QString readMed_DeltaTH2, readMed_ThetaTH, readMed_AlphaTH, readMed_SigmaTH, readMed_BetaTH, readMed_G1TH;

    ui->setupUi(this);

    readName = getConfigValue("Name");
    readDOB = getConfigValue("DOB");
    readSex = getConfigValue("Sex");
    readAlpha = getConfigValue("Alpha");
    readSigma = getConfigValue("Sigma");
    readSleep_DeltaTH2 = getConfigValue("Sleep_DeltaTH2");
    readSleep_DeltaTH1 = getConfigValue("Sleep_DeltaTH1");
    readSleep_ThetaTH = getConfigValue("Sleep_ThetaTH");
    readSleep_SigmaTH = getConfigValue("Sleep_SigmaTH");
    readSleep_BetaTH = getConfigValue("Sleep_BetaTH");
    readSleep_G1TH = getConfigValue("Sleep_G1TH");
    readMed_DeltaTH2 = getConfigValue("Med_DeltaTH2");
    readMed_ThetaTH = getConfigValue("Med_ThetaTH");
    readMed_AlphaTH = getConfigValue("Med_AlphaTH");
    readMed_SigmaTH = getConfigValue("Med_SigmaTH");
    readMed_BetaTH = getConfigValue("Med_BetaTH");
    readMed_G1TH = getConfigValue("Med_G1TH");

    // Fill in existing variables
    if(!readName.isEmpty()) ui->lpidnameLE->setText(readName);
    if(!readDOB.isEmpty()) ui->lpiddobLE->setText(readDOB);
    if(!readSex.isEmpty()) ui->lpidsexLE->setText(readSex);
    if(!readAlpha.isEmpty()) ui->alphaLE->setText(readAlpha);
    if(!readSigma.isEmpty()) ui->sigmaLE->setText(readSigma);
    if(!readSleep_DeltaTH2.isEmpty()) ui->deltaSE_D2->setText(readSleep_DeltaTH2);
    if(!readSleep_DeltaTH1.isEmpty()) ui->deltaSE_D1->setText(readSleep_DeltaTH1);
    if(!readSleep_ThetaTH.isEmpty()) ui->deltaSE_T->setText(readSleep_ThetaTH);
    if(!readSleep_SigmaTH.isEmpty()) ui->deltaSE_S->setText(readSleep_SigmaTH);
    if(!readSleep_BetaTH.isEmpty()) ui->deltaSE_B->setText(readSleep_BetaTH);
    if(!readSleep_G1TH.isEmpty()) ui->deltaSE_G->setText(readSleep_G1TH);
    if(!readMed_DeltaTH2.isEmpty()) ui->deltaME_D2->setText(readMed_DeltaTH2);
    if(!readMed_ThetaTH.isEmpty()) ui->deltaME_T->setText(readMed_ThetaTH);
    if(!readMed_AlphaTH.isEmpty()) ui->deltaME_A->setText(readMed_AlphaTH);
    if(!readMed_SigmaTH.isEmpty()) ui->deltaME_S->setText(readMed_SigmaTH);
    if(!readMed_BetaTH.isEmpty()) ui->deltaME_B->setText(readMed_BetaTH);
    if(!readMed_G1TH.isEmpty()) ui->deltaME_G->setText(readMed_G1TH);
}


ProfileDialog::~ProfileDialog()
{
    delete ui;
}


void ProfileDialog::on_buttonBox_accepted()
{
    QString writeName, writeDOB, writeSex, writeAlpha, writeSigma;
    QString writeSleep_DeltaTH2, writeSleep_DeltaTH1, writeSleep_ThetaTH, writeSleep_SigmaTH, writeSleep_BetaTH, writeSleep_G1TH;
    QString writeMed_DeltaTH2, writeMed_ThetaTH, writeMed_AlphaTH, writeMed_SigmaTH, writeMed_BetaTH, writeMed_G1TH;

    writeName = ui->lpidnameLE->text();
    writeDOB = ui->lpiddobLE->text();
    writeSex = ui->lpidsexLE->text();
    writeAlpha = ui->alphaLE->text();
    writeSigma = ui->sigmaLE->text();
    writeSleep_DeltaTH2 = ui->deltaSE_D2->text();
    writeSleep_DeltaTH1 = ui->deltaSE_D1->text();
    writeSleep_ThetaTH = ui->deltaSE_T->text();
    writeSleep_SigmaTH = ui->deltaSE_S->text();
    writeSleep_BetaTH = ui->deltaSE_B->text();
    writeSleep_G1TH = ui->deltaSE_G->text();
    writeMed_DeltaTH2 = ui->deltaME_D2->text();
    writeMed_ThetaTH = ui->deltaME_T->text();
    writeMed_AlphaTH = ui->deltaME_A->text();
    writeMed_SigmaTH = ui->deltaME_S->text();
    writeMed_BetaTH = ui->deltaME_B->text();
    writeMed_G1TH = ui->deltaME_G->text();

    // Test, correct, and write out variables
    if(!writeName.isEmpty())
    {
        writeName.replace(QRegExp("\\s+"),QChar('_'));   // replace whitespace with '_'
        writeConfigValue("Name", writeName);
    }
    if(QString::compare(writeDOB, QString("01-JAN-1900")))
        writeConfigValue("DOB", writeDOB);
    if(!writeSex.isEmpty())
        writeConfigValue("Sex", writeSex);
    writeConfigValue("Alpha", writeAlpha);
    writeConfigValue("Sigma", writeSigma);
    writeConfigValue("Sleep_DeltaTH2", writeSleep_DeltaTH2);
    writeConfigValue("Sleep_DeltaTH1", writeSleep_DeltaTH1);
    writeConfigValue("Sleep_ThetaTH", writeSleep_ThetaTH);
    writeConfigValue("Sleep_SigmaTH", writeSleep_SigmaTH);
    writeConfigValue("Sleep_BetaTH", writeSleep_BetaTH);
    writeConfigValue("Sleep_G1TH", writeSleep_G1TH);
    writeConfigValue("Med_DeltaTH2", writeMed_DeltaTH2);
    writeConfigValue("Med_ThetaTH", writeMed_ThetaTH);
    writeConfigValue("Med_AlphaTH", writeMed_AlphaTH);
    writeConfigValue("Med_SigmaTH", writeMed_SigmaTH);
    writeConfigValue("Med_BetaTH", writeMed_BetaTH);
    writeConfigValue("Med_G1TH", writeMed_G1TH);
}
