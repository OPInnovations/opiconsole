#include "convertwindow.h"
#include "ui_convertwindow.h"
#include "opi_osx.h"
#include "opi_helper.h"
#include "convertoptionswindow.h"


/***
  * Constructor
  */
ConvertWindow::ConvertWindow(QWidget *parent) :
   // QMainWindow(parent),
    ui(new Ui::ConvertWindow)
{
    ui->setupUi(this);
    Sigma = Sigma_Default;
    alpha = alpha_Default;
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    //set window icon
    this->setWindowIcon(QIcon("../images/opi_ico.ico"));
}


/***
  * Destructor
  */
ConvertWindow::~ConvertWindow()
{
    delete ui;
}


/***
  * When the input file Browse button is clicked, allow user to choose file
  */
void ConvertWindow::on_inBrwsPB_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QDir::currentPath(),
                       "Supported Files( *.edf  *.opi ) ;; EDF File( *.edf ) ;; OPI File( *.opi ) ;; All Files( *.* )");

    if (!fileName.isEmpty())
    {
        ui->inLE->setText(fileName);
    }
}


/***
  * When the convert button is clicked, disable all inputs and try to convert and then
  * reenable inputs
  */
void ConvertWindow::on_cnvtPB_clicked()
{
    qint32 inType;

    // Disable inputs
    ui->cnvtPB->setEnabled(false);
    ui->inBrwsPB->setEnabled(false);
    ui->inLE->setEnabled(false);
    ui->outTypCBox->setEnabled(false);
    qApp->processEvents();      // make sure display gets updated

    // first determine type of input file
    inType = whatInType(ui->inLE->text());

    if(inType < 0)  // input file error
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file %1").arg(ui->inLE->text()));
    }
    else if(inType == 0)  // unrecognized file
    {
        ui->msgPTE->appendPlainText("Unrecognized input file type");
    }
    else if(inType == 1) // opi type
    {
        if(ui->outTypCBox->currentIndex() == 0)         // EDF+ output
        {
            ui->msgPTE->appendPlainText("Converting OPI to EDF+... ");
            qApp->processEvents();      // make sure display gets updated
            convertopitoedf();
        }
        else if(ui->outTypCBox->currentIndex() == 1)    // WAV output
        {
            ui->msgPTE->appendPlainText("Converting OPI to WAV... ");
            qApp->processEvents();      // make sure display gets updated
            convertopitowav();
        }
    }
    else if(inType == 2) // generic edf opi
    {
        if(ui->outTypCBox->currentIndex() == 0)         // EDF+ output
        {
            ui->msgPTE->appendPlainText("Converting EDF+ to EDF+... ");
            qApp->processEvents();      // make sure display gets updated
            convertedfDtoedf();
        }
        else if(ui->outTypCBox->currentIndex() == 1)    // WAV output
        {
            ui->msgPTE->appendPlainText("Converting EDF+ to WAV... ");
            qApp->processEvents();      // make sure display gets updated
            convertedfDtowav();
        }
    }
    else if(inType == 3) // eeg edf opi
    {
        if(ui->outTypCBox->currentIndex() == 0)         // EDF+ output
        {
            ui->msgPTE->appendPlainText("Converting EEG EDF+ to EEG EDF+... ");
            qApp->processEvents();      // make sure display gets updated
            convertedfMeegtoedf();
        }
        else if(ui->outTypCBox->currentIndex() == 1)    // WAV output
        {
            ui->msgPTE->appendPlainText("Converting EEG EDF+ to EEG WAV... ");
            qApp->processEvents();      // make sure display gets updated
            convertedfMeegtowav();
        }
    }
    else if(inType == 4) // ecg edf opi
    {
        if(ui->outTypCBox->currentIndex() == 0)         // EDF+ output
        {
            ui->msgPTE->appendPlainText("Converting ECG EDF+ to ECG EDF+... ");
            qApp->processEvents();      // make sure display gets updated
            convertedfMecgtoedf();
        }
        else if(ui->outTypCBox->currentIndex() == 1)    // WAV output
        {
            ui->msgPTE->appendPlainText("Converting ECG EDF+ to ECG WAV... ");
            qApp->processEvents();      // make sure display gets updated
            convertedfMecgtowav();
        }
    }

    // Enable inputs
    ui->cnvtPB->setEnabled(true);
    ui->inBrwsPB->setEnabled(true);
    ui->inLE->setEnabled(true);
    ui->outTypCBox->setEnabled(true);
}


bool ConvertWindow::convertedfMecgtoedf()
{
    QString outFileName, outDirName;
    QFile *infilep, *outfile1p;
    QDataStream *instrp;
    QDataStream *out1strp;
    QString datetimestr, tempqstr;
    QDateTime firstFrmDT, stDT, endDT;
    QVector<qint16> *RRQVpp[PDNLISTLEN];
    QVector<qint16> *ampQVpp[PDNLISTLEN];
    QVector<qint16> *LFperQVpp[PDNLISTLEN];//SDNN
    QVector<qint16> *HFperQVpp[PDNLISTLEN]; //log HFpower
    QVector<qint16> *LHRatioQVpp[PDNLISTLEN]; //log
    QVector<qint64> *tsQVpp[PDNLISTLEN];
    QVector<quint8> *skpQVpp[PDNLISTLEN];
    QVector<quint8> *batQVpp[PDNLISTLEN];
    QVector<quint8> *edQVpp[PDNLISTLEN];
    QVector<qint64> annOnsetTSQV;
    QVector<QString> annTextQV;
    ConvertOptionsWindow *coWinp;
    QString lpid, lrid, localPatientID, localRecordID;
    QDateTime startDT;
    qint32 numDataRecs, dataRecDur;
    qint32 numSignals;
    QVector<QString> labelSignalsQV, transTypeQV, physDimQV, prefiltQV;
    QVector<qint32> physMinQV, physMaxQV, digMinQV, digMaxQV;
    QVector<qint32> sampsPerDRQV;
    quint8 pdnListp[PDNLISTLEN];
    qint32 pdnSlot;
    qint32 pdnDes;
    qint8 outDataType;
    qint32 dataRecordCt;
    bool useProfile;
    QString readName, readDOB, readSex;
    QMessageBox msgBox;
    QStringList tempQSL;
    QString stDateQS;
    qint32 currMonth;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(ui->inLE->text());
    if(!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(ui->inLE->text()));
        delete infilep;
        return false;
    }

    // open input file stream
    instrp = new QDataStream(infilep);
    instrp->setByteOrder(QDataStream::LittleEndian);

    // get header information
    edfhdrread(instrp, &lpid, &lrid, &startDT, &numDataRecs, &dataRecDur,
               &numSignals, &labelSignalsQV, &transTypeQV, &physDimQV, &physMinQV,
               &physMaxQV, &digMinQV, &digMaxQV, &prefiltQV, &sampsPerDRQV);
    dataRecordCt = numDataRecs;

    // init qvector pointers to null
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        RRQVpp[pdnSlot] = 0;
        ampQVpp[pdnSlot] = 0;
        LFperQVpp[pdnSlot] = 0;
        HFperQVpp[pdnSlot] = 0;
        LHRatioQVpp[pdnSlot] = 0;
        tsQVpp[pdnSlot] = 0;
        skpQVpp[pdnSlot] = 0;
        batQVpp[pdnSlot] = 0;
        edQVpp[pdnSlot] = 0;
    }

    // get PDN from Device field
    pdnDes = getPDNlrid(lrid);
    pdnSlot = 0;

    // copy pdn's
    for(int i = 0; i < PDNLISTLEN; i++)
        pdnListp[i] = 0xFF;
    pdnListp[pdnSlot] = pdnDes;

    // initialize qvectors
    RRQVpp[pdnSlot] = new QVector<qint16>(0);
    ampQVpp[pdnSlot] = new QVector<qint16>(0);
    LFperQVpp[pdnSlot] = new QVector<qint16>(0);
    HFperQVpp[pdnSlot] = new QVector<qint16>(0);
    LHRatioQVpp[pdnSlot] = new QVector<qint16>(0);
    tsQVpp[pdnSlot] = new QVector<qint64>(0);
    skpQVpp[pdnSlot] = new QVector<quint8>(0);
    batQVpp[pdnSlot] = new QVector<quint8>(0);
    edQVpp[pdnSlot] = new QVector<quint8>(0);

    dataRecordCt = edfMecgread(instrp, startDT, numDataRecs, dataRecDur, numSignals,
                sampsPerDRQV, RRQVpp[pdnSlot], ampQVpp[pdnSlot], LFperQVpp[pdnSlot],
                HFperQVpp[pdnSlot], LHRatioQVpp[pdnSlot],
                &annOnsetTSQV, &annTextQV,tsQVpp[pdnSlot], skpQVpp[pdnSlot], batQVpp[pdnSlot],edQVpp[pdnSlot]);

    if(dataRecordCt != numDataRecs)
    {
        msgBox.setText("your file has crashed");
        msgBox.exec();
        return false;
    }
    // At this point, input file is not needed anymore
    infilep->close();
    delete infilep;
    delete instrp;

    if(dataRecordCt<1)
    {   ui->msgPTE->appendPlainText("error: the duration < 1 ");
        return false;   // user terminates so don't do anything else
    }
    // check that there is actually usable data, if not exit
    if((!tsQVpp[0]) && (!tsQVpp[1]) && (!tsQVpp[2]) && (!tsQVpp[3]) &&
            (!tsQVpp[4]) && (!tsQVpp[5]) && (!tsQVpp[6]) && (!tsQVpp[7]))
    {
        ui->msgPTE->appendPlainText("No usable data in files... ");
        return false;   // user terminates so don't do anything else
    }


    // go to converter options, once it returns, data has been processed and only needs
    // to be written to file

    coWinp = new ConvertOptionsWindow(&outDirName, pdnListp, &alpha, &Sigma, &outDataType, tsQVpp, skpQVpp,
                                      batQVpp,edQVpp, RRQVpp, ampQVpp, LFperQVpp, HFperQVpp, LHRatioQVpp,
                                      &annOnsetTSQV, &annTextQV ,EDFMECGTOEDF, &useProfile, this);
    coWinp->show();
    if(coWinp->exec() == QDialog::Rejected)
    {
        ui->msgPTE->appendPlainText("User aborted... ");
        delete coWinp;
        return false;   // user terminates so don't do anything else
    }
    delete coWinp;


    // remove tags that are before the first timestamp
    while(annOnsetTSQV.size() > 0)
    {
        if(annOnsetTSQV.at(0) >= tsQVpp[pdnSlot]->at(0)) break;
                // don't need to remove anymore since
        else                                    // tag text will be monotonically increasing
        {
            annOnsetTSQV.remove(0,1);
            annTextQV.remove(0,1);
        }
    }
    stDT = QDateTime::currentDateTime();
    // Write output files
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((tsQVpp[pdnSlot]->at(0))*1000/UCRTCFREQ);
    datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");

    // Output file stuff
    outFileName = outDirName;
    outFileName.append(QString("D%1_%2_ECG.edf").arg(datetimestr).arg(pdnDes));
    outfile1p = new QFile(outFileName);

    if(outfile1p->exists())
    {
        msgBox.setWindowTitle("Out File Exists");
        msgBox.setText("Out file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delete outfile1p;
            return false;
        }
    }
    if(!outfile1p->open(QIODevice::WriteOnly))
    {
        ui->msgPTE->appendPlainText(QString("PDN#%1: Problem with output files").arg(pdnDes));
        delete outfile1p;
        return false;
    }

    out1strp = new QDataStream(outfile1p);
    out1strp->setByteOrder(QDataStream::LittleEndian);

    // EDF header stuff
    if(useProfile)
    {
        readName = getConfigValue("Name");
        readDOB = getConfigValue("DOB");
        readSex = getConfigValue("Sex");
        localPatientID = QString("X "); // for hospital idcode
        if(readSex.isEmpty())
            localPatientID.append("X ");
        else
            localPatientID.append(readSex).append(" ");
        if(readDOB.isEmpty())
            localPatientID.append("X ");
        else
            localPatientID.append(readDOB).append(" ");
        if(readName.isEmpty())
            localPatientID.append("X");
        else
            localPatientID.append(readName);
    }
    else
        localPatientID = lpid;

    // make sure StartDate is correct in local recording id
    tempQSL = lrid.split(QRegExp("\\s+"));
    stDateQS = firstFrmDT.toString("-yyyy");
    currMonth = firstFrmDT.date().month();      // have to do this because of QDateTime letters being 2 chars
    if(currMonth == 1)  stDateQS.prepend("JAN");
    else if(currMonth == 2) stDateQS.prepend("FEB");
    else if(currMonth == 3) stDateQS.prepend("MAR");
    else if(currMonth == 4) stDateQS.prepend("APR");
    else if(currMonth == 5) stDateQS.prepend("MAY");
    else if(currMonth == 6) stDateQS.prepend("JUN");
    else if(currMonth == 7) stDateQS.prepend("JUL");
    else if(currMonth == 8) stDateQS.prepend("AUG");
    else if(currMonth == 9) stDateQS.prepend("SEP");
    else if(currMonth == 10) stDateQS.prepend("OCT");
    else if(currMonth == 11) stDateQS.prepend("NOV");
    else stDateQS.prepend("DEC");
    stDateQS.prepend(firstFrmDT.toString("dd-"));
    tempQSL.replace(1, stDateQS);
    lrid = tempQSL.join(" ");

    // EDF header, will need to come back later to adjust number of data records
    edfMecghdropiwrite(out1strp, &localPatientID, &lrid, &firstFrmDT, dataRecordCt);

    // Stuff in all the data according to specified format and save for meta data processing
    edftoedfMecgwrite(out1strp,RRQVpp[pdnSlot],ampQVpp[pdnSlot],
                      LFperQVpp[pdnSlot],HFperQVpp[pdnSlot],
                      LHRatioQVpp[pdnSlot],dataRecordCt,tsQVpp[pdnSlot]->at(0), &annOnsetTSQV, &annTextQV);
    //ECG  LF=SDNN*10; HFper=log HFpower; LHRatio=log LHRatio
    // can close file now
    outfile1p->close();
    delete out1strp;
    delete outfile1p;

    ui->msgPTE->appendPlainText(QString("PDN#%1: EDF Conversion Finished, Saved in %2").arg(pdnDes).arg(outFileName));

    endDT = QDateTime::currentDateTime();
    // Show done
    ui->msgPTE->appendPlainText(QString(">> Conversion Finished in %1 seconds").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));

    return true;
}


void ConvertWindow::edftoedfMecgwrite(QDataStream *outstrp, QVector<qint16> *RRQVp,
                                      QVector<qint16> *ampQVp, QVector<qint16> *LFperQVp,
                                      QVector<qint16> *HFperQVp, QVector<qint16> *LHRatioQVp,
                                      qint32 numDataRecs, qint64 firstFrmTS, QVector<qint64> *tagTSQVp,
                                      QVector<QString> *tagTextQVp)
{
    qint32 i, j;
    bool noMore;    // indicate if no more data to write into data records
    qint32 dataRecordCt;
    // read data from vectors into correct format for edf
    // Write EDF in data record segments
    noMore = false;
    dataRecordCt = 0;
    QByteArray tempQBA;
    QString tempQS;

    for(i = 0; i < numDataRecs; i++) //8" record * 32Hz =256 samples
    {
        for(j = 0; j < 256; j++)
        {
            if((j+i*256)<RRQVp->size())
            *outstrp << RRQVp->at(j+i*256);
            else
            *outstrp << (qint16)0; //fill up with 0
        }

        for(j = 0; j < 256; j++)
        {
            if((j+i*256)<ampQVp->size())
            *outstrp << ampQVp->at(j+i*256);
            else
            *outstrp << (qint16)0;
        }

        for(j = 0;j < 256 ;j++)
        {
            if((j+i*256)<LFperQVp->size())
            *outstrp << LFperQVp->at(j+i*256);
            else
            *outstrp << (qint16)0;
        }

        for(j = 0;j < 256;j++)
        {
            if((j+i*256)<HFperQVp->size())
            *outstrp << HFperQVp->at(j+i*256);
            else
            *outstrp << (qint16)0;
        }

        for(j = 0;j <256;j++)
        {
            if((j+i*256)<LHRatioQVp->size())
            *outstrp << LHRatioQVp->at(j+i*256);
            else
            *outstrp << (qint16)0;
        }

        // every data record has EDF annotation to denote relative start time
        // EDF Annotations, max of 2 annotations per data record, each with 33 total chars
        tempQBA.clear();
        tempQBA.append(QString("+%1").arg(dataRecordCt*EDFDRDURSEC).toUtf8().append(QChar(20)).append(QChar(20)).append(QChar(0)));
        for(j = 0; j < 2; j++)
        {
            if(tagTSQVp->size() > 0)
            {
                tempQS = tagTextQVp->at(0);
                tempQS.truncate(14);    // limit text to 14 charcters
                tempQBA.append(QString("+%1").arg((float)(tagTSQVp->at(0)-firstFrmTS)/UCRTCFREQ).toUtf8().append(QChar(20)).append(tempQS.toUtf8()));
                tempQBA.append(QChar(20)).append(QChar(0));
                tagTSQVp->remove(0,1);
                tagTextQVp->remove(0,1);
            }
        }
        outstrp->writeRawData(tempQBA.leftJustified(60,0x00), 60);
        dataRecordCt++;
    }
}


bool ConvertWindow::convertedfMecgtowav()
{
    QString outFileName, outDirName;
    QFile *infilep, *outfile1p ;
    QDataStream *instrp;
    QDataStream *out1strp;
    QString datetimestr, tempqstr;
    QDateTime firstFrmDT, stDT, endDT;
    QVector<qint16> *RRQVpp[PDNLISTLEN];
    QVector<qint16> *ampQVpp[PDNLISTLEN];
    QVector<qint16> *LFperQVpp[PDNLISTLEN]; //SDNN
    QVector<qint16> *HFperQVpp[PDNLISTLEN]; //HFpower
    QVector<qint16> *LHRatioQVpp[PDNLISTLEN]; //log
    QVector<qint64> *tsQVpp[PDNLISTLEN];
    QVector<quint8> *skpQVpp[PDNLISTLEN];
    QVector<quint8> *batQVpp[PDNLISTLEN];
    QVector<quint8> *edQVpp[PDNLISTLEN];
    QVector<qint64> annOnsetTSQV;
    QVector<QString> annTextQV;
    ConvertOptionsWindow *coWinp;
    QString lpid, lrid;
    QDateTime startDT;
    qint32 numDataRecs, dataRecDur;
    qint32 numSignals;
    QVector<QString> labelSignalsQV, transTypeQV, physDimQV, prefiltQV;
    QVector<qint32> physMinQV, physMaxQV, digMinQV, digMaxQV;
    QVector<qint32> sampsPerDRQV;
    quint8 pdnListp[PDNLISTLEN];
    qint32 pdnSlot;
    qint32 pdnDes;
    qint8 outDataType;
    qint32 dataRecordCt;
    bool useProfile;
    QMessageBox msgBox;
    int i;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(ui->inLE->text());
    if (!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(ui->inLE->text()));
        delete infilep;
        return false;
    }

    // open input file stream
    instrp = new QDataStream(infilep);
    instrp->setByteOrder(QDataStream::LittleEndian);

    // get header information
    edfhdrread(instrp, &lpid, &lrid, &startDT, &numDataRecs, &dataRecDur,
               &numSignals, &labelSignalsQV, &transTypeQV, &physDimQV, &physMinQV,
               &physMaxQV, &digMinQV, &digMaxQV, &prefiltQV, &sampsPerDRQV);

    dataRecordCt = numDataRecs;

    // init qvector pointers to null
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        RRQVpp[pdnSlot] = 0;
        ampQVpp[pdnSlot] = 0;
        LFperQVpp[pdnSlot] = 0;
        HFperQVpp[pdnSlot] = 0;
        LHRatioQVpp[pdnSlot] = 0;
        tsQVpp[pdnSlot] = 0;
        skpQVpp[pdnSlot] = 0;
        batQVpp[pdnSlot] = 0;
        edQVpp[pdnSlot] = 0;
    }

    // get PDN from Device field
    pdnDes = getPDNlrid(lrid);

    pdnSlot = 0;

    // copy pdn's
    for(i = 0; i < PDNLISTLEN; i++)
        pdnListp[i] = 0xFF;
    pdnListp[pdnSlot] = pdnDes;

    // initialize qvectors
    RRQVpp[pdnSlot] = new QVector<qint16>(0);
    ampQVpp[pdnSlot] = new QVector<qint16>(0);
    LFperQVpp[pdnSlot] = new QVector<qint16>(0);
    HFperQVpp[pdnSlot] = new QVector<qint16>(0);
    LHRatioQVpp[pdnSlot] = new QVector<qint16>(0);
    tsQVpp[pdnSlot] = new QVector<qint64>(0);
    skpQVpp[pdnSlot] = new QVector<quint8>(0);
    batQVpp[pdnSlot] = new QVector<quint8>(0);
    edQVpp[pdnSlot] = new QVector<quint8>(0);

    dataRecordCt = edfMecgread(instrp, startDT, numDataRecs, dataRecDur, numSignals,
                sampsPerDRQV, RRQVpp[pdnSlot], ampQVpp[pdnSlot], LFperQVpp[pdnSlot],
                HFperQVpp[pdnSlot], LHRatioQVpp[pdnSlot],
                &annOnsetTSQV, &annTextQV,tsQVpp[pdnSlot], skpQVpp[pdnSlot], batQVpp[pdnSlot],edQVpp[pdnSlot]);

    if(dataRecordCt != numDataRecs)
    {
        msgBox.setText("your file has crashed");
        msgBox.exec();
        return false;
    }
    // At this point, input file is not needed anymore
    infilep->close();
    delete infilep;
    delete instrp;

    if(dataRecordCt<1)
    {   ui->msgPTE->appendPlainText("error:the duration < 1 ");
        return false;   // user terminates so don't do anything else
    }
    // check that there is actually usable data, if not exit
    if((!tsQVpp[0]) && (!tsQVpp[1]) && (!tsQVpp[2]) && (!tsQVpp[3]) &&
            (!tsQVpp[4]) && (!tsQVpp[5]) && (!tsQVpp[6]) && (!tsQVpp[7]))
    {
        ui->msgPTE->appendPlainText("No usable data in files... ");
        return false;   // user terminates so don't do anything else
    }
    // go to converter options, once it returns, data has been processed and only needs
    // to be written to file


    coWinp = new ConvertOptionsWindow(&outDirName, pdnListp, &alpha, &Sigma, &outDataType, tsQVpp, skpQVpp,
                                      batQVpp,edQVpp, RRQVpp, ampQVpp, LFperQVpp, HFperQVpp, LHRatioQVpp,
                                      &annOnsetTSQV, &annTextQV ,EDFMECGTOWAV, &useProfile, this);
    coWinp->show();
    if(coWinp->exec() == QDialog::Rejected)
    {
        ui->msgPTE->appendPlainText("User aborted... ");
        //delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
        delete coWinp;
        return false;   // user terminates so don't do anything else
    }
    delete coWinp;

    stDT = QDateTime::currentDateTime();

    // Write output files
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((tsQVpp[pdnSlot]->at(0))*1000/UCRTCFREQ);
    datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");

    // Output file stuff
    outFileName = outDirName;
    outFileName.append(QString("D%1_%2_ECG.wav").arg(datetimestr).arg(pdnDes));
    outfile1p = new QFile(outFileName);

    if(outfile1p->exists())
    {
        msgBox.setWindowTitle("Out File Exists");
        msgBox.setText("Out file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            // delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            delete outfile1p;
            return false;
        }
    }
    if(!outfile1p->open(QIODevice::WriteOnly))
    {
        ui->msgPTE->appendPlainText(QString("PDN#%1: Problem with output files").arg(pdnDes));
        // delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
        delete outfile1p;
        return false;
    }

    out1strp = new QDataStream(outfile1p);
    out1strp->setByteOrder(QDataStream::LittleEndian);

    wavhdrwrite(out1strp, TSRTCFREQ/16, 5, RRQVpp[pdnSlot]->size(), 16); // 32Hz

    edftowavMecgwrite(out1strp,RRQVpp[pdnSlot],ampQVpp[pdnSlot],
                      LFperQVpp[pdnSlot],HFperQVpp[pdnSlot],
                      LHRatioQVpp[pdnSlot],dataRecordCt);

    //ECG  LF=SDNN*10; HFper=log HFpower; LHRatio=log LHRatio
    // can close file now
    outfile1p->close();
    delete out1strp;
    delete outfile1p;

    ui->msgPTE->appendPlainText(QString("PDN#%1: WAV Conversion Finished, Saved in %2").arg(pdnDes).arg(outFileName));
    endDT = QDateTime::currentDateTime();

    // Show done
    ui->msgPTE->appendPlainText(QString(">> Conversion Finished in %1 seconds").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));

    return true;
}


