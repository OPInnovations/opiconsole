#include "signalviewer.h"
#include "ui_signalviewer.h"

QPen FWredPen(Qt::red);
QPen FWgreenPen(Qt::green);
QPen FWblackPen(Qt::black);

#define  FWSTARTCOLOR      FWblackPen
#define  FWENDCOLOR        FWblackPen
#define  FWHIGHLIGHTCOLOR  FWgreenPen
#define  FWSIGNALCOLOR     FWredPen    //define the color of signal line


//GENERIC
signalviewer::signalviewer(QVector<qint16> *adcqvectp, QVector<qint16> *accxqvectp,
                   QVector<qint16> *accyqvectp, QVector<qint16> *acczqvectp,
                   QVector<qint16> *tempqvectp,QVector <qint64> *mytsPointer,qint64 *myfirstFrmTSp,qint64 *mylastFrmTSp,
                   QVector<qint64> *Tsqvectp,QVector<QString> *Textqvectp,quint8 pdntitle,qint8 type,
                   QWidget *parent) :
    QDialog(parent),
    ui(new Ui::signalviewer)
{  
    ui->setupUi(this);
    int j,i;
    while(adcqvect.capacity()!=adcqvectp->capacity())
    adcqvect.reserve(adcqvectp->capacity());
    while(accxqvect.capacity()!=accxqvectp->capacity())
    accxqvect.reserve(accxqvectp->capacity());
    while(accyqvect.capacity()!=accyqvectp->capacity())
    accyqvect.reserve(accyqvectp->capacity());
    while(acczqvect.capacity()!=acczqvectp->capacity())
    acczqvect.reserve(acczqvectp->capacity());
    while(tempqvect.capacity()!=tempqvectp->capacity())
    tempqvect.reserve(tempqvectp->capacity());
    while( mytsQV.capacity()!=mytsPointer->capacity())
    mytsQV.reserve(mytsPointer->capacity());    
    //set scene
    setUpTotalScene();
    ui->pushButton_2D_accel->setVisible(true);
    ui->checkReverse->setVisible(true);
    this->showlabelcontrol(0);
    Filetype=type;
    mypdn = pdntitle;
    this->setWindowTitle(QString("[%1]").arg((int)pdntitle));
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    //assign pointer
    //must use this way,or this will has error
    if(type<5)   //generic  5
    {
        if((tempqvectp->count()/(FRMSPERSEC*EDFDRDURSEC))<1)
        {
            for(i = 0; i < adcqvectp->count(); i++)
                adcqvect.append( (*adcqvectp)[(i)]);

            for(i = 0; i < accxqvectp->count(); i++)
                accxqvect.append( (*accxqvectp)[(i)]);

            for(i = 0; i < accyqvectp->count(); i++)
                accyqvect.append( (*accyqvectp)[(i)]);

            for(i = 0; i < acczqvectp->count(); i++)
                acczqvect.append( (*acczqvectp)[(i)]);

            for(i = 0; i < tempqvectp->count(); i++)
                tempqvect.append( (*tempqvectp)[(i)]);

            for(i = 0;i < mytsPointer->count(); i++)
                mytsQV.append( (*mytsPointer)[(i)]);
        }
        else
        {
            for( j = 0; j < tempqvectp->count()/(FRMSPERSEC*EDFDRDURSEC); j++)
            {
                // make sure there is enough data for another data record, otherwise get out
                if((((j+1)*ADCLEN*FRMSPERSEC*EDFDRDURSEC-1) > adcqvectp->count()) ||
                        (((j+1)*TMPLEN*FRMSPERSEC*EDFDRDURSEC-1) > tempqvectp->count()) ||
                        (((j+1)*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC-1) > accxqvectp->count()) ||
                        (((j+1)*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC-1) > accyqvectp->count()) ||
                        (((j+1)*ACCLEN*FRMSPERSEC*EDFDRDURSEC-1) > acczqvectp->count()) ||
                        (((j+1)*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC-1) > mytsPointer->count())
                        )
                    break;

                for(i = 0; i < ADCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                {
                    adcqvect.append( (*adcqvectp)[j*ADCLEN*FRMSPERSEC*EDFDRDURSEC+i]);
                }

                for(i = 0; i < ACCLEN/4*FRMSPERSEC*EDFDRDURSEC; i++)
                    accxqvect.append( (*accxqvectp)[(j*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC+i)]);


                for(i = 0; i < ACCLEN/4*FRMSPERSEC*EDFDRDURSEC; i++)
                    accyqvect.append( (*accyqvectp)[(j*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC+i)]);


                for(i = 0; i < ACCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                    acczqvect.append( (*acczqvectp)[(j*ACCLEN*FRMSPERSEC*EDFDRDURSEC+i)]);

                for(i = 0; i < TMPLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                    tempqvect.append( (*tempqvectp)[(j*TMPLEN*FRMSPERSEC*EDFDRDURSEC+i)]);


                for(i = 0; i< ACCLEN/4*FRMSPERSEC*EDFDRDURSEC; i++)
                    mytsQV.append( (*mytsPointer)[(j*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC+i)]);
            }
        }
    }

    TSqvect=Tsqvectp;
    Textqvect=Textqvectp;
    QDateTime firstFrmDT, lastFrmDT;
    myfirstFrmTS=myfirstFrmTSp;
    mylastFrmTS=mylastFrmTSp;
    (*mylastFrmTS) = mytsQV.at(mytsQV.count()-1);
    originalfirst= (*myfirstFrmTS);
    originallast= (*mylastFrmTS);
    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*myfirstFrmTS)*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*mylastFrmTS)*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);
    highlightitemindex.clear();
}


