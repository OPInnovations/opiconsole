#include "tutorialgame.h"
#include "ui_tutorialgame.h"

QPen blackpen(Qt::black);
QBrush blackbrush(Qt::black);
QPen ballpen(Qt::blue);
QBrush ballbrush(Qt::blue);
QGraphicsEllipseItem *Hole0=NULL;
QGraphicsEllipseItem *Hole1=NULL;
QGraphicsEllipseItem *Hole2=NULL;
QGraphicsEllipseItem *Hole3=NULL;
QGraphicsEllipseItem *temphit=NULL;


tutorialgame::tutorialgame(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::tutorialgame)
{
    ui->setupUi(this);
    TDV = NULL;
    bioparent=parent;
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    ui->pushButton_Start->setGeometry((this->x()+this->width())/2-(ui->pushButton_Start->width()/2),(this->y()+this->width())/2-(ui->pushButton_Start->width()),ui->pushButton_Start->width(),ui->pushButton_Start->height());
    ui->label_start->setGeometry(2,ui->graphicsView->y()+ui->graphicsView->height()-ui->label_start->height(),ui->label_start->width(),ui->label_start->height());
    sceneMain = new QGraphicsScene(ui->graphicsView->x(),ui->graphicsView->y(),ui->graphicsView->width()-5,ui->graphicsView->height()-5,this);
    ui->graphicsView->setScene(sceneMain);
    MB=sceneMain->addRect(ui->graphicsView->x(),ui->graphicsView->y(),sceneMain->width(),sceneMain->height(),blackpen);
    MB->hide();
    GB=sceneMain->addEllipse(ui->graphicsView->x()+MG_GAP,ui->graphicsView->y()+MG_GAP,sceneMain->width()-2*MG_GAP,sceneMain->height()-2*MG_GAP,blackpen);
    GB->hide();
    BB=sceneMain->addEllipse(ui->graphicsView->x()+GB_GAP+MG_GAP,ui->graphicsView->y()+(MG_GAP+GB_GAP),sceneMain->width()-2*(MG_GAP+GB_GAP),sceneMain->height()-2*(MG_GAP+GB_GAP),blackpen);
    BB->hide();
    AB=sceneMain->addEllipse(ui->graphicsView->x()+GB_GAP+MG_GAP+BA_GAP,ui->graphicsView->y()+(MG_GAP+GB_GAP+BA_GAP),sceneMain->width()-2*(MG_GAP+GB_GAP+BA_GAP),sceneMain->height()-2*(MG_GAP+GB_GAP+BA_GAP),blackpen);
    AB->hide();
    changelevel(LEVEL_M);
    Hole0->hide();
    Hole1->hide();
    Hole2->hide();
    Hole3->hide();
    ballxmin=ui->graphicsView->x();
    ballymin=ui->graphicsView->y();
    ballxmax=ui->graphicsView->x()+sceneMain->width()-BALL_DIAMETER;
    ballymax=ui->graphicsView->y()+ui->graphicsView->height()-BALL_DIAMETER;
    ballx=ui->graphicsView->x()+sceneMain->width()/2-BALL_DIAMETER/2;
    bally=ui->graphicsView->y()+ui->graphicsView->height()/2-BALL_DIAMETER/2;
    ui->BALL->setGeometry(ballx,bally,BALL_DIAMETER,BALL_DIAMETER);
    ui->BALL->hide();
    reset();
    ui->BALL->setAttribute(Qt::WA_TranslucentBackground,true);
    ui->label_A->setAttribute(Qt::WA_TranslucentBackground,true);
    ui->label_B->setAttribute(Qt::WA_TranslucentBackground,true);
    ui->label_G->setAttribute(Qt::WA_TranslucentBackground,true);
    ui->label_M->setAttribute(Qt::WA_TranslucentBackground,true);
    ui->label_start->setAttribute(Qt::WA_TranslucentBackground,true);
    ui->EDM->setAttribute(Qt::WA_TranslucentBackground,true);
}


tutorialgame::~tutorialgame()
{
    delete ui;
}


void tutorialgame::setpdnwindowtitle(int pdn)
{
    this->setWindowTitle(QString("%1").arg(pdn));
    if(TDV==NULL)
        TDV = new twoDaccelviewer(true,(quint8) pdn,this);
    if(TDV!=NULL)
    {
        TDV->livedisplaylayout();
        TDV->show();
    }
}


