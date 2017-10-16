#include "chinesechess.h"

Chessboard* Chessboard::g_board = nullptr;

Chessboard* Chessboard::GetInstance()
{
	if (g_board == nullptr){
		g_board = new Chessboard();
	}
	return g_board;
}

Chessboard::Chessboard()
{
	InitCoord();
	pieceRadius = PiecesRadius;
	InitZobrist();
}

void Chessboard::Startup(const unsigned char* manual, int sd)
{
	vlRed = vlBlack = 0;
	zobr.InitZero();
	InitBoardData(manual, sd);
	InitMvsList();
	bGameOver = false;
	bFlipped = false;
	idxSelected = 0;
	mvLast = 0;
}

void Chessboard::InitCoord()
{
    memset(coord, 0, 2 * 256 * sizeof(float));

	// 初始化 x 坐标
	for (int c = 0; c < 9; ++c){
		for (int r = 0; r < 10; ++r){
			int index = ((r + 3) << 4) + (c + 3);
			coord[0][index] = coord_x[c];
		}	
	}

	// 初始化 y 坐标
	for (int r = 0; r < 10; ++r){
		for (int c = 0; c < 9; ++c){
			int index = ((r + 3) << 4) + (c + 3);
			coord[1][index] = coord_y[r];
		}
	}
}

void Chessboard::InitBoardData(const unsigned char* manual, int sd)
{
	memset(boardData, 0, 256 * sizeof(unsigned char));
	for (int i = 0; i < 256; ++i){
		if (manual[i] != 0){
			AddPiece(i, manual[i]);
		}
	}
	sideGo = sd;
}

void Chessboard::InitMvsList()
{
	mvsList[0].Set(0, 0, IsChecked(), zobr.key);
	nMoveNum = 1;
}

bool Chessboard::GetCoord(int idx, float* x, float* y)
{
	if (ccInBoard[idx] == 1){
        *x = coord[0][idx] - pieceRadius;
		*y = coord[1][idx] - pieceRadius;
        return true;
    }
    return false;
}


bool Chessboard::GetIndex(float x, float y, int* idx)
{
    int c = BinarySearchPos(coord_x, 0, 8, x);
    int r = BinarySearchPos(coord_y, 0, 9, y);
    *idx = ((r + 3) << 4) + c + 3;
    if (c == -1 || r == -1){
        return false;
    }
    return true;
}

int Chessboard::BinarySearchPos(const float* ary, int left, int right, float coord)
{
	while (left + 1 < right){
		int mid = left + (right - left) / 2;
		if (IsWithin(ary[mid], coord)){
			return mid;
		}
		else if (ary[mid] > coord){
			right = mid;
		}
		else{
			left = mid;
		}
	}
	return  IsWithin(ary[left], coord) ? left : (IsWithin(ary[right], coord) ? right : -1);
}

bool Chessboard::IsWithin(float c1, float c2)
{
	return abs(c1 - c2) <= pieceRadius ? true : false;
}

void Chessboard::AddPiece(int idx, int id)
{
    boardData[idx] = id;
	if (id < 16) {
		vlRed += vlPieces[id - 8][idx];
		zobr.Xor(Zobrist.Table[id - 8][idx]);
	}
	else {
		vlBlack += vlPieces[id - 16][GetIdxFlip(idx)];
		zobr.Xor(Zobrist.Table[id - 9][idx]);
	}
}

void Chessboard::DelPiece(int idx, int id)
{
	boardData[idx] = 0;
	if (id < 16) {
		vlRed -= vlPieces[id - 8][idx];
		zobr.Xor(Zobrist.Table[id - 8][idx]);
	}
	else {
		vlBlack -= vlPieces[id - 16][GetIdxFlip(idx)];
		zobr.Xor(Zobrist.Table[id - 9][idx]);
	}
}

int Chessboard::MovePiece(int mv) 
{
	int idxSrc = SRC(mv);
	int idxDst = DST(mv);
	int idCaptured = boardData[idxDst];
	if (idCaptured != 0) {
		DelPiece(idxDst, idCaptured);
	}
	int idSrc = boardData[idxSrc];
	DelPiece(idxSrc, idSrc);
	AddPiece(idxDst, idSrc);
	return idCaptured;
}

void Chessboard::UndoMovePiece(int mv, int idCaptured) 
{
	int idxSrc = SRC(mv);
	int idxDst = DST(mv);
	int idDst = boardData[idxDst];
	DelPiece(idxDst, idDst);
	AddPiece(idxSrc, idDst);
	if (idCaptured != 0) {
		AddPiece(idxDst, idCaptured);
	}
}

