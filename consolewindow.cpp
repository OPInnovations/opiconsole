#include "consolewindow.h"
#include "ui_consolewindow.h"
#include "opi_helper.h"
#include "license/licensedialog.h"

ConsoleWindow::ConsoleWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ConsoleWindow)
{
    ui->setupUi(this);

    cfgWin = new ConfigWindow(this);
    cnvtWin = new ConvertWindow(this);
    pdc = new startwindow(&comPortUser,this);
    bf  = new startwindowbf(&comPortUser,false,this);
    tg  = new startwindowbf(&comPortUser,true,this); //tutorialgame
    slpWin = new SleepAnalysisWindow(this);
    medWin = new MedAnalysisWindow(this);
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());

    // settings related to borrowing the com port
    comPortUser = 0;    // no one is using it
    comPortLender = 0;  // no one borrowed it
    //set window icon
    this->setWindowIcon(QIcon("images/opi_ico.ico"));

    on_contRefreshPB_clicked(); // refresh based on current status
}


ConsoleWindow::~ConsoleWindow()
{
    delete ui;
    delete cfgWin;
    delete cnvtWin;
    delete pdc;
    delete bf;
    delete tg;
    delete slpWin;
    delete medWin;
}


void ConsoleWindow::on_convertPB_clicked()
{
    delete cnvtWin;
    cnvtWin = new ConvertWindow(this);
    cnvtWin->show();
}


void ConsoleWindow::on_profMgrPB_clicked()
{
    pdc->hide();
    bf->hide();
    tg->hide();
    cfgWin->show();
}


void ConsoleWindow::on_rtDisplayPB_clicked()
{
    cfgWin->hide();
    if(comPortUser == COMPORTFREE)
    {
        if(bf->isVisible())
            bf->close();
        if(tg->isVisible())
            tg->close();
        delete pdc;
        pdc = new startwindow(&comPortUser,this);
        pdc->show();
    }
    else if(comPortUser != PDCCOMPORTUSER)
    {
        QMessageBox show_message;
        show_message.setWindowTitle("WARNING");
        show_message.setStyleSheet("background-image: url(:/images/images/bioshare-BGtiletexturedMixedGloss600px.jpg);");
        QFont temp;
        temp.setPointSize(WORNNINGWORDSIZE);
        show_message.setFont(temp);
        show_message.setText("Comport is busy,try again later");
        show_message.show();
        show_message.exec();
    }
}


void ConsoleWindow::on_tgPB_clicked()
{
    cfgWin->hide();
    if(comPortUser == COMPORTFREE)
    {
        if(pdc->isVisible())
            pdc->close();
        if(bf->isVisible())
            bf->close();
        delete tg;
        tg  = new startwindowbf(&comPortUser,true,this);
        tg->show();
    }
    else if(comPortUser != TGCOMPORTUSER)
    {
        QMessageBox show_message;
        show_message.setWindowTitle("WARNING");
        show_message.setStyleSheet("background-image: url(:/images/images/bioshare-BGtiletexturedMixedGloss600px.jpg);");
        QFont temp;
        temp.setPointSize(WORNNINGWORDSIZE);
        show_message.setFont(temp);
        show_message.setText("Comport is busy,try again later");
        show_message.show();
        show_message.exec();
    }
}


void ConsoleWindow::on_bioFBPB_clicked()
{
    QMessageBox show_message;
    QFont temp;

    cfgWin->hide();
    if(comPortUser == COMPORTFREE)
    {
        if(pdc->isVisible())
        pdc->close();
        if(tg->isVisible())
        tg->close();
        delete bf;
        bf  = new startwindowbf(&comPortUser,false,this);
        bf->show();
    }
    else if(comPortUser != BFCOMPORTUSER)
    {
        show_message.setWindowTitle("WARNING");
        show_message.setStyleSheet("background-image: url(:/images/images/bioshare-BGtiletexturedMixedGloss600px.jpg);");
        temp.setPointSize(WORNNINGWORDSIZE);
        show_message.setFont(temp);
        show_message.setText("Comport is busy,try again later");
        show_message.show();
        show_message.exec();
    }
}


void ConsoleWindow::closeEvent(QCloseEvent *)
{
    HANDLE comport;
    QMessageBox myQMB;
    OPIPKT_t ucOpipkt;
    quint8 ucusdStatus;

    if(conBorrowCom()) return;      // can't borrow it so do nothing

    // move controller to off
    if(opi_openucd_com(&comport))
    {
        qDebug() << "unable to open com port/set off mode when exiting";
    }
    else
    {
        opiucd_status(&comport, &ucOpipkt);
        ucusdStatus = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+1];
        opiucd_offmode(&comport);
        myQMB.setText("Shutdown each Sensor for low-power storage");
        myQMB.setStandardButtons(QMessageBox::Ok);
        myQMB.exec();
    }

    opi_closeucd_com(&comport);
    conReturnCom();

    qApp->processEvents();
    qApp->quit();
}


/**
 *  Allow console to borrow comport for configuration
 *  Only case where it can be refused if other console function is using it
 *  Modifies internal variables comPortUser and comPortLender
 *  Returns:
 *      0, if successfully borrowed
 *      -1, if not successful
 */
