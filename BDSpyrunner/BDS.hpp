#pragma once
#include "bdxcore.h"
#define f(type,ptr) (*(type*)(ptr))
using namespace std;
#pragma region Block
struct BlockLegacy {
	string getBlockName() {
		return f(string, this + 128);
	}
	short getBlockItemID() {	// IDA VanillaItems::initCreativeItemsCallback Item::beginCreativeGroup "itemGroup.name.planks"2533 line
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
	BlockPos* getPosition() {				// IDA BlockActor::BlockActor
		return f(BlockPos*, this + 44);
	}
};
struct BlockSource {
	Block* getBlock(BlockPos* bp) {
		return SYMCALL<Block*>("?getBlock@BlockSource@@QEBAAEBVBlock@@AEBVBlockPos@@@Z",
			this, bp);
	}
	// ��ȡ��������ά��
	int getDimensionId() {	// IDA Dimension::onBlockChanged
		return f(int, (f(VA, this + 32) + 200));
	}
};
#pragma endregion
struct Level {};
struct Vec3 { float x = 0.0f, y = 0.0f, z = 0.0f; };
struct Vec2 { float x = 0.0f, y = 0.0f; };
struct MobEffectInstance { char fill[0x1C]; };
struct CompoundTag {
	string toString() {
		string a;
		return SYMCALL<string&>("?toString@CompoundTag@@UEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &a);
	}
};
struct Item;
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