void ConvertWindow::edftowavMecgwrite(QDataStream *outstrp, QVector<qint16> *RRQVp, QVector<qint16> *ampQVp,
                                      QVector<qint16> *LFperQVp, QVector<qint16> *HFperQVp,
                                      QVector<qint16> *LHRatioQVp, qint32 numDataRecs)
{
    qint32 i;
    for(i = 0; i < RRQVp->size(); i++)
    {
        *outstrp << RRQVp->at(i) << ampQVp->at(i) <<LFperQVp->at(i)<<HFperQVp->at(i)<<LHRatioQVp->at(i);
    }
}


qint32 ConvertWindow::edfMecgread(QDataStream *instrp, QDateTime startDT, qint32 numDataRecs, qint32 dataRecDur, qint32 numSignals, QVector<qint32> sampsPerDRQV,
                                  QVector<qint16> *RRQVp, QVector<qint16> *ampQVp,
                                  QVector<qint16> *LFperQVp, QVector<qint16> *HFperQVp,
                                  QVector<qint16> *LHRatioQVp, QVector<qint64> *annOnsetTSQVp,
                                  QVector<QString> *annTextQVp,
                                  QVector<qint64> *tsQVp, QVector<quint8> *skpQVp,
                                  QVector<quint8> *batQVp, QVector<quint8> *edQVp)
    //ECG  LF=SDNN*10; HFper=log HFpower; LHRatio=log LHRatio
{
    qint16 tempRRs[256];
    qint16 tempamps[256];
    qint16 tempLFpers[256];
    qint16 tempHFpers[256];
    qint16 tempLHRatios[256];
    quint8 tempanns[128];
    qint32 i, j, dataRecCt;
    qint64 prevTS, tempTS, stepTS; //stepTS for each sample
    QStringList annsQL, annQL;

    // check to make sure things are right
    if((sampsPerDRQV.at(0) != 256) ||
            (sampsPerDRQV.at(1) != 256) ||
            (sampsPerDRQV.at(2) != 256) ||
            (sampsPerDRQV.at(3) != 256) ||
            (sampsPerDRQV.at(4) != 256) ||
            ((sampsPerDRQV.at(5) != 64) && (sampsPerDRQV.at(5) != 30)))
        return -1;

    // Initialization
    stepTS = (UCRTCFREQ/ECGSAMPLERATE);  //no record, 32Hz=128 counts
    prevTS = (startDT.toMSecsSinceEpoch()-QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000;
    prevTS -= stepTS;   // must make it previous TS
    if(prevTS < 0) prevTS = 0;
    dataRecCt = 0;

    // Read in data until end
    //32hz duration 8
    while(!instrp->atEnd()) // read until no more data
    {

        // adc data 8192 = sampsPerDRQV.at(0)*2bytes
        if(instrp->readRawData((char *)tempRRs, sampsPerDRQV.at(0)*2) < 0) break;  // not enough data
        if(instrp->readRawData((char *)tempamps, sampsPerDRQV.at(1)*2) < 0) break;
        if(instrp->readRawData((char *)tempLFpers, sampsPerDRQV.at(2)*2) < 0) break;
        if(instrp->readRawData((char *)tempHFpers, sampsPerDRQV.at(3)*2) < 0) break;
        if(instrp->readRawData((char *)tempLHRatios, sampsPerDRQV.at(4)*2) < 0) break;
        if(instrp->readRawData((char *)tempanns, sampsPerDRQV.at(5)*2) < 0) break;

        // put into qvectors because data record is complete
        for(j = 0; j < 256; j++)  //256 samples per record
        {
                RRQVp->append(tempRRs[j]);
                ampQVp->append(tempamps[j]);
                LFperQVp->append(tempLFpers[j]); //SDNN
                HFperQVp->append(tempHFpers[j]); //HFpower
                LHRatioQVp->append(tempLHRatios[j]); //log
                tsQVp->append(prevTS+stepTS);
                prevTS += stepTS;
                skpQVp->append(0);
                batQVp->append(1);
                edQVp->append(0);
        }


        // take care of annotations
        annsQL = QString::fromAscii((const char *)tempanns,128).split(QChar(0),QString::SkipEmptyParts);
        for(i = 0; i < annsQL.size(); i++)
        {
            annQL = annsQL.at(i).split(QChar(20),QString::SkipEmptyParts); // split each entry
            if(annQL.size() < 2) continue; // no tag entries
            // first parts is always the time
            tempTS = (qint64) (annQL.at(0).toFloat()*UCRTCFREQ+(startDT.toMSecsSinceEpoch()-QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000);
            for(j = 1; j < annQL.size(); j++)
            {
                annOnsetTSQVp->append(tempTS);
                annTextQVp->append(annQL.at(j));
            }
        }
        dataRecCt++;
    }

    return dataRecCt;
}


bool ConvertWindow::convertedfMeegtoedf()
{
    QString outFileName, outDirName;
    QFile *infilep, *outfile1p;
    QDataStream *instrp;
    QDataStream *out1strp;
    QString datetimestr;
    QDateTime firstFrmDT, stDT, endDT;
    quint8 pdnListp[PDNLISTLEN];
    QVector<qint16> *M2QVpp[PDNLISTLEN];
    QVector<qint16> *M1QVpp[PDNLISTLEN];
    QVector<qint16> *G2QVpp[PDNLISTLEN];
    QVector<qint16> *G1QVpp[PDNLISTLEN];
    QVector<qint16> *UPQVpp[PDNLISTLEN];
    QVector<qint16> *BetaQVpp[PDNLISTLEN];
    QVector<qint16> *SigmaQVpp[PDNLISTLEN];
    QVector<qint16> *alphaQVpp[PDNLISTLEN];
    QVector<qint16> *thetaQVpp[PDNLISTLEN];
    QVector<qint16> *deltaQVpp[PDNLISTLEN];
    QVector<qint64> *tsQVpp[PDNLISTLEN];
    QVector<quint8> *skpQVpp[PDNLISTLEN];
    QVector<quint8> *batQVpp[PDNLISTLEN];
    QVector<quint8> *edQVpp[PDNLISTLEN];
    QVector<qint64> annOnsetTSQV;
    QVector<QString> annTextQV;
    ConvertOptionsWindow *coWinp;
    QString localPatientID, lpid, lrid;
    QDateTime startDT;
    qint32 numDataRecs, dataRecDur;
    qint32 numSignals;
    QVector<QString> labelSignalsQV, transTypeQV, physDimQV, prefiltQV;
    QVector<qint32> physMinQV, physMaxQV, digMinQV, digMaxQV;
    QVector<qint32> sampsPerDRQV;
    qint32 pdnSlot;
    qint32 pdnDes;
    qint8 outDataType;
    qint32 dataRecordCt;
    bool useProfile;
    QString readName, readDOB, readSex;
    QMessageBox msgBox;
    QStringList tempQSL;
    QString stDateQS;
    qint32 currMonth;
    int i;
    qint32 DSSize;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(ui->inLE->text());
    if (!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(ui->inLE->text()));
        delete infilep;
        return false;
    }

    // open input file stream
    instrp = new QDataStream(infilep);
    instrp->setByteOrder(QDataStream::LittleEndian);

    // get header information
    edfhdrread(instrp, &lpid, &lrid, &startDT, &numDataRecs, &dataRecDur,
               &numSignals, &labelSignalsQV, &transTypeQV, &physDimQV, &physMinQV,
               &physMaxQV, &digMinQV, &digMaxQV, &prefiltQV, &sampsPerDRQV);
    dataRecordCt = numDataRecs;
    // init qvector pointers to null
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        M2QVpp[pdnSlot] = 0;
        M1QVpp[pdnSlot] = 0;
        G2QVpp[pdnSlot] = 0;
        G1QVpp[pdnSlot] = 0;
        UPQVpp[pdnSlot] = 0;
        BetaQVpp[pdnSlot] = 0;
        SigmaQVpp[pdnSlot] = 0;
        alphaQVpp[pdnSlot] = 0;
        thetaQVpp[pdnSlot] = 0;
        deltaQVpp[pdnSlot] = 0;
        tsQVpp[pdnSlot] = 0;
        skpQVpp[pdnSlot] = 0;
        batQVpp[pdnSlot] = 0;
        edQVpp[pdnSlot] = 0;
    }

    // get PDN from Device field
    pdnDes = getPDNlrid(lrid);

    pdnSlot = 0;

    // copy pdn's
       for(i = 0; i < PDNLISTLEN; i++)
           pdnListp[i] = 0xFF;
       pdnListp[pdnSlot] = pdnDes;

    // initialize qvectors
    M2QVpp[pdnSlot] = new QVector<qint16>(0);
    M1QVpp[pdnSlot] = new QVector<qint16>(0);
    G2QVpp[pdnSlot] = new QVector<qint16>(0);
    G1QVpp[pdnSlot] = new QVector<qint16>(0);
    UPQVpp[pdnSlot] = new QVector<qint16>(0);
    BetaQVpp[pdnSlot] = new QVector<qint16>(0);
    SigmaQVpp[pdnSlot] = new QVector<qint16>(0);
    alphaQVpp[pdnSlot] = new QVector<qint16>(0);
    thetaQVpp[pdnSlot] = new QVector<qint16>(0);
    deltaQVpp[pdnSlot] = new QVector<qint16>(0);
    tsQVpp[pdnSlot] = new QVector<qint64>(0);
    skpQVpp[pdnSlot] = new QVector<quint8>(0);
    batQVpp[pdnSlot] = new QVector<quint8>(0);
    edQVpp[pdnSlot] = new QVector<quint8>(0);

    dataRecordCt = edfMeegread(instrp, startDT, numDataRecs, dataRecDur, numSignals,
                sampsPerDRQV, M2QVpp[pdnSlot], M1QVpp[pdnSlot], G2QVpp[pdnSlot],
                G1QVpp[pdnSlot], UPQVpp[pdnSlot], BetaQVpp[pdnSlot], SigmaQVpp[pdnSlot],
                alphaQVpp[pdnSlot], thetaQVpp[pdnSlot], deltaQVpp[pdnSlot],
                &annOnsetTSQV, &annTextQV,tsQVpp[pdnSlot], skpQVpp[pdnSlot], batQVpp[pdnSlot],edQVpp[pdnSlot]);
    if(dataRecordCt!= numDataRecs)
    {
        msgBox.setText("your file has crashed");
        msgBox.exec();
        return false;
    }
    // At this point, input file is not needed anymore
    infilep->close();
    delete infilep;
    delete instrp;

    if(dataRecordCt<1)
    {   ui->msgPTE->appendPlainText("error:the duration < 1 ");
        return false;   // user terminates so don't do anything else
    }

    // check that there is actually usable data, if not exit
    if((!tsQVpp[0]) && (!tsQVpp[1]) && (!tsQVpp[2]) && (!tsQVpp[3]) &&
            (!tsQVpp[4]) && (!tsQVpp[5]) && (!tsQVpp[6]) && (!tsQVpp[7]))
    {
        ui->msgPTE->appendPlainText("No usable data in files... ");
        return false;   // user terminates so don't do anything else
    }
    // go to converter options, once it returns, data has been processed and only needs
    // to be written to file
    coWinp = new ConvertOptionsWindow(&outDirName, &alpha,&Sigma,pdnListp,&outDataType,tsQVpp,skpQVpp,batQVpp,
                                      edQVpp,M2QVpp,M1QVpp,G2QVpp,G1QVpp,UPQVpp,BetaQVpp,SigmaQVpp,
                                      alphaQVpp,thetaQVpp,deltaQVpp,&annOnsetTSQV, &annTextQV ,EDFMEEGTOEDF, &useProfile, this);

    if(coWinp->exec() == QDialog::Rejected)
    {
        ui->msgPTE->appendPlainText("User aborted... ");
        delete coWinp;
        return false;   // user terminates so don't do anything else
    }
    delete coWinp;

    // remove tags that are before the first timestamp
    while(annOnsetTSQV.size() > 0)
    {
        if(annOnsetTSQV.at(0) >= tsQVpp[pdnSlot]->at(0)) break;
                // don't need to remove anymore since
        else                                    // tag text will be monotonically increasing
        {
            annOnsetTSQV.remove(0,1);
            annTextQV.remove(0,1);
        }
    }
    stDT = QDateTime::currentDateTime();
    // Write output files
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((tsQVpp[pdnSlot]->at(0))*1000/UCRTCFREQ);
    datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");

    // Output file stuff
    outFileName = outDirName;
    outFileName.append(QString("D%1_%2_EEG.edf").arg(datetimestr).arg(pdnDes));
    outfile1p = new QFile(outFileName);

    if(outfile1p->exists())
    {
        msgBox.setWindowTitle("Out File Exists");
        msgBox.setText("Out file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delete outfile1p;
            return false;
        }
    }
    if(!outfile1p->open(QIODevice::WriteOnly))
    {
        ui->msgPTE->appendPlainText(QString("PDN#%1: Problem with output files").arg(pdnDes));
        delete outfile1p;
        return false;
    }

    out1strp = new QDataStream(outfile1p);
    out1strp->setByteOrder(QDataStream::LittleEndian);

    // EDF header stuff
    if(useProfile)
    {
        readName = getConfigValue("Name");
        readDOB = getConfigValue("DOB");
        readSex = getConfigValue("Sex");
        localPatientID = QString("X "); // for hospital idcode
        if(readSex.isEmpty())
            localPatientID.append("X ");
        else
            localPatientID.append(readSex).append(" ");
        if(readDOB.isEmpty())
            localPatientID.append("X ");
        else
            localPatientID.append(readDOB).append(" ");
        if(readName.isEmpty())
            localPatientID.append("X");
        else
            localPatientID.append(readName);
    }
    else
        localPatientID = lpid;

    // make sure StartDate is correct in local recording id
    tempQSL = lrid.split(QRegExp("\\s+"));
    stDateQS = firstFrmDT.toString("-yyyy");
    currMonth = firstFrmDT.date().month();      // have to do this because of QDateTime letters being 2 chars
    if(currMonth == 1)  stDateQS.prepend("JAN");
    else if(currMonth == 2) stDateQS.prepend("FEB");
    else if(currMonth == 3) stDateQS.prepend("MAR");
    else if(currMonth == 4) stDateQS.prepend("APR");
    else if(currMonth == 5) stDateQS.prepend("MAY");
    else if(currMonth == 6) stDateQS.prepend("JUN");
    else if(currMonth == 7) stDateQS.prepend("JUL");
    else if(currMonth == 8) stDateQS.prepend("AUG");
    else if(currMonth == 9) stDateQS.prepend("SEP");
    else if(currMonth == 10) stDateQS.prepend("OCT");
    else if(currMonth == 11) stDateQS.prepend("NOV");
    else stDateQS.prepend("DEC");
    stDateQS.prepend(firstFrmDT.toString("dd-"));
    tempQSL.replace(1, stDateQS);
    lrid = tempQSL.join(" ");

    // Because this was read in, all the vectors are 8x original, so need to downsample
    DSSize = M2QVpp[pdnSlot]->size()/EEGSAMPLERATE;
    for(i = 0; i < DSSize; i++)
    {
        M2QVpp[pdnSlot]->remove(i+1, 7);
        M1QVpp[pdnSlot]->remove(i+1, 7);
        G2QVpp[pdnSlot]->remove(i+1, 7);
        G1QVpp[pdnSlot]->remove(i+1, 7);
        UPQVpp[pdnSlot]->remove(i+1, 7);
        BetaQVpp[pdnSlot]->remove(i+1, 7);
        SigmaQVpp[pdnSlot]->remove(i+1, 7);
        alphaQVpp[pdnSlot]->remove(i+1, 7);
        thetaQVpp[pdnSlot]->remove(i+1, 7);
        deltaQVpp[pdnSlot]->remove(i+1, 7);
    }
    // EDF header, will need to come back later to adjust number of data records
    edfMeeghdropiwrite(out1strp, &localPatientID, &lrid, &firstFrmDT,M2QVpp[pdnSlot]->size()/EDFDRDURSEC);

    // Stuff in all the data according to specified format and save for meta data processing
    edfMeegwrite(out1strp,M2QVpp[pdnSlot],M1QVpp[pdnSlot],
                 G2QVpp[pdnSlot],G1QVpp[pdnSlot],
                 UPQVpp[pdnSlot],BetaQVpp[pdnSlot],
                 SigmaQVpp[pdnSlot],alphaQVpp[pdnSlot],
                 thetaQVpp[pdnSlot],deltaQVpp[pdnSlot],
                 M2QVpp[pdnSlot]->size()/EDFDRDURSEC,
                 tsQVpp[pdnSlot]->at(0), &annOnsetTSQV, &annTextQV);

    // can close file now
    outfile1p->close();
    delete out1strp;
    delete outfile1p;

    ui->msgPTE->appendPlainText(QString("PDN#%1: EDF Conversion Finished, Saved in %2").arg(pdnDes).arg(outFileName));
    endDT = QDateTime::currentDateTime();

    // Show done
    ui->msgPTE->appendPlainText(QString(">> Conversion Finished in %1 seconds").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));
    return true;

}


bool ConvertWindow::convertedfMeegtowav()
{
    QString outFileName, outDirName;
    QFile *infilep, *outfile1p;
    QDataStream *instrp;
    QDataStream *out1strp;
    QString datetimestr, tempqstr;
    QDateTime firstFrmDT, stDT, endDT;
    quint8 pdnListp[PDNLISTLEN];
    QVector<qint16> *M2QVpp[PDNLISTLEN];
    QVector<qint16> *M1QVpp[PDNLISTLEN];
    QVector<qint16> *G2QVpp[PDNLISTLEN];
    QVector<qint16> *G1QVpp[PDNLISTLEN];
    QVector<qint16> *UPQVpp[PDNLISTLEN];
    QVector<qint16> *BetaQVpp[PDNLISTLEN];
    QVector<qint16> *SigmaQVpp[PDNLISTLEN];
    QVector<qint16> *alphaQVpp[PDNLISTLEN];
    QVector<qint16> *thetaQVpp[PDNLISTLEN];
    QVector<qint16> *deltaQVpp[PDNLISTLEN];
    QVector<qint64> *tsQVpp[PDNLISTLEN];
    QVector<quint8> *skpQVpp[PDNLISTLEN];
    QVector<quint8> *batQVpp[PDNLISTLEN];
    QVector<quint8> *edQVpp[PDNLISTLEN];
    QVector<qint64> annOnsetTSQV;
    QVector<QString> annTextQV;
    ConvertOptionsWindow *coWinp;
    QString lpid, lrid;
    QDateTime startDT;
    qint32 numDataRecs, dataRecDur;
    qint32 numSignals;
    QVector<QString> labelSignalsQV, transTypeQV, physDimQV, prefiltQV;
    QVector<qint32> physMinQV, physMaxQV, digMinQV, digMaxQV;
    QVector<qint32> sampsPerDRQV;
    qint32 pdnSlot;
    qint32 pdnDes;
    qint8 outDataType;
    qint32 dataRecordCt;
    bool useProfile;
    QMessageBox msgBox;
    int i;
    qint32 DSSize;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(ui->inLE->text());
    if (!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(ui->inLE->text()));
        delete infilep;
        return false;
    }

    // open input file stream
    instrp = new QDataStream(infilep);
    instrp->setByteOrder(QDataStream::LittleEndian);

    // get header information
    edfhdrread(instrp, &lpid, &lrid, &startDT, &numDataRecs, &dataRecDur,
               &numSignals, &labelSignalsQV, &transTypeQV, &physDimQV, &physMinQV,
               &physMaxQV, &digMinQV, &digMaxQV, &prefiltQV, &sampsPerDRQV);

    dataRecordCt = numDataRecs;
    // init qvector pointers to null
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        M2QVpp[pdnSlot] = 0;
        M1QVpp[pdnSlot] = 0;
        G2QVpp[pdnSlot] = 0;
        G1QVpp[pdnSlot] = 0;
        UPQVpp[pdnSlot] = 0;
        BetaQVpp[pdnSlot] = 0;
        SigmaQVpp[pdnSlot] = 0;
        alphaQVpp[pdnSlot] = 0;
        thetaQVpp[pdnSlot] = 0;
        deltaQVpp[pdnSlot] = 0;
        tsQVpp[pdnSlot] = 0;
        skpQVpp[pdnSlot] = 0;
        batQVpp[pdnSlot] = 0;
        edQVpp[pdnSlot] = 0;
    }

    // get PDN from Device field
    pdnDes = getPDNlrid(lrid);

    pdnSlot = 0;

    // copy pdn's
    for(i = 0; i < PDNLISTLEN; i++)
        pdnListp[i] = 0xFF;
    pdnListp[pdnSlot] = pdnDes;

    // initialize qvectors
    M2QVpp[pdnSlot] = new QVector<qint16>(0);
    M1QVpp[pdnSlot] = new QVector<qint16>(0);
    G2QVpp[pdnSlot] = new QVector<qint16>(0);
    G1QVpp[pdnSlot] = new QVector<qint16>(0);
    UPQVpp[pdnSlot] = new QVector<qint16>(0);
    BetaQVpp[pdnSlot] = new QVector<qint16>(0);
    SigmaQVpp[pdnSlot] = new QVector<qint16>(0);
    alphaQVpp[pdnSlot] = new QVector<qint16>(0);
    thetaQVpp[pdnSlot] = new QVector<qint16>(0);
    deltaQVpp[pdnSlot] = new QVector<qint16>(0);
    tsQVpp[pdnSlot] = new QVector<qint64>(0);
    skpQVpp[pdnSlot] = new QVector<quint8>(0);
    batQVpp[pdnSlot] = new QVector<quint8>(0);
    edQVpp[pdnSlot] = new QVector<quint8>(0);

    if(edfMeegread(instrp, startDT, numDataRecs, dataRecDur, numSignals,
                sampsPerDRQV, M2QVpp[pdnSlot], M1QVpp[pdnSlot], G2QVpp[pdnSlot],
                G1QVpp[pdnSlot], UPQVpp[pdnSlot], BetaQVpp[pdnSlot], SigmaQVpp[pdnSlot],
                alphaQVpp[pdnSlot], thetaQVpp[pdnSlot], deltaQVpp[pdnSlot],
                &annOnsetTSQV, &annTextQV,tsQVpp[pdnSlot], skpQVpp[pdnSlot], batQVpp[pdnSlot],edQVpp[pdnSlot]) < 0)
    {

    }
    // At this point, input file is not needed anymore
    infilep->close();
    delete infilep;
    delete instrp;

    if(dataRecordCt < 1)
    {
        ui->msgPTE->appendPlainText("error:the duration < 1 ");
        return false;   // user terminates so don't do anything else
    }
    // check that there is actually usable data, if not exit
    if((!tsQVpp[0]) && (!tsQVpp[1]) && (!tsQVpp[2]) && (!tsQVpp[3]) &&
            (!tsQVpp[4]) && (!tsQVpp[5]) && (!tsQVpp[6]) && (!tsQVpp[7]))
    {
        ui->msgPTE->appendPlainText("No usable data in files... ");
        return false;   // user terminates so don't do anything else
    }

    // go to converter options, once it returns, data has been processed and only needs
    // to be written to file

    coWinp = new ConvertOptionsWindow(&outDirName, &alpha,&Sigma,pdnListp,&outDataType,tsQVpp,skpQVpp,batQVpp,
                                      edQVpp,M2QVpp,M1QVpp,G2QVpp,G1QVpp,UPQVpp,BetaQVpp,SigmaQVpp,
                                      alphaQVpp,thetaQVpp,deltaQVpp,&annOnsetTSQV, &annTextQV ,EDFMEEGTOWAV, &useProfile, this);


    if(coWinp->exec() == QDialog::Rejected)
    {
        ui->msgPTE->appendPlainText("User aborted... ");
        delete coWinp;
        return false;   // user terminates so don't do anything else
    }
    delete coWinp;

    stDT = QDateTime::currentDateTime();
    // Write output files
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((tsQVpp[pdnSlot]->at(0))*1000/UCRTCFREQ);
    datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");

    // Output file stuff
    outFileName = outDirName;
    outFileName.append(QString("D%1_%2_EEG.wav").arg(datetimestr).arg(pdnDes));
    outfile1p = new QFile(outFileName);

    if(outfile1p->exists())
    {
        msgBox.setWindowTitle("Out File Exists");
        msgBox.setText("Out file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delete outfile1p;
            return false;
        }
    }
    if(!outfile1p->open(QIODevice::WriteOnly))
    {
        ui->msgPTE->appendPlainText(QString("PDN#%1: Problem with output files").arg(pdnDes));
        delete outfile1p;
        return false;
    }

    out1strp = new QDataStream(outfile1p);
    out1strp->setByteOrder(QDataStream::LittleEndian);

    // Because this was read in, all the vectors are 8x original, so need to downsample
    DSSize = M2QVpp[pdnSlot]->size()/EEGSAMPLERATE;
    for(i = 0; i < DSSize; i++)
    {
        M2QVpp[pdnSlot]->remove(i+1, 7);
        M1QVpp[pdnSlot]->remove(i+1, 7);
        G2QVpp[pdnSlot]->remove(i+1, 7);
        G1QVpp[pdnSlot]->remove(i+1, 7);
        UPQVpp[pdnSlot]->remove(i+1, 7);
        BetaQVpp[pdnSlot]->remove(i+1, 7);
        SigmaQVpp[pdnSlot]->remove(i+1, 7);
        alphaQVpp[pdnSlot]->remove(i+1, 7);
        thetaQVpp[pdnSlot]->remove(i+1, 7);
        deltaQVpp[pdnSlot]->remove(i+1, 7);
    }

    wavhdrwrite(out1strp, 1, 10,M2QVpp[pdnSlot]->size(), 16); // 1Hz

    wavMeegwrite(out1strp,M2QVpp[pdnSlot],M1QVpp[pdnSlot],
                 G2QVpp[pdnSlot],G1QVpp[pdnSlot],
                 UPQVpp[pdnSlot],BetaQVpp[pdnSlot],
                 SigmaQVpp[pdnSlot],alphaQVpp[pdnSlot],
                 thetaQVpp[pdnSlot],deltaQVpp[pdnSlot],
                 M2QVpp[pdnSlot]->size());
    // can close file now
    outfile1p->close();
    delete out1strp;
    delete outfile1p;

    ui->msgPTE->appendPlainText(QString("PDN#%1: WAV Conversion Finished, Saved in %2").arg(pdnDes).arg(outFileName));
    endDT = QDateTime::currentDateTime();
    // Show done
    ui->msgPTE->appendPlainText(QString(">> Conversion Finished in %1 seconds").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));

    return true;
}