qint32 ConsoleWindow::conBorrowCom()
{
    if(comPortUser == COMPORTFREE)    // not in use so can use
    {
        comPortUser = CONCOMPORTUSER;
        return 0;
    }
    else if(comPortUser == CONCOMPORTUSER)   // console is using, so refuse
    {
        return -1;
    }
    else if(comPortUser == PDCCOMPORTUSER)  // PDC is using
    {
        pdc->borrowCOM();
        comPortLender = PDCCOMPORTUSER;
        comPortUser = CONCOMPORTUSER;
        return 0;
    }
    else if(comPortUser == BFCOMPORTUSER)  // BF is using
    {
        bf->borrowCOM();
        comPortLender = BFCOMPORTUSER;
        comPortUser = CONCOMPORTUSER;
        return 0;
    }
    else if(comPortUser == TGCOMPORTUSER)  // TG is using
    {
        tg->borrowCOM();
        comPortLender = TGCOMPORTUSER;
        comPortUser = CONCOMPORTUSER;
        return 0;
    }
    else    // unknown so just don't let it borrow
    {
        return -1;
    }
}


/**
 *  Console to return comport
 *  Modifies internal variables comPortUser and comPortLender
 *  Returns:
 *      0, if successfully returned
 *      -1, if not successful
 */
qint32 ConsoleWindow::conReturnCom()
{
    if(comPortLender == COMPORTFREE) // did not borrow from anyone
    {
        comPortUser = COMPORTFREE;
        return 0;
    }
    else if(comPortLender == PDCCOMPORTUSER)  // PDC used before
    {
        comPortLender = COMPORTFREE;
        comPortUser = COMPORTFREE;
        pdc->returnCOM();
        return 0;
    }
    else if(comPortLender == BFCOMPORTUSER)  // BF used before
    {
        comPortLender = COMPORTFREE;
        comPortUser = COMPORTFREE;
        bf->returnCOM();
        return 0;
    }
    else if(comPortLender == TGCOMPORTUSER)  // TG used before
    {
        comPortLender = COMPORTFREE;
        comPortUser = COMPORTFREE;
        tg->returnCOM();
        return 0;
    }
    else    // unknown so don't know what to do
    {
        return -1;
    }
}


/**
 *  Default console state, all things disabled, sensor info empty/hidden,
 *  default colors
 *  Returns:
 *      Nothing
 */
void ConsoleWindow::defaultConsoleState()
{
    qint32 i;

    disableAllConfigElements();
    hideAllSensConfigElements();
    restoreColorAllSensConfigElements();
    for(i = 0; i < TSCTSUPP; i++) tsSlotPdns[i] = 0xFF;
}


/**
 *	Refresh controller and everything
 *      1) update controller time if not set
 *      2) update sensor time if not set
 *      3) update all buttons and fields based on information
 *  Assumes the comport has already been opened.
 *	Inputs:
 *		comportptr, pointer to handle
 *	Returns:
 *      0, if successful
 *      -1, if error encountered
 */