void tutorialgame::reset()
{
    x_calm = 0;
    y_calm = 0;
    z_calm = 0;
    x_old_value=0;
    y_old_value=0;
    z_old_value=0;
    M1oldvalue=0;
    M2oldvalue=0;
    MB->hide();
    GB->hide();
    BB->hide();
    AB->hide();
    ui->HIT->hide();
    ui->pushButton_Start->hide();
    ui->label_start->show();
    calmdown=false;
    checked=true;
    LEVEL=LEVEL_M;
    gamedone=false;
}


void tutorialgame::closeEvent(QCloseEvent *)
{
    qApp->processEvents();
    if(TDV!=NULL)
        TDV->setVisible(false);
    bioparent->close();
}


void tutorialgame::setpostureacc(QVector <qint16>*accxQVp, QVector <qint16>*accyQVp,
                                 QVector <qint16>*acczQVp)
{
    TDV->livedisplayroutine(accxQVp,accyQVp,acczQVp);
}


void tutorialgame::setdata(unsigned char EDM,qint16 RR,qint16 peak,
                           qint16 M2,
                           qint16 M1,qint16 G2,
                           qint16 G1,qint16 UP,
                           qint16 Beta,qint16 Sigma,
                           qint16 alpha,qint16 theta,
                           qint16 delta,float x_new,
                           float y_new,float z_new,
                           bool FFTFLAG)
{
    ui->EDM->setText(QString("RF%1").arg(EDM));
    RRnewvalue=RR;
    peaknewvalue=peak;
    M2newvalue=M2;
    M1newvalue=M1;
    G2newvalue=G2;
    G1newvalue=G1;
    UPnewvalue=UP;
    Betanewvalue=Beta;
    Sigmanewvalue=Sigma;
    alphanewvalue=alpha;
    thetanewvalue=theta;
    deltanewvalue=delta;
    FFTflag=FFTFLAG;
    y_new_value=y_new/4+y_old_value*0.75; //moving4 smoothing
    x_new_value=x_new/4+x_old_value*0.75;
    z_new_value=z_new/2+z_old_value*0.5;  //already average of 4
    x_old_value=x_new_value;
    y_old_value=y_new_value;
    z_old_value=z_new_value;
    processData();
    if(FFTflag==true)
    {
        dumpdata();
        M1oldvalue=M1newvalue;
        M2oldvalue=M2newvalue;
        FFTflag=false;
    }
}


void tutorialgame::dumpdata()
{
    ui->label_M->setText(QString("EMG: %1").arg((M1newvalue+M2newvalue)/655.4));
    ui->label_G->setText(QString("Gamma: %1").arg((G1newvalue+G2newvalue)/655.4));
    ui->label_B->setText(QString("Beta: %1").arg(Betanewvalue/327.7));
    ui->label_A->setText(QString("Alpha: %1").arg(alphanewvalue/327.7));
}


