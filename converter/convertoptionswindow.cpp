#include "convertoptionswindow.h"
#include "ui_convertoptionswindow.h"
#include "opi_win.h"
#include "opi_helper.h"
#include "profile/profiledialog.h"


//generic
ConvertOptionsWindow::ConvertOptionsWindow(QString *outDirp, quint8 *pdnListp, qint8 *outDataTypep, float *alphap,
                                           float *sigmap, QVector<qint64> **tsQVpp,
                                           QVector<quint8> **skpQVpp, QVector<quint8> **batQVpp,
                                           QVector<qint16> **adcQVpp, QVector<qint16> **tmpQVpp,
                                           QVector<qint16> **axQVpp, QVector<qint16> **ayQVpp,
                                           QVector<qint16> **azQVpp, QVector<qint16> **sqQVpp,
                                           QVector<quint8> **edQVpp,
                                           QVector<qint64> *annOnsetTSQVp,
                                           QVector<QString> *annTextQVp, qint8 convTypeDes,
                                           bool *useProfileFlagp, QWidget *parent, bool *doRRfile) :
    QDialog(parent),
    ui(new Ui::ConvertOptionsWindow)
{
    qint32 i;
    QString tempstr, readAlpha, readSigma;

    ui->setupUi(this);

    doRRfilep=doRRfile;
    ui->checkBox_RRfile->setVisible(false);
    if(doRRfilep != 0)
        (*doRRfilep)=ui->checkBox_RRfile->isChecked();

    ui->checkReverse->setVisible(true);
    hide_eeg_userinput();

    // get first and last frame, guaranteed to work since incoming data is not empty
    getfirstlastfrmTS(tsQVpp, &myfirstFrmTS, &mylastFrmTS);

    // save pointers for modification when user is done
    myoutDataTypep = outDataTypep;
    mypdnListp = pdnListp;
    mytsQVpp = tsQVpp;
    myskpQVpp = skpQVpp;
    mybatQVpp = batQVpp;
    myadcQVpp = adcQVpp;
    mytmpQVpp = tmpQVpp;
    myaxQVpp = axQVpp;
    myayQVpp = ayQVpp;
    myazQVpp = azQVpp;
    mysqQVpp = sqQVpp;
    myedQVpp = edQVpp;
    myannOnsetTSQVp = annOnsetTSQVp;
    myannTextQVp = annTextQVp;
    mysigmap = sigmap;
    myalphap = alphap;
    myoutDirp = outDirp;
    ui->outDirLE->setText(QDir::currentPath().append(QDir::separator()));

    readSigma = getConfigValue("Sigma");
    readAlpha = getConfigValue("Alpha");
    if(!readSigma.isEmpty())
        *mysigmap = readSigma.toFloat();
    if(!readAlpha.isEmpty())
        *myalphap = readAlpha.toFloat();

    tagProcessed = false; // indicates tag file has not been processed

    tempstr.clear();
    ui->SigmatextEdit->setText(tempstr.setNum(*mysigmap));
    tempstr.clear();
    ui->alphatextEdit->setText(tempstr.setNum(*myalphap));
    QDateTime firstFrmDT, lastFrmDT;
    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(myfirstFrmTS*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(firstFrmDT.toMSecsSinceEpoch());

    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(mylastFrmTS*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(lastFrmDT.toMSecsSinceEpoch());

    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);

    // setup the combo box that allows selection of which pdn data
    ui->ViewercomboBox->clear();
    pdnSlotWant=0;
    for(i = 0; i < PDNLISTLEN; i++)  //check the index
    {
        if(myadcQVpp[i] != 0)
            ui->ViewercomboBox->addItem(QString("%1").arg(pdnListp[i]));
    }

    convType=convTypeDes;
    if(convType==EDFDTOWAV || convType==OPITOWAV)
    {
        ui->tagFileLA->setVisible(false);
        ui->tagFileLE->setVisible(false);
        ui->tagFileBrwsPB->setVisible(false);
    }

    myuseProfileFlagp = useProfileFlagp;
}


//EEG
ConvertOptionsWindow::ConvertOptionsWindow(QString *outDirp, float *alphap, float *sigmap, quint8 *pdnListp, qint8 *outDataTypep,
                                           QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                                           QVector<quint8> **batQVpp, QVector<quint8> **edQVpp,
                                           QVector<qint16> **M2QVpp, QVector<qint16> **M1QVpp,
                                           QVector<qint16> **G2QVpp, QVector<qint16> **G1QVpp,
                                           QVector<qint16> **UPQVpp, QVector<qint16> **BetaQVpp,
                                           QVector<qint16> **SigmaQVpp, QVector<qint16> **alphaQVpp,
                                           QVector<qint16> **thetaQVpp, QVector<qint16> **deltaQVpp,
                                           QVector<qint64> *annOnsetTSQVp, QVector<QString> *annTextQVp,
                                           qint8 convTypeDes, bool *useProfileFlagp,
                                           QWidget *parent):
    QDialog(parent),
    ui(new Ui::ConvertOptionsWindow)
{

    qint32 i;
    QString tempstr;
    ui->setupUi(this);
    ui->checkBox_RRfile->setVisible(false);
    ui->checkReverse->setVisible(false);
    hide_eeg_userinput();

    // get first and last frame, guaranteed to work since incoming data is not empty
    getfirstlastfrmTS(tsQVpp, &myfirstFrmTS, &mylastFrmTS);

    // save pointers for modification when user is done
    myoutDataTypep = outDataTypep;
    mypdnListp = pdnListp;
    mytsQVpp = tsQVpp;
    myskpQVpp = skpQVpp;
    mybatQVpp = batQVpp;
    myedQVpp = edQVpp;

    myM2QVpp = M2QVpp;
    myM1QVpp = M1QVpp;
    myG2QVpp = G2QVpp;
    myG1QVpp = G1QVpp;
    myUPQVpp = UPQVpp;
    myBetaQVpp = BetaQVpp;
    mySigmaQVpp = SigmaQVpp;
    myalphaQVpp = alphaQVpp;
    mythetaQVpp = thetaQVpp;
    mydeltaQVpp = deltaQVpp;

    myannOnsetTSQVp = annOnsetTSQVp;
    myannTextQVp = annTextQVp;
    mysigmap = sigmap;
    myalphap = alphap;
    myoutDirp = outDirp;
    ui->outDirLE->setText(QDir::currentPath().append(QDir::separator()));

    tagProcessed = false; // indicates tag file has not been processed

    tempstr.clear();
    ui->SigmatextEdit->setText(tempstr.setNum(*mysigmap));
    tempstr.clear();
    ui->alphatextEdit->setText(tempstr.setNum(*myalphap));
    QDateTime firstFrmDT, lastFrmDT;
    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(myfirstFrmTS*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);

    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(mylastFrmTS*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);

    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);

    // setup the combo box that allows selection of which pdn data
    ui->ViewercomboBox->clear();
    pdnSlotWant=0;

    for(i = 0; i < PDNLISTLEN; i++)  //check the index
    {
        if(myG1QVpp[i] != 0)
            ui->ViewercomboBox->addItem(QString("%1").arg(pdnListp[i]));
    }

    convType=convTypeDes;

    if(convType==EDFMEEGTOWAV)
    {
        ui->tagFileLA->setVisible(false);
        ui->tagFileLE->setVisible(false);
        ui->tagFileBrwsPB->setVisible(false);
    }
    ui->alphalabel->setVisible(false);
    ui->alphalabelHz->setVisible(false);
    ui->alphatextEdit->setVisible(false);
    ui->Sigmalabel->setVisible(false);
    ui->SigmalabelHz->setVisible(false);
    ui->SigmatextEdit->setVisible(false);
    ui->dataTypeLA->setVisible(false);
    ui->dataTypeValCB->setVisible(false);
    ui->tmpAvgCB->setVisible(false);

    myuseProfileFlagp = useProfileFlagp;

}


