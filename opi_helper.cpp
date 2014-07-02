/* ****************************
 * Helper functions definition file
 * OP Innovations
 *
 *
 * v0.97 - 20130306 mpeng
 * --------------------------
 * 1. Error checking inside get_stream so that packet payload
 *      doesn't exceed 1024
 * 2. Added readMMPktsToEDF and other associated EDF functions so data can
 *      read out from memory card and directly written to EDF format
 *
 * v0.96 - 20130226 mpeng
 * --------------------------
 * 1. Error checking inside get_stream so that it doesn't
 *      try to get data when there is more data in stream
 *
 * v0.95 - 20130119	mpeng
 * --------------------------
 * 1. Original
 *
 * ****************************/

#include "opi_helper.h"
#include "readmm/readmmdialog.h"
#include "converter/twodaccelviewer.h"

/***
  * Function definitions
  */


/***
  *	Set the Controller time to current time.
  *  Assumes the comport has already been opened by SDK.
  *	Inputs:
  *		comportptr, pointer to handle
  *	Returns:
  *      0, if successful
  *      -1, if error
  */
qint32 setUCTime(HANDLE *comportptr)
{
    qint64 ucRefEpochMSecs, ucSetTS;
    QDateTime currDT, refDT;
    int timeStamp[6];

    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    currDT = QDateTime::currentDateTime();
    refDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz");
    ucRefEpochMSecs = currDT.toMSecsSinceEpoch() - refDT.toMSecsSinceEpoch();
    ucSetTS = ucRefEpochMSecs*UCRTCFREQ/1000;

    // Set Timestamp, need to correct and put correct date time in
    timeStamp[0] = (ucSetTS >> 40) & 0xFF;
    timeStamp[1] = (ucSetTS >> 32) & 0xFF;
    timeStamp[2] = (ucSetTS >> 24) & 0xFF;
    timeStamp[3] = (ucSetTS >> 16) & 0xFF;
    timeStamp[4] = (ucSetTS >> 8) & 0xFF;
    timeStamp[5] = (ucSetTS >> 0) & 0xFF;

    if(opiucd_setpktts(comportptr, timeStamp)) return -1;
    else return 0;
}


/***
  *	Use the Controller to take 100 measurements of current ZigBee Channel
  *  returning the maximum peak Energy Detected.
  *  Assumes the comport has already been opened by SDK.
  *	Inputs:
  *		comportptr, pointer to handle
  *	Returns:
  *      non-negative integer representing maximum peak Energy Detected
  *      -1, if error
  */
qint32 maxWLMeasure100(HANDLE *comportptr)
{
    OPIPKT_t opipkt;
    qint32 i;
    qint32 maxED;

    maxED = 0;
    for(i = 0; i < 100; i++)
    {
        if(opiucd_wlmeasure(comportptr, &opipkt))
        {
            return -1;
        }
        if(opipkt.payload[2] > maxED) maxED = opipkt.payload[2];
    }
    return maxED;
}


/***
  *	Write an OPIPKT_t to stream with OPI Link wrapper
  *	Inputs:
  *		pktptr, pointer to the packet
  *		osptr, pointer to output stream
  *	Returns:
  *      nothing
  */
void opipkt_put_stream(OPIPKT_t *pktptr, QDataStream *osptr)
{
    qint32 i;
    quint16 calcChksm;
    quint8 tempui8;

    calcChksm = 0;
    tempui8 = SYNCBYTE;
    *osptr << tempui8 << tempui8;       // 2 sync bytes
    tempui8 = ((pktptr->length+1) >> 8) & 0xFF;
    *osptr << tempui8;                  // length high byte
    tempui8 = (pktptr->length+1) & 0xFF;
    *osptr << tempui8;                  // length low byte
    tempui8 = pktptr->dataCode;
    *osptr << tempui8;
    calcChksm += pktptr->dataCode;
    for(i = 0; i < pktptr->length; i++)
    {
        tempui8 = pktptr->payload[i];
        *osptr << tempui8;
        calcChksm += pktptr->payload[i];
    }
    tempui8 = (calcChksm >> 8) & 0xFF;  // checksum high byte
    *osptr << tempui8;
    tempui8 = calcChksm & 0xFF;         // checksum low byte
    *osptr << tempui8;
}


/***
  *	Get an OPIPKT_t from stream with OPI Link wrapper
  *	Inputs:
  *		pktptr, pointer to the packet
  *		isptr, pointer to input stream
  *	Returns:
  *		0, valid packet
  *      -1, end of stream
  *      2, null packet
  *      1, invalid packet
  */
qint8 opipkt_get_stream(OPIPKT_t *pktptr, QDataStream *isptr)
{
    qint32 i;
    quint16 pktChksm, calcChksm;
    quint8 tempui8;

    calcChksm = 0;
    while(!isptr->atEnd())     // must have data
    {
        *isptr >> tempui8;
        if(tempui8 != SYNCBYTE) continue;
        if(isptr->atEnd()) break;
        *isptr >> tempui8;
        if(tempui8 != SYNCBYTE) continue;
        if(isptr->atEnd()) break;
        *isptr >> tempui8;
        pktptr->length = tempui8 << 8;
        if(isptr->atEnd()) break;
        *isptr >> tempui8;
        pktptr->length += tempui8 - 1;
        if(!tempui8) return 2;	// empty packet
        if(isptr->atEnd()) break;
        *isptr >> pktptr->dataCode;
        calcChksm += pktptr->dataCode;
        for (i = 0; i < pktptr->length; i++)
        {
            if(i >= 1024) break;    // only limited space
            if(isptr->atEnd()) break;
            *isptr >> pktptr->payload[i];
            calcChksm += pktptr->payload[i];
        }
        if(isptr->atEnd()) break;
        *isptr >> tempui8;
        pktChksm = tempui8 << 8;
        if(isptr->atEnd()) break;
        *isptr >> tempui8;
        pktChksm += tempui8;
        if (pktChksm != calcChksm) return 1;
        else return 0;
    }
    return -1;
}


/***
  *	Determine sample quality based on adcData[0] and REMOVE
  *  it from adcData[0]. The sample quality is in lowest 2 bits.
  *      0 - best quality
  *      1 - good quality
  *      2 - mediocre quality
  *      3 - poorest quality
  *	Inputs:
  *		adcData, pointer to array where adc data is
  *	Returns:
  *		sample quality
  */
qint8 getSampQual(qint16 *adcData)
{
    qint8 temp;
    temp = adcData[0] & 0x0003;
    adcData[0] &= 0xFFFC;   // remove this info from adcData
    return temp;
}


/***
  *	Read out the events in the unified controller out to a text file
  *  if there are events. 2 fields in text file, tab separated:
  *      1) offset time, 2) text annotation
  *  Assumes the comport has already been opened by SDK.
  *	Inputs:
  *		comportptr, pointer to handle
  *	Returns:
  *      positive integer, if there are event(s)
  *      0, if no events
  *      -1, if error
  */
