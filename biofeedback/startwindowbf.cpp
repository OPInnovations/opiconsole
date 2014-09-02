#include "startwindowbf.h"
#include "ui_startwindowbf.h"

startwindowbf::startwindowbf(qint32 *comPortUser,bool setgamemode, QWidget *parent) :
    ui(new Ui::startwindowbf)
{
      //set screen size
    ui->setupUi(this);
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    gamemode = setgamemode;
    comPortUserp = comPortUser;
    pdnshowcountp = 0;
    for(int i =0;i<OPIWANT_MAX_BF+1;i++)
      sdw[i] = new biofeedback(&pdnshowcountp,this);

    //set message window title
    show_message.setWindowTitle("Warning");
    show_message.setStyleSheet("background-image: url(:/images/images/bioshare-BGtiletexturedMixedGloss600px.jpg);");
    QFont temp;
    temp.setPointSize(BFWORNNINGWORDSIZE);
    show_message.setFont(temp);
    hide_check_pdn();
    //initialize
    Show_count = 0;
    firsttimer=true;
    for(int i=0;i<=255;i++)
        PDN_LOCATION[i]=0;
    for(int i=0;i<OPIWANT_MAX_BF+1;i++)
       { PDNBOX[i]=false;
         pdnOn[i]=false;
         PDN_NUMBER[i]=0xff;
       }

    //initialize the pdn is first used
    //so that if it is the second time we use we will get the information from showdatawindow
    for(int i=0;i<5;i++)
        firstpdn[i]=true;
    //portOn off not yet connect
    portOn = false;
    //set window icon
    this->setWindowIcon(QIcon("../images/opi_ico.ico"));
    ui->pushButton->setEnabled(true);
    //file is the first time open
    firstfile=true;
}

startwindowbf::~startwindowbf()
{
    delete ui;
    for(int i =0;i<OPIWANT_MAX_BF+1;i++)
          delete sdw[i];
}

void startwindowbf::timerEvent(QTimerEvent *event)
{   if(portOn&&(pdnshowcountp>0))
        fresh();
    else if(pdnshowcountp==0)
    {
        if(portOn)
            opi_closeucd_com(&com);
        portOn=false;
        *comPortUserp = BFCOMPORTFREE;
        if(!firsttimer)
            this->killTimer(timeid);
        if(!firstfile)
            fclose(file);
        firsttimer=true;
        firstfile=true;
        ui->checkWriteFileBox->setDisabled(false);
    }
}



int startwindowbf::fresh()
{
    if((opiucd_getwltsdata(&com,&PACKAGE_tp[0])==NEWDATAGETCODE_BF)) //check new data
    {
        if(((PACKAGE_tp[0].payload[1+TSLEN+WLFRMHDRLEN+1])&(0X03))<3) //check quality 0=clean, 1=0fix, 2=<20fix, 3=>20fix
        {
            freshpdnindex=PDN_LOCATION[PACKAGE_tp[0].payload[7]];
            if(PACKAGE_tp[0].payload[7]==sdw[freshpdnindex]->PDN_NUMBER)
            {
                if(sdw[freshpdnindex]->getStruct(&PACKAGE_tp[0])==1)//if we get the struct then we draw
                {
                    if(ui->checkWriteFileBox->isChecked()) //if true then write file
                        opipkt_put_file(&PACKAGE_tp[0], file);   // write to file
                    //draw routine end
                }//if get structure end
            }
        }//if new data end
        else
        {
            qDebug()<<"bad quality";
            return 0;
        }
        return 1;
    }//if quality end
    else
    {
        return 0;
    }

}



void startwindowbf::hide_check_pdn()
{
    ui->CBP1->hide();
    ui->CBP2->hide();
    ui->CBP3->hide();
    ui->CBP4->hide();
}

void startwindowbf::show_check_pdn()
{
    int i;

    for(i=1;i<=4;i++)
    {
        if(PDN_NUMBER[i]!=0xff)
        {
            switch(i)
            {
            case 1:
                ui->CBP1->show();
                pdntemp.sprintf("[%d]",PDN_NUMBER[1]);
                ui->CBP1->setText(pdntemp);
                break;
            case 2:
                ui->CBP2->show();
                pdntemp.sprintf("[%d]",PDN_NUMBER[2]);
                ui->CBP2->setText(pdntemp);
                break;
            case 3:
                ui->CBP3->show();
                pdntemp.sprintf("[%d]",PDN_NUMBER[3]);
                ui->CBP3->setText(pdntemp);
                break;
            case 4:
                ui->CBP4->show();
                pdntemp.sprintf("[%d]",PDN_NUMBER[4]);
                ui->CBP4->setText(pdntemp);
                break;
            default:
                break;
            }
        }
    }
}


