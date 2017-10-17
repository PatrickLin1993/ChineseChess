#include "chinesechess.h"
#include <time.h>


// 与搜索有关的全局变量
static struct {
	// 博弈树搜索结果
	int mvResult;
	// 历史表
	int historyTable[65536];
	// 杀手走法表
	int mvKillers[LIMIT_DEPTH][2]; 
	// 置换表
	HashItem HashTable[HASH_SIZE]; 
} Search;

// 求MVV/LVA值
inline int MvvLva(int mv) {
	auto data = Chessboard::GetInstance()->GetBoardData();
	return (cucMvvLva[data[DST(mv)]] << 3) - cucMvvLva[data[SRC(mv)]];
}

// "qsort"按历史表排序的比较函数
static int CompareHistory(const void *lpmv1, const void *lpmv2) {
	return Search.historyTable[*(int *)lpmv2] - Search.historyTable[*(int *)lpmv1];
}

// "qsort"按 MVV/LVA 值排序的比较函数
static int CompareMvvLva(const void *lpmv1, const void *lpmv2) {
	return MvvLva(*(int *)lpmv2) - MvvLva(*(int *)lpmv1);
}

// 对最佳走法的处理
inline void SetBestMove(int mv, int nDepth, int nDistance) {
	int *lpmvKillers;
	Search.historyTable[mv] += nDepth * nDepth;
	lpmvKillers = Search.mvKillers[nDistance];
	if (lpmvKillers[0] != mv) {
		lpmvKillers[1] = lpmvKillers[0];
		lpmvKillers[0] = mv;
	}
}

ComputerPlayer* ComputerPlayer::g_cp = nullptr;

ComputerPlayer* ComputerPlayer::GetInstance()
{
	if (g_cp == nullptr){
		g_cp = new ComputerPlayer();
	}
	return g_cp;
}

ComputerPlayer::ComputerPlayer()
{
    nDistance = 0;
}

GORESULT ComputerPlayer::Go(bool bOnlyHint, int* mvHint)
{
    SearchMain();

	if (bOnlyHint && mvHint != nullptr){
		*mvHint = Search.mvResult;
		return GORESULT::RES_HINT;
	}

    auto board = Chessboard::GetInstance();
    board->TakeAStep(Search.mvResult, this);
	board->SetMvLast(Search.mvResult);

    // 检查重复局面
    int vlRep = board->CheckRepeat(3);
    if (board->IsMate()) {
        // 如果分出胜负，那么播放胜负的声音，并且弹出不带声音的提示框
        // PlayResWav(IDR_LOSS);
        // MessageBoxMute("请再接再厉！");
        board->bGameOver = true;
		return BUZZER;
    }
    else if (vlRep > 0) {
        vlRep = board->CheckRepeat(vlRep);
        // 注意："vlRep"是对玩家来说的分值
        //PlayResWav(vlRep < -WIN_VALUE ? IDR_LOSS : vlRep > WIN_VALUE ? IDR_WIN : IDR_DRAW);
        //MessageBoxMute(vlRep < -WIN_VALUE ? "长打作负，请不要气馁！" :
        //vlRep > WIN_VALUE ? "电脑长打作负，祝贺你取得胜利！" : "双方不变作和，辛苦了！");
        board->bGameOver = true;
		return vlRep < -WIN_VALUE ? LOSS : vlRep > WIN_VALUE ? WIN : STALEMATE;
    }
    else if (board->GetnMoveNum() > 100) {
        //PlayResWav(IDR_DRAW);
        //MessageBoxMute("超过自然限着作和，辛苦了！");
        board->bGameOver = true;
		return STALEMATE;
    }
    else {
        // 如果没有分出胜负，那么播放将军、吃子或一般走子的声音
        //PlayResWav(g_pos.InCheck() ? IDR_CHECK2 : g_pos.Captured() ? IDR_CAPTURE2 : IDR_MOVE2);
        if (board->IsCapturedLast()) {
            board->InitMvsList();
        }
		return board->IsCheckedLast() ? CHECKED : board->IsCapturedLast() ? CAPTURE : NEXT;
    }
}