qint32 readUCEvents(HANDLE *comportptr)
{
    OPIPKT_t evOpipkt, ucOpipkt;
    qint64 eventTS, prevEventTS, sysTS, eventRefEpochMSecs, firstEventRefEpochMSecs;
    qint64 eventRSec, eventRMSec;
    qint32 i, eventCt, discardTagCt;
    QFile *outfilep;
    QTextStream *outf;
    QString outfileName;
    QDateTime eventDT;
    QMessageBox msgBox;

    if(opiucd_status(comportptr, &ucOpipkt)) return -1;

    if(opiucd_evcapread(comportptr, &evOpipkt)) return -1;

    if(evOpipkt.length < 2) return 0;


    eventCt = (evOpipkt.length-1)/7;
    // got some data so setup
    discardTagCt = 0;
    // find first valid tag
    sysTS = (QDateTime::currentMSecsSinceEpoch() - QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000;
    for(i = 0; i < eventCt; i++)
    {       
        eventTS = ((qint64) evOpipkt.payload[1+7*i] << 40) + ((qint64) evOpipkt.payload[1+7*i+1] << 32) +
                ((qint64) evOpipkt.payload[1+7*i+2] << 24) + ((qint64) evOpipkt.payload[1+7*i+3] << 16) +
                ((qint64) evOpipkt.payload[1+7*i+4] << 8) + ((qint64) evOpipkt.payload[1+7*i+5]);
        if(((eventTS - sysTS) < VALIDTAGTHRESMS) && ((sysTS-eventTS) < VALIDTAGTHRESMS))
        {
            break; // got a valid first tag
        }
        discardTagCt++;
    }
    prevEventTS = eventTS;
    if(i >= eventCt)    // didn't get a valid first tag so exit
    {
        msgBox.setText(QString("%1 invalid tags discarded").arg(discardTagCt));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return 0;
    }
    firstEventRefEpochMSecs = eventTS*1000/UCRTCFREQ;
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    eventDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(firstEventRefEpochMSecs);

    outfileName = QFileDialog::getExistingDirectory(0, "Tag File Out Directory", QDir::currentPath()).append("\\");
    outfileName.append(QString("D%1_TAG.txt").arg(eventDT.toString("yyyyMMdd_hhmmss")));
    outfilep = new QFile(outfileName);

    if(outfilep->exists())
    {
        msgBox.setWindowTitle("TAG Upload");
        msgBox.setText("Out file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delete outfilep;
            return -1;
        }
    }

    if(!outfilep->open(QIODevice::WriteOnly | QIODevice::Text))
    {
        delete outfilep;
        return -1;
    }
    outf = new QTextStream(outfilep);

    // write start time
    *outf << QString("0.000\tS%1").arg(eventDT.toString("yyyyMMdd_hhmmss")) << endl;

    for(; i < eventCt; i++)
    {
        eventTS = ((qint64) evOpipkt.payload[1+7*i] << 40) + ((qint64) evOpipkt.payload[1+7*i+1] << 32) +
                ((qint64) evOpipkt.payload[1+7*i+2] << 24) + ((qint64) evOpipkt.payload[1+7*i+3] << 16) +
                ((qint64) evOpipkt.payload[1+7*i+4] << 8) + ((qint64) evOpipkt.payload[1+7*i+5]);
        if(eventTS < prevEventTS)  // ensure monotonicity of tags
        {
            discardTagCt++;
            continue;
        }
        if(((eventTS - sysTS) > VALIDTAGTHRESMS) || ((sysTS-eventTS) > VALIDTAGTHRESMS)) // discard tags not within range
        {
            discardTagCt++;
            continue;
        }
        eventRefEpochMSecs = eventTS*1000/UCRTCFREQ;
        // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
        eventDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(eventRefEpochMSecs);
        eventRSec = (eventRefEpochMSecs-firstEventRefEpochMSecs)/1000;
        eventRMSec = (eventRefEpochMSecs-firstEventRefEpochMSecs)%1000;
        if(eventRMSec < 0) eventRMSec *= -1;
        if(evOpipkt.payload[1+7*i+6] == 1)    // minus
            *outf << QString("%1.%2\tNEG").arg(eventRSec).arg(eventRMSec,3,10,QChar('0')) << endl;
        else if(evOpipkt.payload[1+7*i+6] == 2)    // plus
            *outf << QString("%1.%2\tPOS").arg(eventRSec).arg(eventRMSec,3,10,QChar('0')) << endl;
        prevEventTS = eventTS;
    }

    // write end time
    *outf << QString("%1.%2\tEND").arg((eventRefEpochMSecs-firstEventRefEpochMSecs)/1000).arg((eventRefEpochMSecs-firstEventRefEpochMSecs)%1000,3,10,QChar('0')) << endl;

    outfilep->close();
    delete outf;
    delete outfilep;

    if(discardTagCt)
    {
        msgBox.setWindowTitle("TAG Upload");
        msgBox.setInformativeText("");
        msgBox.setText(QString("%1 invalid tags discarded").arg(discardTagCt));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
    return eventCt;

}


/***
  *	Set the Sensor time to current time.
  *  Assumes the comport has already been opened by SDK.
  *	Inputs:
  *		comportptr, pointer to handle
  *	Returns:
  *      0, if successful
  *      -1, if error
  */
qint32 setTSTime(HANDLE *comportptr)
{
    if (opiucd_settsrtc(comportptr)) return -1;
    else return 0;
}


/***
  *	Read out the packets in the Memory Module to a .opi file.
  *  Starting packet number is always rounded down to the nearest multiple of 5.
  *  Assumes the comport has already been opened by SDK.
  *	Inputs:
  *		comportptr, pointer to handle
  *	Returns:
  *      non-negative number indicating number of packets if successful
  *      -1, if error
  */
qint32 readMMPkts(HANDLE *comportptr, qint32 fromPktNum, qint32 toPktNum)
{
    OPIPKT_t ucOpipkt, mmOpipkt, tmpOpipkt;
    qint32 i, j, k, pktCt, consecBlanks, currpl, pktNum;
    QString outfileName;
    QFile *outfile;
    QDataStream *outf;
    quint8 tempui8, tspdn;
    qint64 mmFirstFrmTS, mmRefEpochMSecs;
    QDateTime mmFirstFrmDT;
    QProgressDialog *progQPDp;
    QTime killTime;

    if(opiucd_status(comportptr, &ucOpipkt)) return -1;

    if(opiucd_get5mmtsdata(comportptr, fromPktNum, &mmOpipkt) == -1)   return -1;
    else if(mmOpipkt.payload[1] == 0) // means no data in packet
    {
        return 0;
    }
    // got some data so check first one to setup
    tspdn = mmOpipkt.payload[8];

    mmFirstFrmTS = ((qint64) mmOpipkt.payload[2] << 40) + ((qint64) mmOpipkt.payload[3] << 32) +
            ((qint64) mmOpipkt.payload[4] << 24) + ((qint64) mmOpipkt.payload[5] << 16) +
            ((qint64) mmOpipkt.payload[6] << 8) + ((qint64) mmOpipkt.payload[7]);
    mmRefEpochMSecs = mmFirstFrmTS*1000/UCRTCFREQ;
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    mmFirstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(mmRefEpochMSecs);

    outfileName = QString("D%1_%2").arg(mmFirstFrmDT.toString("yyyyMMdd_hhmmss")).arg(tspdn);
    outfileName.append(".opi");
    outfile = new QFile(outfileName);
    if (!outfile->open(QIODevice::WriteOnly))
    {
        delete outfile;
        return -1;
    }
    outf = new QDataStream(outfile);

    // if this sensor is not in pdnlist, then overwrite it onto last slot
    if((tspdn != ucOpipkt.payload[20]) && (tspdn != ucOpipkt.payload[21]) &&
            (tspdn != ucOpipkt.payload[22]) && (tspdn != ucOpipkt.payload[23]) &&
            (tspdn != ucOpipkt.payload[24]) && (tspdn != ucOpipkt.payload[25]) &&
            (tspdn != ucOpipkt.payload[26]) && (tspdn != ucOpipkt.payload[27]))
        ucOpipkt.payload[27] = tspdn;
    // write header
    for(i = 0; i < ucOpipkt.length; i++) *outf << ucOpipkt.payload[i];
    tempui8 = 0xFF;
    for(; i < OPIHDRLEN; i++) *outf << tempui8; // fill the rest with FF

    // write first 5 packets that already read out
    tmpOpipkt.dataCode = 0x2A;
    tmpOpipkt.payload[0] = 0x01;
    currpl = 1; // current place in packet
    for(i = 0; i < 5; i++)
    {
        if(mmOpipkt.payload[currpl] == 0)   // skip this packet since empty
        {
            currpl++;
            continue;
        }
        tmpOpipkt.length = mmOpipkt.payload[currpl]+1;    // add 1 for sub datacode @payload[0]
        for(j = 1; j < tmpOpipkt.length; j++) tmpOpipkt.payload[j] = mmOpipkt.payload[j+currpl];
        opipkt_put_stream(&tmpOpipkt, outf);
        currpl += mmOpipkt.payload[currpl]+1; // go to next packet
    }

    // scan the memory to determine roughly how full it is
    for(i = 1; i < 20; i++)
    {
        pktNum = (toPktNum-fromPktNum)*i/20;
        if(opiucd_get5mmtsdata(comportptr, pktNum+fromPktNum, &mmOpipkt) == -1)   return -1;
        if(mmOpipkt.payload[1] == 0) break; // found end
    }

    // show progress with a progressBar
    progQPDp = new QProgressDialog("Reading Packets from Memory Module...", "Abort",
                                   fromPktNum, (toPktNum-fromPktNum)*i/20);
    progQPDp->setWindowModality(Qt::WindowModal);
    progQPDp->setMinimumDuration(0);

    // read rest of the packets until get 5 consecutive packets that have no data
    consecBlanks = 0;
    pktCt = 5;  // read 5 packets

    for(i = fromPktNum+5; i < toPktNum; i += 5)
    {
        if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))
        {
            killTime = QTime::currentTime().addMSecs(100);
            while(killTime > QTime::currentTime());
            if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))
            {
                killTime = QTime::currentTime().addMSecs(100);
                while(killTime > QTime::currentTime());
                if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))
                {
                    killTime = QTime::currentTime().addMSecs(100);
                    while(killTime > QTime::currentTime());
                    if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))
                    {
                        killTime = QTime::currentTime().addMSecs(100);
                        while(killTime > QTime::currentTime());
                        if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))   // try 5 times before aborting
                        {
                            outfile->close();
                            delete outf;
                            delete outfile;
                            delete progQPDp;
                            return -1;
                        }
                    }
                }
            }
        }
        // write 5 packets or skip
        tmpOpipkt.dataCode = 0x2A;
        tmpOpipkt.payload[0] = 0x01;
        currpl = 1; // current place in packet
        for(j = 0; j < 5; j++)
        {
            if(mmOpipkt.payload[currpl] == 0)   // skip this packet since empty
            {
                currpl++;
                consecBlanks++;
                continue;
            }
            consecBlanks = 0;
            tmpOpipkt.length = mmOpipkt.payload[currpl]+1;    // add 1 for sub datacode @payload[0]
            for(k = 1; k < tmpOpipkt.length; k++) tmpOpipkt.payload[k] = mmOpipkt.payload[k+currpl];
            opipkt_put_stream(&tmpOpipkt, outf);
            pktCt++;
            currpl += mmOpipkt.payload[currpl]+1; // go to next packet
        }
        if(consecBlanks >= 5) break;    // stop reading since no more
        if((pktCt % 100) == 0)
        {
            progQPDp->setValue(i);
            progQPDp->setLabelText(QString("Reading from Memory Module...\nRead %1 packets so far").arg(pktCt));
            qApp->processEvents();
        }
        if(progQPDp->wasCanceled()) break;
    }
    outfile->close();
    delete outf;
    delete outfile;

    progQPDp->setValue(progQPDp->maximum());
    delete progQPDp;

    return pktCt;
}