bool Chessboard::TakeAStep(int mv, ComputerPlayer* cp)
{
	int idCaptured;
	unsigned long dwKey;

	dwKey = zobr.key;
	idCaptured = MovePiece(mv);
	if (IsChecked()) {
		UndoMovePiece(mv, idCaptured);
		return false;
	}
	ChangeSide();
	mvsList[nMoveNum].Set(mv, idCaptured, IsChecked(), dwKey);
	nMoveNum++;
	if (cp != nullptr){
		cp->AddDistance();
	}
	return true;
}

void Chessboard::UndoTakeAStep(ComputerPlayer* cp)
{
	if (cp != nullptr){
		cp->DedDistance();
	}
	nMoveNum--;
	ChangeSide();
	UndoMovePiece(mvsList[nMoveNum].mv, mvsList[nMoveNum].idCaptured);
}

void Chessboard::ChangeSide() 
{
	sideGo = 1 - sideGo;
	zobr.Xor(Zobrist.Player);
}

int Chessboard::Evaluate() const 
{
	return (sideGo == 0 ? vlRed - vlBlack : vlBlack - vlRed) + ADVANCED_VALUE;
}

bool Chessboard::IsChecked() const
{
	int i, j, idxDst;
	int idDst, nDelta;
	int idSelfSide = SIDE_TAG(sideGo);
	int idOppSide = OPP_SIDE_TAG(sideGo);
	// 找到棋盘上的帅(将)，再做以下判断：

	for (int idxSrc = 0; idxSrc < 256; idxSrc++) {
		if (boardData[idxSrc] != idSelfSide + PIECE_KING) {
			continue;
		}

		// 1. 判断是否被对方的兵(卒)将军
		if (boardData[SQUARE_FORWARD(idxSrc, sideGo)] == idOppSide + PIECE_PAWN) {
			return true;
		}
		for (nDelta = -1; nDelta <= 1; nDelta += 2) {
			if (boardData[idxSrc + nDelta] == idOppSide + PIECE_PAWN) {
				return true;
			}
		}

		// 2. 判断是否被对方的马将军(以仕(士)的步长当作马腿)
		for (i = 0; i < 4; i++) {
			if (boardData[idxSrc + ccAdvisorDelta[i]] != 0) {
				continue;
			}
			for (j = 0; j < 2; j++) {
				idDst = boardData[idxSrc + ccKnightCheckDelta[i][j]];
				if (idDst == idOppSide + PIECE_KNIGHT) {
					return true;
				}
			}
		}

		// 3. 判断是否被对方的车或炮将军(包括将帅对脸)
		for (i = 0; i < 4; i++) {
			nDelta = ccKingDelta[i];
			idxDst = idxSrc + nDelta;
			while (IN_BOARD(idxDst)) {
				idDst = boardData[idxDst];
				if (idDst != 0) {
					if (idDst == idOppSide + PIECE_ROOK || idDst == idOppSide + PIECE_KING) {
						return true;
					}
					break;
				}
				idxDst += nDelta;
			}
			idxDst += nDelta;
			while (IN_BOARD(idxDst)) {
				int idDst = boardData[idxDst];
				if (idDst != 0) {
					if (idDst == idOppSide + PIECE_CANNON) {
						return true;
					}
					break;
				}
				idxDst += nDelta;
			}
		}
		return false;
	}
	return false;
}