void ComputerPlayer::SearchMain()
{
	int i, t, vl;

	// 初始化
	// 清空历史表
	memset(Search.historyTable, 0, 65536 * sizeof(int));       
	// 清空杀手走法表
	memset(Search.mvKillers, 0, LIMIT_DEPTH * 2 * sizeof(int)); 
	// 清空置换表
	memset(Search.HashTable, 0, HASH_SIZE * sizeof(HashItem));  

	// 初始化定时器
	t = clock();
	// 初始步数
	nDistance = 0; 

	// 迭代加深过程
	for (i = 1; i <= LIMIT_DEPTH; i++) {
		vl = SearchFull(-MATE_VALUE, MATE_VALUE, i);
		// 搜索到杀棋，就终止搜索
		if (vl > WIN_VALUE || vl < -WIN_VALUE) {
			break;
		}
		// 超过一秒，就终止搜索
		if (clock() - t > CLOCKS_PER_SEC) {
			break;
		}
	}
}

int ComputerPlayer::SearchFull(int vlAlpha, int vlBeta, int nDepth, bool bNoNull)
{
	int nHashFlag, vl, vlBest;
	int mv, mvBest, mvHash;
	SortStruct Sort;
	auto board = Chessboard::GetInstance();

	// 一个Alpha-Beta完全搜索分为以下几个阶段
	if (nDistance > 0) {
		// 1. 到达水平线，则调用静态搜索(注意：由于空步裁剪，深度可能小于零)
		if (nDepth <= 0) {
			return SearchQuiesc(vlAlpha, vlBeta);
		}

		// 1-1. 检查重复局面(注意：不要在根节点检查，否则就没有走法了)
		vl = board->CheckRepeat();
		if (vl != 0) {
			return board->CheckRepeat(vl);
		}

		// 1-2. 到达极限深度就返回局面评价
		if (nDistance == LIMIT_DEPTH) {
			return board->Evaluate();
		}

		// 1-3. 尝试置换表裁剪，并得到置换表走法
		vl = ProbeHash(vlAlpha, vlBeta, nDepth, mvHash);
		if (vl > -MATE_VALUE) {
			return vl;
		}

		// 1-4. 尝试空步裁剪(根节点的Beta值是"MATE_VALUE"，所以不可能发生空步裁剪)
		if (!bNoNull && !board->IsCheckedLast() && board->IsAllowNullMove(NULL_MARGIN)) {
			board->NullMove();
			vl = -SearchFull(-vlBeta, 1 - vlBeta, nDepth - NULL_DEPTH - 1, true);
			board->UndoNullMove();
			if (vl >= vlBeta) {
				return vl;
			}
		}
	}
	else {
		mvHash = 0;
	}

	// 2. 初始化最佳值和最佳走法
	nHashFlag = HASH_ALPHA;
	vlBest = -MATE_VALUE; // 这样可以知道，是否一个走法都没走过(杀棋)
	mvBest = 0;           // 这样可以知道，是否搜索到了Beta走法或PV走法，以便保存到历史表

	// 3. 初始化走法排序结构
	Sort.Init(mvHash, nDistance);

	// 4. 逐一走这些走法，并进行递归
	while ((mv = Sort.Next()) != 0) {
		if (board->TakeAStep(mv, this)) {
			// 将军延伸
			vl = -SearchFull(-vlBeta, -vlAlpha, board->IsCheckedLast() ? nDepth : nDepth - 1);
			board->UndoTakeAStep(this);

			// 5. 进行Alpha-Beta大小判断和截断
			if (vl > vlBest) {    // 找到最佳值(但不能确定是Alpha、PV还是Beta走法)
				vlBest = vl;        // "vlBest"就是目前要返回的最佳值，可能超出Alpha-Beta边界
				if (vl >= vlBeta) { // 找到一个Beta走法
					nHashFlag = HASH_BETA;
					mvBest = mv;      // Beta走法要保存到历史表
					break;            // Beta截断
				}
				if (vl > vlAlpha) { // 找到一个PV走法
					nHashFlag = HASH_PV;
					mvBest = mv;      // PV走法要保存到历史表
					vlAlpha = vl;     // 缩小Alpha-Beta边界
				}
			}
		}
	}

	// 5. 所有走法都搜索完了，把最佳走法(不能是Alpha走法)保存到历史表，返回最佳值
	if (vlBest == -MATE_VALUE) {
		// 如果是杀棋，就根据杀棋步数给出评价
		return nDistance - MATE_VALUE;
	}
	// 记录到置换表
	RecordHash(nHashFlag, vlBest, nDepth, mvBest);
	if (mvBest != 0) {
		// 如果不是Alpha走法，就将最佳走法保存到历史表
		SetBestMove(mvBest, nDepth, nDistance);
		if (nDistance == 0) {
			// 搜索根节点时，总是有一个最佳走法(因为全窗口搜索不会超出边界)，将这个走法保存下来
			Search.mvResult = mvBest;
		}
	}
	return vlBest;
}