qint32 ConvertWindow::edfMeegread(QDataStream *instrp, QDateTime startDT, qint32 numDataRecs, qint32 dataRecDur, qint32 numSignals, QVector<qint32> sampsPerDRQV,
                                  QVector<qint16> *M2QVp, QVector<qint16> *M1QVp,
                                  QVector<qint16> *G2QVp, QVector<qint16> *G1QVp,
                                  QVector<qint16> *UPQVp, QVector<qint16> *BetaQVp,
                                  QVector<qint16> *SigmaQVp, QVector<qint16> *alphaQVp,
                                  QVector<qint16> *thetaQVp, QVector<qint16> *deltaQVp,
                                  QVector<qint64> *annOnsetTSQVp, QVector<QString> *annTextQVp,
                                  QVector<qint64> *tsQVp, QVector<quint8> *skpQVp,
                                  QVector<quint8> *batQVp, QVector<quint8> *edQVp)
{
    qint16 tempM2s[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempM1s[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempG2s[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempG1s[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempUPs[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempBetas[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempSigmas[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempalphas[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempthetas[EEGSAMPLERATE*EDFDRDURSEC];
    qint16 tempdeltas[EEGSAMPLERATE*EDFDRDURSEC];
    quint8 tempanns[128];
    qint32 i, j, dataRecCt;
    qint64 prevTS, tempTS, stepTS; //add stepEEGTS
    QStringList annsQL, annQL;

    // check to make sure things are right
    if((sampsPerDRQV.at(0) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(1) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(2) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(3) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(4) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(5) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(6) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(7) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(8) != EEGSAMPLERATE*EDFDRDURSEC) ||
            (sampsPerDRQV.at(9) != EEGSAMPLERATE*EDFDRDURSEC) ||
            ((sampsPerDRQV.at(10) != 64) && (sampsPerDRQV.at(10) != 30))
           )
        return -1;

    // Initialization
    stepTS = UCRTCFREQ/EEGSAMPLERATE; //#ticks=512 per EEG sample 8Hz
    prevTS = (startDT.toMSecsSinceEpoch()-QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000;
    prevTS -= stepTS;   // must make it previous TS
    if(prevTS < 0) prevTS = 0;
    dataRecCt = 0;

    // Read in data until end
    while(!instrp->atEnd()) // read until no more data
    {
        if(instrp->readRawData((char *)tempM2s, sampsPerDRQV.at(0)*2) < 0) break;  // not enough data
        if(instrp->readRawData((char *)tempM1s, sampsPerDRQV.at(1)*2) < 0) break;
        if(instrp->readRawData((char *)tempG2s, sampsPerDRQV.at(2)*2) < 0) break;
        if(instrp->readRawData((char *)tempG1s, sampsPerDRQV.at(3)*2) < 0) break;
        if(instrp->readRawData((char *)tempUPs, sampsPerDRQV.at(4)*2) < 0) break;
        if(instrp->readRawData((char *)tempBetas, sampsPerDRQV.at(5)*2) < 0) break;
        if(instrp->readRawData((char *)tempSigmas, sampsPerDRQV.at(6)*2) < 0) break;
        if(instrp->readRawData((char *)tempalphas, sampsPerDRQV.at(7)*2) < 0) break;
        if(instrp->readRawData((char *)tempthetas, sampsPerDRQV.at(8)*2) < 0) break;
        if(instrp->readRawData((char *)tempdeltas, sampsPerDRQV.at(9)*2) < 0) break;
        if(instrp->readRawData((char *)tempanns, sampsPerDRQV.at(10)*2) < 0) break;

        for(i = 0; i < EEGSAMPLERATE*EDFDRDURSEC ;i++)  //read a record = 1sec
        {
            M2QVp->append(tempM2s[i]);
            M1QVp->append(tempM1s[i]);
            G2QVp->append(tempG2s[i]);
            G1QVp->append(tempG1s[i]);
            UPQVp->append(tempUPs[i]);
            BetaQVp->append(tempBetas[i]);
            SigmaQVp->append(tempSigmas[i]);
            alphaQVp->append(tempalphas[i]);
            thetaQVp->append(tempthetas[i]);
            deltaQVp->append(tempdeltas[i]);
            tsQVp->append(prevTS+stepTS);
            prevTS += stepTS;
            skpQVp->append(0);
            batQVp->append(1);
            edQVp->append(0);
       }

        // take care of annotations
        annsQL = QString::fromAscii((const char *)tempanns,128).split(QChar(0),QString::SkipEmptyParts);
        for(i = 0; i < annsQL.size(); i++)
        {
            annQL = annsQL.at(i).split(QChar(20),QString::SkipEmptyParts); // split each entry
            if(annQL.size() < 2) continue; // no tag entries
            // first parts is always the time
            tempTS = (qint64) (annQL.at(0).toFloat()*UCRTCFREQ+(startDT.toMSecsSinceEpoch()-QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000);
            for(j = 1; j < annQL.size(); j++)
            {
                annOnsetTSQVp->append(tempTS);
                annTextQVp->append(annQL.at(j));
            }
        }
        dataRecCt++;
    }

    return dataRecCt;
}


/***
  * Figure out what kind of file type
  * Inputs:
  *     filename, name of file
  * Returns:
  *     -1, if error
  *     0, if unrecognized type
  *     1, if .opi
  *     2, if generic opi data .edf
  *     3, if eeg opi data .edf
  *     4, if ecg opi data .edf
  */
qint32 ConvertWindow::whatInType(QString filename)
{
    QFile *infilep;
    QDataStream *instrp;
    quint8 opiHdr[OPIHDRLEN];
    qint32 i;
    QString lpid, lrid;
    QDateTime startDT;
    qint32 numDataRecs, dataRecDur;
    qint32 numSignals;
    QVector<QString> labelSignalsQV, transTypeQV, physDimQV, prefiltQV;
    QVector<qint32> physMinQV, physMaxQV, digMinQV, digMaxQV;
    QVector<qint32> sampsPerDRQV;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(filename.trimmed());
    if (!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(filename));
        delete infilep;
        return -1;
    }

    // open input file stream
    instrp = new QDataStream(infilep);
    instrp->setByteOrder(QDataStream::LittleEndian);

    // check if it is opi
    for(i = 0; i < OPIHDRLEN; i++)
    {
        if(instrp->atEnd()) break;  // no data
        *instrp >> opiHdr[i];
    }
    if(instrp->atEnd())
    {
        infilep->close();
        delete instrp;
        delete infilep;
        return -1;
    }

    if(!((opiHdr[11] != 0x4F) || (opiHdr[12] != 0x50) || (opiHdr[13] != 0x49)
            || (opiHdr[14] != 0x55) || (opiHdr[15] != 0x43) || (opiHdr[16] != 0x44)))
    {
        ui->msgPTE->appendPlainText(QString("Input File is OPI raw data file"));
        infilep->close();
        delete instrp;
        delete infilep;
        return 1;
    }

    infilep->reset();
    // check if it is edf
    if(edfhdrread(instrp, &lpid, &lrid, &startDT, &numDataRecs, &dataRecDur,
                  &numSignals, &labelSignalsQV, &transTypeQV, &physDimQV, &physMinQV,
                  &physMaxQV, &digMinQV, &digMaxQV, &prefiltQV, &sampsPerDRQV) == 0)
    {
        // check if generic opi data
        if((numSignals == 7) && (dataRecDur == EDFDRDURSEC) &&
                (sampsPerDRQV.at(0) == 4096) &&
                (sampsPerDRQV.at(1) == 64) && (sampsPerDRQV.at(2) == 64) &&
                (sampsPerDRQV.at(3) == 256) && (sampsPerDRQV.at(4) == 64) &&
                (sampsPerDRQV.at(5) == 64) && ((sampsPerDRQV.at(6) == 64) || (sampsPerDRQV.at(6) == 30)))
        {
            ui->msgPTE->appendPlainText(QString("Input File is generic OPI EDF data file"));
            infilep->close();
            delete instrp;
            delete infilep;
            return 2;
        }
        if((numSignals == 11) && (dataRecDur == EDFDRDURSEC) &&
                (sampsPerDRQV.at(0) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(1) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(2) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(3) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(4) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(5) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(6) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(7) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(8) == EEGSAMPLERATE*EDFDRDURSEC) &&
                (sampsPerDRQV.at(9) == EEGSAMPLERATE*EDFDRDURSEC) &&
                ((sampsPerDRQV.at(10) == 64) || (sampsPerDRQV.at(10) == 30)))
        {
            ui->msgPTE->appendPlainText(QString("Input File is OPI EEG EDF data file"));
            infilep->close();
            delete instrp;
            delete infilep;
            return 3;
        }
        if((numSignals == 6) && (dataRecDur == EDFDRDURSEC) &&
                (sampsPerDRQV.at(0) == 256) &&
                (sampsPerDRQV.at(1) == 256) && (sampsPerDRQV.at(2) == 256) &&
                (sampsPerDRQV.at(3) == 256) && (sampsPerDRQV.at(4) == 256)
                && ((sampsPerDRQV.at(5) == 64) || (sampsPerDRQV.at(5) == 30)))
        {
            ui->msgPTE->appendPlainText(QString("Input File is OPI ECG EDF data file"));
            infilep->close();
            delete instrp;
            delete infilep;
            return 4;
        }
    }
    // cleanup
    infilep->close();
    delete instrp;
    delete infilep;

    return 0;   // if here, didn't pass any recognized tests
}


/***
  * Read EDF header information into pointed to qvectors.
  * Returns:
  *     -1, if error
  *     0, if successful
  */
qint32 ConvertWindow::edfhdrread(QDataStream *instrp, QString *lpidp, QString *lridp,
                                 QDateTime *startDTp, qint32 *numDataRecsp, qint32 *dataRecDurp,
                                 qint32 *numSignalsp, QVector<QString> *labelSignalsQVp,
                                 QVector<QString> *transTypeQVp, QVector<QString> *physDimQVp,
                                 QVector<qint32> *physMinQVp, QVector<qint32> *physMaxQVp,
                                 QVector<qint32> *digMinQVp, QVector<qint32> *digMaxQVp,
                                 QVector<QString> *prefiltQVp, QVector<qint32> *sampsPerDRQVp)
{
    qint32 i, hdrBytes;
    char buff[100];

    //8 ascii : version of this data format (0)
    if(instrp->readRawData(buff,8) < 0) return -1;
    if(QString::compare(QString::fromAscii(buff,8), "0       ")) return -1;

    //80 ascii : local patient identification
    if(instrp->readRawData(buff,80) < 0) return -1;
    *lpidp = QString::fromAscii(buff,80);

    //80 ascii : local recording identification.
    if(instrp->readRawData(buff,80) < 0) return -1;
    *lridp = QString::fromAscii(buff,80);

    //8 ascii : startdate of recording (dd.mm.yy),
    //+8 ascii : starttime of recording (hh.mm.ss).
    if(instrp->readRawData(buff,16) < 0) return -1;
    *startDTp = QDateTime::fromString(QString::fromAscii(buff,16),"dd.MM.yyHH.mm.ss");
    if(*startDTp < QDateTime::fromString("19850101","yyyyMMdd")) // if yy=13 should be 2013
        *startDTp = startDTp->addYears(100);

    //8 ascii : number of bytes in header record
    if(instrp->readRawData(buff,8) < 0) return -1;
    hdrBytes = QString::fromAscii(buff,8).toInt();

    //44 ascii : reserved
    if(instrp->readRawData(buff,44) < 0) return -1;
    if(QString::compare(QString::fromAscii(buff,44), QString("EDF+C").leftJustified(44,' ')))
        return -1;

    //8 ascii : number of data records (-1 if unknown)
    if(instrp->readRawData(buff,8) < 0) return -1;
    *numDataRecsp = QString::fromAscii(buff,8).toInt();

    //8 ascii : duration of a data record, in seconds
    if(instrp->readRawData(buff,8) < 0) return -1;
    *dataRecDurp = QString::fromAscii(buff,8).toInt();

    //4 ascii : number of signals (ns) in data record
    if(instrp->readRawData(buff,4) < 0) return -1;
    *numSignalsp = QString::fromAscii(buff,4).toInt();

    //ns * 16 ascii : ns * label
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,16) < 0) return -1;
        labelSignalsQVp->append(QString::fromAscii(buff,16));
    }

    //ns * 80 ascii : ns * transducer type (e.g. AgAgCl electrode)
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,80) < 0) return -1;
        transTypeQVp->append(QString::fromAscii(buff,80));
    }

    //ns * 8 ascii : ns * physical dimension (e.g. uV)
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,8) < 0) return -1;
        physDimQVp->append(QString::fromAscii(buff,8));
    }

    //ns * 8 ascii : ns * physical minimum (e.g. -500 or 34)
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,8) < 0) return -1;
        physMinQVp->append(QString::fromAscii(buff,8).toInt());
    }

    //ns * 8 ascii : ns * physical maximum (e.g. 500 or 40)
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,8) < 0) return -1;
        physMaxQVp->append(QString::fromAscii(buff,8).toInt());
    }

    //ns * 8 ascii : ns * digital minimum (e.g. -2048)
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,8) < 0) return -1;
        digMinQVp->append(QString::fromAscii(buff,8).toInt());
    }

    //ns * 8 ascii : ns * digital maximum (e.g. 2047)
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,8) < 0) return -1;
        digMaxQVp->append(QString::fromAscii(buff,8).toInt());
    }

    //ns * 80 ascii : ns * prefiltering (e.g. HP:0.1Hz LP:75Hz)
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,80) < 0) return -1;
        prefiltQVp->append(QString::fromAscii(buff,8));
    }

    //ns * 8 ascii : ns * nr of samples in each data record
    for(i = 0; i < (*numSignalsp); i++)
    {
        if(instrp->readRawData(buff,8) < 0) return -1;
        sampsPerDRQVp->append(QString::fromAscii(buff,8).toInt());
    }

    //ns * 32 ascii : ns * reserved
    for(i = 0; i < (*numSignalsp); i++)
        if(instrp->readRawData(buff,32) < 0) return -1;

    return 0;   // if got here, then read all of edf header
}


/***
  * Delete qvectors
  */
void ConvertWindow::delQVs(QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                           QVector<quint8> **batQVpp, QVector<qint16> **adcQVpp,
                           QVector<qint16> **tmpQVpp, QVector<qint16> **axQVpp,
                           QVector<qint16> **ayQVpp, QVector<qint16> **azQVpp,
                           QVector<qint16> **sqQVpp, QVector<quint8> **edQVpp)
{
    qint32 pdnSlot;

    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        if(tsQVpp[pdnSlot] != 0)
        {
            delete tsQVpp[pdnSlot];
            tsQVpp[pdnSlot] = 0;
        }
        if(skpQVpp[pdnSlot] != 0)
        {
            delete skpQVpp[pdnSlot];
            skpQVpp[pdnSlot] = 0;
        }
        if(batQVpp[pdnSlot] != 0)
        {
            delete batQVpp[pdnSlot];
            batQVpp[pdnSlot] = 0;
        }
        if(adcQVpp[pdnSlot] != 0)
        {
            delete adcQVpp[pdnSlot];
            adcQVpp[pdnSlot] = 0;
        }
        if(tmpQVpp[pdnSlot] != 0)
        {
            delete tmpQVpp[pdnSlot];
            tmpQVpp[pdnSlot] = 0;
        }
        if(axQVpp[pdnSlot] != 0)
        {
            delete axQVpp[pdnSlot];
            axQVpp[pdnSlot] = 0;
        }
        if(ayQVpp[pdnSlot] != 0)
        {
            delete ayQVpp[pdnSlot];
            ayQVpp[pdnSlot] = 0;
        }
        if(azQVpp[pdnSlot] != 0)
        {
            delete azQVpp[pdnSlot];
            azQVpp[pdnSlot] = 0;
        }
        if(sqQVpp[pdnSlot] != 0)
        {
            delete sqQVpp[pdnSlot];
            sqQVpp[pdnSlot] = 0;
        }
        if(edQVpp[pdnSlot] != 0)
        {
            delete edQVpp[pdnSlot];
            edQVpp[pdnSlot] = 0;
        }
    }
}


/***
  * Delete sepcific set of qvectors
  */
void ConvertWindow::delspecQVs(qint32 pdnSlot,
                               QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                               QVector<quint8> **batQVpp, QVector<qint16> **adcQVpp,
                               QVector<qint16> **tmpQVpp, QVector<qint16> **axQVpp,
                               QVector<qint16> **ayQVpp, QVector<qint16> **azQVpp,
                               QVector<qint16> **sqQVpp, QVector<quint8> **edQVpp)
{
    if(tsQVpp[pdnSlot] != 0)
    {
        delete tsQVpp[pdnSlot];
        tsQVpp[pdnSlot] = 0;
    }
    if(skpQVpp[pdnSlot] != 0)
    {
        delete skpQVpp[pdnSlot];
        skpQVpp[pdnSlot] = 0;
    }
    if(batQVpp[pdnSlot] != 0)
    {
        delete batQVpp[pdnSlot];
        batQVpp[pdnSlot] = 0;
    }
    if(adcQVpp[pdnSlot] != 0)
    {
        delete adcQVpp[pdnSlot];
        adcQVpp[pdnSlot] = 0;
    }
    if(tmpQVpp[pdnSlot] != 0)
    {
        delete tmpQVpp[pdnSlot];
        tmpQVpp[pdnSlot] = 0;
    }
    if(axQVpp[pdnSlot] != 0)
    {
        delete axQVpp[pdnSlot];
        axQVpp[pdnSlot] = 0;
    }
    if(ayQVpp[pdnSlot] != 0)
    {
        delete ayQVpp[pdnSlot];
        ayQVpp[pdnSlot] = 0;
    }
    if(azQVpp[pdnSlot] != 0)
    {
        delete azQVpp[pdnSlot];
        azQVpp[pdnSlot] = 0;
    }
    if(sqQVpp[pdnSlot] != 0)
    {
        delete sqQVpp[pdnSlot];
        sqQVpp[pdnSlot] = 0;
    }
    if(edQVpp[pdnSlot] != 0)
    {
        delete edQVpp[pdnSlot];
        edQVpp[pdnSlot] = 0;
    }
}


/***
  * Conversion to wav output files
  * Will automatically write files
  * Inputs:
  *     None
  * Returns:
  *     true, if successful
  *     false, if not successful
  */
