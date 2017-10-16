#include "ui_maindialog.h"
#include "chinesechess.h"
#include <QMouseEvent>
#include <QPainter>
#include <windows.h>
#include <fstream>
#include <qfiledialog.h>
#include <qurl.h>
#include <qmediaplayer.h>
#include <qtimer.h>
#include <qdatetime.h>

static int editor_idSelected = 0;
static int hint_mv = 0;

void MainDialog::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPixmap pix;
	float x, y;

    // 背景
    pix.load(":/res/res/chessboard.png");
    painter.drawPixmap(0, 0, 1000, 800, pix);
	
	// 棋子
	auto board = Chessboard::GetInstance();
	auto data = Chessboard::GetInstance()->GetBoardData();
	for (int i = 0; i < 256; ++i){
		if (data[i] != 0){
			pix.load(tr(":/res/res/%1.png").arg(data[i]));
			board->GetCoord(i, &x, &y);
			painter.drawPixmap(x, y, PiecesDiameter, PiecesDiameter, pix);
		}
	}

	// 选择标记
	if (board->GetSelectedCoord(&x, &y)){
		pix.load(tr(":/res/res/selected.png"));
		painter.drawPixmap(x, y, PiecesDiameter, PiecesDiameter, pix);
	}

	// 走法显示
	float x1, y1;
	if (board->GetMvLast(&x, &y, &x1, &y1)){
		pix.load(tr(":/res/res/selected.png"));
		painter.drawPixmap(x, y, PiecesDiameter, PiecesDiameter, pix);
		painter.drawPixmap(x1, y1, PiecesDiameter, PiecesDiameter, pix);
	}

	// 提示
	if (hint_mv != 0){
		if (board->GetCoord(SRC(hint_mv), &x, &y) && board->GetCoord(DST(hint_mv), &x1, &y1)){
			pix.load(tr(":/res/res/selected.png"));
			painter.drawPixmap(x, y, PiecesDiameter, PiecesDiameter, pix);
			painter.drawPixmap(x1, y1, PiecesDiameter, PiecesDiameter, pix);
		}
	}
}

QString MainDialog::Int2Date(unsigned int t)
{
	int sec = t % 60;
	int min = (t - sec) % 3600;
	int h = t / 3600;
	return tr("%1 : %2 : %3").arg(h).arg(min).arg(sec);
}

void MainDialog::update_etime()
{
	// 所用时间
	if (sideGo == PLAYER_HOST){
		ui->host_etime->setText(QStringLiteral("我方所用时间: ") + etimeHost->toString());
		*etimeHost = etimeHost->addSecs(1);
	}
	else if (sideGo == PLAYER_OTHER && mode == LOCALPVP){
		*etimeOther = etimeOther->addSecs(1);
	}
	ui->other_etime->setText(QStringLiteral("对方所用时间: ") + etimeOther->toString());
}

void MainDialog::mousePressEvent(QMouseEvent *e)
{
	if (mode != EDITOR){
		auto res = Chessboard::GetInstance()->Go(e->x(), e->y());
		hint_mv = 0;
		repaint();
		PlaySounds(res, true);

		if (res == NEXT){
			sideGo = (sideGo == PLAYER_HOST ? PLAYER_OTHER : PLAYER_HOST);
			
			if (mode == PVP){
				return;
			}
			if (mode == PVE){
				QTime etime;
				etime.restart();
				this->setCursor(Qt::CursorShape::WaitCursor);
				res = ComputerPlayer::GetInstance()->Go();
				this->setCursor(Qt::CursorShape::ArrowCursor);
				*etimeOther = etimeOther->addMSecs(etime.elapsed());
				sideGo = PLAYER_HOST;
			}
			PlaySounds(res, false);
			repaint();
		}
	}
	else {
		int idx;
		if (editor_idSelected != 0 && Chessboard::GetInstance()->GetIndex(e->x(), e->y(), &idx)){
			Chessboard::GetInstance()->GetBoardData()[idx] = editor_idSelected;
			//editor_manual[idx] = editor_idSelected;
			PlaySounds(GORESULT::NEXT, true);
			repaint();
		}
	}
}