qint32 ConsoleWindow::ucRefresh(HANDLE *comportptr)
{
    OPIPKT_t ucOpipkt, evOpipkt, tsOpipkt, mmOpipkt;
    qint64 ucdsn, ucCurrTS;
    qint32 ucfwv;
    qint16 ucMode, uctsList[PDNLISTLEN], uczbChan, ucusdStatus, ucChgStat;
    qint64 evTS, evRefEpochMSecs;
    QDateTime evDT;
    qint64 tsdsn, tsCurrTS;
    qint64 nowTS;
    qint32 tsfwv;
    qint16 tspdn, tszbChan, tsRFMode, tsRFTxPwr, tsMMWrite, tsRFTxTimeout;
    qint64 memTS, memRefEpochMSecs;
    QDateTime memDT;
    QPalette myPal;
    qint32 i, tsCt, pktNum;

    defaultConsoleState();

    if(opiucd_status(comportptr, &ucOpipkt)) return -1;

    ucdsn = ((qint64) ucOpipkt.payload[0] << 32) + ((qint64) ucOpipkt.payload[1] << 24)
            + ((qint64) ucOpipkt.payload[2] << 16) + ((qint64) ucOpipkt.payload[3] << 8)
            + ((qint64) ucOpipkt.payload[4]);
    ucCurrTS = ((qint64) ucOpipkt.payload[5] << 40) + ((qint64) ucOpipkt.payload[6] << 32) +
            ((qint64) ucOpipkt.payload[7] << 24) + ((qint64) ucOpipkt.payload[8] << 16) +
            ((qint64) ucOpipkt.payload[9] << 8) + ((qint64) ucOpipkt.payload[10]);
    ucfwv = (ucOpipkt.payload[DSNLEN+TSLEN+6] << 8) + ucOpipkt.payload[DSNLEN+TSLEN+6+1];
    ucMode = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN];
    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];
    uczbChan = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN];
    ucusdStatus = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+1];
    ucChgStat = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+2];

    ui->contIndPB->setEnabled(true);    // controller is up so light it up
    ui->contIDLa->setEnabled(true);
    ui->contIDLa->setText(QString(""));
    ui->contShutdownPB->setEnabled(true);
    ui->sensShutdownPB->setEnabled(true);

    // truesense status display
    tsCt = 0;
    for(i = 0; i < TSCTSUPP; i++)  // only support 4 truesense, first line
    {
        if(uctsList[i] != 0xFF)
        {
            tsCt++;
            tsSlotPdns[0] = uctsList[i];
            tsdsn = ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN] << 32)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+1] << 24)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+2] << 16)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+3] << 8)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+4]);
            tsfwv = (ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+5] << 8)
                    + ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+6];
            tszbChan = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+7];
            tsRFMode = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+8];
            tsRFTxPwr = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+9];
            tsMMWrite = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+10];
            tsRFTxTimeout = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+11];

            // show everything, but don't enable
            ui->sensAIndPB->setHidden(false);
            ui->sensAIDLa->setHidden(false);
            ui->sensAIDLa->setText(QString("( %1 )").arg(uctsList[i]));
            ui->sensARFRB->setHidden(false);
            if(tsRFMode & 0x01) ui->sensARFRB->setChecked(true);
            else ui->sensARFRB->setChecked(false);
            ui->sensAMMRB->setHidden(false);
            if(tsMMWrite) ui->sensAMMRB->setChecked(true);
            else ui->sensAMMRB->setChecked(false);
            ui->sensADTRB->setHidden(false);
            if(tsRFMode & 0x02) ui->sensADTRB->setChecked(true);
            else ui->sensADTRB->setChecked(false);

            ui->sensAUnpairPB->setHidden(false);
            ui->sensAUnpairPB->setEnabled(false);
            i++;
            break;  // go to next line for next sensor info
        }
    }

    for(; i < TSCTSUPP; i++)   // second line
    {
        if(uctsList[i] != 0xFF)
        {
            tsCt++;
            tsSlotPdns[1] = uctsList[i];
            tsdsn = ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN] << 32)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+1] << 24)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+2] << 16)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+3] << 8)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+4]);
            tsfwv = (ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+5] << 8)
                    + ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+6];
            tszbChan = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+7];
            tsRFMode = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+8];
            tsRFTxPwr = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+9];
            tsMMWrite = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+10];
            tsRFTxTimeout = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+11];

            // show everything, but don't enable
            ui->sensBIndPB->setHidden(false);
            ui->sensBIDLa->setHidden(false);
            ui->sensBIDLa->setText(QString("( %1 )").arg(uctsList[i]));
            ui->sensBRFRB->setHidden(false);
            if(tsRFMode & 0x01) ui->sensBRFRB->setChecked(true);
            else ui->sensBRFRB->setChecked(false);
            ui->sensBMMRB->setHidden(false);
            if(tsMMWrite) ui->sensBMMRB->setChecked(true);
            else ui->sensBMMRB->setChecked(false);
            ui->sensBDTRB->setHidden(false);
            if(tsRFMode & 0x02) ui->sensBDTRB->setChecked(true);
            else ui->sensBDTRB->setChecked(false);

            ui->sensBUnpairPB->setHidden(false);
            ui->sensBUnpairPB->setEnabled(false);
            i++;
            break;  // go to next line for next sensor info
        }
    }

    for(; i < TSCTSUPP; i++)   // third line
    {
        if(uctsList[i] != 0xFF)
        {
            tsCt++;
            tsSlotPdns[2] = uctsList[i];
            tsdsn = ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN] << 32)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+1] << 24)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+2] << 16)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+3] << 8)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+4]);
            tsfwv = (ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+5] << 8)
                    + ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+6];
            tszbChan = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+7];
            tsRFMode = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+8];
            tsRFTxPwr = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+9];
            tsMMWrite = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+10];
            tsRFTxTimeout = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+11];

            // show everything, but don't enable
            ui->sensCIndPB->setHidden(false);
            ui->sensCIDLa->setHidden(false);
            ui->sensCIDLa->setText(QString("( %1 )").arg(uctsList[i]));
            ui->sensCRFRB->setHidden(false);
            if(tsRFMode & 0x01) ui->sensCRFRB->setChecked(true);
            else ui->sensCRFRB->setChecked(false);
            ui->sensCMMRB->setHidden(false);
            if(tsMMWrite) ui->sensCMMRB->setChecked(true);
            else ui->sensCMMRB->setChecked(false);
            ui->sensCDTRB->setHidden(false);
            if(tsRFMode & 0x02) ui->sensCDTRB->setChecked(true);
            else ui->sensCDTRB->setChecked(false);

            ui->sensCUnpairPB->setHidden(false);
            ui->sensCUnpairPB->setEnabled(false);
            i++;
            break;  // go to next line for next sensor info
        }
    }

    for(; i < TSCTSUPP; i++)   // 4th line
    {
        if(uctsList[i] != 0xFF)
        {
            tsCt++;
            tsSlotPdns[3] = uctsList[i];
            tsdsn = ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN] << 32)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+1] << 24)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+2] << 16)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+3] << 8)
                    + ((qint64) ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+4]);
            tsfwv = (ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+5] << 8)
                    + ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+6];
            tszbChan = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+7];
            tsRFMode = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+8];
            tsRFTxPwr = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+9];
            tsMMWrite = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+10];
            tsRFTxTimeout = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+3+i*PDNINFOLEN+11];

            // show everything, but don't enable
            ui->sensDIndPB->setHidden(false);
            ui->sensDIDLa->setHidden(false);
            ui->sensDIDLa->setText(QString("( %1 )").arg(uctsList[i]));
            ui->sensDRFRB->setHidden(false);
            if(tsRFMode & 0x01) ui->sensDRFRB->setChecked(true);
            else ui->sensDRFRB->setChecked(false);
            ui->sensDMMRB->setHidden(false);
            if(tsMMWrite) ui->sensDMMRB->setChecked(true);
            else ui->sensDMMRB->setChecked(false);
            ui->sensDDTRB->setHidden(false);
            if(tsRFMode & 0x02) ui->sensDDTRB->setChecked(true);
            else ui->sensDDTRB->setChecked(false);

            ui->sensDUnpairPB->setHidden(false);
            ui->sensDUnpairPB->setEnabled(false);
            i++;
            break;  // go to next line for next sensor info
        }
    }

    // set time in unified controller and ts if not set
    // check the timestamp, must be within .1 sec or 409 ticks of current TS
    nowTS = (QDateTime::currentDateTime().toMSecsSinceEpoch() - QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000;
    if(((nowTS-ucCurrTS) > UCRTCFREQ/10) || ((nowTS-ucCurrTS) < -UCRTCFREQ/10))
    {
        if(setUCTime(comportptr)) return -1;
        if(ucusdStatus & 0x01) if(setTSTime(comportptr)) return -1;
    }

    // check if any events, if so, then print first event date/time
    if(opiucd_evcapread(comportptr, &evOpipkt)) return -1;

    if(evOpipkt.length < 2)
    {
        ui->contFTagDTLa->setEnabled(true);
        ui->contFTagDTLa->setText("No tags");
    }
    else
    {
        evTS = ((qint64) evOpipkt.payload[1] << 40) + ((qint64) evOpipkt.payload[1+1] << 32) +
                ((qint64) evOpipkt.payload[1+2] << 24) + ((qint64) evOpipkt.payload[1+3] << 16) +
                ((qint64) evOpipkt.payload[1+4] << 8) + ((qint64) evOpipkt.payload[1+5]);
        evRefEpochMSecs = evTS*1000/UCRTCFREQ;
        evDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(evRefEpochMSecs);
        ui->contFTagDTLa->setEnabled(true);
        ui->contFTagDTLa->setText(QString("%1 (%2)").arg(evDT.toString("yyyyMMdd_hhmmss")).arg((evOpipkt.length-1)/7));
        ui->contTagUploadPB->setEnabled(true);
    }

    // if module is shutdown, then indicate that on button
    if(ucusdStatus & 0x04)
    {
        ui->sensShutdownPB->setChecked(false);
    }
    else
    {
        ui->sensShutdownPB->setChecked(true);
    }

    if(ucusdStatus & 0x01)
    {
        if(opiucd_tsstatus(comportptr,&tsOpipkt)) return -1;
        tsdsn = ((qint64) tsOpipkt.payload[1] << 32) + ((qint64)tsOpipkt.payload[2] << 24)
                + (tsOpipkt.payload[3] << 16) + (tsOpipkt.payload[4] << 8) + tsOpipkt.payload[5];
        tsCurrTS = ((qint64) tsOpipkt.payload[1+DSNLEN] << 32) + ((qint64) tsOpipkt.payload[1+DSNLEN+1] << 24)
                + (tsOpipkt.payload[1+DSNLEN+2] << 16) + (tsOpipkt.payload[1+DSNLEN+3] << 8) + tsOpipkt.payload[1+DSNLEN+4];
        tsfwv = (tsOpipkt.payload[1+DSNLEN+5] << 8) + tsOpipkt.payload[1+DSNLEN+5+1];
        tspdn = tsOpipkt.payload[1+DSNLEN+5+FWVLEN];
        tszbChan = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+1];
        tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];
        tsRFTxPwr = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+3];
        tsMMWrite = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+4];
        tsRFTxTimeout = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+5];

        // check the timestamp, must be within .1 sec or 50 ticks of current TS
        nowTS = (QDateTime::currentDateTime().toMSecsSinceEpoch() - QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*TSRTCFREQ/1000;

        if(((nowTS-tsCurrTS) > TSRTCFREQ/10) || ((nowTS-tsCurrTS) < -TSRTCFREQ/10))
        {
            if(setTSTime(comportptr)) return -1;
        }

        // check against tsSlotPdns to determine slot, and then compare items and set items accordingly
        if(tsSlotPdns[0] == tspdn)  // first line, slot A
        {
            // enable this row
            ui->sensAIndPB->setEnabled(true);
            ui->sensAIDLa->setEnabled(true);
            ui->sensARFRB->setEnabled(true);
            ui->sensAMMRB->setEnabled(true);
            ui->sensADTRB->setEnabled(true);
            ui->sensAUnpairPB->setEnabled(true);

            // make sure zigbee channel is the same
            if(tszbChan != uczbChan)
            {
                if(opiucd_settszbchan(comportptr, uczbChan)) return -1;
            }

            // check inconsistencies
            if(((bool)(tsRFMode & 0x01)) != ui->sensARFRB->isChecked())
            {
                myPal = ui->sensARFRB->palette();
                myPal.setColor(ui->sensARFRB->foregroundRole(), Qt::red);
                ui->sensARFRB->setPalette(myPal);
            }
            if(((bool) tsMMWrite) != ui->sensAMMRB->isChecked())
            {
                myPal = ui->sensAMMRB->palette();
                myPal.setColor(ui->sensAMMRB->foregroundRole(), Qt::red);
                ui->sensAMMRB->setPalette(myPal);
            }
            if(((bool)(tsRFMode & 0x02)) != ui->sensADTRB->isChecked())
            {
                myPal = ui->sensADTRB->palette();
                myPal.setColor(ui->sensADTRB->foregroundRole(), Qt::red);
                ui->sensADTRB->setPalette(myPal);
            }
        }
        else if(tsSlotPdns[1] == tspdn) // second line, slot B
        {
            // enable this row
            ui->sensBIndPB->setEnabled(true);
            ui->sensBIDLa->setEnabled(true);
            ui->sensBRFRB->setEnabled(true);
            ui->sensBMMRB->setEnabled(true);
            ui->sensBDTRB->setEnabled(true);
            ui->sensBUnpairPB->setEnabled(true);

            // make sure zigbee channel is the same
            if(tszbChan != uczbChan)
            {
                if(opiucd_settszbchan(comportptr, uczbChan)) return -1;
            }

            // check inconsistencies
            if(((bool)(tsRFMode & 0x01)) != ui->sensBRFRB->isChecked())
            {
                myPal = ui->sensBRFRB->palette();
                myPal.setColor(ui->sensBRFRB->foregroundRole(), Qt::red);
                ui->sensBRFRB->setPalette(myPal);
            }
            if(((bool) tsMMWrite) != ui->sensBMMRB->isChecked())
            {
                myPal = ui->sensBMMRB->palette();
                myPal.setColor(ui->sensBMMRB->foregroundRole(), Qt::red);
                ui->sensBMMRB->setPalette(myPal);
            }
            if(((bool)(tsRFMode & 0x02)) != ui->sensBDTRB->isChecked())
            {
                myPal = ui->sensBDTRB->palette();
                myPal.setColor(ui->sensBDTRB->foregroundRole(), Qt::red);
                ui->sensBDTRB->setPalette(myPal);
            }
        }
        else if(tsSlotPdns[2] == tspdn) // second line, slot C
        {
            // enable this row
            ui->sensCIndPB->setEnabled(true);
            ui->sensCIDLa->setEnabled(true);
            ui->sensCRFRB->setEnabled(true);
            ui->sensCMMRB->setEnabled(true);
            ui->sensCDTRB->setEnabled(true);
            ui->sensCUnpairPB->setEnabled(true);

            // make sure zigbee channel is the same
            if(tszbChan != uczbChan)
            {
                if(opiucd_settszbchan(comportptr, uczbChan)) return -1;
            }

            // check inconsistencies
            if(((bool)(tsRFMode & 0x01)) != ui->sensCRFRB->isChecked())
            {
                myPal = ui->sensCRFRB->palette();
                myPal.setColor(ui->sensCRFRB->foregroundRole(), Qt::red);
                ui->sensCRFRB->setPalette(myPal);
            }
            if(((bool) tsMMWrite) != ui->sensCMMRB->isChecked())
            {
                myPal = ui->sensCMMRB->palette();
                myPal.setColor(ui->sensCMMRB->foregroundRole(), Qt::red);
                ui->sensCMMRB->setPalette(myPal);
            }
            if(((bool)(tsRFMode & 0x02)) != ui->sensCDTRB->isChecked())
            {
                myPal = ui->sensCDTRB->palette();
                myPal.setColor(ui->sensCDTRB->foregroundRole(), Qt::red);
                ui->sensCDTRB->setPalette(myPal);
            }
        }
        else if(tsSlotPdns[3] == tspdn) // third line, slot D
        {
            // enable this row
            ui->sensDIndPB->setEnabled(true);
            ui->sensDIDLa->setEnabled(true);
            ui->sensDRFRB->setEnabled(true);
            ui->sensDMMRB->setEnabled(true);
            ui->sensDDTRB->setEnabled(true);
            ui->sensDUnpairPB->setEnabled(true);

            // make sure zigbee channel is the same
            if(tszbChan != uczbChan)
            {
                if(opiucd_settszbchan(comportptr, uczbChan)) return -1;
            }

            // check inconsistencies
            if(((bool)(tsRFMode & 0x01)) != ui->sensDRFRB->isChecked())
            {
                myPal = ui->sensDRFRB->palette();
                myPal.setColor(ui->sensDRFRB->foregroundRole(), Qt::red);
                ui->sensDRFRB->setPalette(myPal);
            }
            if(((bool) tsMMWrite) != ui->sensDMMRB->isChecked())
            {
                myPal = ui->sensDMMRB->palette();
                myPal.setColor(ui->sensDMMRB->foregroundRole(), Qt::red);
                ui->sensDMMRB->setPalette(myPal);
            }
            if(((bool)(tsRFMode & 0x02)) != ui->sensDDTRB->isChecked())
            {
                myPal = ui->sensDDTRB->palette();
                myPal.setColor(ui->sensDDTRB->foregroundRole(), Qt::red);
                ui->sensDDTRB->setPalette(myPal);
            }
        }
        else   // not in the list
        {
            // first unhide
            ui->sensUIndPB->setHidden(false);
            ui->sensUIDLa->setHidden(false);
            ui->sensUPairPB->setHidden(false);

            // enable and set the right text
            ui->sensUIndPB->setEnabled(true);
            ui->sensUIDLa->setEnabled(true);
            ui->sensUIDLa->setText(QString("( %1 )").arg(tspdn));
            ui->sensUPairPB->setEnabled(true);
        }
    }

    // check memory module and execute related things if set
    if(ucusdStatus & 0x02)
    {
        // show buttons
        ui->memIndPB->setHidden(false);
        ui->memIDLa->setHidden(false);
        ui->memFPktDTLa->setHidden(false);
        ui->memUploadPB->setHidden(false);
        ui->memErasePB->setHidden(false);
        ui->memIndPB->setEnabled(true);
        ui->memIDLa->setEnabled(true);
        ui->memFPktDTLa->setEnabled(true);

        if(opiucd_get5mmtsdata(comportptr,0,&mmOpipkt) == -1) return -1;
        else if(mmOpipkt.payload[1] == 0)    // check length, if 0, then no data
        {
            ui->memIDLa->setText("Empty");
            ui->memFPktDTLa->setText("No Data");
        }
        else
        {
            ui->memUploadPB->setEnabled(true);
            ui->memErasePB->setEnabled(true);
            ui->memFullQPB->setHidden(false);
            ui->memFullQPB->setEnabled(true);
            ui->memIDLa->setText(QString("( %1 )").arg(mmOpipkt.payload[2+TSLEN]));
            memTS = ((qint64) mmOpipkt.payload[2] << 40) + ((qint64) mmOpipkt.payload[2+1] << 32)
                    + ((qint64) mmOpipkt.payload[2+2] << 24) + ((qint64) mmOpipkt.payload[2+3] << 16)
                    + ((qint64) mmOpipkt.payload[2+4] << 8) + ((qint64) mmOpipkt.payload[2+5]);
            memRefEpochMSecs = memTS*1000/UCRTCFREQ;
            memDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(memRefEpochMSecs);
            // scan the memory to determine roughly how full it is
            for(i = 1; i < 20; i++)
            {
                pktNum = 327680*i/20;
                if(opiucd_get5mmtsdata(comportptr, pktNum, &mmOpipkt) == -1)   return -1;
                if(mmOpipkt.payload[1] == 0) break; // found end
            }
            ui->memFPktDTLa->setText(QString("%1").arg(memDT.toString("yyyyMMdd_hhmmss")));
            ui->memFullQPB->setValue(i*100/20);
        }
    }
    return 0;
}


