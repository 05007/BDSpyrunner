#include "Ԥ����ͷ.h"
#include "�ṹ��.hpp"
#include <thread>
#include <unordered_set>
#include <unordered_map>
#define PY_SSIZE_T_CLEAN
#include "include/Python.h"
#pragma region �궨��
//�������к���
#define CallAll(type,...)									\
	bool res = true;										\
	for (auto & fn :PyFuncs[type]) {							\
		PyObject* r = PyObject_CallFunction(fn,__VA_ARGS__);\
		PyErr_Print();int bar = 1;PyArg_Parse(r,"p",&bar);	\
		if (!bar) res = false;Py_XDECREF(r);				\
	}
//��׼�������Ϣ
#define cout(...) cout <<__VA_ARGS__<< endl
//THook�����ж�
#define RET(...) \
	if (!res) return 0;\
	return original(__VA_ARGS__)
//����Player*��ȡ��һ�����Ϣ
#define getPlayerInfo(p) \
	string pn = p->getNameTag();\
	int did = p->getDimensionId();\
	Vec3* pp = p->getPos();\
	BYTE st = p->isStand();
#pragma endregion
#pragma region ȫ�ֱ���
static VA Cmd_Queue;
static VA Level_;
static VA Server_Network_Handle;
static Scoreboard* Score_Board;//����Ʒְ�����
static const VA STD_COUT_HANDLE = *(VA*)GetServerSymbol("__imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A");
static unordered_map<string, vector<PyObject*>> PyFuncs;//py����
static unordered_map<string, Player*> onlinePlayers;//�������
static unordered_set<Player*> PlayerList;//����б�
static unordered_map<unsigned, bool> FormId;//��ID
static unordered_map<string, string> Command;//ע������
#pragma endregion
#pragma region ��������
void init();
// �ж�ָ���Ƿ�Ϊ����б���ָ��
static inline bool PlayerCheck(void* p) {
	return PlayerList.count((Player*)p);
}
// ���߳���ʱ
static void delay(int time, PyObject* func) {
	Sleep(time);
	if (PyCallable_Check(func))
		PyObject_CallFunction(func, 0);
	else
		Py_DECREF(func);
}
// ִ�к��ָ��
static void runcmd(string cmd) {
	SYMCALL<bool>("??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
		Cmd_Queue, cmd);
}
// ��ȡһ��δ��ʹ�õĻ���ʱ��������id
static unsigned getFormId() {
	unsigned id = (int)time(0) + rand();
	do {
		++id;
	} while (id == 0 || FormId[id]);
	FormId[id] = true;
	return id;
}
// �������ݰ�
static inline void cPacket(VA& tpk, int p) {
	SYMCALL<VA>("?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",
		&tpk, p);
}
// ����һ����
static bool releaseForm(unsigned fid)
{
	if (FormId[fid]) {
		FormId.erase(fid);
		return true;
	}
	return false;
}
// ����һ�������ݰ�
static unsigned sendForm(string uuid, string str) {
	unsigned fid = getFormId();
	Player* p = onlinePlayers[uuid];
	if (PlayerCheck(p)) {
		VA tpk;//ModalFormRequestPacket
		cPacket(tpk, 100);
		*(VA*)(tpk + 40) = fid;
		*(string*)(tpk + 48) = str;
		p->sendPacket(tpk);
	}
	return fid;
}
// �������
static bool transferServer(string uuid, string address, int port) {
	Player* p = onlinePlayers[uuid];
	if (PlayerCheck(p)) {
		VA tpk;//TransferPacket
		cPacket(tpk, 85);
		*(string*)(tpk + 40) = address;
		*(VA*)(tpk + 72) = port;
		p->sendPacket(tpk);
		return true;
	}
	return false;
}
// �������
static void logout(string str) {
	SYMCALL<VA>("??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
		STD_COUT_HANDLE, str.c_str(), str.length());
}
// ��ȡ����
static int getPlayerScore(string uuid, string n) {
	Player* p = onlinePlayers[uuid];
	if (PlayerCheck(p)) {
		Objective* testobj = Score_Board->getObjective(&n);
		if (!testobj) {
			cout("bad objective:" << n);
		}
		ScoreInfo a[2];
		return testobj->getPlayerScore(a, Score_Board->getScoreboardId(p))->getCount();
	}
	return 0;
}
// ���÷���
static void setPlayerScore(string uuid, string n, int count, char mode) {
	Player* p = onlinePlayers[uuid];
	if (PlayerCheck(p)) {
		Objective* obj = Score_Board->getObjective(&n);
		if (!obj) {
			cout("bad objective:" << n); return;
		}
		Score_Board->modifyPlayerScore((ScoreboardId*)Score_Board->getScoreboardId(p), obj, count, mode);
	}
}
// ģ����ҷ�����Ϣ
static bool sendText(string uuid, string msg, int mode) {
	Player* p = onlinePlayers[uuid];
	if (PlayerCheck(p)) {	// IDA ServerNetworkHandler::handle
		VA tpk;//TextPacket
		cPacket(tpk, 9);
		*(char*)(tpk + 40) = mode;
		*(string*)(tpk + 48) = p->getNameTag();
		*(string*)(tpk + 80) = msg;
		p->sendPacket(tpk);
		return true;
	}
	return false;
}
// ģ�����ִ������
static bool runcmdAs(string uuid, string cmd) {
	Player* p = onlinePlayers[uuid];
	if (PlayerCheck(p)) {
		VA tpk;//CommandRequestPacket
		cPacket(tpk, 76);
		*(string*)(tpk + 40) = cmd;
		return true;
	}
	return false;
}
// ���ò����
static bool setSidebar(string uuid, string title, string name) {
	Player* p = onlinePlayers[uuid];
	if (PlayerCheck(p)) {
		//VA tpk;//setDisplayObjectivePacket
		//cPacket(tpk, 107);
		//*(string*)(tpk + 56) = "sidebar";
		//*(string*)(tpk + 88) = name;
		//*(string*)(tpk + 120) = title;
		//*(string*)(tpk + 152) = "dummy";
		//*(string*)(tpk + 184) = "";
		//p->sendPacket(tpk);
		SYMCALL<void>("?setDisplayObjective@Scoreboard@@UEAAPEBVDisplayObjective@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVObjective@@W4ObjectiveSortOrder@@@Z",
			title, Score_Board->getObjective(&name), 0);
	}
	return true;
}
#pragma endregion
#pragma region api����
// ָ�����
static PyObject* api_logout(PyObject* self, PyObject* args) {
	const char* msg;
	if (PyArg_ParseTuple(args, "s", &msg))
		logout(msg);
	return Py_None;
}
// ִ��ָ��
static PyObject* api_runcmd(PyObject* self, PyObject* args) {
	const char* cmd;
	if (PyArg_ParseTuple(args, "s", &cmd))
		runcmd(cmd);
	return Py_None;
}
// ��ʱ
static PyObject* api_setTimeout(PyObject* self, PyObject* args) {
	int time; PyObject* func;
	if (PyArg_ParseTuple(args, "Oi", &func, &time)) {
		thread(delay, time, func).detach();
	}
	return Py_None;
}
// ��ȡuuid
static PyObject* api_UUID(PyObject* self, PyObject* args) {
	const char* name;
	if (PyArg_ParseTuple(args, "s", &name))
		for (Player* p : PlayerList) {
			if (p->getNameTag() == name)
				return Py_BuildValue("s", p->getUuid());
		}
}
// ���ü���
static PyObject* api_setListener(PyObject* self, PyObject* args) {
	const char* m;
	PyObject* func;
	if (PyArg_ParseTuple(args, "sO", &m, &func) && PyCallable_Check(func))
		PyFuncs[m].push_back(func);
	return Py_None;
}
// ���ͱ�
static PyObject* api_sendForm(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* str;
	if (PyArg_ParseTuple(args, "ss", &uuid, &str)) {
		return Py_BuildValue("i", sendForm(uuid, str));
	}
	return Py_None;
}
// �������
static PyObject* api_transferServer(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* address;
	int port;
	if (PyArg_ParseTuple(args, "ssi", &uuid, &address, &port)) {
		return Py_BuildValue("i", transferServer(uuid, address, port));
	}
	return Py_None;
}
// ��ȡ����ֳ�
static PyObject* api_getHand(PyObject* self, PyObject* args) {
	const char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			ItemStack* item = p->getSelectedItem();
			short iaux = item->getAuxValue();
			short iid = item->getId();
			string iname = item->getName();
			return Py_BuildValue("{s:i,s:i,s:s}",
				"itemid", iid,
				"itemaux", iaux,
				"itemname", iname.c_str()
			);
		}
	}
	return Py_None;
}
// ��ȡ�����Ϣ
static PyObject* api_getPlayerInfo(PyObject* self, PyObject* args) {
	const char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			getPlayerInfo(p);
			float hm, h; p->getHealth(hm, h);
			return Py_BuildValue("{s:s,s:[f,f,f],s:i,s:i,s:f}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"isstand", st,
				"health", h
			);
		}
	}
	return Py_None;
}
// ��ȡ���Ȩ��
static PyObject* api_getPlayerPerm(PyObject* self, PyObject* args) {
	const char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			return Py_BuildValue("i", p->getPermission());
		}
	}
	return Py_None;
}
// �������Ȩ��
static PyObject* api_setPlayerPerm(PyObject* self, PyObject* args) {
	const char* uuid;
	int lv;
	if (PyArg_ParseTuple(args, "si", &uuid, &lv)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			p->setPermissionLevel(lv);
		}
	}
	return Py_None;
}
// ������ҵȼ�
static PyObject* api_addLevel(PyObject* self, PyObject* args) {
	const char* uuid;
	int lv = 0;
	if (PyArg_ParseTuple(args, "si", &uuid, &lv)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			SYMCALL<void>("?addLevels@Player@@UEAAXH@Z", p, lv);
		}
	}
	return Py_None;
}
// �����������
static PyObject* api_setNameTag(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* name;
	if (PyArg_ParseTuple(args, "ss", &uuid, &name)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			p->setName(name);
		}
	}
	return Py_None;
}
// ����ָ��˵��
static PyObject* api_setCommandDescribe(PyObject* self, PyObject* args) {
	const char* cmd;
	const char* des;
	if (PyArg_ParseTuple(args, "ss", &cmd, &des)) {
		Command[cmd] = des;
	}
	return Py_None;
}
// ��ҷ���
static PyObject* api_getPlayerScore(PyObject* self, PyObject* args) {
	const char* uuid, * obj;
	if (PyArg_ParseTuple(args, "ss", &uuid, &obj)) {
		return Py_BuildValue("i", getPlayerScore(uuid, obj));
	}
	return Py_None;
}
static PyObject* api_setPlayerScore(PyObject* self, PyObject* args) {
	const char* uuid, * obj; int count; char mode;
	if (PyArg_ParseTuple(args, "ssii", &uuid, &obj, &count, &mode)) {
		setPlayerScore(uuid, obj, count, mode);
	}
	return Py_None;
}
// ģ����ҷ����ı�
static PyObject* api_talkAs(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* msg;
	if (PyArg_ParseTuple(args, "ss", &uuid, &msg)) {
		return Py_BuildValue("i", sendText(uuid, msg, 1));
	}
	return Py_None;
}
// ģ�����ִ��ָ��
static PyObject* api_runcmdAs(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* cmd;
	if (PyArg_ParseTuple(args, "ss", &uuid, &cmd)) {
		return Py_BuildValue("i", runcmdAs(uuid, cmd));
	}
	return Py_None;
}
// �������
static PyObject* api_teleport(PyObject* self, PyObject* args) {
	const char* pn; float x, y, z; int did;
	if (PyArg_ParseTuple(args, "sfffi", &pn, &x, &y, &z, &did)) {
		Player* p = onlinePlayers[pn];
		p->teleport({ x,y,z }, did);
	}
	return Py_None;
}
// ԭʼ���
static PyObject* api_tellraw(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* msg;
	if (PyArg_ParseTuple(args, "ss", &uuid, &msg)) {
		return Py_BuildValue("i", sendText(uuid, msg, 0));
	}
	return Py_None;
}
// ���ò����
static PyObject* api_addItem(PyObject* self, PyObject* args) {
	const char* uuid;
	int id, aux, count;
	if (PyArg_ParseTuple(args, "siii", &uuid, &id, &aux, &count)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			ItemStack item;
			item.getFromId(id, aux, count);
			p->addItem(&item);
		}
	}
	return Py_None;
}
// �����б�
static PyMethodDef mcMethods[] = {
	{"logout", api_logout, 1,0},//��3��ֵ:1�в���4�޲���,��4��ֵ:����
	{"runcmd", api_runcmd, 1,0},
	{"setTimeout", api_setTimeout, 1,0},
	{"UUID", api_UUID, 1,0},
	{"setListener", api_setListener, 1,0},
	{"sendForm", api_sendForm, 1,0},
	{"transferServer", api_transferServer, 1,0},
	{"getHand", api_getHand, 1,0},
	{"getPlayerInfo", api_getPlayerInfo, 1,0},
	{"getPlayerPerm", api_getPlayerPerm, 1,0},
	{"setPlayerPerm",api_setPlayerPerm, 1,0},
	{"addLevel", api_addLevel, 1,0},
	{"setNameTag", api_setNameTag, 1,0},
	{"setCommandDescribe", api_setCommandDescribe, 1,0},
	{"setPlayerScore", api_setPlayerScore, 1,0},
	{"getPlayerScore", api_getPlayerScore, 1,0},
	{"talkAs", api_talkAs, 1,0},
	{"runcmdAs", api_runcmdAs, 1,0},
	{"teleport",api_teleport, 1,0},
	{"tellraw",api_tellraw, 1,0},
	{"addItem",api_addItem, 1,0},
	{0,0,0,0}
};
// ģ������
static PyModuleDef mcModule = {
	PyModuleDef_HEAD_INIT, "mc", NULL, -1, mcMethods,
	NULL, NULL, NULL, NULL
};
// ģ���ʼ������
static PyObject* PyInit_mc() {
	return PyModule_Create(&mcModule);
}
#pragma endregion
#pragma region Hook
// ��ȡָ�����
SYMHOOK(_CmdQueue, VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	Cmd_Queue = original(_this);
	return Cmd_Queue;
}
// ��ȡ��ͼ��ʼ����Ϣ
SYMHOOK(_Level, VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	Level_ = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	return Level_;
}
// ��ȡ��Ϸ��ʼ��ʱ������Ϣ
SYMHOOK(_GameSession, VA, "??0GameSession@@QEAA@AEAVNetworkHandler@@V?$unique_ptr@VServerNetworkHandler@@U?$default_delete@VServerNetworkHandler@@@std@@@std@@AEAVLoopbackPacketSender@@V?$unique_ptr@VNetEventCallback@@U?$default_delete@VNetEventCallback@@@std@@@3@V?$unique_ptr@VLevel@@U?$default_delete@VLevel@@@std@@@3@E@Z",
	void* a1, void* a2, VA* a3, void* a4, void* a5, void* a6, void* a7) {
	Server_Network_Handle = *a3;
	return original(a1, a2, a3, a4, a5, a6, a7);
}
// ����ע��
SYMHOOK(_Command, void, "?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",
	void* _this) {
	for (pair<const string, string> cmd : Command) {
		SYMCALL<void>("?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@UCommandFlag@@3@Z",
			_this, cmd.first.c_str(), cmd.second.c_str(), 0, 0, 64);
	}
	original(_this);
}
// �Ʒְ�����ע�ᣨ����ʱ��ȡ���еļƷְ����ƣ�
SYMHOOK(_Scoreboard, void*, "??0ServerScoreboard@@QEAA@VCommandSoftEnumRegistry@@PEAVLevelStorage@@@Z",
	void* _this, void* a2, void* a3) {
	Score_Board = (Scoreboard*)original(_this, a2, a3);
	return Score_Board;
}
// ��������ָ̨�����
SYMHOOK(_logout, VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
	VA handle, const char* str, VA size) {
	if (handle == STD_COUT_HANDLE) {
		CallAll(u8"��̨���", "s", str);
		if (!res)return 0;
	}
	return original(handle, str, size);
}
// ����̨����
SYMHOOK(_Input, bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "pyreload") {
		if (!Py_FinalizeEx()) {
			PyFuncs.clear();
			init();
			return false;
		}
	}
	CallAll(u8"��̨����", "s", *cmd);
	RET(_this, cmd);
}
// ��Ҽ�������
SYMHOOK(_LoadName, VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	string uuid = p->getUuid();
	onlinePlayers[uuid] = p;
	PlayerList.insert(p);
	getPlayerInfo(p);
	CallAll(u8"��������", "{s:s,s:[f,f,f],s:i,s:i,s:s}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"uuid", uuid.c_str()
	);
	return original(a1, p);
}
// ����뿪��Ϸ
SYMHOOK(_PlayerLeft, void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	string uuid = p->getUuid();
	PlayerList.erase(p);
	onlinePlayers.erase(uuid);
	getPlayerInfo(p);
	CallAll(u8"�뿪��Ϸ", "{s:s,s:[f,f,f],s:i,s:i,s:s}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"uuid", uuid.c_str()
	);
	return original(_this, p, v3);
}
// ��Ҽ�����Ʒ
SYMHOOK(_take, char, "?take@Player@@QEAA_NAEAVActor@@HH@Z",
	Player* p, Actor* a, VA a3, VA a4) {
	//getPlayerInfo(p);
	//CallAll("Take","");
	return original(p, a, a3, a4);
}
// ��Ҳ�����Ʒ
SYMHOOK(_useItem, bool, "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
	void* _this, ItemStack* item, BlockPos* bp, unsigned __int8 a4, void* v5, Block* b) {
	Player* p = *(Player**)((VA)_this + 8);
	getPlayerInfo(p);
	short iid = item->getId();
	short iaux = item->getAuxValue();
	string iname = item->getName();
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	CallAll(u8"ʹ����Ʒ", "{s:s,s:[f,f,f],s:i,s:i,s:i,s:s,s:s,s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"itemid", iid,
		"itemaux", iaux,
		"itemname", iname,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, item, bp, a4, v5, b);
}
// ��ҷ��÷���
SYMHOOK(_PlacedBlock, bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, Actor* p, bool _bool) {
	if (p && PlayerCheck(p)) {
		BlockLegacy* bl = b->getBlockLegacy();
		short bid = bl->getBlockItemID();
		string bn = bl->getBlockName();
		getPlayerInfo(p);
		CallAll(u8"���÷���", "{s:s,s:[f,i,f],s:i,s:s,s:i,s:[i,i,i],s:i}",
			"playername", pn.c_str(),
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"blockname", bn.c_str(),
			"blockid", bid,
			"position", bp->x, bp->y, bp->z,
			"isstand", st
		);
		RET(_this, b, bp, a4, p, _bool);
	}
	return original(_this, b, bp, a4, p, _bool);
}
// ����ƻ�����
SYMHOOK(_DestroyBlock, bool, "?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
	void* _this, BlockPos* bp) {
	Player* p = *(Player**)((VA)_this + 8);
	BlockSource* bs = *(BlockSource**)(*((VA*)_this + 1) + 800);
	Block* b = bs->getBlock(bp);
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	getPlayerInfo(p);
	CallAll(u8"�ƻ�����", "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, bp);
}
// ��ҿ���׼��
SYMHOOK(_OpenChest, bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	getPlayerInfo(p);
	CallAll(u8"������", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, p, bp);
}
// ��ҿ�Ͱ׼��
SYMHOOK(_OpenBarrel, bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	getPlayerInfo(p);
	CallAll(u8"��ľͰ", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, p, bp);
}
// ��ҹر�����
SYMHOOK(_CloseChest, void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	//auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	CallAll(u8"�ر�����", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	return original(_this, p);
}
// ��ҹر�ľͰ
SYMHOOK(_CloseBarrel, void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	getPlayerInfo(p);
	CallAll(u8"�ر�ľͰ", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	return original(_this, p);
}
// ��ҷ���ȡ��
SYMHOOK(_Slot, void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
	LevelContainerModel* a1, VA slot) {
	VA v3 = *((VA*)a1 + 26);	// IDA LevelContainerModel::_getContainer
	BlockSource* bs = *(BlockSource**)(*(VA*)(v3 + 808) + 72);
	BlockPos* bp = (BlockPos*)((char*)a1 + 216);
	BlockLegacy* bl = bs->getBlock(bp)->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	if (bid == 54 || bid == 130 || bid == 146 || bid == -203 || bid == 205 || bid == 218) {	// �����ӡ�Ͱ��ǱӰ�е������������
		VA v5 = (*(VA(**)(LevelContainerModel*))(*(VA*)a1 + 160))(a1);
		if (v5) {
			ItemStack* is = (ItemStack*)(*(VA(**)(VA, VA))(*(VA*)v5 + 40))(v5, slot);
			auto iid = is->getId();
			auto iaux = is->getAuxValue();
			auto isize = is->getStackSize();
			auto iname = is->getName();
			auto p = a1->getPlayer();
			getPlayerInfo(p);
			CallAll(u8"����ȡ��", "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i,s:i,s:i,s:s,s:i,s:i}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"blockname", bn.c_str(),
				"blockid", bid,
				"position", bp->x, bp->y, bp->z,
				"isstand", st,
				"itemid", iid,
				"itemcount", isize,
				"itemname", iname.c_str(),
				"itemaux", iaux,
				"slot", slot
			);
		}
	}
	original(a1, slot);
}
// ���ѡ���
SYMHOOK(_SelectedForm, void, "?handle@?$PacketHandlerDispatcherInstance@VModalFormResponsePacket@@$0A@@@UEBAXAEBVNetworkIdentifier@@AEAVNetEventCallback@@AEAV?$shared_ptr@VPacket@@@std@@@Z",
	VA _this, VA id, VA handle,/*(ModalFormResponsePacket**)*/VA* fp) {
	VA fmp = *fp;
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		handle, id, *((char*)fmp + 16));
	if (PlayerCheck(p)) {
		unsigned fid = *((unsigned*)fmp + 40);
		string x = *((string*)fmp + 48);
		if (releaseForm(fid)) {
			getPlayerInfo(p);
			CallAll(u8"ѡ���", "{s:s,s:[f,f,f],s:i,s:i,s:s,s:s,s:i}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"isstand", st,
				"uuid", p->getUuid().c_str(),
				"selected", x.c_str(),
				"formid", fid
			);
			return;
		}
	}
	original(_this, id, handle, fp);
}
// ��ҹ���
SYMHOOK(_attack, bool, "?attack@Player@@UEAA_NAEAVActor@@@Z",
	Player* p, Actor* a) {
	string an = a->getNameTag();
	string atn = a->getTypeName();
	float hm, h;
	a->getHealth(hm, h);
	getPlayerInfo(p);
	CallAll(u8"��ҹ���", "{s:s,s:[f,f,f],s:i,s:i,s:s,s:s,s:f}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"actorname", an.c_str(),
		"actortype", atn.c_str(),
		"actorhealth", h
	);
	RET(p, a);
}
// ����л�ά��
SYMHOOK(_ChangeDimension, bool, "?_playerChangeDimension@Level@@AEAA_NPEAVPlayer@@AEAVChangeDimensionRequest@@@Z",
	void* l, Player* p, void* req) {
	bool result = original(l, p, req);
	if (result) {
		getPlayerInfo(p);
		CallAll(u8"�л�γ��", "{s:s,s:[f,f,f],s:i,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"isstand", st
		);
	}
	return result;
}
// ��������
SYMHOOK(_MobDie, void, "?die@Mob@@UEAAXAEBVActorDamageSource@@@Z",
	Mob* _this, VA dmsg) {
	PyObject* args = PyDict_New();
	char v72;
	Actor* SrAct = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		*(VA*)(_this + 816), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	string sr_name = "";
	string sr_type = "";
	if (SrAct) {
		sr_name = SrAct->getNameTag();
		int srtype = PlayerCheck(SrAct) ? 319 : SrAct->getEntityTypeId();
		SYMCALL<string&>("?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&sr_type, srtype);
	}
	if (PlayerCheck(_this)) {
		getPlayerInfo(((Player*)_this));
		string pt;
		SYMCALL<string&>("?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&pt, 319);
		PyDict_SetItemString(args, "playername", Py_BuildValue("s", pn));
		PyDict_SetItemString(args, "isstand", Py_BuildValue("i", st));
		PyDict_SetItemString(args, "mobname", Py_BuildValue("s", pn.c_str()));
		PyDict_SetItemString(args, "mobtype", Py_BuildValue("s", pt.c_str()));
		PyDict_SetItemString(args, "XYZ", Py_BuildValue("[f,f,f]", pp->x, pp->y, pp->z));
	}
	else {
		Vec3* pp = _this->getPos();
		PyDict_SetItemString(args, "mobname", Py_BuildValue("s", _this->getNameTag().c_str()));
		PyDict_SetItemString(args, "mobtype", Py_BuildValue("s", _this->getEntityTypeName().c_str()));
		PyDict_SetItemString(args, "XYZ", Py_BuildValue("[f,f,f]", pp->x, pp->y, pp->z));
	}
	PyDict_SetItemString(args, "srcname", Py_BuildValue("s", sr_name.c_str()));
	PyDict_SetItemString(args, "srctype", Py_BuildValue("s", sr_type.c_str()));
	PyDict_SetItemString(args, "dmcase", Py_BuildValue("i", *((unsigned*)dmsg + 2)));
	CallAll(u8"��������", "O", args);
	Py_DECREF(args);
	if (res) original(_this, dmsg);
}
// ��������
SYMHOOK(_MobHurt, bool, "?_hurt@Mob@@MEAA_NAEBVActorDamageSource@@H_N1@Z",
	Mob* _this, VA dmsg, int a3, int a4, bool a5) {
	PyObject* args = PyDict_New();
	char v72;
	Actor* SrAct = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		*(VA*)(_this + 816), *(VA*)(*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72), 0);
	string sr_name = "";
	string sr_type = "";
	if (SrAct) {
		sr_name = SrAct->getNameTag();
		int srtype = PlayerCheck(SrAct) ? 319 : SrAct->getEntityTypeId();
		SYMCALL<string&>("?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&sr_type, srtype);
	}
	if (PlayerCheck(_this)) {
		getPlayerInfo(((Player*)_this));
		string pt;
		SYMCALL<string&>("?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&pt, 319);
		PyDict_SetItemString(args, "playername", Py_BuildValue("s", pn));
		PyDict_SetItemString(args, "isstand", Py_BuildValue("i", st));
		PyDict_SetItemString(args, "mobname", Py_BuildValue("s", pn.c_str()));
		PyDict_SetItemString(args, "mobtype", Py_BuildValue("s", pt.c_str()));
		PyDict_SetItemString(args, "XYZ", Py_BuildValue("[f,f,f]", pp->x, pp->y, pp->z));
	}
	else {
		Vec3* pp = _this->getPos();
		PyDict_SetItemString(args, "mobname", Py_BuildValue("s", _this->getNameTag().c_str()));
		PyDict_SetItemString(args, "mobtype", Py_BuildValue("s", _this->getEntityTypeName().c_str()));
		PyDict_SetItemString(args, "XYZ", Py_BuildValue("[f,f,f]", pp->x, pp->y, pp->z));
	}
	PyDict_SetItemString(args, "srcname", Py_BuildValue("s", sr_name.c_str()));
	PyDict_SetItemString(args, "srctype", Py_BuildValue("s", sr_type.c_str()));
	PyDict_SetItemString(args, "dmcase", Py_BuildValue("i", *((unsigned*)dmsg + 2)));
	CallAll(u8"��������", "O", args);
	Py_DECREF(args);
	RET(_this, dmsg, a3, a4, a5);
}
// �������
SYMHOOK(_respawn, void, "?respawn@Player@@UEAAXXZ",
	Player* p) {
	getPlayerInfo(p);
	CallAll(u8"�������", "{s:s,s:[f,f,f],s:i,s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st
	);
	original(p);
}
// ������Ϣ
SYMHOOK(_Chat, void, "?fireEventPlayerMessage@MinecraftEventing@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@000@Z",
	void* _this, string& sender, string& target, string& msg, string& style) {
	CallAll(u8"������Ϣ", "{s:s,s:s,s:s,s:s}",
		"sender", sender,
		"target", target,
		"msg", msg,
		"style", style
	);
	original(_this, sender, target, msg, style);
}
// ��������ı�
SYMHOOK(_InputText, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVTextPacket@@@Z",
	VA _this, VA id, TextPacket* tp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)tp + 16));
	if (p) {
		getPlayerInfo(p);
		CallAll(u8"�����ı�", "{s:s,s:[f,f,f],s:i,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"msg", tp->toString().c_str(),
			"isstand", st
		);
	}
	original(_this, id, tp);
}
// �������ָ��
SYMHOOK(_InputCommand, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z",
	VA _this, VA id, CommandRequestPacket* crp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)crp + 16));
	if (p) {
		getPlayerInfo(p);
		CallAll(u8"����ָ��", "{s:s,s:[f,f,f],s:i,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"cmd", crp->toString(),
			"isstand", st
		);
	}
	original(_this, id, crp);
}
// ��ը
SYMHOOK(_explode, bool, "?explode@Level@@QEAAXAEAVBlockSource@@PEAVActor@@AEBVVec3@@M_N3M3@Z",
	Level* _this, BlockSource* bs, Actor* a3, const Vec3 pos, float a5, bool a6, bool a7, float a8, bool a9) {
	if (a3) {
		CallAll(u8"��ը", "{s:i,s:[f,f,f]}",
			"power", a5,
			"XYZ", pos.x, pos.y, pos.z);
	}
	return original(_this, bs, a3, pos, a5, a6, a7, a8, a9);
}
#pragma endregion
// �������
void init() {
	cout(u8"[���]BDSPyrunner���سɹ�~");
	PyPreConfig cfg;
	PyPreConfig_InitIsolatedConfig(&cfg);
	Py_PreInitialize(&cfg);
	PyImport_AppendInittab("mc", &PyInit_mc); //����һ��ģ��
	Py_Initialize();
	FILE* f;
	_finddata64i32_t fileinfo;//���ڲ��ҵľ��
	long long handle = _findfirst64i32("./py/*.py", &fileinfo);
	do {
		Py_NewInterpreter();
		cout(u8"��ȡPy�ļ�:" << fileinfo.name);
		string fn = "./py/";
		fn += fileinfo.name;
		fopen_s(&f, fn.c_str(), "rb");
		PyRun_SimpleFileExFlags(f, fileinfo.name, 1, 0);
	} while (!_findnext64i32(handle, &fileinfo));
	_findclose(handle);
}