void tutorialgame::setbackgroundfftcolor()
{
    //M
    //clamp
    fftnowvalue = ((double)M2newvalue+(double)M1newvalue-800)/40; //32767=100db 10000=30db
    if (fftnowvalue>TG_FFTSIGNALMAX) fftnowvalue=TG_FFTSIGNALMAX;
    if (fftnowvalue<TG_FFTSIGNALMIN)  fftnowvalue=TG_FFTSIGNALMIN;
    //convert to pseudo-color
    //draw
    gg=long (fftnowvalue);
    if(gg<32)
    {
        rr=255;
        bb=32+gg*3;
    }
    else if (gg<224)
    {
        rr=298-gg*4/3;
        bb=128;
    }
    else
    {
        rr=0;
        bb=gg*4-765;
    }
    ffthsl.setHsl(rr,255,bb);   //for using HSL model
    //fftPen.setColor(ffthsl);
    QBrush fftBrushM(ffthsl);
    MB->setBrush(fftBrushM);
    //MB->setPen(blackpen);
    //G
    //clamp
    fftnowvalue = ((double)G1newvalue+(double)G2newvalue-800)/40; //adjustable
    if (fftnowvalue>TG_FFTSIGNALMAX) fftnowvalue=TG_FFTSIGNALMAX;
    if (fftnowvalue<TG_FFTSIGNALMIN)  fftnowvalue=TG_FFTSIGNALMIN;
    //convert to pseudo-color
    //draw
    gg=long (fftnowvalue);
    if(gg<32)
    {
        rr=255;
        bb=32+gg*3;
    }
    else if (gg<224)
    {
        rr=298-gg*4/3;
        bb=128;
    }
    else
    {
        rr=0;
        bb=gg*4-765;
    }
    ffthsl.setHsl(rr,255,bb);   //for using HSL model
    //fftPen.setColor(ffthsl);
    QBrush fftBrushG(ffthsl);
    GB->setBrush(fftBrushG);
    //GB->setPen(blackpen);
    //Beta
    //clamp
    fftnowvalue = (double)(Betanewvalue-1500)/20; //adjustable
    if (fftnowvalue>TG_FFTSIGNALMAX) fftnowvalue=TG_FFTSIGNALMAX;
    if (fftnowvalue<TG_FFTSIGNALMIN)  fftnowvalue=TG_FFTSIGNALMIN;
    //convert to pseudo-color
    //draw
    gg=long (fftnowvalue);
    if(gg<32)
    {
        rr=255;
        bb=32+gg*3;
    }
    else if (gg<224)
    {
        rr=298-gg*4/3;
        bb=128;
    }
    else
    {
        rr=0;
        bb=gg*4-765;
    }
    ffthsl.setHsl(rr,255,bb);   //for using HSL model
    //fftPen.setColor(ffthsl);
    QBrush fftBrushB(ffthsl);
    // BB->setPen(blackpen);
    BB->setBrush(fftBrushB);
    //alpha
    //clamp
    fftnowvalue = (double)alphanewvalue/20; //adjustable
    if (fftnowvalue>TG_FFTSIGNALMAX) fftnowvalue=TG_FFTSIGNALMAX;
    if (fftnowvalue<TG_FFTSIGNALMIN)  fftnowvalue=TG_FFTSIGNALMIN;
    //convert to pseudo-color
    //draw
    gg=long (fftnowvalue);
    if(gg<32)
    {
        rr=255;
        bb=32+gg*3;
    }
    else if (gg<224)
    {
        rr=298-gg*4/3;
        bb=128;
    }
    else
    {
        rr=0;
        bb=gg*4-765;
    }
    ffthsl.setHsl(rr,255,bb);   //for using HSL model
    //fftPen.setColor(ffthsl);
    QBrush fftBrushA(ffthsl);
    // AB->setPen(blackpen);
    AB->setBrush(fftBrushA);
}


void tutorialgame::processData()
{
    if(!calmdown)
    {
        beforecalm();
    }
    else
    {
        gamestart();
        //if start the game then ...
        if((!ui->pushButton_Start->isVisible()))
        {
            if(checked)  //check the direction,but not yet program
            {
                //move the ball
                moveball();

                if(FFTflag==true) //new FFT data
                {
                    //check the ball is in hole or not
                    temphit = checkBallInGroup();

                    //fire
                    fireit();  //check Jaw/Hit

                    //checknextlevel
                    if(checknextlevel()&&this->isVisible())
                    {
//                        qDebug()<<"level up";
                        changelevel(LEVEL);
                    }
                    setbackgroundfftcolor(); //only draw if needed
                }
            }
        }
    }
}


void tutorialgame::on_HIT_clicked()
{
    if(temphit!=NULL&&temphit->isVisible())
    {
        temphit->setVisible(false);
        temphit=NULL;
    }
}


