#include "startwindowbf.h"
#include "ui_startwindowbf.h"

#define fclose(fp)  ((fp) ? fclose(fp) : 0, (fp) = 0)

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

	timeid = 0;

	for(int i=0;i<=255;i++)
		PDN_LOCATION[i]=0;

	for(int i=0;i<OPIWANT_MAX_BF+1;i++)
	{ 
		PDNBOX[i]=false;
		pdnOn[i]=false;
		PDN_NUMBER[i]=0xff;
	}

	//initialize the pdn is first used
	//so that if it is the second time we use we will get the information from showdatawindow
	for(int i=0;i<5;i++)
		firstpdn[i]=true;
	
	com = 0;

	//set window icon
	this->setWindowIcon(QIcon("../images/opi_ico.ico"));
	ui->btnRefresh->setEnabled(true);

	opiFile = NULL;
	edfFile = NULL;

	edfFileHandle = -1;
}

startwindowbf::~startwindowbf()
{
	delete ui;
	for(int i =0;i<OPIWANT_MAX_BF+1;i++)
		delete sdw[i];
}

void startwindowbf::timerEvent(QTimerEvent *event)
{   
	if((com > 0)&&(pdnshowcountp>0))
	{
		fresh();
	}
	else if(pdnshowcountp==0)
	{
		closeComPort();

		killUIRefreshTimer();

		closeFiles();

		ui->chkSaveToOPI->setDisabled(false);
		ui->chkSaveToEDF->setDisabled(false);
	}
}


