#include "twodaccelviewer.h"
#include "ui_twodaccelviewer.h"


QPen TDVredPen(Qt::red);

#define  TDVSIGNALCOLOR      TDVredPen


twoDaccelviewer::twoDaccelviewer(bool livedisplaymode, quint8 pdn, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::twoDaccelviewer)
{
    ui->setupUi(this);
    //Mainscene = 0;  // initialize to null
    metaAnalysisText = 0;
    activityText = 0;
    Mainscene = new QGraphicsScene(ui->graphicsView->x(),ui->graphicsView->y(),ui->graphicsView->width(),ui->graphicsView->height(),this);
    ui->graphicsView->setScene(Mainscene);
    livedisplaymodebool=livedisplaymode;
    zScene = new QGraphicsScene(ui->zGV->x(),ui->zGV->y(),ui->zGV->width(),ui->zGV->height(),this);
    ui->zGV->setScene(zScene);
    reset();
    mypdn = pdn;
    this->setWindowTitle(QString("Posture Viewer [%1]").arg((int)pdn));
}


twoDaccelviewer::~twoDaccelviewer()
{
    delete ui;
}


void twoDaccelviewer::reset()
{
    //if(Mainscene != 0)
    //{
    //    delete Mainscene;
    //    metaAnalysisText = 0;
     //   activityText = 0;
    //}
    //Mainscene = new QGraphicsScene(ui->graphicsView->x(),ui->graphicsView->y(),ui->graphicsView->width(),ui->graphicsView->height(),this);
    //ui->graphicsView->setScene(Mainscene);
    Mainscene->clear();
    zScene->clear();
    metaAnalysisText = 0;
    activityText = 0;

    this->setWindowTitle(QString("Posture Viewer [%1]").arg((int)mypdn));

    firstdraw=true;
    x_calm = 0;
    y_calm = 0;
    z_calm = 0;
    lowcount = 0;
    medcount = 0;
    highcount = 0;
    intensecount = 0;
    totalcount = 0;
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
    stepcount = 0;
    accxqvect.clear();
    accyqvect.clear();
    acczqvect.clear();
    acczqvectoriginal.clear();
    activityqvect.clear();
    activitylow.clear();
    totalcountsave.clear();
    stepcountsave.clear();
    activitymed.clear();
    activityhigh.clear();
    activityintense.clear();
    windowxscale = ((float)(70000))/((float)(ui->graphicsView->height())); //fixed range
    windowyscale = ((float)(50000))/((float)(ui->graphicsView->height())); //fixed range
}


void twoDaccelviewer::closeEvent(QCloseEvent *)
{
    if(!livedisplaymodebool)
    {
        QObject::killTimer(timerid);
        ui->genActTextPB->setEnabled(true);
    }
}


