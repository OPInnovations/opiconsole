#include "mixsignaldialog.h"
#include "ui_mixsignaldialog.h"

mixsignaldialog::mixsignaldialog(QVector<qint16> *M2qvectp, QVector<qint16> *M1qvectp,
                                 QVector<qint16> *G2qvectp, QVector<qint16> *G1qvectp,
                                 QVector<qint16> *UPqvectp,
                                 QVector<qint16> *Betaqvectp, QVector<qint16> *Sigmaqvectp,
                                 QVector<qint16> *alphaqvectp,
                                 QVector<qint16> *thetaqvectp, QVector<qint16> *deltaqvectp,
                                 QWidget *parent ) :
    QDialog(parent),
    ui(new Ui::mixsignaldialog)
{
    int i,j;
    ui->setupUi(this);
    reset();
    if((M2qvectp->size()/(EEGSAMPLERATE*EEGEDFDRDURSEC))<1)
    {
        for(i = 0; i < M2qvectp->size(); i++)
            M2qvect.append( M2qvectp->at(i)); //M2

        for(i = 0; i < M1qvectp->size(); i++)
            M1qvect.append( M1qvectp->at(i));//M1

        for(i = 0; i < G2qvectp->size(); i++)
            G2qvect.append( G2qvectp->at(i));//G2

        for(i = 0; i < G1qvectp->size(); i++)
            G1qvect.append( G1qvectp->at(i));//G1

        for(i = 0; i < UPqvectp->size(); i++)
            UPqvect.append( UPqvectp->at(i));//UP

        for(i = 0; i < Betaqvectp->size(); i++)
            Betaqvect.append( Betaqvectp->at(i));//Beta

        for(i = 0; i < Sigmaqvectp->size(); i++)
            Sigmaqvect.append( Sigmaqvectp->at(i));//Sigma

        for(i = 0; i < alphaqvectp->size(); i++)
            alphaqvect.append( alphaqvectp->at(i)); //alpha

        for(i = 0; i < thetaqvectp->size(); i++)
            thetaqvect.append( thetaqvectp->at(i));//theta

        for(i = 0; i < deltaqvectp->size(); i++)
            deltaqvect.append( deltaqvectp->at(i));//delta

    }
    else
    {
        for( j = 0; j < M2qvectp->size()/(EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC); j++)
        {
            // make sure there is enough data for another data record, otherwise get out
            if((((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > M2qvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > M1qvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > G2qvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > G1qvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > UPqvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > Betaqvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > Sigmaqvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > alphaqvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > thetaqvectp->size()) ||
                    (((j+1)*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC-1) > deltaqvectp->size())
                    )
                break;

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                M2qvect.append( M2qvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i)); //M2

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                M1qvect.append( M1qvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//M1

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                G2qvect.append( G2qvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//G2

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                G1qvect.append( G1qvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//G1

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                UPqvect.append( UPqvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//UP

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                Betaqvect.append( Betaqvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//Beta

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                Sigmaqvect.append( Sigmaqvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//Sigma

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                alphaqvect.append( alphaqvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//alpha

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                thetaqvect.append( thetaqvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//theta

            for(i = 0; i < EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC; i++)
                deltaqvect.append( deltaqvectp->at(j*EEGLEN*EEGSAMPLERATE*EEGEDFDRDURSEC+i));//delta
        }
    }
}


mixsignaldialog::~mixsignaldialog()
{
    delete ui;
}


void mixsignaldialog::reset()
{
     ui->M2textEdit->setText("0");
     ui->M1textEdit->setText("0");
     ui->G2textEdit->setText("0");
     ui->G1textEdit->setText("0");
     ui->UPtextEdit->setText("0");
     ui->BetatextEdit->setText("0");
     ui->SigmatextEdit->setText("0");
     ui->alphatextEdit->setText("0");
     ui->thetatextEdit->setText("0");
     ui->deltatextEdit->setText("0");
}


QVector <qint16> mixsignaldialog::getMixSignal()
{
    int index;

    MixSignal.clear();
    MixSignal.resize(M2qvect.size());

    for(index =0;index<M2qvect.size();index++)
    {
        MixSignal[index]=(((ui->M2textEdit->toPlainText().toFloat()*M2qvect.at(index))+
                          (ui->M1textEdit->toPlainText().toFloat()*M1qvect.at(index))+
                          (ui->G2textEdit->toPlainText().toFloat()*G2qvect.at(index))+
                          (ui->G1textEdit->toPlainText().toFloat()*G1qvect.at(index))+
                          (ui->UPtextEdit->toPlainText().toFloat()*UPqvect.at(index))+
                          (ui->BetatextEdit->toPlainText().toFloat()*Betaqvect.at(index))+
                          (ui->SigmatextEdit->toPlainText().toFloat()*Sigmaqvect.at(index))+
                          (ui->alphatextEdit->toPlainText().toFloat()*alphaqvect.at(index))+
                          (ui->thetatextEdit->toPlainText().toFloat()*thetaqvect.at(index))+
                          (ui->deltatextEdit->toPlainText().toFloat()*deltaqvect.at(index))));
    }
    return MixSignal;
}
