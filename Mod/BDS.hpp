#pragma once
#include "pch.h"
using namespace std;
struct BlockLegacy {
	string getBlockName() {
		return f(string, this + 128);
	}
	short getBlockItemID() {// IDA Item::beginCreativeGroup(,Block*,) 18~22
		short v3 = f(short, this + 328);
		if (v3 < 0x100) {
			return v3;
		}
		return (short)(255 - v3);
	}
};
struct Block {
	BlockLegacy* getBlockLegacy() {
		return SYMCALL<BlockLegacy*>("?getLegacyBlock@Block@@QEBAAEBVBlockLegacy@@XZ", this);
	}
};
struct BlockPos {
	int x = 0, y = 0, z = 0;
};
struct BlockActor {
	Block* getBlock() {
		return f(Block*, this + 16);
	}
	// ȡ����λ��
	BlockPos* getPosition() {// IDA BlockActor::BlockActor 18~20
		return f(BlockPos*, this + 44);
	}
};
struct BlockSource {
	Block* getBlock(BlockPos* bp) {
		return SYMCALL<Block*>("?getBlock@BlockSource@@QEBAAEBVBlock@@AEBVBlockPos@@@Z",
			this, bp);
	}
	// ��ȡ��������ά��
	int getDimensionId() {	// IDA Dimension::onBlockChanged 42
		return f(int, (f(VA, this + 32) + 208));
	}
};
struct Level {};
struct Vec3 { float x = 0.0f, y = 0.0f, z = 0.0f; };
struct Vec2 { float x = 0.0f, y = 0.0f; };
struct MobEffectInstance { char fill[0x1C]; };
struct Item;
struct ItemStackBase {
	VA vtable;
	VA mItem;
	VA mUserData;
	VA mBlock;
	short mAuxValue;
	char mCount;
	char mValid;
	char unk[4]{};
	VA mPickupTime;
	char mShowPickUp;
	char unk2[7]{};
	vector<VA*> mCanPlaceOn;
	VA mCanPlaceOnHash;
	vector<VA*> mCanDestroy;
	VA mCanDestroyHash;
	VA mBlockingTick;
	ItemStackBase* mChargedItem;
	VA uk;

