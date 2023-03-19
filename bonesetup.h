#pragma once

enum BoneSetupFlags {
	None = 0,
	UseInterpolatedOrigin = (1 << 0),
	UseCustomOutput = (1 << 1),
	ForceInvalidateBoneCache = (1 << 2),
	AttachmentHelper = (1 << 3),
};

class Bones {

public:
	bool m_running;
	bool m_updating_anims;

public:
	bool setup(Player* player, BoneArray* out, LagRecord* record);
	bool BuildBones(Player* target, int mask, BoneArray* out, LagRecord* record);
	bool SetupBones(Player* entity, BoneArray* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags);
	bool SetupBonesRebuild(Player* entity, BoneArray* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags);
};

extern Bones g_bones;