int startwindowbf::opiucd_getPDN(void)
{
    int totalcount=0;
    int i;

    opiucd_status(&com,&PACKAGE_tp[0]);


    for(i=0;i<5;i++)
        ucd[i]= PACKAGE_tp[0].payload[i];

    if(PACKAGE_tp[0].payload[20]!=0xFF)
    {
        PDN_NUMBER[1]=PACKAGE_tp[0].payload[20];
        totalcount++;
    }
    else
        PDN_NUMBER[1]=0xff;
    if(PACKAGE_tp[0].payload[21]!=0xFF)
    {
        PDN_NUMBER[2]=PACKAGE_tp[0].payload[21];
        totalcount++;
    }
    else
        PDN_NUMBER[2]=0xff;
    if(PACKAGE_tp[0].payload[22]!=0xFF)
    {
        PDN_NUMBER[3]=PACKAGE_tp[0].payload[22];
        totalcount++;
    }
    else
        PDN_NUMBER[3]=0xff;
    if(PACKAGE_tp[0].payload[23]!=0xFF)
    {
        PDN_NUMBER[4]=PACKAGE_tp[0].payload[23];
        totalcount++;
    }
    else
        PDN_NUMBER[4]=0xff;

    zbchan=PACKAGE_tp[0].payload[28];
    return totalcount;

}


void startwindowbf::closeEvent(QCloseEvent *)
{
    qApp->processEvents();
    int i;

    if(portOn&&!get_opipkt_total&&(pdnshowcountp==0))
    {
        if(!firsttimer)
            this->killTimer(timeid);
        if(!firstfile)
            fclose(file);
        firstfile=true;
        firsttimer=true;
        if(portOn)
            opi_closeucd_com(&com);
        portOn=false;
        *comPortUserp = BFCOMPORTFREE;
    }
    else if(portOn&&get_opipkt_total&&(pdnshowcountp==0))
    {
        firsttimer=true;
        if(portOn)
            opi_closeucd_com(&com);
        if(!firstfile)
            fclose(file);
        firstfile=true;
        portOn=false;
        *comPortUserp = BFCOMPORTFREE;
    }

    for( i=0;i<OPIWANT_MAX_BF+1;i++)
    {  if(sdw[i]->already_open)
        {
            sdw[i]->close();
            qApp->processEvents();
        }
    }
    for( i =0;i<OPIWANT_MAX_BF+1;i++)
    {
        delete sdw[i];
        qApp->processEvents();
    }
    for( i =0;i<OPIWANT_MAX_BF+1;i++)
    {
        sdw[i] = new biofeedback(&pdnshowcountp,this);
        qApp->processEvents();
    }
}


void startwindowbf::borrowCOM(void)
{
    if(portOn)
        opi_closeucd_com(&com);
    if(!firstfile)
        fclose(file);
    firstfile=true;
    *comPortUserp = BFCOMPORTFREE;
    portOn = false;
}


void startwindowbf::returnCOM(void)
{
    on_pushButton_clicked();
}