// get new data from the sensor
int startwindowbf::fresh()
{
	OPIPKT_t receivedPackage = {};	// Data Code 0x01, 0x10 or 0x40

	if((opiucd_getwltsdata(&com,&receivedPackage) == NEWDATAGETCODE_BF)) //continue if the Data Code is 0x01 (Interpreted/Fixed Received Wireless TrueSense Data)
	{
		OPIPKT_DC01_SDC01_t trueSenseData = {};
		trueSenseData = buildDC01SDC01(receivedPackage);

		if (trueSenseData.wirelessDataCorrectionCode < 3) //check quality 0=clean, 1=0fix, 2=<20fix, 3=>20fix
		{
			int freshPDNIndex = PDN_LOCATION[trueSenseData.sensorPDN];
			if(trueSenseData.sensorPDN == sdw[freshPDNIndex]->PDN_NUMBER)
			{
				if(ui->chkSaveToOPI->isChecked())
					opipkt_put_file(&receivedPackage, opiFile);   // write to OPI file

				if(ui->chkSaveToEDF->isChecked())
					writeDataToEDFFile(trueSenseData, edfFileHandle);   // write to EDF file
				
				// update the UI controls
				sdw[freshPDNIndex]->getStruct(trueSenseData);
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
		// this package was not a TrueSense Data package os just skip it
		return 0;
	}

}



void startwindowbf::hide_check_pdn()
{
	ui->btnCBP1->hide();
	ui->btnCBP2->hide();
	ui->btnCBP3->hide();
	ui->btnCBP4->hide();
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
				ui->btnCBP1->show();
				pdntemp.sprintf("[%d]",PDN_NUMBER[1]);
				ui->btnCBP1->setText(pdntemp);
				break;
			case 2:
				ui->btnCBP2->show();
				pdntemp.sprintf("[%d]",PDN_NUMBER[2]);
				ui->btnCBP2->setText(pdntemp);
				break;
			case 3:
				ui->btnCBP3->show();
				pdntemp.sprintf("[%d]",PDN_NUMBER[3]);
				ui->btnCBP3->setText(pdntemp);
				break;
			case 4:
				ui->btnCBP4->show();
				pdntemp.sprintf("[%d]",PDN_NUMBER[4]);
				ui->btnCBP4->setText(pdntemp);
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

	OPIPKT_t statusInfoPackage = {};	// Data Code 0x10

	opiucd_status(&com,&statusInfoPackage);	// returns UCD Status Info of slave


	for(i=0;i<5;i++)
		ucd[i]= statusInfoPackage.payload[i];

	if(statusInfoPackage.payload[20]!=0xFF)
	{
		PDN_NUMBER[1]=statusInfoPackage.payload[20];
		totalcount++;
	}
	else
		PDN_NUMBER[1]=0xff;
	if(statusInfoPackage.payload[21]!=0xFF)
	{
		PDN_NUMBER[2]=statusInfoPackage.payload[21];
		totalcount++;
	}
	else
		PDN_NUMBER[2]=0xff;
	if(statusInfoPackage.payload[22]!=0xFF)
	{
		PDN_NUMBER[3]=statusInfoPackage.payload[22];
		totalcount++;
	}
	else
		PDN_NUMBER[3]=0xff;
	if(statusInfoPackage.payload[23]!=0xFF)
	{
		PDN_NUMBER[4]=statusInfoPackage.payload[23];
		totalcount++;
	}
	else
		PDN_NUMBER[4]=0xff;

	zbchan=statusInfoPackage.payload[28];
	return totalcount;

}


void startwindowbf::closeEvent(QCloseEvent *)
{
	qApp->processEvents();
	int i;

	if((com > 0)&&!get_opipkt_total&&(pdnshowcountp==0))
	{
		killUIRefreshTimer();

		closeFiles();

		closeComPort();
	}
	else if((com > 0)&&get_opipkt_total&&(pdnshowcountp==0))
	{
		//firsttimer=true;
		
		closeComPort();

		closeFiles();
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
	closeComPort();

	closeFiles();
}


void startwindowbf::returnCOM(void)
{
	on_btnRefresh_clicked();
}


void startwindowbf::showEvent(QShowEvent *)
{
	if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==BFCOMPORTUSER)&&(!gamemode))
	{
		closeComPort();

		hide_check_pdn();

		if(opi_openucd_com(&com)==0)
		{
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
			*comPortUserp = BFCOMPORTFREE;

			show_message.setText("Connect the Device again");
			show_message.show();
			show_message.exec();
		}
		//if all showdatawindow are close then close the comport
		if(!sdw[0]->isVisible()&&!sdw[1]->isVisible()&&!sdw[2]->isVisible()&&!sdw[3]->isVisible()&&!sdw[4]->isVisible())
		{
			closeComPort();

			closeFiles();
		}
	}//comport free for pdc
	else if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==TGCOMPORTUSER)&&(gamemode))
	{
		closeComPort();

		hide_check_pdn();

		if(opi_openucd_com(&com)==0)
		{
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
			closeComPort();

			show_message.setText("Connect the Device again");
			show_message.show();
			show_message.exec();
		}
		//if all showdatawindow are close then close the comport
		if(!sdw[0]->isVisible()&&!sdw[1]->isVisible()&&!sdw[2]->isVisible()&&!sdw[3]->isVisible()&&!sdw[4]->isVisible())
		{
			closeComPort();

			closeFiles();
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


void startwindowbf::on_btnCBP1_clicked()
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


void startwindowbf::on_btnCBP2_clicked()
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


void startwindowbf::on_btnCBP3_clicked()
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


void startwindowbf::on_btnCBP4_clicked()
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


void startwindowbf::on_btnRefresh_clicked()
{
	if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==BFCOMPORTUSER)&&(!gamemode))
	{
		closeComPort();
			
		closeFiles();
		
		hide_check_pdn();
		
		if(opi_openucd_com(&com)==0)
		{
			*comPortUserp = BFCOMPORTUSER;

			get_opipkt_total=opiucd_getPDN();
			if(!get_opipkt_total)
			{
				show_message.setText("there are no PDN devices");
				show_message.show();
				show_message.exec();
			}
			else
				show_check_pdn();
			//after open comport if no pdn is showing then close the comport
			if(pdnshowcountp==0)
			{
				closeComPort();
				
				killUIRefreshTimer();
				
				closeFiles();
			}
		}
		else
		{
			closeComPort();

			show_message.setText("Connect the device again");
			show_message.show();
			show_message.exec();
		}
	}//other device doesn't use comport
	else if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==TGCOMPORTUSER)&&(gamemode))
	{
		closeComPort();

		closeFiles();

		hide_check_pdn();
		
		if(opi_openucd_com(&com)==0)
		{
			*comPortUserp = TGCOMPORTUSER;
			get_opipkt_total=opiucd_getPDN();
			if(!get_opipkt_total)
			{
				show_message.setText("there are no PDN devices");
				show_message.show();
				show_message.exec();
			}
			else
				show_check_pdn();
			//after open comport if no pdn is showing then close the comport
			if(pdnshowcountp==0)
			{
				closeComPort();
				
				killUIRefreshTimer();

				closeFiles();
			}
		}
		else
		{
			closeComPort();

			show_message.setText("Connect the device again");
			show_message.show();
			show_message.exec();
		}
	}//other device doesn't use comport
	else //other device uses comport
	{
		show_message.setText("COM port is busy, try again later");
		show_message.show();
		show_message.exec();
	}
}


