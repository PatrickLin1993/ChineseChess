#include "chinesechess.h"
#include "maindialog.h"
#include "saveddialog.h"
#include "startingdialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	QApplication::setQuitOnLastWindowClosed(true);

    StartingDialog sd;
    
	
	if (sd.exec() == QDialog::Accepted){
		return 0;
	}

	return a.exec();
}