// Gets the PDN from the local recording ID. Usually stored in device
// field, but use regular expression so will find anywhere. If can't find
// it then will set it 255.
qint32 getPDNlrid(QString lrid)
{
    QRegExp pdnQRE;
    QRegExp pdnQRE2;
    QStringList tempQSL;
    qint32 retPDN;

    // get PDN from Device field
    pdnQRE2.setPattern("OPITS[A-Fa-f0-9]{2}");  // match OPITSxx where xx is hexadecimal digits
    pdnQRE.setPattern("OPITS[0-9]{3}");     // match OPITSxxx where xxx is decimal digits
    tempQSL = lrid.trimmed().split(QRegExp("\\s+"));
    if(tempQSL.size() > 0)
    {
        if(pdnQRE.indexIn(tempQSL.last()) > -1)
        {
            retPDN = pdnQRE.cap(0).replace("OPITS","").toInt(0, 10);
        }
        else if(pdnQRE2.indexIn(tempQSL.last()) > -1)
        {
            retPDN = pdnQRE2.cap(0).replace("OPITS","").toInt(0, 16);
        }
        else
        {
            retPDN = 255;   // unknown PDN
        }
    }
    else
    {
        retPDN = 255;   // empty so unknown PDN
    }

    return retPDN;
}


// Returns a QString in format of "UTC+HH:mm" with the offset of local time
// from UTC contained in +HH:mm. This function depends on the system time.
// granularity is 15 minutes
QString localUTCOffset(void)
{
    qint64 diffMSecs, diffMins;
    QString retQS;

    diffMSecs = QDateTime::fromString(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"),"yyyyMMddhhmmsszzz").toMSecsSinceEpoch()
            - QDateTime::fromString(QDateTime::currentDateTimeUtc().toString("yyyyMMddhhmmsszzz"),"yyyyMMddhhmmsszzz").toMSecsSinceEpoch();
    diffMins = diffMSecs/60000;
    retQS = QString("UTC");
    if(diffMins < 0)
    {
        retQS.append("-");
        diffMins *= -1;
    }
    else
    {
        retQS.append("+");
    }
    diffMins = (diffMins+7)/15*15;  // Round to nearest 15 minutes
    retQS.append(QString("%1:%2").arg(diffMins/60,2,10,QChar('0')).arg(diffMins%60,2,10,QChar('0')));

    return retQS;
}

/***
  *	Read out the packets in the Memory Module and convert to a .edf file.
  *  Starting packet number is always rounded down to the nearest multiple of 5.
  *  Assumes the comport has already been opened by SDK.
  *	Inputs:
  *		comportptr, pointer to handle
  *	Returns:
  *      non-negative number indicating number of packets if successful
  *      -1, if error
  *      -2, user aborted
  */
qint32 readMMPktsToEDF(HANDLE *comportptr, qint32 fromPktNum, qint32 toPktNum)
{
    OPIPKT_t ucOpipkt, mmOpipkt, tmpOpipkt;
    qint32 i, j, k, pktCt, oldPktCt, consecBlanks, currpl, pktNum;
    QString outfileName, tagfileName;
    QFile *outfilep;
    QDataStream *outstrp;
    quint8 tspdn;
    qint32 pdnSlot;
    qint64 mmFirstFrmTS, mmRefEpochMSecs;
    QDateTime mmFirstFrmDT;
    QTime killTime;
    QVector<qint64> tsQV(0);
    QVector<quint8> skpQV(0);
    QVector<quint8> batQV(0);
    QVector<qint16> adcQV(0);
    QVector<qint16> tmpQV(0);
    QVector<qint16> axQV(0);
    QVector<qint16> ayQV(0);
    QVector<qint16> azQV(0);
    QVector<qint16> sqQV(0);
    QVector<quint8> edQV(0);
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
    QString localPatientID;
    QString localRecordID;
    qint32 currMonth;
    qint64 frmTS, prevFrmTS, absFirstTS, absLastTS;
    quint8 pdn;
    qint16 adcData[ADCLEN];
    qint32 usedFrmCt;
    qint32 totalDataRecCt, dataRecCt;
    QDateTime stDT, endDT;
    qint32 tagCt;
    ReadMMDialog *rmmWinp;
    bool useProfile;
    QString readName, readDOB, readSex;
    QVector<qint64> tagTSQV(0);
    QVector<QString> tagTextQV(0);
    double beginOffFrms, beginOffFrmsj;
    qint32 beginOffFrmsi;
    QMessageBox msgBox;
    qint32 newstep, azold, axold, ayold, az4old, ax2old, ay2old, ax3old, ay3old, ax4old, ay4old, az8old, az12old, az16old;
    twoDaccelviewer *TDVp;
    QVector <qint16> accxQV,accyQV,acczQV;
    quint8 sampQual;

    accxQV.resize(1);
    accyQV.resize(1);
    acczQV.resize(4);

    if(opiucd_status(comportptr, &ucOpipkt)) return -1;

    if(opiucd_get5mmtsdata(comportptr, fromPktNum, &mmOpipkt) == -1)   return -1;
    else if(mmOpipkt.payload[1] == 0) // means no data in packet
        return 0;

    // got some data so check first one to setup
    tspdn = mmOpipkt.payload[8];

    for(pdnSlot = 1; pdnSlot < PDNLISTLEN; pdnSlot++)
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

    pdnSlot = 0;
    tsQVpp[pdnSlot] = &tsQV;
    skpQVpp[pdnSlot] = &skpQV;
    batQVpp[pdnSlot] = &batQV;
    adcQVpp[pdnSlot] = &adcQV;
    tmpQVpp[pdnSlot] = &tmpQV;
    axQVpp[pdnSlot] = &axQV;
    ayQVpp[pdnSlot] = &ayQV;
    azQVpp[pdnSlot] = &azQV;
    sqQVpp[pdnSlot] = &sqQV;
    edQVpp[pdnSlot] = &edQV;
    absFirstTS = 0;
    absLastTS = ((qint64) 1 << 48) - 1;

    mmFirstFrmTS = ((qint64) mmOpipkt.payload[2] << 40) + ((qint64) mmOpipkt.payload[3] << 32) +
            ((qint64) mmOpipkt.payload[4] << 24) + ((qint64) mmOpipkt.payload[5] << 16) +
            ((qint64) mmOpipkt.payload[6] << 8) + ((qint64) mmOpipkt.payload[7]);
    mmRefEpochMSecs = mmFirstFrmTS*1000/UCRTCFREQ;
    // Conversion to QDateTime, ref date & time for all sensors is 2012/sep/28 08:00:00.000
    mmFirstFrmDT = QDateTime::fromString("20120928080000000","yyyyMMddhhmmsszzz").addMSecs(mmRefEpochMSecs);

    // Need floor(time) for file since fill in, toString function automatically does this
    outfileName = QString("D%1_%2").arg(QDateTime::fromMSecsSinceEpoch(mmFirstFrmDT.toMSecsSinceEpoch()).toString("yyyyMMdd_hhmmss")).arg(tspdn);
    outfileName.append(".edf");

    // scan the memory to determine roughly how full it is
    for(i = 1; i < 20; i++)
    {
        pktNum = (toPktNum-fromPktNum)*i/20;
        if(opiucd_get5mmtsdata(comportptr, pktNum+fromPktNum, &tmpOpipkt) == -1)   return -1;
        if(tmpOpipkt.payload[1] == 0) break; // found end
    }

    // options dialog
    rmmWinp = new ReadMMDialog(&outfileName, fromPktNum, (toPktNum-fromPktNum)*i/20, &tagfileName, &useProfile);
    if(rmmWinp->exec() == QDialog::Rejected)
    {
        delete rmmWinp;
        return -2;
    }

    TDVp = new twoDaccelviewer(true,(quint8) tspdn);

    TDVp->livedisplaylayout();

    // tag stuff
    tagCt = 0;
    if(!tagfileName.isEmpty())
        tagCt = tagFileRead(tagfileName, &tagTSQV, &tagTextQV); // read in tags
    if(tagCt > 0) rmmWinp->setTagStatText("Tag file read");
    else if(tagCt == 0) rmmWinp->setTagStatText("No tags found");
    else if(tagCt < 0) rmmWinp->setTagStatText("Tag file error");
    rmmWinp->show();

    outfilep = new QFile(outfileName);
    if(outfilep->exists())
    {
        msgBox.setWindowTitle("Out File Exists");
        msgBox.setText("Out file already exists in selected directory.");
        msgBox.setInformativeText("Do you want to overwrite?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        if(msgBox.exec() == QMessageBox::Cancel)
        {
            delete outfilep;
            delete rmmWinp;
            return -2;
        }
    }
    if(!outfilep->open(QIODevice::WriteOnly))
    {
        delete outfilep;
        delete rmmWinp;
        return -1;
    }
    outstrp = new QDataStream(outfilep);
    outstrp->setByteOrder(QDataStream::LittleEndian);

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

    localRecordID = QString("X X %1_OPITS%2").arg(localUTCOffset()).arg(tspdn,3,10,QChar('0'));
    localRecordID.prepend(mmFirstFrmDT.toString("-yyyy "));
    currMonth = mmFirstFrmDT.date().month();      // have to do this because of QDateTime letters being 2 chars
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
    localRecordID.prepend(mmFirstFrmDT.toString("dd-")).prepend("Startdate ");

    // write EDF header, will need to come back later to adjust number of data records
    // which are unknown (-1) at this point
    // also pass the dataType and use the write header if need to write events
    edfDhdropiwrite(outstrp, &localPatientID, &localRecordID, &mmFirstFrmDT, -1);

    // write first 5 packets that already read out
    tmpOpipkt.dataCode = 0x2A;
    tmpOpipkt.payload[0] = 0x01;
    currpl = 1; // current place in packet
    prevFrmTS = 0;  // start with 0 for previous time
    TDVp->setVisible(true);
    TDVp->setGeometry(rmmWinp->x()+rmmWinp->width()+25, rmmWinp->y()+30, TDVp->width(), TDVp->height()); // hardwired since don't have parent&this
    for(i = 0; i < 5; i++)
    {        
        if(mmOpipkt.payload[currpl] == 0)   // skip this packet since empty
        {
            currpl++;
            continue;
        }
        tmpOpipkt.length = mmOpipkt.payload[currpl]+1;    // add 1 for sub datacode @payload[0]
        for(j = 1; j < tmpOpipkt.length; j++) tmpOpipkt.payload[j] = mmOpipkt.payload[j+currpl];

        if((tmpOpipkt.length != (TSFRMLEN-1)) && (tmpOpipkt.length != (TSFRMLEN-5))) continue; // wrong packet size
        frmTS = (((qint64) tmpOpipkt.payload[WFRMHDRLEN-1]) << 40) + (((qint64) tmpOpipkt.payload[WFRMHDRLEN-1+1]) << 32) +
                (((qint64) tmpOpipkt.payload[WFRMHDRLEN-1+2]) << 24) + ((tmpOpipkt.payload[WFRMHDRLEN-1+3]) << 16) +
                (tmpOpipkt.payload[WFRMHDRLEN-1+4] << 8) + (tmpOpipkt.payload[WFRMHDRLEN-1+5]);
         if(frmTS < prevFrmTS)     // make sure monotonically increasing
        {
            currpl += mmOpipkt.payload[currpl]+1; // go to next packet
            continue;
        }
        pdn = tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN];
        if (pdn != tspdn)
        {
            currpl += mmOpipkt.payload[currpl]+1; // go to next packet
            continue;
        }
        // if reached here, then this packet will be used
        prevFrmTS = frmTS;
        tsQV.append(frmTS);
        skpQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+1] >> 7);
        batQV.append(tmpOpipkt.payload[WFRMHDRLEN+TSLEN+1] & 0x01);
        if(tmpOpipkt.length == (TSFRMLEN-1))    // adcdata of 64 case
        {
            // get all data out into proper size variables
            for(k = 0; k < ADCLEN; k++)
                adcData[k] = (tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*k] << 8) + tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*k+1];
            tmpQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN] << 4);
            axQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN] << 8);
            accxQV[0]=(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN] << 8);
            ayQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+1] << 8);
            accyQV[0]=(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+1] << 8);
            for(k = 0; k < ACCLEN; k++)
            {
                azQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+2+k] << 8);
                acczQV[k]=(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+2+k] << 8);
            }
            edQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+ACCDLEN]);
            sampQual = getSampQual(adcData);    // func. deletes sampQual info from adcData[0]
            // write adc data last because getSampQual modified adcData array
            for(k = 0; k < ADCLEN; k++) adcQV.append(adcData[k]);
        }
        else    // adcdata of 62 case
        {
            for (k = 0; k < ADCLEN-2; k++)
                adcData[k] = (tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*k] << 8) + tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*k+1];
            tmpQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)] << 4);
            axQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN] << 8);
            accxQV[0]=(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN] << 8);
            ayQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+1] << 8);
            accyQV[0]=(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+1] << 8);
            for(k = 0; k < ACCLEN; k++)
            {
                azQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+2+k] << 8);
                acczQV[k]=(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+2+k] << 8);
            }
            edQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+ACCDLEN]);
            sampQual = getSampQual(adcData);    // func. deletes sampQual info from adcData[0]
            // write adc data last because getSampQual modified adcData array
            for(k = 0; k < ADCLEN-2; k++) adcQV.append(adcData[k]);
        }
        if(sampQual>0)
        {
            sqQV.append(-1000*sampQual); //*-1.5dB
            qDebug() << sampQual;
        }
        else
            sqQV.append(calcAct(&axQV, &ayQV, &azQV, &newstep, &azold, &axold, &ayold, &az4old, &ax2old, &ay2old, &ax3old, &ay3old, &ax4old, &ay4old, &az8old, &az12old, &az16old));

        currpl += mmOpipkt.payload[currpl]+1; // go to next packet
        TDVp->livedisplayroutine(&accxQV,&accyQV,&acczQV);
    }

    // First time updating posture viewer
    //TDVp->setVisible(true);
    //TDVp->livedisplayroutine(&axQV, &ayQV, &azQV);

    // remove tags that are before the first timestamp
    while(tagTSQV.size() > 0)
    {
        if(tagTSQV.at(0) >= tsQV.at(0)) break;  // don't need to remove anymore since
        else                                    // tag text will be monotonically increasing
        {
            tagTSQV.remove(0,1);
            tagTextQV.remove(0,1);
        }
    }
    if(tsQV.size() > 0)
    {
        tagTSQV.prepend(tsQV.at(0));
        tagTextQV.prepend("DataStart");
    }

    // read rest of the packets until get 5 consecutive packets that have no data
    consecBlanks = 0;
    pktCt = 5;  // read 5 packets
    oldPktCt = 5;
    totalDataRecCt = 0; // no data written to edf yet
    usedFrmCt = 5;

    stDT = QDateTime::currentDateTime();
    for(i = fromPktNum+5; i < toPktNum; i += 5)
    {
        if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))
        {
            killTime = QTime::currentTime().addMSecs(100);
            while(killTime > QTime::currentTime());
            if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))
            {
                killTime = QTime::currentTime().addMSecs(100);
                while(killTime > QTime::currentTime());
                if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))
                {
                    killTime = QTime::currentTime().addMSecs(100);
                    while(killTime > QTime::currentTime());
                    if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))
                    {
                        killTime = QTime::currentTime().addMSecs(100);
                        while(killTime > QTime::currentTime());
                        if(opiucd_get5mmtsdata(comportptr, i, &mmOpipkt))   // try 5 times before aborting
                        {
                            outfilep->close();
                            delete outstrp;
                            delete outfilep;
                            delete rmmWinp;
                            return -1;
                        }
                    }
                }
            }
        }
        // write 5 packets or skip
        tmpOpipkt.dataCode = 0x2A;
        tmpOpipkt.payload[0] = 0x01;
        currpl = 1; // current place in packet



        for(j = 0; j < 5; j++)
        {
            if(mmOpipkt.payload[currpl] == 0)   // skip this packet since empty
            {
                currpl++;
                consecBlanks++;
                continue;
            }
            consecBlanks = 0;
            tmpOpipkt.length = mmOpipkt.payload[currpl]+1;    // add 1 for sub datacode @payload[0]
            for(k = 1; k < tmpOpipkt.length; k++) tmpOpipkt.payload[k] = mmOpipkt.payload[k+currpl];

            if((tmpOpipkt.length != (TSFRMLEN-1)) && (tmpOpipkt.length != (TSFRMLEN-5))) continue; // wrong packet size
            frmTS = (((qint64) tmpOpipkt.payload[WFRMHDRLEN-1]) << 40) + (((qint64) tmpOpipkt.payload[WFRMHDRLEN-1+1]) << 32) +
                    (((qint64) tmpOpipkt.payload[WFRMHDRLEN-1+2]) << 24) + ((tmpOpipkt.payload[WFRMHDRLEN-1+3]) << 16) +
                    (tmpOpipkt.payload[WFRMHDRLEN-1+4] << 8) + (tmpOpipkt.payload[WFRMHDRLEN-1+5]);
            if(frmTS < prevFrmTS)     // make sure monotonically increasing
            {
                currpl += mmOpipkt.payload[currpl]+1; // go to next packet
                rmmWinp->setStatusText(QString("Error: TimeStamp not increasing"));
                qApp->processEvents();
                continue;
            }
            pdn = tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN];
            if (pdn != tspdn)
            {
                currpl += mmOpipkt.payload[currpl]+1; // go to next packet
                rmmWinp->setStatusText(QString("Error: Different device data"));
                qApp->processEvents();
                continue;
            }
            // if reached here, then this packet will be used
            usedFrmCt++;

            prevFrmTS = frmTS;
            tsQV.append(frmTS);
            skpQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+1] >> 7);
            batQV.append(tmpOpipkt.payload[WFRMHDRLEN+TSLEN+1] & 0x01);

            if(tmpOpipkt.length == (TSFRMLEN-1))    // adcdata of 64 case
            {
                // get all data out into proper size variables
                for(k = 0; k < ADCLEN; k++)
                    adcData[k] = (tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*k] << 8) + tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*k+1];
                tmpQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN] << 4);
                axQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN] << 8);
                //accxQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN] << 8);
                ayQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+1] << 8);
                //accyQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+1] << 8);
                for(k = 0; k < ACCLEN; k++)
                {
                    azQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+2+k] << 8);
                    //acczQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+2+k] << 8);
                }
                edQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*ADCLEN+TMPLEN+ACCDLEN]);
                sampQual = getSampQual(adcData);    // func. also deletes sampQual info from adcData[0]

                // write adc data last because getSampQual modified adcData array
                for(k = 0; k < ADCLEN; k++) adcQV.append(adcData[k]);
            }
            else    // adcdata of 62 case
            {
                for (k = 0; k < ADCLEN-2; k++)
                    adcData[k] = (tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*k] << 8) + tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*k+1];
                tmpQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)] << 4);
                axQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN] << 8);
                //accxQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN] << 8);
                ayQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+1] << 8);
                //accyQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+1] << 8);
                for(k = 0; k < ACCLEN; k++)
                {
                    azQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+2+k] << 8);
                    //acczQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+2+k] << 8);
                }
                edQV.append(tmpOpipkt.payload[WFRMHDRLEN-1+TSLEN+WLFRMHDRLEN+2*(ADCLEN-2)+TMPLEN+ACCDLEN]);
                sampQual = getSampQual(adcData);    // func. also deletes sampQual info from adcData[0]

                // write adc data last because getSampQual modified adcData array
                for(k = 0; k < ADCLEN-2; k++) adcQV.append(adcData[k]);
            }
            if(sampQual>0)
            {
                sqQV.append(-1000*sampQual); //*-1.5dB
            }
            else
                sqQV.append(calcAct(&axQV, &ayQV, &azQV, &newstep, &azold, &axold, &ayold, &az4old, &ax2old, &ay2old, &ax3old, &ay3old, &ax4old, &ay4old, &az8old, &az12old, &az16old));

            pktCt++;
            currpl += mmOpipkt.payload[currpl]+1; // go to next packet
            //TDVp->livedisplayroutine(&accxQV,&accyQV,&acczQV);
        }        

        // check if enough data to write into EDF, adc will always have the least
        if(adcQV.size() > ADCLEN*FRMSPERSEC*EDFDRDURSEC)
        {
            // process vectors to add missing frames
            procQVs(&absFirstTS, &absLastTS, tsQVpp, skpQVpp, batQVpp, adcQVpp,
                    tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);

            // add data at beginning (first frame) to fill up to floor(second) (within 2 ms)
            if(!totalDataRecCt)
            {
                // difference between floored exact second and first time in frames
                beginOffFrms = ((double) (mmFirstFrmDT.toMSecsSinceEpoch() % 1000))*512.0/1000/ADCLEN;
                beginOffFrmsi = (qint32) (beginOffFrms);
                beginOffFrmsj = beginOffFrms-((double) beginOffFrmsi);
                // put in default values for beginning in constant frame units
                for(; beginOffFrmsi > 0; beginOffFrmsi--)
                {
                    tsQV.prepend(mmFirstFrmTS-512*beginOffFrmsi);
                    skpQV.prepend(0);
                    batQV.prepend(1);
                    sqQV.prepend(0);
                    edQV.prepend(0);
                    for(k = 0; k < ADCLEN; k++)
                        adcQV.prepend(0);
                    for(k = 0; k < TMPLEN; k++)
                        tmpQV.prepend(1024);   // temp ~25
                    for(k = 0; k < ACCLEN/4; k++)
                    {
                        axQV.prepend(0);
                        ayQV.prepend(0);
                    }
                    for(k = 0; k < ACCLEN; k++)
                        azQV.prepend(0);
                }

                // add less than a frame data
                if(beginOffFrmsj > 0.5)
                {
                    tsQV.prepend(mmFirstFrmTS-512*(beginOffFrmsi+1));
                    skpQV.prepend(0);
                    batQV.prepend(1);
                    sqQV.prepend(0);
                    edQV.prepend(0);
                    tmpQV.prepend(1024);   // temp ~25
                    axQV.prepend(0);
                    ayQV.prepend(0);
                }
                for(k = 0; k < ((qint32) (beginOffFrmsj*ADCLEN)); k++)
                    adcQV.prepend(0);
                for(k = 0; k < ((qint32) (beginOffFrmsj*ACCLEN)); k++)
                    azQV.prepend(0);

                // fix up tag times by adding
                for(k = 0; k < tagTSQV.size(); k++)
                    tagTSQV.replace(k, tagTSQV.at(k)+((qint64) (beginOffFrms*512.0)));
            }

            // write to file, add any tags that are within the timestamp ranges
            dataRecCt = edfDwrite(outstrp, &adcQV, &tmpQV, &axQV, &ayQV, &azQV, &sqQV, totalDataRecCt, mmFirstFrmTS, &tsQV, &tagTSQV, &tagTextQV);
            totalDataRecCt += dataRecCt;


            // update posture viewer
            if(TDVp->isVisible())
            TDVp->MMfastdisplayroutine(&axQV,&ayQV,&azQV,dataRecCt);

            // clip off the parts it wrote
            tsQV.remove(0, dataRecCt*1*FRMSPERSEC*EDFDRDURSEC);
            skpQV.remove(0, dataRecCt*1*FRMSPERSEC*EDFDRDURSEC);
            batQV.remove(0, dataRecCt*1*FRMSPERSEC*EDFDRDURSEC);
            adcQV.remove(0, dataRecCt*ADCLEN*FRMSPERSEC*EDFDRDURSEC);
            tmpQV.remove(0, dataRecCt*TMPLEN*FRMSPERSEC*EDFDRDURSEC);
            axQV.remove(0, dataRecCt*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC);
            ayQV.remove(0, dataRecCt*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC);
            azQV.remove(0, dataRecCt*ACCLEN*FRMSPERSEC*EDFDRDURSEC);
            sqQV.remove(0, dataRecCt*1*FRMSPERSEC*EDFDRDURSEC);
            edQV.remove(0, dataRecCt*1*FRMSPERSEC*EDFDRDURSEC);
        }

        if(consecBlanks >= 5) break;    // stop reading since no more

        if(pktCt > (200+oldPktCt))
        {
            rmmWinp->setProgBarValue(pktCt);
            rmmWinp->setStatusText(QString("Read %1 packets...").arg(pktCt));
            qApp->processEvents();    
            oldPktCt = pktCt;
        }
        if(rmmWinp->isHidden() || rmmWinp->abortedFlag) break;
    }

    endDT = QDateTime::currentDateTime();

    // write last DataEnd tag
    tagTSQV.append(frmTS+512+((qint64) (beginOffFrms*512.0)));
    tagTextQV.append("DataEnd");

    // put in default values for last record
    while(tsQV.size() < 1*FRMSPERSEC*EDFDRDURSEC)
    {
        tsQV.append(tsQV.at(tsQV.size()-1)+512); // each frame is ~512ticks later
        skpQV.append(0);
        batQV.append(1);
        sqQV.append(0);
        edQV.append(0);
    }
    while(adcQV.size() < ADCLEN*FRMSPERSEC*EDFDRDURSEC)
        adcQV.append(0);
    while(tmpQV.size() < TMPLEN*FRMSPERSEC*EDFDRDURSEC)
        tmpQV.append(1024);   // temp ~25
    while(axQV.size() < ACCLEN/4*FRMSPERSEC*EDFDRDURSEC)
    {
        axQV.append(0);
        ayQV.append(0);
    }
    while(azQV.size() < ACCLEN*FRMSPERSEC*EDFDRDURSEC)
        azQV.append(0);

    procQVs(&absFirstTS, &absLastTS, tsQVpp, skpQVpp, batQVpp, adcQVpp,
            tmpQVpp, axQVpp, ayQVpp, azQVpp, sqQVpp, edQVpp);
    // write to file, add any tags that are within the timestamp ranges
    dataRecCt = edfDwrite(outstrp, &adcQV, &tmpQV, &axQV, &ayQV, &azQV, &sqQV, totalDataRecCt, mmFirstFrmTS, &tsQV, &tagTSQV, &tagTextQV);
    totalDataRecCt += dataRecCt;

    // write correct number of data now
    outfilep->reset();
    edfDhdropiwrite(outstrp, &localPatientID, &localRecordID, &mmFirstFrmDT, totalDataRecCt);

    outfilep->close();
    delete outstrp;
    delete outfilep;

    // status message
    rmmWinp->endState();
    rmmWinp->setStatusText(QString("Read %1 packets in %2 seconds").arg(pktCt).arg((endDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));
    rmmWinp->exec();

    delete rmmWinp;

    delete TDVp;

    return pktCt;

}


