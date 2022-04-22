#pragma once

#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/GeometryGenerator.h"
#include "Common/Camera.h"
#include "FrameResource.h"
#include "AudioSystem.h"
#include "Weapon.h"
#include "Boss.h"
#include "Terrain.h"
#include "AmmoBox.h"
#include "Constants.h"
#include "SpriteSystem.h" // screenspace sprites and text
#include "HealthBox.h"
#include "Mob.h"
#include "RenderItemStruct.h"
#include "Sprint.h"
#include "ParticleCtrl.h"
#include "ShieldPowerup.h"
#include "SpeedPowerup.h"
#include "QuadPowerup.h"
#include "InfinitePowerup.h"
#include "Animation.h"
#include <Mouse.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

typedef DirectX::SimpleMath::Vector2 Vector2;
typedef DirectX::SimpleMath::Vector3 Vector3;

typedef DirectX::SimpleMath::Matrix Matrix;


#define TERRAIN_SHADER_TOGGLE 1

enum class GameplayState : uint32_t
{
	Start,
	Playing,
	Win,
	Lose
};
struct Countdown
{
	const float timeRange = gc::TIME_LIMIT_SECS;
	float timeLeft = gc::TIME_LIMIT_SECS;
	bool isPaused = false;
	Countdown() = default;
	void Update(float dt)
	{
		if (!isPaused)
		{
			timeLeft -= dt;
		}

		if (timeLeft < 0.0f)
		{
			timeLeft = 0.0f;
		}
	}

	bool HasTimeElpased()
	{
		return timeLeft <= 0.0f;
	}



};
// used for health and stamina
struct Stat
{
	const float maximum = 100.0f;
	float current = 100.0f;

	bool AboveZero()
	{
		return current > 0.0f;
	}

	float Normalise()
	{
		Clamp();
		return current / maximum;
	}

	Stat() = default;
	Stat(float maxValue)
		:
		maximum(maxValue),
		current(maxValue)
	{

	}
	// keep within limits
	void Clamp()
	{
		if (current > maximum)
		{
			current = maximum;
		} 
		else if (current < 0.0f)
		{
			current = 0.0f;
		}
	}

	void Update(const GameTimer& gt)
	{
		Clamp();
	}
};
enum class RenderLayer : int
{
	World = 0,
	Enemy,
	AlphaClipped,
	AmmoBox,
	HealthBox,
	ShieldBox,
	SpeedBox,
	QuadBox,
	InfiniteBox,
	UI,
	Terrain,
	PointsGS,
	Count
};

class Application : public D3DApp
{
public:
	Application(HINSTANCE hInstance);
	Application(const Application& rhs) = delete;
	Application& operator=(const Application& rhs) = delete;
	~Application();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialBuffer(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdatePoints(const GameTimer& gt);
	void LoadTexture(const std::wstring& filename, const std::string& name);
	void LoadTextures();
	void BuildAudio();

	void CreateSRV(const std::string& texName, CD3DX12_CPU_DESCRIPTOR_HANDLE& hdesc, D3D12_SRV_DIMENSION dim = D3D12_SRV_DIMENSION_TEXTURE2D);
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildTerrainGeometry();
	void BuildPointsGeometry();
	void BuildGeometry();
	void BuildEnemySpritesGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterial(int texIndex, const std::string& name, float roughness = 0.5f, const DirectX::XMFLOAT4& diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f }, const DirectX::XMFLOAT3& fresnel = { 0.05f, 0.05f, 0.05f });
	void BuildMaterials();
	std::unique_ptr<RenderItem> BuildRenderItem(UINT& objCBindex, const std::string& geoName, const std::string& subGeoName, const std::string& matName, D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	void BuildObjGeometry(const std::string& filepath, const std::string& meshName, const std::string& subMeshName);
	void Shoot();
	void UpdateEnemies(float dt);
	void SpawnEnemy(const XMFLOAT3& pos = {-5.0f,5.0f,0.0f}, const XMFLOAT3& scale = { 03.0f,03.0f,03.0f });
	void SpawnBoss(const XMFLOAT3& pos = { 0.0f,0.0f,0.0f }, const XMFLOAT3& scale = { 10.0f,10.0f,10.0f });
	void BuildEnemyObjects();
	void CheckCameraCollision();
	void PlayFootAudio(float);
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
public:
	// access textures for screen space sprites
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSpriteGpuDescHandle(const std::string& textureName);
	const UINT GetCbvSrvDescriptorSize() const;
	float GetGameTime() const;
	std::vector<std::unique_ptr<RenderItem>>* GetAllRItems()
	{
		return &mAllRitems;
	}

private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector< D3D12_INPUT_ELEMENT_DESC> mInputLayoutUi;
	// for gs processing
	std::vector< D3D12_INPUT_ELEMENT_DESC> mPointSpriteInputLayout;
	PassConstants mMainPassCB;

	Camera cam;
	Camera* mCamera = &cam;
	
	POINT mLastMousePos;

	bool fpsReady = false;

	int matIndex = 0;
	//all existing items
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	
	// data from gameobjects to be passed to geometry shader
	std::array<std::vector<Point>, GeoPointIndex::COUNT> mGeoPoints;
	// allows updated vertex data to be passed to geometry shader
	std::array <RenderItem*, GeoPointIndex::COUNT> mGeoPointsRitems;



	AudioSystem mGameAudio;

	std::vector<BoundingBox> mobBox;
	BoundingBox ammoBox[4];
	BoundingBox healthBox[4];
	BoundingBox obstBox[gc::NUM_OBSTACLE];
	AmmoBox ammoBoxClass[4];
	HealthBox healthBoxClass[4];
	BoundingBox cameraBox;

	Boss bossStats;
	Mob mobStats;
	std::vector<Mob> mobs;
	Weapon currentGun;
	TerrainParams terrainParam;
	UICharLine pointsDisplay;
	UICharLine timeDisplay;
	UICharLine ammoDisplay;
	UISprite spriteCtrl[gc::NUM_UI_SPRITES];
	UISprite wordCtrl[gc::UI_NUM_RITEM_WORD];
	float mAudioVolume = 0.3f;
	float footStepTimer = 0.0f;
	float footStepInterval = 0.4f;
	float COOLDOWN = 1;
	bool initialSpawn = true;


	BoundingBox shieldBox[4];
	BoundingBox speedBox[4];
	BoundingBox quadBox[4];
	BoundingBox infiniteBox[4];
	ShieldPowerup shieldBoxClass[4];
	SpeedPowerup speedBoxClass[4];
	QuadPowerup quadBoxClass[4];
	InfinitePowerup infiniteBoxClass[4];
	float MAX_POWERUPS = 4.f;
	bool shieldActive = false;
	bool speedActive = false;
	bool quadActive = false;
	bool infiniteActive = false;
	float shieldTimer;
	float speedTimer;
	float quadTimer;
	float infiniteTimer;

	GameplayState gameplayState = GameplayState::Start;

	float tempMaxHealth = 100.0f;
	float tempCurrentHealth = 100.0f;

	Stat playerHealth;
	Stat playerStamina;
	Stat bossHealth;

	Sprint sprint;

	Countdown countdown;
	ParticleSys particleCtrl;

	Animation tempAnim;
	uint32_t enemySpawnIndex = 0;

	std::unique_ptr<Mouse> mouse;

	std::vector<Animation> mobAnims;
	Animation bossAnim;
	std::vector<Animation> particleAnims;
	float points = 0.0f;
};