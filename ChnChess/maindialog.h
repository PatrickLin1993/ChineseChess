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

	// 初始化信息
	void InitMainDialog(GAMEMODE _mode);
	// 保存棋谱
	void SaveManual(QString fileName);
	// 读取棋谱
	bool ReadManual(QString qurl);
	// 播放生效， bHost 记录是否为主机
	void PlaySounds(enum GORESULT RESULT, bool bHost = true);

protected:
    void mousePressEvent(QMouseEvent*);
    void paintEvent(QPaintEvent*);

private slots:
	// 保存棋谱按钮
    void on_pushButton_clicked(bool);
    // 读取棋谱按钮
    void on_pushButton_2_clicked();
	void on_pushButton_4_clicked();
	void on_pushButton_3_clicked();
	void on_pushButton_5_clicked();

	void btn_chessType_clicked();

	void update_etime();

private:
	// 重新开始
	void Startup(const unsigned char* manual, int sd);

	// 初始化音效队列
	void InitSoundPlayer();

	// 将 int 转成时分秒格式的 QString 格式
	QString Int2Date(unsigned int t);

	void InitEditor();
	
	// 初始化 sideGo
	void InitSideGo();

private:
    Ui::MainDialog *ui;
	// 模式
	GAMEMODE mode;
	// 音效
	QList<class QMediaPlayer*> soundPlayer;
	// 记录双方步时，etime1 为主机时间
	class QTime *etimeHost, *etimeOther;

	// EDITOR
	// 记录每个棋种个数
	int nChessType[15];

	// 棋谱
	unsigned char editor_manual[256];

	class QTimer *etimer;

	PLAYERTYPE sideGo;
};

#endif // MAINDIALOG_H
