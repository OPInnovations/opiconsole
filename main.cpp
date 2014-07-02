#include <QtGui>
#include "consolewindow.h"
#include "license/licensedialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ConsoleWindow w;
    QFile configQFile("opic_cfg.txt");
    QTextStream *configstrp;
    LicenseDialog *licDp;

    // look for the config file to determine if license should be shown
    if(!configQFile.open(QIODevice::ReadOnly))
    {
        licDp = new LicenseDialog(true);
        if(licDp->exec() == QDialog::Rejected)
        {
            delete licDp;
            configQFile.close();
            return 0;
        }
        else
        {
            delete licDp;
            configQFile.close();
            configQFile.open(QIODevice::WriteOnly | QIODevice::Text);
            configstrp = new QTextStream(&configQFile);
            *configstrp << "UserLicenseAccepted" << endl;
            *configstrp << "Alpha	9.0" << endl;
            *configstrp << "Sigma	11.5" << endl;
            *configstrp << "Sleep_DeltaTH2	10.5" << endl; //default value is set here
            *configstrp << "Sleep_DeltaTH1	8.0" << endl;
            *configstrp << "Sleep_ThetaTH	10.0" << endl;
            *configstrp << "Sleep_SigmaTH	7.0" << endl;
            *configstrp << "Sleep_BetaTH	10.0" << endl;
            *configstrp << "Sleep_G1TH	4.5" << endl;
            *configstrp << "Med_DeltaTH2	10.5" << endl;
            *configstrp << "Med_ThetaTH	10.0" << endl;
            *configstrp << "Med_AlphaTH	8.0" << endl;
            *configstrp << "Med_SigmaTH	7.0" << endl;
            *configstrp << "Med_BetaTH	10.0" << endl;
            *configstrp << "Med_G1TH	4.5" << endl;

            configQFile.close();
            delete configstrp;
        }
    }

    w.show();
    return a.exec();
}
