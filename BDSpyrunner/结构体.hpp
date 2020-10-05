#pragma once
#include "预编译头.h"
using namespace std;
struct BlockLegacy {
	string getBlockName() {
		return *(string*)((__int64)this + 112);
	}
	// 获取方块ID号
	auto getBlockItemID() const {			// IDA VanillaItems::initCreativeItemsCallback Item::beginCreativeGroup "itemGroup.name.planks"
		short v3 = *(short*)((VA)this + 268);
		if (v3 < 0x100) {
			return v3;
		}
		return (short)(255 - v3);
	}

};
struct Block {
	BlockLegacy* getBlockLegacy() {
		return SYMCALL(BlockLegacy*, "?getLegacyBlock@Block@@QEBAAEBVBlockLegacy@@XZ",
			this);
	}
};
struct BlockPos {
	int x, y, z;
};
struct BlockActor {
	// 取方块
	Block* getBlock() {
		return *reinterpret_cast<Block**>(reinterpret_cast<VA>(this) + 16);
	}
	// 取方块位置
	BlockPos* getPosition() {				// IDA BlockActor::BlockActor
		return reinterpret_cast<BlockPos*>(reinterpret_cast<VA>(this) + 44);
	}
};
struct BlockSource {
	Block* getBlock(BlockPos* bp) {
		return SYMCALL(Block*, "?getBlock@BlockSource@@QEBAAEBVBlock@@AEBVBlockPos@@@Z",
			this, bp);
	}
};
struct Dimension {
	// 获取方块源
	VA getBlockSouce() {					// IDA Level::tickEntities
		return *((VA*)this + 9);
	}
};
struct Level {
	// 获取维度
	Dimension* getDimension(int did) {
		return SYMCALL(Dimension*, "?getDimension@Level@@QEBAPEAVDimension@@V?$AutomaticID@VDimension@@H@@@Z",
			this, did);
	}
	// 获取计分板
	VA getScoreBoard() {				// IDA Level::removeEntityReferences
		return *(VA*)((VA)this + 8280);
	}
};
struct MCUUID {
	// 取uuid字符串
	string toString() {
		string s;
		SYMCALL(string&, "?asString@UUID@mce@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ", this, &s);
		return s;
	}
};
struct Vec3 {
	float x, y, z;
};
struct MobEffectInstance {
	char fill[0x1C];
};
struct Item{
};
struct ItemStackBase {
	VA vtable;
	VA mItem;
	VA mUserData;
	VA mBlock;
	short mAuxValue;
	char mCount;
	char mValid;
	char unk[4]{ 0 };
	VA mPickupTime;
	char mShowPickUp;
	char unk2[7]{ 0 };
	vector<VA*> mCanPlaceOn;
	VA mCanPlaceOnHash;
	vector<VA*> mCanDestroy;
	VA mCanDestroyHash;
	VA mBlockingTick;
	ItemStackBase* mChargedItem;
	VA uk;
public:
	/*VA save() {
		VA* cp = new VA[8]{ 0 };
		return SYMCALL(VA, MSSYM_MD5_e02d5851c93a43bfe24a4396ecb87cde, this, cp);
	}*/
#if (COMMERCIAL)
	Json::Value toJson() {
		VA t = save();
		Json::Value jv = (*(Tag**)t)->toJson();
		(*(Tag**)t)->clearAll();
		*(VA*)t = 0;
		delete (VA*)t;
		return jv;
	}
	void fromJson(Json::Value& jv) {
		VA t = Tag::fromJson(jv);
		SYMCALL(VA, MSSYM_B1QA7fromTagB1AA9ItemStackB2AAA2SAB1QA3AV1B1AE15AEBVCompoundTagB3AAAA1Z, this, *(VA*)t);
		(*(Tag**)t)->clearAll();
		*(VA*)t = 0;
		delete (VA*)t;
	}
	void fromTag(VA t) {
		SYMCALL(VA, MSSYM_B1QA7fromTagB1AA9ItemStackB2AAA2SAB1QA3AV1B1AE15AEBVCompoundTagB3AAAA1Z, this, t);
	}
#endif
	/*bool getFromId(short id, short aux, char count) {
		memcpy(this, SYM_POINT(void, MSSYM_B1QA5EMPTYB1UA4ITEMB1AA9ItemStackB2AAA32V1B1AA1B), 0x90);
		bool ret = SYMCALL(bool, MSSYM_B2QUA7setItemB1AE13ItemStackBaseB2AAA4IEAAB1UA2NHB1AA1Z, this, id);
		mCount = count;
		mAuxValue = aux;
		mValid = true;
		return ret;
	}*/
};
struct ItemStack : ItemStackBase {
	// 取物品ID
	short getId() {
		return SYMCALL(short, "?getId@ItemStackBase@@QEBAFXZ",
			this);
	}
	// 取物品特殊值
	short getAuxValue() {
		return SYMCALL(short, "?getAuxValue@ItemStackBase@@QEBAFXZ",
			this);
	}
	// 取物品名称
	string getName() {
		string str;
		SYMCALL(__int64, "?getName@ItemStackBase@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &str);
		return str;
	}
	// 取容器内数量
	int getStackSize() {			// IDA ContainerModel::networkUpdateItem
		return *((char*)this + 34);
	}
	// 判断是否空容器
	/*bool isNull() {
		return SYMCALL(bool,
			MSSYM_B1QA6isNullB1AE13ItemStackBaseB2AAA4QEBAB1UA3NXZ,
			this);
	}*/
};
struct Actor {
	// 获取生物名称信息
	string getNameTag() {
		return SYMCALL(string&, "?getNameTag@Actor@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this);
	}
	// 获取生物当前所处维度ID
	int getDimensionId() {
		int dimensionId;
		SYMCALL(int&, "?getDimensionId@Actor@@UEBA?AV?$AutomaticID@VDimension@@H@@XZ",
			this, &dimensionId);
		return dimensionId;
	}
	// 获取生物当前所在坐标
	Vec3* getPos() {
		return SYMCALL(Vec3*, "?getPos@Actor@@UEBAAEBVVec3@@XZ",
			this);
	}
	// 是否悬空
	const BYTE isStand() {				// IDA MovePlayerPacket::MovePlayerPacket
		return *reinterpret_cast<BYTE*>(reinterpret_cast<VA>(this) + 416);
	}
	// 取方块源
	BlockSource* getRegion() {
		return *reinterpret_cast<BlockSource**>(reinterpret_cast<VA>(this) + 414 * sizeof(void*));
	}
	// 获取生物类型
	string getTypeName() {
		string actor_typename;
		SYMCALL(string&,
			"?getEntityName@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVActor@@@Z",
			&actor_typename, this);
		return actor_typename;
	}

	// 获取实体类型
	int getEntityTypeId() {
		return SYMCALL(int,
			"?getEntityTypeId@Actor@@UEBA?AW4ActorType@@XZ",
			this);
		//		if (t == 1)		// 未知类型，可能是玩家
		//			return 319;
	}

	// 获取查询用ID
	VA* getUniqueID() {
		return SYMCALL(VA*, "?getUniqueID@Actor@@QEBAAEBUActorUniqueID@@XZ", this);
	}

	// 获取实体名称
	string getEntityTypeName() {
		string en_name;
		SYMCALL(string&,
			"?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&en_name, getEntityTypeId());
		return en_name;
	}

	// 更新属性
	VA updateAttrs() {
		return SYMCALL(VA, "?_sendDirtyActorData@Actor@@QEAAXXZ", this);
	}
	// 添加一个状态
	VA addEffect(VA ef) {
		return SYMCALL(VA, "?addEffect@Actor@@QEAAXAEBVMobEffectInstance@@@Z", this, ef);
	}
};
struct Mob : Actor {
	// 获取状态列表
	vector<MobEffectInstance>* getEffects() {					// IDA Mob::addAdditionalSaveData
		return (vector<MobEffectInstance>*)((VA*)this + 152);
	}

	// 获取装备容器
	VA getArmor() {					// IDA Mob::addAdditionalSaveData
		return VA(this) + 1400;
	}
	// 获取手头容器
	VA getHands() {
		return VA(this) + 1408;		// IDA Mob::readAdditionalSaveData
	}
	// 保存当前副手至容器
	VA saveOffhand(VA hlist) {
		return SYMCALL(VA, "?saveOffhand@Mob@@IEBA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, hlist);
	}
	// 获取地图信息
	VA getLevel() {					// IDA Mob::die
		return *((VA*)((VA)this + 816));
	}
};
struct Player : Mob {
	// 取uuid
	MCUUID* getUuid() {	// IDA ServerNetworkHandler::_createNewPlayer
		return (MCUUID*)((char*)this + 2720);
	}

	// 根据地图信息获取玩家xuid
	string& getXuid(VA level) {
		return SYMCALL(string&, "?getPlayerXUID@Level@@QEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVUUID@mce@@@Z",
			level, (char*)this + 2720);
	}
	// 重设服务器玩家名
	void reName(string name) {
		SYMCALL(void, "?setName@Player@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			this, name);
	}
	// 获取网络标识符
	VA getNetId() {
		return (VA)this + 2432;		// IDA ServerPlayer::setPermissions
	}

	VA getContainerManager() {
		return (VA)this + 2912;		// IDA Player::setContainerManager
	}
	// 获取背包
	VA getSupplies() {				// IDA Player::add
		return *(VA*)(*((VA*)this + 366) + 176);
	}
	// 获取末影箱
	VA getEnderChestContainer() {
		return SYMCALL(VA, "?getEnderChestContainer@Player@@QEAAPEAVEnderChestContainer@@XZ", this);
	}
	// 设置一个装备
	VA setArmor(int i, VA item) {
		return SYMCALL(VA, "?setArmor@ServerPlayer@@UEAAXW4ArmorSlot@@AEBVItemStack@@@Z", this, i, item);
	}
	// 设置副手
	VA setOffhandSlot(VA item) {
		return SYMCALL(VA, "?setOffhandSlot@Player@@UEAAXAEBVItemStack@@@Z", this, item);
	}
	// 添加一个物品
	void addItem(VA item) {
		SYMCALL(VA, "?add@Player@@UEAA_NAEAVItemStack@@@Z", this, item);
	}
	// 获取当前选中的框位置
	int getSelectdItemSlot() {			// IDA Player::getSelectedItem
		VA v1 = *((VA*)this + 366);
		return *(unsigned int*)(v1 + 16);
	}
	// 获取当前物品
	ItemStack* getSelectedItem() {
		return SYMCALL(ItemStack*, "?getSelectedItem@Player@@QEBAAEBVItemStack@@XZ", this);
	}
	// 获取游戏时命令权限
	char getPermission() {						// IDA ServerPlayer::setPermissions
		return **(char**)((VA)this + 2112);
	}
	// 设置游戏时命令权限
	void setPermission(char m) {
		SYMCALL(void, "?setPermissions@ServerPlayer@@UEAAXW4CommandPermissionLevel@@@Z",
			this, m);
	}
	// 获取游戏时游玩权限
	char getPermissionLevel() {						// IDA Abilities::setPlayerPermissions
		return (*(char**)((VA)this + 2112))[1];
	}
	// 设置游戏时游玩权限
	void setPermissionLevel(char m) {
		SYMCALL(void, "?setPlayerPermissions@Abilities@@QEAAXW4PlayerPermissionLevel@@@Z",
			(VA)this + 2112, m);
	}
	// 更新所有物品列表
	void updateInventory() {
		VA itm = (VA)this + 4472;				// IDA Player::drop
		SYMCALL(VA, "?forceBalanceTransaction@InventoryTransactionManager@@QEAAXXZ", itm);
	}
	// 发送数据包
	VA sendPacket(VA pkt) {
		return SYMCALL(VA, "?sendNetworkPacket@ServerPlayer@@UEBAXAEAVPacket@@@Z",
			this, pkt);
	}
};
struct LevelContainerModel {
	// 取开容者
	Player* getPlayer() {
		return ((Player**)this)[26];
	}
};
struct Container {
	VA vtable;
	// 获取容器内所有物品
	VA getSlots(vector<ItemStack*>* s) {
		return SYMCALL(VA, "?getSlots@Container@@UEBA?BV?$vector@PEBVItemStack@@V?$allocator@PEBVItemStack@@@std@@@std@@XZ",
			this, s);
	}

};
struct SimpleContainer : Container {
	// 获取一个指定框内物品
	ItemStack* getItem(int slot) {
		return SYMCALL(ItemStack*, "?getItem@SimpleContainer@@UEBAAEBVItemStack@@H@Z", this, slot);
	}
	// 设置一个指定框内的物品
	VA setItem(int slot, ItemStack* item) {
		return SYMCALL(VA, "?setItem@SimpleContainer@@UEAAXHAEBVItemStack@@@Z",
			this, slot, item);
	}
};
struct FillingContainer : Container {
	// 格式化容器所有物品至tag
	VA save(VA tag) {
		return SYMCALL(VA, "?save@FillingContainer@@QEAA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, tag);
	}
	// 设置容器中指定位置的物品
	VA setItem(int i, VA item) {
		return SYMCALL(VA, "?setItem@FillingContainer@@UEAAXHAEBVItemStack@@@Z",
			this, i, item);
	}
};
struct IContainerManager {
	VA vtable;
};
struct PlayerInventoryProxy {
	VA vtable;
	VA vtable2;
	int mSelected;
	char uk1[4];
	ItemStack mInfiniteItem;
	char mSelectedContainerId;
	char uk2[7];
	VA mInventory;
	vector<ItemStack> mComplexItems;
	weak_ptr<VA> mHudContainerManager;
};
struct ContainerItemStack
	:ItemStack {

};
struct ContainerManagerModel {
	// 取开容者
	Player* getPlayer() {				// IDA ContainerManagerModel::ContainerManagerModel
		return *reinterpret_cast<Player**>(reinterpret_cast<VA>(this) + 8);
	}
};
struct LevelContainerManagerModel
	:ContainerManagerModel {
};
struct TextPacket {
	char filler[0xC8];
	// 取输入文本
	string toString() {			// IDA ServerNetworkHandler::handle
		string str = string(*(string*)((VA)this + 80));
		return str;
	}
};
struct CommandRequestPacket {
	char filler[0x90];
	// 取命令文本
	string toString() {			// IDA ServerNetworkHandler::handle
		string str = string(*(string*)((VA)this + 40));
		return str;
	}
};
struct ModalFormRequestPacket {
	char filler[0x48];
};
struct ModalFormResponsePacket {
	// 取发起表单ID
	UINT getFormId() {
		return *(UINT*)((VA)this + 40);
	}
	// 取选择序号
	string getSelectStr() {
		string x = *(string*)((VA)this + 48);
		VA l = x.length();
		if (x.c_str()[l - 1] == '\n') {
			return l > 1 ? x.substr(0, l - 1) :
				x;
		}
		return x;
	}
};
struct ScoreboardId {
	//
};
struct PlayerScoreboardId {
	//
};
struct PlayerScore {
	//  *(_QWORD *)this = *(_QWORD *)a2;//ScoreboardId *a2
	//*((_QWORD*)this + 1) = *((_QWORD*)a2 + 1);
	// *((_DWORD *)this + 4) = int;
	auto getscore() {
		return *((__int64*)this + 4);
	}
};
struct ScoreInfo {
	//scoreboardcmd list; objective::objective; scoreboard getscores
	//int scores    +12 this[12]
	// string displayname  +96
	//string name +64
	int getCount() {
		return *(int*)((VA)(this) + 12);
	}
};
struct Objective {
	ScoreInfo* getPlayerScore(ScoreInfo* a1, ScoreboardId* a2) {
		return SYMCALL(ScoreInfo*, "?getPlayerScore@Objective@@QEBA?AUScoreInfo@@AEBUScoreboardId@@@Z", this, a1, a2);
	}
	ScoreInfo* getPlayerScore(ScoreInfo* a1, PlayerScoreboardId* a2) {
		return SYMCALL(ScoreInfo*, "?getPlayerScore@Objective@@QEBA?AUScoreInfo@@AEBUScoreboardId@@@Z", this, a1, a2);
	}
};
struct IdentityDictionary {
	//4个unmap
	ScoreboardId* getScoreboardID(PlayerScoreboardId* a2) {
		return SYMCALL(ScoreboardId*, "??$_getScoreboardId@UPlayerScoreboardId@@@IdentityDictionary@@AEBAAEBUScoreboardId@@AEBUPlayerScoreboardId@@AEBV?$unordered_map@UPlayerScoreboardId@@UScoreboardId@@U?$hash@UPlayerScoreboardId@@@std@@U?$equal_to@UPlayerScoreboardId@@@4@V?$allocator@U?$pair@$$CBUPlayerScoreboardId@@UScoreboardId@@@std@@@4@@std@@@Z", this, a2, this);
	}
};
struct ScoreboardIdentityRef {
	//bool请设置为1; a6=0 set,a6=1,add,a6=2,remove
	bool modifiedscores(Objective* obj, __int64 num, unsigned __int8 setfun) {
		int v25 = 0;
		int nums = static_cast<int>(num);
		return SYMCALL(bool, "?modifyScoreInObjective@ScoreboardIdentityRef@@QEAA_NAEAHAEAVObjective@@HW4PlayerScoreSetFunction@@@Z", this, &v25, obj, nums, setfun);
	}
};
struct Scoreboard {
	auto getObjective(string* str) {
		return SYMCALL(Objective*, "?getObjective@Scoreboard@@QEBAPEAVObjective@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z", this, str);
	}
	auto getScoreboardId(string* str) {
		return SYMCALL(ScoreboardId*, "?getScoreboardId@Scoreboard@@QEBAAEBUScoreboardId@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z", this, str);
	}
	vector<Objective*>* getObjectives() {
		return SYMCALL(vector<Objective*>*, "?getObjectives@Scoreboard@@QEBA?AV?$vector@PEBVObjective@@V?$allocator@PEBVObjective@@@std@@@std@@XZ", this);
	}
	auto getDisplayInfoFiltered(string* str) {
		return SYMCALL(vector<PlayerScore>*, "?getDisplayInfoFiltered@Scoreboard@@QEBA?AV?$vector@UPlayerScore@@V?$allocator@UPlayerScore@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@@Z", this, str);
	}
	auto getTrackedIds() {
		return SYMCALL(vector<ScoreboardId>*, "?getTrackedIds@Scoreboard@@QEBA?AV?$vector@UScoreboardId@@V?$allocator@UScoreboardId@@@std@@@std@@XZ", this);
	}
	auto getIdentityDictionary() {
		return (IdentityDictionary*)((char*)this + 80);//gouzaohanshu
	}
	auto getScoreboardID(Player* a2) {
		return SYMCALL(PlayerScoreboardId*, "?getScoreboardId@Scoreboard@@QEBAAEBUScoreboardId@@AEBVActor@@@Z", this, a2);
	}
	auto getScoreboardIdentityRef(ScoreboardId* a2) {
		return SYMCALL(ScoreboardIdentityRef*, "?getScoreboardIdentityRef@Scoreboard@@QEAAPEAVScoreboardIdentityRef@@AEBUScoreboardId@@@Z", this, a2);
	}
	//bool请设置为1; a6=0 set,a6=1,add,a6=2,remove
	auto modifyPlayerScore(bool* a2, ScoreboardId* a3, Objective* a4, int a5, unsigned __int8 a6) {
		return SYMCALL(int, "?modifyPlayerScore@Scoreboard@@QEAAHAEA_NAEBUScoreboardId@@AEAVObjective@@HW4PlayerScoreSetFunction@@@Z", this, a2, a3, a4, a5, a6);
	}
};