//ECG
ConvertOptionsWindow::ConvertOptionsWindow(QString *outDirp, quint8 *pdnListp, float *alphap, float *sigmap, qint8 *outDataTypep,
                                           QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                                           QVector<quint8> **batQVpp, QVector<quint8> **edQVpp,
                                           QVector<qint16> **RRQVpp, QVector<qint16> **ampQVpp,
                                           QVector<qint16> **LFperQVpp, QVector<qint16> **HFperQVpp,
                                           QVector<qint16> **LHRatioQVpp,
                                           QVector<qint64> *annOnsetTSQVp, QVector<QString> *annTextQVp,
                                           qint8 convTypeDes, bool *useProfileFlagp,
                                           QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConvertOptionsWindow)
{
    qint32 i;
    QString tempstr;
    ui->setupUi(this);
    ui->checkBox_RRfile->setVisible(false);
    ui->checkReverse->setVisible(false);
    hide_eeg_userinput();

    // get first and last frame, guaranteed to work since incoming data is not empty
    getfirstlastfrmTS(tsQVpp, &myfirstFrmTS, &mylastFrmTS);

    // save pointers for modification when user is done
    myoutDataTypep = outDataTypep;
    mypdnListp = pdnListp;
    mytsQVpp = tsQVpp;
    myskpQVpp = skpQVpp;
    mybatQVpp = batQVpp;
    myedQVpp = edQVpp;
    //qDebug()<<mytsQVpp[0]->size()/8;
    myRRQVpp = RRQVpp;
    myampQVpp = ampQVpp;
    myLFperQVpp = LFperQVpp; //SDNN*10
    myHFperQVpp = HFperQVpp; //log HFpower
    myLHRatioQVpp = LHRatioQVpp; //log LHRatio

    myannOnsetTSQVp = annOnsetTSQVp;
    myannTextQVp = annTextQVp;
    mysigmap = sigmap;
    myalphap = alphap;
    myoutDirp = outDirp;
    ui->outDirLE->setText(QDir::currentPath().append(QDir::separator()));

    tagProcessed = false; // indicates tag file has not been processed

    tempstr.clear();
    ui->SigmatextEdit->setText(tempstr.setNum(*mysigmap));
    tempstr.clear();
    ui->alphatextEdit->setText(tempstr.setNum(*myalphap));
    QDateTime firstFrmDT, lastFrmDT;
    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(myfirstFrmTS*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);

    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(mylastFrmTS*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);

    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);

    // setup the combo box that allows selection of which pdn data
    ui->ViewercomboBox->clear();
    pdnSlotWant=0;

    for(i = 0; i < PDNLISTLEN; i++)  //check the index
    {
        if(mytsQVpp[i] != 0)
            ui->ViewercomboBox->addItem(QString("%1").arg(pdnListp[i]));
    }

    convType=convTypeDes;

    if(convType==EDFMECGTOWAV)
    {
        ui->tagFileLA->setVisible(false);
        ui->tagFileLE->setVisible(false);
        ui->tagFileBrwsPB->setVisible(false);
    }
    ui->alphalabel->setVisible(false);
    ui->alphalabelHz->setVisible(false);
    ui->alphatextEdit->setVisible(false);
    ui->Sigmalabel->setVisible(false);
    ui->SigmalabelHz->setVisible(false);
    ui->SigmatextEdit->setVisible(false);
    ui->dataTypeLA->setVisible(false);
    ui->dataTypeValCB->setVisible(false);
    ui->tmpAvgCB->setVisible(false);

    myuseProfileFlagp = useProfileFlagp;

}