	//ItemStackBase() { memcpy(this, GetServerSymbol("?EMPTY_ITEM@ItemStack@@2V1@B"), 0x90); }
	// ȡ��ƷID
	short getId() {
		return SYMCALL<short>("?getId@ItemStackBase@@QEBAFXZ", this);
	}
	// ȡ��Ʒ����ֵ
	short getAuxValue() {
		return SYMCALL<short>("?getAuxValue@ItemStackBase@@QEBAFXZ", this);
	}
	// ȡ��Ʒ����
	string getName() {
		string str;
		SYMCALL<__int64>("?getName@ItemStackBase@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this, &str);
		return str;
	}
	// ȡ����������
	int getStackSize() {			// IDA ContainerModel::networkUpdateItem
		return f(int, this + 34);
	}
	// �ж��Ƿ������
	bool isNull() {
		return SYMCALL<bool>("?isNull@ItemStackBase@@QEBA_NXZ", this);
	}
	CompoundTag* getNetworkUserData() {
		void* a;
		return SYMCALL<CompoundTag*>("?getNetworkUserData@ItemStackBase@@QEBA?AV?$unique_ptr@VCompoundTag@@U?$default_delete@VCompoundTag@@@std@@@std@@XZ",
			this, &a);
	}
	CompoundTag* save() {
		VA* cp = new VA[8]{ 0 };
		return SYMCALL<CompoundTag*>("?save@ItemStackBase@@QEBA?AV?$unique_ptr@VCompoundTag@@U?$default_delete@VCompoundTag@@@std@@@std@@XZ",
			this, cp);
	}
	bool isEmptyStack() {
		return f(char, this + 34) == 0;
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
struct ItemStack : ItemStackBase {};
struct Container {
	// ��ȡ������������Ʒ
	auto getSlots() {
		vector<ItemStack*> s;
		return SYMCALL<vector<ItemStack*>*>("?getSlots@Container@@UEBA?BV?$vector@PEBVItemStack@@V?$allocator@PEBVItemStack@@@std@@@std@@XZ",
			this, &s);
	}
};
#pragma region Actor
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
	bool isStand() {// IDA MovePlayerPacket::MovePlayerPacket
		return f(bool, this + 416);
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
		VA bpattrmap = f(VA, this + 135);// IDA ScriptHealthComponent::retrieveComponentFrom
		if (bpattrmap) {
			VA hattr = SYMCALL<VA>("?getMutableInstance@BaseAttributeMap@@QEAAPEAVAttributeInstance@@I@Z",
				bpattrmap, 7);//SYM_OBJECT(UINT32, 0x019700B8 + 4));// SharedAttributes::HEALTH
			if (hattr) {
				return { f(float, hattr + 33), f(float, hattr + 32) };
			}
		}
		return { 0.0f,0.0f };
	}
	bool setHealth(float& value, float& max) {
		VA bpattrmap = ((VA*)this)[135];
		if (bpattrmap) {
			VA hattr = SYMCALL<VA>("?getMutableInstance@BaseAttributeMap@@QEAAPEAVAttributeInstance@@I@Z",
				bpattrmap, 7);// SYM_OBJECT(UINT32, 0x019700B8 + 4));// SharedAttributes::HEALTH
			if (hattr) {
				f(float, hattr + 33) = value;
				f(float, hattr + 32) = max;
				SYMCALL<VA>("?_setDirty@AttributeInstance@@AEAAXXZ", hattr);
				return true;
			}
		}
		return false;
	}
};
struct Mob : Actor {
	// ��ȡ״̬�б�
	auto getEffects() {	// IDA Mob::addAdditionalSaveData
		return (vector<MobEffectInstance>*)((VA*)this + 152);
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
	VA getLevel() {					// IDA Mob::die
		return f(VA, this + 816);
	}
};
struct Player : Mob {
	// ȡuuid
	string getUuid() {	// IDA ServerNetworkHandler::_createNewPlayer
		string p;
		SYMCALL<string&>("?asString@UUID@mce@@QEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",
			this + 2720, &p);
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
		SYMCALL<void>("?setName@Player@@UEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
			this, name);
	}
	// ��ȡ�����ʶ��
	VA getNetId() {
		return (VA)this + 2432;// IDA ServerPlayer::setPermissions
	}
	// ��ȡ����
	Container* getContainer() {
		return (Container*)f(VA, f(VA, this + 366) + 176);
	}
	VA getContainerManager() {
		return (VA)this + 2912;		// IDA Player::setContainerManager
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
	int getSelectdItemSlot() {			// IDA Player::getSelectedItem
		return f(unsigned, f(VA, this + 366) + 16);
	}
	// ��ȡ��ǰ��Ʒ
	ItemStack* getSelectedItem() {
		return SYMCALL<ItemStack*>("?getSelectedItem@Player@@QEBAAEBVItemStack@@XZ", this);
	}
	// ��ȡ��Ϸʱ����Ȩ��
	char getPermission() {						// IDA ServerPlayer::setPermissions
		return *f(char*, this + 2112);
	}
	// ������Ϸʱ����Ȩ��
	void setPermission(char m) {
		SYMCALL<void>("?setPermissions@ServerPlayer@@UEAAXW4CommandPermissionLevel@@@Z",
			this, m);
	}
	// ��ȡ��Ϸʱ����Ȩ��
	char getPermissionLevel() {		// IDA Abilities::setPlayerPermissions
		return f(char, f(char*, this + 2192) + 1);
	}
	// ������Ϸʱ����Ȩ��
	void setPermissionLevel(char m) {
		SYMCALL<void>("?setPlayerPermissions@Abilities@@QEAAXW4PlayerPermissionLevel@@@Z",
			this + 2192, m);
	}
	// ����������Ʒ�б�
	void updateInventory() {
		SYMCALL<VA>("?forceBalanceTransaction@InventoryTransactionManager@@QEAAXXZ",
			this + 4472);// IDA Player::drop
	}
	//����
	void teleport(Vec3 target, int dim) {
		SYMCALL<void>("?teleport@TeleportCommand@@SAXAEAVActor@@VVec3@@PEAV3@V?$AutomaticID@VDimension@@H@@VRelativeFloat@@4HAEBUActorUniqueID@@@Z",
			this, target, 0, dim, 0, 0, 0, GetServerSymbol("?INVALID_ID@ActorUniqueID@@2U1@B"));
	}
};
#pragma endregion
#pragma region �Ʒְ�
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
struct ScoreboardIdentityRef {
	//bool������Ϊ1; a6=0 set,a6=1,add,a6=2,remove
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
	auto getScoreboardId(Player* a2) {
		return SYMCALL<ScoreboardId*>("?getScoreboardId@Scoreboard@@QEBAAEBUScoreboardId@@AEBVActor@@@Z", this, a2);
	}
	auto getScoreboardIdentityRef(ScoreboardId* a2) {
		return SYMCALL<ScoreboardIdentityRef*>("?getScoreboardIdentityRef@Scoreboard@@QEAAPEAVScoreboardIdentityRef@@AEBUScoreboardId@@@Z", this, a2);
	}
	//������ҷ���
	int modifyPlayerScore(ScoreboardId* a3, Objective* a4, int count, char mode) {
		bool a2 = true;
		return SYMCALL<int>("?modifyPlayerScore@Scoreboard@@QEAAHAEA_NAEBUScoreboardId@@AEAVObjective@@HW4PlayerScoreSetFunction@@@Z",
			this, &a2, a3, a4, count, mode);
	}
};
#pragma endregion
