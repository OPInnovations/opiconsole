#include "showdatawindow.h"
#include "ui_showdatawindow.h"

QPen redPen(Qt::red);
QPen whitePen(Qt::white);
QBrush whiteBrush(Qt::white);
QBrush redBrush(Qt::red);
QPen blackPen(Qt::black);

#define  GUIDELINECOLOR  blackPen  //define the color of guide line
#define  SIGNALCOLOR     redPen    //define the color of signal line


showdatawindow::showdatawindow(qint32 *pdnshowcountp,QWidget *parent) :
    ui(new Ui::showdatawindow)
{
    pdnshowcount = pdnshowcountp;
    show_message.setWindowTitle("Warnning");
    ui->setupUi(this);
    TDV = NULL;
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    //set draw pen width
    SIGNALCOLOR.setWidth(PENWIDTH);
    //set fft pen's width
    fftPen.setWidth(FFTPENWIDTH);
    //set all scenes for graphics view
    setUpTotalScene();
    //reset the showdatawindow
    reset();
    //Setting a fixed size for the transformation
    transformer.setSize(NUMPATHFFT);
}


void showdatawindow::closeEvent(QCloseEvent *)
{

    (*pdnshowcount)-=1;
    if(pdnshowcount<0) (*pdnshowcount)=0;
    if(already_open) already_open=false;
    if(TDV!=NULL)
    TDV->setVisible(false);
}


void showdatawindow::showEvent(QShowEvent *)
{
    if(!already_open)
    {
        (*pdnshowcount)+=1;
        already_open=true;
    }
}


void showdatawindow::showDataControl(void)
{
    //RF
    ui->EDM->setText(QString("%1").arg(PACKAGE_t.payload[144]));
}


void showdatawindow::reset(void)
{
    int i;

    beforevalue=0;
    countguidelinez=0;
    countguidelineadc=0;
    ui->EDM->setText("");
    PDN_NUMBER=-1;//in case the old pdn number equals the pdn number now;
    fft_is_ready=false;
    newdata=false;
    numpathfftcol=scenefft->width()/FFTPENWIDTH;  //define how many columns in fft
    //the screen is not full,so dont clean
    fft_clean=false;
    //let the final data can reach the next start data
    first_Time_Samplez=true;
    first_Time_Sampleadc=true;
    //initial unit gain
    //from here
    ui->FFTGainSlider->setValue(0); //assume when value equals middle value of the zoomctrl then the zoomctrl has unit gain
    fft_zoom_in_step=(float)FFT_MAX_SCALE/(float)ui->FFTGainSlider->maximum();
    fft_zoom_out_step=(1-(float)FFT_MIN_SCALE)/(float)ui->FFTGainSlider->minimum();
    fft_zoomrate= 1;  // set initial vertical zoom rate 1
    //end
    //setup FFT increase_log[i] table here for Equalization and draw length
    for( i=0; i<=NUMPATHFFT/2; i++)
    {
        ffttemp = (i+1);
        increase_log[i]=10*log10(ffttemp);//10db/dec equalization && draw segment length
    }
    // end setup
    //before show new data we clean the screen
    //from here
    sceneTemp->clear();
    sceneX->clear();
    sceneY->clear();
    sceneZ->clear();
    sceneADC->clear();
    scenefft->clear();
    //initalize the window is close
    already_open=false;
    temp_new_data=MAX_TEMP; //initialize to high value
    //for fft
    //from here
    fftk=8;  // set initial gain multiplier 8=32db 6=43db dynamic range
    fftco=0;   // (-)fftco value 2 => -70-2=-72 db for 1024FFT (-70db built-in)
    ui->fftkfactor->setText(QString::number(fftk));
    ui->fftcofactor->setText(QString::number(fftco));
    //end
    //for zoom ctrl initialize value
    //from here
    ui->zoomctrl->setValue(0); //assume when value equals middle value of the zoomctrl then the zoomctrl has unit gain
    zoom_in_step=(float)MAX_SCALE/(float)ui->zoomctrl->maximum();
    zoom_out_step=(1-(float)MIN_SCALE)/(float)ui->zoomctrl->minimum();
    zoomrate= 1;  // set initial vertical zoom rate 1
    //end
    //defining the scaled show points size for adc
    numScaleADC=INITIALADC; //should be times of NUMPOINTSAMPLE
    numScalefft=NUMPATHFFT/4; //256 bins, half of 256Hz BW
    //for zoom timeslider initialize value
    //from here
    ui->timeSlider->setValue(numScaleADC); //initialize the value equals the initial numScalseADC
    ui->timeSlider->setMaximum(NUMPATHADC);
    ui->timeSlider->setMinimum(TIMESLIDER_MIN_SAMPLE_POINTS);
    ui->timeSlider->setSingleStep(TIMESLIDER_MIN_SAMPLE_POINTS);
    ui->timeSlider->setPageStep(TIMESLIDER_MIN_SAMPLE_POINTS);
    //end
    ui->FFT_TimeSlider->setValue(numScalefft);
    //to count how many points is being save
    //from here
    countx=0;
    county=0;
    countz=0;
    countadc=0; //not used for Draw ADC
    counttemp=0;
    countfft=0;
    countfftcolumn=0;
    //end
    //set window title
    title.sprintf("PDN%d",PDN_NUMBER);
    this->setWindowTitle(title);
    //test dump
    tempavg=0;
    adcavg=0;
    accxavg=0;
    accyavg=0;
    acczavg=0;
    ffgavg=0;
    //for sure every index has a data,otherwise the adc reverse will crash
    for( i=0;i<NUMPOINTSAMPLE;i++)
        adc_new_data[i]=0;
}