ConvertOptionsWindow::~ConvertOptionsWindow()
{
    delete ui;
}


void ConvertOptionsWindow::hide_eeg_userinput()
{
    ui->alphalabel->setVisible(false);
    ui->alphatextEdit->setVisible(false);
    ui->Sigmalabel->setVisible(false);
    ui->SigmatextEdit->setVisible(false);
    ui->alphalabelHz->setVisible(false);
    ui->SigmalabelHz->setVisible(false);
}


void ConvertOptionsWindow::show_eeg_userinput()
{
    ui->alphalabel->setVisible(true);
    ui->alphatextEdit->setVisible(true);
    ui->Sigmalabel->setVisible(true);
    ui->SigmatextEdit->setVisible(true);
    ui->alphalabelHz->setVisible(true);
    ui->SigmalabelHz->setVisible(true);
}


void ConvertOptionsWindow::on_buttonBox_accepted()
{
    qint64 refTS;
    int alphaIntTemp, SigmaIntTemp;
    QString outFileName,datetimestr;
    QDateTime firstFrmDT;
    int pdnSlot, i;

    refTS = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch();

    // update start and end
    myfirstFrmTS = (ui->stDTE->dateTime().toMSecsSinceEpoch() - refTS)*UCRTCFREQ/1000;
    mylastFrmTS = (ui->endDTE->dateTime().toMSecsSinceEpoch() - refTS)*UCRTCFREQ/1000;
    ui->stDTE->setDateTimeRange(ui->stDTE->dateTime(), ui->endDTE->dateTime());
    ui->endDTE->setDateTimeRange(ui->stDTE->dateTime(), ui->endDTE->dateTime());
    *myoutDataTypep = ui->dataTypeValCB->currentIndex();
    *myoutDirp = ui->outDirLE->text();

    if(convType<5)  //only generic to EDF or WAV need this
    {
        //alpha
        //remainder
        alphaIntTemp = (int)(ui->alphatextEdit->toPlainText().toFloat()*EEGCONVERT05ACCURACY)%EEGCONVERT05ACCURACY;
        //left decimal point
        *myalphap = (int)(ui->alphatextEdit->toPlainText().toFloat()*EEGCONVERT05ACCURACY)/EEGCONVERT05ACCURACY;
        //choose
        if(alphaIntTemp>=75) (*myalphap) += 1;
        else if(alphaIntTemp<75&&alphaIntTemp>=25) (*myalphap) += 0.5;

        if(((*myalphap)<0) || ((*myalphap)>alpha_MAX))
        {
            *myalphap = alpha_Default;
        }
        //Sigma
        //remainder
        SigmaIntTemp = (int)(ui->SigmatextEdit->toPlainText().toFloat()*EEGCONVERT05ACCURACY)%EEGCONVERT05ACCURACY;
        //left decimal point
        *mysigmap = (int)(ui->SigmatextEdit->toPlainText().toFloat()*EEGCONVERT05ACCURACY)/EEGCONVERT05ACCURACY;
        //choose
        if(SigmaIntTemp>=75) (*mysigmap) += 1;
        else if(SigmaIntTemp<75&&SigmaIntTemp>=25) (*mysigmap) += 0.5;
        if(((*mysigmap)<0) || ((*mysigmap)>Sigma_MAX))
        {
            *mysigmap = Sigma_Default;
        }
        // always do conversion, filters data for time range and fills in
        procQVs(&myfirstFrmTS, &mylastFrmTS, mytsQVpp, myskpQVpp, mybatQVpp, myadcQVpp, mytmpQVpp, myaxQVpp, myayQVpp, myazQVpp, mysqQVpp, myedQVpp);

        if(ui->tmpAvgCB->isChecked())
            avgtmpQVs(mytmpQVpp);

        //reverse the adc signal
        if(ui->checkReverse->isChecked())
        {
            for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
            {
                if(mytsQVpp[pdnSlot] != 0)
                {
                    if(mytsQVpp[pdnSlot]->size() > 2)
                    {
                        for(i=0;i<myadcQVpp[pdnSlot]->size();i++)
                            myadcQVpp[pdnSlot]->replace(i,-1*myadcQVpp[pdnSlot]->at(i));
                    }
                }
            }
        }
    }
    else if(convType<7)  //only ECG to EDF or WAV need it
    {
        procMecgQVs(&myfirstFrmTS, &mylastFrmTS, mytsQVpp[pdnSlotWant], myskpQVpp[pdnSlotWant], mybatQVpp[pdnSlotWant],
                    myRRQVpp[pdnSlotWant],myampQVpp[pdnSlotWant],myLFperQVpp[pdnSlotWant],myHFperQVpp[pdnSlotWant], myLHRatioQVpp[pdnSlotWant]);
    }
    else if(convType<9)  //only EEG to EDF or WAV need it
    {
        procMeegQVs(&myfirstFrmTS, &mylastFrmTS, mytsQVpp[pdnSlotWant],
                    myskpQVpp[pdnSlotWant], mybatQVpp[pdnSlotWant],
                    myM2QVpp[pdnSlotWant],myM1QVpp[pdnSlotWant],
                    myG2QVpp[pdnSlotWant],myG1QVpp[pdnSlotWant],
                    myUPQVpp[pdnSlotWant],myBetaQVpp[pdnSlotWant],
                    mySigmaQVpp[pdnSlotWant],myalphaQVpp[pdnSlotWant],
                    mythetaQVpp[pdnSlotWant],mydeltaQVpp[pdnSlotWant]);
    }

    if(!tagProcessed)   // if data has been processed already (e.g. for viewer, then skip)
    {
        // tag stuff
        if(!ui->tagFileLE->toPlainText().isEmpty())
        {
            tagFileRead(ui->tagFileLE->toPlainText(), myannOnsetTSQVp, myannTextQVp); // read in tags

        }
        ui->tagFileLA->setEnabled(false);
        ui->tagFileLE->setEnabled(false);
        ui->tagFileBrwsPB->setEnabled(false);
        tagProcessed = true;
    }

    // write tag text file
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((mytsQVpp[pdnSlotWant]->at(0))*1000/UCRTCFREQ);
    datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");
    if(convType<5)
        outFileName.append(QString("D%1_%2_generic_TAG.txt").arg(datetimestr).arg(mypdnListp[pdnSlotWant]));
    else if(convType<7)
        outFileName.append(QString("D%1_%2_ECG_TAG.txt").arg(datetimestr).arg(mypdnListp[pdnSlotWant]));
    else if(convType<9)
        outFileName.append(QString("D%1_%2_EEG_TAG.txt").arg(datetimestr).arg(mypdnListp[pdnSlotWant]));
    procTagQVs(&myfirstFrmTS, &mylastFrmTS, outFileName, myannOnsetTSQVp, myannTextQVp);

    *myuseProfileFlagp = ui->useProfileCB->isChecked();
}


