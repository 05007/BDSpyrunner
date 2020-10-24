#include "预编译头.h"
#include "结构体.hpp"
#include <thread>
#define PY_SSIZE_T_CLEAN
#include "include/Python.h"
#pragma region 宏定义
//调用所有函数
#define CallAll(type,...)									\
	bool res = true;										\
	for (auto & fn :funcs[type]) {							\
		PyObject* r = PyObject_CallFunction(fn,__VA_ARGS__);\
		PyErr_Print();int bar = 1;PyArg_Parse(r,"p",&bar);	\
		if (!bar) res = false;Py_XDECREF(r);				\
	}
//标准流输出信息
#define cout(...) cout <<__VA_ARGS__<< endl
//THook返回判断
#define RET(...) \
	if (res) return original(__VA_ARGS__);\
	else return 0
//根据Player*获取玩家基本信息
#define getPlayerInfo(p) \
	string pn = p->getNameTag();\
	int did = p->getDimensionId();\
	Vec3* pp = p->getPos();\
	BYTE st = p->isStand();
#pragma endregion
#pragma region 全局变量
static VA cmd_queue = 0;
static VA level = 0;
static VA server_network_handle = 0;
static const VA STD_COUT_HANDLE = *(VA*)GetServerSymbol("__imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A");
static unordered_map<string, vector<PyObject*>> funcs;//py函数
static unordered_map<string, Player*> onlinePlayers;//在线玩家
static unordered_map<Player*, bool> playerSign;//玩家在线
static map<unsigned, bool> fids;//表单ID
static map<string, string> command;//注册命令
static Scoreboard* scoreboard;//储存计分板名称
#pragma endregion
#pragma region 函数定义
// 插件载入
static PyObject* PyInit_mc();
void init() {
	cout(u8"[插件]BDSPyrunner加载成功~");
	PyPreConfig cfg;
	PyPreConfig_InitIsolatedConfig(&cfg);
	cfg.utf8_mode = 1;
	PyStatus status = Py_PreInitialize(&cfg);
	if (PyStatus_Exception(status))Py_ExitStatusException(status);
	PyImport_AppendInittab("mc", &PyInit_mc); //增加一个模块
	Py_Initialize();
	FILE* f;
	_finddata64i32_t fileinfo;//用于查找的句柄
	long long handle = _findfirst64i32("./py/*.py", &fileinfo);
	do {
		Py_NewInterpreter();
		cout(u8"读取Py文件:" << fileinfo.name);
		string fn = "./py/";
		fn += fileinfo.name;
		fopen_s(&f, fn.c_str(), "rb");
		PyRun_SimpleFileExFlags(f, fileinfo.name, 1, 0);
	} while (!_findnext64i32(handle, &fileinfo));
	_findclose(handle);
}
// 多线程延时
static void delay(int time, PyObject* func) {
	Sleep(time);
	if (PyCallable_Check(func))
		PyObject_CallFunction(func, 0);
	else
		Py_DECREF(func);
}
// 执行后端指令
static bool runcmd(string cmd) {
	if (cmd_queue) {
		SYMCALL<bool>("??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			cmd_queue, cmd);
		return true;
	}
	return false;
}
// 获取一个未被使用的基于时间秒数的id
static unsigned getFormId() {
	unsigned id = (int)time(0) + rand();
	do {
		++id;
	} while (id == 0 || fids[id]);
	fids[id] = true;
	return id;
}
// 创建数据包
static inline void cPacket(VA& tpk, int p) {
	SYMCALL<VA>("?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",
		&tpk, p);
}
// 放弃一个表单
static bool releaseForm(unsigned fid)
{
	if (fids[fid]) {
		fids.erase(fid);
		return true;
	}
	return false;
}
// 发送一个表单数据包
static unsigned sendForm(string uuid, string str) {
	unsigned fid = getFormId();
	Player* p = onlinePlayers[uuid];
	if (playerSign[p]) {
		VA tpk;//ModalFormRequestPacket
		cPacket(tpk, 100);
		*(VA*)(tpk + 40) = fid;
		*(string*)(tpk + 48) = str;
		p->sendPacket(tpk);
	}
	return fid;
}
// 跨服传送
static bool transferServer(string uuid, string address, int port) {
	Player* p = onlinePlayers[uuid];
	if (playerSign[p]) {
		VA tpk;//TransferPacket
		cPacket(tpk, 85);
		*(string*)(tpk + 40) = address;
		*(VA*)(tpk + 72) = port;
		p->sendPacket(tpk);
		return true;
	}
	return false;
}
// 命令输出
static void logout(string str) {
	SYMCALL<VA>("??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
		STD_COUT_HANDLE, str.c_str(), str.length());
}
// 获取计分板
static int getscoreboard(Player* p, string n) {
	Objective* obj = scoreboard->getObjective(&n);
	ScoreInfo si[2];
	auto scores = obj->getPlayerScore(si, scoreboard->getScoreboardID(p));
	return scores->getCount();
}
// 设置计分板
static bool setScoreborad(Player* p, string n) {}
// 模拟玩家发送消息
static bool sendText(string uuid, string msg, int mode) {
	Player* p = onlinePlayers[uuid];
	if (playerSign[p]) {	// IDA ServerNetworkHandler::handle
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
// 模拟玩家执行命令
static bool runcmdAs(string uuid, string cmd) {
	Player* p = onlinePlayers[uuid];
	if (playerSign[p]) {
		VA tpk;//CommandRequestPacket
		cPacket(tpk, 76);
		*(string*)(tpk + 40) = cmd;
		return true;
	}
	return false;
}
// 设置侧边栏
static bool setSidebar(string uuid) {
	Player* p = onlinePlayers[uuid];
	if (playerSign[p]) {
		VA tpk;//SetDisplayObjectivePacket
		cPacket(tpk, 108);
		//*(string*)(tpk + 32) = "aa";
		//*(string*)(tpk + 56) = "bb";
		//*(string*)(tpk + 88) = "cc";
		//*(string*)(tpk + 120) = "dd";
		//*(string*)(tpk + 152) = "ee";
		p->sendPacket(tpk);
	}
	return true;
}
// 判断指针是否为玩家列表中指针
static inline bool checkIsPlayer(void* p) {
	return playerSign[(Player*)p];
}
#pragma endregion
#pragma region api函数
// 指令输出
static PyObject* api_logout(PyObject* self, PyObject* args) {
	const char* msg;
	if (PyArg_ParseTuple(args, "s", &msg))
		logout(msg);
	return Py_None;
}
// 执行指令
static PyObject* api_runcmd(PyObject* self, PyObject* args) {
	const char* cmd;
	if (PyArg_ParseTuple(args, "s", &cmd))
		runcmd(cmd);
	return Py_None;
}
// 延时
static PyObject* api_setTimeout(PyObject* self, PyObject* args) {
	int time = 0;
	PyObject* func = 0;
	if (PyArg_ParseTuple(args, "Oi", &func, &time)) {
		thread(delay, time, func).detach();
	}
	return Py_None;
}
// 获取在线玩家列表
static PyObject* api_getOnLinePlayers(PyObject* self, PyObject* args) {
	PyObject* ret = PyDict_New();
	for (auto& op : playerSign) {
		Player* p = op.first;
		if (op.second) {
			PyDict_SetItemString(ret, p->getNameTag().c_str(), Py_BuildValue("[s,s]", p->getUuid().c_str(), p->getXuid(level).c_str()));
		}
	}
	thread(delay, 1, ret).detach();
	return ret;
}
// 设置监听
static PyObject* api_setListener(PyObject* self, PyObject* args) {
	const char* m;
	PyObject* func;
	if (PyArg_ParseTuple(args, "sO", &m, &func) && PyCallable_Check(func))
		funcs[m].push_back(func);
	return Py_None;
}
// 发送表单
static PyObject* api_sendForm(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* str;
	if (PyArg_ParseTuple(args, "ss", &uuid, &str)) {
		return Py_BuildValue("i", sendForm(uuid, str));
	}
	return Py_None;
}
// 跨服传送
static PyObject* api_transferServer(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* address;
	int port;
	if (PyArg_ParseTuple(args, "ssi", &uuid, &address, &port)) {
		return Py_BuildValue("i", transferServer(uuid, address, port));
	}
	return Py_None;
}
// 获取玩家手持
static PyObject* api_getHand(PyObject* self, PyObject* args) {
	const char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* p = onlinePlayers[uuid];
		if (playerSign[p]) {
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
// 获取玩家信息
static PyObject* api_getPlayerInfo(PyObject* self, PyObject* args) {
	const char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* p = onlinePlayers[uuid];
		if (playerSign[p]) {
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
// 获取玩家权限
static PyObject* api_getPlayerPerm(PyObject* self, PyObject* args) {
	const char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			return Py_BuildValue("i", pl->getPermission());
		}
	}
	return Py_None;
}
// 设置玩家权限
static PyObject* api_setPlayerPerm(PyObject* self, PyObject* args) {
	const char* uuid;
	int lv;
	if (PyArg_ParseTuple(args, "si", &uuid, &lv)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			pl->setPermissionLevel(lv);
		}
	}
	return Py_None;
}
// 增加玩家等级
static PyObject* api_addLevel(PyObject* self, PyObject* args) {
	const char* uuid;
	int lv = 0;
	if (PyArg_ParseTuple(args, "si", &uuid, &lv)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			SYMCALL<void>("?addLevels@Player@@UEAAXH@Z", pl, lv);
		}
	}
	return Py_None;
}
// 设置玩家名字
static PyObject* api_setNameTag(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* name;
	if (PyArg_ParseTuple(args, "ss", &uuid, &name)) {
		Player* pl = onlinePlayers[uuid];
		if (playerSign[pl]) {
			pl->setName(name);
		}
	}
	return Py_None;
}
// 设置指令说明
static PyObject* api_setCommandDescribe(PyObject* self, PyObject* args) {
	const char* cmd;
	const char* des;
	if (PyArg_ParseTuple(args, "ss", &cmd, &des)) {
		command[cmd] = des;
	}
	return Py_None;
}
// 获取玩家计分板分数
static PyObject* api_getPlayerScore(PyObject* self, PyObject* args) {
	const char* pn;
	const char* objname;
	if (PyArg_ParseTuple(args, "ss", &pn, &objname)) {
		return Py_BuildValue("i", getscoreboard(onlinePlayers[pn], objname));
	}
	return Py_None;
}
// 模拟玩家发送文本
static PyObject* api_talkAs(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* msg;
	if (PyArg_ParseTuple(args, "ss", &uuid, &msg)) {
		return Py_BuildValue("i", sendText(uuid, msg, 1));
	}
	return Py_None;
}
// 模拟玩家执行指令
static PyObject* api_runcmdAs(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* cmd;
	if (PyArg_ParseTuple(args, "ss", &uuid, &cmd)) {
		return Py_BuildValue("i", runcmdAs(uuid, cmd));
	}
	return Py_None;
}
// 传送玩家
static PyObject* api_teleport(PyObject* self, PyObject* args) {
	const char* pn; float x, y, z; int did;
	if (PyArg_ParseTuple(args, "sfffi", &pn, &x, &y, &z, &did)) {
		Player* p = onlinePlayers[pn];
		p->teleport({ x,y,z }, did);
	}
	return Py_None;
}
// 原始输出
static PyObject* api_tellraw(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* msg;
	if (PyArg_ParseTuple(args, "ss", &uuid, &msg)) {
		return Py_BuildValue("i", sendText(uuid, msg, 0));
	}
	return Py_None;
}
// 设置侧边栏
static PyObject* api_setSidebar(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* msg;
	if (PyArg_ParseTuple(args, "ss", &uuid, &msg)) {
		return Py_BuildValue("i", setSidebar(uuid));
	}
	return Py_None;
}
// 方法列表
static PyMethodDef mcMethods[] = {
	{"logout", api_logout, METH_VARARGS,""},
	{"runcmd", api_runcmd, METH_VARARGS,""},
	{"setTimeout", api_setTimeout, METH_VARARGS,""},
	{"getOnLinePlayers", api_getOnLinePlayers, METH_NOARGS,""},
	{"setListener", api_setListener, METH_VARARGS,""},
	{"sendForm", api_sendForm, METH_VARARGS,""},
	{"transferServer", api_transferServer, METH_VARARGS,""},
	{"getHand", api_getHand, METH_VARARGS,""},
	{"getPlayerInfo", api_getPlayerInfo, METH_VARARGS,""},
	{"getPlayerPerm", api_getPlayerPerm, METH_VARARGS,""},
	{"setPlayerPerm",api_setPlayerPerm, METH_VARARGS,""},
	{"addLevel", api_addLevel, METH_VARARGS,""},
	{"setNameTag", api_setNameTag, METH_VARARGS,""},
	{"setCommandDescribe", api_setCommandDescribe, METH_VARARGS,""},
	{"getPlayerScore", api_getPlayerScore, METH_VARARGS,""},
	{"talkAs", api_talkAs, METH_VARARGS,""},
	{"runcmdAs", api_runcmdAs, METH_VARARGS,""},
	{"teleport",api_teleport, METH_VARARGS,""},
	{"tellraw",api_tellraw, METH_VARARGS,""},
	{"setSidebar",api_setSidebar, METH_VARARGS,""},
	{NULL,NULL,NULL,NULL}
};
// 模块声明
static PyModuleDef mcModule = {
	PyModuleDef_HEAD_INIT, "mc", NULL, -1, mcMethods,
	NULL, NULL, NULL, NULL
};
// 模块初始化函数
static PyObject* PyInit_mc() {
	return PyModule_Create(&mcModule);
}
#pragma endregion
#pragma region THook列表
// 获取指令队列
THook(VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	cmd_queue = original(_this);
	return cmd_queue;
}
// 获取地图初始化信息
THook(VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	level = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	return level;
}
// 获取游戏初始化时基本信息
THook(VA, "??0GameSession@@QEAA@AEAVNetworkHandler@@V?$unique_ptr@VServerNetworkHandler@@U?$default_delete@VServerNetworkHandler@@@std@@@std@@AEAVLoopbackPacketSender@@V?$unique_ptr@VNetEventCallback@@U?$default_delete@VNetEventCallback@@@std@@@3@V?$unique_ptr@VLevel@@U?$default_delete@VLevel@@@std@@@3@E@Z",
	void* a1, void* a2, VA* a3, void* a4, void* a5, void* a6, void* a7) {
	server_network_handle = *a3;
	return original(a1, a2, a3, a4, a5, a6, a7);
}
// 命令注册
THook(void, "?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",
	void* _this) {
	for (auto& cmd : command) {
		SYMCALL<void>("?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@UCommandFlag@@3@Z",
			_this, cmd.first.c_str(), cmd.second.c_str(), 0, 0, 64);
	}
	original(_this);
}
// 计分板命令注册（开服时获取所有的计分板名称）
THook(void*, "??0ServerScoreboard@@QEAA@VCommandSoftEnumRegistry@@PEAVLevelStorage@@@Z",
	void* _this, void* a2, void* a3) {
	scoreboard = (Scoreboard*)original(_this, a2, a3);
	return scoreboard;
}
// 服务器后台指令输出
THook(VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
	VA handle, char* str, VA size) {
	if (handle == STD_COUT_HANDLE) {
		CallAll(u8"后台输出", "s", str);
		RET(handle, str, size);
	}
	return original(handle, str, size);
}
// 控制台输入
THook(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "pyreload") {
		if (!Py_FinalizeEx()) {
			funcs.clear();
			init();
			return false;
		}
	}
	CallAll(u8"后台输入", "s", *cmd);
	RET(_this, cmd);
}
// 玩家加载名字
THook(VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	string uuid = p->getUuid();
	onlinePlayers[uuid] = p;
	playerSign[p] = true;
	getPlayerInfo(p);
	CallAll(u8"加载名字", "{s:s,s:[f,f,f],s:i,s:i,s:s}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"uuid", uuid.c_str()
	);
	return original(a1, p);
}
// 玩家离开游戏
THook(void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	string uuid = p->getUuid();
	playerSign.erase(p);
	onlinePlayers.erase(uuid);
	getPlayerInfo(p);
	CallAll(u8"离开游戏", "{s:s,s:[f,f,f],s:i,s:i,s:s}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"uuid", uuid.c_str()
	);
	return original(_this, p, v3);
}
// 玩家捡起物品
THook(char, "?take@Player@@QEAA_NAEAVActor@@HH@Z",
	Player* p, Actor* a, VA a3, VA a4) {
	//getPlayerInfo(p);
	//CallAll("Take","");
	return original(p, a, a3, a4);
}
// 玩家操作物品
THook(bool, "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
	void* _this, ItemStack* item, BlockPos* bp, unsigned __int8 a4, void* v5, Block* b) {
	Player* p = *(Player**)((VA)_this + 8);
	getPlayerInfo(p);
	short iid = item->getId();
	short iaux = item->getAuxValue();
	string iname = item->getName();
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	CallAll(u8"使用物品", "{s:s,s:[f,f,f],s:i,s:i,s:i,s:s,s:s,s:i,s:[i,i,i],s:i}",
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
// 玩家放置方块
THook(bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, Actor* p, bool _bool) {
	if (p && checkIsPlayer(p)) {
		BlockLegacy* bl = b->getBlockLegacy();
		short bid = bl->getBlockItemID();
		string bn = bl->getBlockName();
		getPlayerInfo(p);
		CallAll(u8"放置方块", "{s:s,s:[f,i,f],s:i,s:s,s:i,s:[i,i,i],s:i}",
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
// 玩家破坏方块
THook(bool, "?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
	void* _this, BlockPos* bp) {
	Player* p = *(Player**)((VA)_this + 8);
	BlockSource* bs = *(BlockSource**)(*((VA*)_this + 1) + 800);
	Block* b = bs->getBlock(bp);
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	getPlayerInfo(p);
	CallAll(u8"破坏方块", "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i}",
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
// 玩家开箱准备
THook(bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	getPlayerInfo(p);
	CallAll(u8"玩家开箱", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, p, bp);
}
// 玩家开桶准备
THook(bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	getPlayerInfo(p);
	CallAll(u8"玩家开桶", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, p, bp);
}
// 玩家关闭箱子
THook(void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	//auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	CallAll(u8"玩家关箱", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	return original(_this, p);
}
// 玩家关闭木桶
THook(void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	//auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	CallAll(u8"玩家关桶", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	return original(_this, p);
}
// 玩家放入取出
THook(void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
	LevelContainerModel* a1, VA slot) {
	VA v3 = *((VA*)a1 + 26);	// IDA LevelContainerModel::_getContainer
	BlockSource* bs = *(BlockSource**)(*(VA*)(v3 + 808) + 72);
	BlockPos* bp = (BlockPos*)((char*)a1 + 216);
	Block* b = bs->getBlock(bp);
	short bid = b->getBlockLegacy()->getBlockItemID();
	string bn = b->getBlockLegacy()->getBlockName();
	if (bid == 54 || bid == 130 || bid == 146 || bid == -203 || bid == 205 || bid == 218) {	// 非箱子、桶、潜影盒的情况不作处理
		VA v5 = (*(VA(**)(LevelContainerModel*))(*(VA*)a1 + 160))(a1);
		if (v5) {
			ItemStack* is = (ItemStack*)(*(VA(**)(VA, VA))(*(VA*)v5 + 40))(v5, slot);
			auto iid = is->getId();
			auto iaux = is->getAuxValue();
			auto isize = is->getStackSize();
			auto iname = string(is->getName());
			auto p = a1->getPlayer();
			getPlayerInfo(p);
			CallAll(u8"放入取出", "{s:s,s:[f,f,f],s:i,s:s,s:i,s:[i,i,i],s:i,s:i,s:i,s:s,s:i,s:i}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"blockname", bn.c_str(),
				"blockid", bid,
				"position", bp->x, bp->y, bp->z,
				"isstand", st,
				"itemid", iid,
				"itemcount", isize,
				"itemname", iname,
				"itemaux", iaux,
				"slot", slot
			);
			return;
		}
	}
	else original(a1, slot);
}
// 玩家选择表单
THook(void, "?handle@?$PacketHandlerDispatcherInstance@VModalFormResponsePacket@@$0A@@@UEBAXAEBVNetworkIdentifier@@AEAVNetEventCallback@@AEAV?$shared_ptr@VPacket@@@std@@@Z",
	VA _this, VA id, VA handle, ModalFormResponsePacket** fp) {
	ModalFormResponsePacket* fmp = *fp;
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		handle, id, *((char*)fmp + 16));
	if (p) {
		unsigned fid = fmp->getFormId();
		if (releaseForm(fid)) {
			getPlayerInfo(p);
			CallAll(u8"选择表单", "{s:s,s:[f,f,f],s:i,s:i,s:s,s:s,s:i}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"isstand", st,
				"uuid", p->getUuid().c_str(),
				"selected", fmp->getSelectStr().c_str(),
				"formid", fid
			);
			return;
		}
	}
	original(_this, id, handle, fp);
}
// 玩家攻击
THook(bool, "?attack@Player@@UEAA_NAEAVActor@@@Z",
	Player* p, Actor* a) {
	string an = a->getNameTag();
	string atn = a->getTypeName();
	float hm, h;
	a->getHealth(hm, h);
	getPlayerInfo(p);
	CallAll(u8"玩家攻击", "{s:s,s:[f,f,f],s:i,s:i,s:s,s:s,s:f}",
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
// 玩家切换维度
THook(bool, "?_playerChangeDimension@Level@@AEAA_NPEAVPlayer@@AEAVChangeDimensionRequest@@@Z",
	void* l, Player* p, void* req) {
	bool result = original(l, p, req);
	if (result) {
		getPlayerInfo(p);
		CallAll(u8"切换纬度", "{s:s,s:[f,f,f],s:i,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"isstand", st
		);
	}
	return result;
}
// 生物死亡
THook(void, "?die@Mob@@UEAAXAEBVActorDamageSource@@@Z",
	Mob* _this, void* dmsg) {
	PyObject* args = PyDict_New();
	char v72;
	VA v0 = (VA)_this;
	VA v1 = (VA)dmsg;
	VA v7 = *((VA*)(v0 + 816));
	VA* srActid = (VA*)(*(VA(__fastcall**)(VA, char*))(*(VA*)v1 + 64))(v1, &v72);
	Actor* SrAct = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		v7, *srActid, 0);
	string sr_name = "";
	string sr_type = "";
	if (SrAct) {
		sr_name = SrAct->getNameTag();
		int srtype = checkIsPlayer(SrAct) ? 319 : SrAct->getEntityTypeId();
		SYMCALL<string&>("?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&sr_type, srtype);
	}
	if (checkIsPlayer(_this)) {
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
	CallAll(u8"生物死亡", "O", args);
	Py_DECREF(args);
	if (res) original(_this, dmsg);
}
// 生物受伤
THook(bool, "?_hurt@Mob@@MEAA_NAEBVActorDamageSource@@H_N1@Z",
	Mob* _this, void* dmsg, int a3, int a4, bool a5) {
	PyObject* args = PyDict_New();
	char v72;
	VA v0 = (VA)_this;
	VA v1 = (VA)dmsg;
	VA v7 = *((VA*)(v0 + 816));
	VA* srActid = (VA*)(*(VA(__fastcall**)(VA, char*))(*(VA*)v1 + 64))(v1, &v72);
	Actor* SrAct = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		v7, *srActid, 0);
	string sr_name = "";
	string sr_type = "";
	if (SrAct) {
		sr_name = SrAct->getNameTag();
		int srtype = checkIsPlayer(SrAct) ? 319 : SrAct->getEntityTypeId();
		SYMCALL<string&>("?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&sr_type, srtype);
	}
	if (checkIsPlayer(_this)) {
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
	CallAll(u8"生物受伤", "O", args);
	Py_DECREF(args);
	RET(_this, dmsg, a3, a4, a5);
}
// 玩家重生
THook(void, "?respawn@Player@@UEAAXXZ",
	Player* p) {
	getPlayerInfo(p);
	CallAll(u8"玩家重生", "{s:s,s:[f,f,f],s:i,s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st
	);
	original(p);
}
// 聊天消息
THook(void, "?fireEventPlayerMessage@MinecraftEventing@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@000@Z",
	void* _this, string& sender, string& target, string& msg, string& style) {
	CallAll(u8"聊天消息", "{s:s,s:s,s:s,s:s}",
		"sender", sender,
		"target", target,
		"msg", msg,
		"style", style
	);
	original(_this, sender, target, msg, style);
}
// 玩家输入文本
THook(void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVTextPacket@@@Z",
	VA _this, VA id, TextPacket* tp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)tp + 16));
	if (p) {
		getPlayerInfo(p);
		CallAll(u8"输入文本", "{s:s,s:[f,f,f],s:i,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"msg", tp->toString().c_str(),
			"isstand", st
		);
	}
	original(_this, id, tp);
}
// 玩家输入指令
THook(void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z",
	VA _this, VA id, CommandRequestPacket* crp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)crp + 16));
	if (p) {
		getPlayerInfo(p);
		CallAll(u8"输入指令", "{s:s,s:[f,f,f],s:i,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"cmd", crp->toString(),
			"isstand", st
		);
	}
	original(_this, id, crp);
}
#pragma endregion