bool ConvertWindow::convertopitowav()
{
    QString outFileName, outDirName;
    QFile *infilep, *outfile1p, *outfile2p, *outfilem1p;
    QDataStream *instrp;
    QDataStream *out1strp, *out2strp, *outm1strp;
    QString datetimestr, tempqstr;
    QDateTime firstFrmDT, stDT, endDT;
    quint8 pdnListp[PDNLISTLEN];
    QVector<qint64> *tsQVpp[PDNLISTLEN];
    QVector<quint8> *skpQVpp[PDNLISTLEN];
    QVector<quint8> *batQVpp[PDNLISTLEN];
    QVector<qint16> *adcQVpp[PDNLISTLEN];
    QVector<qint16> *axQVpp[PDNLISTLEN];
    QVector<qint16> *ayQVpp[PDNLISTLEN];
    QVector<qint16> *azQVpp[PDNLISTLEN];
    QVector<qint16> *tmpQVpp[PDNLISTLEN];
    QVector<qint16> *sqQVpp[PDNLISTLEN];
    QVector<quint8> *edQVpp[PDNLISTLEN];
    QVector<qint64> annOnsetTSQV, annOnsetTS2QV;
    QVector<QString> annTextQV, annText2QV;
    QVector<qint32> adcECGindQV;
    QVector<bool> missBeatFlagQV;
    QVector<qint16> ecgRRQV;
    QVector<qint16> ecgampQV;
    QVector<qint16> eegM2QV;
    QVector<qint16> eegM1QV;
    QVector<qint16> eegG2QV;
    QVector<qint16> eegG1QV;
    QVector<qint16> eegUPQV;
    QVector<qint16> eegBetaQV;
    QVector<qint16> eegSigmaQV;
    QVector<qint16> eegalphaQV;
    QVector<qint16> eegthetaQV;
    QVector<qint16> eegdeltaQV;
    ConvertOptionsWindow *coWinp;
    qint32 i, k;
    quint8 opiHdr[OPIHDRLEN];
    qint32 pdnSlot;
    quint8 pdnDes;
    qint8 outDataType;
    bool useProfile;
    double beginOffFrms, beginOffFrmsj;
    qint32 beginOffFrmsi;
    qint64 firstFrmTS;
    QMessageBox msgBox;
    bool writeRRfile=false;
    int tempPavg,tempDavg;
    int tempDlast;
    QString outFileNameDtoMecg;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(ui->inLE->text().trimmed());
    if (!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(ui->inLE->text()));
        delete infilep;
        return false;
    }

    // open input file stream
    instrp = new QDataStream(infilep);

    // get header information
    for(i = 0; i < OPIHDRLEN; i++) *instrp >> opiHdr[i];
    if((opiHdr[11] != 0x4F) || (opiHdr[12] != 0x50) || (opiHdr[13] != 0x49)
            || (opiHdr[14] != 0x55) || (opiHdr[15] != 0x43) || (opiHdr[16] != 0x44))
    {
        ui->msgPTE->appendPlainText(QString("Input File is not an OPI raw data file"));
        infilep->close();
        delete instrp;
        delete infilep;
        return false;
    }

    // copy pdn's
    for(i = 0; i < PDNLISTLEN; i++)
        pdnListp[i] = opiHdr[20+i];

    // init qvector pointers to null
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        tsQVpp[pdnSlot] = 0;
        skpQVpp[pdnSlot] = 0;
        batQVpp[pdnSlot] = 0;
        adcQVpp[pdnSlot] = 0;
        tmpQVpp[pdnSlot] = 0;
        axQVpp[pdnSlot] = 0;
        ayQVpp[pdnSlot] = 0;
        azQVpp[pdnSlot] = 0;
        sqQVpp[pdnSlot] = 0;
        edQVpp[pdnSlot] = 0;
    }

    // convert all pdn's in pdn list that are not 0xFF and put data in init'd QVectors
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        pdnDes = pdnListp[pdnSlot];    // PDN to convert
        if(pdnDes == 0xFF) // skip over this since no device, go onto next one
        {
            delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            continue;
        }

        // reset file and variables to prepare for reading
        infilep->reset();

        // initialize qvectors
        tsQVpp[pdnSlot] = new QVector<qint64>(0);
        skpQVpp[pdnSlot] = new QVector<quint8>(0);
        batQVpp[pdnSlot] = new QVector<quint8>(0);
        adcQVpp[pdnSlot] = new QVector<qint16>(0);
        tmpQVpp[pdnSlot] = new QVector<qint16>(0);
        axQVpp[pdnSlot] = new QVector<qint16>(0);
        ayQVpp[pdnSlot] = new QVector<qint16>(0);
        azQVpp[pdnSlot] = new QVector<qint16>(0);
        sqQVpp[pdnSlot] = new QVector<qint16>(0);
        edQVpp[pdnSlot] = new QVector<quint8>(0);

        if(!opiDread(instrp, pdnDes, tsQVpp[pdnSlot], skpQVpp[pdnSlot], batQVpp[pdnSlot],
                     adcQVpp[pdnSlot], tmpQVpp[pdnSlot], axQVpp[pdnSlot], ayQVpp[pdnSlot],
                     azQVpp[pdnSlot], sqQVpp[pdnSlot], edQVpp[pdnSlot]))
        {
            delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            continue; // no data, didn't use any frames
        }
    }

    // At this point, input file is not needed anymore
    infilep->close();
    delete infilep;
    delete instrp;

    // check that there is actually usable data, if not exit
    if((!tsQVpp[0]) && (!tsQVpp[1]) && (!tsQVpp[2]) && (!tsQVpp[3]) &&
            (!tsQVpp[4]) && (!tsQVpp[5]) && (!tsQVpp[6]) && (!tsQVpp[7]))
    {
        ui->msgPTE->appendPlainText("No usable data in files... ");
        return false;   // user terminates so don't do anything else
    }

    // go to converter options, once it returns, data has been processed and only needs
    // to be written to file
    coWinp = new ConvertOptionsWindow(&outDirName, pdnListp, &outDataType, &alpha, &Sigma, tsQVpp, skpQVpp,
                                      batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp,
                                      sqQVpp, edQVpp, &annOnsetTSQV, &annTextQV, OPITOWAV, &useProfile, this,&writeRRfile);

    if(coWinp->exec() == QDialog::Rejected)
    {
        ui->msgPTE->appendPlainText("User aborted... ");
        delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
        delete coWinp;
        return false;   // user terminates so don't do anything else
    }
    delete coWinp;

    stDT = QDateTime::currentDateTime();
    // Write output files
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        if(tsQVpp[pdnSlot] == 0) continue;  // no data
        pdnDes = pdnListp[pdnSlot];    // PDN to convert
        // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
        firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((tsQVpp[pdnSlot]->at(0))*1000/UCRTCFREQ);
        // Need floor(time) for file since fill in, toString function automatically does this
        datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");
        qDebug() << firstFrmDT.toString("yyyyMMdd_hhmmss.zzz");

        // Output file stuff
        outFileName = outDirName;
        outFileName.append(QString("D%1_%2").arg(datetimestr).arg(pdnDes).append("_1.wav"));
        outfile1p = new QFile(outFileName);
        outFileName = outDirName;
        outFileName.append(QString("D%1_%2").arg(datetimestr).arg(pdnDes).append("_2.wav"));
        outfile2p = new QFile(outFileName);

        if(outfile1p->exists() || outfile2p->exists())
        {
            msgBox.setWindowTitle("Out File Exists");
            msgBox.setText("Out file already exists in selected directory.");
            msgBox.setInformativeText("Do you want to overwrite?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            if(msgBox.exec() == QMessageBox::Cancel)
            {
                delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
                delete outfile1p;
                return false;
            }
        }
        if (!outfile1p->open(QIODevice::WriteOnly) ||
                !outfile2p->open(QIODevice::WriteOnly))
        {
            ui->msgPTE->appendPlainText(QString("PDN#%1: Problem with output files").arg(pdnDes));
            delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            delete outfile1p;
            delete outfile2p;
            continue;
        }

        out1strp = new QDataStream(outfile1p);
        out2strp = new QDataStream(outfile2p);
        out1strp->setByteOrder(QDataStream::LittleEndian);
        out2strp->setByteOrder(QDataStream::LittleEndian);

        // add data at beginning (first frame) to fill up to floor(second) (within 2 ms)
        // difference between floored exact second and first time in frames
        beginOffFrms = ((double) (firstFrmDT.toMSecsSinceEpoch() % 1000))*512.0/1000/ADCLEN;
        beginOffFrmsi = (qint32) (beginOffFrms);
        beginOffFrmsj = beginOffFrms-((double) beginOffFrmsi);
        // put in default values for beginning in constant frame units
        firstFrmTS = tsQVpp[pdnSlot]->at(0);
        for(; beginOffFrmsi > 0; beginOffFrmsi--)
        {
            tsQVpp[pdnSlot]->prepend(firstFrmTS-512*beginOffFrmsi);
            skpQVpp[pdnSlot]->prepend(0);
            batQVpp[pdnSlot]->prepend(1);
            sqQVpp[pdnSlot]->prepend(4);
            edQVpp[pdnSlot]->prepend(0);
            for(k = 0; k < ADCLEN; k++)
                adcQVpp[pdnSlot]->prepend(0);
            for(k = 0; k < TMPLEN; k++)
                tmpQVpp[pdnSlot]->prepend(1024);   // temp ~25
            for(k = 0; k < ACCLEN/4; k++)
            {
                axQVpp[pdnSlot]->prepend(0);
                ayQVpp[pdnSlot]->prepend(0);
            }
            for(k = 0; k < ACCLEN; k++)
                azQVpp[pdnSlot]->prepend(0);
        }

        // add less than a frame data
        if(beginOffFrmsj > 0.5)
        {
            tsQVpp[pdnSlot]->prepend(firstFrmTS-512*(beginOffFrmsi+1));
            skpQVpp[pdnSlot]->prepend(0);
            batQVpp[pdnSlot]->prepend(1);
            sqQVpp[pdnSlot]->prepend(4);
            edQVpp[pdnSlot]->prepend(0);
            tmpQVpp[pdnSlot]->prepend(1024);   // temp ~25
            axQVpp[pdnSlot]->prepend(0);
            ayQVpp[pdnSlot]->prepend(0);
        }
        for(k = 0; k < ((qint32) (beginOffFrmsj*ADCLEN)); k++)
            adcQVpp[pdnSlot]->prepend(0);
        for(k = 0; k < ((qint32) (beginOffFrmsj*ACCLEN)); k++)
            azQVpp[pdnSlot]->prepend(0);

        // fix up tag times by adding
        annOnsetTS2QV = annOnsetTSQV;
        annText2QV = annTextQV;
        for(k = 0; k < annOnsetTS2QV.size(); k++)
            annOnsetTS2QV.replace(k, annOnsetTS2QV.at(k)+((qint64) (beginOffFrms*512.0)));

        // Add a "DataStart" and "DataEnd" tag
        annOnsetTS2QV.prepend(tsQVpp[pdnSlot]->at(0)+((qint64) (beginOffFrms*512.0)));
        annText2QV.prepend("DataStart");
        annOnsetTS2QV.append(tsQVpp[pdnSlot]->at(tsQVpp[pdnSlot]->size()-1)+((qint64) (beginOffFrms*512.0)));
        annText2QV.append("DataEnd");

        // Wav header
        wavhdrwrite(out1strp, TSRTCFREQ, 1, adcQVpp[pdnSlot]->size(), 16);
        wavhdrwrite(out2strp, TSRTCFREQ/16, 5, azQVpp[pdnSlot]->size(), 16);

        // stuff in all the data according to specified format
        wavDadcwrite(out1strp, adcQVpp[pdnSlot]);
        wavDallelsewrite(out2strp, tmpQVpp[pdnSlot], axQVpp[pdnSlot], ayQVpp[pdnSlot],
                         azQVpp[pdnSlot], sqQVpp[pdnSlot]);

        // can close these files now, data written
        outfile1p->close();
        outfile2p->close();
        delete out1strp;
        delete out2strp;
        delete outfile1p;
        delete outfile2p;

        // Show done
        outFileName = outDirName;
        outFileName.append(QString("D%1_%2_<1,2>").arg(datetimestr).arg(pdnDes).append(".wav"));
        ui->msgPTE->appendPlainText(QString("PDN#%1: WAV Conversion Finished, Saved in %2").arg(pdnDes).arg(outFileName));

        // Meta data processing
        if(outDataType == ECGMDATATYPE)
        {
            adcECGindQV = findHRmax(adcQVpp[pdnSlot], TSRTCFREQ, adcQVpp[pdnSlot]->size()/TSRTCFREQ, &missBeatFlagQV,true,true,&tempPavg,&tempDavg,&tempDlast);
            if(adcECGindQV.size() == 0)
                ui->msgPTE->appendPlainText("No heartbeats found");
            else
            {
                outFileName = outDirName;
                outFileName.append(QString("D%1_%2_ECG.wav").arg(datetimestr).arg(pdnDes));
                outFileNameDtoMecg = outDirName;
                outFileNameDtoMecg.append(QString("D%1_%2_RR.txt").arg(datetimestr).arg(pdnDes));

                outfilem1p = new QFile(outFileName);

                if(outfilem1p->exists())
                {
                    msgBox.setWindowTitle("Out File Exists");
                    msgBox.setText("Out file already exists in selected directory.");
                    msgBox.setInformativeText("Do you want to overwrite?");
                    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                    if(msgBox.exec() == QMessageBox::Cancel)
                    {
                        delete outfilem1p;
                        return false;
                    }
                }
                if(!outfilem1p->open(QIODevice::WriteOnly))
                {
                    ui->msgPTE->appendPlainText(QString("Problem with creating metadata output file %1").arg(outFileName));
                    delete outfilem1p;
                }
                else
                {
                    outm1strp = new QDataStream(outfilem1p);
                    outm1strp->setByteOrder(QDataStream::LittleEndian);
                    //32Hz
                    DtoMecgQVs(&adcECGindQV, adcQVpp[pdnSlot], &missBeatFlagQV, &ecgRRQV, &ecgampQV, 16);
                    if(writeRRfile)
                    writetoFile(outFileNameDtoMecg,&adcECGindQV,512);
                    wavhdrwrite(outm1strp, TSRTCFREQ/16, 5, ecgRRQV.size(), 16); // 32Hz
                    wavMecgwrite(outm1strp, &ecgRRQV, &ecgampQV, 16);
                    ui->msgPTE->appendPlainText(QString("Created metadata output file %1").arg(outFileName));
                    outfilem1p->close();
                    delete outm1strp;
                    delete outfilem1p;
                }
            }
        }
        else if(outDataType == EEGMDATATYPE)
        {
            outFileName = outDirName;
            outFileName.append(QString("D%1_%2_EEG.wav").arg(datetimestr).arg(pdnDes));
            outfilem1p = new QFile(outFileName);
            if(outfilem1p->exists())
            {
                msgBox.setWindowTitle("Out File Exists");
                msgBox.setText("Out file already exists in selected directory.");
                msgBox.setInformativeText("Do you want to overwrite?");
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                if(msgBox.exec() == QMessageBox::Cancel)
                {
                    delete outfilem1p;
                    return false;
                }
            }
            if(!outfilem1p->open(QIODevice::WriteOnly))
            {
                ui->msgPTE->appendPlainText(QString("Problem with creating metadata output file %1").arg(outFileName));
                delete outfilem1p;
            }
            else
            {
                outm1strp = new QDataStream(outfilem1p);
                outm1strp->setByteOrder(QDataStream::LittleEndian);


                eegM2QV.clear();
                eegM1QV.clear();
                eegG2QV.clear();
                eegG1QV.clear();
                eegUPQV.clear();
                eegBetaQV.clear();
                eegSigmaQV.clear();
                eegalphaQV.clear();
                eegthetaQV.clear();
                eegdeltaQV.clear();
                wavhdrwrite(outm1strp, 1, 10, adcQVpp[pdnSlot]->size()/TSRTCFREQ, 16); // 1Hz

                DtoMeegQVs(adcQVpp[pdnSlot],
                           &eegM2QV,&eegM1QV,
                           &eegG2QV,&eegG1QV,
                           &eegUPQV,&eegBetaQV,
                           &eegSigmaQV,&eegalphaQV,
                           &eegthetaQV,&eegdeltaQV);

                wavMeegwrite(outm1strp,&eegM2QV,&eegM1QV,
                             &eegG2QV,&eegG1QV,
                             &eegUPQV,&eegBetaQV,
                             &eegSigmaQV,&eegalphaQV,
                             &eegthetaQV,&eegdeltaQV,
                             adcQVpp[pdnSlot]->size()/TSRTCFREQ);

                ui->msgPTE->appendPlainText(QString("Created metadata output file %1").arg(outFileName));
                outfilem1p->close();
                delete outm1strp;
                delete outfilem1p;
            }
        }

    } // for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)

    endDT = QDateTime::currentDateTime();

    // Show done
    ui->msgPTE->appendPlainText(QString(">> Conversion Finished in %1 seconds").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));

    // cleanup
    delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);

    // if got here, conversion done properly
    return true;
}


/***
  * Conversion from edf to wav output files
  * Will automatically write files
  * Inputs:
  *     None
  * Returns:
  *     true, if successful
  *     false, if not successful
  */
bool ConvertWindow::convertedfDtowav()
{
    QString outFileName, outDirName;
    QFile *infilep, *outfile1p, *outfile2p, *outfilem1p;
    QDataStream *instrp;
    QDataStream *out1strp, *out2strp, *outm1strp;
    QString datetimestr, tempqstr;
    QDateTime firstFrmDT, stDT, endDT;
    quint8 pdnListp[PDNLISTLEN];
    QVector<qint16> *adcQVpp[PDNLISTLEN];
    QVector<qint16> *axQVpp[PDNLISTLEN];
    QVector<qint16> *ayQVpp[PDNLISTLEN];
    QVector<qint16> *azQVpp[PDNLISTLEN];
    QVector<qint16> *tmpQVpp[PDNLISTLEN];
    QVector<qint16> *sqQVpp[PDNLISTLEN];
    QVector<qint64> *tsQVpp[PDNLISTLEN];
    QVector<quint8> *skpQVpp[PDNLISTLEN];
    QVector<quint8> *batQVpp[PDNLISTLEN];
    QVector<quint8> *edQVpp[PDNLISTLEN];
    QVector<qint64> annOnsetTSQV;
    QVector<QString> annTextQV;
    QVector<qint32> adcECGindQV;
    QVector<bool> missBeatFlagQV;
    QVector<qint16> ecgRRQV;
    QVector<qint16> ecgampQV;
    QVector<qint16> eegM2QV;
    QVector<qint16> eegM1QV;
    QVector<qint16> eegG2QV;
    QVector<qint16> eegG1QV;
    QVector<qint16> eegUPQV;
    QVector<qint16> eegBetaQV;
    QVector<qint16> eegSigmaQV;
    QVector<qint16> eegalphaQV;
    QVector<qint16> eegthetaQV;
    QVector<qint16> eegdeltaQV;
    ConvertOptionsWindow *coWinp;
    QString lpid, lrid;
    QDateTime startDT;
    qint32 numDataRecs, dataRecDur;
    qint32 numSignals;
    QVector<QString> labelSignalsQV, transTypeQV, physDimQV, prefiltQV;
    QVector<qint32> physMinQV, physMaxQV, digMinQV, digMaxQV;
    QVector<qint32> sampsPerDRQV;
    qint32 pdnSlot, i;
    qint32 pdnDes;
    qint8 outDataType;
    bool useProfile;
    QMessageBox msgBox;
    bool writeRRfile=false;
    int tempPavg,tempDavg;
    int tempDlast;
    QString outFileNameDtoMecg;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(ui->inLE->text());
    if (!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(ui->inLE->text()));
        delete infilep;
        return false;
    }

    // open input file stream
    instrp = new QDataStream(infilep);
    instrp->setByteOrder(QDataStream::LittleEndian);

    // get header information
    edfhdrread(instrp, &lpid, &lrid, &startDT, &numDataRecs, &dataRecDur,
               &numSignals, &labelSignalsQV, &transTypeQV, &physDimQV, &physMinQV,
               &physMaxQV, &digMinQV, &digMaxQV, &prefiltQV, &sampsPerDRQV);

    // init qvector pointers to null
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        tsQVpp[pdnSlot] = 0;
        skpQVpp[pdnSlot] = 0;
        batQVpp[pdnSlot] = 0;
        adcQVpp[pdnSlot] = 0;
        tmpQVpp[pdnSlot] = 0;
        axQVpp[pdnSlot] = 0;
        ayQVpp[pdnSlot] = 0;
        azQVpp[pdnSlot] = 0;
        sqQVpp[pdnSlot] = 0;
        edQVpp[pdnSlot] = 0;
    }

    // get PDN from Device field
    pdnDes = getPDNlrid(lrid);

    pdnSlot = 0;

    // copy pdn's
    for(i = 0; i < PDNLISTLEN; i++)
        pdnListp[i] = 0xFF;
    pdnListp[pdnSlot] = pdnDes;

    // initialize qvectors
    tsQVpp[pdnSlot] = new QVector<qint64>(0);
    skpQVpp[pdnSlot] = new QVector<quint8>(0);
    batQVpp[pdnSlot] = new QVector<quint8>(0);
    adcQVpp[pdnSlot] = new QVector<qint16>(0);
    tmpQVpp[pdnSlot] = new QVector<qint16>(0);
    axQVpp[pdnSlot] = new QVector<qint16>(0);
    ayQVpp[pdnSlot] = new QVector<qint16>(0);
    azQVpp[pdnSlot] = new QVector<qint16>(0);
    sqQVpp[pdnSlot] = new QVector<qint16>(0);
    edQVpp[pdnSlot] = new QVector<quint8>(0);

    if(edfDread(instrp, startDT, numDataRecs, dataRecDur, numSignals,
                sampsPerDRQV, tsQVpp[pdnSlot], skpQVpp[pdnSlot], batQVpp[pdnSlot],
                adcQVpp[pdnSlot], tmpQVpp[pdnSlot], axQVpp[pdnSlot], ayQVpp[pdnSlot],
                azQVpp[pdnSlot], sqQVpp[pdnSlot], edQVpp[pdnSlot],
                &annOnsetTSQV, &annTextQV) < 0)
    {
        delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
    }

    // At this point, input file is not needed anymore
    infilep->close();
    delete infilep;
    delete instrp;

    // check that there is actually usable data, if not exit
    if((!tsQVpp[0]) && (!tsQVpp[1]) && (!tsQVpp[2]) && (!tsQVpp[3]) &&
            (!tsQVpp[4]) && (!tsQVpp[5]) && (!tsQVpp[6]) && (!tsQVpp[7]))
    {
        ui->msgPTE->appendPlainText("No usable data in files... ");
        return false;   // user terminates so don't do anything else
    }

    // go to converter options, once it returns, data has been processed and only needs
    // to be written to file
    coWinp = new ConvertOptionsWindow(&outDirName, pdnListp, &outDataType, &alpha, &Sigma, tsQVpp, skpQVpp,
                                      batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp,
                                      sqQVpp, edQVpp, &annOnsetTSQV, &annTextQV,EDFDTOWAV, &useProfile, this,&writeRRfile);
    if(coWinp->exec() == QDialog::Rejected)
    {
        ui->msgPTE->appendPlainText("User aborted... ");
        delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
        delete coWinp;
        return false;   // user terminates so don't do anything else
    }
    delete coWinp;

    stDT = QDateTime::currentDateTime();
    // Write output files
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((tsQVpp[pdnSlot]->at(0))*1000/UCRTCFREQ);
    datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");

    // Output file stuff
    outFileName = outDirName;
    outFileName.append(QString("D%1_%2").arg(datetimestr).arg(pdnDes).append("_1.wav"));
    outfile1p = new QFile(outFileName);
    outFileName = outDirName;
    outFileName.append(QString("D%1_%2").arg(datetimestr).arg(pdnDes).append("_2.wav"));
    outfile2p = new QFile(outFileName);
    if(outfile1p->exists() || outfile2p->exists())
    {
        msgBox.setWindowTitle("Out File Exists");
        msgBox.setText("Out file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            delete outfile1p;
            delete outfile2p;
            return false;
        }
    }
    if (!outfile1p->open(QIODevice::WriteOnly) ||
            !outfile2p->open(QIODevice::WriteOnly))
    {
        ui->msgPTE->appendPlainText(QString("PDN#%1: Problem with output files").arg(pdnDes));
        delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
        delete outfile1p;
        delete outfile2p;
        return false;
    }

    out1strp = new QDataStream(outfile1p);
    out2strp = new QDataStream(outfile2p);
    out1strp->setByteOrder(QDataStream::LittleEndian);
    out2strp->setByteOrder(QDataStream::LittleEndian);

    // Wav header
    wavhdrwrite(out1strp, TSRTCFREQ, 1, adcQVpp[pdnSlot]->size(), 16);
    wavhdrwrite(out2strp, TSRTCFREQ/16, 5, azQVpp[pdnSlot]->size(), 16);

    // stuff in all the data according to specified format
    wavDadcwrite(out1strp, adcQVpp[pdnSlot]);
    wavDallelsewrite(out2strp, tmpQVpp[pdnSlot], axQVpp[pdnSlot], ayQVpp[pdnSlot],
                     azQVpp[pdnSlot], sqQVpp[pdnSlot]);

    // can close these files now, data written
    outfile1p->close();
    outfile2p->close();
    delete out1strp;
    delete out2strp;
    delete outfile1p;
    delete outfile2p;

    // Show done
    outFileName = outDirName;
    outFileName.append(QString("D%1_%2_<1,2>").arg(datetimestr).arg(pdnDes).append(".wav"));
    ui->msgPTE->appendPlainText(QString("PDN#%1: WAV Conversion Finished, Saved in %2").arg(pdnDes).arg(outFileName));

    // Meta data processing
    if(outDataType == ECGMDATATYPE)
    {
        adcECGindQV = findHRmax(adcQVpp[pdnSlot], TSRTCFREQ, adcQVpp[pdnSlot]->size()/TSRTCFREQ, &missBeatFlagQV,true,true,&tempPavg,&tempDavg,&tempDlast);
        if(adcECGindQV.size() == 0)
            ui->msgPTE->appendPlainText("No heartbeats found");
        else
        {
            outFileName = outDirName;
            outFileName.append(QString("D%1_%2_ECG.wav").arg(datetimestr).arg(pdnDes));
            outFileNameDtoMecg = outDirName;
            outFileNameDtoMecg.append(QString("D%1_%2_RR.txt").arg(datetimestr).arg(pdnDes));

            outfilem1p = new QFile(outFileName);
            if(outfilem1p->exists())
            {
                msgBox.setWindowTitle("Out File Exists");
                msgBox.setText("Out file already exists in selected directory.");
                msgBox.setInformativeText("Do you want to overwrite?");
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                if(msgBox.exec() == QMessageBox::Cancel)
                {
                    delete outfilem1p;
                    return false;
                }
            }
            if(!outfilem1p->open(QIODevice::WriteOnly))
            {
                ui->msgPTE->appendPlainText(QString("Problem with creating metadata output file %1").arg(outFileName));
                delete outfilem1p;
            }
            else
            {
                outm1strp = new QDataStream(outfilem1p);
                outm1strp->setByteOrder(QDataStream::LittleEndian);
                //32Hz
                DtoMecgQVs(&adcECGindQV, adcQVpp[pdnSlot], &missBeatFlagQV, &ecgRRQV, &ecgampQV, 16);
                if(writeRRfile)
                writetoFile(outFileNameDtoMecg,&adcECGindQV,512);
                wavhdrwrite(outm1strp, TSRTCFREQ/16, 5, ecgRRQV.size(), 16); // 32Hz
                wavMecgwrite(outm1strp, &ecgRRQV, &ecgampQV, 16);
                ui->msgPTE->appendPlainText(QString("Created metadata output file %1").arg(outFileName));
                outfilem1p->close();
                delete outm1strp;
                delete outfilem1p;
            }
        }
    }
    else if(outDataType == EEGMDATATYPE)
    {
        outFileName = outDirName;
        outFileName.append(QString("D%1_%2_EEG.wav").arg(datetimestr).arg(pdnDes));
        outfilem1p = new QFile(outFileName);
        if(outfilem1p->exists())
        {
            msgBox.setWindowTitle("Out File Exists");
            msgBox.setText("Out file already exists in selected directory.");
            msgBox.setInformativeText("Do you want to overwrite?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            if(msgBox.exec() == QMessageBox::Cancel)
            {
                delete outfilem1p;
                return false;
            }
        }
        if(!outfilem1p->open(QIODevice::WriteOnly))
        {
            ui->msgPTE->appendPlainText(QString("Problem with creating metadata output file %1").arg(outFileName));
            delete outfilem1p;
        }
        else
        {
            outm1strp = new QDataStream(outfilem1p);
            outm1strp->setByteOrder(QDataStream::LittleEndian);

            eegM2QV.clear();
            eegM1QV.clear();
            eegG2QV.clear();
            eegG1QV.clear();
            eegUPQV.clear();
            eegBetaQV.clear();
            eegSigmaQV.clear();
            eegalphaQV.clear();
            eegthetaQV.clear();
            eegdeltaQV.clear();

            DtoMeegQVs(adcQVpp[pdnSlot],
                       &eegM2QV,&eegM1QV,
                       &eegG2QV,&eegG1QV,
                       &eegUPQV,&eegBetaQV,
                       &eegSigmaQV,&eegalphaQV,
                       &eegthetaQV,&eegdeltaQV);

            wavhdrwrite(outm1strp, 1, 10, eegM2QV.size(), 16); // 1Hz

            wavMeegwrite(outm1strp,&eegM2QV,&eegM1QV,
                         &eegG2QV,&eegG1QV,
                         &eegUPQV,&eegBetaQV,
                         &eegSigmaQV,&eegalphaQV,
                         &eegthetaQV,&eegdeltaQV,
                         eegM2QV.size());

            ui->msgPTE->appendPlainText(QString("Created metadata output file %1").arg(outFileName));
            outfilem1p->close();
            delete outm1strp;
            delete outfilem1p;
        }
    }

    endDT = QDateTime::currentDateTime();

    // Show done
    ui->msgPTE->appendPlainText(QString(">> Conversion Finished in %1 seconds").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));

    // cleanup
    delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);

    // if got here, conversion done properly
    return true;
}



