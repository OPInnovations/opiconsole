#include "configwindow.h"
#include "ui_configwindow.h"
#include "opi_helper.h"

ConfigWindow::ConfigWindow(QWidget *parent) :
  //  QMainWindow(parent),
    ui(new Ui::ConfigWindow)
{
    ui->setupUi(this);
}


ConfigWindow::~ConfigWindow()
{
    delete ui;
}


void ConfigWindow::ErrorMsgBox(QString errMsg)
{
    QMessageBox msgBox;

    msgBox.setWindowTitle("Error");
    msgBox.setText(errMsg);
    msgBox.setStyleSheet("background-image: url(:/images/images/bioshare-BGtiletexturedMixedGloss600px.jpg);");
    msgBox.setStandardButtons(QMessageBox::Ok);
    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Error: %2").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")).arg(errMsg));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    msgBox.exec();
}


void ConfigWindow::on_ucStatusPB_clicked()
{
    HANDLE comport;
    OPIPKT_t opipkt;
    qint64 ucdsn, ucCurrTS, ucRefEpochMSecs;
    qint32 ucfwv;
    qint16 ucMode, pdnList[PDNLISTLEN], uczbChan, ucusdStatus, ucChgStat;
    qint32 i, pdnCt;
    QString tempstr;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if(opiucd_status(&comport, &opipkt))
    {
        ErrorMsgBox(QString("Could not get UC status"));
        opi_closeucd_com(&comport);
        return;
    }

    ucdsn = ((qint64) opipkt.payload[0] << 32) + ((qint64) opipkt.payload[1] << 24)
            + ((qint64) opipkt.payload[2] << 16) + ((qint64) opipkt.payload[3] << 8)
            + ((qint64) opipkt.payload[4]);
    ucCurrTS = ((qint64) opipkt.payload[5] << 40) + ((qint64) opipkt.payload[6] << 32) +
            ((qint64) opipkt.payload[7] << 24) + ((qint64) opipkt.payload[8] << 16) +
            ((qint64) opipkt.payload[9] << 8) + ((qint64) opipkt.payload[10]);
    ucRefEpochMSecs = ucCurrTS*1000/UCRTCFREQ;
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    QDateTime ucDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(ucRefEpochMSecs);
    ucfwv = (opipkt.payload[DSNLEN+TSLEN+6] << 8) + opipkt.payload[DSNLEN+TSLEN+6+1];
    ucMode = opipkt.payload[DSNLEN+TSLEN+6+FWVLEN];
    for (i = 0; i < PDNLISTLEN; i++)
        pdnList[i] = opipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];
    uczbChan = opipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN];
    ucusdStatus = opipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+1];
    ucChgStat = opipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+2];

    if(ucusdStatus & 0x04) ui->ucModOnOffCB->setCurrentIndex(0);
    else ui->ucModOnOffCB->setCurrentIndex(1);

    ui->msgPTE->appendPlainText(QString(">> UC Status"));
    ui->msgPTE->appendPlainText(QString("Device ShortID: %1   Firmware Version: %2   Time: %3   ZigBee Channel: %4").arg(QChar(opipkt.payload[4]%26 + 65)).arg(ucfwv).arg(ucDT.toString("yyyyMMdd hhmmss")).arg(uczbChan));
    if(ucusdStatus & 0x04) ui->msgPTE->appendPlainText("uSD sensor on");
    else ui->msgPTE->appendPlainText("uSD sensor off");
    if((ucusdStatus & 0x03) == 0) ui->msgPTE->appendPlainText("uSD Slot: Nothing");
    else if((ucusdStatus & 0x03) == 1) ui->msgPTE->appendPlainText("uSD Slot: TS only");
    else if((ucusdStatus & 0x03) == 2) ui->msgPTE->appendPlainText("uSD Slot: MM only");
    else if((ucusdStatus & 0x03) == 3) ui->msgPTE->appendPlainText("uSD Slot: TS & MM");
    if(ucChgStat & 0x30) ui->msgPTE->appendPlainText("Controller is in On Mode");
    else ui->msgPTE->appendPlainText("Controller is in Off Mode");
    if(ucusdStatus & 0x10) ui->msgPTE->appendPlainText("USD SPI is on");
    else ui->msgPTE->appendPlainText("USD SPI is off");
    pdnCt = 0;
    for(i = 0; i < PDNLISTLEN; i++)
    {
        if(pdnList[i] != 0xFF)
        {
            tempstr.append(QString("Slot %1-#%2, ").arg(i+1).arg(pdnList[i]));
            pdnCt++;
        }
    }
    if(!pdnCt) tempstr = QString("No paired devices");
    else tempstr.prepend(QString("%1 Associated PDNs: ").arg(pdnCt));
    ui->msgPTE->appendPlainText(tempstr);

    // update the current ZigBee Channel
    ui->ucZBValCB->setCurrentIndex(uczbChan-11);

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Unified Controller Status").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_ucModOnOffCB_activated(int index)
{
    HANDLE comport;

    if(opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();

    if(index == 0)
    {
        if(opiucd_turnmodon(&comport))
        {
            ErrorMsgBox(QString("Could not turn module on"));
            opi_closeucd_com(&comport);
            return;
        }
        ui->msgPTE->appendPlainText(QString("<< %1 >> Turned module on").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));

    }
    else
    {
        if(opiucd_turnmodoff(&comport))
        {
            ErrorMsgBox(QString("Could not turn module off"));
            opi_closeucd_com(&comport);
            return;
        }
        ui->msgPTE->appendPlainText(QString("<< %1 >> Turned module off").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    }

    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_ucSetTimePB_clicked()
{
    HANDLE comport;

    if(opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if(setUCTime(&comport))
    {
        ErrorMsgBox(QString("Could not set UC time"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Set UC time").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_ucWLMeasurePB_clicked()
{
    HANDLE comport;
    OPIPKT_t ucOpipkt;
    qint32 result[16];
    QString resTopLine, resBotLine;
    int i, uczbChan;
    QProgressDialog progQPD("Measuring ZigBee Channels", QString(), 0, 15);

    progQPD.setWindowModality(Qt::WindowModal);
    progQPD.setMinimumDuration(100);

    if(opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        ErrorMsgBox(QString("Couldn't get UC status"));
        opi_closeucd_com(&comport);
        return;
    }
    // save current channel
    uczbChan = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN];
    ui->msgPTE->appendPlainText(QString("Original ZigBee Channel: %1").arg(uczbChan));
    ui->msgPTE->appendPlainText("Measuring all zigbee channels, please wait");
    qApp->processEvents();

    // Go through each valid channel and get maximum out of 100 tries
    for(i = 0; i < 16; i++)
    {
        if (opiucd_setzbchan(&comport, i+11))
        {
            ErrorMsgBox(QString("Could not set UC ZigBee Channel"));
            opi_closeucd_com(&comport);
            return;
        }
        result[i] = maxWLMeasure100(&comport);
        progQPD.setValue(i);
        qApp->processEvents();
    }

    resTopLine.append("ZigBee Channel: 11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26");
    resBotLine.append("Maximum ED     : ");
    for(i = 0; i < 16; i++) resBotLine.append(QString("%1  ").arg(result[i],2,10,QChar('0')));

    ui->msgPTE->appendPlainText(resTopLine);
    ui->msgPTE->appendPlainText(resBotLine);

    // set original zb channel
    if (opiucd_setzbchan(&comport, uczbChan))
    {
        ErrorMsgBox(QString("Could not set original UC ZigBee Channel"));
        opi_closeucd_com(&comport);
        return;
    }

    opi_closeucd_com(&comport);
    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Wireless Measurements done").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
}


void ConfigWindow::on_ucSetZBPB_clicked()
{
    HANDLE comport;
    int zbChan;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    switch(ui->ucZBValCB->currentIndex())
    {
    case 0:
        zbChan = 11;
        break;
    case 1:
        zbChan = 12;
        break;
    case 2:
        zbChan = 13;
        break;
    case 3:
        zbChan = 14;
        break;
    case 4:
        zbChan = 15;
        break;
    case 5:
        zbChan = 16;
        break;
    case 6:
        zbChan = 17;
        break;
    case 7:
        zbChan = 18;
        break;
    case 8:
        zbChan = 19;
        break;
    case 9:
        zbChan = 20;
        break;
    case 10:
        zbChan = 21;
        break;
    case 11:
        zbChan = 22;
        break;
    case 12:
        zbChan = 23;
        break;
    case 13:
        zbChan = 24;
        break;
    case 14:
        zbChan = 25;
        break;
    case 15:
        zbChan = 26;
        break;
    }

    if (opiucd_setzbchan(&comport, zbChan))
    {
        ErrorMsgBox(QString("Could not set UC ZigBee Channel"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Set UC ZigBee Channel").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_ucReadEventsPB_clicked()
{
    HANDLE comport;
    int eventCt;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    eventCt = readUCEvents(&comport);
    if(eventCt < 0)
    {
        ErrorMsgBox(QString("Error reading out Events to file"));
        opi_closeucd_com(&comport);
        return;
    }
    if(eventCt == 0) ui->msgPTE->appendPlainText(QString("No Events"));
    else ui->msgPTE->appendPlainText(QString("%1 events read out").arg(eventCt));

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Read Events").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_ucEraseEventsPB_clicked()
{
    HANDLE comport;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if (opiucd_evcaperase(&comport))
    {
        ErrorMsgBox(QString("Could not erase captured events"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Erased Events").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_ucCopySensPB_clicked()
{
    HANDLE comport;
    int pdnSlot;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    pdnSlot = ui->ucSensValCB->currentIndex();
    if (opiucd_copytssettings(&comport, pdnSlot))
    {
        ErrorMsgBox(QString("Could not remember module settings"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Remembered Module Settings to UC").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_ucForgetSensPB_clicked()
{
    HANDLE comport;
    int pdnSlot;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    pdnSlot = ui->ucSensValCB->currentIndex();
    if (opiucd_forgettssettings(&comport, pdnSlot))
    {
        ErrorMsgBox(QString("Could not forget module settings"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Forgot Module Settings in UC").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_tsStatusPB_clicked()
{
    HANDLE comport;
    OPIPKT_t opipkt;
    qint64 moddsn, modrtc, modrtcSet, modRefEpochMSecs;
    qint32 modfwv;
    qint16 modpdn, modzbChan, modRFMode, modRFTxPwr, modMMWrite, modRFTxTimeout;
    QString tempstr;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if(opiucd_tsstatus(&comport, &opipkt))
    {
        ErrorMsgBox(QString("Could not get TrueSense status"));
        opi_closeucd_com(&comport);
        return;
    }

    moddsn = ((qint64) opipkt.payload[1] << 32) + ((qint64)opipkt.payload[2] << 24)
            + (opipkt.payload[3] << 16) + (opipkt.payload[4] << 8) + opipkt.payload[5];
    modrtc = ((qint64) opipkt.payload[1+DSNLEN] << 32) + ((qint64) opipkt.payload[1+DSNLEN+1] << 24)
            + (opipkt.payload[1+DSNLEN+2] << 16) + (opipkt.payload[1+DSNLEN+3] << 8) + opipkt.payload[1+DSNLEN+4];
    modrtcSet = modrtc >> 39;
    if (modrtcSet) modRefEpochMSecs = (modrtc - (((qint64) 1) << 39))*64*1000/32768;
    else modRefEpochMSecs = modrtc*64*1000/32768;
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    QDateTime modDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(modRefEpochMSecs);

    modfwv = (opipkt.payload[1+DSNLEN+5] << 8) + opipkt.payload[1+DSNLEN+5+1];
    modpdn = opipkt.payload[1+DSNLEN+5+FWVLEN];
    modzbChan = opipkt.payload[1+DSNLEN+5+FWVLEN+1];
    modRFMode = opipkt.payload[1+DSNLEN+5+FWVLEN+2];
    modRFTxPwr = opipkt.payload[1+DSNLEN+5+FWVLEN+3];
    modMMWrite = opipkt.payload[1+DSNLEN+5+FWVLEN+4];
    modRFTxTimeout = opipkt.payload[1+DSNLEN+5+FWVLEN+5];

    ui->msgPTE->appendPlainText(QString(">> TrueSense Status"));
    ui->msgPTE->appendPlainText(QString("Paired Device Number: %1   Firmware Version: %2   Time: %3   ZigBee Channel: %4").arg(modpdn).arg(modfwv).arg(modDT.toString("yyyyMMdd hhmmss")).arg(modzbChan));
    ui->tsPDNValLE->setText(QString("%1").arg(modpdn));
    ui->tsZBValCB->setCurrentIndex(modzbChan-11);
    if(modRFMode == 0)
    {
        tempstr.append("RF Always Off");
        ui->tsRFModeValCB->setCurrentIndex(0);
        ui->tsRFTOValCB->setCurrentIndex(modRFTxTimeout);
    }
    else if(modRFMode == 1)
    {
        tempstr.append("RF Always On");
        ui->tsRFModeValCB->setCurrentIndex(1);
        ui->tsRFTOValCB->setCurrentIndex(modRFTxTimeout);
    }
    else if(modRFMode == 2)
    {
        if(opiucd_settsrfmode(&comport, 3))
        {
            ErrorMsgBox(QString("Failed to set RF Mode to On with Double Tap and Timeout enabled"));
            opi_closeucd_com(&comport);
            return;
        }
        if(modRFTxTimeout == 0)
            tempstr.append("RF On with double tap enabled");
        else if(modRFTxTimeout == 1)
            tempstr.append("RF On with double tap enabled and timeout of 30 min");
        else if(modRFTxTimeout == 2)
            tempstr.append("RF On with double tap enabled and timeout of 1 hour");
        ui->tsRFModeValCB->setCurrentIndex(2);
        ui->tsRFTOValCB->setCurrentIndex(modRFTxTimeout);
    }
    else if(modRFMode == 3)
    {
        if(modRFTxTimeout == 0)
            tempstr.append("RF On with double tap enabled");
        else if(modRFTxTimeout == 1)
            tempstr.append("RF On with double tap enabled and timeout of 30 min");
        else if(modRFTxTimeout == 2)
            tempstr.append("RF On with double tap enabled and timeout of 1 hour");
        ui->tsRFModeValCB->setCurrentIndex(2);
        ui->tsRFTOValCB->setCurrentIndex(modRFTxTimeout);
    }
    ui->msgPTE->appendPlainText(tempstr);

    if(modMMWrite)
    {
        ui->msgPTE->appendPlainText("Memory Module Write Enabled");
    }
    else
    {
        ui->msgPTE->appendPlainText("Memory Module Write Disabled");
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Got TrueSense Status").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_tsSetTimePB_clicked()
{
    HANDLE comport;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if(setTSTime(&comport))
    {
        ErrorMsgBox(QString("Could not set TrueSense time"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Set TrueSense Time").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_tsTogMMWritePB_clicked()
{
    HANDLE comport;
    OPIPKT_t opipkt;
    int modMMWrite;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if(opiucd_tsstatus(&comport, &opipkt))
    {
        ErrorMsgBox(QString("Could not get TrueSense status"));
        opi_closeucd_com(&comport);
        return;
    }

    modMMWrite = !(opipkt.payload[1+DSNLEN+5+FWVLEN+4]);
    if(opiucd_settsmmwrite(&comport, modMMWrite));

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Toggled TrueSense MMWrite").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_tsSetPDNPB_clicked()
{
    HANDLE comport;
    int modpdn;

    modpdn = ui->tsPDNValLE->text().toInt();
    if((modpdn < 0) || (modpdn > 255))
    {
        ErrorMsgBox(QString("PDN must be in range <0,255>"));
        opi_closeucd_com(&comport);
        return;
    }

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if (opiucd_settspdn(&comport, modpdn))
    {
        ErrorMsgBox(QString("Could not set TrueSense PDN"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Set TrueSense PDN").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_tsSetZBPB_clicked()
{
    HANDLE comport;
    int zbChan;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    switch(ui->tsZBValCB->currentIndex())
    {
    case 0:
        zbChan = 11;
        break;
    case 1:
        zbChan = 12;
        break;
    case 2:
        zbChan = 13;
        break;
    case 3:
        zbChan = 14;
        break;
    case 4:
        zbChan = 15;
        break;
    case 5:
        zbChan = 16;
        break;
    case 6:
        zbChan = 17;
        break;
    case 7:
        zbChan = 18;
        break;
    case 8:
        zbChan = 19;
        break;
    case 9:
        zbChan = 20;
        break;
    case 10:
        zbChan = 21;
        break;
    case 11:
        zbChan = 22;
        break;
    case 12:
        zbChan = 23;
        break;
    case 13:
        zbChan = 24;
        break;
    case 14:
        zbChan = 25;
        break;
    case 15:
        zbChan = 26;
        break;
    }

    if (opiucd_settszbchan(&comport, zbChan))
    {
        ErrorMsgBox(QString("Could not set TrueSense ZigBee Channel"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Set TrueSense ZigBee Channel").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_tsSetRFModePB_clicked()
{
    HANDLE comport;
    int rfMode;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    switch(ui->tsRFModeValCB->currentIndex())
    {
    case 0:     // always off
        rfMode = 0;
        break;
    case 1:     // always on
        rfMode = 1;
        break;
    case 2:     // double tap & timeout enabled
        rfMode = 3;
        break;
    }

    if (opiucd_settsrfmode(&comport, rfMode))
    {
        ErrorMsgBox(QString("Could not set RF Mode"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Set TrueSense RF Mode").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_tsSetRFTOPB_clicked()
{
    HANDLE comport;
    int rfTXTOVal;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    switch(ui->tsRFTOValCB->currentIndex())
    {
    case 0:
        rfTXTOVal = 0;
        break;
    case 1:
        rfTXTOVal = 1;
        break;
    case 2:
        rfTXTOVal = 2;
        break;
    }

    if (opiucd_settsrftxtimeout(&comport, rfTXTOVal))
    {
        ErrorMsgBox(QString("Could not set RF Tx Timeout"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Set TrueSense RF TX Timeout").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_mmReadPktsPB_clicked()
{
    HANDLE comport;
    qint32 pktCt;
    QDateTime stDT, endDT;

    if(opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    stDT = QDateTime::currentDateTime();

    pktCt = readMMPkts(&comport);
    if(pktCt == -1)
    {
        ErrorMsgBox(QString("Error during packet read from memory module"));
        opi_closeucd_com(&comport);
        return;
    }
    else if(pktCt == 0)
    {
        ui->msgPTE->appendPlainText(QString("No packets in memory module"));
    }
    else
    {
        endDT = QDateTime::currentDateTime();
        ui->msgPTE->appendPlainText(QString(">> Read %1 MM Packets Successfully in %2s").arg(pktCt).arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Read MM Packets").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_mmChipErasePB_clicked()
{
    HANDLE comport;
    QDateTime stDT, endDT;

    if(opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    stDT = QDateTime::currentDateTime();

    if(eraseMM(&comport))
    {
        ErrorMsgBox(QString("MM did not erase properly"));
        opi_closeucd_com(&comport);
        return;
    }

    endDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString(">> Erased MM in %1s").arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));

    ui->msgPTE->appendPlainText(QString("<< %1 >> Erased MM").arg(endDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}


void ConfigWindow::on_ucShutdownPB_clicked()
{
    HANDLE comport;

    if (opi_openucd_com(&comport))
    {
        ErrorMsgBox(QString("No UC attached (COM1-COM50)"));
        opi_closeucd_com(&comport);
        return;
    }

    if (opiucd_shutdown(&comport))
    {
        ErrorMsgBox(QString("Could not get into Shutdown Mode"));
        opi_closeucd_com(&comport);
        return;
    }

    QDateTime currDT = QDateTime::currentDateTime();
    ui->msgPTE->appendPlainText(QString("<< %1 >> Shutdown mode should have been entered").arg(currDT.toString("yyyy.MM.dd hh:mm:ss")));
    ui->msgPTE->appendPlainText(QString(" "));
    ui->msgPTE->ensureCursorVisible();
    opi_closeucd_com(&comport);
}
