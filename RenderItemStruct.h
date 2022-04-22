#pragma once

#include "Common/d3dApp.h"
#include "Common/MathHelper.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;


struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;

	/// <summary>
	/// Controls visibility
	/// </summary>
	bool shouldRender = true;
	/// <summary>
	/// This items material
	/// </summary>
	Material* material = nullptr;
	/// <summary>
	/// This items geometry
	/// </summary>
	MeshGeometry* geometry = nullptr;
	/// <summary>
	/// Objects index, should increment 
	/// per render item
	/// </summary>
	UINT objectCBIndex = -1;
	/// <summary>
	/// Position in world
	/// </summary>
	XMFLOAT4X4 position = MathHelper::Identity4x4();
	XMFLOAT4X4 texTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;
	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};
