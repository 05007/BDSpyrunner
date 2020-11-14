#pragma once
#include "预编译头.h"
#include <vector>
#include <unordered_map>
#include "head/Component.h"
//#include "head/ArduinoJson.h"
#define fetch(type,ptr) *(type*)(ptr)
using namespace std;
#pragma region 方块
struct BlockLegacy {
	string getBlockName() {
		return fetch(string, this + 112);
	}
	// 获取方块ID号
	short getBlockItemID() {	// IDA VanillaItems::initCreativeItemsCallback Item::beginCreativeGroup "itemGroup.name.planks"
		short v3 = fetch(short, this + 268);
		if (v3 < 0x100) {
			return v3;
		}
		return (short)(255 - v3);
	}

};
struct Block {
	BlockLegacy* getBlockLegacy() {
		return SYMCALL<BlockLegacy*>("?getLegacyBlock@Block@@QEBAAEBVBlockLegacy@@XZ",
			this);
	}
};
struct BlockPos {
	int x, y, z;
};
struct BlockActor {
	// 取方块
	Block* getBlock() {
		return fetch(Block*, this + 16);
	}
	// 取方块位置
	BlockPos* getPosition() {				// IDA BlockActor::BlockActor
		return fetch(BlockPos*, this + 44);
	}
};
struct BlockSource {
	Block* getBlock(BlockPos* bp) {
		return SYMCALL<Block*>("?getBlock@BlockSource@@QEBAAEBVBlock@@AEBVBlockPos@@@Z",
			this, bp);
	}
};
#pragma endregion
struct Dimension {
	// 获取方块源
	VA getBlockSouce() {// IDA Level::tickEntities
		return fetch(VA, this + 9);
	}
};
struct Level {
	// 获取维度
	Dimension* getDimension(int did) {
		return SYMCALL<Dimension*>("?getDimension@Level@@QEBAPEAVDimension@@V?$AutomaticID@VDimension@@H@@@Z",
			this, did);
	}
	// 获取计分板
	VA getScoreBoard() {				// IDA Level::removeEntityReferences
		return fetch(VA, this + 8280);
	}
};
struct Vec3 {
	float x, y, z;
};
struct Vec2 {
	float x, y;
};
struct MobEffectInstance {
	char fill[0x1C];
};
struct CompoundTag {
	string toString() {
		string a;
		return SYMCALL<string&>("?toString@CompoundTag@@UEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &a);
	}
};
struct Item {
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