void startwindowbf::showEvent(QShowEvent *)
{
    if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==BFCOMPORTUSER)&&(!gamemode))
    {
        if(portOn)
            opi_closeucd_com(&com);
        hide_check_pdn();
        portOn = false;
        if(opi_openucd_com(&com)==0)
        {
            portOn = true;
            *comPortUserp = BFCOMPORTUSER;
            get_opipkt_total=opiucd_getPDN();
            if(!get_opipkt_total)
            {
                show_message.setText("there is no pdn devices");
                show_message.show();
                show_message.exec();
            }
            else
                show_check_pdn();
        }
        else
        {
            portOn = false;
            *comPortUserp = BFCOMPORTFREE;
            show_message.setText("Connect the Device again");
            show_message.show();
            show_message.exec();
        }
        //if all showdatawindow are close then close the comport
        if(!sdw[0]->isVisible()&&!sdw[1]->isVisible()&&!sdw[2]->isVisible()&&!sdw[3]->isVisible()&&!sdw[4]->isVisible())
        {
            if(portOn)
                opi_closeucd_com(&com);
            if(!firstfile)
                fclose(file);
            firstfile=true;
            portOn = false;
            *comPortUserp = BFCOMPORTFREE;
        }
    }//comport free for pdc
    else if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==TGCOMPORTUSER)&&(gamemode))
    {
        if(portOn)
            opi_closeucd_com(&com);
        hide_check_pdn();
        portOn = false;
        if(opi_openucd_com(&com)==0)
        {
            portOn = true;
            *comPortUserp = TGCOMPORTUSER;
            get_opipkt_total=opiucd_getPDN();
            if(!get_opipkt_total)
            {
                show_message.setText("there is no pdn devices");
                show_message.show();
                show_message.exec();
            }
            else
                show_check_pdn();
        }
        else
        {
            portOn = false;
            *comPortUserp = BFCOMPORTFREE;
            show_message.setText("Connect the Device again");
            show_message.show();
            show_message.exec();
        }
        //if all showdatawindow are close then close the comport
        if(!sdw[0]->isVisible()&&!sdw[1]->isVisible()&&!sdw[2]->isVisible()&&!sdw[3]->isVisible()&&!sdw[4]->isVisible())
        {
            if(portOn)
                opi_closeucd_com(&com);
            if(!firstfile)
                fclose(file);
            firstfile=true;
            portOn = false;
            *comPortUserp = BFCOMPORTFREE;
        }
    }//comport free for pdc
    else
    {
        show_message.setText("Comport is busy,try again later");
        show_message.show();
        show_message.exec();
    }

}


/***
  *	Write an OPIPKT_t to file with OPI Link wrapper
  *	Inputs:
  *		pktptr, pointer to the packet
  *		fileptr, pointer to handle
  *	Returns:
  *		code:
  *			0	// successful
  *			-1	// error
  */
int startwindowbf::opipkt_put_file(OPIPKT_t* pktptr, FILE* fileptr)
{
    quint32 i;
    quint16 calcChksm=0;
    quint8 tempuint8;
    tempuint8 = SYNCBYTE;
    if(!fwrite(&tempuint8, 1, 1, fileptr)) return -1;	// sync1
    if(!fwrite(&tempuint8, 1, 1, fileptr)) return -1;	// sync2
    tempuint8 = (pktptr->length+1) >> 8;
    if(!fwrite(&tempuint8, 1, 1, fileptr)) return -1;	// length high byte
    tempuint8 = (pktptr->length+1) & 0xFF;
    if(!fwrite(&tempuint8, 1, 1, fileptr)) return -1;	// length low byte
    tempuint8 = pktptr->dataCode;
    if(!fwrite(&tempuint8, 1, 1, fileptr)) return -1;
    calcChksm += pktptr->dataCode;
    for (i = 0; i < pktptr->length; i++)
    {
        tempuint8 = pktptr->payload[i];
        if(!fwrite(&tempuint8, 1, 1, fileptr)) return -1;
        calcChksm += pktptr->payload[i];
    }
    tempuint8 = calcChksm >> 8;
    if(!fwrite(&tempuint8, 1, 1, fileptr)) return -1;	// checksum high byte
    tempuint8 = calcChksm & 0xFF;
    if(!fwrite(&tempuint8, 1, 1, fileptr)) return -1;	// checksum low byte
    return 0; // all went well
}


void startwindowbf::on_CBP1_clicked()
{
    PDNBOX[1]= !PDNBOX[1];
    if(PDNBOX[1])
    {
        pdnOn[1] = true;
        PDN_LOCATION[PDN_NUMBER[1]]=1;
        Show_count++;
    }
    else
    {
        pdnOn[1] = false;
        PDN_LOCATION[PDN_NUMBER[1]]=1;
        Show_count--;
    }
}


void startwindowbf::on_CBP2_clicked()
{
    PDNBOX[2]= !PDNBOX[2];
    if(PDNBOX[2])
    {

        pdnOn[2] = true;
        PDN_LOCATION[PDN_NUMBER[2]]=2;
        Show_count++;

    }
    else
    {
        pdnOn[2] = false;
        PDN_LOCATION[PDN_NUMBER[2]]=2;
        Show_count--;


    }
}