void showdatawindow::setUpTotalScene()
{
    //new all scenes for graphics view
    sceneTemp = new QGraphicsScene(ui->drawTemp->x(),ui->drawTemp->y(),ui->drawTemp->width(),ui->drawTemp->height(),this);
    sceneX = new QGraphicsScene(ui->drawAccx->x(),ui->drawAccx->y(),ui->drawAccx->width(),ui->drawAccx->height(),this);
    sceneY = new QGraphicsScene(ui->drawAccy->x(),ui->drawAccy->y(),ui->drawAccy->width(),ui->drawAccy->height(),this);
    sceneZ = new QGraphicsScene(ui->drawAccz->x(),ui->drawAccz->y(),ui->drawAccz->width(),ui->drawAccz->height(),this);
    sceneADC = new QGraphicsScene(ui->drawADC->x(),ui->drawADC->y(),ui->drawADC->width(),ui->drawADC->height(),this);
    scenefft= new QGraphicsScene(ui->drawfft->x(),ui->drawfft->y(),ui->drawfft->width()-2,ui->drawfft->height()-2,this);
    //set scene to graphicsScene
    ui->drawTemp->setScene(sceneTemp);
    ui->drawAccx->setScene(sceneX);
    ui->drawAccy->setScene(sceneY);
    ui->drawAccz->setScene(sceneZ);
    ui->drawADC->setScene(sceneADC);
    ui->drawfft->setScene(scenefft);
}