void tutorialgame::changelevel(int choose)
{
    switch(choose)
    {
    case LEVEL_M:
        HOLE_DIAMETER=MHOLE_DIAMETER;
        holex0=ui->graphicsView->x()+HOLE_DIAMETER;
        holey0=ui->graphicsView->y()+HOLE_DIAMETER;
        Hole0=sceneMain->addEllipse(holex0,holey0,HOLE_DIAMETER,HOLE_DIAMETER,blackpen,blackbrush);
        holex1=ui->graphicsView->width()-HOLE_DIAMETER*2;
        holey1=ui->graphicsView->y()+HOLE_DIAMETER;
        Hole1=sceneMain->addEllipse(holex1,holey1,HOLE_DIAMETER,HOLE_DIAMETER,blackpen,blackbrush);
        holex2=ui->graphicsView->x()+HOLE_DIAMETER;
        holey2=ui->graphicsView->height()-HOLE_DIAMETER*2;
        Hole2=sceneMain->addEllipse(holex2,holey2,HOLE_DIAMETER,HOLE_DIAMETER,blackpen,blackbrush);
        holex3=ui->graphicsView->width()-HOLE_DIAMETER*2;
        holey3=ui->graphicsView->height()-HOLE_DIAMETER*2;
        Hole3=sceneMain->addEllipse(holex3,holey3,HOLE_DIAMETER,HOLE_DIAMETER,blackpen,blackbrush);

        break;
    case LEVEL_G:
        HOLE_DIAMETER=GHOLE_DIAMETER;
        holex0=ui->graphicsView->width()/2-HOLE_DIAMETER/2;
        holey0=ui->graphicsView->y()+MG_GAP+5;
        Hole0->setRect(holex0,holey0,HOLE_DIAMETER,HOLE_DIAMETER);
        Hole0->show();
        holex1=ui->graphicsView->width()-MG_GAP-HOLE_DIAMETER-8;
        holey1=ui->graphicsView->height()/2-HOLE_DIAMETER/2+8;
        Hole1->setRect(holex1,holey1,HOLE_DIAMETER,HOLE_DIAMETER);
        Hole1->show();
        holex2=ui->graphicsView->width()/2-HOLE_DIAMETER/2-5;
        holey2=ui->graphicsView->height()-MG_GAP-HOLE_DIAMETER;
        Hole2->setRect(holex2,holey2,HOLE_DIAMETER,HOLE_DIAMETER);
        Hole2->show();
        holex3=MG_GAP+5;
        holey3=ui->graphicsView->height()/2-HOLE_DIAMETER/2+8;
        Hole3->setRect(holex3,holey3,HOLE_DIAMETER,HOLE_DIAMETER);
        Hole3->show();
        break;
    case LEVEL_B:
        HOLE_DIAMETER=BHOLE_DIAMETER;
        Hole0->hide();
        holex1=ui->graphicsView->width()-MG_GAP-GB_GAP-HOLE_DIAMETER-10;
        holey1=ui->graphicsView->height()/2-HOLE_DIAMETER/2+8;
        Hole1->setRect(holex1,holey1,HOLE_DIAMETER,HOLE_DIAMETER);
        Hole1->show();
        Hole2->hide();
        holex3=MG_GAP+GB_GAP+5;
        holey3=ui->graphicsView->height()/2-HOLE_DIAMETER/2+8;
        Hole3->setRect(holex3,holey3,HOLE_DIAMETER,HOLE_DIAMETER);
        Hole3->show();
        break;
    case LEVEL_A:
        HOLE_DIAMETER=AHOLE_DIAMETER;
        Hole0->hide();
        holex1=ui->graphicsView->width()/2-HOLE_DIAMETER/2;
        holey1=ui->graphicsView->height()/2-HOLE_DIAMETER/2+15;
        Hole1->setRect(holex1,holey1,HOLE_DIAMETER,HOLE_DIAMETER);
        Hole1->show();
        Hole2->hide();
        Hole3->hide();
        break;
    default:
        Hole0->hide();
        Hole1->hide();
        Hole2->hide();
        Hole3->hide();
        break;
    }
}


bool tutorialgame::checknextlevel()
{
    if((!Hole0->isVisible())&&(!Hole1->isVisible())&&(!Hole2->isVisible())&&(!Hole3->isVisible()))
    {
        if(LEVEL<LEVEL_A&&LEVEL>=LEVEL_M)
        {
            LEVEL+=1;
            return true;
        }
        else if(!gamedone)
        {
            QMessageBox show_message;
            show_message.setWindowTitle("Game");
            show_message.setStyleSheet("background-image: url(:/images/images/bioshare-BGtiletexturedMixedGloss600px.jpg);");
            show_message.setText("Congratulations!");
            show_message.exec();
            gamedone=true;
            this->close();
            LEVEL=LEVEL_DONE;
            return false;
        }
        return false;
    }
    else
        return false;
}


void tutorialgame::fireit()
{
    //fire
    if((((M1newvalue+M2newvalue)/2-(M1oldvalue+M2oldvalue)/2)>TG_DM_THRESHOLD)||((M1newvalue+M2newvalue)/2>TG_FIRE_THRESHOLD))
    {
      on_HIT_clicked();
    }
}