void startwindowbf::on_CBP3_clicked()
{
    PDNBOX[3]= !PDNBOX[3];
    if(PDNBOX[3])
    {
        pdnOn[3] = true;
        PDN_LOCATION[PDN_NUMBER[3]]=3;
        Show_count++;
    }
    else
    {
        pdnOn[3] = false;
        PDN_LOCATION[PDN_NUMBER[3]]=3;
        Show_count--;
    }
}


void startwindowbf::on_CBP4_clicked()
{
    PDNBOX[4]= !PDNBOX[4];
    if(PDNBOX[4])
    {
        pdnOn[4] = true;
        PDN_LOCATION[PDN_NUMBER[4]]=4;
        Show_count++;
    }
    else
    {
        pdnOn[4] = false;
        PDN_LOCATION[PDN_NUMBER[4]]=4;
        Show_count--;
    }
}


void startwindowbf::on_pushButton_clicked()
{
    if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==BFCOMPORTUSER)&&(!gamemode))
    {
        if(portOn)
            opi_closeucd_com(&com);
        if(!firstfile)
            fclose(file);
        firstfile=true;
        *comPortUserp = BFCOMPORTFREE;
        hide_check_pdn();
        portOn = false;
        if(opi_openucd_com(&com)==0)
        {
            portOn = true;
            *comPortUserp = BFCOMPORTUSER;
            get_opipkt_total=opiucd_getPDN();
            if(!get_opipkt_total)
            {
                show_message.setText("there is no pdn devices");
                show_message.show();
                show_message.exec();
            }
            else
                show_check_pdn();
            //after open comport if no pdn is showing then close the comport
            if(pdnshowcountp==0)
            {
                if(portOn)
                    opi_closeucd_com(&com);
                portOn=false;
                *comPortUserp = BFCOMPORTFREE;
                if(!firsttimer)
                    this->killTimer(timeid);
                if(!firstfile)
                    fclose(file);
                firsttimer=true;
                firstfile=true;
            }
        }
        else
        {
            portOn = false;
            *comPortUserp = BFCOMPORTFREE;
            show_message.setText("Connect the Device again");
            show_message.show();
            show_message.exec();
        }
    }//other device doesn't use comport
    else if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==TGCOMPORTUSER)&&(gamemode))
    {
        if(portOn)
            opi_closeucd_com(&com);
        if(!firstfile)
            fclose(file);
        firstfile=true;
        *comPortUserp = BFCOMPORTFREE;
        hide_check_pdn();
        portOn = false;
        if(opi_openucd_com(&com)==0)
        {
            portOn = true;
            *comPortUserp = TGCOMPORTUSER;
            get_opipkt_total=opiucd_getPDN();
            if(!get_opipkt_total)
            {
                show_message.setText("there is no pdn devices");
                show_message.show();
                show_message.exec();
            }
            else
                show_check_pdn();
            //after open comport if no pdn is showing then close the comport
            if(pdnshowcountp==0)
            {
                if(portOn)
                    opi_closeucd_com(&com);
                portOn=false;
                *comPortUserp = BFCOMPORTFREE;
                if(!firsttimer)
                    this->killTimer(timeid);
                if(!firstfile)
                    fclose(file);
                firsttimer=true;
                firstfile=true;
            }
        }
        else
        {
            portOn = false;
            *comPortUserp = BFCOMPORTFREE;
            show_message.setText("Connect the Device again");
            show_message.show();
            show_message.exec();
        }
    }//other device doesn't use comport
    else //other device uses comport
    {
        show_message.setText("Comport is busy,try again later");
        show_message.show();
        show_message.exec();
    }
}


