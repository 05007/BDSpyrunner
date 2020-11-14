#include "预编译头.h"
#include "结构体.hpp"
#include <thread>
#include <unordered_map>
#include "Python/Python.h"
#pragma region 宏定义
//调用所有函数
#define CallAll(type,...)								\
bool res = true;									\
for (PyObject* fn :PyFuncs[type]) {					\
	if(PyObject_CallFunction(fn,__VA_ARGS__)==Py_False)res = false;\
}PyErr_Print()
#define cout(...) cout <<__VA_ARGS__<< endl
//THook返回判断
#define RET(...) if (!res) return 0;return original(__VA_ARGS__)
//根据Player*获取玩家基本信息
#define getPlayerInfo(p) \
	string pn = p->getNameTag();\
	int did = p->getDimensionId();\
	Vec3* pp = p->getPos();\
	BYTE st = p->isStand();
#pragma endregion
#pragma region 全局变量
static VA Cmd_Queue;
static VA Level_;
static VA Server_Network_Handle;
static Scoreboard* Score_Board;//储存计分板名称
static const VA STD_COUT_HANDLE = fetch(VA, GetServerSymbol("__imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A"));
static unsigned FormId = 0;//表单ID
static unordered_map<string, vector<PyObject*>> PyFuncs;//py函数
static unordered_map<string, Player*> onlinePlayers;//在线玩家
static unordered_map<Player*, bool> PlayerList;//玩家列表
static unordered_map<string, string> Command;//注册命令
#pragma endregion
#pragma region 函数定义
void init();
// 判断指针是否为玩家列表中指针
static inline bool PlayerCheck(void* p) {
	return PlayerList[(Player*)p];
}
// 多线程延时
static void delay(int time, PyObject* func) {
	Sleep(time);
	if (PyCallable_Check(func))
		PyObject_CallFunction(func, 0);
	else
		Py_DECREF(func);
}
// 创建数据包
static inline void cPacket(VA& pkt, int p) {
	SYMCALL<VA>("?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",
		&pkt, p);
}
// 设置侧边栏
static bool setSidebar(string uuid, string title, string name) {
	Player* p = onlinePlayers[uuid];
	if (PlayerCheck(p)) {
		VA pkt;//setDisplayObjectivePacket
		cPacket(pkt, 107);
		*(string*)(pkt + 40) = "sidebar";
		*(string*)(pkt + 72) = name;
		*(string*)(pkt + 104) = title;
		*(string*)(pkt + 136) = "dummy";
		*(char*)(pkt + 168) = 0;
		p->sendPacket(pkt);
	}
	return true;
}
#pragma endregion
#pragma region api函数
// 指令输出
static PyObject* api_logout(PyObject* self, PyObject* args) {
	const char* msg;
	if (PyArg_ParseTuple(args, "s", &msg)) {
		SYMCALL<VA>("??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
			STD_COUT_HANDLE, msg, strlen(msg));
		return Py_True;
	}
	return Py_False;
}
// 执行指令
static PyObject* api_runcmd(PyObject* self, PyObject* args) {
	const char* cmd;
	if (PyArg_ParseTuple(args, "s", &cmd)) {
		SYMCALL<bool>("??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			Cmd_Queue, (string)cmd);
		return Py_True;
	}
	return Py_False;
}
// 延时
static PyObject* api_setTimeout(PyObject* self, PyObject* args) {
	int time; PyObject* func;
	if (PyArg_ParseTuple(args, "Oi", &func, &time)) {
		thread(delay, time, func).detach();
	}
	return Py_None;
}
// 获取uuid
static PyObject* api_UUID(PyObject* self, PyObject* args) {
	const char* name;
	if (PyArg_ParseTuple(args, "s", &name))
		for (auto& p : PlayerList) {
			if (p.first->getNameTag() == name)
				return Py_BuildValue("s", p.first->getUuid().c_str());
		}
	return Py_None;
}
// 设置监听
static PyObject* api_setListener(PyObject* self, PyObject* args) {
	const char* m;
	PyObject* func;
	if (PyArg_ParseTuple(args, "sO", &m, &func) && PyCallable_Check(func)) {
		PyFuncs[m].push_back(func);
		return Py_True;
	}
	return Py_False;
}
// 发送表单
static PyObject* api_sendForm(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* str;
	if (PyArg_ParseTuple(args, "ss", &uuid, &str)) {
		unsigned fid = FormId++;
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			VA pkt;//ModalFormRequestPacket
			cPacket(pkt, 100);
			*(unsigned*)(pkt + 40) = fid;
			*(string*)(pkt + 48) = str;
			p->sendPacket(pkt);
		}
		return Py_BuildValue("i", fid);
	}
	return Py_None;
}
// 跨服传送
static PyObject* api_transferServer(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* address;
	int port;
	if (PyArg_ParseTuple(args, "ssi", &uuid, &address, &port)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			VA pkt;//TransferPacket
			cPacket(pkt, 85);
			*(string*)(pkt + 40) = address;
			*(VA*)(pkt + 72) = port;
			p->sendPacket(pkt);
			return Py_True;
		}
	}
	return Py_False;
}
// 获取玩家手持
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
// 获取玩家信息
static PyObject* api_getPlayerInfo(PyObject* self, PyObject* args) {
	const char* uuid;
	if (PyArg_ParseTuple(args, "s", &uuid)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			getPlayerInfo(p);
			return Py_BuildValue("{s:s,s:[f,f,f],s:i,s:i,s:f}",
				"playername", pn,
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", did,
				"isstand", st,
				"health", p->getHealth()
			);
		}
	}
	return Py_None;
}
// 获取玩家权限
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
// 设置玩家权限
static PyObject* api_setPlayerPerm(PyObject* self, PyObject* args) {
	const char* uuid;
	int lv;
	if (PyArg_ParseTuple(args, "si", &uuid, &lv)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			p->setPermissionLevel(lv);
			return Py_True;
		}
	}
	return Py_False;
}
// 增加玩家等级
static PyObject* api_addLevel(PyObject* self, PyObject* args) {
	const char* uuid;
	int lv = 0;
	if (PyArg_ParseTuple(args, "si", &uuid, &lv)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			SYMCALL<void>("?addLevels@Player@@UEAAXH@Z", p, lv);
			return Py_True;
		}
	}
	return Py_False;
}
// 设置玩家名字
static PyObject* api_setNameTag(PyObject* self, PyObject* args) {
	const char* uuid;
	const char* name;
	if (PyArg_ParseTuple(args, "ss", &uuid, &name)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			p->setName(name);
			return Py_True;
		}
	}
	return Py_False;
}
// 设置指令说明
static PyObject* api_setCommandDescribe(PyObject* self, PyObject* args) {
	const char* cmd;
	const char* des;
	if (PyArg_ParseTuple(args, "ss", &cmd, &des)) {
		Command[cmd] = des;
		return Py_True;
	}
	return Py_False;
}
// 玩家分数
static PyObject* api_getPlayerScore(PyObject* self, PyObject* args) {
	const char* uuid, * obj;
	if (PyArg_ParseTuple(args, "ss", &uuid, &obj)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			Objective* testobj = Score_Board->getObjective(obj);
			if (testobj) {
				ScoreInfo a[2];
				return Py_BuildValue("i", testobj->getPlayerScore(a, Score_Board->getScoreboardId(p))->getCount());
			}
			cout("bad objective:" << obj);
		}
	}
	return Py_BuildValue("i", 0);
}
static PyObject* api_setPlayerScore(PyObject* self, PyObject* args) {
	const char* uuid, * obj; int count; char mode;
	if (PyArg_ParseTuple(args, "ssii", &uuid, &obj, &count, &mode)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			Objective* testobj = Score_Board->getObjective(obj);
			if (testobj) {
				Score_Board->modifyPlayerScore((ScoreboardId*)Score_Board->getScoreboardId(p), testobj, count, mode);
				return Py_True;
			}
			else cout("bad objective:" << obj);
		}
	}
	return Py_False;
}
// 模拟玩家发送文本
static PyObject* api_talkAs(PyObject* self, PyObject* args) {
	const char* uuid, * msg;
	if (PyArg_ParseTuple(args, "ss", &uuid, &msg)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {	// IDA ServerNetworkHandler::handle
			VA pkt;//TextPacket
			cPacket(pkt, 9);
			fetch(char, pkt + 40) = 1;
			fetch(string, pkt + 48) = p->getNameTag();
			fetch(string, pkt + 80) = msg;
			p->sendPacket(pkt);
			return Py_True;
		}
	}
	return Py_False;
}
// 模拟玩家执行指令
static PyObject* api_runcmdAs(PyObject* self, PyObject* args) {
	const char* uuid, * cmd;
	if (PyArg_ParseTuple(args, "ss", &uuid, &cmd)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {
			VA pkt;//CommandRequestPacket
			cPacket(pkt, 76);
			fetch(string, pkt + 40) = cmd;
			return Py_True;
		}
	}
	return Py_False;
}
// 传送玩家
static PyObject* api_teleport(PyObject* self, PyObject* args) {
	const char* pn; float x, y, z; int did;
	if (PyArg_ParseTuple(args, "sfffi", &pn, &x, &y, &z, &did)) {
		Player* p = onlinePlayers[pn];
		p->teleport({ x,y,z }, did);
		return Py_True;
	}
	return Py_False;
}
// 原始输出
static PyObject* api_tellraw(PyObject* self, PyObject* args) {
	const char* uuid, * msg;
	if (PyArg_ParseTuple(args, "ss", &uuid, &msg)) {
		Player* p = onlinePlayers[uuid];
		if (PlayerCheck(p)) {	// IDA ServerNetworkHandler::handle
			VA pkt;//TextPacket
			cPacket(pkt, 9);
			fetch(char, pkt + 40) = 0;
			fetch(string, pkt + 48) = p->getNameTag();
			fetch(string, pkt + 80) = msg;
			p->sendPacket(pkt);
			return Py_True;
		}
	}
	return Py_False;
}
// 设置侧边栏
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
// 方法列表
static PyMethodDef mcMethods[] = {
	{"logout", api_logout, 1,0},//第3个值:1有参数4无参数,第4个值:描述
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
// 模块声明
static PyModuleDef mcModule = {
	PyModuleDef_HEAD_INIT, "mc", NULL, -1, mcMethods,
	NULL, NULL, NULL, NULL
};
#pragma endregion
#pragma region Hook
SYMHOOK(获取指令队列, VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	Cmd_Queue = original(_this);
	return Cmd_Queue;
}
SYMHOOK(获取地图初始化信息, VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	Level_ = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	return Level_;
}
SYMHOOK(获取游戏初始化信息, VA, "??0GameSession@@QEAA@AEAVNetworkHandler@@V?$unique_ptr@VServerNetworkHandler@@U?$default_delete@VServerNetworkHandler@@@std@@@std@@AEAVLoopbackPacketSender@@V?$unique_ptr@VNetEventCallback@@U?$default_delete@VNetEventCallback@@@std@@@3@V?$unique_ptr@VLevel@@U?$default_delete@VLevel@@@std@@@3@E@Z",
	void* a1, void* a2, VA* a3, void* a4, void* a5, void* a6, void* a7) {
	Server_Network_Handle = *a3;
	return original(a1, a2, a3, a4, a5, a6, a7);
}
SYMHOOK(命令注册, void, "?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",
	void* _this) {
	for (pair<const string, string> cmd : Command) {
		SYMCALL<void>("?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@UCommandFlag@@3@Z",
			_this, cmd.first.c_str(), cmd.second.c_str(), 0, 0, 64);
	}
	original(_this);
}
SYMHOOK(计分板, void*, "??0ServerScoreboard@@QEAA@VCommandSoftEnumRegistry@@PEAVLevelStorage@@@Z",
	void* _this, void* a2, void* a3) {
	Score_Board = (Scoreboard*)original(_this, a2, a3);
	return Score_Board;
}
SYMHOOK(后台输出, VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
	VA handle, const char* str, VA size) {
	if (handle == STD_COUT_HANDLE) {
		CallAll(u8"后台输出", "s", str);
		if (!res)return 0;
	}
	return original(handle, str, size);
}
SYMHOOK(控制台输入, bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "pyreload") {
		if (!Py_FinalizeEx()) {
			PyFuncs.clear();
			init();
			return false;
		}
	}
	CallAll(u8"后台输入", "s", *cmd);
	RET(_this, cmd);
}
SYMHOOK(玩家加入, VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	string uuid = p->getUuid();
	onlinePlayers[uuid] = p;
	PlayerList[p] = true;
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
SYMHOOK(玩家离开游戏, void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	string uuid = p->getUuid();
	PlayerList.erase(p);
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
SYMHOOK(玩家捡起物品, char, "?take@Player@@QEAA_NAEAVActor@@HH@Z",
	Player* p, Actor* a, VA a3, VA a4) {
	//getPlayerInfo(p);
	//CallAll("Take","");
	return original(p, a, a3, a4);
}
SYMHOOK(玩家操作物品, bool, "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
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
SYMHOOK(玩家放置方块, bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, Actor* p, bool _bool) {
	if (p && PlayerCheck(p)) {
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
SYMHOOK(玩家破坏方块, bool, "?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
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
SYMHOOK(玩家开箱准备, bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	getPlayerInfo(p);
	CallAll(u8"打开箱子", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, p, bp);
}
SYMHOOK(玩家开桶准备, bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@@Z",
	void* _this, Player* p, BlockPos* bp) {
	getPlayerInfo(p);
	CallAll(u8"打开木桶", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	RET(_this, p, bp);
}
SYMHOOK(玩家关闭箱子, void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	//auto bs = (BlockSource*)((Level*)p->getLevel())->getDimension(p->getDimensionId())->getBlockSouce();
	//auto b = bs->getBlock(bp);
	getPlayerInfo(p);
	CallAll(u8"关闭箱子", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	return original(_this, p);
}
SYMHOOK(玩家关闭木桶, void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	void* _this, Player* p) {
	auto real_this = reinterpret_cast<void*>(reinterpret_cast<VA>(_this) - 248);
	auto bp = reinterpret_cast<BlockActor*>(real_this)->getPosition();
	getPlayerInfo(p);
	CallAll(u8"关闭木桶", "{s:s,s:[f,f,f],s:i,s:[i,i,i],s:i}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"position", bp->x, bp->y, bp->z,
		"isstand", st
	);
	return original(_this, p);
}
SYMHOOK(玩家放入取出, void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
	LevelContainerModel* a1, VA slot) {
	VA v3 = *((VA*)a1 + 26);	// IDA LevelContainerModel::_getContainer
	BlockSource* bs = *(BlockSource**)(*(VA*)(v3 + 808) + 72);
	BlockPos* bp = (BlockPos*)((char*)a1 + 216);
	BlockLegacy* bl = bs->getBlock(bp)->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	if (bid == 54 || bid == 130 || bid == 146 || bid == -203 || bid == 205 || bid == 218) {	// 非箱子、桶、潜影盒的情况不作处理
		VA v5 = (*(VA(**)(LevelContainerModel*))(*(VA*)a1 + 160))(a1);
		if (v5) {
			ItemStack* is = (ItemStack*)(*(VA(**)(VA, VA))(*(VA*)v5 + 40))(v5, slot);
			auto iid = is->getId();
			auto iaux = is->getAuxValue();
			auto isize = is->getStackSize();
			auto iname = is->getName();
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
				"itemname", iname.c_str(),
				"itemaux", iaux,
				"slot", slot
			);
		}
	}
	original(a1, slot);
}
SYMHOOK(玩家攻击, bool, "?attack@Player@@UEAA_NAEAVActor@@@Z",
	Player* p, Actor* a) {
	string an = a->getNameTag();
	string atn = a->getTypeName();
	getPlayerInfo(p);
	CallAll(u8"玩家攻击", "{s:s,s:[f,f,f],s:i,s:i,s:s,s:s,s:f}",
		"playername", pn,
		"XYZ", pp->x, pp->y, pp->z,
		"dimensionid", did,
		"isstand", st,
		"actorname", an.c_str(),
		"actortype", atn.c_str(),
		"actorhealth", a->getHealth()
	);
	RET(p, a);
}
SYMHOOK(玩家切换维度, bool, "?_playerChangeDimension@Level@@AEAA_NPEAVPlayer@@AEAVChangeDimensionRequest@@@Z",
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
SYMHOOK(生物死亡, void, "?die@Mob@@UEAAXAEBVActorDamageSource@@@Z",
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
	CallAll(u8"生物死亡", "O", args);
	Py_DECREF(args);
	if (res) original(_this, dmsg);
}
SYMHOOK(生物受伤, bool, "?_hurt@Mob@@MEAA_NAEBVActorDamageSource@@H_N1@Z",
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
	CallAll(u8"生物受伤", "O", args);
	Py_DECREF(args);
	RET(_this, dmsg, a3, a4, a5);
}
SYMHOOK(玩家重生, void, "?respawn@Player@@UEAAXXZ",
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
SYMHOOK(聊天消息, void, "?fireEventPlayerMessage@MinecraftEventing@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@000@Z",
	void* _this, string& sender, string& target, string& msg, string& style) {
	CallAll(u8"聊天消息", "{s:s,s:s,s:s,s:s}",
		"sender", sender,
		"target", target,
		"msg", msg,
		"style", style
	);
	original(_this, sender, target, msg, style);
}
SYMHOOK(玩家输入文本, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVTextPacket@@@Z",
	VA _this, VA id, /*(TextPacket*)*/VA tp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)tp + 16));
	if (p) {
		string msg = *(string*)(tp + 80);
		getPlayerInfo(p);
		CallAll(u8"输入文本", "{s:s,s:[f,f,f],s:i,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"msg", msg.c_str(),
			"isstand", st
		);
		if (res)original(_this, id, tp);
	}
}
SYMHOOK(玩家输入指令, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z",
	VA _this, VA id, /*(CommandRequestPacket*)*/VA crp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)crp + 16));
	if (p) {
		string cmd = *(string*)(crp + 40);
		getPlayerInfo(p);
		CallAll(u8"输入指令", "{s:s,s:[f,f,f],s:i,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"cmd", cmd.c_str(),
			"isstand", st
		);
		if (res)original(_this, id, crp);
	}
}
SYMHOOK(玩家选择表单, void, "?handle@?$PacketHandlerDispatcherInstance@VModalFormResponsePacket@@$0A@@@UEBAXAEBVNetworkIdentifier@@AEAVNetEventCallback@@AEAV?$shared_ptr@VPacket@@@std@@@Z",
	VA _this, VA id, VA handle,/*(ModalFormResponsePacket**)*/VA* fp) {
	VA fmp = *fp;
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		handle, id, *((char*)fmp + 16));
	if (PlayerCheck(p)) {
		unsigned fid = fetch(unsigned, fmp + 40);
		string x = *(string*)(fmp + 48);
		if (x[x.length() - 1] == '\n')x[x.length() - 1] = '\0';
		getPlayerInfo(p);
		CallAll(u8"选择表单", "{s:s,s:[f,f,f],s:i,s:i,s:s,s:s,s:i}",
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"isstand", st,
			"uuid", p->getUuid().c_str(),
			"selected", x.c_str(),
			"formid", fid
		);
	}
	original(_this, id, handle, fp);
}
SYMHOOK(更新命令方块, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandBlockUpdatePacket@@@Z",
	VA _this, VA id, /*(CommandBlockUpdatePacket*)*/VA cbp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)cbp + 16));
	if (PlayerCheck(p)) {
		BlockPos bp = fetch(BlockPos, cbp + 40);
		string cmd = fetch(string, cbp + 64);
		getPlayerInfo(p);
		CallAll(u8"命令方块", "{s:s,s:s,s:[f,f,f],s:i,s:i,s:[i,i,i]}",
			"cmd", cmd.c_str(),
			"playername", pn,
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", did,
			"isstand", st,
			"position", bp.x, bp.y, bp.z
		);
		if (res)original(_this, id, cbp);
	}
}
SYMHOOK(爆炸, bool, "?explode@Level@@QEAAXAEAVBlockSource@@PEAVActor@@AEBVVec3@@M_N3M3@Z",
	Level* _this, BlockSource* bs, Actor* a3, const Vec3 pos, float a5, bool a6, bool a7, float a8, bool a9) {
	if (a3) {
		CallAll(u8"爆炸", "{s:[f,f,f],s:s,s:i,s:i,s:i}",
			"XYZ", pos.x, pos.y, pos.z,
			"entity", a3->getEntityTypeName().c_str(),
			"entityid", a3->getEntityTypeId(),
			"dimensionid", a3->getDimensionId(),
			"power", a5
		);
	}
	else {
		CallAll(u8"爆炸", "{s:[f,f,f],s:i,s:i}",
			"XYZ", pos.x, pos.y, pos.z,
			"dimensionid", a3->getDimensionId(),
			"power", a5
		);
	}
	return original(_this, bs, a3, pos, a5, a6, a7, a8, a9);
}
#pragma endregion
// 插件载入
void init() {
	cout(u8"[插件]BDSPyrunner v0.0.1 Load...");
	PyPreConfig cfg;
	PyPreConfig_InitIsolatedConfig(&cfg);
	Py_PreInitialize(&cfg);
	PyImport_AppendInittab("mc", []() {return PyModule_Create(&mcModule); }); //增加一个模块
	Py_Initialize();
	FILE* f;
	_finddata64i32_t fileinfo;//用于查找的句柄
	long long handle = _findfirst64i32("./py/*.py", &fileinfo);
	if (handle != -1) {
		do {
			Py_NewInterpreter();
			cout(u8"读取Py文件:" << fileinfo.name);
			fopen_s(&f, ("./py/" + (string)fileinfo.name).c_str(), "rb");
			PyRun_SimpleFileExFlags(f, fileinfo.name, 1, 0);
		} while (!_findnext64i32(handle, &fileinfo));
		_findclose(handle);
	}
	else cout(u8"没有找到目录");
}