	ItemStackBase() { memcpy(this, GetServerSymbol("?EMPTY_ITEM@ItemStack@@2V1@B"), 0x90); }
	// 取物品ID
	short getId() {
		return SYMCALL<short>("?getId@ItemStackBase@@QEBAFXZ", this);
	}
	// 取物品特殊值
	short getAuxValue() {
		return SYMCALL<short>("?getAuxValue@ItemStackBase@@QEBAFXZ", this);
	}
	// 取物品名称
	string getName() {
		string str;
		SYMCALL<__int64>("?getName@ItemStackBase@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &str);
		return str;
	}
	// 取容器内数量
	int getStackSize() {			// IDA ContainerModel::networkUpdateItem
		return fetch(int, this + 34);
	}
	// 判断是否空容器
	bool isNull() {
		return SYMCALL<bool>("?isNull@ItemStackBase@@QEBA_NXZ", this);
	}
	CompoundTag* getNetworkUserData() {
		void* a;
		return SYMCALL<CompoundTag*>("?getNetworkUserData@ItemStackBase@@QEBA?AV?$unique_ptr@VCompoundTag@@U?$default_delete@VCompoundTag@@@std@@@std@@XZ",
			this, &a);
	}
	CompoundTag* save() {
		string* a;
		return SYMCALL<CompoundTag*>("?save@ItemStackBase@@QEBA?AV?$unique_ptr@VCompoundTag@@U?$default_delete@VCompoundTag@@@std@@@std@@XZ",
			this, &a);
	}
	bool isEmptyStack() {
		return fetch(char, this + 34) == 0;
	}
	//Json::Value toJson() {
	//	VA t = save();
	//	Json::Value jv = (*(Tag**)t)->toJson();
	//	(*(Tag**)t)->clearAll();
	//	*(VA*)t = 0;
	//	delete (VA*)t;
	//	return jv;
	//}
	//void fromJson(Json::Value& jv) {
	//	VA t = Tag::fromJson(jv);
	//	SYMCALL<VA, "?fromTag@ItemStack@@SA?AV1@AEBVCompoundTag@@@Z",
	//		this, *(VA*)t);
	//	(*(Tag**)t)->clearAll();
	//	*(VA*)t = 0;
	//	delete (VA*)t;
	//}
	void fromTag(CompoundTag* t) {
		SYMCALL<VA>("?fromTag@ItemStack@@SA?AV1@AEBVCompoundTag@@@Z", this, t);
	}
	bool getFromId(short id, short aux, char count) {
		memcpy(this, GetServerSymbol("?EMPTY_ITEM@ItemStack@@2V1@B"), 0x90);
		bool ret = SYMCALL<bool>("?_setItem@ItemStackBase@@IEAA_NH@Z", this, id);
		mCount = count;
		mAuxValue = aux;
		mValid = true;
		return ret;
	}
	Item* getItem() {
		return SYMCALL<Item*>("?getItem@ItemStackBase@@QEBAPEBVItem@@XZ", this);
	}
	string toString() {
		string a;
		return SYMCALL<string&>("?toString@ItemStackBase@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &a);
	}
};
struct ItemStack : ItemStackBase {
};
#pragma region Actor
struct Container {
	// 获取容器内所有物品
	VA getSlots(vector<ItemStack*>* s) {
		return SYMCALL<VA>("?getSlots@Container@@UEBA?BV?$vector@PEBVItemStack@@V?$allocator@PEBVItemStack@@@std@@@std@@XZ",
			this, s);
	}
};
struct Actor {
	// 获取生物名称信息
	string getNameTag() {
		return SYMCALL<string&>("?getNameTag@Actor@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ", this);
	}
	// 获取生物当前所处维度ID
	int getDimensionId() {
		int did;
		SYMCALL<int&>("?getDimensionId@Actor@@UEBA?AV?$AutomaticID@VDimension@@H@@XZ",
			this, &did);
		return did;
	}
	// 获取生物当前所在坐标
	Vec3* getPos() {
		return SYMCALL<Vec3*>("?getPos@Actor@@UEBAAEBVVec3@@XZ", this);
	}
	// 是否悬空
	bool isStand() {// IDA MovePlayerPacket::MovePlayerPacket
		return fetch(bool, this + 416);
	}
	// 取方块源
	BlockSource* getRegion() {
		return fetch(BlockSource*, this + 3312);
	}
	// 获取生物类型
	string getTypeName() {
		string tn;
		SYMCALL<string&>("?getEntityName@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVActor@@@Z",
			&tn, this);
		return tn;
	}
	// 获取实体类型
	int getEntityTypeId() {
		return SYMCALL<int>("?getEntityTypeId@Actor@@UEBA?AW4ActorType@@XZ",
			this);
	}
	// 获取查询用ID
	VA* getUniqueID() {
		return SYMCALL<VA*>("?getUniqueID@Actor@@QEBAAEBUActorUniqueID@@XZ", this);
	}
	// 获取实体名称
	string getEntityTypeName() {
		string en_name;
		SYMCALL<string&>("?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&en_name, getEntityTypeId());
		return en_name;
	}
	// 更新属性
	VA updateAttrs() {
		return SYMCALL<VA>("?_sendDirtyActorData@Actor@@QEAAXXZ", this);
	}
	// 添加一个状态
	VA addEffect(VA ef) {
		return SYMCALL<VA>("?addEffect@Actor@@QEAAXAEBVMobEffectInstance@@@Z", this, ef);
	}
	// 获取生命值
	float getHealth() {
		VA bpattrmap = fetch(VA, this + 135);// IDA ScriptHealthComponent::retrieveComponentFrom
		if (bpattrmap) {
			VA hattr = SYMCALL<VA>("?getMutableInstance@BaseAttributeMap@@QEAAPEAVAttributeInstance@@I@Z",
				bpattrmap, 7);//SYM_OBJECT(UINT32, 0x019700B8 + 4));// SharedAttributes::HEALTH
			if (hattr) {
				return fetch(float, hattr + 33);
			}
		}
		return 0;
	}
	bool setHealth(float& value, float& max) {
		VA bpattrmap = ((VA*)this)[135];
		if (bpattrmap) {
			VA hattr = SYMCALL<VA>("?getMutableInstance@BaseAttributeMap@@QEAAPEAVAttributeInstance@@I@Z",
				bpattrmap, 7);// SYM_OBJECT(UINT32, 0x019700B8 + 4));// SharedAttributes::HEALTH
			if (hattr) {
				fetch(float, hattr + 33) = value;
				fetch(float, hattr + 32) = max;
				SYMCALL<VA>("?_setDirty@AttributeInstance@@AEAAXXZ", hattr);
				return true;
			}
		}
		return false;
	}
};
struct Mob : Actor {
	// 获取状态列表
	vector<MobEffectInstance>* getEffects() {					// IDA Mob::addAdditionalSaveData
		return (vector<MobEffectInstance>*)((VA*)this + 152);
	}
	// 获取装备容器
	VA getArmor() {		// IDA Mob::addAdditionalSaveData
		return VA(this) + 1400;
	}
	// 获取手头容器
	VA getHands() {
		return VA(this) + 1408;		// IDA Mob::readAdditionalSaveData
	}
	// 保存当前副手至容器
	VA saveOffhand(VA hlist) {
		return SYMCALL<VA>("?saveOffhand@Mob@@IEBA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, hlist);
	}
	// 获取地图信息
	VA getLevel() {					// IDA Mob::die
		return fetch(VA, this + 816);
	}
};
struct Player : Mob {
	// 取uuid
	string getUuid() {	// IDA ServerNetworkHandler::_createNewPlayer
		string p;
		SYMCALL<string&>("?asString@UUID@mce@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this + 2720, &p);
		return p;
	}
	// 发送数据包
	void sendPacket(VA pkt) {
		return SYMCALL<void>("?sendNetworkPacket@ServerPlayer@@UEBAXAEAVPacket@@@Z",
			this, pkt);
	}
	// 根据地图信息获取玩家xuid
	string& getXuid(Level* level) {
		return SYMCALL<string&>("?getPlayerXUID@Level@@QEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVUUID@mce@@@Z",
			level, this + 2720);
	}
	// 重设服务器玩家名
	void setName(string name) {
		SYMCALL<void>("?setName@Player@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			this, name);
	}
	// 获取网络标识符
	VA getNetId() {
		return (VA)this + 2432;// IDA ServerPlayer::setPermissions
	}
	// 获取背包
	Container* getContainer() {
		return (Container*)fetch(VA, fetch(VA, this + 366) + 176);
	}
	VA getContainerManager() {
		return (VA)this + 2912;		// IDA Player::setContainerManager
	}
	// 获取末影箱
	VA getEnderChestContainer() {
		return SYMCALL<VA>("?getEnderChestContainer@Player@@QEAAPEAVEnderChestContainer@@XZ", this);
	}
	// 设置一个装备
	VA setArmor(int i, ItemStack* item) {
		return SYMCALL<VA>("?setArmor@ServerPlayer@@UEAAXW4ArmorSlot@@AEBVItemStack@@@Z", this, i, item);
	}
	// 设置副手
	VA setOffhandSlot(ItemStack* item) {
		return SYMCALL<VA>("?setOffhandSlot@Player@@UEAAXAEBVItemStack@@@Z", this, item);
	}
	// 添加一个物品
	void addItem(ItemStack* item) {
		(*(__int64(__fastcall**)(VA, struct ItemStack*))(*fetch(VA*, fetch(VA, this + 366) + 176) + 256))(
			fetch(VA, fetch(VA, this + 366) + 176),
			item);
		//SYMCALL<VA>("?add@Player@@UEAA_NAEAVItemStack@@@Z", this, item);
	}
	// 获取当前选中的框位置
	int getSelectdItemSlot() {			// IDA Player::getSelectedItem
		return fetch(unsigned, fetch(VA, this + 366) + 16);
	}
	// 获取当前物品
	ItemStack* getSelectedItem() {
		return SYMCALL<ItemStack*>("?getSelectedItem@Player@@QEBAAEBVItemStack@@XZ", this);
	}
	// 获取游戏时命令权限
	char getPermission() {						// IDA ServerPlayer::setPermissions
		return *fetch(char*, this + 2112);
	}
	// 设置游戏时命令权限
	void setPermission(char m) {
		SYMCALL<void>("?setPermissions@ServerPlayer@@UEAAXW4CommandPermissionLevel@@@Z",
			this, m);
	}
	// 获取游戏时游玩权限
	char getPermissionLevel() {		// IDA Abilities::setPlayerPermissions
		return fetch(char, fetch(char*, this + 2112) + 1);
	}
	// 设置游戏时游玩权限
	void setPermissionLevel(char m) {
		SYMCALL<void>("?setPlayerPermissions@Abilities@@QEAAXW4PlayerPermissionLevel@@@Z",
			this + 2112, m);
	}
	// 更新所有物品列表
	void updateInventory() {
		SYMCALL<VA>("?forceBalanceTransaction@InventoryTransactionManager@@QEAAXXZ",
			this + 4472);// IDA Player::drop
	}
	//传送
	void teleport(Vec3 target, int dim) {
		SYMCALL<void>("?teleport@TeleportCommand@@SAXAEAVActor@@VVec3@@PEAV3@V?$AutomaticID@VDimension@@H@@VRelativeFloat@@4HAEBUActorUniqueID@@@Z",
			this, target, 0, dim, 0, 0, 0, GetServerSymbol("?INVALID_ID@ActorUniqueID@@2U1@B"));
	}
};
#pragma endregion
#pragma region 容器
struct LevelContainerModel {
	// 取开容者
	Player* getPlayer() {
		return ((Player**)this)[26];
	}
};
struct SimpleContainer : Container {
	// 获取一个指定框内物品
	ItemStack* getItem(int slot) {
		return SYMCALL<ItemStack*>("?getItem@SimpleContainer@@UEBAAEBVItemStack@@H@Z", this, slot);
	}
	// 设置一个指定框内的物品
	VA setItem(int slot, ItemStack* item) {
		return SYMCALL<VA>("?setItem@SimpleContainer@@UEAAXHAEBVItemStack@@@Z",
			this, slot, item);
	}
};
struct FillingContainer : Container {
	// 格式化容器所有物品至tag
	VA save(VA tag) {
		return SYMCALL<VA>("?save@FillingContainer@@QEAA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, tag);
	}
	// 设置容器中指定位置的物品
	VA setItem(int i, VA item) {
		return SYMCALL<VA>("?setItem@FillingContainer@@UEAAXHAEBVItemStack@@@Z",
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
#pragma endregion
#pragma region 计分板
struct ScoreboardId {
};
struct PlayerScoreboardId {
};
struct PlayerScore {
	//  *(_QWORD *)this = *(_QWORD *)a2;//ScoreboardId *a2
	//*((_QWORD*)this + 1) = *((_QWORD*)a2 + 1);
	// *((_DWORD *)this + 4) = int;
	VA getscore() {
		return fetch(VA, this + 4);
	}
};
struct ScoreInfo {
	//scoreboardcmd list; objective::objective; scoreboard getscores
	//int scores    +12 this[12]
	// string displayname  +96
	//string name +64
	int getCount() {
		return fetch(int, this + 12);
	}
};
struct Objective {
	unordered_map<ScoreboardId, int> smap;
	string name, name2;
	VA Criteria;
	ScoreInfo* getPlayerScore(ScoreInfo* a1, ScoreboardId* a2) {
		return SYMCALL<ScoreInfo*>("?getPlayerScore@Objective@@QEBA?AUScoreInfo@@AEBUScoreboardId@@@Z",
			this, a1, a2);
	}
	ScoreInfo* getPlayerScore(ScoreInfo* a1, PlayerScoreboardId* a2) {
		return SYMCALL<ScoreInfo*>("?getPlayerScore@Objective@@QEBA?AUScoreInfo@@AEBUScoreboardId@@@Z",
			this, a1, a2);
	}
};
struct IdentityDictionary {
	//4个unmap
	ScoreboardId* getScoreboardID(PlayerScoreboardId* a2) {
		return SYMCALL<ScoreboardId*>("??$_getScoreboardId@UPlayerScoreboardId@@@IdentityDictionary@@AEBAAEBUScoreboardId@@AEBUPlayerScoreboardId@@AEBV?$unordered_map@UPlayerScoreboardId@@UScoreboardId@@U?$hash@UPlayerScoreboardId@@@std@@U?$equal_to@UPlayerScoreboardId@@@4@V?$allocator@U?$pair@$$CBUPlayerScoreboardId@@UScoreboardId@@@std@@@4@@std@@@Z", this, a2, this);
	}
};
struct ScoreboardIdentityRef {
	//bool请设置为1; a6=0 set,a6=1,add,a6=2,remove
	bool modifiedscores(Objective* obj, __int64 num, unsigned __int8 setfun) {
		int v25 = 0;
		//int nums = static_cast<int>(num);
		return SYMCALL<bool>("?modifyScoreInObjective@ScoreboardIdentityRef@@QEAA_NAEAHAEAVObjective@@HW4PlayerScoreSetFunction@@@Z",
			this, &v25, obj, num, setfun);
	}
};
struct Scoreboard {
	auto getObjective(string str) {
		return SYMCALL<Objective*>("?getObjective@Scoreboard@@QEBAPEAVObjective@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z", this, &str);
	}
	auto getScoreboardId(string* str) {
		return SYMCALL<ScoreboardId*>("?getScoreboardId@Scoreboard@@QEBAAEBUScoreboardId@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z", this, str);
	}
	vector<Objective*>* getObjectives() {
		return SYMCALL<vector<Objective*>*>("?getObjectives@Scoreboard@@QEBA?AV?$vector@PEBVObjective@@V?$allocator@PEBVObjective@@@std@@@std@@XZ", this);
	}
	auto getDisplayInfoFiltered(string* str) {
		return SYMCALL<vector<PlayerScore>*>("?getDisplayInfoFiltered@Scoreboard@@QEBA?AV?$vector@UPlayerScore@@V?$allocator@UPlayerScore@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@3@@Z", this, str);
	}
	auto getTrackedIds() {
		return SYMCALL<vector<ScoreboardId>*>("?getTrackedIds@Scoreboard@@QEBA?AV?$vector@UScoreboardId@@V?$allocator@UScoreboardId@@@std@@@std@@XZ", this);
	}
	auto getIdentityDictionary() {
		return (IdentityDictionary*)((char*)this + 80);//gouzaohanshu
	}
	auto getScoreboardId(Player* a2) {
		return SYMCALL<PlayerScoreboardId*>("?getScoreboardId@Scoreboard@@QEBAAEBUScoreboardId@@AEBVActor@@@Z", this, a2);
	}
	ScoreboardIdentityRef* getScoreboardIdentityRef(ScoreboardId* a2) {
		return SYMCALL<ScoreboardIdentityRef*>("?getScoreboardIdentityRef@Scoreboard@@QEAAPEAVScoreboardIdentityRef@@AEBUScoreboardId@@@Z", this, a2);
	}
	//mode:{0:set,1:add,2:remove}
	int modifyPlayerScore(ScoreboardId* a3, Objective* a4, int count, char mode) {
		bool a2 = true;
		return SYMCALL<int>("?modifyPlayerScore@Scoreboard@@QEAAHAEA_NAEBUScoreboardId@@AEAVObjective@@HW4PlayerScoreSetFunction@@@Z",
			this, &a2, a3, a4, count, mode);
	}
};
#pragma endregion