void startwindowbf::on_pushButton_2_clicked()
{
    OPIPKT_t opipkttmp;
    quint8 tempui8arr[512];
    int i;

    //re open the comport
    if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==BFCOMPORTUSER)&&(!gamemode))
    {
        if(portOn)
            opi_closeucd_com(&com);
        if(!firstfile)
            fclose(file);
        firstfile=true;
        *comPortUserp = BFCOMPORTFREE;
        hide_check_pdn();
        portOn = false;
        if(opi_openucd_com(&com)==0)
        {
            portOn = true;
            *comPortUserp = BFCOMPORTUSER;
            get_opipkt_total=opiucd_getPDN();
            if(!get_opipkt_total)
            {
                show_message.setText("there is no pdn devices");
                show_message.show();
                show_message.exec();
                return ;
            }
            else
                show_check_pdn();
        }
        else
        {
            portOn = false;
            *comPortUserp = BFCOMPORTFREE;
            show_message.setText("Connect the Device again");
            show_message.show();
            show_message.exec();
            return ;
        }
    }//other device doesnt use comport
    else if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==TGCOMPORTUSER)&&(gamemode))
    {
        if(portOn)
            opi_closeucd_com(&com);
        if(!firstfile)
            fclose(file);
        firstfile=true;
        *comPortUserp = BFCOMPORTFREE;
        hide_check_pdn();
        portOn = false;
        if(opi_openucd_com(&com)==0)
        {
            portOn = true;
            *comPortUserp = TGCOMPORTUSER;
            get_opipkt_total=opiucd_getPDN();
            if(!get_opipkt_total)
            {
                show_message.setText("there is no pdn devices");
                show_message.show();
                show_message.exec();
                return ;
            }
            else
                show_check_pdn();
        }
        else
        {
            portOn = false;
            *comPortUserp = BFCOMPORTFREE;
            show_message.setText("Connect the Device again");
            show_message.show();
            show_message.exec();
            return ;
        }
    }//other device doesnt use comport
    else
    {
        show_message.setText("Comport is busy,try again later");
        show_message.show();
        show_message.exec();
        return ;
    }

    //re open the comport end
    if(!(portOn))
    {
        *comPortUserp = BFCOMPORTFREE;
        show_message.setText("You have to connect a Device");
        show_message.show();
        show_message.exec();
        return ;
    }
    else if(Show_count==0)
    {
        show_message.setText("You have to choose Devices");
        show_message.show();
        show_message.exec();
        return ;
    }
    else
    {
        if(!gamemode)
            *comPortUserp = BFCOMPORTUSER;
        else
            *comPortUserp = TGCOMPORTUSER;
        //if user show the showdatawindow,user can't control whether write file or not
        ui->checkWriteFileBox->setDisabled(true);
        if(!firstfile)
            fclose(file);
        //regulatefilename = userfilename;
        opiucd_status(&com, &opipkttmp);    // should have opened device successfully before
        QDateTime stDT = QDateTime::currentDateTime();
        regulatefilename = QString("D%1_ALL").arg(stDT.toString("yyyyMMdd_hhmmss"));
        if(ui->checkWriteFileBox->isChecked()) //if true then write it
        {
            firstfile=false;
            file = fopen(regulatefilename.append(".opi").toLatin1().data(), "wb+");
            // write opihdr information
            fwrite(opipkttmp.payload, 1, OPIUCDSTLEN-1, file);
            for(i = 0; i < (512-(OPIUCDSTLEN-1)); i++) tempui8arr[i] = 0xFF;
            fwrite(tempui8arr, 1, (512-(OPIUCDSTLEN-1)), file);
        }
        else
            firstfile=true;
        //check which the pdn the user what to show
        for(i=1;i<OPIWANT_MAX_BF+1;i++)
        {
            if(pdnOn[i])  //if the user choose the pdn then we open the showdatawindow
            {
                qDebug()<<"sdw "<<i<<"= "<<sdw[i];
                if(!sdw[i]->already_open)  //if already open we will not open and reset again
                {
                    firstpdn[i]=false;
                    sdw[i]->reset(gamemode);
                    sdw[i]->setPdnNum(PDN_NUMBER[i]);
                    if(gamemode)
                    {
                        sdw[i]->showgamewindow(PDN_NUMBER[i]);
                        sdw[i]->secretstart(gamemode);
                    }
                    else
                        sdw[i]->show();
                }
            }
            else  //if the user don't choose the pdn then we close the showdatawindow
            {
                if(sdw[i]->isVisible())
                {
                    sdw[i]->close();
                    sdw[i]->already_open=false;
                }
            }
        }
        //to avoid reopening timer
        if(firsttimer)
        {
            timeid=QObject::startTimer(startwindow_fresh_time_BF);
            firsttimer=false;
        }
    }
}