int showdatawindow::routinedrawgroup(void)
{
    int i;

    if(newdata&&(!ui->CheckPauseShow->isChecked()))  //show the data when get new package
    {
        calNewPack();
        saveData();
        for( i= 0;i< NUMPOINTSAMPLE; i++)
        {
            if(!ui->checkReverse->isChecked())
                adc_new_data[i]=(float)adc_new_data[i]*zoomrate;
            else
                adc_new_data[i]=(float)adc_new_data[i]*zoomrate*-1;
        }
        //after save new data to array we write the new data on the scene
        //from here
        if(ui->checkTemp->isChecked())
        {
            drawData(ui->drawTemp,sceneTemp ,MAX_TEMP,MIN_TEMP,saveTemp,counttemp,NUMPATHTEMP,NUMPOINTSAMPLE_TEMP,0,false);  //no use countguileline and firsttimesample
        }
        if(ui->checkAccx->isChecked())
        {
            drawData(ui->drawAccx,sceneX ,MAX_MIN_X,-1*MAX_MIN_X,saveX,countx,NUMPATHX,NUMPOINTSAMPLE_X,0,false); //no use countguileline and firsttimesample
        }
        if(ui->checkAccy->isChecked())
        {
            drawData(ui->drawAccy,sceneY ,MAX_MIN_Y,-1*MAX_MIN_Y,saveY,county,NUMPATHY,NUMPOINTSAMPLE_Y,0,false); //no use countguileline and firsttimesample
        }
        if(ui->checkAccz->isChecked())
        {
            drawData(ui->drawAccz,sceneZ ,MAX_MIN_Z,-1*MAX_MIN_Z,saveZ,countz,NUMPATHZ,NUMPOINTSAMPLE_Z,&countguidelinez,first_Time_Samplez); //use countguileline and firsttimesample
        }
        if(ui->checkADC->isChecked())
        {
            drawData(ui->drawADC,sceneADC ,MAX_MIN_ADC,-1*MAX_MIN_ADC,adc_new_data,countadc,numScaleADC,NUMPOINTSAMPLE,&countguidelineadc,first_Time_Sampleadc);// use countguileline and firsttimesample
        }
        if(ui->checkSpectrogram->isChecked())
        {
//            QTime nowtime = QTime::currentTime();
            drawFFT();
//            QTime newtime = QTime::currentTime();
//            qDebug()<< "Spectrogram" <<nowtime.toString("hhmmsszzz")<<newtime.toString("hhmmsszzz");
        }
        //end
        showDataControl();
        newdata=false;
        return 1;
    }
    return 0;
}


int showdatawindow::drawFFT(void)
{
    int j;
    int tempy;

    begin=ui->drawfft->x();
    button=ui->drawfft->y()+ui->drawfft->height();
    tempy=button;
    //decide the gap between every points(y-axis)
    if(numScalefft>NUMPATHFFT/2)  //display to max of 256Hz
        numScalefft=NUMPATHFFT/2;
    increase = (float)ui->drawfft->height();

    // add LOG scale increase here
    fft_total =increase_log[numScalefft]-increase_log[1]; //start from 2=1Hz
    increase=increase/fft_total;  //now normalized to display section

    //clean the screen
    if(fft_clean)
    {
//        scenefft->clear();  // right place?
        fft_clean=false;
    }
    if(fft_is_ready) //choose one of the column to draw
    {

        for( j=2;j<=numScalefft;j++) //skip DC, start from 2=1Hz;
        {
            //distribute the data
            fftnowvalue=effected_by_ko_co_fft[j];
            fftnowvalue=fftnowvalue*fft_zoomrate;
            //clamp
            if (fftnowvalue>FFTSIGNALMAX) fftnowvalue=FFTSIGNALMAX;
            if (fftnowvalue<FFTSIGNALMIN)  fftnowvalue=FFTSIGNALMIN;
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
            fftPen.setColor(ffthsl);
            scenefft->addRect(begin+(countfftcolumn-1)*FFTPENWIDTH,
                              button-(increase_log[j]-increase_log[1])*increase,
                              FFTPENWIDTH,
                              tempy-(button-(increase_log[j]-increase_log[1])*increase),
                              fftPen,QBrush(ffthsl)
                              );
            tempy=button-(increase_log[j]-increase_log[1])*increase;
        }//for(int j=2;j<=numScalefft;j++) end
        fft_is_ready=false;  //set false when drawing is done

        return 1;
    }//if(fft_is_ready) end
    return 0;
}