void ConvertWindow::DtoMeegQVs(QVector<qint16> *adcqvectQvp,
                               QVector<qint16> *M2Qvp, QVector<qint16> *M1Qvp,
                               QVector<qint16> *G2Qvp, QVector<qint16> *G1Qvp,
                               QVector<qint16> *UPQvp, QVector<qint16> *BetaQvp,
                               QVector<qint16> *SigmaQvp, QVector<qint16> *alphaQvp,
                               QVector<qint16> *thetaQvp, QVector<qint16> *deltaQvp)
{
    float saveFFTstd[FFTSIZE];
    int countfftdata=0;
    QFourierTransformer transformer;  //should Setting a fixed size for the transformation
    transformer.setSize(FFTSIZE);
    QVector <QComplexFloat> fftresult;
    QVector <float>  amplitudeFFT(FFTSIZE);  //save the final fft result
    double re,im;
    float calculatedFFTstd[FFTSIZE];
    qint16 tempfftdata;  //save the fft value
    float  tempsavetotalfft; //save the .... data
    qint32 i, j, k;

    for(i=0;i<adcqvectQvp->size()/FFTOVERLAP;i++)  //count seconds: use actual vector size
    {
        countfftdata=0;
        for( k=i*FFTOVERLAP;k<i*FFTOVERLAP+FFTSIZE;k++)  //count every samples
        {
            if(k<adcqvectQvp->size())
                saveFFTstd[countfftdata]=0.5*(1.0-qCos((2.0*M_PI*countfftdata)/(FFTSIZE-1)))*(((double)adcqvectQvp->at(k)));
            else
                saveFFTstd[countfftdata]=(0);
            countfftdata++;
        }//count every samples end

        //start calculate
        //calculation for fft
        transformer.forwardTransform(saveFFTstd,calculatedFFTstd);
        fftresult=transformer.toComplex(calculatedFFTstd);
        for(j = 0; j <= FFTSIZE/2; j++)
        {
            re=(double)fftresult[j].real()*(double)fftresult[j].real();
            im=(double)fftresult[j].imaginary()*(double)fftresult[j].imaginary();
            amplitudeFFT[j]=re+im;
        }//for(j=0;j<=FFTSIZE/2;J++) end

        //M2 start
        tempsavetotalfft=0;
        for(k = M2A/0.5; k <=M2B/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = M2C/0.5; k <= M2D/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = M2E/0.5; k <= M2F/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = M2G/0.5; k <= M2H/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        M2Qvp->append(tempfftdata);
        //M2 END

        //M1 start
        tempsavetotalfft=0;
        for(k = M1A/0.5; k <= M1B/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = M1C/0.5; k <= M1D/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        M1Qvp->append(tempfftdata);
        //M1 END

        //G2 start
        tempsavetotalfft=0;
        for(k = G2A/0.5; k <=G2B/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        G2Qvp->append(tempfftdata);
        //G2 END

        //G1 start
        tempsavetotalfft=0;
        for(k = G1A/0.5; k <= G1B/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = G1C/0.5; k <= G1D/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        G1Qvp->append(tempfftdata);
        //G1 END

        //UP start
        tempsavetotalfft=0;
        for(k = UPA/0.5; k <= UPB/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = UPC/0.5; k <= UPD/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = UPE/0.5; k <= UPF/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = UPG/0.5; k <= UPH/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = UPI/0.5; k <= UPJ/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = UPK/0.5; k <= UPL/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = UPM/0.5; k <= UPN/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        for(k = UPO/0.5; k <= UPP/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        UPQvp->append(tempfftdata);
        //UP END

        //Beta start
        tempsavetotalfft=0;
        for(k = BetaA/0.5; k <= BetaB/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        BetaQvp->append(tempfftdata);
        //Beta END

        //Sigma start
        tempsavetotalfft=0;
        for(k = (Sigma-SigmaGap)/0.5; k <= (Sigma+SigmaGap)/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        SigmaQvp->append(tempfftdata);
        //Sigma END

        //alpha start
        tempsavetotalfft=0;
        for(k = (alpha-alphaGap)/0.5; k <= (alpha+alphaGap)/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        alphaQvp->append(tempfftdata);
        //alpha END

        //theta start
        tempsavetotalfft=0;
        for(k = thetaA/0.5; k <= thetaB/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        thetaQvp->append(tempfftdata);
        //theta END

        //delta start
        tempsavetotalfft=0;
        for(k = deltaA/0.5; k <= deltaB/0.5; k++)
            tempsavetotalfft+=amplitudeFFT[k];
        tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
        deltaQvp->append(tempfftdata);
        //delta END
    }
}


/***
  * Write wav header
  * Inputs:
  *     out, ptr to output data stream
  *     sampFreq, sampling frequency
  *     numOfChan, number of channels
  *     dataLen, number of data points per channel
  *     bitsPerSamp, bits per sample
  * Returns:
  *     true, if successful
  *     false, if not successful
  */
void ConvertWindow::wavhdrwrite(QDataStream *out, quint32 sampFreq, quint16 numOfChan,
                                quint32 dataLen, quint16 bitsPerSamp)
{
    quint32 fourbytes;
    quint16 twobytes;

    out->writeRawData("RIFF",4);
    fourbytes = dataLen*numOfChan*bitsPerSamp/8+36;
    *out << fourbytes;
    out->writeRawData("WAVEfmt ",8);
    fourbytes = 16;     // subchunk 1 size
    *out << fourbytes;
    twobytes = 1;		// audioformat, PCM=1
    *out << twobytes;
    *out << numOfChan;
    *out << sampFreq;
    fourbytes = sampFreq*numOfChan*bitsPerSamp/8;	// byterate = samplingfreq*numofchannels*samplebits/8
    *out << fourbytes;
    twobytes = numOfChan*bitsPerSamp/8;		// block align = byterate/samplingfreq
    *out << twobytes;
    *out << bitsPerSamp;
    out->writeRawData("data",4);
    fourbytes = dataLen*numOfChan*bitsPerSamp/8;
    *out << fourbytes;
}


void ConvertWindow::wavDadcwrite(QDataStream *outstrp, QVector<qint16> *adcQVp)
{
    qint32 i;
    QProgressDialog progQPD("Writing 1st WAV file", QString(), 0, adcQVp->size());

    progQPD.setWindowModality(Qt::WindowModal);
    progQPD.setMinimumDuration(2000);

    for(i = 0; i < adcQVp->size(); i++)
    {
        if((i % 2000) == 0)
        {
            progQPD.setValue(i);
            qApp->processEvents();
        }
        *outstrp << adcQVp->at(i);
    }
}


/***
  * Need to know beforedhand knowledge how data is formatted in different qvectors
  * wave output sampling freq is 32Hz
  */
void ConvertWindow::wavDallelsewrite(QDataStream *outstrp, QVector<qint16> *tmpQVp,
                                     QVector<qint16> *axQVp, QVector<qint16> *ayQVp,
                                     QVector<qint16> *azQVp, QVector<qint16> *sqQVp)
{
    qint32 i, j;
    QProgressDialog progQPD("Writing 2nd WAV file", QString(), 0, tmpQVp->size());
    qint16 tempi16;

    progQPD.setWindowModality(Qt::WindowModal);
    progQPD.setMinimumDuration(2000);

    for(i = 0; i < azQVp->size()/ACCLEN; i++)
    {
        if((i % 100) == 0)
        {
            progQPD.setValue(i);
            qApp->processEvents();
        }

        for(j = 0; j < ACCLEN; j++)
        {
            *outstrp << axQVp->at(i);
            *outstrp << ayQVp->at(i);
            *outstrp << azQVp->at(i*ACCLEN+j);
            tempi16 = ((qint16) (((float)tmpQVp->at(i))*370.3/16.0-15335.0)); //??
            *outstrp << tempi16;
            //tempi16 = (qint16) sqQVp->at(i);
            *outstrp << sqQVp->at(i);
        }
    }
}


/***
  * Write wav metadata file for ECG
  * must know the sampling rate to be written to, and outgoing format
  */
void ConvertWindow::wavMecgwrite(QDataStream *outm1strp, QVector<qint16> *ecgRRQVp,
                                 QVector<qint16> *ecgampQVp, qint32 decimateFactor)
{
    qint32 i,j;
    int calculateagaincount=1024;
    float saveFFTstd[ECGFFTSIZE],saveSDNN[ECGFFTSIZE];
    int countfftdata=0;
    int countfftoverlapindex=0;
    QFourierTransformer transformer;  //should Setting a fixed size for the transformation
    transformer.setSize(ECGFFTSIZE);
    QVector <QComplexFloat> fftresult;
    QVector <float>  amplitudeFFT(ECGFFTSIZE);  //save the final fft result
    double re,im, RRave, tempSDNN, SDNN;
    float calculatedFFTstd[ECGFFTSIZE];
    qint16 tempLFfftdata,tempHFfftdata,tempLHRatio;  //save the fft value
    float  tempsavetotalLFfft,tempsavetotalHFfft; //save the .... data
    double LFpersum,HFpersum,LHRatiosum;

    for(i = 0; i < ecgRRQVp->size(); i++)
    {
        *outm1strp << ecgRRQVp->at(i) << ecgampQVp->at(i);

        //FFT
        if(calculateagaincount>=1024)
        {
            countfftdata=0;
            calculateagaincount=0;
            for(j = countfftoverlapindex*ECGFFTOVERLAP; j <countfftoverlapindex*ECGFFTOVERLAP+ECGFFTSIZE ;j++)
            {
                if(j<ecgRRQVp->size())
                {
                    saveSDNN[countfftdata]= (ecgRRQVp->at(j)); //in 0.1msec
                    saveFFTstd[countfftdata]=0.5*(1.0-qCos((2.0*M_PI*countfftdata)/(ECGFFTSIZE-1)))*(((double)ecgRRQVp->at(j)));//value in 0.1msec
                }
                else
                {
                    saveSDNN[countfftdata]=(0);
                    saveFFTstd[countfftdata]=(0);
                }
                countfftdata++;
            }
            //start calculate
            countfftoverlapindex++;
            //calculation for fft
            transformer.forwardTransform(saveFFTstd,calculatedFFTstd);
            fftresult=transformer.toComplex(calculatedFFTstd);
            for(j=0;j<=ECGFFTSIZE/2;j++)
            {
                re=(double)fftresult[j].real()*(double)fftresult[j].real();
                im=(double)fftresult[j].imaginary()*(double)fftresult[j].imaginary();
                amplitudeFFT[j]=re+im;
            }//for(int j=0;j<=FFTSIZE/2;J++) end
            //LF
            tempsavetotalLFfft=0;
            for(j=LFA/LFHFTOINDEXRATIO;j<=LFB/LFHFTOINDEXRATIO;j++)
            {tempsavetotalLFfft+=amplitudeFFT[j];
            }
            //HF
            tempsavetotalHFfft=0;
            for(j=HFA/LFHFTOINDEXRATIO;j<=HFB/LFHFTOINDEXRATIO;j++)
            {tempsavetotalHFfft+=amplitudeFFT[j];
            }
            //LFper
            LFpersum = tempsavetotalLFfft/(tempsavetotalLFfft+tempsavetotalHFfft);
            //HFper
            HFpersum = (log10(tempsavetotalHFfft)-9.8062); //log HFpower -log(100*8000*8000)
            //LHRatio
            LHRatiosum = log(LFpersum/(1.0-LFpersum)); //log LHRatio

            //calc RRave, SDNN
            RRave=0;
            for(j=0;j<ECGFFTSIZE;j++) RRave+=saveSDNN[j]; //sum up
            RRave=RRave/ECGFFTSIZE; //average value
            //                HFpersum = RRave; //same scale as RR
            tempSDNN=0;
            for(j=0;j<ECGFFTSIZE;j++) tempSDNN+=(saveSDNN[j]-RRave)*(saveSDNN[j]-RRave); //sum up square of difference
            SDNN=qSqrt(tempSDNN/ECGFFTSIZE); //in 0.1mSec
            LFpersum = SDNN*10; //10x scale in 0.01msec

            //transfer double to qint16

            if(LFpersum>=0&&LFpersum<=32767) //SDNN
                tempLFfftdata=LFpersum;
            else if(LFpersum<0)
                tempLFfftdata=0;
            else if(LFpersum>=32767)
                tempLFfftdata=32767;

            if(HFpersum>=0&&HFpersum<=4) //log HFpower with 40db range
                tempHFfftdata=HFpersum*32767/4;
            else if(HFpersum<0)
                tempHFfftdata=0;
            else if(HFpersum>4)
                tempHFfftdata=32767;

            if(LHRatiosum>=-2&&LHRatiosum<=2) //log LHRatio 0.01 to 100
                tempLHRatio=LHRatiosum*32767/2;//max=1
            else if(LHRatiosum<-2)
                tempLHRatio=-32768;
            else if(LHRatiosum>2)
                tempLHRatio=32767;

        }
        calculateagaincount++;
        //LFper
        *outm1strp << tempLFfftdata; //SDNN*10 actually, repeat 1024 times * 32Hz = 32"
        //HFper
        *outm1strp << tempHFfftdata; //log HFpower actually
        //LHRatio
        *outm1strp << tempLHRatio; //log LHRatio actually
       }
}


void ConvertWindow::wavMeegwrite(QDataStream *outstrp,
                                 QVector<qint16> *M2Qvp, QVector<qint16> *M1Qvp,
                                 QVector<qint16> *G2Qvp, QVector<qint16> *G1Qvp,
                                 QVector<qint16> *UPQvp, QVector<qint16> *BetaQvp,
                                 QVector<qint16> *SigmaQvp, QVector<qint16> *alphaQvp,
                                 QVector<qint16> *thetaQvp, QVector<qint16> *deltaQvp,
                                 qint32 numDataRecs)
{
    QProgressDialog progQPD("Writing EEG EDF file",QString(),0,numDataRecs);
    qint32 i;

    progQPD.setWindowModality(Qt::WindowModal);
    progQPD.setMinimumDuration(2000);

    for( i=0;i<numDataRecs;i++)  //count seconds
    {
        //prepare data
        if((i % 10) == 0)
        {
            progQPD.setValue(i);
            qApp->processEvents();
        }

        *outstrp<<M2Qvp->at(i);
        *outstrp<<M1Qvp->at(i);
        *outstrp<<G2Qvp->at(i);
        *outstrp<<G1Qvp->at(i);
        *outstrp<<UPQvp->at(i);
        *outstrp<<BetaQvp->at(i);
        *outstrp<<SigmaQvp->at(i);
        *outstrp<<alphaQvp->at(i);
        *outstrp<<thetaQvp->at(i);
        *outstrp<<deltaQvp->at(i);
    }
}


/***
  * Conversion to edf output files
  * Will automatically write files
  * Inputs:
  *     None
  * Returns:
  *     true, if successful
  *     false, if not successful
  */
bool ConvertWindow::convertopitoedf()
{
    QString outFileName, outDirName;
    QFile *infilep, *outfile1p, *outfilem1p;
    QDataStream *instrp;
    QDataStream *out1strp, *outm1strp;
    QString datetimestr, tempqstr;
    QDateTime firstFrmDT, stDT, endDT;
    quint8 pdnListp[PDNLISTLEN];
    QVector<qint64> *tsQVpp[PDNLISTLEN];
    QVector<quint8> *skpQVpp[PDNLISTLEN];
    QVector<quint8> *batQVpp[PDNLISTLEN];
    QVector<qint16> *adcQVpp[PDNLISTLEN];
    QVector<qint16> *axQVpp[PDNLISTLEN];
    QVector<qint16> *ayQVpp[PDNLISTLEN];
    QVector<qint16> *azQVpp[PDNLISTLEN];
    QVector<qint16> *tmpQVpp[PDNLISTLEN];
    QVector<qint16> *sqQVpp[PDNLISTLEN];
    QVector<quint8> *edQVpp[PDNLISTLEN];
    QVector<qint64> annOnsetTSQV, annOnsetTS2QV;
    QVector<QString> annTextQV, annText2QV;
    QVector<qint32> adcECGindQV;
    QVector<bool> missBeatFlagQV;
    QVector<qint16> ecgRRQV;
    QVector<qint16> ecgampQV;
    QVector<qint16> eegM2QV;
    QVector<qint16> eegM1QV;
    QVector<qint16> eegG2QV;
    QVector<qint16> eegG1QV;
    QVector<qint16> eegUPQV;
    QVector<qint16> eegBetaQV;
    QVector<qint16> eegSigmaQV;
    QVector<qint16> eegalphaQV;
    QVector<qint16> eegthetaQV;
    QVector<qint16> eegdeltaQV;
    ConvertOptionsWindow *coWinp;
    qint32 i, k;
    quint8 opiHdr[OPIHDRLEN];
    qint32 pdnSlot;
    quint8 pdnDes;
    qint8 outDataType;
    QString localPatientID;
    QString localRecordID;
    qint32 currMonth;
    qint32 dataRecordCt;
    bool useProfile;
    QString readName, readDOB, readSex;
    double beginOffFrms, beginOffFrmsj;
    qint32 beginOffFrmsi;
    qint64 firstFrmTS;
    QMessageBox msgBox;
    bool writeRRfile=false;
    int tempPavg,tempDavg;
    int tempDlast;
    QString outFileNameDtoMecg;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(ui->inLE->text().trimmed());
    if (!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(ui->inLE->text()));
        delete infilep;
        return false;
    }

    // open input file stream
    instrp = new QDataStream(infilep);

    // get header information
    for(i = 0; i < OPIHDRLEN; i++) *instrp >> opiHdr[i];
    if((opiHdr[11] != 0x4F) || (opiHdr[12] != 0x50) || (opiHdr[13] != 0x49)
            || (opiHdr[14] != 0x55) || (opiHdr[15] != 0x43) || (opiHdr[16] != 0x44))
    {
        ui->msgPTE->appendPlainText(QString("Input File is not an OPI raw data file"));
        infilep->close();
        delete instrp;
        delete infilep;
        return false;
    }

    // copy pdn's
    for(i = 0; i < PDNLISTLEN; i++)
        pdnListp[i] = opiHdr[20+i];

    // init qvector pointers to null
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        tsQVpp[pdnSlot] = 0;
        skpQVpp[pdnSlot] = 0;
        batQVpp[pdnSlot] = 0;
        adcQVpp[pdnSlot] = 0;
        tmpQVpp[pdnSlot] = 0;
        axQVpp[pdnSlot] = 0;
        ayQVpp[pdnSlot] = 0;
        azQVpp[pdnSlot] = 0;
        sqQVpp[pdnSlot] = 0;
        edQVpp[pdnSlot] = 0;
    }

    // convert all pdn's in pdn list that are not 0xFF and put data in init'd QVectors
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        pdnDes = pdnListp[pdnSlot];    // PDN to convert
        if(pdnDes == 0xFF) // skip over this since no device, go onto next one
        {
            delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            continue;
        }

        // reset file and variables to prepare for reading
        infilep->reset();

        // initialize qvectors
        tsQVpp[pdnSlot] = new QVector<qint64>(0);
        skpQVpp[pdnSlot] = new QVector<quint8>(0);
        batQVpp[pdnSlot] = new QVector<quint8>(0);
        adcQVpp[pdnSlot] = new QVector<qint16>(0);
        tmpQVpp[pdnSlot] = new QVector<qint16>(0);
        axQVpp[pdnSlot] = new QVector<qint16>(0);
        ayQVpp[pdnSlot] = new QVector<qint16>(0);
        azQVpp[pdnSlot] = new QVector<qint16>(0);
        sqQVpp[pdnSlot] = new QVector<qint16>(0);
        edQVpp[pdnSlot] = new QVector<quint8>(0);

        if(!opiDread(instrp, pdnDes, tsQVpp[pdnSlot], skpQVpp[pdnSlot], batQVpp[pdnSlot],
                     adcQVpp[pdnSlot], tmpQVpp[pdnSlot], axQVpp[pdnSlot], ayQVpp[pdnSlot],
                     azQVpp[pdnSlot], sqQVpp[pdnSlot], edQVpp[pdnSlot]))
        {
            delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            continue; // no data, didn't use any frames
        }
    }

    // At this point, input file is not needed anymore
    infilep->close();
    delete infilep;
    delete instrp;

    // check that there is actually usable data, if not exit
    if((!tsQVpp[0]) && (!tsQVpp[1]) && (!tsQVpp[2]) && (!tsQVpp[3]) &&
            (!tsQVpp[4]) && (!tsQVpp[5]) && (!tsQVpp[6]) && (!tsQVpp[7]))
    {
        ui->msgPTE->appendPlainText("No usable data in files... ");
        return false;   // user terminates so don't do anything else
    }

    // go to converter options, once it returns, data has been processed and only needs
    // to be written to file

    coWinp = new ConvertOptionsWindow(&outDirName, pdnListp, &outDataType, &alpha, &Sigma, tsQVpp, skpQVpp,
                                      batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp,
                                      sqQVpp, edQVpp, &annOnsetTSQV, &annTextQV,OPITOEDF, &useProfile, this,&writeRRfile);
    if(coWinp->exec() == QDialog::Rejected)
    {
        ui->msgPTE->appendPlainText("User aborted... ");
        delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
        delete coWinp;
        return false;   // user terminates so don't do anything else
    }
    delete coWinp;  // don't need anymore

    stDT = QDateTime::currentDateTime();
    // Write output files
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        if(tsQVpp[pdnSlot] == 0) continue;  // no data
        pdnDes = pdnListp[pdnSlot];    // PDN to convert
        // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
        firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((tsQVpp[pdnSlot]->at(0))*1000/UCRTCFREQ);
        // Need floor(time) for file since fill in, toString function automatically does this
        datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");
        qDebug() << firstFrmDT.toString("yyyyMMdd_hhmmss.zzz");
        // EDF header stuff
        if(useProfile)
        {
            readName = getConfigValue("Name");
            readDOB = getConfigValue("DOB");
            readSex = getConfigValue("Sex");
            localPatientID = QString("X "); // for hospital idcode
            if(readSex.isEmpty())
                localPatientID.append("X ");
            else
                localPatientID.append(readSex).append(" ");
            if(readDOB.isEmpty())
                localPatientID.append("X ");
            else
                localPatientID.append(readDOB).append(" ");
            if(readName.isEmpty())
                localPatientID.append("X");
            else
                localPatientID.append(readName);
        }
        else
            localPatientID.append("X X X X");

        localRecordID = QString("X X %1_OPITS%2").arg(localUTCOffset()).arg(pdnDes,3,10,QChar('0'));
        localRecordID.prepend(firstFrmDT.toString("-yyyy "));
        currMonth = firstFrmDT.date().month();      // have to do this because of QDateTime letters being 2 chars
        if(currMonth == 1)  localRecordID.prepend("JAN");
        else if(currMonth == 2) localRecordID.prepend("FEB");
        else if(currMonth == 3) localRecordID.prepend("MAR");
        else if(currMonth == 4) localRecordID.prepend("APR");
        else if(currMonth == 5) localRecordID.prepend("MAY");
        else if(currMonth == 6) localRecordID.prepend("JUN");
        else if(currMonth == 7) localRecordID.prepend("JUL");
        else if(currMonth == 8) localRecordID.prepend("AUG");
        else if(currMonth == 9) localRecordID.prepend("SEP");
        else if(currMonth == 10) localRecordID.prepend("OCT");
        else if(currMonth == 11) localRecordID.prepend("NOV");
        else localRecordID.prepend("DEC");
        localRecordID.prepend(firstFrmDT.toString("dd-")).prepend("Startdate ");

        // Output file stuff
        outFileName = outDirName;
        outFileName.append(QString("D%1_%2.edf").arg(datetimestr).arg(pdnDes));
        outfile1p = new QFile(outFileName);
        if(outfile1p->exists())
        {
            msgBox.setWindowTitle("Out File Exists");
            msgBox.setText("Out file already exists in selected directory.");
            msgBox.setInformativeText("Do you want to overwrite?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            if(msgBox.exec() == QMessageBox::Cancel)
            {
                delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
                delete outfile1p;
                return false;
            }
        }
        if(!outfile1p->open(QIODevice::WriteOnly))
        {
            ui->msgPTE->appendPlainText(QString("PDN#%1: Problem with output files").arg(pdnDes));
            delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            delete outfile1p;
            continue;
        }

        out1strp = new QDataStream(outfile1p);
        out1strp->setByteOrder(QDataStream::LittleEndian);

        // EDF header, will need to come back later to adjust number of data records
        // which are unknown (-1) at this point
        // also pass the dataType and use the write header if need to write events
        edfDhdropiwrite(out1strp, &localPatientID, &localRecordID, &firstFrmDT, -1);

        // Copy tags in case more than one device and if EEG/ECG need it
        annOnsetTS2QV = annOnsetTSQV;
        annText2QV = annTextQV;

        // add data at beginning (first frame) to fill up to floor(second) (within 2 ms)
        // difference between floored exact second and first time in frames
        beginOffFrms = ((double) (firstFrmDT.toMSecsSinceEpoch() % 1000))*512.0/1000/ADCLEN;
        beginOffFrmsi = (qint32) (beginOffFrms);
        beginOffFrmsj = beginOffFrms-((double) beginOffFrmsi);
        // put in default values for beginning in constant frame units
        firstFrmTS = tsQVpp[pdnSlot]->at(0);
        for(; beginOffFrmsi > 0; beginOffFrmsi--)
        {
            tsQVpp[pdnSlot]->prepend(firstFrmTS-512*beginOffFrmsi);
            skpQVpp[pdnSlot]->prepend(0);
            batQVpp[pdnSlot]->prepend(1);
            sqQVpp[pdnSlot]->prepend(-4000);
            edQVpp[pdnSlot]->prepend(0);
            for(k = 0; k < ADCLEN; k++)
                adcQVpp[pdnSlot]->prepend(0);
            for(k = 0; k < TMPLEN; k++)
                tmpQVpp[pdnSlot]->prepend(1024);   // temp ~25
            for(k = 0; k < ACCLEN/4; k++)
            {
                axQVpp[pdnSlot]->prepend(0);
                ayQVpp[pdnSlot]->prepend(0);
            }
            for(k = 0; k < ACCLEN; k++)
                azQVpp[pdnSlot]->prepend(0);
        }

        // add less than a frame data
        if(beginOffFrmsj > 0.5)
        {
            tsQVpp[pdnSlot]->prepend(firstFrmTS-512*(beginOffFrmsi+1));
            skpQVpp[pdnSlot]->prepend(0);
            batQVpp[pdnSlot]->prepend(1);
            sqQVpp[pdnSlot]->prepend(4);
            edQVpp[pdnSlot]->prepend(0);
            tmpQVpp[pdnSlot]->prepend(1024);   // temp ~25
            axQVpp[pdnSlot]->prepend(0);
            ayQVpp[pdnSlot]->prepend(0);
        }
        for(k = 0; k < ((qint32) (beginOffFrmsj*ADCLEN)); k++)
            adcQVpp[pdnSlot]->prepend(0);
        for(k = 0; k < ((qint32) (beginOffFrmsj*ACCLEN)); k++)
            azQVpp[pdnSlot]->prepend(0);

        // fix up tag times by adding
        annOnsetTS2QV = annOnsetTSQV;
        annText2QV = annTextQV;
        for(k = 0; k < annOnsetTS2QV.size(); k++)
            annOnsetTS2QV.replace(k, annOnsetTS2QV.at(k)+((qint64) (beginOffFrms*512.0)));

        // Add a "DataStart" and "DataEnd" tag
        annOnsetTS2QV.prepend(tsQVpp[pdnSlot]->at(0)+((qint64) (beginOffFrms*512.0)));
        annText2QV.prepend("DataStart");
        annOnsetTS2QV.append(tsQVpp[pdnSlot]->at(tsQVpp[pdnSlot]->size()-1)+((qint64) (beginOffFrms*512.0)));
        annText2QV.append("DataEnd");

        // put in default values to make last record full (add one full one for margin/simplicity)
        for(k = 0; k < 1.0*FRMSPERSEC*EDFDRDURSEC; k++)
        {
            tsQVpp[pdnSlot]->append(tsQVpp[pdnSlot]->at(tsQVpp[pdnSlot]->size()-1)+512);
            skpQVpp[pdnSlot]->append(0);
            batQVpp[pdnSlot]->append(1);
            sqQVpp[pdnSlot]->append(-4000);
            edQVpp[pdnSlot]->append(0);
            tmpQVpp[pdnSlot]->append(1024);   // temp ~25
            axQVpp[pdnSlot]->append(0);
            ayQVpp[pdnSlot]->append(0);
        }
        for(k = 0; k < ADCLEN*FRMSPERSEC*EDFDRDURSEC; k++)
            adcQVpp[pdnSlot]->append(0);
        for(k = 0; k < ACCLEN*FRMSPERSEC*EDFDRDURSEC; k++)
            azQVpp[pdnSlot]->append(0);

        // Stuff in all the data according to specified format and save for meta data processing
        dataRecordCt = edfDwrite(out1strp, adcQVpp[pdnSlot], tmpQVpp[pdnSlot],
                                 axQVpp[pdnSlot], ayQVpp[pdnSlot], azQVpp[pdnSlot],
                                 sqQVpp[pdnSlot], 0, tsQVpp[pdnSlot]->at(0),
                                 tsQVpp[pdnSlot], &annOnsetTS2QV, &annText2QV);

        // Rewrite header, since number of data records now known
        outfile1p->reset();
        edfDhdropiwrite(out1strp, &localPatientID, &localRecordID, &firstFrmDT, dataRecordCt);

        // can close file now
        outfile1p->close();
        delete out1strp;
        delete outfile1p;

        ui->msgPTE->appendPlainText(QString("PDN#%1: EDF Conversion Finished, Saved in %2").arg(pdnDes).arg(outFileName));

        // Meta data processing
        if(outDataType == ECGMDATATYPE)
        {
            adcECGindQV = findHRmax(adcQVpp[pdnSlot], TSRTCFREQ, adcQVpp[pdnSlot]->size()/TSRTCFREQ, &missBeatFlagQV,true,true,&tempPavg,&tempDavg,&tempDlast);
            if(adcECGindQV.size() == 0)
                ui->msgPTE->appendPlainText("No heartbeats found");
            else
            {
                outFileName = outDirName;
                outFileName.append(QString("D%1_%2_ECG.edf").arg(datetimestr).arg(pdnDes));
                outFileNameDtoMecg = outDirName;
                outFileNameDtoMecg.append(QString("D%1_%2_RR.txt").arg(datetimestr).arg(pdnDes));

                outfilem1p = new QFile(outFileName);
                if(outfilem1p->exists())
                {
                    msgBox.setWindowTitle("Out File Exists");
                    msgBox.setText("Out file already exists in selected directory.");
                    msgBox.setInformativeText("Do you want to overwrite?");
                    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                    if(msgBox.exec() == QMessageBox::Cancel)
                    {
                        delete outfilem1p;
                        return false;
                    }
                }
                if(!outfilem1p->open(QIODevice::WriteOnly))
                {
                    ui->msgPTE->appendPlainText(QString("Problem with creating metadata output file %1").arg(outFileName));
                    delete outfilem1p;
                }
                else
                {
                    outm1strp = new QDataStream(outfilem1p);
                    outm1strp->setByteOrder(QDataStream::LittleEndian);
                    //32Hz
                    edfMecghdropiwrite(outm1strp, &localPatientID, &localRecordID, &firstFrmDT, dataRecordCt); // need to modify this according to sampfreq
                    ecgRRQV.clear(); // may have been used before
                    ecgampQV.clear();
                    // Copy tags in case more than one device and if EEG/ECG need it
                    annOnsetTS2QV = annOnsetTSQV;
                    annText2QV = annTextQV;
                    // will output a specific sampling rate
                    DtoMecgQVs(&adcECGindQV, adcQVpp[pdnSlot], &missBeatFlagQV, &ecgRRQV, &ecgampQV, 16);
                    if(writeRRfile)
                    writetoFile(outFileNameDtoMecg,&adcECGindQV,512);
                    edfMecgwrite(outm1strp, &ecgRRQV, &ecgampQV, 16, dataRecordCt, tsQVpp[pdnSlot]->at(0), &annOnsetTS2QV, &annText2QV);

                    ui->msgPTE->appendPlainText(QString("Created metadata output file %1").arg(outFileName));
                    outfilem1p->close();
                    delete outm1strp;
                    delete outfilem1p;
                }
            }
        }
        else if(outDataType == EEGMDATATYPE)
        {
            outFileName = outDirName;
            outFileName.append(QString("D%1_%2_EEG.edf").arg(datetimestr).arg(pdnDes));
            outfilem1p = new QFile(outFileName);
            if(outfilem1p->exists())
            {
                msgBox.setWindowTitle("Out File Exists");
                msgBox.setText("Out file already exists in selected directory.");
                msgBox.setInformativeText("Do you want to overwrite?");
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                if(msgBox.exec() == QMessageBox::Cancel)
                {
                    delete outfilem1p;
                    return false;
                }
            }
            if(!outfilem1p->open(QIODevice::WriteOnly))
            {
                ui->msgPTE->appendPlainText(QString("Problem with creating metadata output file %1").arg(outFileName));
                delete outfilem1p;
            }
            else
            {
                outm1strp = new QDataStream(outfilem1p);
                outm1strp->setByteOrder(QDataStream::LittleEndian);
                // Copy tags in case more than one device and if EEG/ECG need it
                annOnsetTS2QV = annOnsetTSQV;
                annText2QV = annTextQV;

                eegM2QV.clear();
                eegM1QV.clear();
                eegG2QV.clear();
                eegG1QV.clear();
                eegUPQV.clear();
                eegBetaQV.clear();
                eegSigmaQV.clear();
                eegalphaQV.clear();
                eegthetaQV.clear();
                eegdeltaQV.clear();

                DtoMeegQVs(adcQVpp[pdnSlot],
                           &eegM2QV,&eegM1QV,
                           &eegG2QV,&eegG1QV,
                           &eegUPQV,&eegBetaQV,
                           &eegSigmaQV,&eegalphaQV,
                           &eegthetaQV,&eegdeltaQV);
                edfMeeghdropiwrite(outm1strp, &localPatientID, &localRecordID,
                                   &firstFrmDT,eegM2QV.size()/EDFDRDURSEC);

                edfMeegwrite(outm1strp,&eegM2QV,&eegM1QV,
                             &eegG2QV,&eegG1QV,
                             &eegUPQV,&eegBetaQV,
                             &eegSigmaQV,&eegalphaQV,
                             &eegthetaQV,&eegdeltaQV,
                             eegM2QV.size()/EDFDRDURSEC,tsQVpp[pdnSlot]->at(0),
                             &annOnsetTS2QV, &annText2QV);

                ui->msgPTE->appendPlainText(QString("Created metadata output file %1").arg(outFileName));
                outfilem1p->close();
                delete outm1strp;
                delete outfilem1p;
            }
        }
    } // for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)

    endDT = QDateTime::currentDateTime();
    // Show done
    ui->msgPTE->appendPlainText(QString(">> Conversion Finished in %1 seconds").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));

    // cleanup
    delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);

    // if got here, conversion done properly
    return true;
}


/***
  * Conversion from edf to edf output files
  * Will automatically write files
  * Inputs:
  *     None
  * Returns:
  *     true, if successful
  *     false, if not successful
  */
bool ConvertWindow::convertedfDtoedf()
{
    QString outFileName, outDirName;
    QFile *infilep, *outfile1p, *outfilem1p;
    QDataStream *instrp;
    QDataStream *out1strp, *outm1strp;
    QString datetimestr, tempqstr;
    QDateTime firstFrmDT, stDT, endDT;
    quint8 pdnListp[PDNLISTLEN];
    QVector<qint64> *tsQVpp[PDNLISTLEN];
    QVector<quint8> *skpQVpp[PDNLISTLEN];
    QVector<quint8> *batQVpp[PDNLISTLEN];
    QVector<qint16> *adcQVpp[PDNLISTLEN];
    QVector<qint16> *axQVpp[PDNLISTLEN];
    QVector<qint16> *ayQVpp[PDNLISTLEN];
    QVector<qint16> *azQVpp[PDNLISTLEN];
    QVector<qint16> *tmpQVpp[PDNLISTLEN];
    QVector<qint16> *sqQVpp[PDNLISTLEN];
    QVector<quint8> *edQVpp[PDNLISTLEN];
    QVector<qint64> annOnsetTSQV, annOnsetTS2QV;
    QVector<QString> annTextQV, annText2QV;
    QVector<qint32> adcECGindQV;
    QVector<bool> missBeatFlagQV;
    QVector<qint16> ecgRRQV;
    QVector<qint16> ecgampQV;
    QVector<qint16> eegM2QV;
    QVector<qint16> eegM1QV;
    QVector<qint16> eegG2QV;
    QVector<qint16> eegG1QV;
    QVector<qint16> eegUPQV;
    QVector<qint16> eegBetaQV;
    QVector<qint16> eegSigmaQV;
    QVector<qint16> eegalphaQV;
    QVector<qint16> eegthetaQV;
    QVector<qint16> eegdeltaQV;
    ConvertOptionsWindow *coWinp;
    QString localPatientID, lpid, lrid;
    QDateTime startDT;
    qint32 numDataRecs, dataRecDur;
    qint32 numSignals;
    QVector<QString> labelSignalsQV, transTypeQV, physDimQV, prefiltQV;
    QVector<qint32> physMinQV, physMaxQV, digMinQV, digMaxQV;
    QVector<qint32> sampsPerDRQV;
    qint32 pdnSlot, i;
    qint32 pdnDes;
    qint8 outDataType;
    qint32 dataRecordCt;
    bool useProfile;
    QString readName, readDOB, readSex;
    QMessageBox msgBox;
    bool writeRRfile=false;
    QStringList tempQSL;
    QString stDateQS;
    qint32 currMonth;
    int tempPavg,tempDavg;
    int tempDlast;
    QString outFileNameDtoMecg;

    // Opening of inputs/outputs and Error Checking
    infilep = new QFile(ui->inLE->text());
    if (!infilep->open(QIODevice::ReadOnly))
    {
        ui->msgPTE->appendPlainText(QString("Problem with input file \"%1\"").arg(ui->inLE->text()));
        delete infilep;
        return false;
    }

    // open input file stream
    instrp = new QDataStream(infilep);
    instrp->setByteOrder(QDataStream::LittleEndian);

    // get header information
    edfhdrread(instrp, &lpid, &lrid, &startDT, &numDataRecs, &dataRecDur,
               &numSignals, &labelSignalsQV, &transTypeQV, &physDimQV, &physMinQV,
               &physMaxQV, &digMinQV, &digMaxQV, &prefiltQV, &sampsPerDRQV);

    // init qvector pointers to null
    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        tsQVpp[pdnSlot] = 0;
        skpQVpp[pdnSlot] = 0;
        batQVpp[pdnSlot] = 0;
        adcQVpp[pdnSlot] = 0;
        tmpQVpp[pdnSlot] = 0;
        axQVpp[pdnSlot] = 0;
        ayQVpp[pdnSlot] = 0;
        azQVpp[pdnSlot] = 0;
        sqQVpp[pdnSlot] = 0;
        edQVpp[pdnSlot] = 0;
    }

    // get PDN from Device field
    pdnDes = getPDNlrid(lrid);
    pdnSlot = 0;

    // copy pdn's
    for(i = 0; i < PDNLISTLEN; i++)
        pdnListp[i] = 0xFF;
    pdnListp[pdnSlot] = pdnDes;

    // initialize qvectors
    tsQVpp[pdnSlot] = new QVector<qint64>(0);
    skpQVpp[pdnSlot] = new QVector<quint8>(0);
    batQVpp[pdnSlot] = new QVector<quint8>(0);
    adcQVpp[pdnSlot] = new QVector<qint16>(0);
    tmpQVpp[pdnSlot] = new QVector<qint16>(0);
    axQVpp[pdnSlot] = new QVector<qint16>(0);
    ayQVpp[pdnSlot] = new QVector<qint16>(0);
    azQVpp[pdnSlot] = new QVector<qint16>(0);
    sqQVpp[pdnSlot] = new QVector<qint16>(0);
    edQVpp[pdnSlot] = new QVector<quint8>(0);

    if(edfDread(instrp, startDT, numDataRecs, dataRecDur, numSignals,
                sampsPerDRQV, tsQVpp[pdnSlot], skpQVpp[pdnSlot], batQVpp[pdnSlot],
                adcQVpp[pdnSlot], tmpQVpp[pdnSlot], axQVpp[pdnSlot], ayQVpp[pdnSlot],
                azQVpp[pdnSlot], sqQVpp[pdnSlot], edQVpp[pdnSlot],
                &annOnsetTSQV, &annTextQV) < 0)
    {
        delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
    }

    // At this point, input file is not needed anymore
    infilep->close();
    delete infilep;
    delete instrp;

    // check that there is actually usable data, if not exit
    if((!tsQVpp[0]) && (!tsQVpp[1]) && (!tsQVpp[2]) && (!tsQVpp[3]) &&
            (!tsQVpp[4]) && (!tsQVpp[5]) && (!tsQVpp[6]) && (!tsQVpp[7]))
    {
        ui->msgPTE->appendPlainText("No usable data in files... ");
        return false;   // user terminates so don't do anything else
    }

    // go to converter options, once it returns, data has been processed and only needs
    // to be written to file

    coWinp = new ConvertOptionsWindow(&outDirName, pdnListp, &outDataType, &alpha, &Sigma, tsQVpp, skpQVpp,
                                      batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp,
                                      sqQVpp, edQVpp, &annOnsetTSQV, &annTextQV ,EDFDTOEDF, &useProfile, this,&writeRRfile);

    if(coWinp->exec() == QDialog::Rejected)
    {
        ui->msgPTE->appendPlainText("User aborted... ");
        delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
        delete coWinp;
        return false;   // user terminates so don't do anything else
    }
    delete coWinp;

    // remove tags that are before the first timestamp
    while(annOnsetTSQV.size() > 0)
    {
        if(annOnsetTSQV.at(0) >= tsQVpp[pdnSlot]->at(0)) break;
                // don't need to remove anymore since
        else                                    // tag text will be monotonically increasing
        {
            annOnsetTSQV.remove(0,1);
            annTextQV.remove(0,1);
        }
    }

    stDT = QDateTime::currentDateTime();
    // Write output files
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((tsQVpp[pdnSlot]->at(0))*1000/UCRTCFREQ);
    datetimestr = firstFrmDT.toString("yyyyMMdd_hhmmss");

    // Output file stuff
    outFileName = outDirName;
    outFileName.append(QString("D%1_%2.edf").arg(datetimestr).arg(pdnDes));
    outfile1p = new QFile(outFileName);
    if(outfile1p->exists())
    {
        msgBox.setWindowTitle("Out File Exists");
        msgBox.setText("Out file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
            delete outfile1p;
            return false;
        }
    }
    if(!outfile1p->open(QIODevice::WriteOnly))
    {
        ui->msgPTE->appendPlainText(QString("PDN#%1: Problem with output files").arg(pdnDes));
        delspecQVs(pdnSlot, tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
        delete outfile1p;
        return false;
    }

    out1strp = new QDataStream(outfile1p);
    out1strp->setByteOrder(QDataStream::LittleEndian);

    // EDF header stuff
    if(useProfile)
    {
        readName = getConfigValue("Name");
        readDOB = getConfigValue("DOB");
        readSex = getConfigValue("Sex");
        localPatientID = QString("X "); // for hospital idcode
        if(readSex.isEmpty())
            localPatientID.append("X ");
        else
            localPatientID.append(readSex).append(" ");
        if(readDOB.isEmpty())
            localPatientID.append("X ");
        else
            localPatientID.append(readDOB).append(" ");
        if(readName.isEmpty())
            localPatientID.append("X");
        else
            localPatientID.append(readName);
    }
    else
        localPatientID = lpid;

    // make sure StartDate is correct in local recording id
    tempQSL = lrid.split(QRegExp("\\s+"));
    stDateQS = firstFrmDT.toString("-yyyy");
    currMonth = firstFrmDT.date().month();      // have to do this because of QDateTime letters being 2 chars
    if(currMonth == 1)  stDateQS.prepend("JAN");
    else if(currMonth == 2) stDateQS.prepend("FEB");
    else if(currMonth == 3) stDateQS.prepend("MAR");
    else if(currMonth == 4) stDateQS.prepend("APR");
    else if(currMonth == 5) stDateQS.prepend("MAY");
    else if(currMonth == 6) stDateQS.prepend("JUN");
    else if(currMonth == 7) stDateQS.prepend("JUL");
    else if(currMonth == 8) stDateQS.prepend("AUG");
    else if(currMonth == 9) stDateQS.prepend("SEP");
    else if(currMonth == 10) stDateQS.prepend("OCT");
    else if(currMonth == 11) stDateQS.prepend("NOV");
    else stDateQS.prepend("DEC");
    stDateQS.prepend(firstFrmDT.toString("dd-"));
    tempQSL.replace(1, stDateQS);
    lrid = tempQSL.join(" ");

    // EDF header, will need to come back later to adjust number of data records
    // which are unknown (-1) at this point
    // also pass the dataType and use the write header if need to write events
    edfDhdropiwrite(out1strp, &localPatientID, &lrid, &firstFrmDT, -1);

    // Copy tags in case EEG/ECG need it
    annOnsetTS2QV = annOnsetTSQV;
    annText2QV = annTextQV;

    // Stuff in all the data according to specified format and save for meta data processing
    dataRecordCt = edfDwrite(out1strp, adcQVpp[pdnSlot], tmpQVpp[pdnSlot],
                             axQVpp[pdnSlot], ayQVpp[pdnSlot], azQVpp[pdnSlot],
                             sqQVpp[pdnSlot], 0, tsQVpp[pdnSlot]->at(0),
                             tsQVpp[pdnSlot], &annOnsetTS2QV, &annText2QV);

    // Rewrite header, since number of data records now known
    outfile1p->reset();
    edfDhdropiwrite(out1strp, &lpid, &lrid, &firstFrmDT, dataRecordCt);

    // can close file now
    outfile1p->close();
    delete out1strp;
    delete outfile1p;

    ui->msgPTE->appendPlainText(QString("PDN#%1: EDF Conversion Finished, Saved in %2").arg(pdnDes).arg(outFileName));

    // Meta data processing
    if(outDataType == ECGMDATATYPE)
    {
        adcECGindQV = findHRmax(adcQVpp[pdnSlot], TSRTCFREQ, adcQVpp[pdnSlot]->size()/TSRTCFREQ, &missBeatFlagQV,true,true,&tempPavg,&tempDavg,&tempDlast);
        if(adcECGindQV.size() == 0)
            ui->msgPTE->appendPlainText("No heartbeats found");
        else
        {
            outFileName = outDirName;
            outFileName.append(QString("D%1_%2_ECG.edf").arg(datetimestr).arg(pdnDes));           
            outFileNameDtoMecg = outDirName;
            outFileNameDtoMecg.append(QString("D%1_%2_RR.txt").arg(datetimestr).arg(pdnDes));
            outfilem1p = new QFile(outFileName);
            if(outfilem1p->exists())
            {
                msgBox.setWindowTitle("Out File Exists");
                msgBox.setText("Out file already exists in selected directory.");
                msgBox.setInformativeText("Do you want to overwrite?");
                msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                if(msgBox.exec() == QMessageBox::Cancel)
                {
                    delete outfilem1p;
                    return false;
                }
            }
            if(!outfilem1p->open(QIODevice::WriteOnly))
            {
                ui->msgPTE->appendPlainText(QString("Problem with creating metadata output file %1").arg(outFileName));
                delete outfilem1p;
            }
            else
            {
                outm1strp = new QDataStream(outfilem1p);
                outm1strp->setByteOrder(QDataStream::LittleEndian);
                // Copy tags in case EEG/ECG need it
                annOnsetTS2QV = annOnsetTSQV;
                annText2QV = annTextQV;
                edfMecghdropiwrite(outm1strp, &lpid, &lrid, &firstFrmDT, dataRecordCt); // need to modify this according to sampfreq
                DtoMecgQVs(&adcECGindQV, adcQVpp[pdnSlot], &missBeatFlagQV, &ecgRRQV, &ecgampQV, 16);
                if(writeRRfile)
                writetoFile(outFileNameDtoMecg,&adcECGindQV,512);
                edfMecgwrite(outm1strp, &ecgRRQV, &ecgampQV, 16, dataRecordCt, tsQVpp[pdnSlot]->at(0), &annOnsetTS2QV, &annText2QV);

                ui->msgPTE->appendPlainText(QString("Created metadata output file %1").arg(outFileName));
                outfilem1p->close();
                delete outm1strp;
                delete outfilem1p;
            }
        }
    }
    else if(outDataType == EEGMDATATYPE)
    {
        outFileName = outDirName;
        outFileName.append(QString("D%1_%2_EEG.edf").arg(datetimestr).arg(pdnDes));
        outfilem1p = new QFile(outFileName);
        if(outfilem1p->exists())
        {
            msgBox.setWindowTitle("Out File Exists");
            msgBox.setText("Out file already exists in selected directory.");
            msgBox.setInformativeText("Do you want to overwrite?");
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            if(msgBox.exec() == QMessageBox::Cancel)
            {
                delete outfilem1p;
                return false;
            }
        }
        if(!outfilem1p->open(QIODevice::WriteOnly))
        {
            ui->msgPTE->appendPlainText(QString("Problem with creating metadata output file %1").arg(outFileName));
            delete outfilem1p;
        }
        else
        {
            outm1strp = new QDataStream(outfilem1p);
            outm1strp->setByteOrder(QDataStream::LittleEndian);
            // Copy tags in case EEG/ECG need it
            annOnsetTS2QV = annOnsetTSQV;
            annText2QV = annTextQV;
            eegM2QV.clear();
            eegM1QV.clear();
            eegG2QV.clear();
            eegG1QV.clear();
            eegUPQV.clear();
            eegBetaQV.clear();
            eegSigmaQV.clear();
            eegalphaQV.clear();
            eegthetaQV.clear();
            eegdeltaQV.clear();
            DtoMeegQVs(adcQVpp[pdnSlot],
                       &eegM2QV,&eegM1QV,
                       &eegG2QV,&eegG1QV,
                       &eegUPQV,&eegBetaQV,
                       &eegSigmaQV,&eegalphaQV,
                       &eegthetaQV,&eegdeltaQV);
            edfMeeghdropiwrite(outm1strp, &lpid, &lrid, &firstFrmDT, eegM2QV.size()/EDFDRDURSEC);
            edfMeegwrite(outm1strp,&eegM2QV,&eegM1QV,
                         &eegG2QV,&eegG1QV,
                         &eegUPQV,&eegBetaQV,
                         &eegSigmaQV,&eegalphaQV,
                         &eegthetaQV,&eegdeltaQV,
                         eegM2QV.size()/EDFDRDURSEC,tsQVpp[pdnSlot]->at(0), &annOnsetTS2QV, &annText2QV);

            ui->msgPTE->appendPlainText(QString("Created metadata output file %1").arg(outFileName));
            outfilem1p->close();
            delete outm1strp;
            delete outfilem1p;
        }
    }

    endDT = QDateTime::currentDateTime();

    // Show done
    ui->msgPTE->appendPlainText(QString(">> Conversion Finished in %1 seconds").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));

    // cleanup
    delQVs(tsQVpp, skpQVpp, batQVpp, adcQVpp, tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);

    // if got here, conversion done properly
    return true;
}


/***
  * Write EDF header especially for opi meta data files, 8 sec data record duration
  * Inputs:
  *     out, ptr to output data stream
  *     lpidp, ptr to local patient id
  *     lridp, ptr to local recording id
  *     startDTp, ptr to starting date and time
  *     numDataRecs, number of data records
  * Returns:
  *     true, if successful
  *     false, if not successful
  */
void ConvertWindow::edfMecghdropiwrite(QDataStream *out, QString *lpidp, QString *lridp,
                                QDateTime *startDTp, qint32 numDataRecs)
{
    QString tempstr;

    out->writeRawData("0       ", 8);     // edf version of data format
    out->writeRawData(lpidp->toUtf8().leftJustified(80,' ').data(),80);   // local patient identification
    out->writeRawData(lridp->toUtf8().leftJustified(80,' ').data(),80);   // local recording identification
    out->writeRawData(startDTp->toString("dd.MM.yyhh.mm.ss").toUtf8().data(),16); // startdate and starttime
    out->writeRawData("1792    ", 8);     // number of header bytes (256+6signals*256)
    out->writeRawData(QByteArray("EDF+C").leftJustified(44,' ').data(),44); // format type (reserved)
    out->writeRawData(QString("%1").arg(numDataRecs).toUtf8().leftJustified(8,' ').data(),8);  // number of data records
    out->writeRawData("8       ", 8);     // duration of a data record in seconds
    out->writeRawData("6   ", 4);     // number of signals: RR, amp,LFper HFper LHRatio EDF annotations

    // signal labels
    out->writeRawData("RR              ", 16);
    out->writeRawData("RPeak           ", 16);
    out->writeRawData("SDNN            ", 16);
    out->writeRawData("HFpower         ", 16);
    out->writeRawData("LHRatio         ", 16);
    out->writeRawData("EDF Annotations ", 16);

    // transducer type
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);

    // physical dimensions
    out->writeRawData("mS      ", 8);
    out->writeRawData("uV      ", 8);
    out->writeRawData("mS      ", 8);
    out->writeRawData("Log     ", 8);
    out->writeRawData("Log     ", 8);
    out->writeRawData("        ", 8);

    // physical mins and maxs
    out->writeRawData("0       ", 8);
    out->writeRawData("-800    ", 8);
    out->writeRawData("0       ", 8);
    out->writeRawData("-4      ", 8);
    out->writeRawData("-2      ", 8);
    out->writeRawData("-1      ", 8);

    out->writeRawData("3276.7  ", 8);
    out->writeRawData("800     ", 8);
    out->writeRawData("327.67  ", 8);
    out->writeRawData("4       ", 8);
    out->writeRawData("2       ", 8);
    out->writeRawData("1       ", 8);

    // digital mins and maxs
    out->writeRawData("0       ", 8);
    out->writeRawData("-32768  ", 8);
    out->writeRawData("0       ", 8);
    out->writeRawData("-32768  ", 8);
    out->writeRawData("-32768  ", 8);
    out->writeRawData("-32768  ", 8);

    out->writeRawData("32767   ", 8);
    out->writeRawData("32767   ", 8);
    out->writeRawData("32767   ", 8);
    out->writeRawData("32767   ", 8);
    out->writeRawData("32767   ", 8);
    out->writeRawData("32767   ", 8);

    // prefiltering
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);

    // number of samples in each data record (8s)
    out->writeRawData("256     ", 8);
    out->writeRawData("256     ", 8);
    out->writeRawData("256     ", 8);
    out->writeRawData("256     ", 8);
    out->writeRawData("256     ", 8);
    out->writeRawData("30      ", 8);

    // reserved fields
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
}


void ConvertWindow::writetoFile(QString filename,QVector<qint32> *adcECGindQVpp,int dividend)
{
    QFile *outfilep;
    QTextStream *outstrp;
    QMessageBox msgBox;
    int i=0;
    outfilep = new QFile(filename);

    if(outfilep->exists())
    {
        msgBox.setWindowTitle("Out RR File Exists");
        msgBox.setText("Out RR file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delete outfilep;
            return;
        }
    }
    if(!outfilep->open(QIODevice::WriteOnly))
    {
        delete outfilep;
    }
    else
    {   outstrp = new QTextStream(outfilep);
        for(i = 1 ; i<adcECGindQVpp->size() ; i++)
        {
            *outstrp<<QString("%1.%2 %3.%4").arg(adcECGindQVpp->at(i)/dividend).arg(((int)(((float)adcECGindQVpp->at(i)/dividend)*1000))%1000,3,10,QChar('0')).arg((adcECGindQVpp->at(i)-adcECGindQVpp->at(i-1))/dividend).arg(((int)(((float)(adcECGindQVpp->at(i)-adcECGindQVpp->at(i-1))/dividend)*1000))%1000,3,10,QChar('0'))<<endl;
        }
        outfilep->close();
        delete outfilep;
        delete outstrp;
    }
}


void ConvertWindow::DtoMecgQVs(QVector<qint32> *adcECGindQVp, QVector<qint16> *adcQVp,
                              QVector<bool> *missBeatFlagQVp, QVector<qint16> *ecgRRQVp,
                              QVector<qint16> *ecgampQVp, qint32 decimateFactor)
{
    qint32 i, j;
    qint32 indexDiff;
    qint16 RRData, ampData, RRDataOld;
    qint32 dataCt, fillCt;

    // first fill up qvectors with data to write
    dataCt = 0;
    RRData = 0;
    ampData = 0;
    RRDataOld = 0;
    //fillCt = (qint32) (((float)(adcECGindqvectp->at(0))/((float) decimateFactor))+0.5);
    fillCt = (adcECGindQVp->at(0)+decimateFactor/2)/decimateFactor;
    for(j = 0; j < fillCt; j++)
    {
        ecgRRQVp->append(0);
        ecgampQVp->append(0);
        dataCt++;
    }
    // do this for all data except first since it establishes timebase
    for(i = 1; i < adcECGindQVp->size(); i++)
    {

        indexDiff = adcECGindQVp->at(i)-adcECGindQVp->at(i-1);
        if(!missBeatFlagQVp->at(i))    // use RRDataOld if missed a beat
        {
            RRData=(qint16)indexDiff*19.53125; //RR mapped from 0-3276.7ms to 0-32767, each sample=1.953125msec
            RRDataOld=RRData;
        }
        else
        {
            RRData=RRDataOld; //if missed beat(s)
        }
        ampData = adcQVp->at(adcECGindQVp->at(i));
        //fillCt = (qint32) (((float) indexDiff)/((float) decimateFactor)+0.5);
        fillCt = (indexDiff+decimateFactor/2)/decimateFactor;
        // fix any rounding error by adding or subtracting one at a time
        if((dataCt+fillCt)*decimateFactor < adcECGindQVp->at(i))
        {
            fillCt++;
        }
        else if((dataCt+fillCt)*decimateFactor > adcECGindQVp->at(i))
        {
            if(fillCt > 0) fillCt--;
        }

        for(j = 0; j < fillCt; j++)
        {
            ecgRRQVp->append(RRData);
            ecgampQVp->append(ampData);
            dataCt++;
        }
    }
}

/***
  * Write edf metadata file for ECG
  * must know the sampling rate to be written to, and outgoing format
  */
qint32 ConvertWindow::edfMecgwrite(QDataStream *outstrp, QVector<qint16> *ecgRRQVp,
                                   QVector<qint16> *ecgampQVp, qint32 decimateFactor,
                                   int FiledataRecordCt, qint64 firstFrmTS,
                                   QVector<qint64> *tagTSQVp,
                                   QVector<QString> *tagTextQVp)
{
    qint32 i, j;
    int calculateagaincount=ECGFFTOVERLAP/(ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor); //initialized to 4 to do full size FFT
    bool noMore;    // indicate if no more data to write into data records
    qint32 dataRecordCt;
    QString tempQS;
    // read data from vectors into correct format for edf
    // Write EDF in data record segments
    noMore = false;
    dataRecordCt = 0;

    float saveFFTstd[ECGFFTSIZE],saveSDNN[ECGFFTSIZE]; //add data set for RRave and SDNN
    int countfftdata=0;
    int countfftoverlapindex=0;
    QFourierTransformer transformer;  //should Setting a fixed size for the transformation
    transformer.setSize(ECGFFTSIZE);
    QVector <QComplexFloat> fftresult;
    QVector <float>  amplitudeFFT(ECGFFTSIZE);  //save the final fft result
    double re,im, RRave, tempSDNN, SDNN;
    float calculatedFFTstd[ECGFFTSIZE];
    qint16 tempLFfftdata,tempHFfftdata,tempLHRatio;  //save the fft value
    double tempsavetotalLFfft,tempsavetotalHFfft; //save the .... data
    double LFpersum,HFpersum,LHRatiosum;
    QByteArray tempQBA;

    for(i = 0; i < FiledataRecordCt; i++)
    {
        for(j = 0; j < ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor; j++) // count 256 samples/record
        {
            if((j+i*ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor)<ecgRRQVp->size())
                *outstrp << ecgRRQVp->at(j+i*ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor);
            else
                *outstrp << (qint16)0;
        }
        for(j = 0; j < ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor; j++)
        {
            if((j+i*ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor)<ecgampQVp->size())
                *outstrp << ecgampQVp->at(j+i*ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor);
            else
                *outstrp << (qint16)0;
        }

        //FFT
        if(calculateagaincount>=ECGFFTOVERLAP/(ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor)) //# record: 256*4=1024 samples for calc-again
        {
            calculateagaincount=0;
            countfftdata=0;
            for(j = countfftoverlapindex*ECGFFTOVERLAP; j <countfftoverlapindex*ECGFFTOVERLAP+ECGFFTSIZE ;j++)
            {
                if(j<ecgRRQVp->size())
                {
                    saveSDNN[countfftdata]= (ecgRRQVp->at(j)); //in 0.1msec
                    saveFFTstd[countfftdata]=0.5*(1.0-qCos((2.0*M_PI*countfftdata)/(ECGFFTSIZE-1)))*(((double)ecgRRQVp->at(j)));//in 0.1msec.
                }
                else
                {
                    saveSDNN[countfftdata]=(0);
                    saveFFTstd[countfftdata]=(0);
                }
                countfftdata++;
            }
            countfftoverlapindex++;
            //start calculate
            //calculation for fft
            transformer.forwardTransform(saveFFTstd,calculatedFFTstd);
            fftresult=transformer.toComplex(calculatedFFTstd);
            for(j=0;j<=ECGFFTSIZE/2;j++)
            {
                re=(double)fftresult[j].real()*(double)fftresult[j].real();
                im=(double)fftresult[j].imaginary()*(double)fftresult[j].imaginary();
                amplitudeFFT[j]=re+im;
            }//for(int j=0;j<=FFTSIZE/2;J++) end
            //LF
            tempsavetotalLFfft=0;
            for(j=LFA/LFHFTOINDEXRATIO;j<=LFB/LFHFTOINDEXRATIO;j++)
            {
                tempsavetotalLFfft+=amplitudeFFT[j];
            }
            //HF
            tempsavetotalHFfft=0;
            for(j=HFA/LFHFTOINDEXRATIO;j<=HFB/LFHFTOINDEXRATIO;j++)
            {
                tempsavetotalHFfft+=amplitudeFFT[j];
            }
            //LFper
            LFpersum = tempsavetotalLFfft/(tempsavetotalLFfft+tempsavetotalHFfft);
            //HFper
            HFpersum = (log10(tempsavetotalHFfft)-9.8062); //log HFpower -log(100*8000*8000)
            //LHRatio
            LHRatiosum = log(LFpersum/(1.0-LFpersum)); //log LHRatio
            //calc RRave, SDNN
            RRave=0;
            for(j=0;j<ECGFFTSIZE;j++) RRave+=saveSDNN[j]; //sum up
            RRave=RRave/ECGFFTSIZE; //average value
            //        HFpersum = RRave; //same scale as RR
            tempSDNN=0;
            for(j=0;j<ECGFFTSIZE;j++) tempSDNN+=(saveSDNN[j]-RRave)*(saveSDNN[j]-RRave); //sum up square of difference
            SDNN=qSqrt(tempSDNN/ECGFFTSIZE); //in 0.1mSec
            LFpersum = SDNN*10; //10x scale in 0.01msec

            //transfer double to qint16

            if(LFpersum>=0&&LFpersum<=32767) //SDNN
                tempLFfftdata=LFpersum;
            else if(LFpersum<0)
                tempLFfftdata=0;
            else if(LFpersum>=32767)
                tempLFfftdata=32767;

            if(HFpersum>=0&&HFpersum<=4) //log HFpower with 40db range
                tempHFfftdata=HFpersum*32767/4;
            else if(HFpersum<0)
                tempHFfftdata=0;
            else if(HFpersum>4)
                tempHFfftdata=32767;

            if(LHRatiosum>=-2&&LHRatiosum<=2) //log LHRatio 0.01 to 100
                tempLHRatio=LHRatiosum*32767/2;//max=1
            else if(LHRatiosum<-2)
                tempLHRatio=-32768;
            else if(LHRatiosum>2)
                tempLHRatio=32767;
        }
        calculateagaincount++;
        for(j = 0; j < ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor; j++) //repeat 256 * 4 times =1024: 32Hz*32"
        {
            //LFper
            *outstrp << tempLFfftdata; //actually SDNN*10
        }
        for(j = 0; j < ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor; j++)
        {
            //HFper
            *outstrp << tempHFfftdata; //actually LOG HF power
        }
        for(j = 0; j < ADCLEN*FRMSPERSEC*EDFDRDURSEC/decimateFactor; j++)
        {
            //LHRatio
            *outstrp << tempLHRatio; //LOG LHRatio
        }
        // every data record has EDF annotation to denote relative start time
        // EDF Annotations, max of 2 annotations per data record, each with 33 total chars
        tempQBA.clear();
        tempQBA.append(QString("+%1").arg(dataRecordCt*EDFDRDURSEC).toUtf8().append(QChar(20)).append(QChar(20)).append(QChar(0)));
        for(j = 0; j < 2; j++)
        {
            if(tagTSQVp->size() > 0)
            {
                tempQS = tagTextQVp->at(0);
                tempQS.truncate(14);    // limit text to 14 charcters
                tempQBA.append(QString("+%1").arg((float)(tagTSQVp->at(0)-firstFrmTS)/UCRTCFREQ).toUtf8().append(QChar(20)).append(tempQS.toUtf8()));
                tempQBA.append(QChar(20)).append(QChar(0));
                tagTSQVp->remove(0,1);
                tagTextQVp->remove(0,1);
            }
        }
        outstrp->writeRawData(tempQBA.leftJustified(60,0x00), 60);
        dataRecordCt++;
    }
    return dataRecordCt;
}


/***
  * Write EDF header especially for opi FFT data files, 1 sec data record duration
  * Inputs:
  *     out, ptr to output data stream
  *     lpidp, ptr to local patient id
  *     lridp, ptr to local recording id
  *     startDTp, ptr to starting date and time
  *     numDataRecs, number of data records
  * Returns:
  *     true, if successful
  *     false, if not successful
  */
bool ConvertWindow::edfMeeghdropiwrite(QDataStream *out, QString *lpidp, QString *lridp,
                                QDateTime *startDTp, qint32 numDataRecs)
{
    QString tempstr;
    int k;

    out->writeRawData("0       ", 8);     // edf version of data format
    out->writeRawData(lpidp->toUtf8().leftJustified(80,' ').data(),80);   // local patient identification
    out->writeRawData(lridp->toUtf8().leftJustified(80,' ').data(),80);   // local recording identification
    out->writeRawData(startDTp->toString("dd.MM.yyhh.mm.ss").toUtf8().data(),16); // startdate and starttime
    out->writeRawData("3072     ", 8);     // number of header bytes (256+11signals*256)
    out->writeRawData(QByteArray("EDF+C").leftJustified(44,' ').data(),44); // format type (reserved)
    out->writeRawData(QString("%1").arg(numDataRecs).toUtf8().leftJustified(8,' ').data(),8);  // number of data records
    out->writeRawData("8       ", 8);     // duration of a data record in seconds
    out->writeRawData("11  ", 4);       // number of signals: fft signal(10),annotatino

    out->writeRawData("M2              ", 16);//M2
    out->writeRawData("M1              ", 16);//M1
    out->writeRawData("G2              ", 16);//G2
    out->writeRawData("G1              ", 16);//G1
    out->writeRawData("UP              ", 16);//UP
    out->writeRawData("Beta            ", 16);//Beta
    out->writeRawData("Sigma           ", 16);//Sigma
    out->writeRawData("Alpha           ", 16);//alpha
    out->writeRawData("Theta           ", 16);//theta
    out->writeRawData("Delta           ", 16);//delta
    out->writeRawData("EDF Annotations ", 16);

    // transducer type
    for(k=0;k<11;k++)
        out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);

    // physical dimensions
    for(k=0;k<10;k++)
        out->writeRawData("db      ", 8);
    out->writeRawData("        ", 8);

    // physical mins and maxs
    for(k=0;k<10;k++)
    {
        tempstr.clear();
        out->writeRawData(tempstr.setNum(FFTMIN).append("        ").toLatin1().data(), 8);
    }
    out->writeRawData("-100    ", 8);

    for(k=0;k<10;k++)
    {
        tempstr.clear();
        out->writeRawData(tempstr.setNum(FFTMAX).append("        ").toLatin1().data(), 8);
    }
    out->writeRawData("100     ", 8);

    // digital mins and maxs
    for(k=0;k<11;k++)
        out->writeRawData("-32768  ", 8);

    for(k=0;k<11;k++)
        out->writeRawData("32767   ", 8);

    // prefiltering
    for(k=0;k<11;k++)
        out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);

    // number of samples in each data record (8s)
    for(k=0;k<10;k++)
        out->writeRawData("64       ", 8);
    out->writeRawData("30      ", 8);

    // reserved fields
    for(k=0;k<11;k++)
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);

    return true;
}