	// ȡ��ƷID,����ֵ,���
	short getId() {
		return SYMCALL<short>("?getId@ItemStackBase@@QEBAFXZ", this);
	}
	short getDamageValue() {
		return SYMCALL<short>("?getDamageValue@ItemStackBase@@QEBAFXZ", this);
	}
	// ȡ��Ʒ����
	string getName() {
		string str;
		SYMCALL<string*>("?getName@ItemStackBase@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &str);
		return str;
	}
	// ȡ����������
	int getStackCount() {// IDA ContainerModel::networkUpdateItem
		return f(int, this + 34);
	}
	// �ж��Ƿ������
	bool isNull() {
		return SYMCALL<bool>("?isNull@ItemStackBase@@QEBA_NXZ", this);
	}
	VA getNetworkUserData() {
		VA a;
		return SYMCALL<VA>("?getNetworkUserData@ItemStackBase@@QEBA?AV?$unique_ptr@VCompoundTag@@U?$default_delete@VCompoundTag@@@std@@@std@@XZ",
			this, &a);
	}
	VA save() {
		VA cp;
		return SYMCALL<VA>("?save@ItemStackBase@@QEBA?AV?$unique_ptr@VCompoundTag@@U?$default_delete@VCompoundTag@@@std@@@std@@XZ",
			this, &cp);
	}
	bool isEmptyStack() {
		return f(char, this + 34) == 0;
	}
	void fromTag(VA t) {
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
};
static_assert(sizeof(ItemStackBase) == 0x90);
struct ItemStack : ItemStackBase {};
struct Container {
	// ��ȡ������������Ʒ
	auto getSlots() {
		vector<ItemStack*> s;
		SYMCALL<VA>("?getSlots@Container@@UEBA?BV?$vector@PEBVItemStack@@V?$allocator@PEBVItemStack@@@std@@@std@@XZ",
			this, &s);
		return s;
	}
};
struct Actor {
	// ��ȡ����������Ϣ
	string getNameTag() {
		return SYMCALL<string&>("?getNameTag@Actor@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ", this);
	}
	// ��ȡ���ﵱǰ����ά��ID
	int getDimensionId() {
		int did;
		SYMCALL<int&>("?getDimensionId@Actor@@UEBA?AV?$AutomaticID@VDimension@@H@@XZ",
			this, &did);
		return did;
	}
	// ��ȡ���ﵱǰ��������
	Vec3* getPos() {
		return SYMCALL<Vec3*>("?getPos@Actor@@UEBAAEBVVec3@@XZ", this);
	}
	// �Ƿ�����
	bool isStand() {// IDA MovePlayerPacket::MovePlayerPacket 30
		return f(bool, this + 448);
	}
	// ȡ����Դ
	BlockSource* getRegion() {
		return f(BlockSource*, this + 3312);
	}
	// ��ȡʵ������
	unsigned getEntityTypeId() {
		return f(unsigned, this + 948);
	}
	// ��ȡ��ѯ��ID
	VA getUniqueID() {
		return SYMCALL<VA>("?getUniqueID@Actor@@QEBAAEBUActorUniqueID@@XZ", this);
	}
	// ��ȡʵ������
	string getEntityTypeName() {
		string en_name;
		SYMCALL<string&>("?EntityTypeToLocString@@YA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4ActorType@@W4ActorTypeNamespaceRules@@@Z",
			&en_name, getEntityTypeId());
		return en_name;
	}
	// ��������
	VA updateAttrs() {
		return SYMCALL<VA>("?_sendDirtyActorData@Actor@@QEAAXXZ", this);
	}
	// ���һ��״̬
	VA addEffect(VA ef) {
		return SYMCALL<VA>("?addEffect@Actor@@QEAAXAEBVMobEffectInstance@@@Z", this, ef);
	}
	// ��ȡ����ֵ
	pair<float, float> getHealth() {
		VA hattr = SYMCALL<VA>("?getMutableInstance@BaseAttributeMap@@QEAAPEAVAttributeInstance@@I@Z",// IDA ScriptHealthComponent::applyComponentTo line 30 132 180
			f(VA, this + 1144), f(unsigned, (VA)GetServerSymbol("?HEALTH@SharedAttributes@@2VAttribute@@B") + 4));
		return { f(float, hattr + 132), f(float, hattr + 128) };
	}
	void setHealth(float& value, float& max) {
		VA hattr = SYMCALL<VA>("?getMutableInstance@BaseAttributeMap@@QEAAPEAVAttributeInstance@@I@Z",// IDA ScriptHealthComponent::applyComponentTo line 30 132 180
			f(VA, this + 1144), f(unsigned, (VA)GetServerSymbol("?HEALTH@SharedAttributes@@2VAttribute@@B") + 4));
		f(float, hattr + 132) = value;
		f(float, hattr + 128) = max;
		SYMCALL("?_setDirty@AttributeInstance@@AEAAXXZ", hattr);
	}
};
struct Mob : Actor {
	// ��ȡ״̬�б�
	auto getEffects() {	// IDA Mob::addAdditionalSaveData 84
		return (vector<MobEffectInstance>*)((VA*)this + 190);
	}
	// ��ȡװ������
	VA getArmor() {		// IDA Mob::addAdditionalSaveData
		return VA(this) + 1400;
	}
	// ��ȡ��ͷ����
	VA getHands() {
		return VA(this) + 1408;		// IDA Mob::readAdditionalSaveData
	}
	// ���浱ǰ����������
	VA saveOffhand(VA hlist) {
		return SYMCALL<VA>("?saveOffhand@Mob@@IEBA?AV?$unique_ptr@VListTag@@U?$default_delete@VListTag@@@std@@@std@@XZ",
			this, hlist);
	}
	// ��ȡ��ͼ��Ϣ
	VA getLevel() {// IDA Mob::die 143
		return f(VA, this + 856);
	}
};
struct Player : Mob {
	// ȡuuid
	string getUuid() {// IDA ServerNetworkHandler::_createNewPlayer 217
		string p;
		SYMCALL<string&>("?asString@UUID@mce@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this + 2824, &p);
		return p;
	}
	// �������ݰ�
	void sendPacket(VA pkt) {
		return SYMCALL<void>("?sendNetworkPacket@ServerPlayer@@UEBAXAEAVPacket@@@Z",
			this, pkt);
	}
	// ���ݵ�ͼ��Ϣ��ȡ���xuid
	string& getXuid(Level* level) {
		return SYMCALL<string&>("?getPlayerXUID@Level@@QEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVUUID@mce@@@Z",
			level, this + 2720);
	}
	// ��������������
	void setName(string name) {
		SYMCALL("?setName@Player@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			this, name);
	}
	// ��ȡ�����ʶ��
	VA getNetId() {
		return (VA)this + 2536;// IDA ServerPlayer::setPermissions 34
	}
	// ��ȡ����
	Container* getContainer() {
		return (Container*)f(VA, f(VA, this + 3048) + 176);
	}
	VA getContainerManager() {
		return (VA)this + 3040;		// IDA Player::setContainerManager 18
	}
	// ��ȡĩӰ��
	VA getEnderChestContainer() {
		return SYMCALL<VA>("?getEnderChestContainer@Player@@QEAAPEAVEnderChestContainer@@XZ", this);
	}
	// ����һ��װ��
	VA setArmor(int i, ItemStack* item) {
		return SYMCALL<VA>("?setArmor@ServerPlayer@@UEAAXW4ArmorSlot@@AEBVItemStack@@@Z", this, i, item);
	}
	// ���ø���
	VA setOffhandSlot(ItemStack* item) {
		return SYMCALL<VA>("?setOffhandSlot@Player@@UEAAXAEBVItemStack@@@Z", this, item);
	}
	// ���һ����Ʒ
	void addItem(ItemStack* item) {
		(*(__int64(__fastcall**)(VA, struct ItemStack*))(*f(VA*, f(VA, this + 366) + 176) + 256))(
			f(VA, f(VA, this + 366) + 176),
			item);
		//SYMCALL<VA>("?add@Player@@UEAA_NAEAVItemStack@@@Z", this, item);
	}
	// ��ȡ��ǰѡ�еĿ�λ��
	int getSelectdItemSlot() {// IDA Player::getSelectedItem 12
		return f(unsigned, f(VA, this + 3048) + 16);
	}
	// ��ȡ��ǰ��Ʒ
	ItemStack* getSelectedItem() {
		return SYMCALL<ItemStack*>("?getSelectedItem@Player@@QEBAAEBVItemStack@@XZ", this);
	}
	// ��ȡ��Ϸʱ����Ȩ��
	char getPermission() {// IDA ServerPlayer::setPermissions 17
		return *f(char*, this + 2216);
	}
	// ������Ϸʱ����Ȩ��
	void setPermission(char m) {
		SYMCALL("?setPermissions@ServerPlayer@@UEAAXW4CommandPermissionLevel@@@Z",
			this, m);
	}
	// ��ȡ��Ϸʱ����Ȩ��
	char getPermissionLevel() {// IDA Abilities::setPlayerPermissions ?
		return f(char, f(char*, this + 2192) + 1);
	}
	// ������Ϸʱ����Ȩ��
	void setPermissionLevel(char m) {
		SYMCALL("?setPlayerPermissions@Abilities@@QEAAXW4PlayerPermissionLevel@@@Z",
			this + 2192, m);
	}
	// ����������Ʒ�б�
	void updateInventory() {
		SYMCALL<VA>("?forceBalanceTransaction@InventoryTransactionManager@@QEAAXXZ",
			this + 4592);// IDA Player::drop 65
	}
	//����
	void teleport(Vec3 target, int dim) {
		SYMCALL("?teleport@TeleportCommand@@SAXAEAVActor@@VVec3@@PEAV3@V?$AutomaticID@VDimension@@H@@VRelativeFloat@@4HAEBUActorUniqueID@@@Z",
			this, target, 0, dim, 0, 0, 0, GetServerSymbol("?INVALID_ID@ActorUniqueID@@2U1@B"));
	}
};
struct ScoreboardId {};
struct PlayerScore {
	VA getscore() {
		return f(VA, this + 4);
	}
};
struct ScoreInfo {
	//scoreboardcmd list; objective::objective; scoreboard getscores
	//int scores    +12 this[12]
	// string displayname  +96
	//string name +64
	int getCount() {
		return f(int, this + 12);
	}
};
struct Objective {
	ScoreInfo* getPlayerScore(ScoreboardId* a2) {
		char a1[12];
		return SYMCALL<ScoreInfo*>("?getPlayerScore@Objective@@QEBA?AUScoreInfo@@AEBUScoreboardId@@@Z",
			this, a1, a2);
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
	auto getScoreboardId(Player* a2) {
		return SYMCALL<ScoreboardId*>("?getScoreboardId@Scoreboard@@QEBAAEBUScoreboardId@@AEBVActor@@@Z", this, a2);
	}
	//������ҷ���
	int modifyPlayerScore(ScoreboardId* a3, Objective* a4, int count, char mode) {
		bool a2 = true;
		return SYMCALL<int>("?modifyPlayerScore@Scoreboard@@QEAAHAEA_NAEBUScoreboardId@@AEAVObjective@@HW4PlayerScoreSetFunction@@@Z",
			this, &a2, a3, a4, count, mode);
	}
};