int ComputerPlayer::SearchQuiesc(int vlAlpha, int vlBeta)
{
	auto board = Chessboard::GetInstance();
	int mvs[MAX_GEN_MOVES];
	// 一个静态搜索分为以下几个阶段
	// 1. 检查重复局面
	int vl = board->CheckRepeat();
	if (vl != 0) {
		return board->CheckRepeat(vl);
	}

	// 2. 到达极限深度就返回局面评价
	if (nDistance == LIMIT_DEPTH) {
		return board->Evaluate();
	}

	// 3. 初始化最佳值
	int vlBest = -MATE_VALUE; // 这样可以知道，是否一个走法都没走过(杀棋)
	int nGenMoves;
	if (board->IsCheckedLast()) {
		// 4. 如果被将军，则生成全部走法
		nGenMoves = board->MovesGenerator(mvs);
		qsort(mvs, nGenMoves, sizeof(int), CompareHistory);
	}
	else {

		// 5. 如果不被将军，先做局面评价
		vl = board->Evaluate();
		if (vl > vlBest) {
			vlBest = vl;
			if (vl >= vlBeta) {
				return vl;
			}
			if (vl > vlAlpha) {
				vlAlpha = vl;
			}
		}

		// 6. 如果局面评价没有截断，再生成吃子走法
		nGenMoves = board->MovesGenerator(mvs, true);
		qsort(mvs, nGenMoves, sizeof(int), CompareMvvLva);
	}

	// 7. 逐一走这些走法，并进行递归
	for (int i = 0; i < nGenMoves; i++) {
		if (board->TakeAStep(mvs[i], this)) {
			vl = -SearchQuiesc(-vlBeta, -vlAlpha);
			board->UndoTakeAStep(this);

			// 8. 进行Alpha-Beta大小判断和截断
			if (vl > vlBest) {    // 找到最佳值(但不能确定是Alpha、PV还是Beta走法)
				vlBest = vl;        // "vlBest"就是目前要返回的最佳值，可能超出Alpha-Beta边界
				if (vl >= vlBeta) { // 找到一个Beta走法
					return vl;        // Beta截断
				}
				if (vl > vlAlpha) { // 找到一个PV走法
					vlAlpha = vl;     // 缩小Alpha-Beta边界
				}
			}
		}
	}

	// 9. 所有走法都搜索完了，返回最佳值
	return vlBest == -MATE_VALUE ? nDistance - MATE_VALUE : vlBest;
}

int ComputerPlayer::ProbeHash(int vlAlpha, int vlBeta, int nDepth, int &mv)
{
	auto zobr = Chessboard::GetInstance()->GetZobr();
	HashItem hsh = Search.HashTable[zobr->key & (HASH_SIZE - 1)];
	if (hsh.lock0 != zobr->lock0 || hsh.lock1 != zobr->lock1) {
		mv = 0;
		return -MATE_VALUE;
	}

	mv = hsh.mv;
	// 杀棋标志：如果是杀棋，那么不需要满足深度条件
	bool bMate = false;
	if (hsh.vl > WIN_VALUE) {
		hsh.vl -= nDistance;
		bMate = true;
	}
	else if (hsh.vl < -WIN_VALUE) {
		hsh.vl += nDistance;
		bMate = true;
	}
	if (hsh.depth >= nDepth || bMate) {
		if (hsh.flag == HASH_BETA) {
			return (hsh.vl >= vlBeta ? hsh.vl : -MATE_VALUE);
		}
		else if (hsh.flag == HASH_ALPHA) {
			return (hsh.vl <= vlAlpha ? hsh.vl : -MATE_VALUE);
		}
		return hsh.vl;
	}
	return -MATE_VALUE;
}

