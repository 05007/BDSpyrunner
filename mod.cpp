#include "pch.h"
#include "BDS.hpp"
#pragma warning(disable:4996)
#pragma region �궨��
#define api_method(name) {#name, api_##name, 1, 0}
#define api_function(name) static PyObject* api_##name(PyObject*, PyObject*args)
#define check_ret(...) if (!res) return 0;return original(__VA_ARGS__)
#define PlayerCheck(ptr)  PlayerList[(Player*)ptr]
#define createPacket(pkt, i) SYMCALL("?createPacket@MinecraftPackets@@SA?AV?$shared_ptr@VPacket@@@std@@W4MinecraftPacketIds@@@Z",pkt,i);
#pragma endregion
#pragma region ȫ�ֱ���
static VA _cmdqueue, _level, _ServerNetworkHandle;
static const VA STD_COUT_HANDLE = f(VA, SYM("__imp_?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A"));
static Scoreboard* _scoreboard;//����Ʒְ�����
static unsigned _formid = 1;//��ID
static unordered_map<string, vector<PyObject*>> PyFuncs;//Py����
static unordered_map<Player*, bool> PlayerList;//����б�
static unordered_map<string, string> Command;//ע������
static unordered_map<string, PyObject*> ShareData;//ע������
#pragma endregion
#pragma region ��������
void init();
static bool callpy(const char* type, PyObject* val) {
	bool result = true;
	for (PyObject* fn : PyFuncs[type]) {
		if (PyObject_CallOneArg(fn, val) == Py_False)
			result = false;
	}
	PyErr_Print();
	return result;
}
static void delay(PyObject* func, PyObject* args, unsigned time) {
	if (time)
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
static bool TextPacket(Player* p, char mode, string msg) {
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
		VA nid = p->getNetId();
		SYMCALL<VA>("?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z", _ServerNetworkHandle, nid, pkt);
		//p->sendPacket(pkt);
		return true;
	}
	return false;
}
static bool BossEventPacket(Player* p, string name, float per, int eventtype) {
	if (PlayerCheck(p)) {
		VA pkt;//BossEventPacket
		createPacket(&pkt, 74);
		f(VA, pkt + 48) = f(VA, pkt + 56) = f(VA, p->getUniqueID());
		f(int, pkt + 64) = eventtype;//0��ʾ,1����,2����,
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
#pragma region API����
// ָ�����
api_function(logout) {
	const char* msg;
	if (PyArg_ParseTuple(args, "s:logout", &msg)) {
		SYMCALL<VA>("??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
			STD_COUT_HANDLE, msg, strlen(msg));
		return Py_True;
	}
	return Py_False;
}
// ִ��ָ��
api_function(runcmd) {
	const char* cmd;
	if (PyArg_ParseTuple(args, "s:runcmd", &cmd)) {
		SYMCALL<bool>("??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			_cmdqueue, (string)cmd);
		return Py_True;
	}
	return Py_False;
}
// ��ʱ
api_function(setTimeout) {
	unsigned time; PyObject* func; PyObject* arg;
	if (PyArg_ParseTuple(args, "OOI:setTimeout", &func, &arg, &time)) {
		thread(delay, func, arg, time).detach();
	}
	return Py_None;
}
// ���ü���
api_function(setListener) {
	const char* m;
	PyObject* func;
	if (PyArg_ParseTuple(args, "sO:setListener", &m, &func) && PyCallable_Check(func)) {
		PyFuncs[m].push_back(func);
		return Py_True;
	}
	return Py_False;
}
// ��������
api_function(setShareData) {
	const char* index; PyObject* data;
	if (PyArg_ParseTuple(args, "sO:setShareData", &index, &data)) {
		ShareData[index] = data;
		return Py_True;
	}
	return Py_False;
}
api_function(getShareData) {
	const char* index;
	if (PyArg_ParseTuple(args, "s:getShareData", &index)) {
		return ShareData[index];
	}
	return _PyLong_Zero;
}
// ����ָ��˵��
api_function(setCommandDescription) {
	const char* cmd, * des;
	if (PyArg_ParseTuple(args, "ss:setCommandDescribe", &cmd, &des)) {
		Command[cmd] = des;
		return Py_True;
	}
	return Py_False;
}
api_function(getPlayerList) {
	auto list = PyList_New(0);
	PyArg_ParseTuple(args, ":getPlayerList");
	for (auto& p : PlayerList) {
		PyList_Append(list, PyLong_FromUnsignedLongLong((VA)p.first));
	}
	return list;
}
// ���ͱ�
api_function(sendCustomForm) {
	Player* p; const char* str;
	if (PyArg_ParseTuple(args, "Ks:sendCustomForm", &p, &str)) {
		return PyLong_FromLong(ModalFormRequestPacket(p, str));
	}
	return PyLong_FromLong(0);
}
api_function(sendSimpleForm) {
	Player* p; const char* title, * content, * buttons;
	if (PyArg_ParseTuple(args, "Ksss:sendSimpleForm", &p, &title, &content, &buttons)) {
		char str[0x400];
		sprintf(str, R"({"title":"%s","content":"%s","buttons":%s,"type":"form"})", title, content, buttons);
		return PyLong_FromLong(ModalFormRequestPacket(p, str));
	}
	return _PyLong_Zero;
}
api_function(sendModalForm) {
	Player* p; const char* title, * content, * button1, * button2;
	if (PyArg_ParseTuple(args, "Kssss:sendModalForm", &p, &title, &content, &button1, &button2)) {
		char str[0x400];
		sprintf(str, R"({"title":"%s","content":"%s","button1":"%s","button2":"%s","type":"modal"})", title, content, button1, button2);
		return PyLong_FromLong(ModalFormRequestPacket(p, str));
	}
	return _PyLong_Zero;
}
// �������
api_function(transferServer) {
	Player* p; const char* address; int port;
	if (PyArg_ParseTuple(args, "Ksi:transferServer", &p, &address, &port)) {
		if (TransferPacket(p, address, port))
			return Py_True;
	}
	return Py_False;
}
// ��ȡ����ֳ�
api_function(getSelectedItem) {
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
// ��ȡ��ұ�����Ʒ
api_function(getInventoryItem) {
	Player* p; int slot;
	if (PyArg_ParseTuple(args, "Ki:getInventoryItem", &p, &slot)) {
		if (PlayerCheck(p)) {
			ItemStack* item = p->getInventoryItem(slot);
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
// ��ȡ�����Ϣ
api_function(getPlayerInfo) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:getPlayerInfo", &p)) {
		if (PlayerCheck(p)) {
			Vec3* pp = p->getPos();
			return Py_BuildValue("{s:s,s:s,s:[f,f,f],s:i,s:b,s:i,s:i}",
				"xuid", p->getXuid().c_str(),
				"playername", p->getNameTag().c_str(),
				"XYZ", pp->x, pp->y, pp->z,
				"dimensionid", p->getDimensionId(),
				"isstand", p->isStand(),
				"health", p->getHealth(),
				"maxhealth", p->getMaxHealth()
			);
		}
	}
	return PyDict_New();
}
api_function(getActorInfo) {
	Actor* a;
	if (PyArg_ParseTuple(args, "K:getActorInfo", &a)) {
		assert(a);
		Vec3* pp = a->getPos();
		return Py_BuildValue("{s:s,s:[f,f,f],s:i,s:b,s:f,s:f}",
			"actorname", a->getNameTag().c_str(),
			"XYZ", pp->x, pp->y, pp->z,
			"dimensionid", a->getDimensionId(),
			"isstand", a->isStand(),
			"health", a->getHealth(),
			"maxhealth", a->getMaxHealth()
		);
	}
	return PyDict_New();
}
// ���Ȩ��
api_function(getPlayerPerm) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:getPlayerPerm", &p)) {
		if (PlayerCheck(p)) {
			return PyLong_FromLong(p->getPermission());
		}
	}
	return _PyLong_Zero;
}
api_function(setPlayerPerm) {
	Player* p; unsigned char lv;
	if (PyArg_ParseTuple(args, "Kb:setPlayerPerm", &p, &lv)) {
		if (PlayerCheck(p)) {
			p->setPermissionLevel(lv);
			return Py_True;
		}
	}
	return Py_False;
}
// ������ҵȼ�
api_function(addLevel) {
	Player* p; int lv;
	if (PyArg_ParseTuple(args, "Ki::addLevel", &p, &lv)) {
		if (PlayerCheck(p)) {
			SYMCALL("?addLevels@Player@@UEAAXH@Z", p, lv);
			return Py_True;
		}
	}
	return Py_False;
}
// �����������
api_function(setName) {
	Player* p; const char* name;
	if (PyArg_ParseTuple(args, "Ks:setName", &p, &name)) {
		if (PlayerCheck(p)) {
			p->setName(name);
			return Py_True;
		}
	}
	return Py_False;
}
// ��ҷ���
api_function(getPlayerScore) {
	Player* p; const char* obj;
	if (PyArg_ParseTuple(args, "Ks:getPlayerScore", &p, &obj)) {
		if (PlayerCheck(p)) {
			Objective* testobj = _scoreboard->getObjective(obj);
			if (testobj) {
				auto id = _scoreboard->getScoreboardId(p);
				auto score = testobj->getPlayerScore(id);
				return PyLong_FromLong(score->getCount());
			}
			else printf("bad objective:%s", obj);
		}
	}
	return _PyLong_Zero;
}
api_function(modifyPlayerScore) {
	Player* p; const char* obj; int count; int mode;
	if (PyArg_ParseTuple(args, "Ksii:modifyPlayerScore", &p, &obj, &count, &mode)) {
		if (PlayerCheck(p)) {
			Objective* testobj = _scoreboard->getObjective(obj);
			if (testobj) {
				//mode:{set,add,remove}
				_scoreboard->modifyPlayerScore((ScoreboardId*)_scoreboard->getScoreboardId(p), testobj, count, mode);
				return Py_True;
			}
			else printf("bad objective:%s", obj);
		}
	}
	return Py_False;
}
// ģ����ҷ����ı�
api_function(talkAs) {
	Player* p; const char* msg;
	if (PyArg_ParseTuple(args, "Ks:talkAs", &p, &msg)) {
		if (TextPacket(p, 1, msg))
			return Py_True;
	}
	return Py_False;
}
// ģ�����ִ��ָ��
api_function(runcmdAs) {
	Player* p; const char* cmd;
	if (PyArg_ParseTuple(args, "Ks:runcmdAs", &p, &cmd)) {
		if (CommandRequestPacket(p, cmd))
			return Py_True;
	}
	return Py_False;
}
// �������
api_function(teleport) {
	Player* p; float x, y, z; int did;
	if (PyArg_ParseTuple(args, "Kfffi:teleport", &p, &x, &y, &z, &did)) {
		p->teleport({ x,y,z }, did);
		return Py_True;
	}
	return Py_False;
}
// ԭʼ���
api_function(tellraw) {
	const char* msg;
	Player* p;
	if (PyArg_ParseTuple(args, "Ks:tellraw", &p, &msg)) {
		if (TextPacket(p, 0, msg))
			return Py_True;
	}
	return Py_False;
}
// boss��
api_function(setBossBar) {
	Player* p; const char* name; float per;
	if (PyArg_ParseTuple(args, "Ksf:", &p, &name, &per)) {
		if (BossEventPacket(p, name, per, 0))
			return Py_True;
	}
	return Py_False;
}
api_function(removeBossBar) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:removeBossBar", &p)) {
		if (BossEventPacket(p, "", 0.0, 2))
			return Py_True;
	}
	return Py_False;
}
//ͨ�����ָ���ȡ�Ʒְ�id
api_function(getScoreBoardId) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:getScoreBoardId", &p)) {
		if (PlayerCheck(p)) {
			return Py_BuildValue("{s:i}",
				"scoreboardid", _scoreboard->getScoreboardId(p)
			);
		}
	}
	return Py_False;
}
api_function(createScoreBoardId) {
	Player* p;
	if (PyArg_ParseTuple(args, "K:createScoreBoardId", &p)) {
		if (PlayerCheck(p)) {
			_scoreboard->createScoreBoardId(p);
			return Py_True;
		}
	}
	return Py_False;
}
//�޸��������˵��˺�ֵ!
int _Damage;
api_function(setDamage) {
	int a;
	if (PyArg_ParseTuple(args, "i:setDamage", &a)) {
		_Damage = a;
		return Py_True;
	}
	return Py_False;
}
// �����б�
PyMethodDef api_list[] = {
api_method(logout),
api_method(runcmd),
api_method(setTimeout),
api_method(setListener),
api_method(setShareData),
api_method(getShareData),
api_method(setCommandDescription),
api_method(getPlayerList),
api_method(sendSimpleForm),
api_method(sendModalForm),
api_method(sendCustomForm),
api_method(transferServer),
api_method(getSelectedItem),
api_method(getInventoryItem),
api_method(getPlayerInfo),
api_method(getActorInfo),
api_method(getPlayerPerm),
api_method(setPlayerPerm),
api_method(addLevel),
api_method(setName),
api_method(getPlayerScore),
api_method(modifyPlayerScore),
api_method(talkAs),
api_method(runcmdAs),
api_method(teleport),
api_method(tellraw),
api_method(setBossBar),
api_method(removeBossBar),
api_method(getScoreBoardId),
api_method(createScoreBoardId),
api_method(setDamage),
{}
};
// ģ������
PyModuleDef api_module = { PyModuleDef_HEAD_INIT, "mc", 0, -1, api_list, 0, 0, 0, 0 };
extern "C" PyObject * mc_init() {
	return PyModule_Create(&api_module);
}
#pragma endregion
#pragma region Hook
Hook(��ȡָ�����, VA, "??0?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@QEAA@_K@Z",
	VA _this) {
	_cmdqueue = original(_this);
	return _cmdqueue;
}
Hook(��ȡ��ͼ��ʼ����Ϣ, VA, "??0Level@@QEAA@AEBV?$not_null@V?$NonOwnerPointer@VSoundPlayerInterface@@@Bedrock@@@gsl@@V?$unique_ptr@VLevelStorage@@U?$default_delete@VLevelStorage@@@std@@@std@@V?$unique_ptr@VLevelLooseFileStorage@@U?$default_delete@VLevelLooseFileStorage@@@std@@@4@AEAVIMinecraftEventing@@_NEAEAVScheduler@@AEAVStructureManager@@AEAVResourcePackManager@@AEAVIEntityRegistryOwner@@V?$unique_ptr@VBlockComponentFactory@@U?$default_delete@VBlockComponentFactory@@@std@@@4@V?$unique_ptr@VBlockDefinitionGroup@@U?$default_delete@VBlockDefinitionGroup@@@std@@@4@@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7, VA a8, VA a9, VA a10, VA a11, VA a12, VA a13) {
	_level = original(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
	return _level;
}
Hook(��ȡ��Ϸ��ʼ����Ϣ, VA, "??0GameSession@@QEAA@AEAVNetworkHandler@@V?$unique_ptr@VServerNetworkHandler@@U?$default_delete@VServerNetworkHandler@@@std@@@std@@AEAVLoopbackPacketSender@@V?$unique_ptr@VNetEventCallback@@U?$default_delete@VNetEventCallback@@@std@@@3@V?$unique_ptr@VLevel@@U?$default_delete@VLevel@@@std@@@3@E@Z",
	VA a1, VA a2, VA a3, VA a4, VA a5, VA a6, VA a7) {
	_ServerNetworkHandle = f(VA, a3);
	return original(a1, a2, a3, a4, a5, a6, a7);
}
Hook(����ע��, void, "?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",//"?setup@ChangeSettingCommand@@SAXAEAVCommandRegistry@@@Z",?setup@KillCommand@@SAXAEAVCommandRegistry@@@Z
	VA _this) {
	for (auto& cmd : Command) {
		SYMCALL("?registerCommand@CommandRegistry@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBDW4CommandPermissionLevel@@UCommandFlag@@3@Z",
			_this, cmd.first.c_str(), cmd.second.c_str(), 0, 0, 0);
	}
	original(_this);
}
Hook(�Ʒְ�, Scoreboard*, "??0ServerScoreboard@@QEAA@VCommandSoftEnumRegistry@@PEAVLevelStorage@@@Z",
	VA _this, VA a2, VA a3) {
	_scoreboard = (Scoreboard*)original(_this, a2, a3);
	return _scoreboard;
}
Hook(��̨���, VA, "??$_Insert_string@DU?$char_traits@D@std@@_K@std@@YAAEAV?$basic_ostream@DU?$char_traits@D@std@@@0@AEAV10@QEBD_K@Z",
	VA handle, const char* str, VA size) {
	if (handle == STD_COUT_HANDLE) {
		bool res = callpy(u8"��̨���", PyUnicode_FromString(str));
		if (!res)return 0;
	}
	return original(handle, str, size);
}
Hook(��̨����, bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
	VA _this, string* cmd) {
	/*����
	if (*cmd == "pyreload") {
		Py_Finalize();
		PyFuncs.clear();
		init();
		return false;
	}*/
	bool res = callpy(u8"��̨����", PyUnicode_FromString((*cmd).c_str()));
	check_ret(_this, cmd);
}
Hook(��Ҽ���, VA, "?onPlayerJoined@ServerScoreboard@@UEAAXAEBVPlayer@@@Z",
	VA a1, Player* p) {
	PlayerList[p] = true;
	callpy(u8"��Ҽ���", PyLong_FromUnsignedLongLong((VA)p));
	return original(a1, p);
}
Hook(�뿪��Ϸ, void, "?_onPlayerLeft@ServerNetworkHandler@@AEAAXPEAVServerPlayer@@_N@Z",
	VA _this, Player* p, char v3) {
	callpy(u8"�뿪��Ϸ", PyLong_FromUnsignedLongLong((VA)p));
	PlayerList.erase(p);
	return original(_this, p, v3);
}
Hook(ʹ����Ʒ, bool, "?useItemOn@GameMode@@UEAA_NAEAVItemStack@@AEBVBlockPos@@EAEBVVec3@@PEBVBlock@@@Z",
	VA _this, ItemStack* item, BlockPos* bp, unsigned __int8 a4, VA v5, Block* b) {
	Player* p = f(Player*, _this + 8);
	//auto tag = item->save();
	//tag->value["Name"] = "ss";
	//p->getContainer()->getSlots()[8]->fromTag(tag);
	//p->updateInventory();
	//p->addItem(item);
	//auto name = tag->getList("ench");
	//auto l = tag->getByte("Count");
	//TextPacket(p, 0, name);
	short iid = item->getId();
	short iaux = item->mAuxValue;
	string iname = item->getName();
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	bool res = true;
	res = callpy(u8"ʹ����Ʒ", Py_BuildValue("{s:K,s:i,s:i,s:s,s:s,s:i,s:[i,i,i]}",
		"player", p,
		"itemid", iid,
		"itemaux", iaux,
		"itemname", iname.c_str(),
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	));
	check_ret(_this, item, bp, a4, v5, b);
}
Hook(���÷���, bool, "?mayPlace@BlockSource@@QEAA_NAEBVBlock@@AEBVBlockPos@@EPEAVActor@@_N@Z",
	BlockSource* _this, Block* b, BlockPos* bp, unsigned __int8 a4, Actor* p, bool _bool) {
	bool res = true;
	if (p && PlayerCheck(p)) {
		BlockLegacy* bl = b->getBlockLegacy();
		short bid = bl->getBlockItemID();
		string bn = bl->getBlockName();
		res = callpy(u8"���÷���", Py_BuildValue("{s:K,s:s,s:i,s:[i,i,i]}",
			"player", p,
			"blockname", bn.c_str(),
			"blockid", bid,
			"position", bp->x, bp->y, bp->z
		));
	}
	check_ret(_this, b, bp, a4, p, _bool);
}
Hook(�ƻ�����, bool, "?_destroyBlockInternal@GameMode@@AEAA_NAEBVBlockPos@@E@Z",
	VA _this, BlockPos* bp) {
	Player* p = f(Player*, _this + 8);
	BlockSource* bs = f(BlockSource*, f(VA, _this + 8) + 840);
	Block* b = bs->getBlock(bp);
	BlockLegacy* bl = b->getBlockLegacy();
	short bid = bl->getBlockItemID();
	string bn = bl->getBlockName();
	bool res = true;
	res = callpy(u8"�ƻ�����", Py_BuildValue("{s:K,s:s,s:i,s:[i,i,i]}",
		"player", p,
		"blockname", bn.c_str(),
		"blockid", bid,
		"position", bp->x, bp->y, bp->z
	));
	check_ret(_this, bp);
}
Hook(������, bool, "?use@ChestBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
	VA _this, Player* p, BlockPos* bp) {
	bool res = callpy(u8"������", Py_BuildValue("{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	));
	check_ret(_this, p, bp);
}
Hook(��ľͰ, bool, "?use@BarrelBlock@@UEBA_NAEAVPlayer@@AEBVBlockPos@@E@Z",
	VA _this, Player* p, BlockPos* bp) {
	bool res = callpy(u8"��ľͰ", Py_BuildValue("{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	));
	check_ret(_this, p, bp);
}
Hook(�ر�����, void, "?stopOpen@ChestBlockActor@@UEAAXAEAVPlayer@@@Z",
	VA _this, Player* p) {
	auto bp = (BlockPos*)(_this - 204);
	callpy(u8"�ر�����", Py_BuildValue("{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	));
	original(_this, p);
}
Hook(�ر�ľͰ, void, "?stopOpen@BarrelBlockActor@@UEAAXAEAVPlayer@@@Z",
	VA _this, Player* p) {
	auto bp = (BlockPos*)(_this - 204);
	callpy(u8"�ر�ľͰ", Py_BuildValue("{s:K,s:[i,i,i]}",
		"player", p,
		"position", bp->x, bp->y, bp->z
	));
	original(_this, p);
}
Hook(����ȡ��, void, "?containerContentChanged@LevelContainerModel@@UEAAXH@Z",
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
			callpy(u8"����ȡ��", Py_BuildValue("{s:K,s:s,s:i,s:[i,i,i],s:i,s:i,s:s,s:i,s:i}",
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
Hook(��ҹ���, bool, "?attack@Player@@UEAA_NAEAVActor@@@Z",
	Player* p, Actor* a) {
	bool res = callpy(u8"��ҹ���", Py_BuildValue("{s:K,s:K}",
		"player", p,
		"actor", a
	));
	check_ret(p, a);
}
Hook(�л�ά��, bool, "?_playerChangeDimension@Level@@AEAA_NPEAVPlayer@@AEAVChangeDimensionRequest@@@Z",
	VA _this, Player* p, VA req) {
	bool result = original(_this, p, req);
	if (result) {
		callpy(u8"�л�γ��", PyLong_FromUnsignedLongLong((VA)p));
	}
	return result;
}
Hook(��������, void, "?die@Mob@@UEAAXAEBVActorDamageSource@@@Z",
	Mob* _this, VA dmsg) {
	char v72;
	Actor* sa = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		f(VA, _this + 856), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	bool res = callpy(u8"��������", Py_BuildValue("{s:i,s:K,s:K}",
		"dmcase", f(unsigned, dmsg + 8),
		"actor1", _this,
		"actor2", sa//����Ϊ0
	));
	if (res) original(_this, dmsg);
}
Hook(��������, bool, "?_hurt@Mob@@MEAA_NAEBVActorDamageSource@@H_N1@Z",
	Mob* _this, VA dmsg, int a3, bool a4, bool a5) {
	_Damage = a3;//���������˵�ֵ����Ϊ�ɵ���
	char v72;
	Actor* sa = SYMCALL<Actor*>("?fetchEntity@Level@@QEBAPEAVActor@@UActorUniqueID@@_N@Z",
		f(VA, _this + 856), *(VA*)((*(VA(__fastcall**)(VA, char*))(*(VA*)dmsg + 64))(dmsg, &v72)), 0);
	bool res = callpy(u8"��������", Py_BuildValue("{s:i,s:K,s:K,s:i}",
		"dmcase", f(unsigned, dmsg + 8),
		"actor1", _this,
		"actor2", sa,//����Ϊ0
		"damage", a3
	));
	check_ret(_this, dmsg, _Damage, a4, a5);
}
Hook(�������, void, "?respawn@Player@@UEAAXXZ",
	Player* p) {
	callpy(u8"�������", PyLong_FromUnsignedLongLong((VA)p));
	original(p);
}
Hook(������Ϣ, void, "?fireEventPlayerMessage@MinecraftEventing@@AEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@000@Z",
	VA _this, string& sender, string& target, string& msg, string& style) {
	callpy(u8"������Ϣ", Py_BuildValue("{s:s,s:s,s:s,s:s}",
		"sender", sender.c_str(),
		"target", target.c_str(),
		"msg", msg.c_str(),
		"style", style.c_str()
	));
	original(_this, sender, target, msg, style);
}
Hook(�����ı�, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVTextPacket@@@Z",
	VA _this, VA id, /*(TextPacket*)*/VA tp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, f(char, tp + 16));
	if (p) {
		string msg = f(string, tp + 80);
		bool res = callpy(u8"�����ı�", Py_BuildValue("{s:K,s:s}",
			"player", p,
			"msg", msg.c_str()
		));
		if (res)original(_this, id, tp);
	}
}
Hook(����ָ��, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandRequestPacket@@@Z",
	VA _this, VA id, /*(CommandRequestPacket*)*/VA crp) {
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		_this, id, f(char, crp + 16));
	if (p) {
		string cmd = f(string, crp + 40);
		bool res = callpy(u8"����ָ��", Py_BuildValue("{s:K,s:s}",
			"player", p,
			"cmd", cmd.c_str()
		));
		if (res)original(_this, id, crp);
	}
}
Hook(ѡ���, void, "?handle@?$PacketHandlerDispatcherInstance@VModalFormResponsePacket@@$0A@@@UEBAXAEBVNetworkIdentifier@@AEAVNetEventCallback@@AEAV?$shared_ptr@VPacket@@@std@@@Z",
	VA _this, VA id, VA handle,/*(ModalFormResponsePacket**)*/VA* fp) {
	VA fmp = *fp;
	Player* p = SYMCALL<Player*>("?_getServerPlayer@ServerNetworkHandler@@AEAAPEAVServerPlayer@@AEBVNetworkIdentifier@@E@Z",
		handle, id, f(char, fmp + 16));
	if (PlayerCheck(p)) {
		unsigned fid = f(unsigned, fmp + 40);
		string data = f(string, fmp + 48);

		//VA len = data.length() - 1;
		//if (data[len] == '\n')data[len] = '\0';
		callpy(u8"ѡ���", Py_BuildValue("{s:K,s:s,s:i}",
			"player", p,
			"selected", data.c_str(),
			"formid", fid
		));

	}
	original(_this, id, handle, fp);
}
Hook(���������, void, "?handle@ServerNetworkHandler@@UEAAXAEBVNetworkIdentifier@@AEBVCommandBlockUpdatePacket@@@Z",
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
		bool res = callpy(u8"��������", Py_BuildValue("{s:K,s:i,s:i,s:i,s:s,s:s,s:s,s:i,s:[i,i,i]}",
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
Hook(���籬ը, bool, "?explode@Level@@QEAAXAEAVBlockSource@@PEAVActor@@AEBVVec3@@M_N3M3@Z",
	Level* _this, BlockSource* bs, Actor* a3, const Vec3 pos, float a5, bool a6, bool a7, float a8, bool a9) {
	bool res = callpy(u8"���籬ը", Py_BuildValue("{s:K,s:[f,f,f],s:i,s:i}",
		"actor", a3,
		"position", pos.x, pos.y, pos.z,
		"dimensionid", bs->getDimensionId(),
		"power", a5
	));
	check_ret(_this, bs, a3, pos, a5, a6, a7, a8, a9);
}
Hook(�����ִ��, bool, "?performCommand@CommandBlockActor@@QEAA_NAEAVBlockSource@@@Z",
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
	bool res = callpy(u8"�����ִ��", Py_BuildValue("{s:s,s:s,s:[i,i,i],s:i,s:i}",
		"cmd", cmd.c_str(),
		"rawname", rawname.c_str(),
		"position", bp.x, bp.y, bp.z,
		"mode", mode,
		"condition", condition
	));
	check_ret(_this, a2);
}
Hook(��Ҵ���, void, "?setArmor@Player@@UEAAXW4ArmorSlot@@AEBVItemStack@@@Z",
	Player* p, unsigned slot, ItemStack* i) {
	if (!i->getId())return original(p, slot, i);
	bool res = callpy(u8"��Ҵ���", Py_BuildValue("{s:K,s:i,s:i,s:s,s:i,s:i}",
		"player", p,
		"itemid", i->getId(),
		"itemcount", i->mCount,
		"itemname", i->getName().c_str(),
		"itemaux", i->mAuxValue,
		"slot", slot
	));
	if (res)original(p, slot, i);
}
Hook(�Ʒְ�ı�, void, "?onScoreChanged@ServerScoreboard@@UEAAXAEBUScoreboardId@@AEBVObjective@@@Z",
	const Scoreboard* _this, ScoreboardId* a2, Objective* a3) {
	/*
	ԭ���
	�����Ʒְ�ʱ��/scoreboard objectives <add|remove> <objectivename> dummy <objectivedisplayname>
	�޸ļƷְ�ʱ���˺���hook�˴�)��/scoreboard players <add|remove|set> <playersname> <objectivename> <playersnum>
	*/
	int scoreboardid = a2->id;
	callpy(u8"�Ʒְ�ı�", Py_BuildValue("{s:i,s:i,s:s,s:s}",
		"scoreboardid", scoreboardid,
		"playersnum", a3->getPlayerScore(a2)->getCount(),
		"objectivename", a3->getScoreName().c_str(),
		"objectivedisname", a3->getScoreDisplayName().c_str()
	));
	/*
	cout << to_string(scoreboardid) << endl;//��ȡ�Ʒְ�id
	cout << to_string(a3->getPlayerScore(a2)->getCount()) << endl;//��ȡ�޸ĺ��<playersnum>
	cout << a3->getscorename() << endl;//��ȡ<objectivename>
	cout << a3->getscoredisplayname() << endl;//��ȡ<objectivedisname>
	*/
	original(_this, a2, a3);
}
Hook(�����ƻ�, void, "?transformOnFall@FarmBlock@@UEBAXAEAVBlockSource@@AEBVBlockPos@@PEAVActor@@M@Z",
	VA _this, BlockSource* a1, BlockPos* a2, Player* p, VA a4) {
	bool res = true;
	if (PlayerCheck(p)) {
		res = callpy(u8"�����ƻ�", Py_BuildValue("{s:K,s:[i,i,i],s:i}",
			"player", p,
			"position", a2->x, a2->y, a2->z,
			"dimensionid", a1->getDimensionId()
		));
	}
	if (res)original(_this, a1, a2, p, a4);
}
#pragma endregion
void init() {
	Py_Initialize();
	//pts["main"] = (VA)PyThreadState_Get();
	_finddata_t Info;//���ڲ��ҵľ��
	long long handle = _findfirst("./py/*.py", &Info);
	if (handle != -1) {
		do {
			//pts[name] = (VA)Py_NewInterpreter();
			FILE* file = fopen(string("./py/").append(Info.name).c_str(), "rb");
			Py_NewInterpreter();
			printf("[BDSpyrunner] reading %s.\n", Info.name);
			PyRun_SimpleFileExFlags(file, Info.name, 1, 0);
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
		PyImport_AppendInittab("mc", mc_init); //����һ��ģ��
		init();
		puts("[BDSpyrunner] v0.1.0 for BDS1.16.201 loaded.");
		puts("[BDSpyrunner] compilation time : " __TIME__ " " __DATE__);
	} return 1;
}