qint16 ConvertWindow::fftDoubleQInt16Conversion(double fftAmpSq)
{
    double tempdoub;
    qint16 retVal;

    tempdoub = FFTDBSCALE*(10.0*log10(fftAmpSq)-FFTDBOFFSET-70); //-72db default
    //tempdoub = fftAmpSq*32767/FFTMAX;

    // clip at extremes of 16bit 2's complement value
    if(tempdoub > 32767) retVal = 32767;
    else if(tempdoub < 0) retVal = 0; //remove negative spikes
    else retVal = (qint16) tempdoub;
    return retVal;
}


void ConvertWindow::edfMeegwrite(QDataStream *outstrp,
                                 QVector<qint16> *M2Qvp, QVector<qint16> *M1Qvp,
                                 QVector<qint16> *G2Qvp, QVector<qint16> *G1Qvp,
                                 QVector<qint16> *UPQvp, QVector<qint16> *BetaQvp,
                                 QVector<qint16> *SigmaQvp, QVector<qint16> *alphaQvp,
                                 QVector<qint16> *thetaQvp, QVector<qint16> *deltaQvp,
                                 qint32 numDataRecs , qint64 firstFrmTS, QVector<qint64> *tagTSQVp,
                                 QVector<QString> *tagTextQVp)
{
    QProgressDialog progQPD("Writing EEG EDF file",QString(),0,numDataRecs);
    qint32 i,j,k;
    QByteArray tempQBA;
    QString tempQS;

    progQPD.setWindowModality(Qt::WindowModal);
    progQPD.setMinimumDuration(2000);

    for(i = 0; i < numDataRecs; i++)  //count seconds
    {
        //prepare data
        if((i % 10) == 0)
        {
            progQPD.setValue(i);
            qApp->processEvents();
        }

        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<M2Qvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<M1Qvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<G2Qvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<G1Qvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<UPQvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<BetaQvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<SigmaQvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<alphaQvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<thetaQvp->at(i*EDFDRDURSEC+j);
        for(j = 0; j < EDFDRDURSEC; j++)
            for(k = 0; k < EEGSAMPLERATE; k++)
                *outstrp<<deltaQvp->at(i*EDFDRDURSEC+j);

        // EDF Annotations, max of 2 annotations per data record, each with 33 total chars
        tempQBA.clear();
        tempQBA.append(QString("+%1").arg(i*EDFDRDURSEC).toUtf8().append(QChar(20)).append(QChar(20)).append(QChar(0)));
        for(j = 0; j < 2; j++)
        {
            if(tagTSQVp->size() > 0)
            {
                tempQS = tagTextQVp->at(0);
                tempQS.truncate(14);    // limit text to 14 charcters
                tempQBA.append(QString("+%1").arg((float)(tagTSQVp->at(0)-firstFrmTS)/UCRTCFREQ).toUtf8().append(QChar(20)).append(tempQS.toUtf8()));
                tempQBA.append(QChar(20)).append(QChar(0));
                tagTSQVp->remove(0,1);
                tagTextQVp->remove(0,1);
            }
        }

        outstrp->writeRawData(tempQBA.leftJustified(60,0x00), 60);
    }
}


