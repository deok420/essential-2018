#include "includes.h"

Bones g_bones{};;

bool Bones::setup(Player* player, BoneArray* out, LagRecord* record) {
	return true;
}

bool Bones::BuildBones(Player* target, int mask, BoneArray* out, LagRecord* record) {
	vec3_t		     pos[128];
	quaternion_t     q[128];
	vec3_t           backup_origin;
	ang_t            backup_angles;
	float            backup_poses[24];
	C_AnimationLayer backup_layers[13];

	// get hdr.
	CStudioHdr* hdr = target->GetModelPtr();
	if (!hdr)
		return false;

	// get ptr to bone accessor.
	CBoneAccessor* accessor = &target->m_BoneAccessor();
	if (!accessor)
		return false;

	// store origial output matrix.
	// likely cachedbonedata.
	auto backup_matrix = accessor->m_pBones;
	if (!backup_matrix)
		return false;

	// backup original.
	backup_origin = target->GetAbsOrigin();
	backup_angles = target->GetAbsAngles();
	target->GetPoseParameters(backup_poses);
	target->GetAnimLayers(backup_layers);

	// compute transform from raw data.
	matrix3x4_t transform;
	math::AngleMatrix(record->m_abs_ang, record->m_pred_origin, transform);

	// set non interpolated data.
	target->AddEffect(EF_NOINTERP);
	target->AddEffect(EFL_SETTING_UP_BONES);
	target->SetAbsOrigin(record->m_origin);
	target->SetAbsAngles(record->m_abs_ang);
	target->SetPoseParameters(record->m_poses);
	target->SetAnimLayers(record->m_server_layers);

	// force game to call AccumulateLayers - pvs fix.
	m_running = true;

	// set bone array for write.
	accessor->m_pBones = out;

	// compute and build bones.
	target->StandardBlendingRules(hdr, pos, q, record->m_pred_time, mask);

	uint8_t computed[0x100];
	std::memset(computed, 0, 0x100);
	target->BuildTransformations(hdr, pos, q, transform, mask, computed);

	// restore old matrix.
	accessor->m_pBones = backup_matrix;

	// restore original interpolated entity data.
	target->RemoveEffect(EF_NOINTERP);
	target->RemoveEffect(EFL_SETTING_UP_BONES);
	target->SetAbsOrigin(backup_origin);
	target->SetAbsAngles(backup_angles);
	target->SetPoseParameters(backup_poses);
	target->SetAnimLayers(backup_layers);

	// revert to old game behavior.
	m_running = false;

	return true;
}

bool Bones::SetupBones(Player* entity, BoneArray* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags) {

	float m_flRealtime = g_csgo.m_globals->m_realtime;
	float m_flCurtime = g_csgo.m_globals->m_curtime;
	float m_flFrametime = g_csgo.m_globals->m_frametime;
	float m_flAbsFrametime = g_csgo.m_globals->m_abs_frametime;
	int m_iFramecount = g_csgo.m_globals->m_frame;
	int m_iTickcount = g_csgo.m_globals->m_tick_count;

	// fixes for networked players
	float m_flTime = entity->m_flSimulationTime();
	g_csgo.m_globals->m_realtime = m_flTime;
	g_csgo.m_globals->m_curtime = m_flTime;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_abs_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_frame = game::TIME_TO_TICKS(m_flTime);
	g_csgo.m_globals->m_tick_count = game::TIME_TO_TICKS(m_flTime);

	bool result = SetupBonesRebuild(entity, pBoneMatrix, nBoneCount, boneMask, time, flags);

	// restore globals
	g_csgo.m_globals->m_realtime = m_flRealtime;
	g_csgo.m_globals->m_curtime = m_flCurtime;
	g_csgo.m_globals->m_frametime = m_flFrametime;
	g_csgo.m_globals->m_abs_frametime = m_flAbsFrametime;
	g_csgo.m_globals->m_frame = m_iFramecount;
	g_csgo.m_globals->m_tick_count = m_iTickcount;

	return result;
}

