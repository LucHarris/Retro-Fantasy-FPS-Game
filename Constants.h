#pragma once

#include <string>
#include <SimpleMath.h>


struct ObstacleData
{
	std::string filename;
	std::string geoName;
	std::string subGeoName;
	DirectX::SimpleMath::Vector3 boundingBox;
};


struct ObjData
{
	std::string filename;
	std::string geoName;
	std::string subGeoName;
	DirectX::SimpleMath::Vector3 position;
};

struct NoramlisedAnimData
{
	// left, top, width, height
	DirectX::SimpleMath::Vector4 initFloatRect;
	// normalised value incremented between frames
	float frameOffset;
	// total number of frames in animation
	int numFrames;
	bool loop = true;
	bool animating = true;
};

namespace gc
{
	const size_t NUM_OBSTACLE = 4;
	const size_t NUM_UI_SPRITES = 12;

	const ObstacleData OBSTACLE_DATA[NUM_OBSTACLE]
	{
		"Data/Models/obst_02_10_06.obj", "obst00Geo", "obst00",{02.0f,10.0f,06.0f},
		"Data/Models/obst_06_10_06.obj", "obst01Geo", "obst01",{06.0f,10.0f,06.0f},
		"Data/Models/obst_08_10_08.obj", "obst02Geo", "obst02",{08.0f,10.0f,08.0f},
		"Data/Models/obst_10_17_06.obj", "obst03Geo", "obst03",{10.0f,17.0f,06.0f}
	};

	enum {
		CHAR_0 = 0, CHAR_1, CHAR_2, CHAR_3, CHAR_4, CHAR_5, CHAR_6, CHAR_7, CHAR_8, CHAR_9,
		CHAR_SPC, CHAR_PRD, CHAR_TIME, CHAR_AMM0, CHAR_PTS, CHAR_SEC, CHAR_COLON, CHAR_UNUSED2,
		CHAR_UNUSED3, CHAR_UNUSED4,
		CHAR_COUNT,

		WORD_HEALTH = 0,
		WORD_AMMO,
		WORD_MONSTER,
		WORD_TIMELEFT,
		WORD_COUNT,

		SPRITE_CONTROLS = 0,
		SPRITE_LOSE,
		SPRITE_WIN,
		SPRITE_HEALTH_PLAYER_GRN,
		SPRITE_HEALTH_PLAYER_RED,
		SPRITE_HEALTH_BOSS_GRN,
		SPRITE_HEALTH_BOSS_RED,
		SPRITE_STAMINA_PLAYER_YLW,
		SPRITE_STAMINA_PLAYER_GRY,
		SPRITE_OBJECTIVE,
		SPRITE_CROSSHAIR,
		SPRITE_ORB,
		SPRITE_COUNT

	};

	// If order changed update enum above
	// pos -1 to 1 in xy
	const ObjData UI_SPRITE_DATA[NUM_UI_SPRITES]
	{
		"Data/Models/UI_Controls.obj", "uiControlGeo", "uiControl",					{00.40f,-0.80f,01.00f},
		"Data/Models/UI_FinWin.obj", "uiFinWinGeo", "uiFinWin",						{00.00f,00.00f,00.00f},
		"Data/Models/UI_FinLose.obj", "uiFinLoseGeo", "uiFinLose",					{00.00f,00.00f,00.00f},

		"Data/Models/UI_BarGreen.obj", "uiHealthPlayerGrnGeo", "uiHealthPlayerGrn",	{00.00f,00.90f,00.00f}, // todo make green, scale based on remaining health
		"Data/Models/UI_BarRed.obj", "uiHealthPlayerRedGeo", "uiHealthPlayerRed",	{00.00f,00.90f,00.00f}, // todo make red
		
		"Data/Models/UI_BarPurple.obj", "uiHealthBossGrnGeo", "uiHealthGrnBoss",	{00.00f,00.70f,00.00f}, // todo make green, scale based on remaining health
		"Data/Models/UI_BarRed.obj", "uiHealthBossRedGeo", "uiHealthRedBoss",		{00.00f,00.70f,00.00f}, // todo make green, scale based on remaining health
		
		"Data/Models/UI_BarYellow.obj", "uiStaminaGryGeo", "uiStaminaGry",			{00.00f,00.80f,00.00f}, // todo make green, scale based on remaining health
		"Data/Models/UI_BarGrey.obj", "uiStaminaYlwGeo", "uiStaminaYlw",			{00.00f,00.80f,00.00f}, // todo make green, scale based on remaining health
		
		"Data/Models/UI_Objective.obj", "uiObjectiveGeo", "uiObjective",			{00.00f,00.00f,00.00f},
		"Data/Models/UI_CrossHair.obj", "uiCrossHairGeo", "uiCrossHair",			{00.00f,00.00f,00.00f},
		"Data/Models/UI_Orb.obj", "uiOrbGeo", "uiOrb",								{00.10f,-1.00f,00.00f}
	};
	// for initialising sprites
	const bool UI_SPRITE_DEFAULT_DISPLAY[NUM_UI_SPRITES]
	{
		true,
		false,
		false,
		true,
		true,
		true,
		true,
		true,
		true,
		true,
		true,
		true
	};