void MainDialog::PlaySounds(GORESULT RESULT, bool bHost)
{
	switch (RESULT)
	{
	case BUZZER:
		bHost ? soundPlayer[SOUND_BUZZER_WIN]->play() : soundPlayer[SOUND_BUZZER_LOSS]->play();
		break;
	case WIN:
		bHost ? soundPlayer[SOUND_WIN]->play() : soundPlayer[SOUND_LOSS]->play();
		break;
	case LOSS:
		bHost ? soundPlayer[SOUND_LOSS]->play() : soundPlayer[SOUND_WIN]->play();
		break;
	case STALEMATE:
		soundPlayer[SOUND_WIN]->play();
		break;
	case SELECT:
		soundPlayer[SOUND_SELECT]->stop();
		soundPlayer[SOUND_SELECT]->play();
		break;
	case NEXT:
		soundPlayer[SOUND_NEXT]->stop();
		soundPlayer[SOUND_NEXT]->play();
		break;
	case CAPTURE:
		soundPlayer[SOUND_CAPTURE]->play();
		break;
	case CHECKED:
		soundPlayer[SOUND_CHECK]->play();
		break;
	case BEGIN:
		soundPlayer[SOUND_BEGIN]->play();
		break;
	default:
		break;
	}
}

void MainDialog::InitSoundPlayer()
{
	soundPlayer.clear();

	for (int i = SOUNDTYPE::SOUND_BEGIN; i < SOUNDTYPE::SOUND_WIN; ++i){
		auto player = new QMediaPlayer;
		connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
		player->setMedia(QUrl::fromLocalFile(tr(".\\sound\\%1.wav").arg(i)));
		player->setVolume(30);
		soundPlayer.insert(i, player);
	}
}

void MainDialog::Startup(const unsigned char* manual, int sd)
{
	Chessboard::GetInstance()->Startup(manual, sd);
	etimeHost = new QTime(0, 0, 0);
	etimeOther = new QTime(0, 0, 0);
}

void MainDialog::InitMainDialog(GAMEMODE _mode)
{
	InitSoundPlayer();
	hint_mv = 0;

	mode = _mode;
	if (mode != EDITOR){
		Startup(normalManual, 0);

		InitSideGo();
		etimer = new QTimer(this);
		connect(etimer, SIGNAL(timeout()), this, SLOT(update_etime()));
		etimer->start(1000);

	}
	else {
		InitEditor();
		Startup(nullManual, 0);
	}
}

MainDialog::MainDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDialog)
{
    ui->setupUi(this);
}

MainDialog::~MainDialog()
{
    delete ui;
}

void MainDialog::on_pushButton_clicked(bool)
{
    SavedDialog *s = new SavedDialog(this);

	s->setWindowModality(Qt::ApplicationModal);
    s->show();

	ofstream file;
}

void MainDialog::SaveManual(QString fileName)
{
	// 嫢涓嶅樺湪 manual 浠跺す欏涘缓
	QFile manualDir(manualURL.c_str());
	if (!manualDir.exists()){
		CreateDirectoryA(manualURL.c_str(), NULL);
	}

	string path = "./manual/" + qstr2str(fileName) + ".manual";
	ofstream outf;
	outf.open(path, fstream::out | fstream::trunc);
	if (outf.is_open()) {
		// 淇濆 sideGo
        outf << Chessboard::GetInstance()->GetSideGo();

		// 淇濆樻嬬
		auto data = Chessboard::GetInstance()->GetBoardData();
		for (int i = 0; i < 256; ++i){
			if (i % 16 == 0){
				outf << endl;
			}
			outf << (int)data[i] << "\t";
		}
	}
}