int Chessboard::MovesGenerator(int *mvs, bool bOnlyCapture) const
{
	// 生成所有走法，需要经过以下几个步骤：
	int nGenMoves = 0;
	int idSelfSide = SIDE_TAG(sideGo);
	int idOppSide = OPP_SIDE_TAG(sideGo);
	for (int idxSrc = 0; idxSrc < 256; idxSrc++) {

		// 1. 找到一个本方棋子，再做以下判断：
		int idSrc = boardData[idxSrc];
		if ((idSrc & idSelfSide) == 0) {
			continue;
		}

		// 2. 根据棋子确定走法
		int idDst, i;
		switch (idSrc - idSelfSide) {
		case PIECE_KING:
			for (i = 0; i < 4; i++) {
				int idxDst = idxSrc + ccKingDelta[i];
				if (!IN_FORT(idxDst)) {
					continue;
				}
				idDst = boardData[idxDst];
				if (bOnlyCapture ? (idDst & idOppSide) != 0 : (idDst & idSelfSide) == 0) {
					mvs[nGenMoves] = MOVE(idxSrc, idxDst);
					nGenMoves++;
				}
			}
			break;
		case PIECE_ADVISOR:
			for (i = 0; i < 4; i++) {
				int idxDst = idxSrc + ccAdvisorDelta[i];
				if (!IN_FORT(idxDst)) {
					continue;
				}
				idDst = boardData[idxDst];
				if (bOnlyCapture ? (idDst & idOppSide) != 0 : (idDst & idSelfSide) == 0) {
					mvs[nGenMoves] = MOVE(idxSrc, idxDst);
					nGenMoves++;
				}
			}
			break;
		case PIECE_BISHOP:
			for (i = 0; i < 4; i++) {
				int idxDst = idxSrc + ccAdvisorDelta[i];
				if (!(IN_BOARD(idxDst) && HOME_HALF(idxDst, sideGo) && boardData[idxDst] == 0)) {
					continue;
				}
				idxDst += ccAdvisorDelta[i];
				idDst = boardData[idxDst];
				if (bOnlyCapture ? (idDst & idOppSide) != 0 : (idDst & idSelfSide) == 0) {
					mvs[nGenMoves] = MOVE(idxSrc, idxDst);
					nGenMoves++;
				}
			}
			break;
		case PIECE_KNIGHT:
			for (i = 0; i < 4; i++) {
				int idxDst = idxSrc + ccKingDelta[i];
				if (boardData[idxDst] != 0) {
					continue;
				}
				for (int j = 0; j < 2; j++) {
					idxDst = idxSrc + ccKnightDelta[i][j];
					if (!IN_BOARD(idxDst)) {
						continue;
					}
					idDst = boardData[idxDst];
					if (bOnlyCapture ? (idDst & idOppSide) != 0 : (idDst & idSelfSide) == 0) {
						mvs[nGenMoves] = MOVE(idxSrc, idxDst);
						nGenMoves++;
					}
				}
			}
			break;
		case PIECE_ROOK:
			for (i = 0; i < 4; i++) {
				int nDelta = ccKingDelta[i];
				int idxDst = idxSrc + nDelta;
				while (IN_BOARD(idxDst)) {
					idDst = boardData[idxDst];
					if (idDst == 0) {
						if (!bOnlyCapture) {
							mvs[nGenMoves] = MOVE(idxSrc, idxDst);
							nGenMoves++;
						}
					}
					else {
						if ((idDst & idOppSide) != 0) {
							mvs[nGenMoves] = MOVE(idxSrc, idxDst);
							nGenMoves++;
						}
						break;
					}
					idxDst += nDelta;
				}
			}
			break;
		case PIECE_CANNON:
			for (i = 0; i < 4; i++) {
				int nDelta = ccKingDelta[i];
				int idxDst = idxSrc + nDelta;
				while (IN_BOARD(idxDst)) {
					idDst = boardData[idxDst];
					if (idDst == 0) {
						if (!bOnlyCapture) {
							mvs[nGenMoves] = MOVE(idxSrc, idxDst);
							nGenMoves++;
						}
					}
					else {
						break;
					}
					idxDst += nDelta;
				}
				idxDst += nDelta;
				while (IN_BOARD(idxDst)) {
					idDst = boardData[idxDst];
					if (idDst != 0) {
						if ((idDst & idOppSide) != 0) {
							mvs[nGenMoves] = MOVE(idxSrc, idxDst);
							nGenMoves++;
						}
						break;
					}
					idxDst += nDelta;
				}
			}
			break;
		case PIECE_PAWN:
			int idxDst = SQUARE_FORWARD(idxSrc, sideGo);
			if (IN_BOARD(idxDst)) {
				idDst = boardData[idxDst];
				if (bOnlyCapture ? (idDst & idOppSide) != 0 : (idDst & idSelfSide) == 0) {
					mvs[nGenMoves] = MOVE(idxSrc, idxDst);
					nGenMoves++;
				}
			}
			if (AWAY_HALF(idxSrc, sideGo)) {
				for (int nDelta = -1; nDelta <= 1; nDelta += 2) {
					idxDst = idxSrc + nDelta;
					if (IN_BOARD(idxDst)) {
						idDst = boardData[idxDst];
						if (bOnlyCapture ? (idDst & idOppSide) != 0 : (idDst & idSelfSide) == 0) {
							mvs[nGenMoves] = MOVE(idxSrc, idxDst);
							nGenMoves++;
						}
					}
				}
			}
			break;
		}
	}
	return nGenMoves;
}