int showdatawindow::drawData(QGraphicsView *view,QGraphicsScene *scene,int max,int min,int *database,int count,int num_of_points,int sample_points,int *countline,bool first_Time_Sample)
{
    int i;
    //calculate where to put the x-axis(the middle of the scene)
    middle=view->y()+(view->height()-2)/2;
    //design what is the middle value of your input siganl
    //from here
    signalmiddle = (max+min)/2; //should be addition, not subtraction
    //end
    //decide where to begin to draw
    begin=view->x();
    //decide the gap between every points(x-axis)
    increase=((float)view->width()-2)/(float)num_of_points;
    //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    scalerate=((float)(view->height()-2))/(float)(max-min); //ratio to max-min

    if(sample_points==1)  //have to wait at least 2 points to form a line
    {
        if(count>=2) //two points form a line
        {
            nowvalue=database[count-1];
            beforevalue=database[count-2];
            //clamp
            if(nowvalue>max)
                nowvalue=max;
            else if(nowvalue<min)
                nowvalue=min;
            if(beforevalue>max)
                beforevalue=max;
            else if(beforevalue<min)
                beforevalue=min;
            scene->addLine(begin+(count-2)*increase,middle-(beforevalue-signalmiddle)*scalerate,begin+(count-1)*increase,middle-(nowvalue-signalmiddle)*scalerate,SIGNALCOLOR);
            if(count-1==NUMPATHX-1)
            {
                scene->clear();
            }
        }//if(count>=2) end
        else
        {
            nowvalue=database[count-1];
            scene->addLine(begin+(count-2)*increase,middle-(beforevalue-signalmiddle)*scalerate,begin+(count-1)*increase,middle-(nowvalue-signalmiddle)*scalerate,SIGNALCOLOR);
        }
        return 1;
    }// if(sample_points==1) end
    else if(sample_points==NUMPOINTSAMPLE)
    {
        QPainterPath *qp;
        qp = new QPainterPath();
        for(i=sample_points;i>0;i--) //total sample points
        {
            (*countline)++;
            if((*countline)*increase>scene->width()){
                (*countline)=0;
                scene->addPath((*qp),SIGNALCOLOR);
                delete qp;
                qp = new QPainterPath();
                scene->clear();
            }//ADC end
            if(sample_points-i>0){
                nowvalue=database[sample_points-i];
                beforevalueadc= nowvalue;
                beforevalue=database[sample_points-i-1];
            }
            else if((sample_points-i==0)){
                nowvalue=database[0];
                beforevalue = beforevalueadc;
            }
            //clamp
            if(nowvalue>max)
                nowvalue=max;
            else if(nowvalue<min)
                nowvalue=min;
            if(beforevalue>max)
                beforevalue=max;
            else if(beforevalue<min)
                beforevalue=min;
            qp->moveTo(begin+((*countline)-1)*increase,middle-(beforevalue-signalmiddle)*scalerate);
            qp->lineTo(begin+(*countline)*increase,middle-(nowvalue-signalmiddle)*scalerate);
        }//for end
        first_Time_Sample=false;
        scene->addPath((*qp),SIGNALCOLOR);
        delete qp;
        return 1;
    }//else if ADC
    else if(sample_points==NUMPOINTSAMPLE_Z)
    {
        QPainterPath *qp;
        qp = new QPainterPath();
        for(i=sample_points;i>0;i--) //total sample points
        {
            (*countline)++;
            if(((count-i)==(NUMPATHZ-1)))
            {
                (*countline)=0;
                scene->addPath((*qp),SIGNALCOLOR);
                delete qp;
                qp = new QPainterPath();
                scene->clear();
            }//ACCZ

            if(count-i>0)   //save the data if it is not the particular case
            {
                nowvalue=database[count-i];
                beforevalue=database[count-i-1];
            }
            else if((count-i==0)&&!first_Time_Sample) //if it is not the first,we draw the save[0] to catch the save[NUMPATH-1] case
            {
                nowvalue=database[0];
                beforevalue=database[NUMPATHZ-1];
            }
            //clamp
            if(nowvalue>max)
                nowvalue=max;
            else if(nowvalue<min)
                nowvalue=min;
            if(beforevalue>max)
                beforevalue=max;
            else if(beforevalue<min)
                beforevalue=min;
            qp->moveTo(begin+((*countline)-1)*increase,middle-(beforevalue-signalmiddle)*scalerate);
            qp->lineTo(begin+(*countline)*increase,middle-(nowvalue-signalmiddle)*scalerate);
        }//for end
        first_Time_Sample=false;
        scene->addPath((*qp),SIGNALCOLOR);
        delete qp;
        return 1;
    }//else if(sample_points>1) end

    return 0;
}