bool Bones::SetupBonesRebuild(Player* entity, BoneArray* pBoneMatrix, int nBoneCount, int boneMask, float time, int flags) {

	if (entity->m_nSequence() == -1)
		return false;

	if (boneMask == -1)
		boneMask = entity->m_iPrevBoneMask();

	boneMask = boneMask | 0x80000;

	int nLOD = 0;
	int nMask = BONE_USED_BY_VERTEX_LOD0;

	for (; nLOD < 8; ++nLOD, nMask <<= 1) {
		if (boneMask & nMask)	
			break;
	}

	for (; nLOD < 8; ++nLOD, nMask <<= 1) {
		boneMask |= nMask;
	}

	auto model_bone_counter = g_csgo.InvalidateBoneCache.add(0x000A).to< size_t >();

	CBoneAccessor backup_bone_accessor = entity->m_BoneAccessor();
	CBoneAccessor* bone_accessor = &entity->m_BoneAccessor();

	if (!bone_accessor)
		return false;

	if (entity->m_iMostRecentModelBoneCounter() != model_bone_counter || (flags & BoneSetupFlags::ForceInvalidateBoneCache)) {
		if (FLT_MAX >= entity->m_flLastBoneSetupTime() || time < entity->m_flLastBoneSetupTime()) {
			bone_accessor->m_ReadableBones = 0;
			bone_accessor->m_WritableBones = 0;
			entity->m_flLastBoneSetupTime() = (time);
		}

		entity->m_iPrevBoneMask() = entity->m_iAccumulatedBoneMask();
		entity->m_iAccumulatedBoneMask() = 0;

		auto hdr = entity->GetModelPtr();
		if (hdr) { // profiler stuff
			((CStudioHdrEx*)hdr)->m_nPerfAnimatedBones = 0;
			((CStudioHdrEx*)hdr)->m_nPerfUsedBones = 0;
			((CStudioHdrEx*)hdr)->m_nPerfAnimationLayers = 0;
		}
	}

	entity->m_iAccumulatedBoneMask() |= boneMask;
	entity->m_iOcclusionFramecount() = 0;
	entity->m_iOcclusionFlags() = 0;
	entity->m_iMostRecentModelBoneCounter() = model_bone_counter;

	bool bReturnCustomMatrix = (flags & BoneSetupFlags::UseCustomOutput) && pBoneMatrix;

	CStudioHdr* hdr = entity->GetModelPtr();

	if (!hdr)
		return false;

	vec3_t origin = (flags & BoneSetupFlags::UseInterpolatedOrigin) ? entity->GetAbsOrigin() : entity->m_vecOrigin();
	ang_t angles = entity->GetAbsAngles();

	alignas(16) matrix3x4_t parentTransform;
	math::AngleMatrix(angles, origin, parentTransform);

	boneMask |= entity->m_iPrevBoneMask();

	if (bReturnCustomMatrix)
		bone_accessor->m_pBones = pBoneMatrix;

	int oldReadableBones = bone_accessor->m_ReadableBones;
	int oldWritableBones = bone_accessor->m_WritableBones;

	int newWritableBones = oldReadableBones | boneMask;

	bone_accessor->m_WritableBones = newWritableBones;
	bone_accessor->m_ReadableBones = newWritableBones;

	if (!(hdr->pStudioHdr->m_flags & 0x00000010)) {

		entity->m_fEffects() |= EF_NOINTERP;
		entity->m_iEFlags() |= EFL_SETTING_UP_BONES;

		entity->m_pIK() = 0;
		entity->m_ClientEntEffects() |= 2;

		alignas(16) vec3_t pos[128];
		alignas(16) quaternion_t q[128];

		entity->StandardBlendingRules(hdr, pos, q, time, boneMask);

		uint8_t computed[0x100];
		std::memset(computed, 0, 0x100);

		entity->BuildTransformations(hdr, pos, q, parentTransform, boneMask, computed);

		entity->m_iEFlags() &= ~EFL_SETTING_UP_BONES;

		// lol.
		// entity->m_fEffects() &= ~EF_NOINTERP;

	}
	else
		parentTransform = bone_accessor->m_pBones[0];

	if (flags & BoneSetupFlags::AttachmentHelper)
		entity->attachments_helper(entity->GetModelPtr());

	if (bReturnCustomMatrix) {
		*bone_accessor = backup_bone_accessor;
		return true;
	}

	return true;
}