bool Chessboard::IsMoveLegal(int mv) const
{
	// 判断走法是否合法，需要经过以下的判断过程：
	// 1. 判断起始格是否有自己的棋子
	int idxSrc = SRC(mv);
	int idSrc = boardData[idxSrc];
	int idSelfSide = SIDE_TAG(sideGo);
	if ((idSrc & idSelfSide) == 0) {
		return false;
	}

	// 2. 判断目标格是否有自己的棋子
	int idxDst = DST(mv);
	int idDst = boardData[idxDst];
	if ((idDst & idSelfSide) != 0) {
		return false;
	}

	// 3. 根据棋子的类型检查走法是否合理
	int idxPin;
	switch (idSrc - idSelfSide) {
	case PIECE_KING:
		return IN_FORT(idxDst) && KING_SPAN(idxSrc, idxDst);
	case PIECE_ADVISOR:
		return IN_FORT(idxDst) && ADVISOR_SPAN(idxSrc, idxDst);
	case PIECE_BISHOP:
		return SAME_HALF(idxSrc, idxDst) && BISHOP_SPAN(idxSrc, idxDst) &&
			boardData[BISHOP_PIN(idxSrc, idxDst)] == 0;
	case PIECE_KNIGHT:
		idxPin = KNIGHT_PIN(idxSrc, idxDst);
		return idxPin != idxSrc && boardData[idxPin] == 0;
	case PIECE_ROOK:
	case PIECE_CANNON: 
		int nDelta;
		if (SAME_RANK(idxSrc, idxDst)) {
			nDelta = (idxDst < idxSrc ? -1 : 1);
		}
		else if (SAME_FILE(idxSrc, idxDst)) {
			nDelta = (idxDst < idxSrc ? -16 : 16);
		}
		else {
			return false;
		}
		idxPin = idxSrc + nDelta;
		while (idxPin != idxDst && boardData[idxPin] == 0) {
			idxPin += nDelta;
		}
		if (idxPin == idxDst) {
			return idDst == 0 || idSrc - idSelfSide == PIECE_ROOK;
		}
		else if (idDst != 0 && idSrc - idSelfSide == PIECE_CANNON) {
			idxPin += nDelta;
			while (idxPin != idxDst && boardData[idxPin] == 0) {
				idxPin += nDelta;
			}
			return idxPin == idxDst;
		}
		else {
			return false;
		}
	case PIECE_PAWN:
		if (AWAY_HALF(idxDst, sideGo) && (idxDst == idxSrc - 1 || idxDst == idxSrc + 1)) {
			return true;
		}
		return idxDst == SQUARE_FORWARD(idxSrc, sideGo);
	default:
		return false;
	}
}

bool Chessboard::IsMate()
{
	int i, nGenMoveNum, idCaptured;
	int mvs[MAX_GEN_MOVES];

	nGenMoveNum = MovesGenerator(mvs);
	for (i = 0; i < nGenMoveNum; i++) {
		idCaptured = MovePiece(mvs[i]);
		if (!IsChecked()) {
			UndoMovePiece(mvs[i], idCaptured);
			return false;
		}
		else {
			UndoMovePiece(mvs[i], idCaptured);
		}
	}
	return true;
}

int Chessboard::CheckRepeat(int nRecur) const
{
	bool bSelfSide = false;
	bool bPerpCheck = true, bOppPerpCheck = true;
	const MoveStruct *lpmvs;
	lpmvs = mvsList + nMoveNum - 1;
	while (lpmvs->mv != 0 && lpmvs->idCaptured == 0) {
		if (bSelfSide) {
			bPerpCheck = bPerpCheck && lpmvs->bChecked;
			if (lpmvs->key == zobr.key) {
				nRecur--;
				if (nRecur == 0) {
					return 1 + (bPerpCheck ? 2 : 0) + (bOppPerpCheck ? 4 : 0);
				}
			}
		}
		else {
			bOppPerpCheck = bOppPerpCheck && lpmvs->bChecked;
		}
		bSelfSide = !bSelfSide;
		lpmvs--;
	}
	return 0;
}


bool Chessboard::IsCheckedLast() const
{
	return mvsList[nMoveNum - 1].bChecked;
}


bool Chessboard::IsCapturedLast() const
{
	return mvsList[nMoveNum - 1].idCaptured != 0;
}

void Chessboard::NullMove(ComputerPlayer* cp)
{
	unsigned long dwKey = zobr.key;
	ChangeSide();
	mvsList[nMoveNum].Set(0, 0, false, dwKey);
	nMoveNum++;
	if (cp != nullptr){
		cp->AddDistance();
	}
}