void startwindowbf::on_btnStart_clicked()
{
	//re open the comport
	if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==BFCOMPORTUSER)&&(!gamemode))
	{
		closeFiles();

		closeComPort();
		
		hide_check_pdn();
		
		if(opi_openucd_com(&com)==0)
		{
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
			closeComPort();

			show_message.setText("Connect the Device again");
			show_message.show();
			show_message.exec();
			return ;
		}
	}//other device doesnt use comport
	else if((*comPortUserp==BFCOMPORTFREE||*comPortUserp==TGCOMPORTUSER)&&(gamemode))
	{
		closeFiles();

		closeComPort();

		hide_check_pdn();

		if(opi_openucd_com(&com)==0)
		{
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
			closeComPort();

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

	if (com == 0)
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

		//if user opened the showdatawindow, then he can't control whether write file or not
		ui->chkSaveToOPI->setDisabled(true);
		ui->chkSaveToEDF->setDisabled(true);
		
		//regulatefilename = userfilename;
		OPIPKT_t statusInfoPackage = {};
		opiucd_status(&com, &statusInfoPackage);    // get the status of the sensor

		QDateTime stDT = QDateTime::currentDateTime();

		closeFiles();
		if(ui->chkSaveToOPI->isChecked()) //if true then write it
		{
			opiFile = fopen(QString("D%1_ALL").arg(stDT.toString("yyyyMMdd_hhmmss")).append(".opi").toLatin1().data(), "wb+");
			
			// write OPI header which was received from the controller (127 bytes)
			fwrite(statusInfoPackage.payload, 1, OPIUCDSTLEN-1, opiFile);
			
			// the OPI header must be 512 bytes long, so we will the remaining part with 0xFF
			quint8 tempui8arr[512];
			for(int i = 0; i < (512-(OPIUCDSTLEN-1)); i++) 
			{
				tempui8arr[i] = 0xFF;
			}
			fwrite(tempui8arr, 1, (512-(OPIUCDSTLEN-1)), opiFile);
		}

		if(ui->chkSaveToEDF->isChecked()) //if true then write it
		{
			OPIPKT_DC10_t status = buildDC10(statusInfoPackage);

			edfFileHandle = edfopen_file_writeonly(QString("D%1_ALL").arg(stDT.toString("yyyyMMdd_hhmmss")).append(".edf").toLatin1().data(), EDFLIB_FILETYPE_EDFPLUS, 8);	// ADC, AccelX, AccelY, AccelZ, Temperature, ED, Data Correction Type, Low Battery, (EDF Annotations)
			edf_set_datarecord_duration(edfFileHandle, 12500);		// one data record represents 1/8 seconds

			edf_set_label(edfFileHandle, 0, "ADC");
			edf_set_samplefrequency(edfFileHandle, 0, 512 / 8);		// we must set the sample frequency per data record
			edf_set_digital_minimum(edfFileHandle, 0, -20480);
			edf_set_digital_maximum(edfFileHandle, 0, +20480);
			//edf_set_digital_minimum(edfFileHandle, 0, -8192);
			//edf_set_digital_maximum(edfFileHandle, 0, +8191);
			edf_set_physical_minimum(edfFileHandle, 0, -800);
			edf_set_physical_maximum(edfFileHandle, 0, +800);
			edf_set_physical_dimension(edfFileHandle, 0, "uV");

			edf_set_label(edfFileHandle, 1, "Accelerometer X");
			edf_set_samplefrequency(edfFileHandle, 1, 8 / 8);
			edf_set_digital_minimum(edfFileHandle, 1, -32768);
			edf_set_digital_maximum(edfFileHandle, 1, +32767);
			//edf_set_digital_minimum(edfFileHandle, 1, -128);
			//edf_set_digital_maximum(edfFileHandle, 1, +127);
			edf_set_physical_minimum(edfFileHandle, 1, -2);
			edf_set_physical_maximum(edfFileHandle, 1, +2);
			edf_set_physical_dimension(edfFileHandle, 1, "g");

			edf_set_label(edfFileHandle, 2, "Accelerometer Y");
			edf_set_samplefrequency(edfFileHandle, 2, 8 / 8);
			edf_set_digital_minimum(edfFileHandle, 2, -32768);
			edf_set_digital_maximum(edfFileHandle, 2, +32767);
			//edf_set_digital_minimum(edfFileHandle, 2, -128);
			//edf_set_digital_maximum(edfFileHandle, 2, +127);
			edf_set_physical_minimum(edfFileHandle, 2, -2);
			edf_set_physical_maximum(edfFileHandle, 2, +2);
			edf_set_physical_dimension(edfFileHandle, 2, "g");

			edf_set_label(edfFileHandle, 3, "Accelerometer Z");
			edf_set_samplefrequency(edfFileHandle, 3, 32 / 8);
			edf_set_digital_minimum(edfFileHandle, 3, -32768);
			edf_set_digital_maximum(edfFileHandle, 3, +32767);
			//edf_set_digital_minimum(edfFileHandle, 3, -127);
			//edf_set_digital_maximum(edfFileHandle, 3, +128);
			edf_set_physical_minimum(edfFileHandle, 3, -2);
			edf_set_physical_maximum(edfFileHandle, 3, +2);
			edf_set_physical_dimension(edfFileHandle, 3, "g");

			edf_set_label(edfFileHandle, 4, "Temperature");
			edf_set_samplefrequency(edfFileHandle, 4, 8 / 8);
			//edf_set_digital_minimum(edfFileHandle, 4,  0);
			//edf_set_digital_maximum(edfFileHandle, 4, +4080);
			edf_set_digital_minimum(edfFileHandle, 4,  -470);
			edf_set_digital_maximum(edfFileHandle, 4, +2410);
			edf_set_physical_minimum(edfFileHandle, 4,  -47);
			edf_set_physical_maximum(edfFileHandle, 4, +241);
			edf_set_physical_dimension(edfFileHandle, 4, "degreeC");

			edf_set_label(edfFileHandle, 5, "Wireless Signal Strength");
			edf_set_samplefrequency(edfFileHandle, 5, 8 / 8);
			//edf_set_digital_minimum(edfFileHandle, 5, -32768);
			//edf_set_digital_maximum(edfFileHandle, 5, +32767);
			edf_set_digital_minimum(edfFileHandle, 5,  0);
			edf_set_digital_maximum(edfFileHandle, 5, +84);
			//edf_set_physical_minimum(edfFileHandle, 5, -50);
			//edf_set_physical_maximum(edfFileHandle, 5, +50);
			edf_set_physical_minimum(edfFileHandle, 5,  0);
			edf_set_physical_maximum(edfFileHandle, 5, +84);
			edf_set_physical_dimension(edfFileHandle, 5, "dB");

			edf_set_label(edfFileHandle, 6, "Data Correction Type");
			edf_set_samplefrequency(edfFileHandle, 6, 8 / 8);
			edf_set_digital_minimum(edfFileHandle, 6,  0);
			edf_set_digital_maximum(edfFileHandle, 6, +3);
			edf_set_physical_minimum(edfFileHandle, 6,  0);
			edf_set_physical_maximum(edfFileHandle, 6, +3);
			edf_set_physical_dimension(edfFileHandle, 6, "");

			edf_set_label(edfFileHandle, 7, "Low Battery");
			edf_set_samplefrequency(edfFileHandle, 7, 8 / 8);
			edf_set_digital_minimum(edfFileHandle, 7,  0);
			edf_set_digital_maximum(edfFileHandle, 7, +1);
			edf_set_physical_minimum(edfFileHandle, 7,  0);
			edf_set_physical_maximum(edfFileHandle, 7, +1);
			edf_set_physical_dimension(edfFileHandle, 7, "");

			//edf_set_label(edfFileHandle, 8, "Annotations");
			//edf_set_samplefrequency(edfFileHandle, 8, 8 / 8);
			//edf_set_digital_minimum(edfFileHandle, 8,  0);
			//edf_set_digital_maximum(edfFileHandle, 8, +1);
			//edf_set_physical_minimum(edfFileHandle, 8, 0);
			//edf_set_physical_maximum(edfFileHandle, 8, 0);
			//edf_set_physical_dimension(edfFileHandle, 8, "");

			QDateTime currentTime = QDateTime::currentDateTime();
			edf_set_startdatetime(edfFileHandle, currentTime.date().year(), currentTime.date().month(), currentTime.date().day(), currentTime.time().hour(), currentTime.time().minute(), currentTime.time().second());
		}

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
		if(!timeid)	timeid=QObject::startTimer(startwindow_fresh_time_BF);
	}
}

