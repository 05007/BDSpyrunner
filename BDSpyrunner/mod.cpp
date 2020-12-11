#include "pch.h"
#pragma warning(disable:4996)
#pragma region �궨��
//�������к���
#define CallAll(type,...)							\
bool res = true;									\
for (PyObject* fn :PyFuncs[type]) {					\
	if(PyObject_CallFunction(fn,__VA_ARGS__)==Py_False)res = false;\
}PyErr_Print()
#define out(...) cout <<__VA_ARGS__<< endl
//THook�����ж�
#define RET(...) if (!res) return 0;return original(__VA_ARGS__)
#define cpkt(pkt,p)	SYMCALL<VA>("?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",&pkt, p);
#define PlayerCheck(ptr)  PlayerList[(Player*)ptr]
#pragma endregion
#pragma region ȫ�ֱ���
static VA _cmdqueue, _level, _ServerNetworkHandle;
static const VA STD_COUT_HANDLE = f(VA, GetServerSymbol("__imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A"));
static Scoreboard* _scoreboard;//����Ʒְ�����
static unsigned _formid = 1;//��ID
static unordered_map<string, vector<PyObject*>> PyFuncs;//Py����
static unordered_map<Player*, bool> PlayerList;//����б�
static unordered_map<string, string> Command;//ע������
static Player* _p;//��ʱ
#pragma endregion
#pragma region ��������
void init();
static void setTimeout(int time, PyObject* func) {
	Sleep(time);
	if (PyCallable_Check(func))
		PyObject_CallFunction(func, 0);
}
static unsigned ModalFormRequestPacket(Player* p, string str) {
	unsigned fid = _formid++;
	if (PlayerCheck(_p)) {
		VA pkt;//ModalFormRequestPacket
		cpkt(pkt, 100);
		f(unsigned, pkt + 40) = fid;
		f(string, pkt + 48) = str;
		_p->sendPacket(pkt);
	}
	return fid;
}
static bool TransferPacket(Player* p, string address, int port) {
	if (PlayerCheck(_p)) {
		VA pkt;//TransferPacket
		cpkt(pkt, 85);
		f(string, pkt + 40) = address;
		f(VA, pkt + 72) = port;
		_p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool TextPacket(Player* p, int mode, string name) {
	if (PlayerCheck(p)) {
		VA pkt;//TextPacket
		cpkt(pkt, 9);
		f(char, pkt + 40) = mode;
		f(string, pkt + 48) = p->getNameTag();
		f(string, pkt + 80) = name;
		p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool CommandRequestPacket(Player* p, string cmd) {
	if (PlayerCheck(p)) {
		VA pkt;//CommandRequestPacket
		cpkt(pkt, 77);
		f(string, pkt + 40) = cmd;
		p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool BossEventPacket(Player* p, string name, float per, int eventtype) {
	if (PlayerCheck(p)) {
		VA pkt;//BossEventPacket
		cpkt(pkt, 74);
		f(VA, pkt + 48) = f(VA, pkt + 56) = f(VA, p->getUniqueID());
		f(int, pkt + 64) = eventtype;//0��ʾ,1����,2����,
		f(string, pkt + 72) = name;
		f(float, pkt + 104) = per;
		p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool setSidebar(Player* p, string title, string name) {
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
#pragma region api����
// ָ�����
static PyObject* api_logout(PyObject* self, PyObject* args) {
	const char* msg;
	if (PyArg_ParseTuple(args, "s:logout", &msg)) {
		SYMCALL<VA>("??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
			STD_COUT_HANDLE, msg, strlen(msg));
		return Py_True;
	}
	return Py_False;
}
// ִ��ָ��
static PyObject* api_runcmd(PyObject* self, PyObject* args) {
	const char* cmd;
	if (PyArg_ParseTuple(args, "s:runcmd", &cmd)) {
		SYMCALL<bool>("??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			_cmdqueue, (string)cmd);
		return Py_True;
	}
	return Py_False;
}
// ��ʱ
static PyObject* api_setTimeout(PyObject* self, PyObject* args) {
	int time; PyObject* func;
	if (PyArg_ParseTuple(args, "Oi:setTimeout", &func, &time)) {
		thread(setTimeout, time, func).detach();
	}
	return Py_None;
}
// ���ü���
static PyObject* api_setListener(PyObject* self, PyObject* args) {
	const char* m;
	PyObject* func;
	if (PyArg_ParseTuple(args, "sO:setListener", &m, &func) && PyCallable_Check(func)) {
		PyFuncs[m].push_back(func);
		return Py_True;
	}
	return Py_False;
}
// ����ָ��˵��
static PyObject* api_setCommandDescription(PyObject* self, PyObject* args) {
	const char* cmd, * des;
	if (PyArg_ParseTuple(args, "ss:setCommandDescribe", &cmd, &des)) {
		Command[cmd] = des;
		return Py_True;
	}
	return Py_False;
}
// ���ͱ�
static PyObject* api_sendCustomForm(PyObject* self, PyObject* args) {
	const char* str;
	if (PyArg_ParseTuple(args, "Ks:sendCustomForm", &_p, &str)) {
		return PyLong_FromLong(ModalFormRequestPacket(_p, str));
	}
	return PyLong_FromLong(0);
}
static PyObject* api_sendSimpleForm(PyObject* self, PyObject* args) {
	const char* title, * content, * buttons;
	if (PyArg_ParseTuple(args, "Ksss:sendSimpleForm", &_p, &title, &content, &buttons)) {
		char str[512];
		sprintf(str, R"({"title":"%s","content":"%s","buttons":%s,"type":"form"})", title, content, buttons);
		return PyLong_FromLong(ModalFormRequestPacket(_p, str));
	}
	return PyLong_FromLong(0);
}
static PyObject* api_sendModalForm(PyObject* self, PyObject* args) {
	const char* title, * content, * button1, * button2;
	if (PyArg_ParseTuple(args, "Kssss:sendModalForm", &_p, &title, &content, &button1, &button2)) {
		char str[512];
		sprintf(str, R"({"title":"%s","content":"%s","button1":"%s","button2":"%s","type":"modal"})", title, content, button1, button2);
		return PyLong_FromLong(ModalFormRequestPacket(_p, str));
	}
	return PyLong_FromLong(0);
}
// �������
static PyObject* api_transferServer(PyObject* self, PyObject* args) {
	const char* address; int port;
	if (PyArg_ParseTuple(args, "Ksi:transferServer", &_p, &address, &port)) {
		if (TransferPacket(_p, address, port))
			return Py_True;
	}
	return Py_False;
}
// ��ȡ����ֳ�
static PyObject* api_getSelectedItem(PyObject* self, PyObject* args) {
	if (PyArg_ParseTuple(args, "K:getSelectedItem", &_p)) {
		if (PlayerCheck(_p)) {
			ItemStack* item = _p->getSelectedItem();
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
	return PyDict_New();
}
// ��ȡ�����Ϣ
static PyObject* api_getPlayerInfo(PyObject* self, PyObject* args) {
	if (PyArg_ParseTuple(args, "K:getPlayerInfo", &_p)) {
		if (PlayerCheck(_p)) {
			Vec3* pp = _p->getPos();
			return Py_BuildValue("{s:s,s:[f,f,f],s:i,s:i,s:f,s:f,s:s}",
				"playername", _p->getNameTag().c_str(),
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", _p->getDimensionId(),
				"isstand", _p->isStand(),
				"health", _p->getHealth().first,
				"maxhealth", _p->getHealth().second,
				"uuid", _p->getUuid().c_str()
			);
		}
	}
	return PyDict_New();
}
static PyObject* api_getActorInfo(PyObject* self, PyObject* args) {
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
// ��ȡ���Ȩ��
static PyObject* api_getPlayerPerm(PyObject* self, PyObject* args) {
	if (PyArg_ParseTuple(args, "K:getPlayerPerm", &_p)) {
		if (PlayerCheck(_p)) {
			return PyLong_FromLong(_p->getPermission());
		}
	}
	return PyLong_FromLong(-1);
}
// �������Ȩ��
static PyObject* api_setPlayerPerm(PyObject* self, PyObject* args) {
	int lv;
	if (PyArg_ParseTuple(args, "Ki:setPlayerPerm", &_p, &lv)) {
		if (PlayerCheck(_p)) {
			_p->setPermissionLevel(lv);
			return Py_True;
		}
	}
	return Py_False;
}
// ������ҵȼ�
static PyObject* api_addLevel(PyObject* self, PyObject* args) {
	int lv;
	if (PyArg_ParseTuple(args, "Ki::addLevel", &_p, &lv)) {
		if (PlayerCheck(_p)) {
			SYMCALL<void>("?addLevels@Player@@UEAAXH@Z", _p, lv);
			return Py_True;
		}
	}
	return Py_False;
}
// �����������
static PyObject* api_setName(PyObject* self, PyObject* args) {
	const char* name;
	if (PyArg_ParseTuple(args, "Ks:setName", &_p, &name)) {
		if (PlayerCheck(_p)) {
			_p->setName(name);
			return Py_True;
		}
	}
	return Py_False;
}
// ��ҷ���
static PyObject* api_getPlayerScore(PyObject* self, PyObject* args) {
	const char* obj;
	if (PyArg_ParseTuple(args, "Ks:getPlayerScore", &_p, &obj)) {
		if (PlayerCheck(_p)) {
			Objective* testobj = _scoreboard->getObjective(obj);
			if (testobj) {
				auto id = _scoreboard->getScoreboardId(_p);
				auto score = testobj->getPlayerScore(id);
				return PyLong_FromLong(score->getCount());
			}
			out("bad objective:" << obj);
		}
	}
	return PyLong_FromLong(0);
}
static PyObject* api_modifyPlayerScore(PyObject* self, PyObject* args) {
	const char* obj; int count; char mode;
	if (PyArg_ParseTuple(args, "Ksii:modifyPlayerScore", &_p, &obj, &count, &mode)) {
		if (PlayerCheck(_p)) {
			Objective* testobj = _scoreboard->getObjective(obj);
			if (testobj) {
				//mode:{set,add,remove}
				_scoreboard->modifyPlayerScore((ScoreboardId*)_scoreboard->getScoreboardId(_p), testobj, count, mode);
				return Py_True;
			}
			else out("bad objective:" << obj);
		}
	}
	return Py_False;
}
// ģ����ҷ����ı�
static PyObject* api_talkAs(PyObject* self, PyObject* args) {
	const char* msg;
	if (PyArg_ParseTuple(args, "Ks:talkAs", &_p, &msg)) {
		if (TextPacket(_p, 1, msg))
			return Py_True;
	}
	return Py_False;
}
// ģ�����ִ��ָ��
static PyObject* api_runcmdAs(PyObject* self, PyObject* args) {
	const char* cmd;
	if (PyArg_ParseTuple(args, "Ks:runcmdAs", &_p, &cmd)) {
		if (CommandRequestPacket(_p, cmd))
			return Py_True;
	}
	return Py_False;
}
// �������
static PyObject* api_teleport(PyObject* self, PyObject* args) {
	float x, y, z; int did;
	if (PyArg_ParseTuple(args, "Kfffi:teleport", &_p, &x, &y, &z, &did)) {
		_p->teleport({ x,y,z }, did);
		return Py_True;
	}
	return Py_False;
}
// ԭʼ���
static PyObject* api_tellraw(PyObject* self, PyObject* args) {
	const char* msg;
	if (PyArg_ParseTuple(args, "Ks:tellraw", &_p, &msg)) {
		if (TextPacket(_p, 0, msg))
			return Py_True;
	}
	return Py_False;
}
// ������Ʒ
static PyObject* api_addItem(PyObject* self, PyObject* args) {
	int id, aux, count;
	if (PyArg_ParseTuple(args, "Kiii:addItem", &_p, &id, &aux, &count)) {
		if (PlayerCheck(_p)) {
			ItemStack item;
			item.getFromId(id, aux, count);
			_p->addItem(&item);
		}
	}
	return Py_None;
}
// boss��
static PyObject* api_setBossBar(PyObject* self, PyObject* args) {
	const char* name; float per;
	if (PyArg_ParseTuple(args, "Ksf:", &_p, &name, &per)) {
		if (BossEventPacket(_p, name, per, 0))
			return Py_True;
	}
	return Py_False;
}
static PyObject* api_removeBossBar(PyObject* self, PyObject* args) {
	if (PyArg_ParseTuple(args, "K:removeBossBar", &_p)) {
		if (BossEventPacket(_p, "", 0.0, 2))
			return Py_True;
	}
	return Py_False;
}
// �����б�
PyMethodDef m[] = {
   {"logout", api_logout, 1,0},//��3��ֵ:1�в���4�޲���,��4��ֵ:����
   {"runcmd", api_runcmd, 1,0},
   {"setTimeout", api_setTimeout, 1,0},
   {"setListener", api_setListener, 1,0},
   {"setCommandDescription", api_setCommandDescription, 1,0},
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
// ģ������
PyModuleDef mc = { PyModuleDef_HEAD_INIT, "mc", 0, -1,m,0,0,0,0 };
#pragma endregion
#pragma region Hook
THook(��ȡָ�����, VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	_cmdqueue = original(_this);
	return _cmdqueue;
}
THook(��ȡ��ͼ��ʼ����Ϣ, VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	_level = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	return _level;
}
THook(��ȡ��Ϸ��ʼ����Ϣ, VA, "??0GameSession@@QEAA@AEAVNetworkHandler@@V?$unique_ptr@VServerNetworkHandler@@U?$default_delete@VServerNetworkHandler@@@std@@@std@@AEAVLoopbackPacketSender@@V?$unique_ptr@VNetEventCallback@@U?$default_delete@VNetEventCallback@@@std@@@3@V?$unique_ptr@VLevel@@U?$default_delete@VLevel@@@std@@@3@E@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7) {
	_ServerNetworkHandle = f(VA, a3);
	return original(a1, a2, a3, a4, a5, a6, a7);
}
THook(����ע��, void, "?setup@KillCommand@@SAXAEAVCommandRegistry@@@Z",//"?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",
	VA _this) {
	for (auto& cmd : Command) {
		SYMCALL<void>("?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@UCommandFlag@@3@Z",
			_this, cmd.first.c_str(), cmd.second.c_str(), 0, 0, 0);
	}
	original(_this);
}
THook(�Ʒְ�, Scoreboard*, "??0ServerScoreboard@@QEAA@VCommandSoftEnumRegistry@@PEAVLevelStorage@@@Z",
	VA _this, VA a2, VA a3) {
	_scoreboard = (Scoreboard*)original(_this, a2, a3);
	return _scoreboard;
}
THook(��̨���, VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
	VA handle, const char* str, VA size) {
	if (handle == STD_COUT_HANDLE) {
		CallAll(u8"��̨���", "s", str);
		if (!res)return 0;
	}
	return original(handle, str, size);
}
THook(��̨����, bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	if (*cmd == "pyreload") {
		Py_Finalize();
		PyFuncs.clear();
		init();
		return false;
	}
	CallAll(u8"��̨����", "s", (*cmd).c_str());
	RET(_this, cmd);
}
THook(��Ҽ���, VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	PlayerList[p] = true;
	CallAll(u8"��������", "K", p);
	return original(a1, p);
}
THook(����뿪��Ϸ, void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	PlayerList.erase(p);
	CallAll(u8"�뿪��Ϸ", "K", p);
	return original(_this, p, v3);
}
THook(��Ҽ�����Ʒ, bool, "?take@Player@@QEAA_NAEAVActor@@HH@Z",
	Player* p, Actor* a, VA a3, VA a4) {
	CallAll(u8"������Ʒ", "");
	return original(p, a, a3, a4);
}
THook(��Ҳ�����Ʒ, bool, "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
	VA _this, ItemStack* item, BlockPos* bp, unsigned __int8 a4, VA v5, Block* b) {
	Player* p = f(Player*, _this + 8);
	short iid = item->getId();
	short iaux = item->getAuxValue();
	string iname = item->getName();
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	CallAll(u8"ʹ����Ʒ", "{s:K,s:i,s:i,s:s,s:s,s:i,s:[i,i,i]}",
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
THook(��ҷ��÷���, bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, Actor* p, bool _bool) {
	if (p && PlayerCheck(p)) {
		BlockLegacy* bl = b->getBlockLegacy();
		short bid = bl->getBlockItemID();
		string bn = bl->getBlockName();
		CallAll(u8"���÷���", "{s:K,s:s,s:i,s:[i,i,i]}",
			"player", p,
			"blockname", bn.c_str(),
			"blockid", bid,
			"position", bp->x, bp->y, bp->z
		);
		RET(_this, b, bp, a4, p, _bool);
	}
	return original(_this, b, bp, a4, p, _bool);
}
THook(����ƻ�����, bool, "?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
	VA _this, BlockPos* bp) {
	Player* p = f(Player*, _this + 8);
	BlockSource* bs = f(BlockSource*, f(VA, _this + 8) + 840);
	Block* b = bs->getBlock(bp);
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	CallAll(u8"�ƻ�����", "{s:K,s:s,s:i,s:[i,i,i]}",
		"player", p,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, bp);
}
THook(��ҿ���׼��, bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
	VA _this, Player* p, BlockPos* bp) {
	CallAll(u8"������", "{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, p, bp);
}
THook(��ҿ�Ͱ׼��, bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
	VA _this, Player* p, BlockPos* bp) {
	CallAll(u8"��ľͰ", "{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	);
	RET(_this, p, bp);
}
THook(�ر�����, void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	VA _this, Player* p) {
	auto bp = (BlockPos*)(_this - 204);
	CallAll(u8"�ر�����", "{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	);
	original(_this, p);
}
THook(�ر�ľͰ, void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	VA _this, Player* p) {
	auto bp = (BlockPos*)(_this - 204);
	CallAll(u8"�ر�ľͰ", "{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	);
	original(_this, p);
}
THook(��ҷ���ȡ��, void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
	VA a1, VA slot) {
	VA v3 = f(VA, a1 + 208);// IDA LevelContainerModel::_getContainer line 15 25
	BlockSource* bs = f(BlockSource*, f(VA, v3 + 848) + 88);
	BlockPos* bp = (BlockPos*)(a1 + 216);
	BlockLegacy* bl = bs->getBlock(bp)->getBlockLegacy();
	short bid = bl->getBlockItemID();
	if (bid == 54 || bid == 130 || bid == 146 || bid == -203 || bid == 205 || bid == 218) {	// �����ӡ�Ͱ��ǱӰ�е������������
		VA v5 = (*(VA(**)(VA))(*(VA*)a1 + 160))(a1);
		if (v5) {
			ItemStack* i = (ItemStack*)(*(VA(**)(VA, VA))(*(VA*)v5 + 40))(v5, slot);
			CallAll(u8"����ȡ��", "{s:K,s:s,s:i,s:[i,i,i],s:i,s:i,s:s,s:i,s:i}",
				"player", f(Player*, a1 + 208),
				"blockname", bl->getBlockName().c_str(),
				"blockid", bid,
				"position", bp->x, bp->y, bp->z,
				"itemid", i->getId(),
				"itemcount", i->getStackSize(),
				"itemname", i->getName().c_str(),
				"itemaux", i->getAuxValue(),
				"slot", slot
			);
		}
	}
	original(a1, slot);
}
THook(��ҹ���, bool, "?attack@Player@@UEAA_NAEAVActor@@@Z",
	Player* p, Actor* a) {
	CallAll(u8"��ҹ���", "{s:K,s:K}",
		"player", p,
		"actor", a
	);
	RET(p, a);
}
THook(����л�ά��, bool, "?_playerChangeDimension@Level@@AEAA_NPEAVPlayer@@AEAVChangeDimensionRequest@@@Z",
	VA _this, Player* p, VA req) {
	bool result = original(_this, p, req);
	if (result) {
		CallAll(u8"�л�γ��", "K", p);
	}
	return result;
}
THook(��������, void, "?die@Mob@@UEAAXAEBVActorDamageSource@@@Z",
	Mob* _this, VA dmsg) {
	char v72;
	Actor* sa = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		f(VA, _this + 856), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	CallAll(u8"��������", "{s:i,s:K,s:K}",
		"dmcase", f(unsigned, dmsg + 8),
		"actor1", _this,
		"actor2", sa//����Ϊ0
	);
	if (res) original(_this, dmsg);
}
THook(��������, bool, "?_hurt@Mob@@MEAA_NAEBVActorDamageSource@@H_N1@Z",
	Mob* _this, VA dmsg, int a3, bool a4, bool a5) {
	char v72;
	Actor* sa = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		f(VA, _this + 856), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	CallAll(u8"��������", "{s:i,s:K,s:K,s:i}",
		"dmcase", f(unsigned, dmsg + 8),
		"actor1", _this,
		"actor2", sa,//����Ϊ0
		"damage", a3
	);
	RET(_this, dmsg, a3, a4, a5);
}
THook(�������, void, "?respawn@Player@@UEAAXXZ",
	Player* p) {
	CallAll(u8"�������", "K", p);
	original(p);
}
THook(������Ϣ, void, "?fireEventPlayerMessage@MinecraftEventing@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@000@Z",
	VA _this, string& sender, string& target, string& msg, string& style) {
	CallAll(u8"������Ϣ", "{s:s,s:s,s:s,s:s}",
		"sender", sender.c_str(),
		"target", target.c_str(),
		"msg", msg.c_str(),
		"style", style.c_str()
	);
	original(_this, sender, target, msg, style);
}
THook(��������ı�, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVTextPacket@@@Z",
	VA _this, VA id, /*(TextPacket*)*/VA tp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)tp + 16));
	if (p) {
		string msg = *(string*)(tp + 80);
		CallAll(u8"�����ı�", "{s:K,s:s}",
			"player", p,
			"msg", msg.c_str()
		);
		if (res)original(_this, id, tp);
	}
}
THook(�������ָ��, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z",
	VA _this, VA id, /*(CommandRequestPacket*)*/VA crp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, *((char*)crp + 16));
	if (p) {
		string cmd = f(string, crp + 40);
		CallAll(u8"����ָ��", "{s:K,s:s}",
			"player", p,
			"cmd", cmd.c_str()
		);
		if (res)original(_this, id, crp);
	}
}
THook(���ѡ���, void, "?handle@?$PacketHandlerDispatcherInstance@VModalFormResponsePacket@@$0A@@@UEBAXAEBVNetworkIdentifier@@AEAVNetEventCallback@@AEAV?$shared_ptr@VPacket@@@std@@@Z",
	VA _this, VA id, VA handle,/*(ModalFormResponsePacket**)*/VA* fp) {
	VA fmp = *fp;
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		handle, id, f(char, fmp + 16));
	if (PlayerCheck(p)) {
		unsigned fid = f(unsigned, fmp + 40);
		string x = f(string, fmp + 48);
		if (x[x.length() - 1] == '\n')x[x.length() - 1] = '\0';
		CallAll(u8"ѡ���", "{s:K,s:s,s:i}",
			"player", p,
			"selected", x.c_str(),
			"formid", fid
		);
	}
	original(_this, id, handle, fp);
}
THook(���������, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandBlockUpdatePacket@@@Z",
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
		CallAll(u8"��������", "{s:K,s:i,s:i,s:i,s:s,s:s,s:s,s:i,s:[i,i,i]}",
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
THook(��ը����, bool, "?explode@Level@@QEAAXAEAVBlockSource@@PEAVActor@@AEBVVec3@@M_N3M3@Z",
	Level* _this, BlockSource* bs, Actor* a3, const Vec3 pos, float a5, bool a6, bool a7, float a8, bool a9) {
	if (a3) {
		CallAll(u8"��ը����", "{s:[f,f,f],s:s,s:i,s:i,s:i}",
			"position", pos.x, pos.y, pos.z,
			"entity", a3->getEntityTypeName().c_str(),
			"entityid", a3->getEntityTypeId(),
			"dimensionid", a3->getDimensionId(),
			"power", a5
		);
		RET(_this, bs, a3, pos, a5, a6, a7, a8, a9);
	}
	CallAll(u8"��ը����", "{s:[f,f,f],s:i,s:i}",
		"XYZ", pos.x, pos.y, pos.z,
		"dimensionid", bs->getDimensionId(),
		"power", a5
	);
	RET(_this, bs, a3, pos, a5, a6, a7, a8, a9);
}
THook(�����ִ��, bool, "?performCommand@CommandBlockActor@@QEAA_NAEAVBlockSource@@@Z",
	VA _this, BlockSource* a2) {
	//����:0,�ظ�:1,��:2
	int mode = SYMCALL<int>("?getMode@CommandBlockActor@@QEBA?AW4CommandBlockMode@@AEAVBlockSource@@@Z",
		_this, a2);
	//������:0,������:1
	bool condition = SYMCALL<bool>("?getConditionalMode@CommandBlockActor@@QEBA_NAEAVBlockSource@@@Z",
		_this, a2);
	string cmd = f(string, _this + 264);
	string rawname = f(string, _this + 296);
	BlockPos bp = f(BlockPos, _this + 44);
	CallAll(u8"�����ִ��", "{s:s,s:s,s:[i,i,i],s:i,s:i}",
		"cmd", cmd.c_str(),
		"rawname", rawname.c_str(),
		"position", bp.x, bp.y, bp.z,
		"mode", mode,
		"condition", condition
	);
	RET(_this, a2);
}
#pragma endregion
void init() {
	puts("BDSpyrunner v0.0.8 Loading...");
	PyPreConfig cfg;
	PyPreConfig_InitPythonConfig(&cfg);
	Py_PreInitialize(&cfg);
	PyObject* (*initfunc)(void) = [] {return PyModule_Create2(&mc, 1013); };
	PyImport_AppendInittab("mc", initfunc); //����һ��ģ��
	Py_Initialize();
	_finddata_t fileinfo;//���ڲ��ҵľ��
	long long handle = _findfirst("./py/*.py", &fileinfo);
	if (handle != -1) {
		do {
			Py_NewInterpreter();
			printf("Reading Python file : %s\n", fileinfo.name);
			PyRun_SimpleFileExFlags(fopen(("./py/" + (string)fileinfo.name).c_str(), "rb"), fileinfo.name, 1, 0);
		} while (!_findnext(handle, &fileinfo));
		_findclose(handle);
	}
	else puts(u8"û���ҵ�Ŀ¼");
}
int __stdcall DllMain(HINSTANCE__* hModule, unsigned long res, void* lpReserved) {
	if (res == 1)
		init();
	return 1;
}