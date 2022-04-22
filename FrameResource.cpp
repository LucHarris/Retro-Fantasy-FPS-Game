#include "FrameResource.h"

FrameResource::FrameResource(
    ID3D12Device* device, 
    UINT passCount, 
    UINT objectCount, 
    UINT materialCount, 
    UINT bossCount, 
    UINT enemyCount, 
    UINT particleCount, 
    UINT sceneryCount)
{
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

    PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
    ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);

    GeoPointVB[GeoPointIndex::BOSS] = std::make_unique<UploadBuffer<Point>>(device, bossCount, false);
    GeoPointVB[GeoPointIndex::ENEMY] = std::make_unique<UploadBuffer<Point>>(device, particleCount, false);
    GeoPointVB[GeoPointIndex::PARTICLE] = std::make_unique<UploadBuffer<Point>>(device, particleCount, false);
    GeoPointVB[GeoPointIndex::SCENERY] = std::make_unique<UploadBuffer<Point>>(device, sceneryCount, false);
    
}

FrameResource::~FrameResource()
{

}