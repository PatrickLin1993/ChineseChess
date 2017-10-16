#include "ui_startingdialog.h"
#include "chinesechess.h"
#include <QMouseEvent>

StartingDialog::StartingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartingDialog)
{
	this->setWindowFlags(Qt::FramelessWindowHint);
	this->setAttribute(Qt::WA_TranslucentBackground);

    ui->setupUi(this);
    //ui->btn_localpvp->setFlat(true);
    //ui->btn_pve->setFlat(true);
    //ui->btn_editor->setFlat(true);
    //ui->pushButton_4->setFlat(true);
    connect(ui->btn_localpvp, &QPushButton::clicked, this, &StartingDialog::btn_clicked);
    connect(ui->btn_pve, &QPushButton::clicked, this, &StartingDialog::btn_clicked);
    connect(ui->btn_editor, &QPushButton::clicked, this, &StartingDialog::btn_clicked);
}

StartingDialog::~StartingDialog()
{
    delete ui;
}

void StartingDialog::btn_clicked()
{
	QString name = sender()->objectName();
	if (name == tr("btn_localpvp")){
		mode = LOCALPVP;
	}
	else if (name == tr("btn_pve")){
		mode = PVE;
	}
	else if (name == tr("btn_editor")){
		mode = EDITOR;
	}

	accept();
	MainDialog* md = new MainDialog(this);
	md->InitMainDialog(mode);
	md->PlaySounds(GORESULT::BEGIN);
	md->exec();
}

void StartingDialog::on_pushButton_4_clicked()
{
	accept();
	QApplication::quit();
}

void StartingDialog::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_Drag = true;
		m_DragPosition = event->globalPos() - this->pos();
		event->accept();
	}
}

void StartingDialog::mouseMoveEvent(QMouseEvent *event)
{
	if (m_Drag && (event->buttons() && Qt::LeftButton))
	{
		move(event->globalPos() - m_DragPosition);
		event->accept();
	}
}

void StartingDialog::mouseReleaseEvent(QMouseEvent *event)
{
	m_Drag = false;
}