void ConsoleWindow::hideAllSensConfigElements()
{
    ui->sensAIndPB->setHidden(true);
    ui->sensAIDLa->setHidden(true);
    ui->sensARFRB->setHidden(true);
    ui->sensAMMRB->setHidden(true);
    ui->sensADTRB->setHidden(true);
    ui->sensAUnpairPB->setHidden(true);
    ui->sensBIndPB->setHidden(true);
    ui->sensBIDLa->setHidden(true);
    ui->sensBRFRB->setHidden(true);
    ui->sensBMMRB->setHidden(true);
    ui->sensBDTRB->setHidden(true);
    ui->sensBUnpairPB->setHidden(true);
    ui->sensCIndPB->setHidden(true);
    ui->sensCIDLa->setHidden(true);
    ui->sensCRFRB->setHidden(true);
    ui->sensCMMRB->setHidden(true);
    ui->sensCDTRB->setHidden(true);
    ui->sensCUnpairPB->setHidden(true);
    ui->sensDIndPB->setHidden(true);
    ui->sensDIDLa->setHidden(true);
    ui->sensDRFRB->setHidden(true);
    ui->sensDMMRB->setHidden(true);
    ui->sensDDTRB->setHidden(true);
    ui->sensDUnpairPB->setHidden(true);
    ui->sensUIndPB->setHidden(true);
    ui->sensUIDLa->setHidden(true);
    ui->sensUPairPB->setHidden(true);
    ui->memIndPB->setHidden(true);
    ui->memIDLa->setHidden(true);
    ui->memFPktDTLa->setHidden(true);
    ui->memFullQPB->setHidden(true);
    ui->memUploadPB->setHidden(true);
    ui->memErasePB->setHidden(true);
}