int showdatawindow::calNewPack(void)
{
    int i;
    QVector <qint16> accxQV,accyQV,acczQV;

    //temperature
    //from here to moving-8 x100
    temp_last_data = temp_new_data *0.875;
    temp_new_data=temp_last_data + 0.125*(PACKAGE_t.payload[1+TSLEN+WLFRMHDRLEN+2*ADCLEN]*113-4680); // moving8 x100
    //end temperature
    //accx
    //from here
    x_new_data=PACKAGE_t.payload[138];
    x_new_data=(x_new_data>127)?(x_new_data-256):x_new_data;   
    accxQV.append(wirelessaccscale(x_new_data));
    //end accx
    //accy
    //from here
    y_new_data=PACKAGE_t.payload[139];
    y_new_data=(y_new_data>127)?(y_new_data-256):y_new_data;
    accyQV.append(wirelessaccscale(y_new_data));
    //end accy
    //accz
    //from here
    for(i=0;i<NUMPOINTSAMPLE_Z;i++)
    {
        z_new_data[i]=PACKAGE_t.payload[140+i];      
        z_new_data[i]=(z_new_data[i]>127)?(z_new_data[i]-256):z_new_data[i];
        acczQV.append(wirelessaccscale(z_new_data[i]));
    }
    //end accz
    TDV->livedisplayroutine(&accxQV,&accyQV,&acczQV);
    //adc
    //from here  //default NUMPOINTSAMPLE=64 for full FFT
    max_min_pair=ADCLEN/NUMPOINTSAMPLE;  //skip range
    for(i=0; i<NUMPOINTSAMPLE; i++)//skip range
    {
        adc_new_data[i]=(PACKAGE_t.payload[1+TSLEN+WLFRMHDRLEN+2*(max_min_pair*i)] << 8) + PACKAGE_t.payload[1+TSLEN+WLFRMHDRLEN+2*(max_min_pair*i)+1];
        if(adc_new_data[i]>32767)
            adc_new_data[i]=adc_new_data[i]-65536;
    }
    //end adc
    return 1;
}


int showdatawindow::saveData()
{
    int i,j;
    //temperature
    //from here
    if(ui->checkTemp->isChecked())
    {
        if(counttemp>=0&&counttemp<NUMPATHTEMP)
        {
            saveTemp[counttemp]=temp_new_data;
            counttemp++;
        }
        else
        {
            counttemp=0;
            saveTemp[counttemp]=temp_new_data;
            counttemp++;
        }
    }
    //end temperature

    //accx
    //from here
    if(ui->checkAccx->isChecked())
    {
        if(countx>=0&&countx<NUMPATHX)
        {
            saveX[countx]=x_new_data;
            countx++;
        }
        else
        {
            countx=0;
            saveX[countx]=x_new_data;
            countx++;
        }
    }
    //end accx

    //accy
    //from here
    if(ui->checkAccy->isChecked())
    {
        if(county>=0&&county<NUMPATHY)
        {
            saveY[county]=y_new_data;
            county++;
        }
        else
        {
            county=0;
            saveY[county]=y_new_data;
            county++;
        }
    }
    //end accy

    //accz
    //from here
    if(ui->checkAccz->isChecked())
        for( i=0;i<NUMPOINTSAMPLE_Z;i++)
        {
            if(countz>=0&&countz<NUMPATHZ)
            {
                saveZ[countz]=z_new_data[i];
                countz++;
            }
            else
            {
                countz=0;
                saveZ[countz]=z_new_data[i];
                countz++;
            }
        }
    //end accz

    //fft
    //from here
    if(ui->checkSpectrogram->isChecked())
    {
        for(i=0; i<NUMPOINTSAMPLE; i++)  //default NUMPOINTSAMPLE=64 for full FFT
        {
            if(countfft>=0&&countfft<NUMPATHFFT){
                saveFFT[countfft]=adc_new_data[i];
                countfft++;
            }else{
                fft_is_ready=true;
                if(countfftcolumn>=0&&countfftcolumn<numpathfftcol)
                    countfftcolumn++;
                else{
                    countfftcolumn=0;
                    fft_clean=true; //the screen is full
                }//if(countfftcolumn>=0&&countfftcolumn<NUMPATHFFTROW) end
                for(j=0;j<NUMPATHFFT;j++) //add Hanning window
                    saveFFT[j]=0.5*(1-qCos((2*M_PI*j)/(NUMPATHFFT-1)))*saveFFT[j]; //add Hanning window
                transformer.forwardTransform(saveFFT,calculatedFFT);
                fftresult=transformer.toComplex(calculatedFFT);
                //effected by fftk and fftco
                for(j=0;j<NUMPATHFFT/2;j++){
                    re=(double)fftresult[j].real()*(double)fftresult[j].real();
                    im=(double)fftresult[j].imaginary()*(double)fftresult[j].imaginary();
                    ffttemp=re+im;
                    if(ffttemp<=0)  //can't log10(0)
                        effected_by_ko_co_fft[j]= 0;//in db
                    else
                        effected_by_ko_co_fft[j]=fftk*((10*log10(ffttemp))-70-fftco+increase_log[j]);//in db + equalization
                    //equalization 10db/dec added
                }//for(int j=0;j<NUMPATHFFTROW/2;J++) end
                countfft=NUMPATHFFT/2; //half way
                for(j=0;j<countfft;j++)
                    saveFFT[j]=saveFFT[countfft+j];
                saveFFT[countfft]=adc_new_data[i];
                countfft++;
            }//else end
        }//for(int i=0;i<NUMPOINTSAMPLE;i++) end
    }
    return 1;
}


