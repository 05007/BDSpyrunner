#include "pch.h"
#include "BDS.hpp"
#pragma warning(disable:4996)
#pragma region 宏定义
#define CPYFUNC(name) static PyObject* name(PyObject*, PyObject*args)
#define out(...) cout <<__VA_ARGS__<< endl
#define RET(...) if (!res) return 0;return original(__VA_ARGS__)
#define PlayerCheck(ptr)  PlayerList[(Player*)ptr]
#define createPacket(pkt, i) SYMCALL("?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",pkt,i);
#pragma endregion
#pragma region 全局变量
static VA _cmdqueue, _level, _ServerNetworkHandle;
static const VA STD_COUT_HANDLE = f(VA, GetServerSymbol("__imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A"));
static Scoreboard* _scoreboard;//储存计分板名称
static unsigned _formid = 1;//表单ID
static unordered_map<string, vector<PyObject*>> PyFuncs;//Py函数
static unordered_map<Player*, bool> PlayerList;//玩家列表
static unordered_map<string, string> Command;//注册命令
static unordered_map<string, PyObject*> ShareData;//注册命令
//static Json::Value pts;
#pragma endregion
#pragma region 函数定义
void init();
extern "C" _declspec(dllimport) void __stdcall	Sleep(unsigned);
static Json::Value toJson(string s) {
	Json::Value jv;
	Json::CharReaderBuilder re;
	Json::CharReader* r(re.newCharReader());
	string errs;
	bool ok = r->parse(s.c_str(), s.c_str() + s.length(), &jv, &errs);
	if (!ok)out(u8"Json Error" << errs);
	delete r;
	return jv;
}
static string toString(Json::Value jv) {
	Json::StreamWriterBuilder wb;
	Json::StreamWriter* w(wb.newStreamWriter());
	ostringstream os;
	w->write(jv, &os);
	delete w;
	return os.str();
}
static bool callpy(const char* type, PyObject* val) {
	for (PyObject*& fn : PyFuncs[type]) {
		if (PyObject_CallFunctionObjArgs(fn, val) == Py_False)
			return false;
	}
	PyErr_Print();
	return true;
}
static void setTimeout(PyObject* func, PyObject* args, unsigned time) {
	Sleep(time);
	if (PyCallable_Check(func))
		PyObject_CallOneArg(func, args);
}
static unsigned ModalFormRequestPacket(Player* p, string str) {
	unsigned fid = _formid++;
	if (PlayerCheck(p)) {
		VA pkt;//ModalFormRequestPacket
		createPacket(&pkt, 100);
		f(unsigned, pkt + 40) = fid;
		f(string, pkt + 48) = str;
		p->sendPacket(pkt);
	}
	return fid;
}
static bool TransferPacket(Player* p, string address, int port) {
	if (PlayerCheck(p)) {
		VA pkt;//TransferPacket
		createPacket(&pkt, 85);
		f(string, pkt + 40) = address;
		f(VA, pkt + 72) = port;
		p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool TextPacket(Player* p, int mode, const char* msg) {
	if (PlayerCheck(p)) {
		VA pkt;//TextPacket
		createPacket(&pkt, 9);
		f(char, pkt + 40) = mode;
		f(string, pkt + 48) = p->getNameTag();
		f(string, pkt + 80) = msg;
		p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool CommandRequestPacket(Player* p, string cmd) {
	if (PlayerCheck(p)) {
		VA pkt;//CommandRequestPacket
		createPacket(&pkt, 77);
		f(string, pkt + 40) = cmd;
		p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool BossEventPacket(Player* p, string name, float per, int eventtype) {
	if (PlayerCheck(p)) {
		VA pkt;//BossEventPacket
		createPacket(&pkt, 74);
		f(VA, pkt + 48) = f(VA, pkt + 56) = f(VA, p->getUniqueID());
		f(int, pkt + 64) = eventtype;//0显示,1更新,2隐藏,
		f(string, pkt + 72) = name;
		f(float, pkt + 104) = per;
		p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool setDisplayObjectivePacket(Player* p, string title, string name) {
	if (PlayerCheck(p)) {
		VA pkt;//setDisplayObjectivePacket
		createPacket(&pkt, 107);
		f(string, pkt + 40) = "sidebar";
		f(string, pkt + 72) = name;
		f(string, pkt + 104) = title;
		f(string, pkt + 136) = "dummy";
		f(char, pkt + 168) = 0;
		p->sendPacket(pkt);
	}
	return true;
}
static bool SetScorePacket(Player* p, char type, string name) {
	if (PlayerCheck(p)) {
		VA pkt;//SetScorePacket
		createPacket(&pkt, 108);
		f(char, pkt + 40) = type;//{set,remove}
		//f(vector<ScorePacketInfo>, pkt + 48) = { s };
		p->sendPacket(pkt);
	}
	return true;
}
#pragma endregion
#pragma region API函数
// 指令输出
CPYFUNC(api_logout) {
	const char* msg;
	if (PyArg_ParseTuple(args, "s:logout", &msg)) {
		SYMCALL<VA>("??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
			STD_COUT_HANDLE, msg, strlen(msg));
		return Py_True;
	}
	return Py_False;
}
// 执行指令
CPYFUNC(api_runcmd) {
	const char* cmd;
	if (PyArg_ParseTuple(args, "s:runcmd", &cmd)) {
		SYMCALL<bool>("??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			_cmdqueue, (string)cmd);
		return Py_True;
	}
	return Py_False;
}
// 延时
CPYFUNC(api_setTimeout) {
	unsigned time; PyObject* func; PyObject* arg;
	if (PyArg_ParseTuple(args, "OOI:setTimeout", &func, &arg, &time)) {
		thread(setTimeout, func, arg, time).detach();
	}
	return Py_None;
}
// 设置监听
CPYFUNC(api_setListener) {
	const char* m;
	PyObject* func;
	if (PyArg_ParseTuple(args, "sO:setListener", &m, &func) && PyCallable_Check(func)) {
		PyFuncs[m].push_back(func);
		return Py_True;
	}
	return Py_False;
}
// 共享数据
CPYFUNC(api_setShareData) {
	const char* index; PyObject* data;
	if (PyArg_ParseTuple(args, "sO:setShareData", &index, &data)) {
		ShareData[index] = data;
		return Py_True;
	}
	return Py_False;
}
CPYFUNC(api_getShareData) {
	const char* index;
	if (PyArg_ParseTuple(args, "s:getShareData", &index)) {
		return ShareData[index];
	}
	return _PyLong_Zero;
}
// 设置指令说明
CPYFUNC(api_setCommandDescription) {
	const char* cmd, * des;
	if (PyArg_ParseTuple(args, "ss:setCommandDescribe", &cmd, &des)) {
		Command[cmd] = des;
		return Py_True;
	}
	return Py_False;
}
CPYFUNC(api_getPlayerList) {
	PyObject* list = PyList_New(0);
	if (PyArg_ParseTuple(args, ":getPlayerList")) {
		for (auto& p : PlayerList) {
			PyList_Append(list, PyLong_FromUnsignedLongLong((VA)p.first));
		}
	}
	return list;
}
// 发送表单
CPYFUNC(api_sendCustomForm) {
	Player* p; const char* str;
	if (PyArg_ParseTuple(args, "Ks:sendCustomForm", &p, &str)) {
		return PyLong_FromLong(ModalFormRequestPacket(p, str));
	}
	return PyLong_FromLong(0);
}
CPYFUNC(api_sendSimpleForm) {
	Player* p; const char* title, * content, * buttons;
	if (PyArg_ParseTuple(args, "Ksss:sendSimpleForm", &p, &title, &content, &buttons)) {
		char str[0x400];
		sprintf_s(str, R"({"title":"%s","content":"%s","buttons":%s,"type":"form"})", title, content, buttons);
		return PyLong_FromLong(ModalFormRequestPacket(p, str));
	}
	return PyLong_FromLong(0);
}
CPYFUNC(api_sendModalForm) {
	Player* p; const char* title, * content, * button1, * button2;
	if (PyArg_ParseTuple(args, "Kssss:sendModalForm", &p, &title, &content, &button1, &button2)) {
		char str[0x400];
		sprintf_s(str, R"({"title":"%s","content":"%s","button1":"%s","button2":"%s","type":"modal"})", title, content, button1, button2);
		return PyLong_FromLong(ModalFormRequestPacket(p, str));
	}
	return PyLong_FromLong(0);
}
// 跨服传送
CPYFUNC(api_transferServer) {
	Player* p; const char* address; int port;
	if (PyArg_ParseTuple(args, "Ksi:transferServer", &p, &address, &port)) {
		if (TransferPacket(p, address, port))
			return Py_True;
	}
	return Py_False;
}
// 获取玩家手持
CPYFUNC(api_getSelectedItem) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:getSelectedItem", &p)) {
		if (PlayerCheck(p)) {
			ItemStack* item = p->getSelectedItem();
			short iaux = item->mAuxValue;
			short iid = item->getId();
			string iname = item->getName();
			return Py_BuildValue("{s:i,s:i,s:s,s:i}",
				"itemid", iid,
				"itemaux", iaux,
				"itemname", iname.c_str(),
				"itemcount", item->mCount
			);
		}
	}
	return PyDict_New();
}
// 获取玩家信息
CPYFUNC(api_getPlayerInfo) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:getPlayerInfo", &p)) {
		if (PlayerCheck(p)) {
			Vec3* pp = p->getPos();
			return Py_BuildValue("{s:s,s:[f,f,f],s:i,s:i,s:f,s:f,s:s}",
				"playername", p->getNameTag().c_str(),
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", p->getDimensionId(),
				"isstand", p->isStand(),
				"health", p->getHealth().first,
				"maxhealth", p->getHealth().second,
				"uuid", p->getUuid().c_str()
			);
		}
	}
	return PyDict_New();
}
CPYFUNC(api_getActorInfo) {
	Actor* a = nullptr;
	if (PyArg_ParseTuple(args, "K:getActorInfo", &a)) {
		if (a) {
			Vec3* pp = a->getPos();
			return Py_BuildValue("{s:s,s:[f,f,f],s:i,s:i,s:f,s:f}",
				"actorname", a->getNameTag().c_str(),
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", a->getDimensionId(),
				"isstand", a->isStand(),
				"health", a->getHealth().first,
				"maxhealth", a->getHealth().second
			);
		}
	}
	return PyDict_New();
}
// 玩家权限
CPYFUNC(api_getPlayerPerm) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:getPlayerPerm", &p)) {
		if (PlayerCheck(p)) {
			return PyLong_FromLong(p->getPermission());
		}
	}
	return PyLong_FromLong(-1);
}
CPYFUNC(api_setPlayerPerm) {
	Player* p; int lv;
	if (PyArg_ParseTuple(args, "Ki:setPlayerPerm", &p, &lv)) {
		if (PlayerCheck(p)) {
			p->setPermissionLevel(lv);
			return Py_True;
		}
	}
	return Py_False;
}
// 增加玩家等级
CPYFUNC(api_addLevel) {
	Player* p; int lv;
	if (PyArg_ParseTuple(args, "Ki::addLevel", &p, &lv)) {
		if (PlayerCheck(p)) {
			SYMCALL("?addLevels@Player@@UEAAXH@Z", p, lv);
			return Py_True;
		}
	}
	return Py_False;
}
// 设置玩家名字
CPYFUNC(api_setName) {
	Player* p; const char* name;
	if (PyArg_ParseTuple(args, "Ks:setName", &p, &name)) {
		if (PlayerCheck(p)) {
			p->setName(name);
			return Py_True;
		}
	}
	return Py_False;
}
// 玩家分数
CPYFUNC(api_getPlayerScore) {
	Player* p; const char* obj;
	if (PyArg_ParseTuple(args, "Ks:getPlayerScore", &p, &obj)) {
		if (PlayerCheck(p)) {
			Objective* testobj = _scoreboard->getObjective(obj);
			if (testobj) {
				auto id = _scoreboard->getScoreboardId(p);
				auto score = testobj->getPlayerScore(id);
				return PyLong_FromLong(score->getCount());
			}
			out("bad objective:" << obj);
		}
	}
	return PyLong_FromLong(0);
}
CPYFUNC(api_modifyPlayerScore) {
	Player* p; const char* obj; int count; char mode;
	if (PyArg_ParseTuple(args, "Ksii:modifyPlayerScore", &p, &obj, &count, &mode)) {
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
CPYFUNC(api_talkAs) {
	Player* p; const char* msg;
	if (PyArg_ParseTuple(args, "Ks:talkAs", &p, &msg)) {
		if (TextPacket(p, 1, msg))
			return Py_True;
	}
	return Py_False;
}
// 模拟玩家执行指令
CPYFUNC(api_runcmdAs) {
	Player* p; const char* cmd;
	if (PyArg_ParseTuple(args, "Ks:runcmdAs", &p, &cmd)) {
		if (CommandRequestPacket(p, cmd))
			return Py_True;
	}
	return Py_False;
}
// 传送玩家
CPYFUNC(api_teleport) {
	Player* p; float x, y, z; int did;
	if (PyArg_ParseTuple(args, "Kfffi:teleport", &p, &x, &y, &z, &did)) {
		p->teleport({ x,y,z }, did);
		return Py_True;
	}
	return Py_False;
}
// 原始输出
CPYFUNC(api_tellraw) {
	const char* msg;
	Player* p;
	if (PyArg_ParseTuple(args, "Ks:tellraw", &p, &msg)) {
		if (TextPacket(p, 0, msg))
			return Py_True;
	}
	return Py_False;
}
// 增加物品
CPYFUNC(api_addItem) {
	Player* p; int id, aux, count;
	if (PyArg_ParseTuple(args, "Kiii:addItem", &p, &id, &aux, &count)) {
		if (PlayerCheck(p)) {
			ItemStack item;
			item.getFromId(id, aux, count);
			p->addItem(&item);
		}
	}
	return Py_None;
}
// boss栏
CPYFUNC(api_setBossBar) {
	Player* p; const char* name; float per;
	if (PyArg_ParseTuple(args, "Ksf:", &p, &name, &per)) {
		if (BossEventPacket(p, name, per, 0))
			return Py_True;
	}
	return Py_False;
}
CPYFUNC(api_removeBossBar) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:removeBossBar", &p)) {
		if (BossEventPacket(p, "", 0.0, 2))
			return Py_True;
	}
	return Py_False;
}
// 方法列表
PyMethodDef m[] = {
   {"logout", api_logout, 1,0},//第3个值:1有参数4无参数,第4个值:描述
   {"runcmd", api_runcmd, 1,0},
   {"setTimeout", api_setTimeout, 1,0},
   {"setListener", api_setListener, 1,0},
   {"setShareData", api_setShareData, 1,0},
   {"getShareData", api_getShareData, 1,0},
   {"setCommandDescription", api_setCommandDescription, 1,0},
   {"getPlayerList", api_getPlayerList, 1,0},
   {"sendSimpleForm", api_sendSimpleForm, 1,0},
   {"sendModalForm", api_sendModalForm, 1,0},
   {"sendCustomForm", api_sendCustomForm, 1,0},
   {"transferServer", api_transferServer, 1,0},
   {"getSelectedItem", api_getSelectedItem, 1,0},
   {"getPlayerInfo", api_getPlayerInfo, 1,0},
   {"getActorInfo", api_getActorInfo, 1,0},
   {"getPlayerPerm", api_getPlayerPerm, 1,0},
   {"setPlayerPerm",api_setPlayerPerm, 1,0},
   {"addLevel", api_addLevel, 1,0},
   {"setName", api_setName, 1,0},
   {"getPlayerScore", api_getPlayerScore, 1,0},
   {"modifyPlayerScore", api_modifyPlayerScore, 1,0},
   {"talkAs", api_talkAs, 1,0},
   {"runcmdAs", api_runcmdAs, 1,0},
   {"teleport", api_teleport, 1,0},
   {"tellraw", api_tellraw, 1,0},
   {"addItem", api_addItem, 1,0},
   {"setBossBar", api_setBossBar, 1,0},
   {"removeBossBar", api_removeBossBar, 1,0},
   {0,0,0,0}
};
// 模块声明
PyModuleDef mc = { PyModuleDef_HEAD_INIT, "mc", 0, -1, m, 0, 0, 0, 0 };
extern "C" PyObject * mc_init() {
	return PyModule_Create(&mc);
}
#pragma endregion
#pragma region Hook
Hook(获取指令队列, VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	_cmdqueue = original(_this);
	return _cmdqueue;
}
Hook(获取地图初始化信息, VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	_level = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	return _level;
}
Hook(获取游戏初始化信息, VA, "??0GameSession@@QEAA@AEAVNetworkHandler@@V?$unique_ptr@VServerNetworkHandler@@U?$default_delete@VServerNetworkHandler@@@std@@@std@@AEAVLoopbackPacketSender@@V?$unique_ptr@VNetEventCallback@@U?$default_delete@VNetEventCallback@@@std@@@3@V?$unique_ptr@VLevel@@U?$default_delete@VLevel@@@std@@@3@E@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7) {
	_ServerNetworkHandle = f(VA, a3);
	return original(a1, a2, a3, a4, a5, a6, a7);
}
Hook(命令注册, void, "?setup@KillCommand@@SAXAEAVCommandRegistry@@@Z",//"?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",
	VA _this) {
	for (auto& cmd : Command) {
		SYMCALL("?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@UCommandFlag@@3@Z",
			_this, cmd.first.c_str(), cmd.second.c_str(), 0, 0, 0);
	}
	original(_this);
}
Hook(计分板, Scoreboard*, "??0ServerScoreboard@@QEAA@VCommandSoftEnumRegistry@@PEAVLevelStorage@@@Z",
	VA _this, VA a2, VA a3) {
	_scoreboard = (Scoreboard*)original(_this, a2, a3);
	return _scoreboard;
}
Hook(后台输出, VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
	VA handle, const char* str, VA size) {
	if (handle == STD_COUT_HANDLE) {
		bool res = callpy(u8"后台输出", PyUnicode_FromString(str));
		if (!res)return 0;
	}
	return original(handle, str, size);
}
Hook(后台输入, bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "pyreload") {
		Py_Finalize();
		PyFuncs.clear();
		init();
		return false;
	}
	bool res = callpy(u8"后台输入", PyUnicode_FromString((*cmd).c_str()));
	RET(_this, cmd);
}
Hook(玩家加入, VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	PlayerList[p] = true;
	callpy(u8"加载名字", PyLong_FromUnsignedLongLong((VA)p));
	return original(a1, p);
}
Hook(玩家离开游戏, void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	callpy(u8"离开游戏", PyLong_FromUnsignedLongLong((VA)p));
	PlayerList.erase(p);
	return original(_this, p, v3);
}
Hook(玩家操作物品, bool, "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
	VA _this, ItemStack* item, BlockPos* bp, unsigned __int8 a4, VA v5, Block* b) {
	Player* p = f(Player*, _this + 8);
	short iid = item->getId();
	short iaux = item->mAuxValue;
	string iname = item->getName();
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	bool res = callpy(u8"使用物品", Py_BuildValue("{s:K,s:i,s:i,s:s,s:s,s:i,s:[i,i,i]}",
		"player", p,
		"itemid", iid,
		"itemaux", iaux,
		"itemname", iname,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	));
	RET(_this, item, bp, a4, v5, b);
}
Hook(玩家放置方块, bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, Actor* p, bool _bool) {
	if (p && PlayerCheck(p)) {
		BlockLegacy* bl = b->getBlockLegacy();
		short bid = bl->getBlockItemID();
		string bn = bl->getBlockName();
		bool res = callpy(u8"放置方块", Py_BuildValue("{s:K,s:s,s:i,s:[i,i,i]}",
			"player", p,
			"blockname", bn.c_str(),
			"blockid", bid,
			"position", bp->x, bp->y, bp->z
		));
		RET(_this, b, bp, a4, p, _bool);
	}
	return original(_this, b, bp, a4, p, _bool);
}
Hook(玩家破坏方块, bool, "?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
	VA _this, BlockPos* bp) {
	Player* p = f(Player*, _this + 8);
	BlockSource* bs = f(BlockSource*, f(VA, _this + 8) + 840);
	Block* b = bs->getBlock(bp);
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	bool res = callpy(u8"破坏方块", Py_BuildValue("{s:K,s:s,s:i,s:[i,i,i]}",
		"player", p,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	));
	RET(_this, bp);
}
Hook(玩家开箱准备, bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
	VA _this, Player* p, BlockPos* bp) {
	bool res = callpy(u8"打开箱子", Py_BuildValue("{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	));
	RET(_this, p, bp);
}
Hook(玩家开桶准备, bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
	VA _this, Player* p, BlockPos* bp) {
	bool res = callpy(u8"打开木桶", Py_BuildValue("{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	));
	RET(_this, p, bp);
}
Hook(关闭箱子, void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	VA _this, Player* p) {
	auto bp = (BlockPos*)(_this - 204);
	callpy(u8"关闭箱子", Py_BuildValue("{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	));
	original(_this, p);
}
Hook(关闭木桶, void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	VA _this, Player* p) {
	auto bp = (BlockPos*)(_this - 204);
	callpy(u8"关闭木桶", Py_BuildValue("{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	));
	original(_this, p);
}
Hook(玩家放入取出, void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
	VA a1, VA slot) {
	VA v3 = f(VA, a1 + 208);// IDA LevelContainerModel::_getContainer line 15 25
	BlockSource* bs = f(BlockSource*, f(VA, v3 + 848) + 88);
	BlockPos* bp = (BlockPos*)(a1 + 216);
	BlockLegacy* bl = bs->getBlock(bp)->getBlockLegacy();
	short bid = bl->getBlockItemID();
	if (bid == 54 || bid == 130 || bid == 146 || bid == -203 || bid == 205 || bid == 218) {	// 非箱子、桶、潜影盒的情况不作处理
		VA v5 = (*(VA(**)(VA))(*(VA*)a1 + 160))(a1);
		if (v5) {
			ItemStack* i = (ItemStack*)(*(VA(**)(VA, VA))(*(VA*)v5 + 40))(v5, slot);
			callpy(u8"放入取出", Py_BuildValue("{s:K,s:s,s:i,s:[i,i,i],s:i,s:i,s:s,s:i,s:i}",
				"player", f(Player*, a1 + 208),
				"blockname", bl->getBlockName().c_str(),
				"blockid", bid,
				"position", bp->x, bp->y, bp->z,
				"itemid", i->getId(),
				"itemcount", i->mCount,
				"itemname", i->getName().c_str(),
				"itemaux", i->mAuxValue,
				"slot", slot
			));
		}
	}
	original(a1, slot);
}
Hook(玩家攻击, bool, "?attack@Player@@UEAA_NAEAVActor@@@Z",
	Player* p, Actor* a) {
	bool res = callpy(u8"玩家攻击", Py_BuildValue("{s:K,s:K}",
		"player", p,
		"actor", a
	));
	RET(p, a);
}
Hook(玩家切换维度, bool, "?_playerChangeDimension@Level@@AEAA_NPEAVPlayer@@AEAVChangeDimensionRequest@@@Z",
	VA _this, Player* p, VA req) {
	bool result = original(_this, p, req);
	if (result) {
		callpy(u8"切换纬度", PyLong_FromUnsignedLongLong((VA)p));
	}
	return result;
}
Hook(生物死亡, void, "?die@Mob@@UEAAXAEBVActorDamageSource@@@Z",
	Mob* _this, VA dmsg) {
	char v72;
	Actor* sa = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		f(VA, _this + 856), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	bool res = callpy(u8"生物死亡", Py_BuildValue("{s:i,s:K,s:K}",
		"dmcase", f(unsigned, dmsg + 8),
		"actor1", _this,
		"actor2", sa//可能为0
	));
	if (res) original(_this, dmsg);
}
Hook(生物受伤, bool, "?_hurt@Mob@@MEAA_NAEBVActorDamageSource@@H_N1@Z",
	Mob* _this, VA dmsg, int a3, bool a4, bool a5) {
	char v72;
	Actor* sa = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		f(VA, _this + 856), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	bool res = callpy(u8"生物受伤", Py_BuildValue("{s:i,s:K,s:K,s:i}",
		"dmcase", f(unsigned, dmsg + 8),
		"actor1", _this,
		"actor2", sa,//可能为0
		"damage", a3
	));
	RET(_this, dmsg, a3, a4, a5);
}
Hook(玩家重生, void, "?respawn@Player@@UEAAXXZ",
	Player* p) {
	callpy(u8"玩家重生", PyLong_FromUnsignedLongLong((VA)p));
	original(p);
}
Hook(聊天消息, void, "?fireEventPlayerMessage@MinecraftEventing@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@000@Z",
	VA _this, string& sender, string& target, string& msg, string& style) {
	callpy(u8"聊天消息", Py_BuildValue("{s:s,s:s,s:s,s:s}",
		"sender", sender.c_str(),
		"target", target.c_str(),
		"msg", msg.c_str(),
		"style", style.c_str()
	));
	original(_this, sender, target, msg, style);
}
Hook(玩家输入文本, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVTextPacket@@@Z",
	VA _this, VA id, /*(TextPacket*)*/VA tp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, f(char, tp + 16));
	if (p) {
		string msg = f(string, tp + 80);
		bool res = callpy(u8"输入文本", Py_BuildValue("{s:K,s:s}",
			"player", p,
			"msg", msg.c_str()
		));
		if (res)original(_this, id, tp);
	}
}
Hook(玩家输入指令, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z",
	VA _this, VA id, /*(CommandRequestPacket*)*/VA crp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, f(char, crp + 16));
	if (p) {
		string cmd = f(string, crp + 40);
		bool res = callpy(u8"输入指令", Py_BuildValue("{s:K,s:s}",
			"player", p,
			"cmd", cmd.c_str()
		));
		if (res)original(_this, id, crp);
	}
}
Hook(玩家选择表单, void, "?handle@?$PacketHandlerDispatcherInstance@VModalFormResponsePacket@@$0A@@@UEBAXAEBVNetworkIdentifier@@AEAVNetEventCallback@@AEAV?$shared_ptr@VPacket@@@std@@@Z",
	VA _this, VA id, VA handle,/*(ModalFormResponsePacket**)*/VA* fp) {
	VA fmp = *fp;
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		handle, id, f(char, fmp + 16));
	if (PlayerCheck(p)) {
		unsigned fid = f(unsigned, fmp + 40);
		string data = f(string, fmp + 48);
		if (data != "null\n") {
			VA len = data.length() - 1;
			if (data[len] == '\n')data[len] = '\0';
			callpy(u8"选择表单", Py_BuildValue("{s:K,s:s,s:i}",
				"player", p,
				"selected", data.c_str(),
				"formid", fid
			));
		}
	}
	original(_this, id, handle, fp);
}
Hook(更新命令方块, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandBlockUpdatePacket@@@Z",
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
		bool res = callpy(u8"命令方块更新", Py_BuildValue("{s:K,s:i,s:i,s:i,s:s,s:s,s:s,s:i,s:[i,i,i]}",
			"player", p,
			"mode", mode,
			"condition", condition,
			"redstone", redstone,
			"cmd", cmd.c_str(),
			"output", output.c_str(),
			"rawname", rawname.c_str(),
			"delay", delay,
			"position", bp.x, bp.y, bp.z
		));
		if (res)original(_this, id, cbp);
	}
}
Hook(爆炸监听, bool, "?explode@Level@@QEAAXAEAVBlockSource@@PEAVActor@@AEBVVec3@@M_N3M3@Z",
	Level* _this, BlockSource* bs, Actor* a3, const Vec3 pos, float a5, bool a6, bool a7, float a8, bool a9) {
	bool res = callpy(u8"爆炸监听", Py_BuildValue("{s:K,s:[f,f,f],s:i,s:i}",
		"actor", a3,
		"position", pos.x, pos.y, pos.z,
		"dimensionid", bs->getDimensionId(),
		"power", a5
	));
	RET(_this, bs, a3, pos, a5, a6, a7, a8, a9);
}
Hook(命令方块执行, bool, "?performCommand@CommandBlockActor@@QEAA_NAEAVBlockSource@@@Z",
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
	bool res = callpy(u8"命令方块执行", Py_BuildValue("{s:s,s:s,s:[i,i,i],s:i,s:i}",
		"cmd", cmd.c_str(),
		"rawname", rawname.c_str(),
		"position", bp.x, bp.y, bp.z,
		"mode", mode,
		"condition", condition
	));
	RET(_this, a2);
}
Hook(玩家穿戴, void, "?setArmor@Player@@UEAAXW4ArmorSlot@@AEBVItemStack@@@Z",
	Player* p, unsigned slot, ItemStack* i) {
	if (!i->getId())return;
	bool res = callpy(u8"穿戴盔甲", Py_BuildValue("{s:K,s:i,s:i,s:s,s:i,s:i}",
		"player", p,
		"itemid", i->getId(),
		"itemcount", i->mCount,
		"itemname", i->getName().c_str(),
		"itemaux", i->mAuxValue,
		"slot", slot
	));
	if (res)original(p, slot, i);
}
#pragma endregion
void init() {
	Py_Initialize();
	//pts["main"] = (VA)PyThreadState_Get();
	_finddata_t Info;//用于查找的句柄
	long long handle = _findfirst("./py/*.py", &Info);
	if (handle != -1) {
		do {
			string name = Info.name;
			//pts[name] = (VA)Py_NewInterpreter();
			printf("[BDSpyrunner] reading %s.\n", Info.name);
			PyRun_SimpleFileExFlags(fopen(("./py/" + name).c_str(), "rb"), Info.name, 1, 0);
		} while (!_findnext(handle, &Info));
		_findclose(handle);
	}
	else puts("[BDSpyrunner] can't find py directory.");
}
int DllMain(VA, int dwReason, VA) {
	if (dwReason == 1) {
		puts("[BDSpyrunner] loading...");
		PyPreConfig cfg;
		PyPreConfig_InitIsolatedConfig(&cfg);
		Py_PreInitialize(&cfg);
		PyImport_AppendInittab("mc", mc_init); //增加一个模块
		init();
		puts("[BDSpyrunner] v0.0.11 for BDS1.16.201 loaded.");
	} return 1;
}