void signalviewer::showEvent(QShowEvent *)
{
    reset();
    while(!this->isVisible());
    //set timerslider
    if(Filetype<5)
    {
        while(ui->TimeSlider==NULL)
            qApp->processEvents();
//         qDebug()<<"reset R1";
        while(!ui->TimeSlider->isEnabled())
            qApp->processEvents();
//         qDebug()<<"reset R2";
        if(accxqvect.count()/8>FWMAXSECONDINONESCENE)
            ui->TimeSlider->setMaximum(FWMAXSECONDINONESCENE);//
        else
            ui->TimeSlider->setMaximum(accxqvect.count()/8-1);//
    }
    else if(Filetype<7)
    {
        while(ui->TimeSlider==NULL)
            qApp->processEvents();
//         qDebug()<<"reset R3";
        while(!ui->TimeSlider->isEnabled())
            qApp->processEvents();
//         qDebug()<<"reset R4";
        if(accxqvect.count()/32>FWECGMAXSECONDINONESCENE)
            ui->TimeSlider->setMaximum(FWECGMAXSECONDINONESCENE);//
        else
            ui->TimeSlider->setMaximum(accxqvect.count()/32-1);//
    }
    else if(Filetype<9)
    {
        while(ui->TimeSlider==NULL)
            qApp->processEvents();
//         qDebug()<<"reset R5";
        while(!ui->TimeSlider->isEnabled())
            qApp->processEvents();
//         qDebug()<<"reset R6";
        if(M2qvect.count()/8>FWEEGMAXSECONDINONESCENE)
           ui->TimeSlider->setMaximum(FWEEGMAXSECONDINONESCENE);//
        else
           ui->TimeSlider->setMaximum(M2qvect.count()/8-1);//
    }
    ui->TimeSlider->setValue(ui->TimeSlider->maximum());
    ui->line->setGeometry(ui->drawSignalA->x()+ui->drawSignalA->width()/2,ui->line->y(),ui->line->width(),ui->line->height());

    if(Filetype<5)
    {
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        //adc
        draw(ui->drawSignalA,sceneSignalA,FWADCDIGITALMAX,-1*FWADCDIGITALMAX,&adcqvect,adcgainvalue,signal_index_begin*64,&changablemiddleA,FWSAMPLERATEADC);
        //accx
        draw(ui->drawSignalB,sceneSignalB,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&accxqvect,othergainvalue,signal_index_begin,&changablemiddleB,FWSAMPLERATEACCX);
        //accy
        draw(ui->drawSignalC,sceneSignalC,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&accyqvect,othergainvalue,signal_index_begin,&changablemiddleC,FWSAMPLERATEACCY);
        //accz
        draw(ui->drawSignalD,sceneSignalD,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&acczqvect,othergainvalue,signal_index_begin*4,&changablemiddleD,FWSAMPLERATEACCZ);
        //temp
        draw(ui->drawSignalE,sceneSignalE,FWTMPDIGITALMAX,FWTMPDIGITALMIN,&tempqvect,othergainvalue,signal_index_begin,&changablemiddleE,FWSAMPLERATETEMP);
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
    else if(Filetype <7)
    {
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        //RR
        draw(ui->drawSignalA,sceneSignalA,FWECGDIGITALMAX,FWECGDIGITALMIN,&adcqvect,adcgainvalue,signal_index_begin,&changablemiddleA,FWSAMPLERATERR);
        //amp
        draw(ui->drawSignalB,sceneSignalB,FWECGDIGITALMAX,FWECGDIGITALMIN,&accxqvect,othergainvalue,signal_index_begin,&changablemiddleB,FWSAMPLERATEAMP);
        //LFper=SDNN*10
        draw(ui->drawSignalC,sceneSignalC,FWECGDIGITALMAX,0,&accyqvect,othergainvalue,signal_index_begin,&changablemiddleC,FWSAMPLERATELFPER);
        //HFper=HFpower log
        draw(ui->drawSignalD,sceneSignalD,FWACCDIGITALMAX,0,&acczqvect,othergainvalue,signal_index_begin,&changablemiddleD,FWSAMPLERATEHFPER);
        //LHRatio=log
        draw(ui->drawSignalE,sceneSignalE,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&tempqvect,othergainvalue,signal_index_begin,&changablemiddleE,FWSAMPLERATELHRATIO);
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
    }
    else if(Filetype <9)
    {
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        switchdraw(ui->drawSignalA,sceneSignalA,adcgainvalue,signal_index_begin,&changablemiddleA,EEGSAMPLERATE,ui->comboBoxSignalA->currentIndex());
        switchdraw(ui->drawSignalB,sceneSignalB,othergainvalue,signal_index_begin,&changablemiddleB,EEGSAMPLERATE,ui->comboBoxSignalB->currentIndex());
        switchdraw(ui->drawSignalC,sceneSignalC,othergainvalue,signal_index_begin,&changablemiddleC,EEGSAMPLERATE,ui->comboBoxSignalC->currentIndex());
        switchdraw(ui->drawSignalD,sceneSignalD,othergainvalue,signal_index_begin,&changablemiddleD,EEGSAMPLERATE,ui->comboBoxSignalD->currentIndex());
        switchdraw(ui->drawSignalE,sceneSignalE,othergainvalue,signal_index_begin,&changablemiddleE,EEGSAMPLERATE,ui->comboBoxSignalE->currentIndex());
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);       
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
}


void signalviewer::showlabelcontrol(int choose)
{
    ui->GenericlabelADC->setVisible(false);
    ui->GenericlabelTemp->setVisible(false);
    ui->GenericlabelX->setVisible(false);
    ui->GenericlabelY->setVisible(false);
    ui->GenericlabelZ->setVisible(false);
    ui->ECGlabelamp->setVisible(false);
    ui->ECGlabelRR->setVisible(false);
    ui->ECGlabelHF->setVisible(false);
    ui->ECGlabelLH->setVisible(false);
    ui->ECGlabelLF->setVisible(false);
    ui->comboBoxSignalA->setVisible(false);
    ui->comboBoxSignalB->setVisible(false);
    ui->comboBoxSignalC->setVisible(false);
    ui->comboBoxSignalD->setVisible(false);
    ui->comboBoxSignalE->setVisible(false);
    switch(choose)
    {
    case 0:
        ui->GenericlabelADC->setVisible(true);
        ui->GenericlabelTemp->setVisible(true);
        ui->GenericlabelX->setVisible(true);
        ui->GenericlabelY->setVisible(true);
        ui->GenericlabelZ->setVisible(true);
        break;
    case 1:
        ui->ECGlabelamp->setVisible(true);
        ui->ECGlabelRR->setVisible(true);
        ui->ECGlabelHF->setVisible(true);
        ui->ECGlabelLH->setVisible(true);
        ui->ECGlabelLF->setVisible(true);
        break;
    case 2:
        ui->comboBoxSignalA->setVisible(true);
        ui->comboBoxSignalB->setVisible(true);
        ui->comboBoxSignalC->setVisible(true);
        ui->comboBoxSignalD->setVisible(true);
        ui->comboBoxSignalE->setVisible(true);
        break;
    }
}


//ecg
signalviewer::signalviewer(int dummy, float dummy2, QVector<qint16> *adcqvectp, QVector<qint16> *accxqvectp,
                           QVector<qint16> *accyqvectp, QVector<qint16> *acczqvectp, QVector<qint16> *tempqvectp,
                           QVector<qint64> *mytsPointer, qint64 *myfirstFrmTSp, qint64 *mylastFrmTSp, QVector<qint64> *Tsqvectp,
                           QVector<QString> *Textqvectp, quint8 pdntitle, qint8 type, QWidget *parent):
    QDialog(parent),
    ui(new Ui::signalviewer)
{
    int j,i;

    qApp->processEvents();
    while(adcqvect.capacity()!=adcqvectp->capacity())
    adcqvect.reserve(adcqvectp->capacity());
    while(accxqvect.capacity()!=accxqvectp->capacity())
    accxqvect.reserve(accxqvectp->capacity());
    while(accyqvect.capacity()!=accyqvectp->capacity())
    accyqvect.reserve(accyqvectp->capacity());
    while(acczqvect.capacity()!=acczqvectp->capacity())
    acczqvect.reserve(acczqvectp->capacity());
    while(tempqvect.capacity()!=tempqvectp->capacity())
    tempqvect.reserve(tempqvectp->capacity());
    while( mytsQV.capacity()!=mytsPointer->capacity())
    mytsQV.reserve(mytsPointer->capacity());

    ui->setupUi(this);
    //set scene
    setUpTotalScene();
    ui->pushButton_2D_accel->setVisible(false);
    ui->checkReverse->setVisible(false);
    this->showlabelcontrol(1);
    Filetype=type;
    mypdn = pdntitle;
    this->setWindowTitle(QString("[%1]").arg((int)pdntitle));
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    //assign pointer
    //must use this way,or this will has error

    if(type<7)
    {
        if((tempqvectp->count()/(ECGSAMPLERATE*EDFDRDURSEC))<1)
        {
            for(i = 0; i < adcqvectp->count(); i++)
                adcqvect.append( (*adcqvectp)[(i)]);

            for(i = 0; i < accxqvectp->count(); i++)
                accxqvect.append( (*accxqvectp)[(i)]);

            for(i = 0; i < accyqvectp->count(); i++)
                accyqvect.append( (*accyqvectp)[(i)]);

            for(i = 0; i < acczqvectp->count(); i++)
                acczqvect.append( (*acczqvectp)[(i)]);

            for(i = 0; i < tempqvectp->count(); i++)
                tempqvect.append( (*tempqvectp)[(i)]);

            for(i = 0; i< mytsPointer->count(); i++)
                mytsQV.append( (*mytsPointer)[(i)]);
        }
        else
        {
            for( j = 0; j < tempqvectp->count()/(ECGSAMPLERATE*EDFDRDURSEC); j++)
            {
                // make sure there is enough data for another data record, otherwise get out
                if((((j+1)*ACCLEN*FRMSPERSEC*EDFDRDURSEC-1) > adcqvectp->count()) ||
                        (((j+1)*ACCLEN*FRMSPERSEC*EDFDRDURSEC-1) > tempqvectp->count()) ||
                        (((j+1)*ACCLEN*FRMSPERSEC*EDFDRDURSEC-1) > accxqvectp->count()) ||
                        (((j+1)*ACCLEN*FRMSPERSEC*EDFDRDURSEC-1) > accyqvectp->count()) ||
                        (((j+1)*ACCLEN*FRMSPERSEC*EDFDRDURSEC-1) > acczqvectp->count()) ||
                        (((j+1)*ACCLEN*FRMSPERSEC*EDFDRDURSEC-1) > mytsPointer->count()))
                    break;
                qApp->processEvents();
                for(i = 0; i < ACCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                    adcqvect.append( (*adcqvectp)[(j*ACCLEN*FRMSPERSEC*EDFDRDURSEC+i)]);//RR
                for(i = 0; i < ACCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                    accxqvect.append( (*accxqvectp)[(j*ACCLEN*FRMSPERSEC*EDFDRDURSEC+i)]);//amp
                for(i = 0; i < ACCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                    accyqvect.append( (*accyqvectp)[(j*ACCLEN*FRMSPERSEC*EDFDRDURSEC+i)]);//LFper=SDNN
                for(i = 0; i < ACCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                    acczqvect.append( (*acczqvectp)[(j*ACCLEN*FRMSPERSEC*EDFDRDURSEC+i)]); //HFper=HFpower log
                for(i = 0; i < ACCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                    tempqvect.append( (*tempqvectp)[(j*ACCLEN*FRMSPERSEC*EDFDRDURSEC+i)]); //LHRatio=log
                for(i = 0; i< ACCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
                    mytsQV.append( (*mytsPointer)[(j*ACCLEN*FRMSPERSEC*EDFDRDURSEC+i)]);
            }
        }
    }
    TSqvect=Tsqvectp;
    Textqvect=Textqvectp;

    QDateTime firstFrmDT, lastFrmDT;
    myfirstFrmTS=myfirstFrmTSp;
    mylastFrmTS=mylastFrmTSp;
    (*mylastFrmTS) = mytsQV.at(mytsQV.count()-1);
    originalfirst= (*myfirstFrmTS);
    originallast= (*mylastFrmTS);
    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*myfirstFrmTS)*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*mylastFrmTS)*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);
    highlightitemindex.clear();
}


//EEG
signalviewer::signalviewer(float dummy,QVector<qint16> *M2qvectp, QVector<qint16> *M1qvectp,
                           QVector<qint16> *G2qvectp, QVector<qint16> *G1qvectp,
                           QVector<qint16> *UPqvectp, QVector<qint16> *Betaqvectp,
                           QVector<qint16> *Sigmaqvectp, QVector<qint16> *alphaqvectp,
                           QVector<qint16> *thetaqvectp, QVector<qint16> *deltaqvectp,
                           QVector<qint64> *mytsPointer, qint64 *myfirstFrmTSp,
                           qint64 *mylastFrmTSp, QVector<qint64> *Tsqvectp,
                           QVector<QString> *Textqvectp, quint8 pdntitle, qint8 type, QWidget *parent):
    QDialog(parent),
    ui(new Ui::signalviewer)
{
    int j,i;

    qApp->processEvents();

    while(M2qvect.capacity()!=M2qvectp->capacity())
    M2qvect.reserve(M2qvectp->capacity());
    while(M1qvect.capacity()!=M1qvectp->capacity())
    M1qvect.reserve(M1qvectp->capacity());
    while(G2qvect.capacity()!=G2qvectp->capacity())
    G2qvect.reserve(G2qvectp->capacity());
    while(G1qvect.capacity()!=G1qvectp->capacity())
    G1qvect.reserve(G1qvectp->capacity());
    while(UPqvect.capacity()!=UPqvectp->capacity())
    UPqvect.reserve(UPqvectp->capacity());
    while(Betaqvect.capacity()!=Betaqvectp->capacity())
    Betaqvect.reserve(Betaqvectp->capacity());
    while(Sigmaqvect.capacity()!=Sigmaqvectp->capacity())
    Sigmaqvect.reserve(Sigmaqvectp->capacity());
    while(alphaqvect.capacity()!=alphaqvectp->capacity())
    alphaqvect.reserve(alphaqvectp->capacity());
    while(thetaqvect.capacity()!=thetaqvectp->capacity())
    thetaqvect.reserve(thetaqvectp->capacity());
    while(deltaqvect.capacity()!=deltaqvectp->capacity())
    deltaqvect.reserve(deltaqvectp->capacity());
    while(mytsQV.capacity()!=mytsPointer->capacity())
    mytsQV.reserve(mytsPointer->capacity());

    ui->setupUi(this);
    //set scene
    setUpTotalScene();
    ui->pushButton_2D_accel->setVisible(false);
    ui->checkReverse->setVisible(false);
    this->showlabelcontrol(2);
    Filetype=type;
    mypdn = pdntitle;
    this->setWindowTitle(QString("[%1]").arg((int)pdntitle));
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    //assign pointer
    //must use this way,or this will has error

    if(type==7||type==8)
    {
        if((M2qvectp->count()/(EEGSAMPLERATE*EEGEDFDRDURSEC))<1)
        {
            for(i = 0; i < M2qvectp->count(); i++)
                M2qvect.append( (*M2qvectp)[(i)]); //M2

            for(i = 0; i < M1qvectp->count(); i++)
                M1qvect.append( (*M1qvectp)[(i)]);//M1

            for(i = 0; i < G2qvectp->count(); i++)
                G2qvect.append( (*G2qvectp)[(i)]);//G2

            for(i = 0; i < G1qvectp->count(); i++)
                G1qvect.append( (*G1qvectp)[(i)]);//G1

            for(i = 0; i < UPqvectp->count(); i++)
                UPqvect.append( (*UPqvectp)[(i)]);//UP

            for(i = 0; i < Betaqvectp->count(); i++)
                Betaqvect.append( (*Betaqvectp)[(i)]);//Beta

            for(i = 0; i < Sigmaqvectp->count(); i++)
                Sigmaqvect.append( (*Sigmaqvectp)[(i)]);//Sigma

            for(i = 0; i < alphaqvectp->count(); i++)
                alphaqvect.append( (*alphaqvectp)[(i)]); //alpha

            for(i = 0; i < thetaqvectp->count(); i++)
                thetaqvect.append( (*thetaqvectp)[(i)]);//theta

            for(i = 0; i < deltaqvectp->count(); i++)
                deltaqvect.append( (*deltaqvectp)[(i)]);//delta

            for(i = 0; i <mytsPointer->count() ; i++)
                mytsQV.append( (*mytsPointer)[(i)]);
        }
        else
        {
            for( j = 0; j < M2qvectp->count()/(EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC); j++)
            {  // make sure there is enough data for another data record, otherwise get out
                if((((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > M2qvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > M1qvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > G2qvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > G1qvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > UPqvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > Betaqvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > Sigmaqvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > alphaqvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > thetaqvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > deltaqvectp->count()) ||
                        (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > mytsPointer->count())
                        )
                    break;
                qApp->processEvents();
                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    M2qvect.append( (*M2qvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]); //M2

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    M1qvect.append( (*M1qvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//M1

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    G2qvect.append( (*G2qvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//G2

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    G1qvect.append( (*G1qvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//G1

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    UPqvect.append( (*UPqvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//UP

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    Betaqvect.append( (*Betaqvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//Beta

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    Sigmaqvect.append( (*Sigmaqvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//Sigma

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    alphaqvect.append( (*alphaqvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//alpha

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    thetaqvect.append( (*thetaqvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//theta

                for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                    deltaqvect.append( (*deltaqvectp)[(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)]);//delta

                for(i = 0; i< ACCLEN/4*FRMSPERSEC*EEGEDFDRDURSEC; i++)
                    mytsQV.append( (*mytsPointer)[(j*ACCLEN/4*FRMSPERSEC*EEGEDFDRDURSEC+i)]);
            }
        }
    }

    TSqvect=Tsqvectp;
    Textqvect=Textqvectp;
    QDateTime firstFrmDT, lastFrmDT;
    myfirstFrmTS=myfirstFrmTSp;
    mylastFrmTS=mylastFrmTSp;
    (*mylastFrmTS) = mytsQV.at(mytsQV.count()-1);
    originalfirst= (*myfirstFrmTS);
    originallast= (*mylastFrmTS);
    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*myfirstFrmTS)*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*mylastFrmTS)*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);
    highlightitemindex.clear();
    ui->comboBoxSignalB->setCurrentIndex(3);//3 G1
    ui->comboBoxSignalC->setCurrentIndex(6);//6 Sigma
    ui->comboBoxSignalD->setCurrentIndex(7);//7 Alpha
    ui->comboBoxSignalE->setCurrentIndex(9);//9 Delta
}


void signalviewer::tagcontrol(bool con)
{
    ui->TagText->setHidden(!con);
    ui->label_11->setHidden(!con);
    ui->pushButton_TM->setHidden(!con);
    ui->pushButton_TP->setHidden(!con);
    ui->TagText_2->setHidden(!con);
}


void signalviewer::setUpTotalScene()
{   //new all scenes for graphics view
    sceneSignalA = new QGraphicsScene(ui->drawSignalA->x(),ui->drawSignalA->y(),ui->drawSignalA->width(),ui->drawSignalA->height(),this);
    sceneSignalB = new QGraphicsScene(ui->drawSignalB->x(),ui->drawSignalB->y(),ui->drawSignalB->width(),ui->drawSignalB->height(),this);
    sceneSignalC = new QGraphicsScene(ui->drawSignalC->x(),ui->drawSignalC->y(),ui->drawSignalC->width(),ui->drawSignalC->height(),this);
    sceneSignalD = new QGraphicsScene(ui->drawSignalD->x(),ui->drawSignalD->y(),ui->drawSignalD->width(),ui->drawSignalD->height(),this);
    sceneSignalE = new QGraphicsScene(ui->drawSignalE->x(),ui->drawSignalE->y(),ui->drawSignalE->width(),ui->drawSignalE->height(),this);
    sceneSignalF = new QGraphicsScene(ui->drawSignalF->x(),ui->drawSignalF->y(),ui->drawSignalF->width(),ui->drawSignalF->height(),this);
    //set scene to graphicsScene
    ui->drawSignalA->setScene(sceneSignalA);
    ui->drawSignalB->setScene(sceneSignalB);
    ui->drawSignalC->setScene(sceneSignalC);
    ui->drawSignalD->setScene(sceneSignalD);
    ui->drawSignalE->setScene(sceneSignalE);
    ui->drawSignalF->setScene(sceneSignalF);
}


void signalviewer::updateTag(QVector<QString> *Textqvectp)
{
    int i;

    ui->comboBoxTAG->clear();
    for( i=0 ; i<Textqvectp->count();i++)
    {
        ui->comboBoxTAG->addItem(Textqvectp->at(i));
        qApp->processEvents();
    }
}


void signalviewer::draw(QGraphicsView *view, QGraphicsScene *scene, int max, int min,QVector <qint16> *database,float gainvalue,int index_begin,int *changablemiddle,int samplerate)
{
    //parameter for drawing
    float   begin;
    float everysecondincrease,everywidthincrease;
    float scalerate;
    float nowvaluey,beforevaluey,nowvaluex,beforevaluex;
    float nextvaluey;
    float signalmiddle;
    bool  seperatestart,notenough;
    int i, reduction,j; //reduction factor to display fewer data
    QPainterPath qp;

    //decide where to begin to draw
    begin=view->x();
    //decide the gap between every points(x-axis)
    everywidthincrease=((float)view->width())/(float)show_num_of_seconds;
    everysecondincrease=everywidthincrease/(float)samplerate;
    if(everysecondincrease<0.25) reduction=(0.25/everysecondincrease); //max 4 values per pixel
    else reduction=1;
    if(samplerate>=512 && reduction>512) reduction=512;
    else if(samplerate>=512 && reduction>256) reduction=256;
    else if(samplerate>=512 && reduction>128) reduction=128;
    else if(samplerate>=512 && reduction>64) reduction=64;
    else if(samplerate>=32 && reduction>32) reduction=32;
    else if(samplerate>=32 && reduction>16) reduction=16;
    else if(samplerate>=8 && reduction>8) reduction=8;
    else if(samplerate>=8 && reduction>4) reduction=4;
    else if(samplerate>=8 && reduction>2) reduction=2;
    else reduction=1;
    everysecondincrease=everysecondincrease*reduction; //adjust by reduction factor

    //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    scalerate=((float)(view->height()))/(float)(max-min); //ratio to max-min,-1 to 1
    //decide the middle value of signal
    signalmiddle=((float)max+(float)min)/2;

    if(index_begin<database->count())
        beforevaluey=(float)database->at(index_begin)*gainvalue;
    else
        return ;
    beforevaluey=(*changablemiddle)-(beforevaluey-signalmiddle)*scalerate;
    beforevaluex=begin;
    notenough=false;
    scene->clear();

    for(i=0;i<show_num_of_seconds;i++)//how many seconds
    {
        if(index_begin+samplerate*i<database->count())
            nowvaluey=(float)database->at(index_begin+samplerate*i)*gainvalue;
        else
            break;
        nowvaluey=(*changablemiddle)-(nowvaluey-signalmiddle)*scalerate;
        if(index_begin+samplerate*i+1<database->count())
            nextvaluey=(float)database->at(index_begin+samplerate*i+1)*gainvalue;
        else
            break;
        nextvaluey=(*changablemiddle)-(nextvaluey-signalmiddle)*scalerate;

        nowvaluex=begin;
        if(nowvaluex>finalshowvaluex)
            goto FINALDRAW;
        seperatestart=false;
        for(j=0;j<samplerate/reduction;j++) //how many sample per second adj. for reduction
        {
            if(seperatestart)
            {
                if(index_begin+samplerate*i+j*reduction<database->count()) //reduction
                    nowvaluey=(float)database->at(index_begin+samplerate*i+j*reduction)*gainvalue; //reduction
                else
                    goto FINALDRAW;
                nowvaluey=(*changablemiddle)-(nowvaluey-signalmiddle)*scalerate;
                if(index_begin+samplerate*i+j*reduction+1<database->count()) //reduction
                    nextvaluey=(float)database->at(index_begin+samplerate*i+j*reduction+1)*gainvalue; //reduction
                nextvaluey=(*changablemiddle)-(nextvaluey-signalmiddle)*scalerate;
                nowvaluex=nowvaluex+everysecondincrease; //reduction
                if(nowvaluex>finalshowvaluex)
                    goto FINALDRAW;
            }//if end
            seperatestart=true;
            qp.moveTo(beforevaluex,beforevaluey);
            qp.lineTo(nowvaluex,nowvaluey);
            beforevaluey=nowvaluey;
            beforevaluex=nowvaluex;
        }//samples end for end
        begin=view->x()+everywidthincrease*(i+1);
    }//seconds end
FINALDRAW:
    if(index_begin+samplerate*show_num_of_seconds<database->count())
    {
        nowvaluey=(float)database->at(index_begin+samplerate*show_num_of_seconds)*gainvalue;
        nowvaluey=(*changablemiddle)-(nowvaluey-signalmiddle)*scalerate;
        qp.moveTo(beforevaluex,beforevaluey);
        qp.lineTo(nowvaluex,nowvaluey);
    }
    else
    {
        qp.moveTo(beforevaluex,beforevaluey);
        qp.lineTo(nowvaluex,nowvaluey);
    }
    scene->addPath(qp,FWSIGNALCOLOR);
}


void signalviewer::checktimeandline(QGraphicsView *view, int index_begin, int samplerate)
{
    //parameter for drawing
    int   begin;
    float everysecondincrease,everywidthincrease;
    float nowvaluex,beforevaluex;
    bool  seperatestart,notenough;
    int i,j;

    //decide where to begin to draw
    begin=view->x();
    //decide the gap between every points(x-axis)
    everywidthincrease=((float)view->width())/(float)show_num_of_seconds;
    everysecondincrease=everywidthincrease/(float)samplerate;

    if(everysecondincrease<1)
        everysecondincrease=1;

    beforevaluex=begin;
    notenough=false;
    for(i=0;i<show_num_of_seconds;i++)//how many seconds
    {
        nowvaluex=begin;
        seperatestart=false;
        for(j=0;j<samplerate;j++) //how many sample per second
        {
            if(seperatestart)
                nowvaluex=beforevaluex+everysecondincrease;
            if((index_begin+samplerate*i+j)<mytsQV.count())
            {
                finalshowvaluex=nowvaluex;
            }

            if(nowvaluex>(ui->line->x())-everysecondincrease/2)
            {
                if(nowvaluex<(ui->line->x())+everysecondincrease/2)
                {
                    if((index_begin+samplerate*i+j)<mytsQV.count())
                        middlevaluefortime=mytsQV.at(index_begin+samplerate*i+j);
                    ui->line->setGeometry(nowvaluex,ui->line->y(),ui->line->width(),ui->line->height());
                    QDateTime firstFrmDT;
                    // start and end DateTime settings
                    // put in default values for start and end time
                    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((middlevaluefortime)*1000/UCRTCFREQ);
                    // align to full second
                    //firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
                    ui->middleDTE->setDateTime(firstFrmDT);
                    if((index_begin+samplerate*i+j)>=mytsQV.count())
                        return;
                }
            }
            seperatestart=true;
            beforevaluex=nowvaluex;
        }//samples end for end
        begin=view->x()+everywidthincrease*(i+1);
    }//seconds end
    if((index_begin+samplerate*show_num_of_seconds)<=mytsQV.count())
    {
        finalshowvaluex=begin;
    }
}


void signalviewer::showTag(QGraphicsView *view, QGraphicsScene *scene, QVector<qint64> *database, QVector<QString> *textbase, int index_begin, int samplerate)
{
    //parameter for drawing
    int   begin;
    float everysecondincrease,everywidthincrease;
    float nowvaluex,beforevaluex;
    bool  seperatestart;
    int i=0,j=0;
    int tagcount=0;
    int altrady=0; // to shift the text down

    ui->pushButton_TP->setEnabled(true);
    ui->TagText->setEnabled(true);
    ui->TagText_2->setEnabled(true);

    if(highlightitemindex.count()==0)
        ui->pushButton_TM->setEnabled(false);
    else
        ui->pushButton_TM->setEnabled(true);
    highlightitemindex.clear();
    begin=view->x();
    taglocation.clear();
    linebuffer.clear();
    stringbuffer.clear();
    tagindexbuffer.clear();
    //decide the gap between every points(x-axis)
    everywidthincrease=((float)view->width())/(float)show_num_of_seconds;
    everysecondincrease=everywidthincrease/(float)samplerate;
    beforevaluex=begin;
    scene->clear();
    tagplusindex = 1;
    //check the first time
    if(index_begin>mytsQV.count())
        return;
    while(database->at(tagcount)<mytsQV.at(index_begin))
    {
        tagcount++;
        if(tagcount>=database->count())
            return;
    }
    //check the following time
    for(i=0;i<show_num_of_seconds;i++)//how many seconds
    {
        nowvaluex=begin;
        beforevaluex=nowvaluex;
        seperatestart=false;
        for( j=0;j<samplerate;j++) //how many sample per second
        {
            if(seperatestart)
            {
                nowvaluex=beforevaluex+everysecondincrease;
                beforevaluex=nowvaluex;
            }//if seperatestart end
            if((index_begin+samplerate*i+j<mytsQV.count())&&(tagcount<database->count()))
            {
                altrady=0;
CHECKAGAIN:
                if(abs(database->at(tagcount)-mytsQV.at(index_begin+samplerate*i+j))<=SHOWTAGTHRESHOLD)
                {
                    if(textbase->at(tagcount)!=STARTLINETEXT&&textbase->at(tagcount)!=ENDLINETEXT)
                        linebuffer.append(scene->addLine(nowvaluex,view->y(),nowvaluex,view->y()+view->height(),FWSIGNALCOLOR));
                    else if(textbase->at(tagcount)==STARTLINETEXT)
                        linebuffer.append(scene->addLine(nowvaluex,view->y(),nowvaluex,view->y()+view->height(),FWSTARTCOLOR));
                    else if(textbase->at(tagcount)==ENDLINETEXT)
                        linebuffer.append(scene->addLine(nowvaluex,view->y(),nowvaluex,view->y()+view->height(),FWENDCOLOR));
                    taglocation.append(nowvaluex);
                    stringbuffer.append(scene->addText(textbase->at(tagcount), QFont("Arial", 8)));
                    stringbuffer.at(stringbuffer.count()-1)->setPos(nowvaluex,view->y()+altrady);
                    tagindexbuffer.append(tagcount);
                    tagcount++;
                    if(tagcount>=database->count())
                        return ;
                    else
                    {
                        altrady+=15;
                        goto CHECKAGAIN;
                    }
                }
            }//if seperatestart end
            else
                return ;
            seperatestart=true;
        }//samples end for end
        begin=view->x()+everywidthincrease*(i+1);
    }//seconds end
}


void signalviewer::reset()
{
//    qDebug()<<"reset";
    qApp->processEvents();
    oldSignalAindex=0; //M2
    reversechecked=false;
    show_num_of_seconds=600; //600" initial setting
    //save the increase pixels to move the scene
    mouseincreasex=0;
    mouseincreasey=0;  //initial 0
    //save the mouse pos
    pressx=0;
    pressy=0;
    qApp->processEvents();
//    qDebug()<<"reset A";
    releasex=0;
    releasey=0;  //initial 0
    tagflag = false;
    updateTag(Textqvect);
    tagflag = true;
    SpecFFT = NULL;
    //initial unit gain
    //from here
//    qDebug()<<"reset B";
    qApp->processEvents();
    ui->timemorelabel_now->setText("0s");
    if(Filetype<5)
    {
        TSSTEP=ADCLEN*UCRTCFREQ/TSRTCFREQ;
        SHOWTAGTHRESHOLD=TSSTEP/2;
        ui->pushButton_spectrogram->show();
        total_num_of_points=accxqvect.count();   //8 Hz
        ui->timemorelabel_total->setText(QString("%1s").arg(accxqvect.count()/FWSAMPLERATEACCX));
        //while(ui->TimeSlider==NULL)
        //    qApp->processEvents();
        // qDebug()<<"reset B1";
        //while(!ui->TimeSlider->isEnabled())
        //    qApp->processEvents();
        // qDebug()<<"reset B2";
        //if(accxqvect.count()/8>FWMAXSECONDINONESCENE)
        //    ui->TimeSlider->setMaximum(FWMAXSECONDINONESCENE);//
        //else
        //    ui->TimeSlider->setMaximum(accxqvect.count()/8-1);//
    }
    else if(Filetype<7)
    {
        TSSTEP=UCRTCFREQ/ECGSAMPLERATE;
        SHOWTAGTHRESHOLD=TSSTEP/2;
        ui->pushButton_spectrogram->hide();
        total_num_of_points=mytsQV.count();      //32Hz
        ui->timemorelabel_total->setText(QString("%1s").arg(mytsQV.count()/FWSAMPLERATERR));
        //while(ui->TimeSlider==NULL)
        //    qApp->processEvents();
        // qDebug()<<"reset B3";
        //while(!ui->TimeSlider->isEnabled())
        //    qApp->processEvents();
        // qDebug()<<"reset B4";
        //if(accxqvect.count()/32>FWECGMAXSECONDINONESCENE)
        //    ui->TimeSlider->setMaximum(FWECGMAXSECONDINONESCENE);//
        //else
        //    ui->TimeSlider->setMaximum(accxqvect.count()/32-1);//
    }
    else if(Filetype<9)
    {
        TSSTEP=UCRTCFREQ/EEGSAMPLERATE;
        SHOWTAGTHRESHOLD=TSSTEP/2;
        ui->pushButton_spectrogram->hide();
        total_num_of_points=M2qvect.count();     //8Hz
        ui->timemorelabel_total->setText(QString("%1s").arg(M2qvect.count()/FWSAMPLERATEM2));
        //while(ui->TimeSlider==NULL)
        //    qApp->processEvents();
        // qDebug()<<"reset B5";
        //while(!ui->TimeSlider->isEnabled())
        //    qApp->processEvents();
        // qDebug()<<"reset B6";
        //if(M2qvect.count()/8>FWEEGMAXSECONDINONESCENE)
        //    ui->TimeSlider->setMaximum(FWEEGMAXSECONDINONESCENE);//
        //else
        //    ui->TimeSlider->setMaximum(M2qvect.count()/8-1);//
    }
//    qDebug()<<"reset C";
    qApp->processEvents();
    //ui->TimeSlider->setValue(show_num_of_seconds);   default 86400 by changing the value in signalviewer.ui
    ui->adczoom->setValue(0); //assume when value equals middle value of the y_gain_bar then the y_gain_bar has unit gain
    ui->otherzoom->setValue(0);
    zoom_in_step=(float)FWMAX_SCALE/(float)ui->adczoom->maximum();
    zoom_out_step=(1-(float)FWMIN_SCALE)/(float)ui->adczoom->minimum();
    other_zoom_in_step=(float)FWOTHER_MAX_SCALE/(float)ui->otherzoom->maximum();
    other_zoom_out_step=(1-(float)FWOTHER_MIN_SCALE)/(float)ui->otherzoom->minimum();
    adcgainvalue= 1;  // set initial vertical zoom rate 1
    othergainvalue=1;
    getmouseflag=-1;
    qApp->processEvents();
//    qDebug()<<"reset D";
    changablemiddleA=ui->drawSignalA->y()+ui->drawSignalA->height()/2;
    changablemiddleB=ui->drawSignalB->y()+ui->drawSignalB->height()/2;
    changablemiddleC=ui->drawSignalC->y()+ui->drawSignalC->height()/2;
    changablemiddleD=ui->drawSignalD->y()+ui->drawSignalD->height()/2;
    changablemiddleE=ui->drawSignalE->y()+ui->drawSignalE->height()/2;
    qApp->processEvents();
//    qDebug()<<"reset E";
    ui->timemorelabel->setText(QString("%1s").arg(show_num_of_seconds));   //max
    qApp->processEvents();
//    qDebug()<<"reset F";
    //ui->ShiftSlider->setMinimum(0);
    qApp->processEvents();
//    qDebug()<<"reset G";
    ui->ShiftSlider->setMaximum(total_num_of_points);
    qApp->processEvents();
//    qDebug()<<"reset H";
    signal_index_begin=0;
    //ui->ShiftSlider->setValue(signal_index_begin);
//    qDebug()<<"reset I";
}


signalviewer::~signalviewer()
{
    delete ui;
}


void signalviewer::closeEvent(QCloseEvent *)
{
    qApp->processEvents();
    this->deleteLater();
}


void signalviewer::on_ShiftSlider_valueChanged(int value)
{
    signal_index_begin=value;

    //decide the index begin
    if(signal_index_begin<0)
        signal_index_begin=0;
    if(signal_index_begin>=total_num_of_points)
        signal_index_begin=total_num_of_points-FWSAMPLERATEACCX*1.5; //?


    if(Filetype<5)
    {
        ui->timemorelabel_now->setText(QString("%1s").arg(signal_index_begin/FWSAMPLERATEACCX));
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        //adc
        draw(ui->drawSignalA,sceneSignalA,FWADCDIGITALMAX,-1*FWADCDIGITALMAX,&adcqvect,adcgainvalue,signal_index_begin*64,&changablemiddleA,FWSAMPLERATEADC);
        //accx
        draw(ui->drawSignalB,sceneSignalB,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&accxqvect,othergainvalue,signal_index_begin,&changablemiddleB,FWSAMPLERATEACCX);
        //accy
        draw(ui->drawSignalC,sceneSignalC,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&accyqvect,othergainvalue,signal_index_begin,&changablemiddleC,FWSAMPLERATEACCY);
        //accz
        draw(ui->drawSignalD,sceneSignalD,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&acczqvect,othergainvalue,signal_index_begin*4,&changablemiddleD,FWSAMPLERATEACCZ);
        //temp
        draw(ui->drawSignalE,sceneSignalE,FWTMPDIGITALMAX,FWTMPDIGITALMIN,&tempqvect,othergainvalue,signal_index_begin,&changablemiddleE,FWSAMPLERATETEMP);
        //tag
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    }
    else if (Filetype<7)
    {
        ui->timemorelabel_now->setText(QString("%1s").arg(signal_index_begin/FWSAMPLERATERR));
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        //RR
        draw(ui->drawSignalA,sceneSignalA,FWECGDIGITALMAX,FWECGDIGITALMIN,&adcqvect,adcgainvalue,signal_index_begin,&changablemiddleA,FWSAMPLERATERR);
        //amp
        draw(ui->drawSignalB,sceneSignalB,FWECGDIGITALMAX,FWECGDIGITALMIN,&accxqvect,othergainvalue,signal_index_begin,&changablemiddleB,FWSAMPLERATEAMP);
        //LFper=SDNN
        draw(ui->drawSignalC,sceneSignalC,FWECGDIGITALMAX,0,&accyqvect,othergainvalue,signal_index_begin,&changablemiddleC,FWSAMPLERATELFPER);
        //HFper=HFpower log -4, 4
        draw(ui->drawSignalD,sceneSignalD,FWACCDIGITALMAX,0,&acczqvect,othergainvalue,signal_index_begin,&changablemiddleD,FWSAMPLERATEHFPER);
        //LHRatio=log -2, 2
        draw(ui->drawSignalE,sceneSignalE,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&tempqvect,othergainvalue,signal_index_begin,&changablemiddleE,FWSAMPLERATELHRATIO);
        //tag
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    }
    else if(Filetype <9)
    {
        ui->timemorelabel_now->setText(QString("%1s").arg(signal_index_begin/FWSAMPLERATEACCX));
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        switchdraw(ui->drawSignalA,sceneSignalA,adcgainvalue,signal_index_begin,&changablemiddleA,EEGSAMPLERATE,ui->comboBoxSignalA->currentIndex());
        switchdraw(ui->drawSignalB,sceneSignalB,othergainvalue,signal_index_begin,&changablemiddleB,EEGSAMPLERATE,ui->comboBoxSignalB->currentIndex());
        switchdraw(ui->drawSignalC,sceneSignalC,othergainvalue,signal_index_begin,&changablemiddleC,EEGSAMPLERATE,ui->comboBoxSignalC->currentIndex());
        switchdraw(ui->drawSignalD,sceneSignalD,othergainvalue,signal_index_begin,&changablemiddleD,EEGSAMPLERATE,ui->comboBoxSignalD->currentIndex());
        switchdraw(ui->drawSignalE,sceneSignalE,othergainvalue,signal_index_begin,&changablemiddleE,EEGSAMPLERATE,ui->comboBoxSignalE->currentIndex());
        //tag
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    }
    if(ui->line->x()>=finalshowvaluex)
    {
        ui->line->setGeometry(finalshowvaluex,ui->line->y(),ui->line->width(),ui->line->height());
        if(Filetype<5)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        else if(Filetype<7)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        else if(Filetype<9)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    }
}


void signalviewer::switchdraw(QGraphicsView *view, QGraphicsScene *scene,
                              float gainvalue, int index_begin, int *changeablemiddle, int samplerate,int index)
{
    switch(index)
    {
    case M2INDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&M2qvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case M1INDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&M1qvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case G2INDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&G2qvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case G1INDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&G1qvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case UPINDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&UPqvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case BETAINDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&Betaqvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case SIGMAINDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&Sigmaqvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case ALPHAINDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&alphaqvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case THETAINDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&thetaqvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case DELTAINDEX:
        draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&deltaqvect,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    case MIXINDEX:
        if(MixSignal.count()!=0)
            draw(view,scene,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&MixSignal,gainvalue,index_begin,changeablemiddle,samplerate);
        break;
    default:
        break;
    }
}


void signalviewer::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
    {
        if(e->x()>=ui->drawSignalA->x())
        {
            if(e->x()<=(ui->drawSignalA->x()+ui->drawSignalA->width()))
            {
                if(e->y()>=ui->drawSignalA->y())
                {
                    if(e->y()<=(ui->drawSignalE->y()+ui->drawSignalE->height()))  //line
                    {
                        if((e->x()<ui->line->x()+20)&&(e->x()>ui->line->x()-20))
                        {
                            getmouseflag=-2;
                            QCursor cursor(Qt::ClosedHandCursor);
                            this->setCursor(cursor);
                            this->grabMouse();
                            return;}
                    }

                    if(e->y()<=(ui->drawSignalA->y()+ui->drawSignalA->height()))  //A
                    {
                        pressx=e->x();
                        pressy=e->y();
                        QCursor cursor(Qt::ClosedHandCursor);
                        this->setCursor(cursor);
                        this->grabMouse();
                        getmouseflag=1;
                    }//<=y width end
                    else if(e->y()<=(ui->drawSignalB->y()+ui->drawSignalB->height()))  //B
                    {
                        pressx=e->x();
                        pressy=e->y();
                        QCursor cursor(Qt::ClosedHandCursor);
                        this->setCursor(cursor);
                        this->grabMouse();
                        getmouseflag=2;
                    }//<=y width end
                    else if(e->y()<=(ui->drawSignalC->y()+ui->drawSignalC->height()))  //C
                    {
                        pressx=e->x();
                        pressy=e->y();
                        QCursor cursor(Qt::ClosedHandCursor);
                        this->setCursor(cursor);
                        this->grabMouse();
                        getmouseflag=3;
                    }//<=y width end
                    else if(e->y()<=(ui->drawSignalD->y()+ui->drawSignalD->height()))  //D
                    {
                        pressx=e->x();
                        pressy=e->y();
                        QCursor cursor(Qt::ClosedHandCursor);
                        this->setCursor(cursor);
                        this->grabMouse();
                        getmouseflag=4;
                    }//<=y width end
                    else if(e->y()<=(ui->drawSignalE->y()+ui->drawSignalE->height()))  //E
                    {
                        pressx=e->x();
                        pressy=e->y();
                        QCursor cursor(Qt::ClosedHandCursor);
                        this->setCursor(cursor);
                        this->grabMouse();
                        getmouseflag=5;
                    }//<=y width end
                    else if(e->y()<=(ui->drawSignalF->y()+ui->drawSignalF->height()))  //F
                    {
                        pressx=e->x();
                        pressy=e->y();
                        QCursor cursor(Qt::ClosedHandCursor);
                        this->setCursor(cursor);
                        this->grabMouse();
                        getmouseflag=6;
                    }//<=y width end
                }//>=y end
            }//<=xwidth end
        }//>=x end
    }//left end
}


void signalviewer::mouseMoveEvent(QMouseEvent *m)
{
    if(getmouseflag==-2)
    {
        if((m->x()>ui->drawSignalA->x())&&(m->x()<ui->drawSignalA->x()+ui->drawSignalA->width())&&(m->x()<=finalshowvaluex))
        {
            ui->line->setGeometry(m->x(),ui->line->y(),ui->line->width(),ui->line->height());
            if(Filetype<5)
                checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
            else if(Filetype<7)
                checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
            else if(Filetype<9)
                checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        else if(m->x()>=finalshowvaluex)
        {
            ui->line->setGeometry(finalshowvaluex,ui->line->y(),ui->line->width(),ui->line->height());
            if(Filetype<5)
                checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
            else if(Filetype<7)
                checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
            else if(Filetype<9)
                checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
    }
}


void signalviewer::mouseReleaseEvent(QMouseEvent *r)
{
    QCursor cursor(Qt::ArrowCursor);
    int i=0;

    this->setCursor(cursor);
    switch(getmouseflag)
    {
    case 1: //A
        releasex=r->x();
        releasey=r->y();
        mouseincreasex=pressx-releasex;
        mouseincreasey=pressy-releasey;
        if((mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()>=MOUSEINDEXMOVETHRESHOLE)||(mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()<=-1*MOUSEINDEXMOVETHRESHOLE))
        {
            signal_index_begin=signal_index_begin+mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value();
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        if((mouseincreasey*MOUSEMIDDLEACCURACYRATIO>=MOUSEMIDDLEMOVETHRESHOLE)||(mouseincreasey*MOUSEMIDDLEACCURACYRATIO<=-1*MOUSEMIDDLEMOVETHRESHOLE))
        {
            changablemiddleA-=(mouseincreasey*MOUSEMIDDLEACCURACYRATIO);
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        this->releaseMouse();
        getmouseflag=-1;
        ui->pushButton_TP->setEnabled(true);
        ui->pushButton_TM->setEnabled(false);
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        ui->TagText->setText("");
        ui->TagText_2->setText("");
        break;
    case 2: //B
        releasex=r->x();
        releasey=r->y();
        mouseincreasex=pressx-releasex;
        mouseincreasey=pressy-releasey;
        if((mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()>=MOUSEINDEXMOVETHRESHOLE)||(mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()<=-1*MOUSEINDEXMOVETHRESHOLE))
        {
            signal_index_begin=signal_index_begin+mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value();
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        if((mouseincreasey*MOUSEMIDDLEACCURACYRATIO>=MOUSEMIDDLEMOVETHRESHOLE)||(mouseincreasey*MOUSEMIDDLEACCURACYRATIO<=-1*MOUSEMIDDLEMOVETHRESHOLE))
        {
            changablemiddleB-=(mouseincreasey*MOUSEMIDDLEACCURACYRATIO);
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        this->releaseMouse();
        getmouseflag=-1;
        ui->pushButton_TP->setEnabled(true);
        ui->pushButton_TM->setEnabled(false);
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        ui->TagText->setText("");
        ui->TagText_2->setText("");
        break;
    case 3: //C
        releasex=r->x();
        releasey=r->y();
        mouseincreasex=pressx-releasex;
        mouseincreasey=pressy-releasey;
        if((mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()>=MOUSEINDEXMOVETHRESHOLE)||(mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()<=-1*MOUSEINDEXMOVETHRESHOLE))
        {
            signal_index_begin=signal_index_begin+mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value();
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        if((mouseincreasey*MOUSEMIDDLEACCURACYRATIO>=MOUSEMIDDLEMOVETHRESHOLE)||(mouseincreasey*MOUSEMIDDLEACCURACYRATIO<=-1*MOUSEMIDDLEMOVETHRESHOLE))
        {
            changablemiddleC-=(mouseincreasey*MOUSEMIDDLEACCURACYRATIO);
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        this->releaseMouse();
        getmouseflag=-1;
        ui->pushButton_TP->setEnabled(true);
        ui->pushButton_TM->setEnabled(false);
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        ui->TagText->setText("");
        ui->TagText_2->setText("");
        break;
    case 4: //D
        releasex=r->x();
        releasey=r->y();
        mouseincreasex=pressx-releasex;
        mouseincreasey=pressy-releasey;
        if((mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()>=MOUSEINDEXMOVETHRESHOLE)||(mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()<=-1*MOUSEINDEXMOVETHRESHOLE))
        {
            signal_index_begin=signal_index_begin+mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value();
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        if((mouseincreasey*MOUSEMIDDLEACCURACYRATIO>=MOUSEMIDDLEMOVETHRESHOLE)||(mouseincreasey*MOUSEMIDDLEACCURACYRATIO<=-1*MOUSEMIDDLEMOVETHRESHOLE))
        {
            changablemiddleD-=(mouseincreasey*MOUSEMIDDLEACCURACYRATIO);
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        this->releaseMouse();
        getmouseflag=-1;
        ui->pushButton_TP->setEnabled(true);
        ui->pushButton_TM->setEnabled(false);
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        ui->TagText->setText("");
        ui->TagText_2->setText("");
        break;
    case 5: //E
        releasex=r->x();
        releasey=r->y();
        mouseincreasex=pressx-releasex;
        mouseincreasey=pressy-releasey;
        if((mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()>=MOUSEINDEXMOVETHRESHOLE)||(mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()<=-1*MOUSEINDEXMOVETHRESHOLE))
        {
            signal_index_begin=signal_index_begin+mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value();
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        if((mouseincreasey*MOUSEMIDDLEACCURACYRATIO>=MOUSEMIDDLEMOVETHRESHOLE)||(mouseincreasey*MOUSEMIDDLEACCURACYRATIO<=-1*MOUSEMIDDLEMOVETHRESHOLE))
        {
            changablemiddleE-=(mouseincreasey*MOUSEMIDDLEACCURACYRATIO);
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        this->releaseMouse();
        getmouseflag=-1;
        ui->pushButton_TP->setEnabled(true);
        ui->pushButton_TM->setEnabled(false);
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        }
        ui->TagText->setText("");
        ui->TagText_2->setText("");
        break;
    case 6: //F
        releasex=r->x();
        releasey=r->y();
        mouseincreasex=pressx-releasex;
        mouseincreasey=pressy-releasey;
        if((mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()>=MOUSEINDEXMOVETHRESHOLE)||(mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value()<=-1*MOUSEINDEXMOVETHRESHOLE))
        {
            signal_index_begin=signal_index_begin+mouseincreasex*MOUSEINDEXACCURACYRATIO*ui->TimeSlider->value();
            ui->ShiftSlider->setValue(signal_index_begin);
            this->on_ShiftSlider_valueChanged(signal_index_begin);
        }
        //tag
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        this->releaseMouse();
        getmouseflag=-1;
        highlightitemindex.clear();
        ui->TagText->setText("");
        ui->TagText_2->setText("");
        if(highlightitemindex.count()==0)
            ui->pushButton_TM->setEnabled(false);
        else
            ui->pushButton_TM->setEnabled(true);
        for( i=0;i<taglocation.count();i++)
        {
            if((releasex-taglocation.at(i)<=5)&&(releasex-taglocation.at(i)>=-5))
            {
                if(linebuffer.at(i)->pen()==FWSIGNALCOLOR)
                {
                    linebuffer.at(i)->setPen(FWHIGHLIGHTCOLOR);
                    highlightitemindex.append(i);
                    if(highlightitemindex.count()==1)
                    {
                        ui->TagText->setText(stringbuffer.at(i)->toPlainText());
                        ui->TagText->setEnabled(true);
                        ui->TagText_2->setEnabled(false);
                    }
                    else
                    {
                        ui->TagText_2->setText(stringbuffer.at(i)->toPlainText());
                        ui->TagText_2->setEnabled(true);
                    }
                    ui->pushButton_TP->setEnabled(false);
                    ui->pushButton_TM->setEnabled(true);
                    stringbuffer.at(i)->setHtml(QString("<font color=green>").append(stringbuffer.at(i)->toPlainText()).append("</font>"));
                }
                else if(stringbuffer.at(i)->toPlainText()!=STARTLINETEXT&&stringbuffer.at(i)->toPlainText()!=ENDLINETEXT)
                {
                    linebuffer.at(i)->setPen(FWSIGNALCOLOR);
                    highlightitemindex.clear();
                    ui->TagText->setEnabled(true);
                    ui->TagText_2->setEnabled(true);
                    ui->pushButton_TM->setEnabled(false);
                    stringbuffer.at(i)->setHtml(QString("<font color=black>").append(stringbuffer.at(i)->toPlainText()).append("</font>"));
                }
            }
        }
        break;
    case -2:
        this->releaseMouse();
        break;
    default:
        break;
    }
}


void signalviewer::on_pushButton_4_clicked()
{
    reset();
    QDateTime firstFrmDT, lastFrmDT;
    (*myfirstFrmTS) = originalfirst;
    (*mylastFrmTS) = originallast;
    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*myfirstFrmTS)*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*mylastFrmTS)*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);

    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);
    this->on_ShiftSlider_valueChanged(ui->ShiftSlider->value());
}


void signalviewer::on_StartTimeButtion_clicked()
{
    int i=0;

    if((*mylastFrmTS)<=middlevaluefortime)
        return;
    QDateTime firstFrmDT;
    (*myfirstFrmTS)=middlevaluefortime;
    // start and end DateTime settings
    // put in default values for start and end time
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*myfirstFrmTS)*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    (*myfirstFrmTS) = (firstFrmDT.toMSecsSinceEpoch()-QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000;
    ui->stDTE->setDateTime(firstFrmDT);
    for(i=0;i<TSqvect->count();i++)
    {
        if(Textqvect->at(i)==STARTLINETEXT)
        {
            TSqvect->remove(i);
            Textqvect->remove(i);
        }
    }

    if(TSqvect->count()==0)
        tagplusindex=0;
    else
    {
        tagplusindex=0;
        for(i=0;i<TSqvect->count();i++)
        {
            if((*myfirstFrmTS) > TSqvect->at(i))
                tagplusindex=i+1;
        }
    }
    TSqvect->insert(tagplusindex,(*myfirstFrmTS));
    Textqvect->insert(tagplusindex,STARTLINETEXT);
    //tag
    if(Filetype<5)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
    else if(Filetype<7)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
    }
    else if(Filetype<9)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
    tagflag = false;
    updateTag(Textqvect);
    tagflag = true;
}


void signalviewer::on_EndTimeButton_clicked()
{
    if((*myfirstFrmTS)>=middlevaluefortime)
        return;
    QDateTime lastFrmDT;
    int i=0;
    (*mylastFrmTS)=middlevaluefortime;
    (*mylastFrmTS) = (*myfirstFrmTS) + (((*mylastFrmTS) - (*myfirstFrmTS))/(8*UCRTCFREQ))*8*UCRTCFREQ;

    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs((*mylastFrmTS)*1000/UCRTCFREQ);

    // align to floor of full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch())/1000)*1000);
    (*mylastFrmTS) = (lastFrmDT.toMSecsSinceEpoch()-QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000;

    ui->endDTE->setDateTime(lastFrmDT);

    for( i=0;i<TSqvect->count();i++)
    {
        if(Textqvect->at(i)==ENDLINETEXT)
        {
            TSqvect->remove(i);
            Textqvect->remove(i);
        }
    }

    if(TSqvect->count()==0)
        tagplusindex=0;
    else
    {
        tagplusindex=0;
        for( i=0;i<TSqvect->count();i++)
        {
            if((*mylastFrmTS)>TSqvect->at(i))
                tagplusindex=i+1;
        }
    }
    TSqvect->insert(tagplusindex,(*mylastFrmTS));
    Textqvect->insert(tagplusindex,ENDLINETEXT);
    //tag
    if(Filetype<5)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
    else if(Filetype<7)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
    }
    else if(Filetype<9)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
    tagflag = false;
    updateTag(Textqvect);
    tagflag = true;
}


void signalviewer::on_pushButton_TP_clicked()
{
    int i;

    if(highlightitemindex.count()!=0)
        return;
    if(finalshowvaluex<ui->line->x())
        return ;
    if(middlevaluefortime>mytsQV.at(mytsQV.count()-1))
        return ;
    int tempcount=0;

    if(TSqvect->count()==0)
        tagplusindex=0;
    else
    {
        tagplusindex=0;
        for( i=0;i<TSqvect->count();i++)
        {
            qApp->processEvents();
            if(middlevaluefortime>TSqvect->at(i))
                tagplusindex=i+1;
            if(middlevaluefortime==TSqvect->at(i))
                tempcount++;
        }
    }
    qApp->processEvents();
    if((ui->TagText->toPlainText()=="")&&(ui->TagText_2->toPlainText()==""))
    {
        ui->TagText->setText("New Tag");
        TSqvect->insert(tagplusindex,middlevaluefortime);
        Textqvect->insert(tagplusindex,ui->TagText->toPlainText());
        //tag
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        tagflag = false;
        updateTag(Textqvect);
        tagflag = true;
    }
    else if(ui->TagText->toPlainText()=="")
    {
        TSqvect->insert(tagplusindex,middlevaluefortime);
        Textqvect->insert(tagplusindex,ui->TagText_2->toPlainText());
        ui->TagText->setText(ui->TagText_2->toPlainText());
        ui->TagText_2->setText("");
        //tag
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        tagflag = false;
        updateTag(Textqvect);
        tagflag = true;
    }
    else if(ui->TagText_2->toPlainText()=="")
    {
        TSqvect->insert(tagplusindex,middlevaluefortime);
        if(tempcount==1)
        {
            ui->TagText_2->setText(ui->TagText->toPlainText());
        }
        Textqvect->insert(tagplusindex,ui->TagText->toPlainText());
        //tag
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        tagflag = false;
        updateTag(Textqvect);
        tagflag = true;
    }
    else if((ui->TagText->toPlainText()!="")&&(ui->TagText_2->toPlainText()!=""))
    {
        if(tempcount==0)
        {
            TSqvect->insert(tagplusindex,middlevaluefortime);
            Textqvect->insert(tagplusindex,ui->TagText->toPlainText());

            TSqvect->insert(tagplusindex,middlevaluefortime);
            Textqvect->insert(tagplusindex,ui->TagText_2->toPlainText());}
        else if(tempcount==1)
        {
            TSqvect->insert(tagplusindex,middlevaluefortime);
            Textqvect->insert(tagplusindex,ui->TagText_2->toPlainText());
        }
        //tag
        if(Filetype<5)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        else if(Filetype<7)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
        }
        else if(Filetype<9)
        {
            if(TSqvect->count()!=0)
                showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
        }
        tagflag = false;
        updateTag(Textqvect);
        tagflag = true;
    }
}


void signalviewer::on_pushButton_TM_clicked()
{
    int tempindex=highlightitemindex.count();
    int i;

    ui->pushButton_TP->setEnabled(true);
    if(highlightitemindex.count()!=0)
    {
        for( i=tempindex-1;i>=0;i--)
        {
            if(linebuffer.at(highlightitemindex.at(i))->pen()==FWHIGHLIGHTCOLOR)
            {
                TSqvect->remove(tagindexbuffer.at(highlightitemindex.at(i)));
                Textqvect->remove(tagindexbuffer.at(highlightitemindex.at(i)));
            }
        }
    }

    highlightitemindex.clear();
    ui->TagText->clear();
    ui->TagText_2->clear();
    //tag
    if(Filetype<5)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
    else if(Filetype<7)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
    }
    else if(Filetype<9)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
    tagflag = false;
    updateTag(Textqvect);
    tagflag = true;
}


void signalviewer::on_TagText_textChanged()
{
    if(highlightitemindex.count()>0)
    {
        ui->TagText->setEnabled(true);
        if(linebuffer.at(highlightitemindex.at(0))->pen()==FWHIGHLIGHTCOLOR)
        {
            Textqvect->replace(tagindexbuffer.at(highlightitemindex.at(0)),ui->TagText->toPlainText());
            stringbuffer.at(highlightitemindex.at(0))->setHtml(QString("<font color=green>").append(ui->TagText->toPlainText()).append("</font>"));
            tagflag = false;
            updateTag(Textqvect);
            tagflag = true;
        }
    }
}


void signalviewer::on_TagText_2_textChanged()
{
    if(highlightitemindex.count()>1)
    {
        ui->TagText_2->setEnabled(true);
        if(linebuffer.at(highlightitemindex.at(1))->pen()==FWHIGHLIGHTCOLOR)
        {
            Textqvect->replace(tagindexbuffer.at(highlightitemindex.at(1)),ui->TagText_2->toPlainText());
            stringbuffer.at(highlightitemindex.at(1))->setHtml(QString("<font color=green>").append(ui->TagText_2->toPlainText()).append("</font>"));
            tagflag = false;
            updateTag(Textqvect);
            tagflag = true;
        }
    }
}


void signalviewer::on_comboBoxSignalA_currentIndexChanged(int index)
{
    if(index!=MIXINDEX)
        oldSignalAindex=index;

    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);

    if(Filetype==7||Filetype==8)
        switchdraw(ui->drawSignalA,sceneSignalA,adcgainvalue,signal_index_begin,&changablemiddleA,EEGSAMPLERATE,ui->comboBoxSignalA->currentIndex());
    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
}


void signalviewer::on_comboBoxSignalB_currentIndexChanged(int index)
{    
    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);

    if(Filetype==7||Filetype==8)
        switchdraw(ui->drawSignalB,sceneSignalB,othergainvalue,signal_index_begin,&changablemiddleB,EEGSAMPLERATE,ui->comboBoxSignalB->currentIndex());

    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
}


void signalviewer::on_comboBoxSignalC_currentIndexChanged(int index)
{
    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);

    if(Filetype==7||Filetype==8)
        switchdraw(ui->drawSignalC,sceneSignalC,othergainvalue,signal_index_begin,&changablemiddleC,EEGSAMPLERATE,ui->comboBoxSignalC->currentIndex());

    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
}


void signalviewer::on_comboBoxSignalD_currentIndexChanged(int index)
{
    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);

    if(Filetype==7||Filetype==8)
        switchdraw(ui->drawSignalD,sceneSignalD,othergainvalue,signal_index_begin,&changablemiddleD,EEGSAMPLERATE,ui->comboBoxSignalD->currentIndex());

    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
}


void signalviewer::on_comboBoxSignalE_currentIndexChanged(int index)
{
    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);

    if(Filetype==7||Filetype==8)
        switchdraw(ui->drawSignalE,sceneSignalE,othergainvalue,signal_index_begin,&changablemiddleE,EEGSAMPLERATE,ui->comboBoxSignalE->currentIndex());

    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
}


void signalviewer::on_comboBoxTAG_activated(int index)
{
    int new_begin_index;
    int i;

    if(tagflag)
    {
        for( i=-200;i<200;i++)
        {
            new_begin_index=mytsQV.indexOf(TSqvect->at(index)+i);
            if(new_begin_index!=-1)
                break;
        }
        if(new_begin_index==-1)
        {
            for( i=-800;i<-200;i++)
            {
                new_begin_index=mytsQV.indexOf(TSqvect->at(index)+i);
                if(new_begin_index!=-1)
                    break;
            }
        }
        if(new_begin_index==-1)
        {
            for( i=200;i<800;i++)
            {
                new_begin_index=mytsQV.indexOf(TSqvect->at(index)+i);
                if(new_begin_index!=-1)
                    break;
            }
        }
        //qDebug()<<new_begin_index<<" "<<TSqvect->at(index)-mytsQV.at(new_begin_index);
        if(Filetype <5)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        else if(Filetype <7)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        else if(Filetype <9)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);

        if(Filetype<5)
            signal_index_begin = new_begin_index - (mytsQV.indexOf(middlevaluefortime)-signal_index_begin);
        else if(Filetype<7)
            signal_index_begin = new_begin_index - (mytsQV.indexOf(middlevaluefortime)-signal_index_begin-1);
        else if(Filetype<9)
            signal_index_begin = new_begin_index - (mytsQV.indexOf(middlevaluefortime)-signal_index_begin);  //reverse

        if(signal_index_begin<0)
            signal_index_begin = 0;

        ui->ShiftSlider->setValue(signal_index_begin);

        this->on_ShiftSlider_valueChanged(signal_index_begin);
        if(Filetype <5)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        else if(Filetype <7)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        else if(Filetype <9)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    }
}


void signalviewer::on_TimeSlider_valueChanged(int value)
{
    show_num_of_seconds=ui->TimeSlider->value();
    ui->timemorelabel->setText(QString("%1s").arg(show_num_of_seconds));

    if(Filetype<5)
    {
        //adc
        draw(ui->drawSignalA,sceneSignalA,FWADCDIGITALMAX,-1*FWADCDIGITALMAX,&adcqvect,adcgainvalue,signal_index_begin*64,&changablemiddleA,FWSAMPLERATEADC);
        //accx
        draw(ui->drawSignalB,sceneSignalB,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&accxqvect,othergainvalue,signal_index_begin,&changablemiddleB,FWSAMPLERATEACCX);
        //accy
        draw(ui->drawSignalC,sceneSignalC,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&accyqvect,othergainvalue,signal_index_begin,&changablemiddleC,FWSAMPLERATEACCY);
        //accz
        draw(ui->drawSignalD,sceneSignalD,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&acczqvect,othergainvalue,signal_index_begin*4,&changablemiddleD,FWSAMPLERATEACCZ);
        //temp
        draw(ui->drawSignalE,sceneSignalE,FWTMPDIGITALMAX,FWTMPDIGITALMIN,&tempqvect,othergainvalue,signal_index_begin,&changablemiddleE,FWSAMPLERATETEMP);
    }
    else if(Filetype <7)
    {
        //RR
        draw(ui->drawSignalA,sceneSignalA,FWECGDIGITALMAX,FWECGDIGITALMIN,&adcqvect,adcgainvalue,signal_index_begin,&changablemiddleA,FWSAMPLERATERR);
        //amp
        draw(ui->drawSignalB,sceneSignalB,FWECGDIGITALMAX,FWECGDIGITALMIN,&accxqvect,othergainvalue,signal_index_begin,&changablemiddleB,FWSAMPLERATEAMP);
        //LFper=SDNN*10
        draw(ui->drawSignalC,sceneSignalC,FWECGDIGITALMAX,0,&accyqvect,othergainvalue,signal_index_begin,&changablemiddleC,FWSAMPLERATELFPER);
        //HFper=log power -4, 4
        draw(ui->drawSignalD,sceneSignalD,FWACCDIGITALMAX,0,&acczqvect,othergainvalue,signal_index_begin,&changablemiddleD,FWSAMPLERATEHFPER);
        //LHRatio=log -2, 2
        draw(ui->drawSignalE,sceneSignalE,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&tempqvect,othergainvalue,signal_index_begin,&changablemiddleE,FWSAMPLERATELHRATIO);

    }
    else if(Filetype <9)
    {
        switchdraw(ui->drawSignalA,sceneSignalA,adcgainvalue,signal_index_begin,&changablemiddleA,EEGSAMPLERATE,ui->comboBoxSignalA->currentIndex());
        switchdraw(ui->drawSignalB,sceneSignalB,othergainvalue,signal_index_begin,&changablemiddleB,EEGSAMPLERATE,ui->comboBoxSignalB->currentIndex());
        switchdraw(ui->drawSignalC,sceneSignalC,othergainvalue,signal_index_begin,&changablemiddleC,EEGSAMPLERATE,ui->comboBoxSignalC->currentIndex());
        switchdraw(ui->drawSignalD,sceneSignalD,othergainvalue,signal_index_begin,&changablemiddleD,EEGSAMPLERATE,ui->comboBoxSignalD->currentIndex());
        switchdraw(ui->drawSignalE,sceneSignalE,othergainvalue,signal_index_begin,&changablemiddleE,EEGSAMPLERATE,ui->comboBoxSignalE->currentIndex());
    }
    //tag
    if(Filetype <5)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }
    else if(Filetype <7)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATERR);
    }
    else if(Filetype <9)
    {
        if(TSqvect->count()!=0)
            showTag(ui->drawSignalF,sceneSignalF,TSqvect,Textqvect,signal_index_begin,FWSAMPLERATETAG);
    }

    if(ui->line->x()>=finalshowvaluex)
    {
        ui->line->setGeometry(finalshowvaluex,ui->line->y(),ui->line->width(),ui->line->height());
        if(Filetype<5)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
        else if(Filetype<7)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
        else if(Filetype<9)
            checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    }
}


void signalviewer::on_adczoom_actionTriggered(int action)
{
    if(ui->adczoom->value()>0)
        adcgainvalue=1+ui->adczoom->value()*zoom_in_step;
    else if(ui->adczoom->value()<0)
        adcgainvalue=1-ui->adczoom->value()*zoom_out_step;
    else
        adcgainvalue=1;

    //adc
    if(Filetype<5)
        draw(ui->drawSignalA,sceneSignalA,FWADCDIGITALMAX,-1*FWADCDIGITALMAX,&adcqvect,adcgainvalue,signal_index_begin*64,&changablemiddleA,FWSAMPLERATEADC);
    else if(Filetype <7)
        draw(ui->drawSignalA,sceneSignalA,FWECGDIGITALMAX,FWECGDIGITALMIN,&adcqvect,adcgainvalue,signal_index_begin,&changablemiddleA,FWSAMPLERATERR);
    else if(Filetype <9)
        switchdraw(ui->drawSignalA,sceneSignalA,adcgainvalue,signal_index_begin,&changablemiddleA,EEGSAMPLERATE,ui->comboBoxSignalA->currentIndex());

}


void signalviewer::on_otherzoom_actionTriggered(int action)
{
    if(ui->otherzoom->value()>0)
    {
        othergainvalue=1+ui->otherzoom->value()*other_zoom_in_step;
    }
    else if(ui->otherzoom->value()<0)
    {
        othergainvalue=1-ui->otherzoom->value()*other_zoom_out_step;
    }
    else
    {
        othergainvalue=1;
    }

    if(Filetype<5)
    {
        //accx
        draw(ui->drawSignalB,sceneSignalB,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&accxqvect,othergainvalue,signal_index_begin,&changablemiddleB,FWSAMPLERATEACCX);
        //accy
        draw(ui->drawSignalC,sceneSignalC,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&accyqvect,othergainvalue,signal_index_begin,&changablemiddleC,FWSAMPLERATEACCY);
        //accz
        draw(ui->drawSignalD,sceneSignalD,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&acczqvect,othergainvalue,signal_index_begin*4,&changablemiddleD,FWSAMPLERATEACCZ);
        //temp
        draw(ui->drawSignalE,sceneSignalE,FWTMPDIGITALMAX,FWTMPDIGITALMIN,&tempqvect,othergainvalue,signal_index_begin,&changablemiddleE,FWSAMPLERATETEMP);
    }
    else if(Filetype <7)
    {
        //RR
        draw(ui->drawSignalA,sceneSignalA,FWECGDIGITALMAX,FWECGDIGITALMIN,&adcqvect,adcgainvalue,signal_index_begin,&changablemiddleA,FWSAMPLERATERR);
        //amp
        draw(ui->drawSignalB,sceneSignalB,FWECGDIGITALMAX,FWECGDIGITALMIN,&accxqvect,othergainvalue,signal_index_begin,&changablemiddleB,FWSAMPLERATEAMP);
        //LFper=SDNN*10
        draw(ui->drawSignalC,sceneSignalC,FWECGDIGITALMAX,0,&accyqvect,othergainvalue,signal_index_begin,&changablemiddleC,FWSAMPLERATELFPER);
        //HFper=power log -4, 4
        draw(ui->drawSignalD,sceneSignalD,FWACCDIGITALMAX,0,&acczqvect,othergainvalue,signal_index_begin,&changablemiddleD,FWSAMPLERATEHFPER);
        //LHRatio=log -2, 2
        draw(ui->drawSignalE,sceneSignalE,FWACCDIGITALMAX,-1*FWACCDIGITALMAX,&tempqvect,othergainvalue,signal_index_begin,&changablemiddleE,FWSAMPLERATELHRATIO);
    }
    else if(Filetype <9)
    {
        switchdraw(ui->drawSignalB,sceneSignalB,othergainvalue,signal_index_begin,&changablemiddleB,EEGSAMPLERATE,ui->comboBoxSignalB->currentIndex());
        switchdraw(ui->drawSignalC,sceneSignalC,othergainvalue,signal_index_begin,&changablemiddleC,EEGSAMPLERATE,ui->comboBoxSignalC->currentIndex());
        switchdraw(ui->drawSignalD,sceneSignalD,othergainvalue,signal_index_begin,&changablemiddleD,EEGSAMPLERATE,ui->comboBoxSignalD->currentIndex());
        switchdraw(ui->drawSignalE,sceneSignalE,othergainvalue,signal_index_begin,&changablemiddleE,EEGSAMPLERATE,ui->comboBoxSignalE->currentIndex());
    }

}


void signalviewer::on_checkReverse_toggled(bool checked)
{
    int i;

    for( i=0;i<adcqvect.count();i++)
    {
        adcqvect[i]=adcqvect.at(i)*-1;
    }
    if(Filetype<5)
        draw(ui->drawSignalA,sceneSignalA,FWADCDIGITALMAX,-1*FWADCDIGITALMAX,&adcqvect,adcgainvalue,signal_index_begin*64,&changablemiddleA,FWSAMPLERATEADC);
    reversechecked=checked;
}


void signalviewer::on_TimeSlider_sliderReleased()
{
    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
}


void signalviewer::on_adczoom_sliderReleased()
{
    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
}


void signalviewer::on_otherzoom_sliderReleased()
{
    if(Filetype <5)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
    else if(Filetype <7)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATERR);
    else if(Filetype <9)
        checktimeandline(ui->drawSignalA,signal_index_begin,FWSAMPLERATEACCX);
}


void signalviewer::on_pushButton_2D_accel_clicked()
{
    TDV= new twoDaccelviewer(false,mypdn,this);
    if((signal_index_begin+show_num_of_seconds*FWSAMPLERATEACCX-1)<mytsQV.count())
        TDV->setData(&accxqvect,&accyqvect,&acczqvect,&mytsQV,(qint64)mytsQV.at(signal_index_begin),(qint64)mytsQV.at(signal_index_begin+show_num_of_seconds*FWSAMPLERATEACCX-1));
    else
        TDV->setData(&accxqvect,&accyqvect,&acczqvect,&mytsQV,(qint64)mytsQV.at(signal_index_begin),(qint64)mytsQV.at(mytsQV.count()-1));
    TDV->setVisible(true);
    TDV->setGeometry(this->x()+this->width()+25, this->y()+30, TDV->width(), TDV->height());
}


void signalviewer::on_comboBoxSignalA_activated(int index)
{
    mixsignaldialog *msd;
    if(index==MIXINDEX)
    {
        msd = new mixsignaldialog(&M2qvect,&M1qvect,
                                  &G2qvect,&G1qvect,
                                  &UPqvect,&Betaqvect,
                                  &Sigmaqvect,&alphaqvect,
                                  &thetaqvect,&deltaqvect,this);
        msd->show();
        if(msd->exec()!=QDialog::Accepted)
        {
            ui->comboBoxSignalA->setCurrentIndex(oldSignalAindex);
            msd->deleteLater();
            return ;
        }
        else
        {
            MixSignal.clear();
            MixSignal = msd->getMixSignal();
            if(MixSignal.count()!=0)
                draw(ui->drawSignalA,sceneSignalA,FWEEGDIGITALMAX,FWEEGDIGITALMIN,&MixSignal,adcgainvalue,signal_index_begin,&changablemiddleA,EEGSAMPLERATE);
            msd->deleteLater();
        }
    }
}


void signalviewer::on_pushButton_spectrogram_clicked()
{
    if(SpecFFT!=NULL)
    {
        if(!SpecFFT->isVisible())
        {
            SpecFFT = new Spectrogram(mypdn,this);
            if((signal_index_begin+show_num_of_seconds*FWSAMPLERATEACCX-1)<mytsQV.count())
                SpecFFT->setData(&adcqvect,&mytsQV,(qint64)mytsQV.at(signal_index_begin),(qint64)mytsQV.at(signal_index_begin+show_num_of_seconds*FWSAMPLERATEACCX-1));
            else
                SpecFFT->setData(&adcqvect,&mytsQV,(qint64)mytsQV.at(signal_index_begin),(qint64)mytsQV.at(mytsQV.count()-1));
            SpecFFT->setVisible(true);
        }
        else
        {
            if((signal_index_begin+show_num_of_seconds*FWSAMPLERATEACCX-1)<mytsQV.count())
                SpecFFT->setData(&adcqvect,&mytsQV,(qint64)mytsQV.at(signal_index_begin),(qint64)mytsQV.at(signal_index_begin+show_num_of_seconds*FWSAMPLERATEACCX-1));
            else
                SpecFFT->setData(&adcqvect,&mytsQV,(qint64)mytsQV.at(signal_index_begin),(qint64)mytsQV.at(mytsQV.count()-1));
        }
    }else{
        SpecFFT = new Spectrogram(mypdn,this);
        if((signal_index_begin+show_num_of_seconds*FWSAMPLERATEACCX-1)<mytsQV.count())
            SpecFFT->setData(&adcqvect,&mytsQV,(qint64)mytsQV.at(signal_index_begin),(qint64)mytsQV.at(signal_index_begin+show_num_of_seconds*FWSAMPLERATEACCX-1));
        else
            SpecFFT->setData(&adcqvect,&mytsQV,(qint64)mytsQV.at(signal_index_begin),(qint64)mytsQV.at(mytsQV.count()-1));
        SpecFFT->setVisible(true);
    }

}