void startwindowbf::killUIRefreshTimer()
{
	if(timeid) this->killTimer(timeid);

	timeid = 0;
}

void startwindowbf::closeComPort()
{
	if (com) opi_closeucd_com(&com);
	*comPortUserp = BFCOMPORTFREE;
	com = 0;
}

void startwindowbf::closeFiles()
{
	fclose(opiFile);
	fclose(edfFile);

	if (edfFileHandle >= 0)
	{
		edfclose_file(edfFileHandle);
		edfFileHandle = -1;
	}
}

void startwindowbf::writeDataToEDFFile(OPIPKT_DC01_SDC01_t package, int edfFileHandle)
{
	int* buffer = (int*)malloc(sizeof(int) * 74);

	for (int i=0; i<package.adcDataSampleCount; i++)
	{
		buffer[i] = package.adcValues[i];
	}

	buffer[package.adcDataSampleCount+0] = package.accelerometerX;
	buffer[package.adcDataSampleCount+1] = package.accelerometerY;
	
	for (int i=0; i<4; i++)
	{
		buffer[package.adcDataSampleCount+2+i] = package.accelerometerZs[i];
	}

	buffer[package.adcDataSampleCount+6] = (int)(package.temperatureData*10);
	buffer[package.adcDataSampleCount+7] = package.ed;
	buffer[package.adcDataSampleCount+8] = package.wirelessDataCorrectionCode;
	buffer[package.adcDataSampleCount+9] = package.lowBattery;

	edf_blockwrite_digital_samples(edfFileHandle, buffer);

	free(buffer);
}