void ConsoleWindow::restoreColorAllSensConfigElements()
{
    QPalette myPal;
    // this element is never modified so can get original
    myPal = ui->contIDLa->palette();
    ui->sensARFRB->setPalette(myPal);
    ui->sensAMMRB->setPalette(myPal);
    ui->sensADTRB->setPalette(myPal);
    ui->sensBRFRB->setPalette(myPal);
    ui->sensBMMRB->setPalette(myPal);
    ui->sensBDTRB->setPalette(myPal);
    ui->sensCRFRB->setPalette(myPal);
    ui->sensCMMRB->setPalette(myPal);
    ui->sensCDTRB->setPalette(myPal);
    ui->sensDRFRB->setPalette(myPal);
    ui->sensDMMRB->setPalette(myPal);
    ui->sensDDTRB->setPalette(myPal);
}


/**
  * Disables all configuration elements except uc refresh
  */
void ConsoleWindow::disableAllConfigElements()
{
    ui->contIndPB->setEnabled(false);
    ui->contIDLa->setEnabled(false);
    ui->contFTagDTLa->setEnabled(false);
    ui->contTagUploadPB->setEnabled(false);
    ui->contShutdownPB->setEnabled(false);
    ui->sensAIndPB->setEnabled(false);
    ui->sensAIDLa->setEnabled(false);
    ui->sensARFRB->setEnabled(false);
    ui->sensAMMRB->setEnabled(false);
    ui->sensADTRB->setEnabled(false);
    ui->sensAUnpairPB->setEnabled(false);
    ui->sensShutdownPB->setEnabled(false);
    ui->sensBIndPB->setEnabled(false);
    ui->sensBIDLa->setEnabled(false);
    ui->sensBRFRB->setEnabled(false);
    ui->sensBMMRB->setEnabled(false);
    ui->sensBDTRB->setEnabled(false);
    ui->sensBUnpairPB->setEnabled(false);
    ui->sensCIndPB->setEnabled(false);
    ui->sensCIDLa->setEnabled(false);
    ui->sensCRFRB->setEnabled(false);
    ui->sensCMMRB->setEnabled(false);
    ui->sensCDTRB->setEnabled(false);
    ui->sensCUnpairPB->setEnabled(false);
    ui->sensDIndPB->setEnabled(false);
    ui->sensDIDLa->setEnabled(false);
    ui->sensDRFRB->setEnabled(false);
    ui->sensDMMRB->setEnabled(false);
    ui->sensDDTRB->setEnabled(false);
    ui->sensDUnpairPB->setEnabled(false);
    ui->sensUIndPB->setEnabled(false);
    ui->sensUIDLa->setEnabled(false);
    ui->sensUPairPB->setEnabled(false);
    ui->memIndPB->setEnabled(false);
    ui->memIDLa->setEnabled(false);
    ui->memFPktDTLa->setEnabled(false);
    ui->memFullQPB->setEnabled(false);
    ui->memUploadPB->setEnabled(false);
    ui->memErasePB->setEnabled(false);
}