void MainDialog::on_pushButton_2_clicked()
{  
	QFileDialog *fileDialog = new QFileDialog(this);
	fileDialog->setWindowTitle(QStringLiteral("打开棋谱"));
	fileDialog->setDirectory(".");  
	fileDialog->setNameFilter(tr("manual(*.manual)"));
	fileDialog->setFileMode(QFileDialog::ExistingFile);
	//fileDialog->setViewMode(QFileDialog::Detail);
	
	QList<QUrl> urls;
	if (fileDialog->exec()){
		urls = fileDialog->selectedUrls();
	}

	if (!urls.isEmpty()){
		if (ReadManual(urls[0].toString())){
			QMessageBox::information(this, QStringLiteral("加载棋谱"), QStringLiteral("加载成功！"), QMessageBox::Ok);
		}
		else{
			QMessageBox::critical(this, QStringLiteral("加载棋谱"), QStringLiteral("加载失败！"), QMessageBox::Ok);
		}
	}
}

bool MainDialog::ReadManual(QString qurl)
{
    ifstream inf;
	string url = qstr2str(qurl);
	if (url.substr(0, 8) == "file:///"){
		url = url.substr(8);
	}
	inf.open(url, fstream::in);
	if (inf.is_open()) {
        // 读取 sideGo
		int sideGo;
		inf >> sideGo;

        // 读取棋盘
		int i_id[256];
        for (int i = 0; i < 256; ++i){
			inf >> i_id[i];
        }
		inf.close();
		// int 2 unsigned char
		unsigned char uc_id[256];
		for (int i = 0; i < 256; ++i){
			uc_id[i] = i_id[i];
		}
		Startup(uc_id, sideGo);
		return true;
    }
	else{
		return false;
	}
}

void MainDialog::on_pushButton_4_clicked()
{
	accept();
	StartingDialog* sd = new StartingDialog();
	sd->exec();
}

void MainDialog::on_pushButton_3_clicked()
{
	QApplication::exit();
	//QApplication::quit();
	//accept();
	//QCoreApplication::quit();
}

void MainDialog::InitEditor()
{
	ui->groupBox->hide();
	ui->pushButton_5->hide();

	for (int i = 0; i < 15; ++i){
		nChessType[i] = nChess[i];
	}
	editor_idSelected = 0;

	memset(editor_manual, 0, 256 * sizeof(unsigned char));

	// 初始化右上角棋种按钮
	QPushButton* btn;
	QPixmap pix;
	for (int i = 0; i < 15; ++i){
		if (i == 7){
			continue;
		}
		btn = new QPushButton(this);
		btn->setObjectName(tr("%1").arg(i + 8));
		pix.load(tr(":/res/res/%1.png").arg(i + 8));
		btn->setIcon(pix);
		btn->setIconSize(QSize(PiecesDiameter, PiecesDiameter));
		//btn->setMinimumSize(PiecesDiameter, PiecesDiameter);
		btn->setMaximumSize(PiecesDiameter, PiecesDiameter);
		btn->move(coord_editor[i][0], coord_editor[i][1]);
		connect(btn, &QPushButton::clicked, this, &MainDialog::btn_chessType_clicked);
	}
}

void MainDialog::btn_chessType_clicked()
{
	int id = sender()->objectName().toInt();
	editor_idSelected = id;
}

void MainDialog::InitSideGo()
{
	if (Chessboard::GetInstance()->GetSideGo() == 0){
		sideGo = PLAYER_HOST;
	}
	else{
		sideGo = PLAYER_OTHER;
	}
}

void MainDialog::on_pushButton_5_clicked()
{
	if (mode == LOCALPVP || (mode == PVE && sideGo == PLAYER_HOST)){
		int mv;
		this->setCursor(Qt::CursorShape::WaitCursor);
		ComputerPlayer::GetInstance()->Go(true, &mv);
		this->setCursor(Qt::CursorShape::ArrowCursor);
		hint_mv = mv;
		repaint();
	}
}