/***
  * Calculate activity and return a quint8 (compat. with prev. def.)
  */
qint16 calcAct(QVector<qint16> *axQVp, QVector<qint16> *ayQVp, QVector<qint16> *azQVp,
               qint32 *newstepp, qint32 *azoldp, qint32 *axoldp, qint32 *ayoldp, qint32 *az4oldp, qint32 *ax2oldp, qint32 *ay2oldp,qint32 *ax3oldp, qint32 *ay3oldp, qint32 *ax4oldp, qint32 *ay4oldp, qint32 *az8oldp,qint32 *az12oldp, qint32 *az16oldp)
{
    qint64 i,j, fastdx, fastdy, fastdz, slowdx, slowdy, slowdz, sfastdz;
    qint64 fastAct, slowAct, sfastAct, activ, z4sum, x4mean, y4mean, z16mean, xsum, ysum;
    qint16 retval;
    slowdx = 0; //init
    slowdy = 0; //init
    slowdz = 0; //init
    fastdz = 0; //init
    sfastAct = 0;
    z4sum = 0; //init
    x4mean = 0;
    y4mean = 0;
    z16mean = 0;
    xsum = 0;
    ysum = 0;
    j = (azQVp->size()); //max index
    sfastdz = (azQVp->at(j-4)- *azoldp); //current packet
    sfastAct += sfastdz*sfastdz;
    sfastdz = (azQVp->at(j-3)-azQVp->at(j-4)); //current packet
    sfastAct += sfastdz*sfastdz;
    sfastdz = (azQVp->at(j-2)-azQVp->at(j-3)); //current packet
    sfastAct += sfastdz*sfastdz;
    sfastdz = (azQVp->at(j-1)-azQVp->at(j-2)); //current packet
    sfastAct += sfastdz*sfastdz;
    xsum = axQVp->at(j/4-1); //current packet
    ysum = ayQVp->at(j/4-1); //current packet
    fastdx = xsum - *axoldp; //current vs. last packet
    fastdy = ysum - *ayoldp; //current vs. last packet
    z4sum += (azQVp->at(j-1));
    z4sum += (azQVp->at(j-2));
    z4sum += (azQVp->at(j-3));
    z4sum += (azQVp->at(j-4));
    fastdz = (z4sum - *az4oldp)/4; //average of 4
    fastAct = fastdx*fastdx + fastdy*fastdy + fastdz*fastdz; //sum of square
    slowdz = (z4sum - *az16oldp)/16; //average of 4, 4 packets apart
    x4mean = (*axoldp + *ax2oldp + *ax3oldp + *ax4oldp)/4; //mean of 4 packets
    y4mean = (*ayoldp + *ay2oldp + *ay3oldp + *ay4oldp)/4; //mean of 4 packets
    z16mean = (*az4oldp+ *az8oldp+ *az12oldp+ *az16oldp)/16; //mean of 4 packets
    slowdx = (xsum - *ax4oldp)/4; //4 packets apart
    slowdy = (ysum - *ay4oldp)/4; //4 packets apart
    slowAct = slowdx*slowdx + slowdy*slowdy + slowdz*slowdz; //sum of square

    if(slowAct<=257000) slowAct=0; //noise reduction
    if(fastAct<=257000) fastAct=0; //noise reduction
    if(sfastAct<=257000) sfastAct=0; //noise reduction
    activ = (ACTOFFSET + (ACTGAIN*(fastAct/FASTACTWEIGHT + slowAct/SLOWACTWEIGHT + sfastAct/SFASTACTWEIGHT))); // activ must be qint16
    if(activ<=1000000) activ=1000000; //set noise floor 256*256*4=256K
    activ = 6553.6*(log10(activ)-6.0);  //log10 50dB dynamic range
    if(activ > 32767) activ = 32767;    // clipping, since return value is qint16
    if(activ < 0) activ = 0; // set floor
    if(j<16 && activ>6553.6) activ=6553.6; //block initial spike >10db
    //check step using zero-crossing: newstepp=-1(neg domain), +1(pos domain), +2(pos transition detected=>add 1 step)
    for(i=4; i>0; i--)
    {
        if((azQVp->at(j-i)-xsum+ysum - z16mean+x4mean-y4mean)> 3000) //positive with hysteresis
        {
            if((*newstepp)==-1) *newstepp=2; //advance 1 step
            else if(*newstepp==-2) *newstepp=3; //advance 2 steps
        }
        else if((azQVp->at(j-i)-xsum+ysum - z16mean+x4mean-y4mean) < -3000) //negative with hysteresis
        {
            if((*newstepp)==1) *newstepp=-1; //negative
            else if(*newstepp==2) *newstepp=-2; //negative
        }
    }
    *azoldp = azQVp->at(j-1); //new value
    *az16oldp = *az12oldp; //new value
    *az12oldp = *az8oldp; //new value
    *az8oldp = *az4oldp; //new value
    *az4oldp = z4sum; //new value
    *ax4oldp = *ax3oldp; //new value
    *ax3oldp = *ax2oldp; //new value
    *ax2oldp = *axoldp; //new value
    *axoldp = xsum; //new value
    *ay4oldp = *ay3oldp; //new value
    *ay3oldp = *ay2oldp; //new value
    *ay2oldp = *ayoldp; //new value
    *ayoldp = ysum; //new value

    retval = (qint16) activ;
    return retval; //no motion <5db; slow(low) 5~10db; walk(mid) 10~20db; run(hi) 20~28db; shake(intense) >28db;
}


