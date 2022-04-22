#pragma once

#include "Common/d3dUtil.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include <SimpleMath.h>

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT     MaterialIndex;
	UINT     ObjPad0;
	UINT     ObjPad1;
	UINT     ObjPad2;
};

struct Shockwave
{
    // origin
    DirectX::XMFLOAT3 Pos = {0.0f,0.0f,0.0f};
    // from pos. rig to timer
    float Raduis = 0.0f;
    // from radius
    float Width = 5.0f;
    // on normal
    float Strength = 2.0f;
    // radius multiplier
    float Speed = 20.0f;
    // speed and strength
    float Drag = 0.9999999f;

    void Update(float dt)
    {
        Raduis += Speed * dt;
    }
    void Stop()
    {
        Raduis = 0.0f;
        Strength = 0.0f;
        Width = 0.0f;
        Speed = 0.0f;
    }
    void Reset(DirectX::XMFLOAT3 pos, float strength = 2.0f,float speed = 20.0f, float width = 5.0f, float drag = 0.99f)
    {
        Raduis = 0.0f;
        Pos = pos;
        Strength = strength;
        Width = width;
        Speed = speed;
        Drag = max(min(drag,1.0f),0.0f);
    }
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

    DirectX::XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
    float gFogStart = 5.0f;
    float gFogRange = 150.0f;
    DirectX::XMFLOAT2 cbPerObjectPad2;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light Lights[MaxLights];

    // 96 remaining 4byte elements. removed 8 lights. 8*sizeof(Light) == 8 * 12 == 96
    Shockwave Shockwaves[1];

    // 96 - sizeof(Shockwave) == 88
    float TimeLimit;
    float TimeLeft;
    // 88 - 2 = 86
    float Padding[86]; // 
};

struct MaterialData
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 64.0f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	UINT DiffuseMapIndex = 0;
	UINT MaterialPad0;
	UINT MaterialPad1;
	UINT MaterialPad2;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
};

// implemented in gs
enum class BillboardType : int {
    POINT_ORIENTATION,  // point at cam
    AXIS_ORIENTATION,   // point at cam about Y axis
    FIXED_SINGLE,       // single sided 
    FIXED_DOUBLE,       // double sided
    FIXED_CROSS,         // 2x double sided perpendicular for 3D effect
    NONE, 
};
// point structure for gs
struct Point
{
    // position in world space 
    DirectX::XMFLOAT3 Pos = DirectX::XMFLOAT3(0.0f, 0.0f,0.0f);
    // width and height of sprite
    DirectX::XMFLOAT2 Size = DirectX::XMFLOAT2(1.0f,1.0f);
    // normalised uv coordinates
    // left,top,width,height
    DirectX::SimpleMath::Vector4 TexRect = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
    // set to none to disable rendering of sprite
    BillboardType Billboard = BillboardType::NONE;
};

enum GeoPointIndex
{
    BOSS,
    ENEMY,
    PARTICLE,
    SCENERY,
    COUNT
};

// Stores the resources needed for the CPU to build the command lists
// for a frame.  
struct FrameResource
{
public:
    // todo remove pointsCOunt
    FrameResource(
        ID3D12Device* device, 
        UINT passCount, 
        UINT objectCount, 
        UINT materialCount, 
        UINT bossCount, 
        UINT enemyCount, 
        UINT particleCount, 
        UINT sceneryCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    // We cannot reset the allocator until the GPU is done processing the commands.
    // So each frame needs their own allocator.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // We cannot update a cbuffer until the GPU is done processing the commands
    // that reference it.  So each frame needs their own cbuffers.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

    // geometery shader objects
    // pass data to this in app::update()
    std::unique_ptr<UploadBuffer<Point>> GeoPointVB[GeoPointIndex::COUNT] = { nullptr,nullptr,nullptr,nullptr };


    // Fence value to mark commands up to this fence point.  This lets us
    // check if these frame resources are still in use by the GPU.
    UINT64 Fence = 0;
};