void twoDaccelviewer::setData(QVector<qint16> *accxQVp, QVector<qint16> *accyQVp,
                              QVector<qint16> *acczQVp, QVector<qint64> *mytsp,
                              qint64 firstts, qint64 lastts)
{
    int i;
    int j;
    float zzz;
    qint16 acttemp;
    qint16 z_new_data_average;

    reset();
    mytsQV.clear();
    accxqvect.clear();
    accyqvect.clear();
    acczqvect.clear();
    windowxscale = ((float)(70000))/((float)(ui->graphicsView->height())); //fixed range
    windowyscale = ((float)(50000))/((float)(ui->graphicsView->height())); //fixed range

    myfirstFrmTS=firstts;
    mylastFrmTS=lastts;
    firstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(myfirstFrmTS*1000/UCRTCFREQ);
    // align to full second
    firstFrmDT.setMSecsSinceEpoch(((firstFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    lastFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(mylastFrmTS*1000/UCRTCFREQ);
    // align to full second
    lastFrmDT.setMSecsSinceEpoch(((lastFrmDT.toMSecsSinceEpoch()+999)/1000)*1000);
    ui->stDTE->setDateTime(firstFrmDT);
    ui->endDTE->setDateTime(lastFrmDT);
    for(i =0 ; i<mytsp->size();i++)
    {
        if((mytsp->at(i)>=myfirstFrmTS)&&(mytsp->at(i)<=mylastFrmTS))
        {
            accxqvect.append(accxQVp->at(i));
            accyqvect.append(accyQVp->at(i));
            z_new_data_average=0;
            for(j=0;j<TDV_NUMPOINTSAMPLE_Z;j++)
            {
                if((i*TDV_NUMPOINTSAMPLE_Z+j)<acczQVp->size())
                {
                    zzz=acczQVp->at(i*TDV_NUMPOINTSAMPLE_Z+j);
                    acczqvectoriginal.append(acczQVp->at(i*TDV_NUMPOINTSAMPLE_Z+j));
                    if(zzz>32767) zzz-=32768;
                    z_new_data_average+=zzz/TDV_NUMPOINTSAMPLE_Z; //prevent overflow in qint16
                }
            }
            acczqvect.append(z_new_data_average);
            mytsQV.append(mytsp->at(i));
            acttemp = calcAct(&accxqvect, &accyqvect, &acczqvectoriginal, &newstep, &azold, &axold, &ayold, &az4old, &ax2old, &ay2old, &ax3old, &ay3old, &ax4old, &ay4old, &az8old, &az12old, &az16old);
            if(newstep>1) //update stepcount here
            {
                stepcount++; //add 1 step
                if(newstep>2) stepcount++; //add 2 steps
                newstep=1;
            }
            else if(newstep<-1)
            {
                stepcount++; //add 1 step
                newstep=-1;
            }
            activitydistribu(actscale(acttemp),&lowcount,&medcount,&highcount,&intensecount,&totalcount,&stepcount);
            activitylow.append(lowcount);
            activitymed.append(medcount);
            activityhigh.append(highcount);
            activityintense.append(intensecount);
            activityqvect.append(acttemp);
            totalcountsave.append(totalcount);
            stepcountsave.append(stepcount);
        }
    }
}


void twoDaccelviewer::activitydistribu(double act ,long *lowcountp,long *medcountp,long *highcountp,long *intensecountp,long *totalcountp,long *stepcountp)
{
    qApp->processEvents();

    (*totalcountp)++;
    if(act>=6&&act<=10) //set 6 to 10 for low activity: slow moving
    {
        (*lowcountp)++;
    }
    else if(act>10&&act<=20) //set 10 to 20 for mid activity: walking
    {
        (*medcountp)++;
    }
    else if(act>20&&act<=28) //set 20 to 28 for hi activity: running
    {
        (*highcountp)++;
    }
    else if(act>28) //set 28 for intense activity: shaking
    {
        (*intensecountp)++;
    }
}


void twoDaccelviewer::appendaccxyz(QVector<qint16> *accxQVp, QVector<qint16> *accyQVp, QVector<qint16> *acczQVp,
                                   QVector<qint16> *newaccxQVp, QVector<qint16> *newaccyQVp, QVector<qint16> *newacczQVp)
{
    int j;
    float zzz;
    qint16 z_new_data_average;

    if((newaccxQVp->size() < 1) || (newaccyQVp->size() < 1) || (newacczQVp->size() < 4))
    {
        return;
    }
    accxQVp->append(newaccxQVp->at(newaccxQVp->size()-1));
    accyQVp->append(newaccyQVp->at(newaccyQVp->size()-1));
    z_new_data_average=0;
    for(j=0;j<TDV_NUMPOINTSAMPLE_Z;j++)
    {
        if((j)<newacczQVp->size())
        {
            zzz=newacczQVp->at(j);
            if(zzz>32767) zzz-=32768;
            z_new_data_average+=zzz/TDV_NUMPOINTSAMPLE_Z; //prevent overflow in qint16
        }
    }
    acczQVp->append(z_new_data_average);
}


void twoDaccelviewer::calnewpos(float *posxp, float *posyp,qint16 newposx,qint16 newposy,qint16 newposz)
{
    float xxx,yyy; //prevent qint16 overflow
    qApp->processEvents();
    xxx=newposx;
    xxx+=newposy;
    xxx=(xxx-x_calm-y_calm)/windowxscale;
    yyy=newposz;
    yyy=(yyy-z_calm)/windowyscale;
    (*posxp)=ui->graphicsView->x()+ui->graphicsView->width()/2+xxx;        //right left
    if ((*posxp)<ui->graphicsView->x()) (*posxp)=ui->graphicsView->x();
    else if ((*posxp)>(ui->graphicsView->x()+ui->graphicsView->width())) (*posxp)=(ui->graphicsView->x()+ui->graphicsView->width());

    (*posyp)=ui->graphicsView->y()+ui->graphicsView->height()/2-yyy; //up down
    if ((*posyp)<ui->graphicsView->y()) (*posyp)=ui->graphicsView->y();
    else if ((*posyp)>(ui->graphicsView->y()+ui->graphicsView->height())) (*posyp)=(ui->graphicsView->y()+ui->graphicsView->height());
}


void twoDaccelviewer::draw(float newposx,float newposy,float *oldposxp,float *oldposyp)
{
    qApp->processEvents();
    Mainscene->addLine((*oldposxp),(*oldposyp),newposx,newposy,TDVSIGNALCOLOR);
    (*oldposxp)=newposx;
    (*oldposyp)=newposy;
}


double twoDaccelviewer::actscale(qint16 act)
{
    double scale= (double)50/(double)32767; //convert to db from 50db fullrange
    return (act*scale);
}


void twoDaccelviewer::routinedraw()
{
    float zXDelta;
    QPainterPath myQPP;

    if(!livedisplaymodebool)
    {
        int i, sizemyts, reduction; //reduction factor
        reduction=1;
        Mainscene->clear();
        zScene->clear();
        sizemyts=mytsQV.size();
        if(sizemyts>=10000) reduction=sizemyts/5000; //max 10000 points
        if(reduction<2) reduction=1;

        for(i = 0; i<sizemyts/reduction;i++) //reduction
        {
            if((mytsQV.at(i*reduction)>=myfirstFrmTS)&&(mytsQV.at(i*reduction)<=mylastFrmTS)) //reduction
            {
                if(firstdraw)
                {
                    x_calm=0;
                    y_calm=0;
                    z_calm=0;
                    calnewpos(&posx,&posy,accxqvect.at(i*reduction),accyqvect.at(i*reduction),acczqvect.at(i*reduction)); //reduction
                    oldposx = posx;
                    oldposy = posy;
                    start=Mainscene->addText("start", QFont("Arial",8));
                    start->setPos(posx,posy);
                    firstdraw=false;
                }
                calnewpos(&posx,&posy,accxqvect.at(i*reduction),accyqvect.at(i*reduction),acczqvect.at(i*reduction));//reduction
                draw(posx,posy,&oldposx,&oldposy);
            }

            if(mytsQV.at(i*reduction)>=mylastFrmTS) //reduction
            {
                if(!firstdraw)
                {
                    calnewpos(&posx,&posy,accxqvect.at(i*reduction),accyqvect.at(i*reduction),acczqvect.at(i*reduction));//reduction
                    draw(posx,posy,&oldposx,&oldposy);
                    end=Mainscene->addText("end", QFont("Arial",8));
                    if(posx>=ui->graphicsView->x()+ui->graphicsView->width())
                    {
                        if(posy>=ui->graphicsView->y()+ui->graphicsView->height())
                            end->setPos(posx-50,posy-50);
                        else
                            end->setPos(posx-50,posy);
                    }
                    else
                    {
                        end->setPos(posx,posy);
                    }
                    firstdraw=true;
                    break;
                }
            }
        }
        // draw z, no reduction
        zXDelta = ((float) accxqvect.size())/ui->zGV->width();   // figure out how many data points to take
        myQPP.moveTo(ui->zGV->x(), ui->zGV->y()+ui->zGV->height()/2-acczqvect.at(0)/PVZSCALE);
        for(i = 1; i < ui->zGV->width(); i++)
        {
            if(i*zXDelta > (acczqvect.size()-1))    // no more data
                break;
            myQPP.lineTo(ui->zGV->x()+i, ui->zGV->y()+ui->zGV->height()/2-acczqvect.at((qint32) (i*zXDelta))/PVZSCALE);
        }
        zScene->addPath(myQPP, QPen(Qt::red));
    }
}


//void twoDaccelviewer::setactlabel()
void twoDaccelviewer::showActivityText()
{
    QString tempQS;
    qApp->processEvents();
    if(totalcount!=0)
    {
        tempQS.append(QString("Activity Time: %1s\n").arg(totalcount/8));
        tempQS.append(QString("Pedometer: %1steps\n").arg(stepcount));
        tempQS.append(QString("LowAct:  %1s %2\%\n").arg(lowcount/8).arg(lowcount*100/totalcount));
        tempQS.append(QString("MedAct:  %1s %2\%\n").arg(medcount/8).arg(medcount*100/totalcount));
        tempQS.append(QString("HighAct: %1s %2\%\n").arg(highcount/8).arg(highcount*100/totalcount));
        tempQS.append(QString("Intense: %1s %2\%").arg(intensecount/8).arg(intensecount*100/totalcount));
    }

    // show the activity text
    if(activityText == 0)
    {
        activityText = Mainscene->addText(tempQS, QFont("Arial",8));
    }
    else
    {
        activityText->setPlainText(tempQS);
    }
    activityText->setPos(this->width()-135,this->height()/2);

}


void twoDaccelviewer::showEvent(QShowEvent *)
{
    if(!livedisplaymodebool)
    {
        routinedraw();
    }
    showActivityText();
}


void twoDaccelviewer::on_pushButton_trace_clicked()
{
    QString tempQS;

    firstdraw=true;
    timerindexcount=0;
    // keep text and display first
    if(metaAnalysisText != 0) tempQS = metaAnalysisText->toPlainText();
    Mainscene->clear();
    metaAnalysisText = 0;
    activityText = 0;
    if(!tempQS.isEmpty()) showMetaAnalysisText(&tempQS);
    ui->genActTextPB->setEnabled(false);
    timerid=QObject::startTimer(10); //faster
}


void twoDaccelviewer::timerEvent(QTimerEvent *)
{
    int sizemyts, reduction; //reduction factor
    float zXDelta;
    qint32 i;
    QPainterPath myQPP;

    if(!livedisplaymodebool)
    {
        reduction=1;
        sizemyts=mytsQV.size();
        if(sizemyts>=10000) reduction=sizemyts/5000; //max 10000 points
        if(reduction<2) reduction=1;
        if(timerindexcount<(sizemyts/reduction)) //reduction
        {
            if((mytsQV.at(timerindexcount*reduction)>=myfirstFrmTS)&&(mytsQV.at(timerindexcount*reduction)<=mylastFrmTS)) //reduction
            {
                if(firstdraw)
                {
                    calnewpos(&posx,&posy,accxqvect.at(timerindexcount*reduction),accyqvect.at(timerindexcount*reduction),acczqvect.at(timerindexcount*reduction));//reduction
                    oldposx = posx;
                    oldposy = posy;
                    start=Mainscene->addText("start", QFont("Arial",8));
                    start->setPos(posx,posy);
                    firstdraw=false;
                }
                calnewpos(&posx,&posy,accxqvect.at(timerindexcount*reduction),accyqvect.at(timerindexcount*reduction),acczqvect.at(timerindexcount*reduction));//reduction
                draw(posx,posy,&oldposx,&oldposy);
                lowcount = activitylow.at(timerindexcount*reduction);
                medcount = activitymed.at(timerindexcount*reduction);
                highcount = activityhigh.at(timerindexcount*reduction);
                intensecount = activityintense.at(timerindexcount*reduction);
                totalcount=totalcountsave.at(timerindexcount*reduction);
                stepcount=stepcountsave.at(timerindexcount*reduction);
                showActivityText();
            }
            if(mytsQV.at(timerindexcount*reduction)>=mylastFrmTS) //reduction
            {
                if(!firstdraw)
                {
                    calnewpos(&posx,&posy,accxqvect.at(timerindexcount*reduction),accyqvect.at(timerindexcount*reduction),acczqvect.at(timerindexcount*reduction));//reduction
                    draw(posx,posy,&oldposx,&oldposy);
                    lowcount = activitylow.at(timerindexcount*reduction);
                    medcount = activitymed.at(timerindexcount*reduction);
                    highcount = activityhigh.at(timerindexcount*reduction);
                    intensecount = activityintense.at(timerindexcount*reduction);
                    totalcount=totalcountsave.at(timerindexcount*reduction);
                    stepcount=stepcountsave.at(timerindexcount*reduction);
                    showActivityText();
                    end=Mainscene->addText("end", QFont("Arial",8));
                    if(posx>=ui->graphicsView->x()+ui->graphicsView->width())
                    {
                        if(posy>=ui->graphicsView->y()+ui->graphicsView->height())
                            end->setPos(posx-50,posy-50);
                        else
                            end->setPos(posx-50,posy);
                    }
                    else
                    {
                        end->setPos(posx,posy);
                    }
                    firstdraw=true;
                    QObject::killTimer(timerid);
                    ui->genActTextPB->setEnabled(true);
                }
            }
            // draw z, no reduction, redraw everytime
            zScene->clear();
            zXDelta = ((float) accxqvect.size())/ui->zGV->width();   // figure out how many data points to take
            myQPP.moveTo(ui->zGV->x(), ui->zGV->y()+ui->zGV->height()/2-acczqvect.at(0)/PVZSCALE);
            for(i = 1; i < ui->zGV->width(); i++)
            {
                if(i*zXDelta > (timerindexcount*reduction))    // draw up only until desired place
                    break;
                myQPP.lineTo(ui->zGV->x()+i, ui->zGV->y()+ui->zGV->height()/2-acczqvect.at((qint32) (i*zXDelta))/PVZSCALE);
            }
            zScene->addPath(myQPP, QPen(Qt::red));
            timerindexcount++;
        }
        else
        {
            QObject::killTimer(timerid);
            ui->genActTextPB->setEnabled(true);
        }
    }
}

void twoDaccelviewer::MMfastdisplayroutine(QVector<qint16> *accxQVp,
                                           QVector<qint16> *accyQVp,
                                           QVector<qint16> *acczQVp,
                                           int datarecord)
{
    float zXDelta;
    QPainterPath myQPP;
    int i=0,j=0,k=0;
    qint16 acttemp;
    QVector <qint16> accx;
    QVector <qint16> accy;
    QVector <qint16> accz;
    QPainterPath qp;
    accx.resize(1);
    accy.resize(1);
    accz.resize(4);
    for(j=0;j<datarecord*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC;j++)
    {
        accx[0]=(accxQVp->at(j));
        accy[0]=(accyQVp->at(j));
        accz[0]=(acczQVp->at(k));
        k++;
        accz[1]=(acczQVp->at(k));
        k++;
        accz[2]=(acczQVp->at(k));
        k++;
        accz[3]=(acczQVp->at(k));
        k++;
        appendaccxyz(&accxqvect,&accyqvect,&acczqvect,accxQVp,accyQVp,acczQVp);
        for(i = k-4; i<k; i++)
        {
            acczqvectoriginal.append(acczQVp->at(i));
        }
        acttemp = calcAct(&accxqvect, &accyqvect, &acczqvectoriginal, &newstep, &azold, &axold, &ayold, &az4old, &ax2old, &ay2old, &ax3old, &ay3old, &ax4old, &ay4old, &az8old, &az12old, &az16old);
        if(newstep>1) //update stepcount here
        {
            stepcount++; //add 1 step
            if(newstep>2) stepcount++; //add 2 steps
            newstep=1;
        }
        else if(newstep<-1)
        {
            stepcount++; //add 1 step
            newstep=-1;
        }
        activitydistribu(actscale(acttemp),&lowcount,&medcount,&highcount,&intensecount,&totalcount,&stepcount);
        if(firstdraw)
        {
            x_calm=0;
            y_calm=0;
            z_calm=0;
            calnewpos(&posx,&posy,accxqvect.at(accxqvect.size()-1),accyqvect.at(accyqvect.size()-1),acczqvect.at(acczqvect.size()-1)); //reduction
            oldposx = posx;
            oldposy = posy;
            qp.moveTo(oldposx,oldposy);
            qp.lineTo(posx,posy);
            start=Mainscene->addText("start", QFont("Arial",8));
            start->setPos(posx,posy);
            firstdraw=false;
        }
        calnewpos(&posx,&posy,accxqvect.at(accxqvect.size()-1),accyqvect.at(accyqvect.size()-1),acczqvect.at(acczqvect.size()-1)); //reduction
        qp.moveTo(oldposx,oldposy);
        qp.lineTo(posx,posy);
        oldposx = posx;
        oldposy = posy;
    }
    activitylow.append(lowcount);
    activitymed.append(medcount);
    activityhigh.append(highcount);
    activityintense.append(intensecount);
    activityqvect.append(acttemp);
    totalcountsave.append(totalcount);
    stepcountsave.append(stepcount);
    showActivityText();
    Mainscene->addPath(qp,TDVSIGNALCOLOR);

    // draw z scene
    zScene->clear();
    zXDelta = ((float) accxqvect.size())/ui->zGV->width();   // figure out how many data points to take
    myQPP.moveTo(ui->zGV->x(), ui->zGV->y()+ui->zGV->height()/2-acczqvect.at(0)/PVZSCALE);
    for(i = 1; i < ui->zGV->width(); i++)
    {
        if(i*zXDelta > (acczqvect.size()-1))    // no more data
            break;
        myQPP.lineTo(ui->zGV->x()+i, ui->zGV->y()+ui->zGV->height()/2-acczqvect.at((qint32) (i*zXDelta))/PVZSCALE);
    }
    zScene->addPath(myQPP, QPen(Qt::red));
}

void twoDaccelviewer::livedisplayroutine(QVector<qint16> *accxQVp, QVector<qint16> *accyQVp, QVector<qint16> *acczQVp)
{
    int i, removeXCt;
    qint16 acttemp;
    QPainterPath myQPP, myQPP2;
    float zXDelta;

    appendaccxyz(&accxqvect,&accyqvect,&acczqvect,accxQVp,accyQVp,acczQVp);
    for(i = 0; i<TDV_NUMPOINTSAMPLE_Z; i++)
    {
       acczqvectoriginal.append(acczQVp->at(i));
    }
    acttemp = calcAct(&accxqvect, &accyqvect, &acczqvectoriginal, &newstep, &azold, &axold, &ayold, &az4old, &ax2old, &ay2old, &ax3old, &ay3old, &ax4old, &ay4old,  &az8old, &az12old, &az16old);
    if(newstep>1) //update stepcount here
    {
        stepcount++; //add 1 step
        if(newstep>2) stepcount++; //add 2 steps
        newstep=1;
    }
    else if(newstep<-1)
    {
        stepcount++; //add 1 step
        newstep=-1;
    }

    // clear data that exceeds maximum
    if(accxqvect.size() > LIVEPVSHOWSECMAX*FRMSPERSEC)
    {
        removeXCt = accxqvect.size() - LIVEPVSHOWSECMAX*FRMSPERSEC;
        accxqvect.remove(0, removeXCt);
        accyqvect.remove(0, removeXCt);
        acczqvect.remove(0, removeXCt);
        acczqvectoriginal.remove(0, removeXCt*ACCLEN);
    }

    activitydistribu(actscale(acttemp),&lowcount,&medcount,&highcount,&intensecount,&totalcount,&stepcount);
    activitylow.append(lowcount);
    activitymed.append(medcount);
    activityhigh.append(highcount);
    activityintense.append(intensecount);
    activityqvect.append(acttemp);
    totalcountsave.append(totalcount);
    stepcountsave.append(stepcount);

    // clear scene
    if(!Mainscene->items().isEmpty())
    {
        Mainscene->clear();
        zScene->clear();
        metaAnalysisText = 0;
        activityText = 0;
        start = 0;
        end = 0;
    }

    // write the activity info
    showActivityText();

    if(accxqvect.size() < 2) return; // don't draw anything here, since not even two points
    // first data as starting point for 2D draw
    calnewpos(&posx,&posy,accxqvect.at(0),accyqvect.at(0),acczqvect.at(0)); //reduction
    myQPP.moveTo(posx, posy);
    for(i = 1; i < accxqvect.size(); i++)
    {
        calnewpos(&posx,&posy,accxqvect.at(i),accyqvect.at(i),acczqvect.at(i)); //reduction
        myQPP.lineTo(posx, posy);
    }

    Mainscene->addPath(myQPP, QPen(Qt::red));

    // draw the Z data
    zXDelta = ((float) LIVEPVSHOWSECMAX*FRMSPERSEC)/ui->zGV->width();   // figure out how many data points to take
    myQPP2.moveTo(ui->zGV->x(), ui->zGV->y()+ui->zGV->height()/2-acczqvect.at(0)/PVZSCALE);
    for(i = 1; i < acczqvect.size(); i++)
    {
        myQPP2.lineTo(ui->zGV->x()+((qint32) (((float)i)/zXDelta)), ui->zGV->y()+ui->zGV->height()/2-acczqvect.at(i)/PVZSCALE);
    }
    zScene->addPath(myQPP2, QPen(Qt::red));
}


void twoDaccelviewer::showMetaAnalysisText(QString *sleepQS)
{
    if(metaAnalysisText == 0)
        metaAnalysisText = Mainscene->addText(*sleepQS, QFont("Arial",8));
    else
        metaAnalysisText->setPlainText(*sleepQS);
    metaAnalysisText->setPos(10,this->height()*2/5);
}


void twoDaccelviewer::livedisplaylayout()
{
    ui->label->setVisible(false);
    ui->label_2->setVisible(false);
    ui->stDTE->setVisible(false);
    ui->endDTE->setVisible(false);
    ui->pushButton_trace->setVisible(false);
    ui->genActTextPB->setVisible(false);
}

void twoDaccelviewer::on_genActTextPB_clicked()
{
    QFile *outfileTp;
    QTextStream *outTstrp;
    QString outfileName;
    QMessageBox msgBox;

    outfileName = QFileDialog::getExistingDirectory(0, "Activity Summary Output Directory", QDir::currentPath()).append("\\");
    outfileName.append(QString("D%1_%2_act.txt").arg(firstFrmDT.toString("yyyyMMdd_hhmmss")).arg(mypdn));
    outfileTp = new QFile(outfileName);
    if(outfileTp->exists())
    {
        msgBox.setWindowTitle("Output Text File existed");
        msgBox.setText(QString("Out file %1 existed in selected directory.").arg(QString("D%1_%2_act.txt").arg(firstFrmDT.toString("yyyyMMdd_hhmmss")).arg(mypdn)));
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delete outfileTp;
            return;
        }
    }
    if(!outfileTp->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        delete outfileTp;
        return;
    }
    outTstrp = new QTextStream(outfileTp);

    *outTstrp << QString("Activity Time: %1s\n").arg(totalcount/8);
    if(totalcount!=0)
    {
        *outTstrp << QString("Pedometer: %1 steps\n").arg(stepcount);
        *outTstrp << QString("LowAct:  %1s %2\%\n").arg(lowcount/8).arg(lowcount*100/totalcount);
        *outTstrp << QString("MedAct:  %1s %2\%\n").arg(medcount/8).arg(medcount*100/totalcount);
        *outTstrp << QString("HighAct: %1s %2\%\n").arg(highcount/8).arg(highcount*100/totalcount);
        *outTstrp << QString("Intense: %1s %2\%").arg(intensecount/8).arg(intensecount*100/totalcount);
    }

    outfileTp->close();
    delete outTstrp;
    delete outfileTp;
}

void twoDaccelviewer::setPDN(quint8 pdn)
{
    mypdn = pdn;
    this->setWindowTitle(QString("Posture Viewer [%1]").arg((int)mypdn));

}