/***
  * Read in tag file with tagfileName sorted by timestamps
  * into specified qvector pointers
  * Inputs:
  *     tagfileName, filename of tag file
  *     tagTSQVp, pointer to qvector of timestamps of tags
  *     tagTextQVp, pointer to qvector of text of tags
  * Returns:
  *     non-negative number of tags read out
  *     -1, if some kind of error
  */
qint32 tagFileRead(QString tagfileName, QVector<qint64> *tagTSQVp, QVector<QString> *tagTextQVp)
{
    QFile *tagfilep;
    QTextStream *tagstrp;
    qint32 i, tagCt;
    QString tempqstr, tempqstr2, fileNameTime, firstTagTime;
    qint64 startTS, tempi64;
    QStringList firstTagQSL;

    // check if tag file by seeing "TAG" is in right place
    tempqstr = tagfileName;
    tempqstr.remove(0,tempqstr.size()-7).remove(3,4);

    if(QString::compare(tempqstr,QString("TAG"),Qt::CaseSensitive))
    {
        return -1;
    }

    // get the start date/time from the filename
    fileNameTime = tagfileName;
    fileNameTime.remove(0,fileNameTime.size()-23).remove(15,8);
    startTS = (QDateTime::fromString(fileNameTime,"yyyyMMdd_HHmmss").toMSecsSinceEpoch() - QDateTime::fromString("20120928080000000","yyyyMMddHHmmsszzz").toMSecsSinceEpoch())*UCRTCFREQ/1000;

    // open file
    tagfilep = new QFile(tagfileName);
    if (!tagfilep->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        delete tagfilep;
        return -1;
    }
    tagstrp = new QTextStream(tagfilep);

    tagCt = 0;
    while(!tagstrp->atEnd())
    {
        tempqstr = tagstrp->readLine();
        if(tagCt == 0)
        {
            firstTagQSL = tempqstr.split(QRegExp("\\s+"));
            if(firstTagQSL.isEmpty() || QString::compare(firstTagQSL.last().remove(0,1), fileNameTime))
            {
                tagfilep->close();
                delete tagstrp;
                delete tagfilep;
                return -1;
            }
        }
        QTextStream(&tempqstr) >> tempqstr2;
        tempqstr.remove(0, tempqstr2.size()+1); // remove relative time and whitespace too
        tempi64 = (qint64) (tempqstr2.toFloat()*1000);
        if(tempqstr.isEmpty()) continue;    // if no tag text, then skip
        tempi64 = tempi64*UCRTCFREQ/1000+startTS;
        // put tag into right chronological place
        for(i = 0; i < tagTSQVp->size(); i++)
        {
            if(tempi64 < tagTSQVp->at(i))
            {
                tagTSQVp->insert(i,tempi64);
                tagTextQVp->insert(i,tempqstr);
                break;
            }
        }
        if(i >= tagTSQVp->size()) // append this entry (works for case where size=0)
        {
            tagTSQVp->append(tempi64);
            tagTextQVp->append(tempqstr);
        }
        tagCt++;
    }

    // cleanup
    tagfilep->close();
    delete tagstrp;
    delete tagfilep;

    return tagCt;
}