void ComputerPlayer::RecordHash(int nFlag, int vl, int nDepth, int mv)
{
	auto zobr = Chessboard::GetInstance()->GetZobr();
	HashItem hsh = Search.HashTable[zobr->key & (HASH_SIZE - 1)];
	if (hsh.depth > nDepth) {
		return;
	}
	hsh.flag = nFlag;
	hsh.depth = nDepth;
	if (vl > WIN_VALUE) {
		hsh.vl = vl + nDistance;
	}
	else if (vl < -WIN_VALUE) {
		hsh.vl = vl - nDistance;
	}
	else {
		hsh.vl = vl;
	}
	hsh.mv = mv;
	hsh.lock0 = zobr->lock0;
	hsh.lock1 = zobr->lock1;
	Search.HashTable[zobr->key & (HASH_SIZE - 1)] = hsh;
}

int ComputerPlayer::StaleMateVal() const
{
	return (nDistance & 1) == 0 ? -DRAW_VALUE : DRAW_VALUE;
}

int ComputerPlayer::RepValue(int nRepStatus) const
{
	int vlReturn = ((nRepStatus & 2) == 0 ? 0 : nDistance - MATE_VALUE) + ((nRepStatus & 4) == 0 ? 0 : MATE_VALUE - nDistance);
	return vlReturn == 0 ? StaleMateVal() : vlReturn;
}

void SortStruct::Init(int mvHash_, int nDistance) 
{
	mvHash = mvHash_;
	mvKiller1 = Search.mvKillers[nDistance][0];
	mvKiller2 = Search.mvKillers[nDistance][1];
	nPhase = PHASE_HASH;
}

int error = 0;

// 得到下一个走法
int SortStruct::Next()
{
	auto board = Chessboard::GetInstance();
	switch (nPhase) {
		// "nPhase"表示着法启发的若干阶段，依次为：

		// 0. 置换表着法启发，完成后立即进入下一阶段；
	case PHASE_HASH:
		nPhase = PHASE_KILLER_1;
		if (mvHash != 0) {
			return mvHash;
		}
		// 技巧：这里没有"break"，表示"switch"的上一个"case"执行完后紧接着做下一个"case"，下同

		// 1. 杀手着法启发(第一个杀手着法)，完成后立即进入下一阶段；
	case PHASE_KILLER_1:
		nPhase = PHASE_KILLER_2;
		if (mvKiller1 != mvHash && mvKiller1 != 0 && board->IsMoveLegal(mvKiller1)) {
			return mvKiller1;
		}

		// 2. 杀手着法启发(第二个杀手着法)，完成后立即进入下一阶段；
	case PHASE_KILLER_2:
		nPhase = PHASE_GEN_MOVES;
		if (mvKiller2 != mvHash && mvKiller2 != 0 && board->IsMoveLegal(mvKiller2)) {
			return mvKiller2;
		}

		// 3. 生成所有着法，完成后立即进入下一阶段；
	case PHASE_GEN_MOVES:
		
		// DEBUG
		if (error == 1494){
			int a = 0;
		}

		nPhase = PHASE_REST;
		nGenMoves = board->MovesGenerator(mvs);
		qsort(mvs, nGenMoves, sizeof(int), CompareHistory);
		nIndex = 0;
		// DEBUG
		error++;

		// 4. 对剩余着法做历史表启发；
	case PHASE_REST:
		while (nIndex < nGenMoves) {
			int mv = mvs[nIndex];
			nIndex++;
			if (mv != mvHash && mv != mvKiller1 && mv != mvKiller2) {
				return mv;
			}
		}

		// 5. 没有着法了，返回零。
	default:
		return 0;
	}
}