void ConvertOptionsWindow::on_dataTypeValCB_currentIndexChanged(int index)
{
    if(index==EEGINDEX) this->show_eeg_userinput();
    else this->hide_eeg_userinput();

    if((convType<5))
    {
       if((index==ECGINDEX))
       ui->checkBox_RRfile->setVisible(true);
       else
       ui->checkBox_RRfile->setVisible(false);
    }

}


/***
  * First and last valid timestamp in input stream. Assumes input stream starts
  * from beginning of file. Assumes there is a non-null pointer with data.
  *	Inputs:
  *		 instrp, pointer to QDataStream of input file
  *      firstFrmTS, ptr to variable that will be modified to first frame timestamp
  *      lastFrmTS, ptr to variable that will be modified to last frame timestamp
  *	Returns:
  *     nothing
  */
void ConvertOptionsWindow::getfirstlastfrmTS(QVector<qint64> **tsQVpp, qint64 *firstFrmTSp, qint64 *lastFrmTSp)
{
    qint32 i;

    // get first time
    for(i = 0; i < PDNLISTLEN; i++)
    {
        if(tsQVpp[i] != 0)  // has data
        {
            if(tsQVpp[i]->isEmpty()) continue; // shouldn't happen
            *firstFrmTSp = tsQVpp[i]->at(0);
            *lastFrmTSp = tsQVpp[i]->at(tsQVpp[i]->size()-1);
            break;
        }
    }

    // check against the rest
    for(; i < PDNLISTLEN; i++)
    {
        if(tsQVpp[i] != 0)  // has data
        {
            if(tsQVpp[i]->isEmpty() || (tsQVpp[i]->size() < 2)) continue; // shouldn't happen
            if(tsQVpp[i]->at(0) < *firstFrmTSp)
                *firstFrmTSp = tsQVpp[i]->at(0);
            if(tsQVpp[i]->at(tsQVpp[i]->size()-1) > *lastFrmTSp)
                *lastFrmTSp = tsQVpp[i]->at(tsQVpp[i]->size()-1);
            break;
        }
    }
}


