#ifndef COMPUTERPLAYER_H
#define COMPUTERPLAYER_H

// 最大的生成走法数
const int MAX_GEN_MOVES = 128;
const int NULL_MARGIN = 400;   // 空步裁剪的子力边界
const int DRAW_VALUE = 20;     // 和棋时返回的分数(取负值)
const int LIMIT_DEPTH = 64;    // 最大的搜索深度
const int MATE_VALUE = 10000;  // 最高分值，即将死的分值
const int WIN_VALUE = MATE_VALUE - 100; // 搜索出胜负的分值界限，超出此值就说明已经搜索出杀棋了

const int NULL_DEPTH = 2;      // 空步裁剪的裁剪深度
const int HASH_SIZE = 1 << 20; // 置换表大小
const int HASH_ALPHA = 1;      // ALPHA节点的置换表项
const int HASH_BETA = 2;       // BETA节点的置换表项
const int HASH_PV = 3;         // PV节点的置换表项

// 走法排序阶段
const int PHASE_HASH = 0;
const int PHASE_KILLER_1 = 1;
const int PHASE_KILLER_2 = 2;
const int PHASE_GEN_MOVES = 3;
const int PHASE_REST = 4;

// MVV/LVA每种子力的价值
static unsigned char cucMvvLva[24] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	5, 1, 1, 3, 4, 3, 2, 0,
	5, 1, 1, 3, 4, 3, 2, 0
};

// 置换表项结构
struct HashItem {
	unsigned char depth, flag;
	short vl;
	unsigned short mv, reserved;
	unsigned long lock0, lock1;
};

// 走法排序结构
struct SortStruct {
	int mvHash, mvKiller1, mvKiller2; // 置换表走法和两个杀手走法
	int nPhase, nIndex, nGenMoves;    // 当前阶段，当前采用第几个走法，总共有几个走法
	int mvs[MAX_GEN_MOVES];           // 所有的走法

	// 初始化，设定置换表走法和两个杀手走法
	void Init(int mvHash_, int nDistance);
	// 得到下一个走法
	int Next(void);
};

class ComputerPlayer
{
public:
	GORESULT Go(bool bOnlyHint = false, int* mvHint = nullptr);

	inline void AddDistance(){
		nDistance++;
	}
	inline void DedDistance(){
		nDistance--;
	}
	inline int GetDistance(){
		return nDistance;
	}

	static ComputerPlayer* GetInstance();

private:
    ComputerPlayer();

	// 迭代加深搜索过程
	void SearchMain();

	// 超出边界(Fail-Soft)的Alpha-Beta搜索过程
	int SearchFull(int vlAlpha, int vlBeta, int nDepth, bool bNoNull = false);

	// 静态(Quiescence)搜索过程
	int SearchQuiesc(int vlAlpha, int vlBeta); 

	// 提取置换表项
	int ProbeHash(int vlAlpha, int vlBeta, int nDepth, int &mv);

	// 保存置换表项
	void RecordHash(int nFlag, int vl, int nDepth, int mv);

	// 和棋分值
	int StaleMateVal() const;

	// 重复局面分值
	int RepValue(int nRepStatus) const;

private:
    // 距离根节点的步数
    int nDistance;

	static ComputerPlayer* g_cp;
};

#endif // COMPUTERPLAYER_H