/***
  * Read in an edf file that is generic opi data. Assumes data is in right format.
  * Inputs:
  *     all data will be appended to end of qvectors
  * Returns:
  *     non-negative number indicating number of data records read
  *     -1, error
  */
qint32 ConvertWindow::edfDread(QDataStream *instrp, QDateTime startDT,
                               qint32 numDataRecs, qint32 dataRecDur,
                               qint32 numSignals, QVector<qint32> sampsPerDRQV,
                               QVector<qint64> *tsQVp, QVector<quint8> *skpQVp,
                               QVector<quint8> *batQVp, QVector<qint16> *adcQVp,
                               QVector<qint16> *tmpQVp, QVector<qint16> *axQVp,
                               QVector<qint16> *ayQVp, QVector<qint16> *azQVp,
                               QVector<qint16> *sqQVp, QVector<quint8> *edQVp,
                               QVector<qint64> *annOnsetTSQVp,
                               QVector<QString> *annTextQVp)
{
    qint16 tempadcs[ADCLEN*FRMSPERSEC*EDFDRDURSEC];
    qint16 tempaxs[ACCLEN/4*FRMSPERSEC*EDFDRDURSEC];
    qint16 tempays[ACCLEN/4*FRMSPERSEC*EDFDRDURSEC];
    qint16 tempazs[ACCLEN*FRMSPERSEC*EDFDRDURSEC];
    qint16 temptmps[TMPLEN*FRMSPERSEC*EDFDRDURSEC];
    qint16 tempsqs[1*FRMSPERSEC*EDFDRDURSEC];
    quint8 tempanns[128];
    qint32 i, j, dataRecCt;
    qint64 prevTS, tempTS;
    QStringList annsQL, annQL;
    qint32 newstep, azold, axold, ayold, az4old, ax2old, ay2old, ax3old, ay3old, ax4old, ay4old, az8old, az12old, az16old;
    newstep = 1;//init
    azold = 0;
    axold = 0;
    ayold = 0;
    az4old = 0;
    ax2old = 0;
    ay2old = 0;
    ax3old = 0;
    ay3old = 0;
    ax4old = 0;
    ay4old = 0;
    az8old = 0;
    az12old = 0;
    az16old = 0;
    // check to make sure things are right
    if((sampsPerDRQV.at(0) != ADCLEN*FRMSPERSEC*EDFDRDURSEC) ||
            (sampsPerDRQV.at(1) != ACCLEN/4*FRMSPERSEC*EDFDRDURSEC) ||
            (sampsPerDRQV.at(2) != ACCLEN/4*FRMSPERSEC*EDFDRDURSEC) ||
            (sampsPerDRQV.at(3) != ACCLEN*FRMSPERSEC*EDFDRDURSEC) ||
            (sampsPerDRQV.at(4) != TMPLEN*FRMSPERSEC*EDFDRDURSEC) ||
            (sampsPerDRQV.at(5) != ACCLEN/4*FRMSPERSEC*EDFDRDURSEC) ||
            ((sampsPerDRQV.at(6) != 64) && (sampsPerDRQV.at(6) != 30)))
        return -1;

    // Initialization
    prevTS = (startDT.toMSecsSinceEpoch()-QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000;
    prevTS -= ADCLEN*UCRTCFREQ/TSRTCFREQ;   // must make it previous TS
    if(prevTS < 0) prevTS = 0;
    dataRecCt = 0;

    // Read in data until end
    while(!instrp->atEnd()) // read until no more data
    {
        // adc data 8192 = sampsPerDRQV.at(0)*2bytes
        if(instrp->readRawData((char *)tempadcs, sampsPerDRQV.at(0)*2) < 0) break;  // not enough data
        if(instrp->readRawData((char *)tempaxs, sampsPerDRQV.at(1)*2) < 0) break;
        if(instrp->readRawData((char *)tempays, sampsPerDRQV.at(2)*2) < 0) break;
        if(instrp->readRawData((char *)tempazs, sampsPerDRQV.at(3)*2) < 0) break;
        if(instrp->readRawData((char *)temptmps, sampsPerDRQV.at(4)*2) < 0) break;
        if(instrp->readRawData((char *)tempsqs, sampsPerDRQV.at(5)*2) < 0) break;
        if(instrp->readRawData((char *)tempanns, sampsPerDRQV.at(6)*2) < 0) break;

        // put into qvectors because data record is complete
        for(i = 0; i < 1*FRMSPERSEC*EDFDRDURSEC; i++)
        {
            tsQVp->append(prevTS+ADCLEN*UCRTCFREQ/TSRTCFREQ);
            prevTS += ADCLEN*UCRTCFREQ/TSRTCFREQ;
            skpQVp->append(0);
            batQVp->append(1);
            for(j = 0; j < ADCLEN; j++)
                adcQVp->append(tempadcs[i*ADCLEN+j]);
            for(j = 0; j < ACCLEN/4; j++)
            {
                axQVp->append(tempaxs[i*ACCLEN/4+j]);
                ayQVp->append(tempays[i*ACCLEN/4+j]);
            }
            for(j = 0; j < ACCLEN; j++)
                azQVp->append(tempazs[i*ACCLEN+j]);
            for(j = 0; j < TMPLEN; j++)
                tmpQVp->append(temptmps[i*TMPLEN+j]);
            //sqQVp->append(calcAct(axQVp, ayQVp, azQVp, &newstep, &azold, &axold, &ayold, &az4old, &ax2old, &ay2old, &ax3old, &ay3old, &ax4old, &ay4old, &az8old, &az12old, &az16old));
            sqQVp->append(tempsqs[i+j]);    // preserve original so can keep sample quality
            edQVp->append(0);
        }
        // take care of annotations
        annsQL = QString::fromAscii((const char *)tempanns,128).split(QChar(0),QString::SkipEmptyParts);
        for(i = 0; i < annsQL.size(); i++)
        {
            annQL = annsQL.at(i).split(QChar(20),QString::SkipEmptyParts); // split each entry
            if(annQL.size() < 2) continue; // no tag entries
            // first parts is always the time
            tempTS = (qint64) (annQL.at(0).toFloat()*UCRTCFREQ+(startDT.toMSecsSinceEpoch()-QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000);
            for(j = 1; j < annQL.size(); j++)
            {
                annOnsetTSQVp->append(tempTS);
                annTextQVp->append(annQL.at(j));
            }
        }
        dataRecCt++;
    }

    return dataRecCt;
}


/***
  * Take the input stream of opi file and convert all frames
  * into output qvectors if they have the matching pdn
  * and correct wired sub-datacode. Does not add data for missing
  * frames
  * Inputs:
  *     instrp, pointer to input stream
  *     pdnDes, pdn desired used to filter frames
  *     skpQVp, pointer to QVector flag indicating 62 or 64
  *     batQVp, pointer to QVector battery data
  *     adcQVp, pointer to QVector adc data
  *     axQVp, pointer to QVector accelerometer x data
  *     ayQVp, pointer to QVector accelerometer y data
  *     azQVp, pointer to QVector accelerometer z data
  *     tmpQVp, pointer to QVector accelerometer x data
  *     sqQVp, pointer to QVector sample quality data
  *     edQVp, pointer to QVector frame ED data
  * Returns:
  *     usedFrmCt, number of frames used
  */
qint32 ConvertWindow::opiDread(QDataStream *instrp, quint8 pdnDes,
                               QVector<qint64> *tsQVp, QVector<quint8> *skpQVp,
                               QVector<quint8> *batQVp, QVector<qint16> *adcQVp,
                               QVector<qint16> *tmpQVp, QVector<qint16> *axQVp,
                               QVector<qint16> *ayQVp, QVector<qint16> *azQVp,
                               QVector<qint16> *sqQVp, QVector<quint8> *edQVp)
{
    OPIPKT_t inpkt;
    qint64 frmTS;
    quint8 wSubDataCode, pdn;
    qint16 adcData[ADCLEN];
    quint8 sampQual;
    qint32 i, usedFrmCt, readFrmCt;
    quint8 opiHdr[OPIHDRLEN];
    QProgressDialog *progQPDp;
    qint32 newstep, azold, axold, ayold, az4old, ax2old, ay2old, ax3old, ay3old, ax4old, ay4old, az8old, az12old, az16old;
    newstep = 1;//init
    azold = 0;
    axold = 0;
    ayold = 0;
    az4old = 0;
    ax2old = 0;
    ay2old = 0;
    ax3old = 0;
    ay3old = 0;
    ax4old = 0;
    ay4old = 0;
    az8old = 0;
    az12old = 0;
    az16old = 0;
    readFrmCt = 0;

    // read out header since file was reset
    for(i = 0; i < OPIHDRLEN; i++) *instrp >> opiHdr[i];
    // Get first valid frame for establishing first frame
    while (!instrp->atEnd())
    {
        readFrmCt++;
        if(opipkt_get_stream(&inpkt, instrp)) continue; // something wrong with packet
        if(inpkt.length != (TSFRMLEN-1)) continue; // wrong packet size
        wSubDataCode = inpkt.payload[0];
        if(wSubDataCode != 0x01) continue;   // needs to be truesense data (wireless or mem. mod)
        frmTS = (((qint64) inpkt.payload[WFRMHDRLEN-1]) << 40) + (((qint64) inpkt.payload[WFRMHDRLEN-1+1]) << 32) +
                (((qint64) inpkt.payload[WFRMHDRLEN-1+2]) << 24) + ((inpkt.payload[WFRMHDRLEN-1+3]) << 16) +
                (inpkt.payload[WFRMHDRLEN-1+4] << 8) + (inpkt.payload[WFRMHDRLEN-1+5]);
        pdn = inpkt.payload[WFRMHDRLEN-1+TSLEN];
        if (pdn != pdnDes) continue;
        // only need first adc data to determine quality
        adcData[0] = (inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN] << 8) + inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+1];
        sampQual = getSampQual(adcData);
        if(sampQual == 0) break;    // got packet with good sample quality
    }

    if (instrp->atEnd())
    {
        ui->msgPTE->appendPlainText(QString("PDN#%1: Not enough data").arg(pdnDes));
        return 0; // at end so nothing else to do
    }

    // for first valid frame get data and write it
    tsQVp->append(frmTS);
    skpQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+1] >> 7);
    batQVp->append(inpkt.payload[WFRMHDRLEN+TSLEN+1] & 0x01);
    for(i = 0; i < ADCLEN; i++)
        adcData[i] = (inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*i] << 8) + inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*i+1];
    tmpQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN] << 4);
    axQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN] << 8);
    ayQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+1] << 8);
    for(i = 0; i < ACCLEN; i++)
        azQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+2+i] << 8);
    edQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+ACCDLEN]);
    sampQual = getSampQual(adcData);    // func. also deletes sampQual info from adcData[0]
    if(sampQual>0)
    {
        sqQVp->append(-1000*sampQual); //*-1.5dB
    }
    else
        sqQVp->append(calcAct(axQVp, ayQVp, azQVp, &newstep, &azold, &axold, &ayold, &az4old, &ax2old, &ay2old, &ax3old, &ay3old, &ax4old, &ay4old, &az8old, &az12old, &az16old));

    // write adc data last because getSampQual modified adcData array
    for(i = 0; i < ADCLEN; i++) adcQVp->append(adcData[i]);

    usedFrmCt = 1;
    progQPDp = new QProgressDialog("Reading input file", QString(), 0, instrp->device()->size()/(TSFRMLEN+6));
    progQPDp->setWindowModality(Qt::WindowModal);
    progQPDp->setMinimumDuration(2000);

    // Process rest of frames
    while (!(instrp->atEnd()))
    {
        if((readFrmCt % 100) == 0)
        {
            progQPDp->setValue(readFrmCt);
               // make sure Main event loop gets processed
        }
        readFrmCt++;
        if(opipkt_get_stream(&inpkt, instrp)) continue; // something wrong with packet
        if((inpkt.length != (TSFRMLEN-1)) && (inpkt.length != (TSFRMLEN-5))) continue; // wrong packet size
        wSubDataCode = inpkt.payload[0];
        if(wSubDataCode != 0x01) continue;   // needs to be truesense data (wireless or mem. mod)
        frmTS = (((qint64) inpkt.payload[WFRMHDRLEN-1]) << 40) + (((qint64) inpkt.payload[WFRMHDRLEN-1+1]) << 32) +
                (((qint64) inpkt.payload[WFRMHDRLEN-1+2]) << 24) + ((inpkt.payload[WFRMHDRLEN-1+3]) << 16) +
                (inpkt.payload[WFRMHDRLEN-1+4] << 8) + (inpkt.payload[WFRMHDRLEN-1+5]);
        if(frmTS < tsQVp->at(tsQVp->size()-1)) continue;    // make sure monotonically increasing
        pdn = inpkt.payload[WFRMHDRLEN-1+TSLEN];
        if (pdn != pdnDes) continue;

        // if reached here, then this packet will be used
        usedFrmCt++;

        tsQVp->append(frmTS);
        skpQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+1] >> 7);
        batQVp->append(inpkt.payload[WFRMHDRLEN+TSLEN+1] & 0x01);

        if(inpkt.length == (TSFRMLEN-1))    // adcdata of 64 case
        {
            // get all data out into proper size variables
            for(i = 0; i < ADCLEN; i++)
                adcData[i] = (inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*i] << 8) + inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*i+1];
            tmpQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN] << 4);
            axQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN] << 8);
            ayQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+1] << 8);
            for(i = 0; i < ACCLEN; i++)
                azQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+2+i] << 8);
            edQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+ACCDLEN]);
            sampQual = getSampQual(adcData);    // func. also deletes sampQual info from adcData[0]

            // write adc data last because getSampQual modified adcData array
            for(i = 0; i < ADCLEN; i++) adcQVp->append(adcData[i]);
        }
        else    // adcdata of 62 case
        {
            for (i = 0; i < ADCLEN-2; i++)
                adcData[i] = (inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*i] << 8) + inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*i+1];
            tmpQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)] << 4);
            axQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN] << 8);
            ayQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+1] << 8);
            for(i = 0; i < ACCLEN; i++)
                azQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+2+i] << 8);
            edQVp->append(inpkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+ACCDLEN]);
            sampQual = getSampQual(adcData);    // func. also deletes sampQual info from adcData[0]

            // write adc data last because getSampQual modified adcData array
            for(i = 0; i < ADCLEN-2; i++) adcQVp->append(adcData[i]);
        }
        if(sampQual>0)
        {
            sqQVp->append(-1000*sampQual); //*-1.5dB
        }
        else
            sqQVp->append(calcAct(axQVp, ayQVp, azQVp, &newstep, &azold, &axold, &ayold, &az4old, &ax2old, &ay2old, &ax3old, &ay3old, &ax4old, &ay4old, &az8old, &az12old, &az16old));

    }
    delete progQPDp;

    return usedFrmCt;
}


