#include "chinesechess.h"
#include "ui_saveddialog.h"
#include <qmessagebox.h>

SavedDialog::SavedDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SavedDialog)
{
    ui->setupUi(this);
}

SavedDialog::~SavedDialog()
{
    delete ui;
}

void SavedDialog::on_pushButton_clicked()
{
	QString name = ui->lineEdit->text();
	name = name.trimmed();
	auto parent = (MainDialog*)parentWidget();
	if (name != "" && parent != nullptr){
        parent->SaveManual(name);
		QMessageBox box(QMessageBox::Information, QStringLiteral("���ױ���"), QStringLiteral("����ɹ����ļ��� manual ��"));
		box.setStandardButtons(QMessageBox::Ok);
		box.setButtonText(QMessageBox::Ok, QStringLiteral("ȷ ��"));
		box.exec();
		close();
	}
    else{
        QMessageBox box(QMessageBox::Warning, QStringLiteral("���ױ���"), QStringLiteral("�ļ�������Ϊ�գ�"));
        box.setStandardButtons(QMessageBox::Ok);
        box.setButtonText(QMessageBox::Ok, QStringLiteral("ȷ ��"));
		box.exec();
        ui->lineEdit->clear();
        ui->lineEdit->setFocus();
    }
}

void SavedDialog::on_pushButton_2_clicked()
{
    close();
}