/***
  * Write EDF header especially for opi raw data files, 8 sec data record duration
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
void edfDhdropiwrite(QDataStream *out, QString *lpidp, QString *lridp,
                     QDateTime *startDTp, qint32 numDataRecs)
{
    QString tempstr;

    out->writeRawData("0       ", 8);     // edf version of data format
    out->writeRawData(lpidp->toUtf8().leftJustified(80,' ').data(),80);   // local patient identification
    out->writeRawData(lridp->toUtf8().leftJustified(80,' ').data(),80);   // local recording identification
    out->writeRawData(startDTp->toString("dd.MM.yyhh.mm.ss").toUtf8().data(),16); // startdate and starttime
    out->writeRawData("2048    ", 8);     // number of header bytes (256+7signals*256)
    out->writeRawData(QByteArray("EDF+C").leftJustified(44,' ').data(),44); // format type (reserved)
    out->writeRawData(QString("%1").arg(numDataRecs).toUtf8().leftJustified(8,' ').data(),8);  // number of data records
    out->writeRawData("8       ", 8);     // duration of a data record in seconds
    out->writeRawData("7   ", 4);     // number of signals: adc, accX, accY, accZ, temp, EDF annotations

    // signal labels
    out->writeRawData("ADC             ", 16);   // maybe change if know type later on
    out->writeRawData("Accel. X-axis   ", 16);
    out->writeRawData("Accel. Y-axis   ", 16);
    out->writeRawData("Accel. Z-axis   ", 16);
    out->writeRawData("Temperature     ", 16);
    out->writeRawData("Activity        ", 16);
    out->writeRawData("EDF Annotations ", 16);

    // transducer type
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);

    // physical dimensions
    out->writeRawData("uV      ", 8);
    out->writeRawData("g       ", 8);
    out->writeRawData("g       ", 8);
    out->writeRawData("g       ", 8);
    out->writeRawData("degreeC ", 8);
    out->writeRawData("dB      ", 8);
    out->writeRawData("        ", 8);

    // physical mins and maxs
    out->writeRawData("-800    ", 8);
    out->writeRawData("-2      ", 8);
    out->writeRawData("-2      ", 8);
    out->writeRawData("-2      ", 8);
    out->writeRawData("-47     ", 8);
    out->writeRawData("-50     ", 8);
    out->writeRawData("-1      ", 8);

    out->writeRawData("800     ", 8);
    out->writeRawData("2       ", 8);
    out->writeRawData("2       ", 8);
    out->writeRawData("2       ", 8);
    out->writeRawData("241     ", 8);
    out->writeRawData("50      ", 8);
    out->writeRawData("1       ", 8);

    // digital mins and maxs
    out->writeRawData("-20480  ", 8);
    out->writeRawData("-32768  ", 8);
    out->writeRawData("-32768  ", 8);
    out->writeRawData("-32768  ", 8);
    out->writeRawData("0       ", 8);
    out->writeRawData("-32768   ", 8);
    out->writeRawData("-32768  ", 8);

    out->writeRawData("20480   ", 8);
    out->writeRawData("32767   ", 8);
    out->writeRawData("32767   ", 8);
    out->writeRawData("32767   ", 8);
    out->writeRawData("4080    ", 8);
    out->writeRawData("32767   ", 8);
    out->writeRawData("32767   ", 8);

    // prefiltering
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);
    out->writeRawData(QByteArray(" ").leftJustified(80,' ').data(),80);

    // number of samples in each data record (8s)
    out->writeRawData("4096    ", 8);
    out->writeRawData("64      ", 8);
    out->writeRawData("64      ", 8);
    out->writeRawData("256     ", 8);
    out->writeRawData("64      ", 8);
    out->writeRawData("64      ", 8);
    out->writeRawData("30      ", 8);

    // reserved fields
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
    out->writeRawData(QByteArray(" ").leftJustified(32,' ').data(),32);
}


qint16 wirelessaccscale(int wirelessacc)
{
    double scale=(double)32767/(double)127;
    return (qint16)(wirelessacc*scale);
}


/***
  * Need to know beforedhand knowledge how data is formatted in different streams
  * Will alter temperature stream for averaging so need that flag
  * Returns number of data records written according to opi EDF format
  */