/***
  *
  * Reprocess the temperature qvectors so that data is average of 8 samples
  * Sample number is still the same.
  *
  */
void ConvertOptionsWindow::avgtmpQVs(QVector<qint16> **tmpQVpp)
{
    qint32 i, j, pdnSlot;
    qint16 avgTmp;

    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        if(tmpQVpp[pdnSlot] == 0) continue; // empty
        for(i = 0; i < (tmpQVpp[pdnSlot]->size()-1)-TMPAVGFACTOR; i += TMPAVGFACTOR)
        {
            avgTmp = 0;
            for(j = 0; j < TMPAVGFACTOR; j++)
                avgTmp += tmpQVpp[pdnSlot]->at(i+j);
            avgTmp /= TMPAVGFACTOR;
            for(j = 0; j < TMPAVGFACTOR; j++)
                tmpQVpp[pdnSlot]->replace(i+j, avgTmp);
        }
    }
}


void ConvertOptionsWindow::on_endDTE_dateTimeChanged(const QDateTime &date)
{
    if(ui->endDTE->dateTime() < ui->stDTE->dateTime())
        ui->endDTE->setDateTime(ui->stDTE->dateTime());
}


void ConvertOptionsWindow::on_stDTE_dateTimeChanged(const QDateTime &date)
{
    if(ui->endDTE->dateTime() < ui->stDTE->dateTime())
        ui->stDTE->setDateTime(ui->stDTE->dateTime());
}