	// characters available in texture 
	const int UI_NUM_CHAR = 20;
	// words available in texture
	const int UI_NUM_WORD = 20;

	const size_t UI_LINE_1_LEN = 8;
	const size_t UI_LINE_2_LEN = 8;
	const size_t UI_LINE_3_LEN = 8;
	// number of chars render items
	const size_t UI_NUM_RITEM_CHAR = UI_LINE_3_LEN + UI_LINE_1_LEN + UI_LINE_2_LEN;
	// number of words render items
	const size_t UI_NUM_RITEM_WORD = 6;

	// increment distance between chars
	const float UI_CHAR_SPACING = 0.07f;
	// uv increment in y texture axis
	const float UI_CHAR_INC = 1.0f / (float)UI_NUM_CHAR;
	const float UI_WORD_INC = 1.0f / (float)UI_NUM_WORD;

	const ObjData UI_CHAR = { "Data/Models/UI_Char.obj", "uiCharGeo", "uiChar",		{-0.50f,00.50f,00.0f} };
	const ObjData UI_WORD = { "Data/Models/UI_Word.obj", "uiWordGeo", "uiWord",		{-0.80f,-0.90f,00.0f} };


	const DirectX::SimpleMath::Vector3 UI_POINTS_POS{	-0.90f,-0.75f,00.0f };
	const DirectX::SimpleMath::Vector3 UI_TIME_POS{		-0.90f,-0.85f,00.0f };
	const DirectX::SimpleMath::Vector3 UI_AMMO_POS{		-0.90f,-0.95f,00.0f };

	const DirectX::SimpleMath::Vector3 UI_OFF_POS{ 100.0f,100.0f,00.0f };

	// adapt if frame rsc geoPointVB changes

	// number of points per geometry passed to gs
	// update Application:: 	std::array<BoundingBox, 32> mobBox; to match  NUM_ENEMIES
	const uint32_t NUM_GEO_POINTS[4]
	{
		1u, //NUM_BOSS
		32U, //NUM_ENEMIES 
		100U,//NUM_PARTICLES
		32U  //NUM_SCENERY
	};


	// no filenames
	const ObjData GEO_POINT_NAME[4] = {
		{ "", "bossPointGeo", "bossPoint", { 0.00f,00.00f,00.0f } },
		{ "", "enemyPointGeo", "enemyPoint", { 0.00f,00.00f,00.0f } },
		{ "", "particlePointGeo", "particlePoint", { 0.00f,00.00f,00.0f } },
		{ "", "SceneryPointGeo", "SceneryPoint", { 0.00f,00.00f,00.0f } }
	
	};



	const DirectX::SimpleMath::Vector3 UI_WORD_INIT_POSITION[UI_NUM_RITEM_WORD]
	{
		{-0.40f,00.95f,00.0f }, // HEALTH
		{-9.00f,00.00f,00.0f }, // AMMO
		{-0.40f,00.75f,00.0f }, // MONSTER
		{-9.00f,00.00f,00.0f }, // TIME LEFT
		{-0.40f,00.85f,00.0f }, // STAMINA
		{-9.00f,00.00f,00.0f }, // BOSS
	};



	const float TIME_LIMIT_SECS = 60.0f;

	const float ANIM_FRAME_TIME = 0.2f;


	enum AnimIndex {
		ENEMY_IDLE,
		BOSS_IDLE,
		PARTICLE_PURPLE,
		PARTICLE_BLUE,
		COUNT
	};

	const float ANIM_TEXTURE_DIM = 1.0f / 1024.0f;

	// left, top, width, height
	const NoramlisedAnimData ANIM_DATA[AnimIndex::COUNT]
	{
		// * ANIM_TEXTURE_DIM to normalies texcoord and offset for uv
		// initial float rect(l,t,w,h), offset, numFrames, loop, animating
		{ DirectX::SimpleMath::Vector4(0000.0f,0128.0f,0032.0f,0032.0f) * ANIM_TEXTURE_DIM,	0032.0f * ANIM_TEXTURE_DIM,		2,	true,	true	},
		{ DirectX::SimpleMath::Vector4(0000.0f,0000.0f,0064.0f,0064.0f) * ANIM_TEXTURE_DIM,	0064.0f * ANIM_TEXTURE_DIM,		6,	true,	true	},
		{ DirectX::SimpleMath::Vector4(0000.0f,0160.0f,0008.0f,0008.0f) * ANIM_TEXTURE_DIM,	0008.0f * ANIM_TEXTURE_DIM,		4,	true,	true	},
		{ DirectX::SimpleMath::Vector4(0000.0f,0168.0f,0000.0f,0000.0f) * ANIM_TEXTURE_DIM,	0008.0f * ANIM_TEXTURE_DIM,		2,	true,	true	},
	};

	const int BOSS_MAX_HEALTH = 100;
	//float COOLDOWN = 1;

	const float POINTS_MULTI = 0.3f;
}
