#ifndef STARTINGDIALOG_H
#define STARTINGDIALOG_H

#include <QDialog>

namespace Ui {
class StartingDialog;
}

class StartingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartingDialog(QWidget *parent = 0);
    ~StartingDialog();

protected:
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);

private slots:
	void btn_clicked();

    void on_pushButton_4_clicked();

public:
	enum GAMEMODE mode;

private:
    Ui::StartingDialog *ui;

	// 标记鼠标是否按下
	bool m_Drag;
	//记录鼠标位置  
	QPoint m_DragPosition;
};

#endif // STARTINGDIALOG_H