void ConvertOptionsWindow::on_ViewercomboBox_currentIndexChanged(int index)
{
    qint32 i;
    // have to search for matching PDN
    for(i = 0; i < PDNLISTLEN; i++)
    {
        if(ui->ViewercomboBox->currentText().toInt() == mypdnListp[i])
        {
            pdnSlotWant = i;
            break;
        }
    }
}


void ConvertOptionsWindow::on_tagFileBrwsPB_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(),
                       "Text File( *.txt ) ;; All Files( *.* )");

    if (!fileName.isEmpty())
    {
        ui->tagFileLE->setText(fileName);
    }
}


void ConvertOptionsWindow::on_viewPB_released()
{
    qint64 refTS;
    QDateTime firstFrmDT, lastFrmDT;

    refTS = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch();
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    // update start and end
    myfirstFrmTS = (ui->stDTE->dateTime().toMSecsSinceEpoch() - refTS)*UCRTCFREQ/1000;
    mylastFrmTS = (ui->endDTE->dateTime().toMSecsSinceEpoch() - refTS)*UCRTCFREQ/1000;

    *myoutDataTypep = ui->dataTypeValCB->currentIndex();

    // always do conversion, filter data according to time
    if(convType<5)  //only generic to EDF or WAV need it
    {
        procQVs(&myfirstFrmTS, &mylastFrmTS, mytsQVpp, myskpQVpp, mybatQVpp, myadcQVpp, mytmpQVpp, myaxQVpp, myayQVpp, myazQVpp, mysqQVpp, myedQVpp);
        if(ui->tmpAvgCB->isChecked())
            avgtmpQVs(mytmpQVpp);
    }
    else if(convType<7)  //only ECG to EDF or WAV need it
    {
        procMecgQVs(&myfirstFrmTS, &mylastFrmTS, mytsQVpp[pdnSlotWant], myskpQVpp[pdnSlotWant], mybatQVpp[pdnSlotWant],
                    myRRQVpp[pdnSlotWant],myampQVpp[pdnSlotWant],myLFperQVpp[pdnSlotWant],myHFperQVpp[pdnSlotWant], myLHRatioQVpp[pdnSlotWant]);
    }
    else if(convType<9)  //only EEG to EDF or WAV need it
    {
        procMeegQVs(&myfirstFrmTS, &mylastFrmTS, mytsQVpp[pdnSlotWant],
                    myskpQVpp[pdnSlotWant], mybatQVpp[pdnSlotWant],
                    myM2QVpp[pdnSlotWant],myM1QVpp[pdnSlotWant],
                    myG2QVpp[pdnSlotWant],myG1QVpp[pdnSlotWant],
                    myUPQVpp[pdnSlotWant],myBetaQVpp[pdnSlotWant],
                    mySigmaQVpp[pdnSlotWant],myalphaQVpp[pdnSlotWant],
                    mythetaQVpp[pdnSlotWant],mydeltaQVpp[pdnSlotWant]);
    }
    if(!tagProcessed)   // if data has been processed already (e.g. for viewer, then skip)
    {
        // tag stuff
        if(!ui->tagFileLE->toPlainText().isEmpty())
        {
            tagFileRead(ui->tagFileLE->toPlainText(), myannOnsetTSQVp, myannTextQVp); // read in tags
        }
        ui->tagFileLA->setEnabled(false);
        ui->tagFileLE->setEnabled(false);
        ui->tagFileBrwsPB->setEnabled(false);
        tagProcessed = true;
    }
    // put fileviewer code here
    // data is in <mytsQVpp, myskpQVpp, mybatQVpp, myadcQVpp, mytmpQVpp, myaxQVpp, myayQVpp, myazQVpp, mysqQVpp, myedQVpp>
    // tags are in <myannOnsetTSQVpp, myannTextQVpp>
    // pdn's are in mypdnListp
    qApp->processEvents();
    if(convType<5) //only generic to EDF or WAV need it
    {
        signalviewerpointer = new signalviewer(myadcQVpp[pdnSlotWant],myaxQVpp[pdnSlotWant],myayQVpp[pdnSlotWant],myazQVpp[pdnSlotWant],mytmpQVpp[pdnSlotWant],mytsQVpp[pdnSlotWant],&myfirstFrmTS,&mylastFrmTS,
                                               myannOnsetTSQVp,myannTextQVp,mypdnListp[pdnSlotWant],convType,this);
    }
    else if(convType<7) //ECG
    {
        signalviewerpointer = new signalviewer(1,0.9,myRRQVpp[pdnSlotWant],myampQVpp[pdnSlotWant],myLFperQVpp[pdnSlotWant],myHFperQVpp[pdnSlotWant], myLHRatioQVpp[pdnSlotWant],mytsQVpp[pdnSlotWant],&myfirstFrmTS,&mylastFrmTS,
                                               myannOnsetTSQVp,myannTextQVp,mypdnListp[pdnSlotWant],convType,this);
    }
    else if(convType<9) //EEG
    {
        signalviewerpointer = new signalviewer(0.9,myM2QVpp[pdnSlotWant],myM1QVpp[pdnSlotWant],
                                               myG2QVpp[pdnSlotWant],myG1QVpp[pdnSlotWant],
                                               myUPQVpp[pdnSlotWant],myBetaQVpp[pdnSlotWant],
                                               mySigmaQVpp[pdnSlotWant],myalphaQVpp[pdnSlotWant],
                                               mythetaQVpp[pdnSlotWant],mydeltaQVpp[pdnSlotWant],
                                               mytsQVpp[pdnSlotWant],&myfirstFrmTS,&mylastFrmTS,
                                               myannOnsetTSQVp,myannTextQVp,mypdnListp[pdnSlotWant],convType,this);
    }

    signalviewerpointer->tagcontrol(true);

    while(!signalviewerpointer->isEnabled());
    signalviewerpointer->exec();

    if(convType<5) //only generic to EDF or WAV need it
        ui->checkReverse->setChecked(signalviewerpointer->reversechecked);

    signalviewerpointer->deleteLater();

    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(myfirstFrmTS*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);

    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(mylastFrmTS*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);

    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);
    return ;
}


void ConvertOptionsWindow::on_editProfilePB_clicked()
{
    ProfileDialog profD;
    profD.exec();
}


void ConvertOptionsWindow::on_outDirBrwsPB_clicked()
{
    QString dirName = QFileDialog::getExistingDirectory(this, tr("Out Directory"), QDir::currentPath());

    if(!dirName.isEmpty())
        ui->outDirLE->setText(dirName.append(QDir::separator()));
}


void ConvertOptionsWindow::sleepanalysis_uiconfig()
{
    ui->dataTypeLA->setVisible(false);
    ui->dataTypeValCB->setVisible(false);
    ui->tmpAvgCB->setVisible(false);
    ui->checkBox_RRfile->setVisible(true);
    ui->checkBox_RRfile->setText("Output Sum/Epoch files");
}


void ConvertOptionsWindow::on_checkBox_RRfile_toggled(bool checked)
{
    (*doRRfilep)=checked;
}