QVector <qint32> ConvertWindow::findHRmax(QVector<qint16> *datap, int samplerate, int totalsecond, QVector<bool> *missBeatFlagqvectp,
                                          bool Rreset,bool extcall,int *Pavg,int *Davg,int *Dlast)
{
    QVector <qint32>  HRmaxindex;
    int Pnew,Dnew; //add Dlast in counts
    int dataindexcount,idcount,skipcount,totalsample; //to count the index of data
    bool skip,missHR,firstHR,validHR;//if the max value is bigger than 20000,we skip it; missHR: missed beat need reinitialize
    bool findit;//true for finding the max value
    bool maxormin;
    double maxvalue,minvalue,sumvalue,tmpvalue,lastvalue,nowvalue,nextvalue;//save the max and min and moving8
    double  maxindex,minindex,exmaxindex;//save the index of max and min,and ex max, ex min

    dataindexcount=0;
    skip=false;    //not yet
    missHR=false;  //not yet
    maxormin=true; //real maxormin
    validHR=false;
    totalsample=totalsecond*samplerate;
    skipcount=MINHRSEC*samplerate;
    lastvalue=0;
    nowvalue=0;
    nextvalue=0;
    HRmaxindex.clear();
    missBeatFlagqvectp->clear();

    if(!extcall && !Rreset) //Live and following calls
    {
        exmaxindex=-16;
        findit=true;   //already peak before
        validHR=true;
        goto FINDNEXTHR;
    }
    else
    {
        exmaxindex=0;
        skip=false;     //not yet
        maxormin=true;
        validHR=false;
    }
REFINDREF:       //find the 1st peak and polarity=POSITIVE here
    findit=false;
    (*Davg)=IDAVG*samplerate; //in counts
    (*Dlast)=(*Davg);  //default value
    (*Pavg)=PAVGMAX/6; //default value 18000/6=3000 (WAV 0.1V)
    while(dataindexcount<totalsample-8)
    {
        maxvalue=0; //initial
        minvalue=0; //initial
        lastvalue=0;
        idcount=(*Davg);
        nowvalue=datap->at(dataindexcount+4); //init
        nextvalue=datap->at(dataindexcount+5);
        sumvalue=0;
        for(int i=0;i<8;i++) sumvalue+=datap->at(dataindexcount+i); //moving8 sum
        for(int i=0;i<idcount;i++)
        {
            dataindexcount++;
            if(dataindexcount>=totalsample-8)
            {findit=false;
                break;
            }
            else
            {
                lastvalue=nowvalue; //push up
                nowvalue=nextvalue; //push up
                nextvalue=datap->at(dataindexcount+5);
                sumvalue+=datap->at(dataindexcount+7);
                sumvalue-=datap->at(dataindexcount-1);
                tmpvalue=sumvalue/8;

                if(tmpvalue>=maxvalue)
                {
                    maxvalue=tmpvalue;
                    maxindex=dataindexcount+4;
                }
                if(tmpvalue<=minvalue)
                {
                    minvalue=tmpvalue;
                    minindex=dataindexcount+4;
                }
            }
        } //for end
        if(maxvalue>PAVGMAX || minvalue<-1*PAVGMAX) //check PAVGMAX/PAVGMIN
            continue;
        else if(maxvalue<PAVGMIN && minvalue>-1*PAVGMIN)
            continue;
        else
        {
            findit=true;
            break;
        }
    } //while end
    if(findit)  //find the reference,start to find max
    {
        //decide the HR is in max or min
        {
            if(maxvalue<PAVGMIN) goto REFINDREF; //too small
            lastvalue=datap->at(maxindex-2)+datap->at(maxindex+2); //neighbor
            nextvalue=datap->at(maxindex-1)+datap->at(maxindex+1); //top exclude peak
            nowvalue=nextvalue-lastvalue; //difference
            if(nowvalue>PTOPCHECK*nextvalue) //change is too big >22.5%
            {
                dataindexcount=maxindex+GAPSKIP; //back to peak location+10
                goto REFINDREF;
            }
            maxormin=true; //valid peak in max
            exmaxindex=maxindex;
            HRmaxindex.append(maxindex);
            missBeatFlagqvectp->append(true);
            dataindexcount=maxindex+skipcount; //back to peak location+skipcount
            (*Pavg)=maxvalue; //set 1st Pavg
            if(!extcall)  return HRmaxindex; //Live mode: just 1 peak
        }
        missHR=false;
        firstHR=true;
        validHR=true;

FINDNEXTHR:
        skip=false;
        //start to find next heart beat
        while(dataindexcount<totalsample-8)
        {
            //workingstatusshow->setworkingstatus(dataindexcount);
            maxvalue=0; //start from 0
            minvalue=0; //start from 0
            lastvalue=0;
            idcount=(int)(*Davg)*INCASERATIO;//search range
            if(extcall && validHR==true && idcount>skipcount) idcount-=skipcount; //for valid peak skipcount
            if(!extcall && idcount>(totalsample-8)) idcount=totalsample-8; //live mode

            nowvalue=datap->at(dataindexcount+4); //init
            nextvalue=datap->at(dataindexcount+5);
            sumvalue=0;
            for(int i=0;i<8;i++) sumvalue+=datap->at(dataindexcount+i); //moving8
            for(int i=0;i<idcount;i++)
            {
                dataindexcount++;
                if(dataindexcount>=totalsample-8)
                {
                    skip=true;
                    break;
                }
                lastvalue=nowvalue; //push up
                nowvalue=nextvalue; //push up
                nextvalue=datap->at(dataindexcount+5);
                sumvalue+=datap->at(dataindexcount+7);
                sumvalue-=datap->at(dataindexcount-1);
                tmpvalue=sumvalue/8;

                if(maxormin==false) //negative polarity
                {
                    tmpvalue=-1*tmpvalue; //inverse polarity
                }
                if(tmpvalue>maxvalue) //possible new peak
                {               maxvalue=tmpvalue;
                    maxindex=dataindexcount+4;
                }
                goto FOREND;

FOREND:     ;
            }//for end
            Dnew=(maxindex-exmaxindex);

            if(maxvalue<(*Pavg)*PAVGCHANGE||maxvalue<=PAVGMIN) //if peak is too small
            {
                if(Dnew>IDAVG*samplerate*2.1) goto REFINDREF; // <40bpm
                validHR=false;
                goto FINDNEXTHR;  //find again
            }
            else //high enough peak
            {
                if(maxvalue>(*Pavg)/PAVGCHANGE ||maxvalue>PAVGMAX) //peak increase too much
                {
                    if(Dnew>IDAVG*samplerate*2.1) goto REFINDREF; // <40bpm
                    dataindexcount=(int)maxindex+GAPSKIP; //back to peak location+10
                    validHR=false;
                    goto FINDNEXTHR; //not valid peak
                }
                lastvalue=datap->at(maxindex-2)+datap->at(maxindex+2); //neighbor
                nextvalue=datap->at(maxindex-1)+datap->at(maxindex+1); //top excluding peak
                nowvalue=nextvalue-lastvalue; //difference
                if(maxormin==false) //negative polarity
                {
                    nextvalue=-1*nextvalue;
                    nowvalue=-1*nowvalue; //invert polarity
                }
                if(nowvalue>nextvalue*PTOPCHECK) //change is too big >22.5%
                {
                    dataindexcount=(int)maxindex+GAPSKIP; //back to peak location+10
                    validHR=false; //
                    goto FINDNEXTHR; //not valid peak
                }
                if(Dnew>skipcount && Dnew>(*Davg)*DAVGCHANGE)  //greater than minimum, real HR
                {
                    if(Dnew>IDAVG*samplerate*2.1) goto REFINDREF; // <40bpm
                    if(Dnew>(*Davg)/DAVGCHANGE) //too long
                        missHR=true; //missed beat
                    else missHR=false;
                    Pnew=maxvalue;
                    (*Pavg)= PAVGRATIO*(*Pavg) + (1-PAVGRATIO)*Pnew;
                    dataindexcount=(int)maxindex+skipcount; //skipcount
                    exmaxindex=maxindex;  //only updated if valid peak is found
                    HRmaxindex.append(maxindex);
                    missBeatFlagqvectp->append(missHR);
                    if (missHR==true)  //missed beat before, this is first beat
                    {
                        firstHR=true;
                        missHR=false; //reset missHR
                    }
                    else if (firstHR==true) //this is 2nd beat after missing
                    {
                        (*Dlast)=Dnew; //update Dlast
                        (*Davg)=(*Davg)*DAVGRATIO+Dnew*(1-DAVGRATIO); //update Davg
                        firstHR=false; //reset
                    }
                    else //not missed before nor now => update
                    {
                        (*Dlast)=Dnew; //update Dlast
                        (*Davg)=(*Davg)*DAVGRATIO+Dnew*(1-DAVGRATIO); //update Davg
                    }
                    if(!extcall) return HRmaxindex; //Live mode: just 1 peak
                } //end Dnew not too short
                else //Dnew too short, ignore it
                {
                    validHR=false;
                    dataindexcount=(int)maxindex+GAPSKIP; //go back to peak found
                    goto FINDNEXTHR;  //if noise ,we refind the reference
                }
            }//if end

            if(skip)
            {
                break;
            }
        }//while end
    }//if findit end
    return HRmaxindex;
}


