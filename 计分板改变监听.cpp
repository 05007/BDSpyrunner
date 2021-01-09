using namespace std;
struct Player {
	
};
struct Scoreboard {
	auto getScoreboardId(std::string* str) {
		return SYMCALL(ScoreboardId*, MSSYM_MD5_ecded9d31b4a1c24ba985b0a377bef64, this, str);
	auto getScoreboardID(Player* a2) {
		return SYMCALL(ScoreboardId*, MSSYM_B1QE15getScoreboardIdB1AE10ScoreboardB2AAE20QEBAAEBUScoreboardIdB2AAA9AEBVActorB3AAAA1Z, this, a2);
	}
};
struct ScoreboardId {
	int id;
	//
};
struct ScoreInfo {
	//�� scoreboard::getscores�õ�
	auto getcount() {
		return *(int*)((VA)(this) + 12);
	}
};
struct Objective {
	//��objective::objective�õ�
	//��ȡ�Ʒְ�����
	auto getscorename() {
		return *(std::string*)((VA)(this) + 64);
	}
	//��ȡ�Ʒְ�չʾ����
	auto getscoredisplayname() {
		return *(std::string*)((VA)(this) + 96);
	}
	auto getscoreinfo(ScoreboardId* a2) {
		char a1[12];
		return SYMCALL(ScoreInfo*, MSSYM_B1QE14getPlayerScoreB1AA9ObjectiveB2AAA4QEBAB1QE11AUScoreInfoB2AAE16AEBUScoreboardIdB3AAAA1Z, this, a1, a2);
	}
};

//==============================================��������============================================================================================
Scoreboard* scoreboard;//����Ʒְ�ָ��
static std::map<int, Player*> player_socreboardid;//������ҵļƷְ�id
//�ڷ�������ʼʱ��ȡ�Ʒְ�ָ��
THook(void*, MSSYM_B2QQE170ServerScoreboardB2AAA4QEAAB1AE24VCommandSoftEnumRegistryB2AAE16PEAVLevelStorageB3AAAA1Z, void* _this, void* a2, void* a3) {
	scoreboard = (Scoreboard*)original(_this, a2, a3);
	return scoreboard;
}
// [ԭ��] public: virtual void __cdecl ServerScoreboard::onPlayerJoined(class Player const & __ptr64) __ptr64
// [����] ?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z
//��Ҽ���ʱ��ȡ��ҵļƷְ�ID
THook(void, MSSYM_B1QE14onPlayerJoinedB1AE16ServerScoreboardB2AAE15UEAAXAEBVPlayerB3AAAA1Z, Scoreboard* class_this, Player* player) {
	//����Ҽ���ʱ�洢���ָ�룬����ͨ���Ʒְ�id��ȡ���ָ��
	//��������뿪ʱɾ���������б�д
	int playersocreboardid = landmoney_scoreboard->getScoreboardID(player)->id;
	player_socreboardid[playersocreboardid] = player;
	original(class_this, player);
}
/*
����뿪ʱ��{
	player_socreboardid.erase(playersocreboardid);
}*/

//================================��װ����===================================================================
//ָ�/scoreboard players <add|remove|set> <playersname> <objectivename> <playersnum>����<playersname>��Ӧ�Ʒְ�id
//��ʹ��ʱ����Ҳ�����ʹ�õ��ǵ�һ��������ʱʹ�õڶ���

//ͨ���Ʒְ����ƻ�ȡ�Ʒְ�id
int getScoreBoardId_byString(std::string* str) {
	scoreboard->getScoreboardId(str);
}
//ͨ�����ָ���ȡ�Ʒְ�id
int getScoreBoardId_byPlayer(Player* player) {
	scoreboard->getScoreboardID(player);
}
//===================================================����������Ϊ����======================================================================================
// [ԭ��] public: virtual void __cdecl ServerScoreboard::onScoreChanged(struct ScoreboardId const & __ptr64,class Objective const & __ptr64) __ptr64
// [����] ?onScoreChanged@ServerScoreboard@@UEAAXAEBUScoreboardId@@AEBVObjective@@@Z
// �Ʒְ�ı�ʱ�ļ���
THook(void, MSSYM_B1QE14onScoreChangedB1AE16ServerScoreboardB2AAE21UEAAXAEBUScoreboardIdB2AAE13AEBVObjectiveB3AAAA1Z, const struct Scoreboard* class_this, ScoreboardId* a2, Objective* a3)
{
	/*
	ԭ���
	�����Ʒְ�ʱ��/scoreboard objectives <add|remove> <objectivename> dummy <objectivedisplayname>
	�޸ļƷְ�ʱ���˺���hook�˴�)��/scoreboard players <add|remove|set> <playersname> <objectivename> <playersnum>
	*/
	int scoreboardid = a2->id;
	Player* player = player_socreboardid[scoreboardid];
	if (player) {
		cout << player->getNameTag() << endl;//��ȡ <playersname>
	}
	cout << to_string(scoreboardid) << endl;//��ȡ�Ʒְ�id
	cout << to_string(a3->getscoreinfo(a2)->getcount()) << endl;//��ȡ�޸ĺ��<playersnum>
	cout << a3->getscorename() << endl;//��ȡ<objectivename>
	cout << a3->getscoredisplayname() << endl;//��ȡ<objectivedisname>
	original(class_this, a2, a3);
}
