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
		QMessageBox box(QMessageBox::Information, QStringLiteral("棋谱保存"), QStringLiteral("保存成功，文件在 manual 下"));
		box.setStandardButtons(QMessageBox::Ok);
		box.setButtonText(QMessageBox::Ok, QStringLiteral("确 定"));
		box.exec();
		close();
	}
    else{
        QMessageBox box(QMessageBox::Warning, QStringLiteral("棋谱保存"), QStringLiteral("文件名不能为空！"));
        box.setStandardButtons(QMessageBox::Ok);
        box.setButtonText(QMessageBox::Ok, QStringLiteral("确 定"));
		box.exec();
        ui->lineEdit->clear();
        ui->lineEdit->setFocus();
    }
}

void SavedDialog::on_pushButton_2_clicked()
{
    close();
}