qint32 edfDwrite(QDataStream *outstrp, QVector<qint16> *adcQVp,
                 QVector<qint16> *tmpQVp, QVector<qint16> *axQVp,
                 QVector<qint16> *ayQVp, QVector<qint16> *azQVp,
                 QVector<qint16> *sqQVp, qint32 startDataRecCt,
                 qint64 firstFrmTS, QVector<qint64> *tsQVp,
                 QVector<qint64> *tagTSQVp, QVector<QString> *tagTextQVp)
{
    bool noMore;    // indicate if no more data to write into data records
    qint32 i, j, dataRecordCt;
    QProgressDialog progQPD("Writing EDF file",QString(),0,azQVp->size()/(ACCLEN*FRMSPERSEC*EDFDRDURSEC));
    QByteArray tempQBA;
    progQPD.setWindowModality(Qt::WindowModal);
    progQPD.setMinimumDuration(3000);
    QString tempQS;

    // Write EDF in data record segments
    noMore = false;
    dataRecordCt = 0;

    for(j = 0; j < tmpQVp->size()/(FRMSPERSEC*EDFDRDURSEC); j++)
    {
        if((dataRecordCt % 10) == 0)
        {
            progQPD.setValue(dataRecordCt);
            qApp->processEvents();
        }
        // make sure there is enough data for another data record, otherwise get out
        if((((j+1)*ADCLEN*FRMSPERSEC*EDFDRDURSEC-1) > adcQVp->size()) ||
                (((j+1)*TMPLEN*FRMSPERSEC*EDFDRDURSEC-1) > tmpQVp->size()) ||
                (((j+1)*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC-1) > axQVp->size()) ||
                (((j+1)*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC-1) > ayQVp->size()) ||
                (((j+1)*ACCLEN*FRMSPERSEC*EDFDRDURSEC-1) > azQVp->size()) ||
                (((j+1)*1*FRMSPERSEC*EDFDRDURSEC-1) > sqQVp->size()))
            break;
        for(i = 0; i < ADCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
            *outstrp << adcQVp->at(j*ADCLEN*FRMSPERSEC*EDFDRDURSEC+i);
        for(i = 0; i < ACCLEN/4*FRMSPERSEC*EDFDRDURSEC; i++)
            *outstrp << axQVp->at(j*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC+i);
        for(i = 0; i < ACCLEN/4*FRMSPERSEC*EDFDRDURSEC; i++)
            *outstrp << ayQVp->at(j*ACCLEN/4*FRMSPERSEC*EDFDRDURSEC+i);
        for(i = 0; i < ACCLEN*FRMSPERSEC*EDFDRDURSEC; i++)
            *outstrp << azQVp->at(j*ACCLEN*FRMSPERSEC*EDFDRDURSEC+i);
        for(i = 0; i < TMPLEN*FRMSPERSEC*EDFDRDURSEC; i++)
            *outstrp << tmpQVp->at(j*TMPLEN*FRMSPERSEC*EDFDRDURSEC+i);
        for(i = 0; i < 1*FRMSPERSEC*EDFDRDURSEC; i++)
        {
            *outstrp << sqQVp->at(j*1*FRMSPERSEC*EDFDRDURSEC+i);
        }
        // EDF Annotations, max of 2 annotations per data record, each with 52 total chars
        tempQBA.clear();
        tempQBA.append(QString("+%1").arg((dataRecordCt+startDataRecCt)*EDFDRDURSEC).toUtf8().append(QChar(20)).append(QChar(20)).append(QChar(0)));
        for(i = 0; i < 2; i++)
        {
            if((tagTSQVp->size() > 0) && (tagTSQVp->at(0) <= (tsQVp->at(j*1*FRMSPERSEC*EDFDRDURSEC) + 1*FRMSPERSEC*EDFDRDURSEC*512)))
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


// Write filter tags that are not in selected time range
void procTagQVs(qint64 *filtFirstFrmTSp, qint64 *filtLastFrmTSp,
                QString tagfileName, QVector<qint64> *tagTSQVp,
                QVector<QString> *tagTextQVp)
{

    // remove tags that are before the first timestamp
    while(tagTSQVp->size() > 0)
    {
        if(tagTSQVp->at(0) >= (*filtFirstFrmTSp-4)) break; //back 4 ticks=1msec @ 4096Hz
                // don't need to remove anymore since
        else                                    // tag text will be monotonically increasing
        {
            tagTSQVp->remove(0,1);
            tagTextQVp->remove(0,1);
        }
    }

    // remove tags that are after last timestamp
    while(tagTSQVp->size() > 0)
    {
        if(tagTSQVp->at(tagTSQVp->size()-1) <= *filtLastFrmTSp) break;
                // don't need to remove anymore since
        else                                    // tag text will be monotonically increasing
        {
            tagTSQVp->remove(tagTSQVp->size()-1,1);
            tagTextQVp->remove(tagTextQVp->size()-1,1);
        }
    }

    // if no tags (filtered out or none to begin with) then don't write file and return
    if(tagTSQVp->size() < 1)
    {
        return;
    }
}


void procMecgQVs(qint64 *firstFrmTSp, qint64 *lastFrmTSp, QVector<qint64> *tsQVpp,
                  QVector<quint8> *skpQVpp, QVector<quint8> *batQVpp,
                  QVector<qint16> *RRQVpp, QVector<qint16> *ampQVpp,
                  QVector<qint16> *LFperQVpp, QVector<qint16> *HFperQVpp, QVector<qint16> *LHRatioQVpp)
{
    qint32  i;
    for(i = tsQVpp->size()-1; i > 0; i--)
    {
        // remove frames at end, have to do one by one because must examine
        // if frame had 62 or 64 adc samples
        if(tsQVpp->at(i) > (*lastFrmTSp + 4))
        {
            tsQVpp->remove(i);
            skpQVpp->remove(i);
            batQVpp->remove(i);
            RRQVpp->remove(i);
            ampQVpp->remove(i);
            LFperQVpp->remove(i);
            HFperQVpp->remove(i);
            LHRatioQVpp->remove(i);
            continue;
        }

        // get out to remove frames in beginning, add fudge factor of 4
        // for rounding issues
        if(tsQVpp->at(i) < (*firstFrmTSp - 4)) break;
        if(i > 0) // check previous one too so that we don't add lots of missing frames
        {
            if(tsQVpp->at(i-1) <= (*firstFrmTSp - 4)) break;
        }
    }
    if(i > 0)
    {
        tsQVpp->remove(0,i);
        skpQVpp->remove(0,i);
        batQVpp->remove(0,i);
        RRQVpp->remove(0,i);
        ampQVpp->remove(0,i);
        LFperQVpp->remove(0,i);
        HFperQVpp->remove(0,i);
        LHRatioQVpp->remove(0,i);
    }
}


void procMeegQVs(qint64 *firstFrmTSp, qint64 *lastFrmTSp, QVector<qint64> *tsQVpp, QVector<quint8> *skpQVpp,
                 QVector<quint8> *batQVpp,
                 QVector<qint16> *M2QVpp, QVector<qint16> *M1QVpp,
                 QVector<qint16> *G2QVpp, QVector<qint16> *G1QVpp,
                 QVector<qint16> *upQVpp, QVector<qint16> *betaQVpp,
                 QVector<qint16> *sigmaQVpp, QVector<qint16> *alphaQVpp,
                 QVector<qint16> *thetaQVpp, QVector<qint16> *deltaQVpp)
{
    qint32 i;

    for(i = tsQVpp->size()-1; i > 0; i--)
    {
        // remove frames at end, have to do one by one because must examine
        // if frame had 62 or 64 adc samples
        if(tsQVpp->at(i) > (*lastFrmTSp + 4))
        {
            tsQVpp->remove(i);
            skpQVpp->remove(i);
            batQVpp->remove(i);
            M2QVpp->remove(i);
            M1QVpp->remove(i);
            G2QVpp->remove(i);
            G1QVpp->remove(i);
            upQVpp->remove(i);
            betaQVpp->remove(i);
            sigmaQVpp->remove(i);
            alphaQVpp->remove(i);
            thetaQVpp->remove(i);
            deltaQVpp->remove(i);
            continue;
        }

        // get out to remove frames in beginning, add fudge factor of 4
        // for rounding issues
        if(tsQVpp->at(i) < (*firstFrmTSp - 4)) break;
        if(i > 0) // check previous one too so that we don't add lots of missing frames
        {
            if(tsQVpp->at(i-1) <= (*firstFrmTSp - 4)) break;
        }
    }
    if(i > 0)
    {
        tsQVpp->remove(0,i);
        skpQVpp->remove(0,i);
        batQVpp->remove(0,i);
        M2QVpp->remove(0,i);
        M1QVpp->remove(0,i);
        G2QVpp->remove(0,i);
        G1QVpp->remove(0,i);
        upQVpp->remove(0,i);
        betaQVpp->remove(0,i);
        sigmaQVpp->remove(0,i);
        alphaQVpp->remove(0,i);
        thetaQVpp->remove(0,i);
        deltaQVpp->remove(0,i);
    }
}


/***
  * Process frames by inserting missing frames if needed and
  * removing start and end frames
  */
void procQVs(qint64 *firstFrmTSp, qint64 *lastFrmTSp,
             QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
             QVector<quint8> **batQVpp, QVector<qint16> **adcQVpp,
             QVector<qint16> **tmpQVpp, QVector<qint16> **axQVpp,
             QVector<qint16> **ayQVpp, QVector<qint16> **azQVpp,
             QVector<qint16> **sqQVpp, QVector<quint8> **edQVpp)
{
    qint32 pdnSlot, i, j;
    qint32 currADCIndex;  // need to keep track since sometimes there is 62 data (usually 64)
    double cntr2FrmRatioSet, delFrmCt;
    qint32 missFrmCt;

    // Set the counter to frame ratio
    cntr2FrmRatioSet = ((double) (64*UCRTCFREQ))/((double) (TSRTCFREQ));

    for(pdnSlot = 0; pdnSlot < PDNLISTLEN; pdnSlot++)
    {
        if(tsQVpp[pdnSlot] == 0) continue;
        if(tsQVpp[pdnSlot]->size() < 2) continue;

        // process all the frames backwards since inserting data into vector
        if(skpQVpp[pdnSlot]->at(skpQVpp[pdnSlot]->size()-1) == 0)
            currADCIndex = adcQVpp[pdnSlot]->size()-ADCLEN;
        else    // skipped sample
            currADCIndex = adcQVpp[pdnSlot]->size()-(ADCLEN-2);
        for(i = tsQVpp[pdnSlot]->size()-1; i > 0; i--)
        {
            // remove frames at end, have to do one by one because must examine
            // if frame had 62 or 64 adc samples
            if(tsQVpp[pdnSlot]->at(i) > (*lastFrmTSp + 4))
            {
                tsQVpp[pdnSlot]->remove(i);
                skpQVpp[pdnSlot]->remove(i);
                batQVpp[pdnSlot]->remove(i);
                adcQVpp[pdnSlot]->remove(currADCIndex, adcQVpp[pdnSlot]->size()-currADCIndex);
                if(skpQVpp[pdnSlot]->at(i-1) == 0)
                    currADCIndex -= ADCLEN;
                else
                    currADCIndex -= (ADCLEN-2);
                tmpQVpp[pdnSlot]->remove(i);
                axQVpp[pdnSlot]->remove(i);
                ayQVpp[pdnSlot]->remove(i);
                azQVpp[pdnSlot]->remove(i*ACCLEN, ACCLEN);
                sqQVpp[pdnSlot]->remove(i);
                edQVpp[pdnSlot]->remove(i);
                continue;
            }

            // get out to remove frames in beginning, add fudge factor of 4
            // for rounding issues
            if(tsQVpp[pdnSlot]->at(i) < (*firstFrmTSp - 4)) break;
            if(i > 0) // check previous one too so that we don't add lots of missing frames
            {
                if(tsQVpp[pdnSlot]->at(i-1) <= (*firstFrmTSp - 4)) break;
            }

            delFrmCt = ((double) (tsQVpp[pdnSlot]->at(i)-tsQVpp[pdnSlot]->at(i-1)))/cntr2FrmRatioSet;
            missFrmCt = ((qint32) (delFrmCt + 0.5))-1;	// for rounding

            if(missFrmCt > 0)  // only fill positive frames, shouldn't be negative since
                               // qvector should only be filled with monotonically increasing time
            {
                //qDebug() << "Missing Frames" << missFrmCt;
                if(missFrmCt > 345600) missFrmCt = 345600; // max is half a day addition
                // insert missing timeslots in array
                tsQVpp[pdnSlot]->insert(i, missFrmCt, tsQVpp[pdnSlot]->at(i-1)+((qint64)cntr2FrmRatioSet));
                for(j = 1; j < missFrmCt; j++)
                {
                    tsQVpp[pdnSlot]->replace(i+j, tsQVpp[pdnSlot]->at(i+j-1)+((qint64)cntr2FrmRatioSet));
                }
                skpQVpp[pdnSlot]->insert(i, missFrmCt, 0);
                batQVpp[pdnSlot]->insert(i, missFrmCt, 1);
                adcQVpp[pdnSlot]->insert(currADCIndex, missFrmCt*ADCLEN, 0);
                tmpQVpp[pdnSlot]->insert(i, missFrmCt, tmpQVpp[pdnSlot]->at(i-1));
                axQVpp[pdnSlot]->insert(i, missFrmCt, axQVpp[pdnSlot]->at(i-1));
                ayQVpp[pdnSlot]->insert(i, missFrmCt, ayQVpp[pdnSlot]->at(i-1));
                azQVpp[pdnSlot]->insert(i*ACCLEN, missFrmCt*ACCLEN, azQVpp[pdnSlot]->at(i*ACCLEN-1));
                sqQVpp[pdnSlot]->insert(i, missFrmCt, -4000); //4 for missing pkt
                edQVpp[pdnSlot]->insert(i, missFrmCt, 0);
            }
            if(skpQVpp[pdnSlot]->at(i-1) == 0)
                currADCIndex -= ADCLEN;
            else
                currADCIndex -= (ADCLEN-2);
        }
        if(i > 0)
        {
            tsQVpp[pdnSlot]->remove(0, i);
            skpQVpp[pdnSlot]->remove(0, i);
            batQVpp[pdnSlot]->remove(0, i);
            adcQVpp[pdnSlot]->remove(0, currADCIndex);
            tmpQVpp[pdnSlot]->remove(0, i);
            axQVpp[pdnSlot]->remove(0, i);
            ayQVpp[pdnSlot]->remove(0, i);
            azQVpp[pdnSlot]->remove(0, i*ACCLEN);
            sqQVpp[pdnSlot]->remove(0, i);
            edQVpp[pdnSlot]->remove(0, i);
        }
    }
}


/***
  *	Erase a memory module. Need special handling because it takes a long time.
  *  Assumes the comport has already been opened by SDK.
  *	Inputs:
  *		comportptr, pointer to handle
  *	Returns:
  *      0, if successful
  *      -1, if error
  */
qint32 eraseMM(HANDLE *comportptr)
{
    qint32 retries;
    int resMMEraseEnd;
    QDateTime stDT, currDT;
    QMessageBox msgBox;

    stDT = QDateTime::currentDateTime();

    if(opiucd_mmerasest(comportptr)) return -1;

    // show status and message in messagebox
    msgBox.setWindowTitle("Erase Memory Module");
    msgBox.setText(QString("Erasing entire memory module...\nTakes about 150 seconds.\nDon't interrupt."));
    msgBox.setInformativeText("Time elapsed: 0 seconds");
    msgBox.setStandardButtons(QMessageBox::NoButton);
    msgBox.show();

    for(retries = 150; retries; retries--)
    {
        qApp->processEvents();
        resMMEraseEnd = opiucd_mmeraseend(comportptr);
        if(resMMEraseEnd == 0) break; // done erasing
        else if(resMMEraseEnd == -1)    return -1;
        currDT = QDateTime::currentDateTime();
        msgBox.setInformativeText(QString("Time elapsed: %1 seconds").arg((currDT.toMSecsSinceEpoch()-stDT.toMSecsSinceEpoch())/1000));
    }
    if(!retries) return -1;


    // if got here, then ok
    return 0;
}


/***
  * Read configuration file to get a specific value
  * Inputs:
  *     keyIndex, a QString containing the keyword that is the index
  *     for the value that is requested
  * Returns:
  *     null, if problem or value is null
  *     the value requested
  */
QString getConfigValue(QString keyIndex)
{
    QFile configQFile("opic_cfg.txt");
    QTextStream *configQTSp, *tempQTSp;
    QString tempQS, tempQS2, retQS, currKey;
    QStringList inQSL;
    qint32 i;

    // retQS is null by default

    // check that file is ok
    if(!configQFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return retQS;
    configQTSp = new QTextStream(&configQFile);

    // read all the lines into a list
    while(!configQTSp->atEnd())
        inQSL.append(configQTSp->readLine());
    // clean up file, input file not needed anymore
    configQFile.close();
    delete configQTSp;

    // find the index so can get the value
    for(i = 0; i < inQSL.size(); i++)
    {
        tempQS = inQSL.at(i);
        if(tempQS.isEmpty()) continue;      // skip blank lines
        tempQTSp = new QTextStream(&tempQS);
        *tempQTSp >> currKey;
        *tempQTSp >> tempQS2;
        delete tempQTSp;
        if(!QString::compare(currKey, keyIndex))    // if match, copy value and get out
        {
            retQS = tempQS2.trimmed();
            break;
        }
    }

    return retQS;
}


/***
  * Write a specific index and value to configuration file. Will overwrite
  * existing value if index is the same
  * Inputs:
  *     keyIndex, a QString containing the keyword that is the index
  *     valueQS, a QString containing the value to be written
  * Returns:
  *     0, if successful
  *     -1, if error
  */
qint8 writeConfigValue(QString keyIndex, QString valueQS)
{
    QFile configQFile("opic_cfg.txt");
    QTextStream *configQTSp, *tempQTSp;
    QString tempQS, tempQS2, currKey;
    QStringList inQSL;
    qint32 i;

    // check that file is ok
    if(!configQFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;
    configQTSp = new QTextStream(&configQFile);

    // read all the lines into a list
    while(!configQTSp->atEnd())
        inQSL.append(configQTSp->readLine());

    // clean up input file
    configQFile.close();
    delete configQTSp;

    // find the index if it exists and overwrite
    for(i = 0; i < inQSL.size(); i++)
    {
        tempQS = inQSL.at(i);
        if(tempQS.isEmpty()) continue;      // skip blank lines
        tempQTSp = new QTextStream(&tempQS);
        *tempQTSp >> currKey;
        delete tempQTSp;
        if(!QString::compare(currKey, keyIndex))    // if match, replace value and get out
        {
            tempQS2 = keyIndex;
            inQSL.replace(i,tempQS2.append("\t").append(valueQS));
            break;
        }
    }

    // if index does not exist, append to end of list
    if(i >= inQSL.size())
    {
        tempQS2 = keyIndex;
        inQSL.append(tempQS2.append("\t").append(valueQS));
    }

    // write list back to file
    // check that file is ok
    if(!configQFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return -1;
    configQTSp = new QTextStream(&configQFile);

    for(i = 0; i < inQSL.size(); i++)
    {
        if(inQSL.at(i).isEmpty()) continue; // skip over blank entries
        *configQTSp << inQSL.at(i) << endl;
    }

    // clean up
    configQFile.close();
    delete configQTSp;

    return 0;
}