void tutorialgame::beforecalm()
{
    if((M1newvalue+M2newvalue)/2!=0)
        ui->label_start->setText(QString("Relax, wait for %1 < %2 to Start").arg((M1newvalue+M2newvalue)/655.4).arg(TG_CALMDOWN_THRESHOLD));
    else
        ui->label_start->setText(QString("Head Staight and Steady"));

    if(((M1newvalue+M2newvalue)/655.4<TG_CALMDOWN_THRESHOLD)&&((M1newvalue+M2newvalue)/2!=0))
    {
        calmdown=true;
        ui->pushButton_Start->show();
        ui->label_start->hide();
    }
}


void tutorialgame::gamestart()
{
    //use jaw start
    if((((M1newvalue+M2newvalue)/2-(M1oldvalue+M2oldvalue)/2)>TG_DM_THRESHOLD)&&ui->pushButton_Start->isVisible()&&(M1oldvalue!=0))
    {
        this->on_pushButton_Start_clicked();
    }
    else if((((M1newvalue+M2newvalue)/2-(M1oldvalue+M2oldvalue)/2)<TG_DM_THRESHOLD)&&ui->pushButton_Start->isVisible()&&(M1oldvalue!=0))
    {
        ui->label_start->show();
        ui->label_start->setText("Clench JAW harder or HIT button!");
    }
}


void tutorialgame::moveball()
{
    ballx=(ballxmin+ballxmax)/2+(x_new_value+y_new_value-x_calm-y_calm)*X_MOVE_GAIN;        //right left
    if (ballx<ballxmin) ballx=ballxmin;
    else if (ballx>ballxmax) ballx=ballxmax;
    bally=(ballymin+ballymax)/2+(z_calm-z_new_value)*Z_MOVE_GAIN; //up down
    if (bally<ballymin) bally=ballymin;
    else if (bally>ballymax) bally=ballymax;
    {
        ui->BALL->setGeometry(ballx,bally,BALL_DIAMETER,BALL_DIAMETER);
    }
}


void tutorialgame::on_pushButton_Start_clicked()
{
    x_calm=x_new_value;
    y_calm=y_new_value;
    z_calm=z_new_value;
    ui->pushButton_Start->hide();
    MB->show();
    GB->show();
    BB->show();
    AB->show();
    Hole0->show();
    Hole1->show();
    Hole2->show();
    Hole3->show();
    ui->HIT->show();
    ui->label_start->hide();
    ui->BALL->show();
}


QGraphicsEllipseItem * tutorialgame::checkBallIn(QGraphicsEllipseItem *Hole,
                               int hx, int hy,
                               int holediameter,
                               int bx, int by, int balldiameter)
{
    if(bx+balldiameter<=hx+holediameter)
    {
        if(by+balldiameter<=hy+holediameter)
        {
            if(bx>=hx)
            {
                if(by>=hy)
                {
                    return Hole;
                }
                else
                    return NULL;
            }
            else
                return NULL;
        }
        else
            return NULL;
    }
    else
        return NULL;
}


QGraphicsEllipseItem * tutorialgame::checkBallInGroup()
{
    QGraphicsEllipseItem * temp;
    temp=checkBallIn(Hole0,holex0,holey0,HOLE_DIAMETER+4,ballx,bally,BALL_DIAMETER);
    if(temp!=NULL)
        return temp;
    temp=checkBallIn(Hole1,holex1,holey1,HOLE_DIAMETER+4,ballx,bally,BALL_DIAMETER);
    if(temp!=NULL)
        return temp;
    temp=checkBallIn(Hole2,holex2,holey2,HOLE_DIAMETER+4,ballx,bally,BALL_DIAMETER);
    if(temp!=NULL)
        return temp;
    temp=checkBallIn(Hole3,holex3,holey3,HOLE_DIAMETER+4,ballx,bally,BALL_DIAMETER);
    if(temp!=NULL)
        return temp;
    return NULL;
}


void tutorialgame::on_pushButton_2D_accel_clicked()
{
    if(TDV!=NULL)
    {
        if(!TDV->isVisible())
        {
            TDV->livedisplaylayout();
            TDV->show();
        }
    }
}