void ConsoleWindow::on_contRefreshPB_clicked()
{
    QTime killTime;

    HANDLE comport;
    if(conBorrowCom()) return;  // check if comport can be borrowed, if not do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opiucd_onmode(&comport);    // go to on mode
    opiucd_turnmodon(&comport);    // go to on mode

    killTime = QTime::currentTime().addMSecs(1100);
    while(killTime > QTime::currentTime());

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensShutdownPB_clicked(bool checked)
{
    HANDLE comport;
    QTime killDT;
    QMessageBox myQMB;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(!checked)
    {
        opiucd_onmode(&comport);    // go to on mode
        if(opiucd_turnmodon(&comport))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        opiucd_offmode(&comport);   // go to off mode
        if(opiucd_turnmodoff(&comport))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
        myQMB.setText("Insert & remove each Sensor for low-power storage");
        myQMB.setStandardButtons(QMessageBox::Ok);
        myQMB.exec();
    }

    killDT = QTime::currentTime().addMSecs(1500);
    while(QTime::currentTime() < killDT);

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_contShutdownPB_clicked()
{
    HANDLE comport;
    QMessageBox myQMB;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }
    else
    {        
        opiucd_offmode(&comport);   // go to off mode
        if(opiucd_turnmodoff(&comport))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
        ucRefresh(&comport);
        qApp->processEvents();
        myQMB.setText("Insert & remove each Sensor for low-power storage");
        myQMB.setStandardButtons(QMessageBox::Ok);
        myQMB.exec();
    }
    opiucd_shutdown(&comport);  // don't really need to look at return value, since shutdown

    defaultConsoleState();
    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensAUnpairPB_clicked()
{
    HANDLE comport;
    OPIPKT_t ucOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN];

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[0]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[0]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[0]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[0]) tsSlot = 3;

    if(opiucd_forgettssettings(&comport, tsSlot))    // A is always first sensor
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensBUnpairPB_clicked()
{
    HANDLE comport;
    OPIPKT_t ucOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN];

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[1]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[1]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[1]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[1]) tsSlot = 3;

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensCUnpairPB_clicked()
{
    HANDLE comport;
    OPIPKT_t ucOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN];

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[2]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[2]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[2]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[2]) tsSlot = 3;

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensDUnpairPB_clicked()
{
    HANDLE comport;
    OPIPKT_t ucOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN];

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[3]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[3]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[3]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[3]) tsSlot = 3;

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


