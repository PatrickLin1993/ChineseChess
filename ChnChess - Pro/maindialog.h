#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>

enum GAMEMODE{
	PVP,
	LOCALPVP,
	EDITOR,
	PVE
};

enum SOUNDTYPE{
	SOUND_BEGIN,
	SOUND_BUZZER_LOSS,
	SOUND_BUZZER_WIN,
	SOUND_CAPTURE,
	SOUND_CHECK,
	SOUND_LOSS,
	SOUND_NEXT,
	SOUND_SELECT,
	SOUND_WIN
};

enum PLAYERTYPE{
	PLAYER_HOST,
	PLAYER_OTHER
};

namespace Ui {
class MainDialog;
}

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MainDialog(QWidget *parent = 0);
    ~MainDialog();

	// ��ʼ����Ϣ
	void InitMainDialog(GAMEMODE _mode);
	// ��������
	void SaveManual(QString fileName);
	// ��ȡ����
	bool ReadManual(QString qurl);
	// ������Ч�� bHost ��¼�Ƿ�Ϊ����
	void PlaySounds(enum GORESULT RESULT, bool bHost = true);

protected:
    void mousePressEvent(QMouseEvent*);
    void paintEvent(QPaintEvent*);

private slots:
	// �������װ�ť
    void on_pushButton_clicked(bool);
    // ��ȡ���װ�ť
    void on_pushButton_2_clicked();
	void on_pushButton_4_clicked();
	void on_pushButton_3_clicked();
	void on_pushButton_5_clicked();

	void btn_chessType_clicked();

	void update_etime();

private:
	// ���¿�ʼ
	void Startup(const unsigned char* manual, int sd);

	// ��ʼ����Ч����
	void InitSoundPlayer();

	// �� int ת��ʱ�����ʽ�� QString ��ʽ
	QString Int2Date(unsigned int t);

	void InitEditor();
	
	// ��ʼ�� sideGo
	void InitSideGo();

private:
    Ui::MainDialog *ui;
	// ģʽ
	GAMEMODE mode;
	// ��Ч
	QList<class QMediaPlayer*> soundPlayer;
	// ��¼˫����ʱ��etime1 Ϊ����ʱ��
	class QTime *etimeHost, *etimeOther;

	// EDITOR
	// ��¼ÿ�����ָ���
	int nChessType[15];

	// ����
	unsigned char editor_manual[256];

	class QTimer *etimer;

	PLAYERTYPE sideGo;
};

#endif // MAINDIALOG_H
