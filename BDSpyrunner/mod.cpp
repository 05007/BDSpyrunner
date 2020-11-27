#include "BDS.hpp"
#include <thread>
#include "Python/Python.h"
#pragma warning(disable:4996)
#pragma region 宏定义
//调用所有函数
#define CallAll(type,...)								\
bool res = true;									\
for (PyObject* fn :PyFuncs[type]) {					\
	if(PyObject_CallFunction(fn,__VA_ARGS__)==Py_False)res = false;\
}PyErr_Print()
#define out(...) cout <<__VA_ARGS__<< endl
//THook返回判断
#define RET(...) if (!res) return 0;return original(__VA_ARGS__)
// 创建数据包
#define	cpkt(pkt,m) SYMCALL<VA>("?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",&pkt,m)
#pragma endregion
#pragma region 全局变量
VA _cmdqueue, _level, _ServerNetworkHandle;
const VA STD_COUT_HANDLE = f(VA, GetServerSymbol("__imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A"));
Scoreboard* _scoreboard;//储存计分板名称
unsigned _formid = 0;//表单ID
unordered_map<string, vector<PyObject*>> PyFuncs;//Py函数
unordered_map<Player*, bool> PlayerList;//玩家列表
unordered_map<string, string> Command;//注册命令
#pragma endregion
#pragma region 函数定义
void init();
// 判断指针是否为玩家列表中指针
inline bool PlayerCheck(void* p) {
	return PlayerList[(Player*)p];
}
// 多线程延时
void delay(int time, PyObject* func) {
	Sleep(time);
	if (PyCallable_Check(func))
		PyObject_CallFunction(func, 0);
	else
		Py_DECREF(func);
}
// 设置侧边栏
bool setSidebar(Player* p, string title, string name) {
	if (PlayerCheck(p)) {
		VA pkt;//setDisplayObjectivePacket
		cpkt(pkt, 107);
		f(string, pkt + 40) = "sidebar";
		f(string, pkt + 72) = name;
		f(string, pkt + 104) = title;
		f(string, pkt + 136) = "dummy";
		f(char, pkt + 168) = 0;
		p->sendPacket(pkt);
	}
	return true;
}
#pragma endregion
#pragma region api函数
// 指令输出
PyObject* api_logout(PyObject* self, PyObject* args) {
	const char* msg;
	if (PyArg_ParseTuple(args, "s", &msg)) {
		SYMCALL<VA>("??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
			STD_COUT_HANDLE, msg, strlen(msg));
		return Py_True;
	}
	return Py_False;
}
// 执行指令
PyObject* api_runcmd(PyObject* self, PyObject* args) {
	const char* cmd;
	if (PyArg_ParseTuple(args, "s", &cmd)) {
		SYMCALL<bool>("??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			_cmdqueue, (string)cmd);
		return Py_True;
	}
	return Py_False;
}
// 延时
PyObject* api_setTimeout(PyObject* self, PyObject* args) {
	int time; PyObject* func;
	if (PyArg_ParseTuple(args, "Oi", &func, &time)) {
		thread(delay, time, func).detach();
	}
	return Py_None;
}
// 获取uuid
PyObject* api_UUID(PyObject* self, PyObject* args) {
	const char* name;
	if (PyArg_ParseTuple(args, "s", &name))
		for (auto& p : PlayerList) {
			if (p.first->getNameTag() == name)
				return Py_BuildValue("s", p.first->getUuid().c_str());
		}
	return Py_None;
}
// 设置监听
PyObject* api_setListener(PyObject* self, PyObject* args) {
	const char* m;
	PyObject* func;
	if (PyArg_ParseTuple(args, "sO", &m, &func) && PyCallable_Check(func)) {
		PyFuncs[m].push_back(func);
		return Py_True;
	}
	return Py_False;
}
// 发送表单
PyObject* api_sendForm(PyObject* self, PyObject* args) {
	Player* p;
	const char* str;
	if (PyArg_ParseTuple(args, "Ks", &p, &str)) {
		unsigned fid = _formid++;
		if (PlayerCheck(p)) {
			VA pkt;//ModalFormRequestPacket
			cpkt(pkt, 100);
			f(unsigned, pkt + 40) = fid;
			f(string, pkt + 48) = str;
			p->sendPacket(pkt);
		}
		return Py_BuildValue("i", fid);
	}
	return Py_None;
}
// 跨服传送
PyObject* api_transferServer(PyObject* self, PyObject* args) {
	Player* p;
	const char* address;
	int port;
	if (PyArg_ParseTuple(args, "Ksi", &p, &address, &port)) {
		if (PlayerCheck(p)) {
			VA pkt;//TransferPacket
			cpkt(pkt, 85);
			f(string, pkt + 40) = address;
			f(VA, pkt + 72) = port;
			p->sendPacket(pkt);
			return Py_True;
		}
	}
	return Py_False;
}
// 获取玩家手持
PyObject* api_getHand(PyObject* self, PyObject* args) {
	Player* p;
	if (PyArg_ParseTuple(args, "K", &p)) {
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
PyObject* api_getPlayerInfo(PyObject* self, PyObject* args) {
	Player* p;
	if (PyArg_ParseTuple(args, "K", &p)) {
		if (PlayerCheck(p)) {
			Vec3* pp = p->getPos();
			return Py_BuildValue("{s:s,s:[f,f,f],s:i,s:i,s:f}",
				"playername", p->getNameTag().c_str(),
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", p->getDimensionId(),
				"isstand", p->isStand(),
				"health", p->getHealth().first
			);
		}
	}
	return Py_None;
}
// 获取玩家权限
PyObject* api_getPlayerPerm(PyObject* self, PyObject* args) {
	Player* p;
	if (PyArg_ParseTuple(args, "K", &p)) {
		if (PlayerCheck(p)) {
			return Py_BuildValue("i", p->getPermission());
		}
	}
	return Py_None;
}
// 设置玩家权限
PyObject* api_setPlayerPerm(PyObject* self, PyObject* args) {
	Player* p;
	int lv;
	if (PyArg_ParseTuple(args, "Ki", &p, &lv)) {
		if (PlayerCheck(p)) {
			p->setPermissionLevel(lv);
			return Py_True;
		}
	}
	return Py_False;
}
// 增加玩家等级
PyObject* api_addLevel(PyObject* self, PyObject* args) {
	Player* p;
	int lv = 0;
	if (PyArg_ParseTuple(args, "Ki", &p, &lv)) {
		if (PlayerCheck(p)) {
			SYMCALL<void>("?addLevels@Player@@UEAAXH@Z", p, lv);
			return Py_True;
		}
	}
	return Py_False;
}
// 设置玩家名字
PyObject* api_setNameTag(PyObject* self, PyObject* args) {
	Player* p;
	const char* name;
	if (PyArg_ParseTuple(args, "Ks", &p, &name)) {
		if (PlayerCheck(p)) {
			p->setName(name);
			return Py_True;
		}
	}
	return Py_False;
}
// 设置指令说明
PyObject* api_setCommandDescribe(PyObject* self, PyObject* args) {
	const char* cmd;
	const char* des;
	if (PyArg_ParseTuple(args, "ss", &cmd, &des)) {
		Command[cmd] = des;
		return Py_True;
	}
	return Py_False;
}
// 玩家分数
PyObject* api_getPlayerScore(PyObject* self, PyObject* args) {
	Player* p;
	const char* obj;
	if (PyArg_ParseTuple(args, "Ks", &p, &obj)) {
		if (PlayerCheck(p)) {
			Objective* testobj = _scoreboard->getObjective(obj);
			if (testobj) {
				auto id = _scoreboard->getScoreboardId(p);
				auto score = testobj->getPlayerScore(id);
				return Py_BuildValue("i", score->getCount());
			}
			out("bad objective:" << obj);
		}
	}
	return Py_BuildValue("i", 0);
}
PyObject* api_modifyPlayerScore(PyObject* self, PyObject* args) {
	Player* p;
	const char* obj; int count; char mode;
	if (PyArg_ParseTuple(args, "Ksii", &p, &obj, &count, &mode)) {
		if (PlayerCheck(p)) {
			Objective* testobj = _scoreboard->getObjective(obj);
			if (testobj) {
				//mode:{set,add,remove}
				_scoreboard->modifyPlayerScore((ScoreboardId*)_scoreboard->getScoreboardId(p), testobj, count, mode);
				return Py_True;
			}
			else out("bad objective:" << obj);
		}
	}
	return Py_False;
}
// 模拟玩家发送文本
PyObject* api_talkAs(PyObject* self, PyObject* args) {
	Player* p;
	const char* msg;
	if (PyArg_ParseTuple(args, "Ks", &p, &msg)) {
		if (PlayerCheck(p)) {	// IDA ServerNetworkHandler::handle
			VA pkt;//TextPacket
			cpkt(pkt, 9);
			f(char, pkt + 40) = 1;
			f(string, pkt + 48) = p->getNameTag();
			f(string, pkt + 80) = msg;
			p->sendPacket(pkt);
			return Py_True;
		}
	}
	return Py_False;
}
// 模拟玩家执行指令
PyObject* api_runcmdAs(PyObject* self, PyObject* args) {
	Player* p;
	const char* cmd;
	if (PyArg_ParseTuple(args, "Ks", &p, &cmd)) {
		if (PlayerCheck(p)) {
			VA pkt;//CommandRequestPacket
			cpkt(pkt, 76);
			f(string, pkt + 40) = cmd;
			return Py_True;
		}
	}
	return Py_False;
}
// 传送玩家
PyObject* api_teleport(PyObject* self, PyObject* args) {
	Player* p; float x, y, z; int did;
	if (PyArg_ParseTuple(args, "Kfffi", &p, &x, &y, &z, &did)) {
		p->teleport({ x,y,z }, did);
		return Py_True;
	}
	return Py_False;
}
// 原始输出
PyObject* api_tellraw(PyObject* self, PyObject* args) {
	Player* p;
	const char* msg;
	if (PyArg_ParseTuple(args, "Ks", &p, &msg)) {
		if (PlayerCheck(p)) {	// IDA ServerNetworkHandler::handle
			VA pkt;//TextPacket
			cpkt(pkt, 9);
			f(char, pkt + 40) = 0;
			f(string, pkt + 48) = p->getNameTag();
			f(string, pkt + 80) = msg;
			p->sendPacket(pkt);
			return Py_True;
		}
	}
	return Py_False;
}
// 设置侧边栏
PyObject* api_addItem(PyObject* self, PyObject* args) {
	Player* p;
	int id, aux, count;
	if (PyArg_ParseTuple(args, "Kiii", &p, &id, &aux, &count)) {
		if (PlayerCheck(p)) {
			ItemStack item;
			item.getFromId(id, aux, count);
			p->addItem(&item);
		}
	}
	return Py_None;
}
// 方法列表
PyMethodDef m[] = {
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
   {"modifyPlayerScore", api_modifyPlayerScore, 1,0},
   {"getPlayerScore", api_getPlayerScore, 1,0},
   {"talkAs", api_talkAs, 1,0},
   {"runcmdAs", api_runcmdAs, 1,0},
   {"teleport", api_teleport, 1,0},
   {"tellraw", api_tellraw, 1,0},
   {"addItem", api_addItem, 1,0},
   {0,0,0,0}
};
// 模块声明
PyModuleDef mc = { PyModuleDef_HEAD_INIT, "mc", 0, -1,m,0,0,0,0 };
#pragma endregion
#pragma region Hook
THook(获取指令队列, VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	_cmdqueue = original(_this);
	return _cmdqueue;
}
THook(获取地图初始化信息, VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	_level = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	return _level;
}
THook(获取游戏初始化信息, VA, "??0GameSession@@QEAA@AEAVNetworkHandler@@V?$unique_ptr@VServerNetworkHandler@@U?$default_delete@VServerNetworkHandler@@@std@@@std@@AEAVLoopbackPacketSender@@V?$unique_ptr@VNetEventCallback@@U?$default_delete@VNetEventCallback@@@std@@@3@V?$unique_ptr@VLevel@@U?$default_delete@VLevel@@@std@@@3@E@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7) {
	_ServerNetworkHandle = f(VA, a3);
	return original(a1, a2, a3, a4, a5, a6, a7);
}
THook(命令注册, void, "?setup@KillCommand@@SAXAEAVCommandRegistry@@@Z",//"?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",
	VA _this) {
	for (auto& cmd : Command) {
		SYMCALL<void>("?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@UCommandFlag@@3@Z",
			_this, cmd.first.c_str(), cmd.second.c_str(), 0, 0, 0);
	}
	original(_this);
}
THook(计分板, Scoreboard*, "??0ServerScoreboard@@QEAA@VCommandSoftEnumRegistry@@PEAVLevelStorage@@@Z",
	VA _this, VA a2, VA a3) {
	_scoreboard = (Scoreboard*)original(_this, a2, a3);
	return _scoreboard;
}
THook(后台输出, VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
	VA handle, const char* str, VA size) {
	if (handle == STD_COUT_HANDLE) {
		CallAll(u8"后台输出", "s", str);
		if (!res)return 0;
	}
	return original(handle, str, size);
}
THook(后台输入, bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "pyreload") {
		Py_Finalize();
		PyFuncs.clear();
		init();
		return false;
	}
	CallAll(u8"后台输入", "s", *cmd);
	RET(_this, cmd);
}
THook(玩家加入, VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	PlayerList[p] = true;
	CallAll(u8"加载名字", "K", p);
	return original(a1, p);
}
THook(玩家离开游戏, void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	PlayerList.erase(p);
	CallAll(u8"离开游戏", "K", p);
	return original(_this, p, v3);
}
THook(玩家捡起物品, bool, "?take@Player@@QEAA_NAEAVActor@@HH@Z",
	Player* p, Actor* a, VA a3, VA a4) {
	//getPlayerInfo(p);
	//CallAll("Take","");
	return original(p, a, a3, a4);
}
THook(玩家操作物品, bool, "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
	VA _this, ItemStack* item, BlockPos* bp, unsigned __int8 a4, VA v5, Block* b) {
	Player* p = f(Player*, _this + 8);
	short iid = item->getId();
	short iaux = item->getAuxValue();
	string iname = item->getName();
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	CallAll(u8"使用物品", "{s:K,s:i,s:i,s:s,s:s,s:i,s:[i,i,i]}",
		"player", p,
		"itemid", iid,
		"itemaux", iaux,
		"itemname", iname,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, item, bp, a4, v5, b);
}
THook(玩家放置方块, bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, Actor* p, bool _bool) {
	if (p && PlayerCheck(p)) {
		BlockLegacy* bl = b->getBlockLegacy();
		short bid = bl->getBlockItemID();
		string bn = bl->getBlockName();
		CallAll(u8"放置方块", "{s:K,s:s,s:i,s:[i,i,i]}",
			"player", p,
			"blockname", bn.c_str(),
			"blockid", bid,
			"position", bp->x, bp->y, bp->z
		);
		RET(_this, b, bp, a4, p, _bool);
	}
	return original(_this, b, bp, a4, p, _bool);
}
THook(玩家破坏方块, bool, "?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
	VA _this, BlockPos* bp) {
	Player* p = f(Player*, _this + 8);
	BlockSource* bs = f(BlockSource*, f(VA, _this + 8) + 840);
	Block* b = bs->getBlock(bp);
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	CallAll(u8"破坏方块", "{s:K,s:s,s:i,s:[i,i,i]}",
		"player", p,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, bp);
}
THook(玩家开箱准备, bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
	VA _this, Player* p, BlockPos* bp) {
	CallAll(u8"打开箱子", "{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, p, bp);
}
THook(玩家开桶准备, bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
	VA _this, Player* p, BlockPos* bp) {
	CallAll(u8"打开木桶", "{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, p, bp);
}
THook(关闭箱子, void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	VA _this, Player* p) {
	auto bp = (BlockPos*)(_this - 204);
	CallAll(u8"关闭箱子", "{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	);
	original(_this, p);
}
THook(关闭木桶, void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	VA _this, Player* p) {
	auto bp = (BlockPos*)(_this - 204);
	CallAll(u8"关闭木桶", "{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	);
	original(_this, p);
}
THook(玩家放入取出, void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
	VA a1, VA slot) {
	VA v3 = f(VA, a1 + 208);// IDA LevelContainerModel::_getContainer line 22
	BlockSource* bs = f(BlockSource*, f(VA, v3 + 848) + 80);
	BlockPos* bp = (BlockPos*)(a1 + 216);
	BlockLegacy* bl = bs->getBlock(bp)->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	if (bid == 54 || bid == 130 || bid == 146 || bid == -203 || bid == 205 || bid == 218) {	// 非箱子、桶、潜影盒的情况不作处理
		VA v5 = (*(VA(**)(VA))(*(VA*)a1 + 160))(a1);
		if (v5) {
			ItemStack* is = (ItemStack*)(*(VA(**)(VA, VA))(*(VA*)v5 + 40))(v5, slot);
			auto iid = is->getId();
			auto iaux = is->getAuxValue();
			auto isize = is->getStackSize();
			auto iname = is->getName();
			auto p = f(Player*, a1 + 208);
			CallAll(u8"放入取出", "{s:K,s:s,s:i,s:[i,i,i],s:i,s:i,s:s,s:i,s:i}",
				"player", p,
				"blockname", bn.c_str(),
				"blockid", bid,
				"position", bp->x, bp->y, bp->z,
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
THook(玩家攻击, bool, "?attack@Player@@UEAA_NAEAVActor@@@Z",
	Player* p, Actor* a) {
	CallAll(u8"玩家攻击", "{s:K,s:K}",
		"player", p,
		"actor", a
	);
	RET(p, a);
}
THook(玩家切换维度, bool, "?_playerChangeDimension@Level@@AEAA_NPEAVPlayer@@AEAVChangeDimensionRequest@@@Z",
	VA _this, Player* p, VA req) {
	bool result = original(_this, p, req);
	if (result) {
		CallAll(u8"切换纬度", "K", p);
	}
	return result;
}
THook(生物死亡, void, "?die@Mob@@UEAAXAEBVActorDamageSource@@@Z",
	Mob* _this, VA dmsg) {
	char v72;
	Actor* sa = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		f(VA, _this + 856), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	string srcname;
	string srctype;
	Vec3 srcp;
	Vec3* mobp = _this->getPos();
	if (sa) {
		srcname = sa->getNameTag();
		srctype = sa->getEntityTypeName();
		srcp = *sa->getPos();
	}
	CallAll(u8"生物死亡", "{s:s,s:s,s:[f,f,f],s:i,s:s,s:s,s:[f,f,f]}",
		"srcname", srcname.c_str(),
		"srctype", srctype.c_str(),
		"srcpos", srcp.x, srcp.y, srcp.z,
		"dmcase", f(unsigned, dmsg + 8),
		"mobname", _this->getNameTag().c_str(),
		"mobtype", _this->getEntityTypeName().c_str(),
		"mobpos", mobp->x, mobp->y, mobp->z
	);
	if (res) original(_this, dmsg);
}
THook(生物受伤, bool, "?_hurt@Mob@@MEAA_NAEBVActorDamageSource@@H_N1@Z",
	Mob* _this, VA dmsg, int a3, bool a4, bool a5) {
	char v72;
	Actor* sa = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		f(VA, _this + 856), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	string srcname;
	string srctype;
	Vec3 srcp;
	Vec3* mobp = _this->getPos();
	if (sa) {
		srcname = sa->getNameTag();
		srctype = sa->getEntityTypeName();
		srcp = *sa->getPos();
	}
	CallAll(u8"生物受伤", "{s:s,s:s,s:[f,f,f],s:i,s:s,s:s,s:[f,f,f],s:i}",
		"srcname", srcname.c_str(),
		"srctype", srctype.c_str(),
		"srcpos", srcp.x, srcp.y, srcp.z,
		"dmcase", f(unsigned, dmsg + 8),
		"mobname", _this->getNameTag().c_str(),
		"mobtype", _this->getEntityTypeName().c_str(),
		"mobpos", mobp->x, mobp->y, mobp->z,
		"damage", a3
	);
	if (res) original(_this, dmsg, a3, a4, a5);
	return 0;
}
THook(玩家重生, void, "?respawn@Player@@UEAAXXZ",
	Player* p) {
	CallAll(u8"玩家重生", "K", p);
	original(p);
}
THook(聊天消息, void, "?fireEventPlayerMessage@MinecraftEventing@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@000@Z",
	VA _this, string& sender, string& target, string& msg, string& style) {
	CallAll(u8"聊天消息", "{s:s,s:s,s:s,s:s}",
		"sender", sender,
		"target", target,
		"msg", msg,
		"style", style
	);
	original(_this, sender, target, msg, style);
}
THook(玩家输入文本, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVTextPacket@@@Z",
	VA _this, VA id, /*(TextPacket*)*/VA tp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)tp + 16));
	if (p) {
		string msg = *(string*)(tp + 80);
		CallAll(u8"输入文本", "{s:K,s:s}",
			"player", p,
			"msg", msg.c_str()
		);
		if (res)original(_this, id, tp);
	}
}
THook(玩家输入指令, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z",
	VA _this, VA id, /*(CommandRequestPacket*)*/VA crp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)crp + 16));
	if (p) {
		string cmd = *(string*)(crp + 40);
		CallAll(u8"输入指令", "{s:K,s:s}",
			"player", p,
			"cmd", cmd.c_str()
		);
		if (res)original(_this, id, crp);
	}
}
THook(玩家选择表单, void, "?handle@?$PacketHandlerDispatcherInstance@VModalFormResponsePacket@@$0A@@@UEBAXAEBVNetworkIdentifier@@AEAVNetEventCallback@@AEAV?$shared_ptr@VPacket@@@std@@@Z",
	VA _this, VA id, VA handle,/*(ModalFormResponsePacket**)*/VA* fp) {
	VA fmp = *fp;
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		handle, id, f(char, fmp + 16));
	if (PlayerCheck(p)) {
		unsigned fid = f(unsigned, fmp + 40);
		string x = f(string, fmp + 48);
		if (x[x.length() - 1] == '\n')x[x.length() - 1] = '\0';
		CallAll(u8"选择表单", "{s:K,s:s,s:i}",
			"player", p,
			"selected", x.c_str(),
			"formid", fid
		);
	}
	original(_this, id, handle, fp);
}
THook(更新命令方块, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandBlockUpdatePacket@@@Z",
	VA _this, VA id, /*(CommandBlockUpdatePacket*)*/VA cbp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)cbp + 16));
	if (PlayerCheck(p)) {
		auto bp = f(BlockPos, cbp + 40);
		auto mode = f(unsigned short, cbp + 52);
		auto condition = f(bool, cbp + 54);
		auto redstone = f(bool, cbp + 55);
		auto cmd = f(string, cbp + 64);
		auto output = f(string, cbp + 96);
		auto rawname = f(string, cbp + 128);
		auto delay = f(int, cbp + 160);
		CallAll(u8"命令方块", "{s:Ks:i,s:i,s:i,s:s,s:s,s:s,s:i,s:[i,i,i]}",
			"player", p,
			"mode", mode,
			"condition", condition,
			"redstone", redstone,
			"cmd", cmd.c_str(),
			"output", output.c_str(),
			"rawname", rawname.c_str(),
			"delay", delay,
			"position", bp.x, bp.y, bp.z
		);
		if (res)original(_this, id, cbp);
	}
}
THook(爆炸, bool, "?explode@Level@@QEAAXAEAVBlockSource@@PEAVActor@@AEBVVec3@@M_N3M3@Z",
	Level* _this, BlockSource* bs, Actor* a3, const Vec3 pos, float a5, bool a6, bool a7, float a8, bool a9) {
	if (a3) {
		CallAll(u8"爆炸", "{s:[f,f,f],s:s,s:i,s:i,s:i}",
			"XYZ", pos.x, pos.y, pos.z,
			"entity", a3->getEntityTypeName().c_str(),
			"entityid", a3->getEntityTypeId(),
			"dimensionid", a3->getDimensionId(),
			"power", a5
		);
		RET(_this, bs, a3, pos, a5, a6, a7, a8, a9);
	}
	CallAll(u8"爆炸", "{s:[f,f,f],s:i,s:i}",
		"XYZ", pos.x, pos.y, pos.z,
		"dimensionid", a3->getDimensionId(),
		"power", a5
	);
	RET(_this, bs, a3, pos, a5, a6, a7, a8, a9);
}
THook(命令方块执行, bool, "?performCommand@CommandBlockActor@@QEAA_NAEAVBlockSource@@@Z",
	VA _this, BlockSource* a2) {
	//脉冲:0,重复:1,链:2
	int mode = SYMCALL<int>("?getMode@CommandBlockActor@@QEBA?AW4CommandBlockMode@@AEAVBlockSource@@@Z",
		_this, a2);
	//无条件:0,有条件:1
	bool condition = SYMCALL<bool>("?getConditionalMode@CommandBlockActor@@QEBA_NAEAVBlockSource@@@Z",
		_this, a2);
	string cmd = f(string, _this + 264);
	string rawname = f(string, _this + 296);
	BlockPos bp = f(BlockPos, _this + 44);
	CallAll(u8"命令方块执行", "{s:s,s:s,s:[i,i,i],s:i,s:i}",
		"cmd", cmd.c_str(),
		"name", rawname.c_str(),
		"XYZ", bp.x, bp.y, bp.z,
		"mode", mode,
		"condition", condition
	);
	RET(_this, a2);
}
#pragma endregion
void init() {
	puts("BDSPyrunner v0.0.5 Loading...");
	PyPreConfig cfg;
	PyPreConfig_InitIsolatedConfig(&cfg);
	Py_PreInitialize(&cfg);
	PyImport_AppendInittab("mc", [] {return PyModule_Create(&mc); }); //增加一个模块
	Py_Initialize();
	_finddata64i32_t fileinfo;//用于查找的句柄
	long long handle = _findfirst64i32("./py/*.py", &fileinfo);
	if (handle != -1) {
		do {
			Py_NewInterpreter();
			out(u8"读取Py文件:" << fileinfo.name);;
			PyRun_SimpleFileExFlags(fopen(("./py/" + (string)fileinfo.name).c_str(), "rb"), fileinfo.name, 1, 0);
		} while (!_findnext64i32(handle, &fileinfo));
		_findclose(handle);
	}
	else puts(u8"没有找到目录");
}
int __stdcall DllMain(HINSTANCE__* hModule, unsigned long res, void* lpReserved) {
	if (res == 1)
		init();
	return 1;
}