/**
 *	Pair plugged-in ts to uc
 *      1) check that uc has an empty slot
 *      2) set ts zigbee channel to same as uc
 *      3) check ts pdn if conflicts with existing paired to uc and change if so
 *      4) copy ts settings to uc
 *  Assumes the comport has already been opened.
 *	Inputs:
 *		comportptr, pointer to handle
 *	Returns:
 *      0, if successful
 *      -1, if uc error encountered
 *      -2, if ts error encountered
 */
qint32 ConsoleWindow::ucPair(HANDLE *comportptr)
{
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint16 uctsList[PDNLISTLEN], uczbChan, ucusdStatus;
    qint16 tspdn, tszbChan;
    qint32 i;
    qint32 tsSlot, result[16], resMinInd;
    qint16 tempi16;
    QProgressDialog progQPD("Measuring ZigBee Channels", QString(), 0, 15);

    progQPD.setWindowModality(Qt::WindowModal);
    progQPD.setMinimumDuration(100);

    // first get UC status
    if(opiucd_status(comportptr, &ucOpipkt)) return -1;

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];
    uczbChan = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN];
    ucusdStatus = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+PDNLISTLEN+1];

    // make sure sensor is still in slot
    if(!(ucusdStatus & 0x01)) return -2;

    // find slot that is not occupied
    if(uctsList[0] == 0xFF) tsSlot = 0;  // first pdnslot
    else if(uctsList[1] == 0xFF) tsSlot = 1;   // 2nd pdnslot
    else if(uctsList[2] == 0xFF) tsSlot = 2;  // 3rd pdnslot
    else if(uctsList[3] == 0xFF) tsSlot = 3;   // 4th pdnslot
    else return -1;   // everything is occupied, so don't pair and return

    // if no paired sensors, then get new zigbee channel based on current conditions
    if((uctsList[0] == 0xFF) && (uctsList[1] == 0xFF) && (uctsList[2] == 0xFF) &&
            (uctsList[3] == 0xFF))
    {
        // Go through each valid channel and get maximum out of 100 tries
        for(i = 0; i < 16; i++)
        {
            if(opiucd_setzbchan(comportptr, i+11))
            {
                return -1;
            }
            result[i] = maxWLMeasure100(comportptr);
            progQPD.setValue(i);
            qApp->processEvents();
        }
        // find minimum index
        resMinInd = 0;
        for(i = 0; i < 16; i++) // find first valid value
        {
            if(result[i] >= 0)  // measure must have been successful
            {
                resMinInd = i;
                break;
            }
        }
        for(; i < 16; i++)
        {
            if((result[i] >= 0) && (result[i] < result[resMinInd]))
            {
                resMinInd = i;
            }
        }

        uczbChan = resMinInd+11;
        // set new zb channel
        if(opiucd_setzbchan(comportptr, uczbChan))
        {
            return -1;
        }
    }

    // set the ts rf tx pwr to default value of 7 (-0.5dBm)
    opiucd_settsrftxpwr(comportptr, DEFRFTXPWR);

    // set the ts zbChan to the same as the uc
    if(opiucd_settszbchan(comportptr, uczbChan)) return -2;

    // get the ts information
    if(opiucd_tsstatus(comportptr, &tsOpipkt)) return -2;

    tspdn = tsOpipkt.payload[1+DSNLEN+5+FWVLEN];
    tszbChan = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+1];

    // check if zb chan was set correctly
    if(tszbChan != uczbChan) return -2;

    // see if ts pdn is unique in this uc, if so, then use, if not then randomize
    if((uctsList[0] == tspdn) || (uctsList[1] == tspdn) || (uctsList[2] == tspdn) || (uctsList[3] == tspdn))
    {
        while(1)
        {
            tempi16 = QDateTime::currentMSecsSinceEpoch() % 255;
            if(tempi16 != tspdn) break; // only get out if the new pdn is different
        }
        tspdn = tempi16;
        if(opiucd_settspdn(comportptr, tspdn)) return -2;
    }

    // do the actual copying to uc
    if(opiucd_copytssettings(comportptr, tsSlot)) return -1;

    // if got here, then successful
    return 0;
}