void Chessboard::UndoNullMove(ComputerPlayer* cp)
{
	if (cp != nullptr){
		cp->DedDistance();
	}
	nMoveNum--;
	ChangeSide();
}

bool Chessboard::IsAllowNullMove(int margin) const
{
	return (sideGo == 0 ? vlRed : vlBlack) > margin;
}

// 初始化Zobrist表
void Chessboard::InitZobrist()
{
	int i, j;
	RC4Struct rc4;

	rc4.InitZero();
	Zobrist.Player.InitRC4(rc4);
	for (i = 0; i < 14; i++) {
		for (j = 0; j < 256; j++) {
			Zobrist.Table[i][j].InitRC4(rc4);
		}
	}
}

GORESULT Chessboard::Go(float x, float y)
{
	int idx;
	if (GetIndex(x, y, &idx)){
		int id, mv, vlRep;
		idx = bFlipped ? SQUARE_FLIP(idx) : idx;

		id = boardData[idx];

		if ((id & SIDE_TAG(sideGo)) != 0) {
			// 如果点击自己的子，那么直接选中该子
			//if (idxSelected != 0) {
			//	DrawSquare(Xqwl.idxSelected);
			//}
			idxSelected = idx;
			mvLast = 0;
			//DrawSquare(idx, DRAW_SELECTED);
			/*if (mvLast != 0) {
				DrawSquare(SRC(mvLast));
				DrawSquare(DST(mvLast));
			}*/
			//PlayResWav(IDR_CLICK); // 播放点击的声音
			return SELECT;
		}
		else if (idxSelected != 0 && !bGameOver) {
			// 如果点击的不是自己的子，但有子选中了(一定是自己的子)，那么走这个子
			mv = MOVE(idxSelected, idx);
			if (IsMoveLegal(mv)) {
				if (TakeAStep(mv, ComputerPlayer::GetInstance())) {
					mvLast = mv;
					idxSelected = 0;
					// 检查重复局面
					vlRep = CheckRepeat(3);
					if (IsMate()) {
						bGameOver = true;
						return BUZZER;
					}
					else if (vlRep > 0) {
						vlRep = CheckRepeat(vlRep);
						bGameOver = true;
						return vlRep > WIN_VALUE ? LOSS : vlRep < -WIN_VALUE ? WIN : STALEMATE;
					}
					else if (GetnMoveNum() > 100) {
						bGameOver = true;
						return STALEMATE;
					}
					else {
						if (IsCapturedLast()) {
							InitMvsList();
						}
						// 轮到下一方着手
						return IsCheckedLast() ? CHECKED : IsCapturedLast() ? CAPTURE : NEXT;
					}
				}
				else {
					//PlayResWav(IDR_ILLEGAL); // 播放被将军的声音
					return CHECKED;
				}
			}
		}
	}
	return ILLEGAL;
}

void RC4Struct::InitZero()
{
	int i, j;
	unsigned char uc;

	x = y = j = 0;
	for (i = 0; i < 256; i++) {
		s[i] = i;
	}
	for (i = 0; i < 256; i++) {
		j = (j + s[i]) & 255;
		uc = s[i];
		s[i] = s[j];
		s[j] = uc;
	}
}


unsigned char RC4Struct::NextByte() 
{
	unsigned char uc;
	x = (x + 1) & 255;
	y = (y + s[x]) & 255;
	uc = s[x];
	s[x] = s[y];
	s[y] = uc;
	return s[(s[x] + s[y]) & 255];
}


unsigned long RC4Struct::NextLong() 
{
	unsigned char uc0, uc1, uc2, uc3;
	uc0 = NextByte();
	uc1 = NextByte();
	uc2 = NextByte();
	uc3 = NextByte();
	return uc0 + (uc1 << 8) + (uc2 << 16) + (uc3 << 24);
}


void ZobristStruct::InitZero() 
{
	key = lock0 = lock1 = 0;
}


void ZobristStruct::InitRC4(RC4Struct &rc4)
{
	key = rc4.NextLong();
	lock0 = rc4.NextLong();
	lock1 = rc4.NextLong();
}


void ZobristStruct::Xor(const ZobristStruct &zobr) {
	key ^= zobr.key;
	lock0 ^= zobr.lock0;
	lock1 ^= zobr.lock1;
}


void ZobristStruct::Xor(const ZobristStruct &zobr1, const ZobristStruct &zobr2) {
	key ^= zobr1.key ^ zobr2.key;
	lock0 ^= zobr1.lock0 ^ zobr2.lock0;
	lock1 ^= zobr1.lock1 ^ zobr2.lock1;
}