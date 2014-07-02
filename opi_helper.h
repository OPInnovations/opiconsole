/* ****************************
 * Helper functions header file
 * OP Innovations
 *
 *
 * v0.95 - 20130119	mpeng
 * --------------------------
 * 1. Original
 *
 * ****************************/

#ifndef OPI_HELPER_H
#define OPI_HELPER_H

#include <QtGui>
#include "opi_linux.h"

// EDF related
#define EDFDRDURSEC 8   // EDF data record duration in seconds
#define FRMSPERSEC  8   // number of truesense data frames per second

// activity related
#define SLOWACTWEIGHT   24   // divider of slow activity in activity cal.
#define FASTACTWEIGHT   3   // divider of fast activity
#define SFASTACTWEIGHT  1  // divider of super fast activity in activity calc.
#define ACTOFFSET   0    // offset of activity data when written to file
#define ACTGAIN     1   // gain of activity data when written to file
#define VALIDTAGTHRESMS  360000000   // number of milliseconds a tag has to be within system time to be valid

/***
  * Prototypes
  */
qint32 setUCTime(HANDLE *comportptr);
qint32 maxWLMeasure100(HANDLE *comportptr);
void opipkt_put_stream(OPIPKT_t *pktptr, QDataStream *osptr);
qint8 opipkt_get_stream(OPIPKT_t *pktptr, QDataStream *isptr);
qint8 getSampQual(qint16 *adcData);
qint32 readUCEvents(HANDLE *comportptr);
qint32 setTSTime(HANDLE *comportptr);
qint32 readMMPkts(HANDLE *comportptr, qint32 fromPktNum = 0, qint32 toPktNum = 327680);
qint32 readMMPktsToEDF(HANDLE *comportptr, qint32 fromPktNum = 0, qint32 toPktNum = 327680);
qint16 calcAct(QVector<qint16> *axQVp, QVector<qint16> *ayQVp, QVector<qint16> *azQVp,
                qint32 *newstepp, qint32 *azoldp, qint32 *axoldp, qint32 *ayoldp, qint32 *az4oldp, qint32 *ax2oldp, qint32 *ay2oldp,qint32 *ax3oldp, qint32 *ay3oldp, qint32 *ax4oldp, qint32 *ay4oldp, qint32 *az8oldp,qint32 *az12oldp, qint32 *az16oldp);
qint32 tagFileRead(QString tagfileName, QVector<qint64> *tagTSQVp, QVector<QString> *tagTextQVp);
void edfDhdropiwrite(QDataStream *out, QString *lpidp, QString *lridp,
                     QDateTime *startDTp, qint32 numDataRecs);
QString localUTCOffset(void);
qint32 getPDNlrid(QString lrid);
qint32 edfDwrite(QDataStream *outstrp, QVector<qint16> *adcQVp,
                 QVector<qint16> *tmpQVp, QVector<qint16> *axQVp,
                 QVector<qint16> *ayQVp, QVector<qint16> *azQVp,
                 QVector<qint16> *sqQVp, qint32 startDataRecCt,
                 qint64 firstFrmTS, QVector<qint64> *tsQVp, QVector<qint64> *tagTSQVp,
                 QVector<QString> *tagTextQVp);
void oneprocQVs(QVector<qint64> *tsQVp, QVector<quint8> *skpQVp,
                QVector<quint8> *batQVp, QVector<qint16> *adcQVp,
                QVector<qint16> *tmpQVp, QVector<qint16> *axQVp,
                QVector<qint16> *ayQVp, QVector<qint16> *azQVp,
                QVector<qint16> *sqQVp, QVector<quint8> *edQVp);
void procQVs(qint64 *firstFrmTSp, qint64 *lastFrmTSp,
             QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
             QVector<quint8> **batQVpp, QVector<qint16> **adcQVpp,
             QVector<qint16> **tmpQVpp, QVector<qint16> **axQVpp,
             QVector<qint16> **ayQVpp, QVector<qint16> **azQVpp,
             QVector<qint16> **sqQVpp, QVector<quint8> **edQVpp);
void procTagQVs(qint64 *filtFirstFrmTSp, qint64 *filtLastFrmTSp,
                QString tagfileName, QVector<qint64> *tagTSQVp,
                QVector<QString> *tagTextQVp);
void procMeegQVs(qint64 *firstFrmTSp, qint64 *lastFrmTSp,
                 QVector<qint64> *tsQVpp, QVector<quint8> *skpQVpp,
                 QVector<quint8> *batQVpp, QVector<qint16> *M2QVpp,
                 QVector<qint16> *M1QVpp, QVector<qint16> *G2QVpp,
                 QVector<qint16> *G1QVpp, QVector<qint16> *upQVpp,
                 QVector<qint16> *betaQVpp, QVector<qint16> *sigmaQVpp,
                 QVector<qint16> *alphaQVpp, QVector<qint16> *thetaQVpp,
                 QVector<qint16> *deltaQVpp);
void procMecgQVs(qint64 *firstFrmTSp, qint64 *lastFrmTSp,
                 QVector<qint64> *tsQVpp, QVector<quint8> *skpQVpp,
                 QVector<quint8> *batQVpp, QVector<qint16> *RRQVpp,
                 QVector<qint16> *ampQVpp, QVector<qint16> *LFperQVpp,
                 QVector<qint16> *HFperQVpp, QVector<qint16> *LHRatioQVpp);
qint16 wirelessaccscale(int wirelessacc);
qint32 eraseMM(HANDLE *comportptr);
QString getConfigValue(QString keyIndex);
qint8 writeConfigValue(QString keyIndex, QString valueQS);

#endif // OPI_HELPER_H