void ConsoleWindow::on_sensUPairPB_clicked()
{
    HANDLE comport;
    qint32 resucPair;
    QMessageBox msgBox;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    resucPair = ucPair(&comport);
    if(resucPair == -1) // uc problem so don't refresh
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        msgBox.setWindowTitle("Error");
        msgBox.setText("Pairing unsuccessful, Controller problem");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    }
    else if(resucPair == -2) // ts problem so refresh
    {
        if(ucRefresh(&comport))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
        opi_closeucd_com(&comport);
        conReturnCom();
        msgBox.setWindowTitle("Error");
        msgBox.setText("Pairing unsuccessful, Sensor problem");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensARFRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[0]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[0]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[0]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[0]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x02) | 0x01))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x02)))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensAMMRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[0]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[0]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[0]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[0]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsmmwrite(&comport, 1))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsmmwrite(&comport, 0))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensADTRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[0]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[0]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[0]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[0]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x01) | 0x02))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x01)))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensBRFRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[1]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[1]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[1]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[1]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x02) | 0x01))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x02)))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensBMMRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[1]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[1]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[1]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[1]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsmmwrite(&comport, 1))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsmmwrite(&comport, 0))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensBDTRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[1]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[1]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[1]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[1]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x01) | 0x02))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x01)))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensCRFRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[2]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[2]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[2]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[2]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x02) | 0x01))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x02)))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensCMMRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[2]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[2]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[2]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[2]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsmmwrite(&comport, 1))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsmmwrite(&comport, 0))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensCDTRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[2]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[2]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[2]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[2]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x01) | 0x02))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x01)))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensDRFRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[3]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[3]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[3]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[3]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x02) | 0x01))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x02)))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensDMMRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[3]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[3]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[3]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[3]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsmmwrite(&comport, 1))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsmmwrite(&comport, 0))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_sensDDTRB_clicked(bool checked)
{
    HANDLE comport;
    OPIPKT_t ucOpipkt, tsOpipkt;
    qint32 i;
    qint32 tsSlot;
    qint16 uctsList[PDNLISTLEN], tsRFMode;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_status(&comport, &ucOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    for (i = 0; i < PDNLISTLEN; i++)
        uctsList[i] = ucOpipkt.payload[DSNLEN+TSLEN+6+FWVLEN+1+i];

    // find which uc slot
    tsSlot = 0; // default value
    if(uctsList[0] == tsSlotPdns[3]) tsSlot = 0;
    else if(uctsList[1] == tsSlotPdns[3]) tsSlot = 1;
    else if(uctsList[2] == tsSlotPdns[3]) tsSlot = 2;
    else if(uctsList[3] == tsSlotPdns[3]) tsSlot = 3;

    if(opiucd_tsstatus(&comport, &tsOpipkt))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    tsRFMode = tsOpipkt.payload[1+DSNLEN+5+FWVLEN+2];

    if(checked)
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x01) | 0x02))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }
    else
    {
        if(opiucd_settsrfmode(&comport,(tsRFMode & 0x01)))
        {
            defaultConsoleState();
            opi_closeucd_com(&comport);
            conReturnCom();
            return;
        }
    }

    if(opiucd_forgettssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(opiucd_copytssettings(&comport, tsSlot))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_memUploadPB_clicked()
{
    HANDLE comport;
    qint32 pktCt;
    QMessageBox msgBox;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    // Next take care of memory module readout
    pktCt = readMMPktsToEDF(&comport);

    if(pktCt == -1)
    {
        msgBox.setWindowTitle("Memory Module Readout");
        msgBox.setText(QString("Error during packet read"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }
    else if(pktCt == -2)    // user aborted before started reading
    {
        // do nothing
    }
    else if(pktCt == 0)
    {
        msgBox.setWindowTitle("Memory Module Readout");
        msgBox.setText(QString("No packets in memory module"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
    else
    {
        // status message in helper function already
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_contTagUploadPB_clicked()
{
    HANDLE comport;
    int eventCt;
    QMessageBox msgBox;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    // First take care of events
    eventCt = readUCEvents(&comport);
    if(eventCt < 0)
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }
//    else if(eventCt == 0)
//    {
//        msgBox.setWindowTitle("TAG Upload");
//        msgBox.setText(QString("No valid tags"));
//        msgBox.setStandardButtons(QMessageBox::Ok);
//        msgBox.exec();
//    }
    else
    {
        msgBox.setWindowTitle("TAG Upload");
        msgBox.setText(QString("TAG file saved"));
        msgBox.setInformativeText(QString("Clear tags for fresh start?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        if(msgBox.exec() == QMessageBox::Yes)
        {
            if(opiucd_evcaperase(&comport))
            {
                defaultConsoleState();
                opi_closeucd_com(&comport);
                conReturnCom();
                return;
            }
        }
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();

}


void ConsoleWindow::on_memErasePB_clicked()
{
    HANDLE comport;
    QMessageBox msgBox;

    if(conBorrowCom()) return; // failed to get com so do nothing

    if(opi_openucd_com(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(eraseMM(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    if(ucRefresh(&comport))
    {
        defaultConsoleState();
        opi_closeucd_com(&comport);
        conReturnCom();
        return;
    }

    opi_closeucd_com(&comport);
    conReturnCom();
}


void ConsoleWindow::on_aboutPB_clicked()
{
    LicenseDialog licD;
    licD.exec();
}


void ConsoleWindow::on_slpAnalyzePB_clicked()
{
    if(slpWin->isVisible())
        slpWin->hide();
    slpWin->show();
}


void ConsoleWindow::on_medAnalyzePB_clicked()
{
    if(medWin->isVisible())
        medWin->hide();
    medWin->show(); // This will bring it to the front
}