void showdatawindow::setPdnNum(int num)
{
    PDN_NUMBER=num;
    //set window title
    title.sprintf("[%d]",PDN_NUMBER);
    this->setWindowTitle(title);
    if(TDV==NULL)
        TDV = new twoDaccelviewer(true,(quint8) PDN_NUMBER,this);
    if(TDV!=NULL)
    {
        TDV->livedisplaylayout();
        TDV->show();
        TDV->setGeometry(this->x()+this->width()+25, this->y()+30, TDV->width(), TDV->height());
    }
}


showdatawindow::~showdatawindow()
{
    delete ui;
}


int showdatawindow::getStruct(OPIPKT_t* opipointer)
{
    int i;

    PACKAGE_t.dataCode=opipointer->dataCode;
    PACKAGE_t.length=opipointer->length;
    for(i=0; i<opipointer->length; i++)
        PACKAGE_t.payload[i]=opipointer->payload[i];
    newdata=true;
    return this->routinedrawgroup();
}


void showdatawindow::on_zoomctrl_valueChanged(int value)
{
    zoomctrlvalue=ui->zoomctrl->value();
    if(zoomctrlvalue>0)
        zoomrate=1+zoomctrlvalue*zoom_in_step;
    else if(zoomctrlvalue<0)
        zoomrate=1-zoomctrlvalue*zoom_out_step;
    else
        zoomrate=1;
}


void showdatawindow::on_fftbutton_clicked()
{
    fftk =  ui->fftkfactor->toPlainText().toDouble();
    fftco=  ui->fftcofactor->toPlainText().toDouble();
}


void showdatawindow::on_timeSlider_valueChanged(int value)
{
    sceneADC->clear();
    countguidelineadc=0;
    numScaleADC=ui->timeSlider->value();
    ui->timemorelabel->setText(QString("%1s").arg((ui->timeSlider->value())/512)); //divide by samplerate
}


void showdatawindow::on_FFT_TimeSlider_valueChanged(int value)
{
    numScalefft=ui->FFT_TimeSlider->value();
    QString temp;
    temp.setNum(value*512/NUMPATHFFT).append("Hz");
    ui->label_3->setText(temp);
}


void showdatawindow::on_FFTGainSlider_valueChanged(int value)
{
    fft_zoomctrlvalue=ui->FFTGainSlider->value();
    if(fft_zoomctrlvalue>0)
        fft_zoomrate=(1+fft_zoomctrlvalue*fft_zoom_in_step);
    else if(fft_zoomctrlvalue<0)
        fft_zoomrate=(1-fft_zoomctrlvalue*fft_zoom_out_step);
    else
        fft_zoomrate=1;
}


void showdatawindow::on_checkReverse_toggled(bool checked)
{
    sceneADC->clear();
    countguidelineadc=0;
}


void showdatawindow::on_pushButton_2D_accel_clicked()
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
