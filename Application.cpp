#include "Application.h"
#include "OBJ_Loader.h"
#include "Util.h"

#define OBSTACLE_TOGGLE 1
#define UI_SPRITE_TOGGLE 1
#define GS_TOGGLE 1
#define POINTS_SHADER_INPUT 1


const int gNumFrameResources = 3;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Application theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

Application::Application(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

Application::~Application()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

void DebugMsg(const std::wstring& wstr)
{
	OutputDebugString(wstr.c_str());
}

bool Application::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mCamera->LookAt(
		XMFLOAT3(5.0f, 4.0f, -15.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f));

	for (int i = 0; i < 4; i++)
	{
		ammoBoxClass[i] = AmmoBox(10 * i, 10*i);
		healthBoxClass[i] = HealthBox(10 * i, 10*i);
	}
	for (int i = 0; i < MAX_POWERUPS; ++i)
	{
		shieldBoxClass[i] = ShieldPowerup(20, 20*(i+1));
		speedBoxClass[i] = SpeedPowerup(20, 3, 20*(i+1));
		quadBoxClass[i] = QuadPowerup(20, 4, 20 *(i+1));
		infiniteBoxClass[i] = InfinitePowerup(15, 20 *(i+1));
	}

	BuildAudio();
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildTerrainGeometry();
	BuildGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildEnemyObjects();
	SpawnBoss();
	SpawnEnemy();
	SpawnEnemy({0,0,0});
	BuildFrameResources();
	BuildPSOs();

	cameraBox = BoundingBox(mCamera->GetPosition3f(), XMFLOAT3(1, 1, 1));
	mCamera->LookAt(
		XMFLOAT3(5.0f, 4.0f, -15.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f));
	currentGun.Setup("Pistol", 1, 7);

	cameraBox = BoundingBox(mCamera->GetPosition3f(), XMFLOAT3(1, 1, 1));

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void Application::OnResize()
{
	D3DApp::OnResize();

	mCamera->SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void Application::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}


	if (!playerHealth.AboveZero() || countdown.HasTimeElpased())
	{
		spriteCtrl[gc::SPRITE_LOSE].SetDisplay(this, true);
		spriteCtrl[gc::SPRITE_CROSSHAIR].SetDisplay(this, false);
	}

	if (bossStats.DealDamage(0))
	{
		spriteCtrl[gc::SPRITE_WIN].SetDisplay(this, true);
		spriteCtrl[gc::SPRITE_CROSSHAIR].SetDisplay(this, false);
	}



	countdown.Update(gt.DeltaTime());

	playerHealth.Update(gt);
	playerStamina.Update(gt);
	bossHealth.Update(gt);
	
	particleCtrl.Update(&mGeoPoints.at(GeoPointIndex::PARTICLE), gt.DeltaTime());
	spriteCtrl[gc::SPRITE_HEALTH_PLAYER_GRN].SetXScale(this,max(playerHealth.Normalise(),0.0f),gt.DeltaTime());
	spriteCtrl[gc::SPRITE_HEALTH_BOSS_GRN].SetXScale(this, max((float)bossStats.hp / (float)gc::BOSS_MAX_HEALTH,0.0f),gt.DeltaTime());
	spriteCtrl[gc::SPRITE_STAMINA_PLAYER_YLW].SetXScale(this,max(playerStamina.Normalise(),0.0f),gt.DeltaTime());

	pointsDisplay.Update(this, gt.DeltaTime(), points);
	timeDisplay.Update(this, gt.DeltaTime(), countdown.timeLeft);
	ammoDisplay.Update(this, gt.DeltaTime(), currentGun.GetLoadedAmmo());

	if (gt.TotalTime() > 5.0f)
	{
		spriteCtrl[gc::SPRITE_OBJECTIVE].SetDisplay(this, false);
	}

	mGameAudio.Update(mTimer.DeltaTime(), mCamera->GetPosition3f(), mCamera->GetLook3f(), mCamera->GetUp3f());

	AnimateMaterials(gt);


	for (size_t i = 0; i < gc::NUM_GEO_POINTS[GeoPointIndex::PARTICLE]; i++)
	{
		particleAnims.at(i).Update(gt.DeltaTime());
	}

	UpdateEnemies(gt.DeltaTime());

	UpdateObjectCBs(gt);
	UpdateMaterialBuffer(gt);
	UpdateMainPassCB(gt);
	UpdatePoints(gt);


	int checkIndex = 0;
	for (auto ri : mRitemLayer[(int)RenderLayer::AmmoBox])
	{
		ammoBoxClass[checkIndex].Update(gt.DeltaTime());
		if (ammoBoxClass[checkIndex].hasBeenConsumed == false && ammoBoxClass[checkIndex].hasSpawnedIn == true)
		{
			ri->shouldRender = true;
		}
		else ri->shouldRender = false;
		
		checkIndex++;
	}
	checkIndex = 0;
	for (auto ri : mRitemLayer[(int)RenderLayer::HealthBox])
	{
		healthBoxClass[checkIndex].Update(gt.DeltaTime());
		if (healthBoxClass[checkIndex].hasBeenConsumed == false && healthBoxClass[checkIndex].hasSpawnedIn==true)
		{
			ri->shouldRender = true;
		}
		else ri->shouldRender = false;

		checkIndex++;
	}
	checkIndex = 0;
	for (auto ri : mRitemLayer[(int)RenderLayer::ShieldBox])
	{
		shieldBoxClass[checkIndex].Update(gt.DeltaTime());
		if (shieldBoxClass[checkIndex].hasBeenConsumed == false && shieldBoxClass[checkIndex].hasSpawnedIn == true)
		{
			ri->shouldRender = true;
		}
		else ri->shouldRender = false;

		checkIndex++;
	}
	checkIndex = 0;
	for (auto ri : mRitemLayer[(int)RenderLayer::SpeedBox])
	{
		speedBoxClass[checkIndex].Update(gt.DeltaTime());
		if (speedBoxClass[checkIndex].hasBeenConsumed == false && speedBoxClass[checkIndex].hasSpawnedIn == true)
		{
			ri->shouldRender = true;
		}
		else ri->shouldRender = false;

		checkIndex++;
	}
	checkIndex = 0;
	for (auto ri : mRitemLayer[(int)RenderLayer::QuadBox])
	{
		quadBoxClass[checkIndex].Update(gt.DeltaTime());
		if (quadBoxClass[checkIndex].hasBeenConsumed == false && quadBoxClass[checkIndex].hasSpawnedIn == true)
		{
			ri->shouldRender = true;
		}
		else ri->shouldRender = false;

		checkIndex++;
	}
	checkIndex = 0;
	for (auto ri : mRitemLayer[(int)RenderLayer::InfiniteBox])
	{
		infiniteBoxClass[checkIndex].Update(gt.DeltaTime());
		if (infiniteBoxClass[checkIndex].hasBeenConsumed == false && infiniteBoxClass[checkIndex].hasSpawnedIn == true)
		{
			ri->shouldRender = true;
		}
		else ri->shouldRender = false;

		checkIndex++;
	}
}

/// <summary>
/// Draw all objects, includes PSO swapped
/// </summary>
void Application::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor , 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	// Bind all the materials used in this scene.  For structured buffers, we can bypass the heap and 
	// set as a root descriptor.
	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());

	// Bind all the textures used in this scene.  Observe
	// that we only have to specify the first descriptor in the table.  
	// The root signature knows how many descriptors are expected in the table.
	mCommandList->SetGraphicsRootDescriptorTable(3, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::World]);
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AmmoBox]);
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::HealthBox]);
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::ShieldBox]);
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::SpeedBox]);
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::QuadBox]);
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::InfiniteBox]);

	mCommandList->SetPipelineState(mPSOs["terrain"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Terrain]);

	mCommandList->OMSetStencilRef(0);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Enemy]);
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaClipped]);

	mCommandList->SetPipelineState(mPSOs["ui"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::UI]);

	mCommandList->SetPipelineState(mPSOs["pointSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::PointsGS]);
	
	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

/// <summary>
/// Mouse input click
/// </summary>
void Application::OnMouseDown(WPARAM btnState, int x, int y)
{
	//This has been changed to make the left mouse button shoot, and the ability to have free first person view will be determined by a variable that will be turned on by right clicking and
	// paused using the "P" key
	if ((btnState & MK_LBUTTON) != 0)
	{
		Shoot();

		if (tempCurrentHealth > 0.0f)
		{
			gameplayState = GameplayState::Playing;
		}
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		mGameAudio.Play("evilMusic");

		SetCapture(mhMainWnd);

		fpsReady = true;
	}
}

/// <summary>
/// Mouse input click (release)
/// </summary>
void Application::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

/// <summary>
/// Mouse movement
/// </summary>
void Application::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((fpsReady) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.9f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.9f * static_cast<float>(y - mLastMousePos.y));

		mCamera->Pitch(dy);
		mCamera->RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

/// <summary>
/// Keyboard input
/// </summary>
void Application::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();
	bool isWalking = false;
	float speed = sprint.sprintScale;
	sprint.isSprinting = false;

	if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
	{
		sprint.isSprinting = true;
		sprint.Update(playerStamina.current);
	}

	if (GetAsyncKeyState('W') & 0x8000)
	{
		mCamera->Walk(10.0f * dt * speed);
		isWalking = true;
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		mCamera->Walk(-10.0f * dt * speed);
		isWalking = true;
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		mCamera->Strafe(-10.0f * dt * speed);
		isWalking = true;
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		mCamera->Strafe(10.0f * dt * speed);
		isWalking = true;
	}

	if (GetAsyncKeyState('R') & 0x8000)
	{
		mGameAudio.Play("Pickup", nullptr, false, mAudioVolume, RandomPitchValue());
		currentGun.Reload();
	}

	if (GetAsyncKeyState('M') & 0x8000)
	{
		mMainPassCB.Shockwaves->Reset({0,0,0});
	}

	if (GetAsyncKeyState('L') & 0x8000)
	{
		mMainPassCB.Shockwaves->Reset(mCamera->GetPosition3f());
	}

	if (GetAsyncKeyState('P') & 0x8000)
		fpsReady = false;

	mCamera->UpdateViewMatrix();
	cameraBox.Center = mCamera->GetPosition3f();
	sprint.Update(playerStamina.current);
	CheckCameraCollision();
	if (isWalking == true)
		PlayFootAudio(dt);
}

/// <summary>
/// Animated materials (i.e. scrolling textures)
/// </summary>
void Application::AnimateMaterials(const GameTimer& gt)
{

}

void Application::UpdateEnemies(float dt)
{
	// updateboss
	{
		Vector3 bossPos = ApplyTerrainHeight({ bossStats.posX,bossStats.posY,bossStats.posZ }, terrainParam);
		bossPos.y += 10.5f;
		bossStats.posY = bossPos.y;
		bossAnim.Update(dt);

		bossStats.Update();
	}

	assert(mobs.size() == mobAnims.size());


	for (size_t i = 0; i < mobs.size(); i++)
	{
		Vector3 mp = ApplyTerrainHeight({ mobs.at(i).posX, mobs.at(i).posY, mobs.at(i).posZ }, terrainParam);
		mp.y += 0.5f;
		mobs.at(i).posY = mp.y;

		mobAnims.at(i).Update(dt);
	}

	for (size_t i = 0; i < mobs.size(); i++)
	{
		mobs.at(i).Movement(dt);
	}

	if ((int)GetGameTime() >= 7 && bossStats.SpawnReady())
	{
		SpawnEnemy();
	}
}

void Application::SpawnEnemy(const XMFLOAT3& pos, const XMFLOAT3& scale)
{
	DirectX::SimpleMath::Vector3 position = ApplyTerrainHeight(pos, terrainParam);
	position.y += scale.y * 0.5f;

	{
		if (mGeoPoints.at(GeoPointIndex::ENEMY).at(enemySpawnIndex).Billboard == BillboardType::NONE && COOLDOWN <= 0)
		{
			COOLDOWN = (float)bossStats.GetSpawnRate();
			mGeoPoints.at(GeoPointIndex::ENEMY).at(enemySpawnIndex).Pos = position;
			mGeoPoints.at(GeoPointIndex::ENEMY).at(enemySpawnIndex).Size = { scale.x,scale.y };

			mobAnims.at(enemySpawnIndex).SetAnimation(&gc::ANIM_DATA[gc::AnimIndex::ENEMY_IDLE], &mGeoPoints.at(GeoPointIndex::ENEMY).at(enemySpawnIndex).TexRect);
			mGeoPoints.at(GeoPointIndex::ENEMY).at(enemySpawnIndex).Billboard = BillboardType::AXIS_ORIENTATION;
			mobs.at(enemySpawnIndex).isActive = true;

			mobBox.at(enemySpawnIndex + 1) = BoundingBox(position, XMFLOAT3(scale.x, scale.y, scale.z));
			enemySpawnIndex = (enemySpawnIndex + 1) % gc::NUM_GEO_POINTS[GeoPointIndex::ENEMY];
		}
		COOLDOWN -= mTimer.DeltaTime();

		
	}
}

void Application::SpawnBoss(const XMFLOAT3& pos, const XMFLOAT3& scale)
{
	DirectX::SimpleMath::Vector3 position = ApplyTerrainHeight(pos, terrainParam);
	position.y += scale.y * 0.5f;
	mGeoPoints.at(GeoPointIndex::BOSS).at(0).Pos = position;
	mGeoPoints.at(GeoPointIndex::BOSS).at(0).Size = { scale.x,scale.y };

	bossAnim.SetAnimation(&gc::ANIM_DATA[gc::AnimIndex::BOSS_IDLE], &mGeoPoints.at(GeoPointIndex::BOSS).at(0).TexRect);
	mGeoPoints.at(GeoPointIndex::BOSS).at(0).Billboard = BillboardType::AXIS_ORIENTATION;
	mobBox.at(0) = BoundingBox(position, scale);
}

void Application::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{

		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->position);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->texTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			objConstants.MaterialIndex = e->material->MatCBIndex;

			currObjectCB->CopyData(e->objectCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void Application::UpdateMaterialBuffer(const GameTimer& gt)
{
	auto currMaterialBuffer = mCurrFrameResource->MaterialBuffer.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;

			currMaterialBuffer->CopyData(mat->MatCBIndex, matData);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

/// <summary>
/// Updates camera information, and lighting
/// </summary>
void Application::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera->GetView();
	XMMATRIX proj = mCamera->GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mCamera->GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
	mMainPassCB.Shockwaves[0].Update(gt.DeltaTime());
	// timer for terrain corruption
	mMainPassCB.TimeLeft = countdown.timeLeft;
	mMainPassCB.TimeLimit = countdown.timeRange;

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void Application::LoadTexture(const std::wstring& filename, const std::string& name)
{
	//Use this texture as example
	auto tex = std::make_unique<Texture>();
	tex->Name = name;
	tex->Filename = filename;
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), tex->Filename.c_str(),
		tex->Resource, tex->UploadHeap));

	mTextures[tex->Name] = std::move(tex);
}

/// <summary>
/// Loads texture files from directory
/// </summary>
void Application::LoadTextures()
{
	LoadTexture(L"Data/Textures/white1x1.dds", "tex");
	LoadTexture(L"Data/Textures/spritesheet.dds", "tempTex");
	LoadTexture(L"Data/Textures/obstacle.dds", "houseTex");
	LoadTexture(L"Data/Textures/terrain3.dds", "terrainTex");
	LoadTexture(L"Data/Textures/WireFence.dds", "fenceTex");
	LoadTexture(L"Data/Textures/ui.dds", "uiTex");

	LoadTexture(L"Data/Textures/AGP3HealthBox.dds", "healthBoxTex");
	LoadTexture(L"Data/Textures/AGP3AmmoBox.dds", "ammoBoxTex");
	LoadTexture(L"Data/Textures/AGP3ShieldBox.dds", "shieldBoxTex");
	LoadTexture(L"Data/Textures/AGP3SpeedBox.dds", "speedBoxTex");
	LoadTexture(L"Data/Textures/AGP3QuadBox.dds", "quadBoxTex");
	LoadTexture(L"Data/Textures/AGP3InfiniteBox.dds", "infiniteBoxTex");
}

void Application::BuildAudio()
{
	mGameAudio.Init();

	// sfx channel
	{
		mGameAudio.CreateChannel("sfx", AUDIO_CHANNEL_TYPE::SFX);
		mGameAudio.SetCacheSize("sfx", 30u);
		mGameAudio.ForceAudio("sfx", true);

		mGameAudio.LoadSound("sfx", "BossDead", L"Data/Audio/BossDead.wav");
		mGameAudio.LoadSound("sfx", "BossRoar", L"Data/Audio/BossRoar.wav");
		mGameAudio.LoadSound("sfx", "BossShoot", L"Data/Audio/BossShoot.wav");
		mGameAudio.LoadSound("sfx", "BossTakeDamage", L"Data/Audio/BossTakeDamage.wav");
		mGameAudio.LoadSound("sfx", "EnemyDead", L"Data/Audio/EnemyDead.wav");
		mGameAudio.LoadSound("sfx", "EnemyTakeDamage", L"Data/Audio/EnemyTakeDamage.wav");
		mGameAudio.LoadSound("sfx", "Pickup", L"Data/Audio/Pickup.wav");
		mGameAudio.LoadSound("sfx", "PickupHealth", L"Data/Audio/PickupHealth.wav");
		mGameAudio.LoadSound("sfx", "PlayerDead", L"Data/Audio/PlayerDead.wav");
		mGameAudio.LoadSound("sfx", "PlayerFootstep", L"Data/Audio/PlayerFootstep.wav");
		mGameAudio.LoadSound("sfx", "PlayerShoot", L"Data/Audio/PlayerShoot.wav");
		mGameAudio.LoadSound("sfx", "PlayerTakeDamage", L"Data/Audio/PlayerTakeDamage.wav");
		mGameAudio.LoadSound("sfx", "PlayerWeaponSlash", L"Data/Audio/PlayerWeaponSlash.wav");
		mGameAudio.LoadSound("sfx", "TimeUp", L"Data/Audio/TimeUp.wav");
		mGameAudio.SetChannelVolume("sfx", mAudioVolume);
		mGameAudio.Play("PlayerTakeDamage",nullptr,false, mAudioVolume,RandomPitchValue());
	}
	// music channel
	{
		mGameAudio.CreateChannel("music", AUDIO_CHANNEL_TYPE::MUSIC);
		mGameAudio.SetFade("music", 3.0f);
		mGameAudio.SetChannelVolume("music", mAudioVolume*0.2f);
		mGameAudio.LoadSound("music", "heroMusic", L"Data/Audio/615342__josefpres__8-bit-game-music-001-simple-mix-01-short-loop-120-bpm.wav");
		mGameAudio.LoadSound("music", "evilMusic", L"Data/Audio/545218__victor-natas__evil-music.wav");
		mGameAudio.Play("heroMusic");
	}
	
}

void Application::CreateSRV(const std::string& texName, CD3DX12_CPU_DESCRIPTOR_HANDLE& hdesc, D3D12_SRV_DIMENSION dim)
{
	auto tex = mTextures[texName]->Resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = tex->GetDesc().Format;
	srvDesc.ViewDimension = dim;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(tex.Get(), &srvDesc, hdesc);

}

void Application::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsShaderResourceView(0, 1);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);


	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void Application::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 12; // includes font sprites
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// todo put texture strings into constants.h and iterate through. Dont use for(auto&...) as unordered.
	CreateSRV("tex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("tempTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("houseTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("terrainTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("uiTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);	
	CreateSRV("fenceTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	CreateSRV("healthBoxTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("ammoBoxTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("shieldBoxTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("speedBoxTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("quadBoxTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	CreateSRV("infiniteBoxTex", hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
}

void Application::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mShaders["UIVS"] = d3dUtil::CompileShader(L"Shaders\\UI.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["UIPS"] = d3dUtil::CompileShader(L"Shaders\\UI.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayoutUi =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

#if GS_TOGGLE

	mShaders["pointSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\Geometry.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["pointSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\Geometry.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["pointSpritePS"] = d3dUtil::CompileShader(L"Shaders\\Geometry.hlsl", nullptr, "PS", "ps_5_1");
	
	mPointSpriteInputLayout =
	{
#if POINTS_SHADER_INPUT
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT,				0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXRECT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,		0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BILLBOARD", 0, DXGI_FORMAT_R8_SINT,				0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
#else
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
#endif
	};
#endif

#if	TERRAIN_SHADER_TOGGLE
	mShaders["terrainVS"] = d3dUtil::CompileShader(L"Shaders\\Terrain.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["terrainPS"] = d3dUtil::CompileShader(L"Shaders\\Terrain.hlsl", defines, "PS", "ps_5_1");
#endif
}

void Application::BuildTerrainGeometry()
{
	GeometryGenerator geoGen;
	srand(time(0));

	const DirectX::SimpleMath::Vector2 terrainRes(200.0f);
	
	// terrain parameters
	terrainParam.noiseScale = RandFloat(0.01f, 0.05f);
	terrainParam.seed = RandFloat(1.0f, 1000000.0f);
	terrainParam.curveStrength = RandFloat(1.0f, 3.0f);
	terrainParam.heightMulti = RandFloat(2.0f, 5.0f);

	GeometryGenerator::MeshData grid = geoGen.CreateGrid(terrainRes.x, terrainRes.y, 128, 128);

	std::vector<Vertex> vertices(grid.Vertices.size());

	// apply terrain height to grid
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		grid.Vertices[i].Position = ApplyTerrainHeight(grid.Vertices[i].Position, terrainParam);
	}

	CalcTerrianNormal2(grid);

	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		vertices[i].Pos = grid.Vertices[i].Position;
		vertices[i].Normal = grid.Vertices[i].Normal;
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["landGeo"] = std::move(geo);

}

void Application::UpdatePoints(const GameTimer& gt)
{
	for (size_t vb = 0; vb < GeoPointIndex::COUNT; vb++)
	{
		auto gpVB = mCurrFrameResource->GeoPointVB[vb].get();
		size_t vertexCount = gc::NUM_GEO_POINTS[vb];

		assert(mGeoPoints.at(vb).size() == gc::NUM_GEO_POINTS[vb] && "Cannot be resized after building geometry. Add/Remove in gc::NUM_GEO_POINTS");

		for (size_t v = 0; v < vertexCount; v++)
		{
			gpVB->CopyData(v, mGeoPoints.at(vb).at(v));
		}

		// updates ritems
		mGeoPointsRitems[vb]->geometry->VertexBufferGPU = gpVB->Resource();
	}
}

/// <summary>
/// Construct all geometry, needs to be done prior to run time
/// </summary>
void Application::BuildGeometry()
{
	//Build all geometry here
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	{
		SubmeshGeometry boxSubmesh;
		boxSubmesh.IndexCount = (UINT)box.Indices32.size();
		boxSubmesh.StartIndexLocation = 0;
		boxSubmesh.BaseVertexLocation = 0;

		std::vector<Vertex> vertices(box.Vertices.size());

		for (size_t i = 0; i < box.Vertices.size(); ++i)
		{
			vertices[i].Pos = box.Vertices[i].Position;
			vertices[i].Normal = box.Vertices[i].Normal;
			vertices[i].TexC = box.Vertices[i].TexC;
		}

		std::vector<std::uint16_t> indices = box.GetIndices16();

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "boxGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		geo->DrawArgs["box"] = boxSubmesh;

		mGeometries[geo->Name] = std::move(geo);
	}

#if OBSTACLE_TOGGLE
	// build obstacle geom
	for (size_t i = 0; i < gc::NUM_OBSTACLE; i++)
	{
		BuildObjGeometry(gc::OBSTACLE_DATA[i].filename, gc::OBSTACLE_DATA[i].geoName, gc::OBSTACLE_DATA[i].subGeoName);
	}
#endif //OBSTACLE_TOGGLE
	;
#if UI_SPRITE_TOGGLE

	BuildObjGeometry(gc::UI_CHAR.filename, gc::UI_CHAR.geoName, gc::UI_CHAR.subGeoName);
	BuildObjGeometry(gc::UI_WORD.filename, gc::UI_WORD.geoName, gc::UI_WORD.subGeoName);


	for (auto& s : gc::UI_SPRITE_DATA)
	{
		BuildObjGeometry(s.filename, s.geoName, s.subGeoName);
	}

#endif //UI_SPRITE_TOGGLE

#if GS_TOGGLE
	BuildPointsGeometry();
#endif //GS_TOGGLE
}

void Application::BuildPointsGeometry()
{
	// initialises correct number of points for each geometery point object
	{

		std::vector<std::uint16_t> indices;

		for (size_t vb = 0; vb < GeoPointIndex::COUNT; vb++)
		{
			size_t vertexCount = gc::NUM_GEO_POINTS[vb];

			mGeoPoints.at(vb).resize(vertexCount);
			indices.resize(vertexCount);

			assert(mGeoPoints.at(vb).size() < 0x0000ffff);

			for (size_t v = 0; v < vertexCount; v++)
			{
				Vector3 pos = Vector3(RandFloat(-50.0f, 50.0f), 0.0f, RandFloat(-50.0f, 50.0f));
				mGeoPoints.at(vb).at(v).Pos = ApplyTerrainHeight(pos, terrainParam);
				indices.at(v) = (uint16_t)v;
			}

			UINT vbByteSize = (UINT)vertexCount * sizeof(Point);
			UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

			auto geo = std::make_unique<MeshGeometry>();
			geo->Name = gc::GEO_POINT_NAME[vb].geoName;

			geo->VertexBufferCPU = nullptr;
			geo->VertexBufferGPU = nullptr;

			// setup gpu index buffer
			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
			geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
				mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

			geo->VertexByteStride = sizeof(Point);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = DXGI_FORMAT_R16_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)indices.size();
			submesh.StartIndexLocation = 0;
			submesh.BaseVertexLocation = 0;

			geo->DrawArgs[gc::GEO_POINT_NAME[vb].subGeoName] = submesh;

			mGeometries[geo->Name] = std::move(geo);
		}

		particleCtrl.Init(mGeoPoints.at(GeoPointIndex::PARTICLE).size(), 10);

	}
}

void Application::BuildEnemySpritesGeometry()
{
	
	struct CacoSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int cacoCount = 3;
	std::array<CacoSpriteVertex, 5> vertices;
	for (UINT i = 0; i < cacoCount; ++i)
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = 0.f; //REPLACE WHEN FLOOR HEIGHT IS DECIDED

		// Move enemy high above land height.
		y += 35.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
	}

	std::array<std::uint16_t, 16> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(CacoSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "enemySpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(CacoSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["enemypoints"] = submesh;

	mGeometries["enemySpritesGeo"] = std::move(geo);
}

void Application::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for highlight objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC highlightPsoDesc = opaquePsoDesc;

	// Change the depth test from < to <= so that if we draw the same triangle twice, it will
	// still pass the depth test.  This is needed because we redraw the picked triangle with a
	// different material to highlight it.  If we do not use <=, the triangle will fail the 
	// depth test the 2nd time we try and draw it.
	highlightPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// Standard transparency blending.
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	highlightPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&highlightPsoDesc, IID_PPV_ARGS(&mPSOs["highlight"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPSODesc = opaquePsoDesc;
	alphaTestedPSODesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()), 
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPSODesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC uiPSODesc = alphaTestedPSODesc;
	uiPSODesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["UIVS"]->GetBufferPointer()),
		mShaders["UIVS"]->GetBufferSize()
	};
	uiPSODesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["UIPS"]->GetBufferPointer()),
		mShaders["UIPS"]->GetBufferSize()
	};

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&uiPSODesc, IID_PPV_ARGS(&mPSOs["ui"])));

#if GS_TOGGLE
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pointSpritePsoDesc = opaquePsoDesc;
	pointSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["pointSpriteVS"]->GetBufferPointer()),
		mShaders["pointSpriteVS"]->GetBufferSize()
	};
	pointSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["pointSpriteGS"]->GetBufferPointer()),
		mShaders["pointSpriteGS"]->GetBufferSize()
	};
	pointSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["pointSpritePS"]->GetBufferPointer()),
		mShaders["pointSpritePS"]->GetBufferSize()
	};
	pointSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	pointSpritePsoDesc.InputLayout = { mPointSpriteInputLayout.data(), (UINT)mPointSpriteInputLayout.size() };
	pointSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&pointSpritePsoDesc, IID_PPV_ARGS(&mPSOs["pointSprites"])));

#endif //GS_TOGGLE
#if	TERRAIN_SHADER_TOGGLE
	D3D12_GRAPHICS_PIPELINE_STATE_DESC terrainPSODesc = opaquePsoDesc;
	terrainPSODesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["terrainPS"]->GetBufferPointer()),
		mShaders["terrainPS"]->GetBufferSize()
	};
	terrainPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["terrainVS"]->GetBufferPointer()),
		mShaders["terrainVS"]->GetBufferSize()
	};
	terrainPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&terrainPSODesc, IID_PPV_ARGS(&mPSOs["terrain"])));

	
#endif
	

}

void Application::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(
			md3dDevice.Get(),
			1, 
			(UINT)mAllRitems.size(), 
			(UINT)mMaterials.size(),
			(UINT)mGeoPoints.at(GeoPointIndex::BOSS).size(), 
			(UINT)mGeoPoints.at(GeoPointIndex::ENEMY).size(),
			(UINT)mGeoPoints.at(GeoPointIndex::PARTICLE).size(),
			(UINT)mGeoPoints.at(GeoPointIndex::SCENERY).size()
			));
	}
}

void Application::BuildMaterial(int texSrvHeapIndex, const std::string& name, float roughness, const DirectX::XMFLOAT4& diffuseAlbedo, const DirectX::XMFLOAT3& fresnel)
{
	auto material = std::make_unique<Material>();
	material->Name = name;
	material->MatCBIndex = matIndex;
	material->DiffuseSrvHeapIndex = texSrvHeapIndex;
	material->DiffuseAlbedo = diffuseAlbedo;
	material->FresnelR0 = fresnel;
	material->Roughness = roughness;

	mMaterials[material->Name] = std::move(material);

	++matIndex; //increments for next material
}

/// <summary>
/// Constructs materials from textures, to use on geometry
/// </summary>
void Application::BuildMaterials()
{

	BuildMaterial(0, "Grey", 0.0f, XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f), XMFLOAT3(0.04f, 0.04f, 0.04f));
	BuildMaterial(0, "Black", 0.0f, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT3(0.04f, 0.04f, 0.04f));
	BuildMaterial(0, "Red", 0.0f, XMFLOAT4(1.0f, 0.0f, 0.0f, 0.6f), XMFLOAT3(0.06f, 0.06f, 0.06f));
	BuildMaterial(0, "Orange", 0.0f, XMFLOAT4(.9f, 0.5f, 0.2f, 0.6f), XMFLOAT3(0.06f, 0.06f, 0.06f)); //Temp Shield Texture
	BuildMaterial(0, "Blue", 0.0f, XMFLOAT4(.1f, 0.1f, 0.8f, 0.6f), XMFLOAT3(0.06f, 0.06f, 0.06f)); //Temp Speed Texture
	BuildMaterial(0, "Purple", 0.0f, XMFLOAT4(.45f, 0.1f, 0.6f, 0.6f), XMFLOAT3(0.06f, 0.06f, 0.06f)); //Temp Quad Damage Texture
	BuildMaterial(0, "Green", 0.0f, XMFLOAT4(.3f, 0.8f, 0.3f, 0.6f), XMFLOAT3(0.06f, 0.06f, 0.06f)); //Temp Infinite Ammo Texture

	// MATT TEXTURE STUFF
	BuildMaterial(1, "Tentacle", 0.25f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.1f, 0.1f, 0.1f));
	BuildMaterial(2, "HouseMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	BuildMaterial(3, "TerrainMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	BuildMaterial(4, "uiMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	BuildMaterial(5, "FenceMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	BuildMaterial(6, "HealthBoxMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	BuildMaterial(7, "AmmoBoxMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	BuildMaterial(8, "ShieldBoxMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	BuildMaterial(9, "SpeedBoxMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	BuildMaterial(10, "QuadBoxMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	BuildMaterial(11, "InfiniteBoxMat", 0.99f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

}

std::unique_ptr<RenderItem> Application::BuildRenderItem(UINT& objCBindex, const std::string& geoName, const std::string& subGeoName, const std::string& matName, D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	auto rItem = std::make_unique<RenderItem>();
	rItem->position = MathHelper::Identity4x4();
	rItem->objectCBIndex = objCBindex;
	rItem->geometry = mGeometries[geoName].get();
	rItem->PrimitiveType = primitiveTopology;
	rItem->IndexCount = rItem->geometry->DrawArgs[subGeoName].IndexCount;
	rItem->StartIndexLocation = rItem->geometry->DrawArgs[subGeoName].StartIndexLocation;
	rItem->BaseVertexLocation = rItem->geometry->DrawArgs[subGeoName].BaseVertexLocation;
	rItem->material = mMaterials[matName].get();
	// increment for next render item
	++objCBindex;

	return std::move(rItem);
}

/// <summary>
/// Build render items, we may use different structures which would require different functions
/// </summary>
void Application::BuildRenderItems()
{
	// initial values for boss
	DirectX::SimpleMath::Vector3 position = ApplyTerrainHeight({ 0.0f,0.0f,0.0f }, terrainParam);
	DirectX::SimpleMath::Vector3 scale = { 10.0f,10.0f,10.0f };
	// on ground
	position.y += scale.y * 0.5f;
	//Build render items here
	UINT objectCBIndex = 0;
	
	//Border visual (temp)
	{
		auto border_01 = BuildRenderItem(objectCBIndex, "boxGeo", "box", "FenceMat");
		XMStoreFloat4x4(&border_01->position, XMMatrixScaling(.1f, 10, 200) * XMMatrixTranslation(100, 0, 0));
		XMStoreFloat4x4(&border_01->texTransform, XMMatrixScaling(200.0f, 10.0f, 1.0f));
		mRitemLayer[(int)RenderLayer::AlphaClipped].emplace_back(border_01.get());
		mAllRitems.push_back(std::move(border_01));
		auto border_02 = BuildRenderItem(objectCBIndex, "boxGeo", "box", "FenceMat");
		XMStoreFloat4x4(&border_02->position, XMMatrixScaling(.1f, 10, 200) * XMMatrixTranslation(-100, 0, 0));
		XMStoreFloat4x4(&border_02->texTransform, XMMatrixScaling(200.0f, 10.0f, 1.0f));
		mRitemLayer[(int)RenderLayer::AlphaClipped].emplace_back(border_02.get());
		mAllRitems.push_back(std::move(border_02));
		auto border_03 = BuildRenderItem(objectCBIndex, "boxGeo", "box", "FenceMat");
		XMStoreFloat4x4(&border_03->position, XMMatrixScaling(200, 10, .1f) * XMMatrixTranslation(0, 0, 100));
		XMStoreFloat4x4(&border_03->texTransform, XMMatrixScaling(200.0f, 10.0f, 1.0f));
		mRitemLayer[(int)RenderLayer::AlphaClipped].emplace_back(border_03.get());
		mAllRitems.push_back(std::move(border_03));
		auto border_04 = BuildRenderItem(objectCBIndex, "boxGeo", "box", "FenceMat");
		XMStoreFloat4x4(&border_04->position, XMMatrixScaling(200, 10, .1f) * XMMatrixTranslation(0, 0, -100));
		XMStoreFloat4x4(&border_04->texTransform, XMMatrixScaling(200.0f, 10.0f, 1.0f));
		mRitemLayer[(int)RenderLayer::AlphaClipped].emplace_back(border_04.get());
		mAllRitems.push_back(std::move(border_04));
	}
	// Spawnable crates
	{
		//AMMO
		scale = { 2.0f,2.0f,2.0f };

		for (size_t i = 0; i < 4; i++)
		{
			position.x = sin((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));
			position.z = cos((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));

			// slight inset into terrain
			position = ApplyTerrainHeight(position, terrainParam);
			position.y += 0.8f;

			auto ammoCrate = BuildRenderItem(objectCBIndex, "boxGeo", "box", "AmmoBoxMat");
			XMStoreFloat4x4(&ammoCrate->position, XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixTranslation(position.x, position.y, position.z));
			ammoBox[i] = BoundingBox(position, scale);
			mRitemLayer[(int)RenderLayer::AmmoBox].emplace_back(ammoCrate.get());
			mAllRitems.push_back(std::move(ammoCrate));
		}
		
		//HEALTH
		for (size_t i = 0; i < 4; i++)
		{
			position.x = sin((float)i) * RandFloat(20.0f, (100.0f-mCamera->GetBorderBuffer()));
			position.z = cos((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));

			// slight inset into terrain
			position = ApplyTerrainHeight(position, terrainParam);
			position.y += 0.8f;

			auto healthCrate= BuildRenderItem(objectCBIndex, "boxGeo", "box", "HealthBoxMat");
			XMStoreFloat4x4(&healthCrate->position, XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixTranslation(position.x, position.y, position.z));
			healthBox[i] = BoundingBox(position, scale);
			mRitemLayer[(int)RenderLayer::HealthBox].emplace_back(healthCrate.get());
			mAllRitems.push_back(std::move(healthCrate));
		}

		//SHIELD
		for (size_t i = 0; i < 4; i++)
		{
			position.x = sin((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));
			position.z = cos((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));

			// slight inset into terrain
			position = ApplyTerrainHeight(position, terrainParam);
			position.y += 0.8f;

			auto shieldCrate = BuildRenderItem(objectCBIndex, "boxGeo", "box", "ShieldBoxMat");
			XMStoreFloat4x4(&shieldCrate->position, XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixTranslation(position.x, position.y, position.z));
			shieldBox[i] = BoundingBox(position, scale);
			mRitemLayer[(int)RenderLayer::ShieldBox].emplace_back(shieldCrate.get());
			mAllRitems.push_back(std::move(shieldCrate));
		}

		//SPEED
		for (size_t i = 0; i < 4; i++)
		{
			position.x = sin((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));
			position.z = cos((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));

			// slight inset into terrain
			position = ApplyTerrainHeight(position, terrainParam);
			position.y += 0.8f;

			auto speedCrate = BuildRenderItem(objectCBIndex, "boxGeo", "box", "SpeedBoxMat");
			XMStoreFloat4x4(&speedCrate->position, XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixTranslation(position.x, position.y, position.z));
			speedBox[i] = BoundingBox(position, scale);
			mRitemLayer[(int)RenderLayer::SpeedBox].emplace_back(speedCrate.get());
			mAllRitems.push_back(std::move(speedCrate));
		}

		//QUAD DAMAGE
		for (size_t i = 0; i < 4; i++)
		{
			position.x = sin((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));
			position.z = cos((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));

			// slight inset into terrain
			position = ApplyTerrainHeight(position, terrainParam);
			position.y += 0.8f;

			auto quadCrate = BuildRenderItem(objectCBIndex, "boxGeo", "box", "QuadBoxMat");
			XMStoreFloat4x4(&quadCrate->position, XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixTranslation(position.x, position.y, position.z));
			quadBox[i] = BoundingBox(position, scale);
			mRitemLayer[(int)RenderLayer::QuadBox].emplace_back(quadCrate.get());
			mAllRitems.push_back(std::move(quadCrate));
		}

		//INFINITE AMMO
		for (size_t i = 0; i < 4; i++)
		{
			position.x = sin((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));
			position.z = cos((float)i) * RandFloat(20.0f, (100.0f - mCamera->GetBorderBuffer()));

			// slight inset into terrain
			position = ApplyTerrainHeight(position, terrainParam);
			position.y += 0.8f;

			auto infiniteCrate = BuildRenderItem(objectCBIndex, "boxGeo", "box", "InfiniteBoxMat");
			XMStoreFloat4x4(&infiniteCrate->position, XMMatrixScaling(scale.x, scale.y, scale.z) * XMMatrixTranslation(position.x, position.y, position.z));
			infiniteBox[i] = BoundingBox(position, scale);
			mRitemLayer[(int)RenderLayer::InfiniteBox].emplace_back(infiniteCrate.get());
			mAllRitems.push_back(std::move(infiniteCrate));
		}
	}

	{
		auto terrain = BuildRenderItem(objectCBIndex, "landGeo", "grid", "TerrainMat");
		mRitemLayer[(int)RenderLayer::Terrain].emplace_back(terrain.get());
		mAllRitems.push_back(std::move(terrain));
	}
	
#if OBSTACLE_TOGGLE
	{
	
		for (size_t i = 0; i < gc::NUM_OBSTACLE; i++)
		{

			position.x = sin(2.0f*(float)i) * RandFloat(20.0f, 50.0f);
			position.z = cos(2.0f*(float)i) * RandFloat(20.0f, 50.0f);
			position = ApplyTerrainHeight(position, terrainParam);

			auto obst = BuildRenderItem(objectCBIndex, gc::OBSTACLE_DATA[i].geoName, gc::OBSTACLE_DATA[i].subGeoName, "HouseMat");

			XMStoreFloat4x4(&obst->position, Matrix::CreateTranslation(position));
			XMStoreFloat4x4(&obst->texTransform, Matrix::Identity);
			obstBox[i] = BoundingBox(position, gc::OBSTACLE_DATA[i].boundingBox);
			mRitemLayer[(int)RenderLayer::World].emplace_back(obst.get());
			mAllRitems.push_back(std::move(obst));
		}
	}
#endif

	
#if UI_SPRITE_TOGGLE	

	uint32_t offset = mAllRitems.size();

	// generic sprites
	for (size_t i = 0; i < gc::NUM_UI_SPRITES; i++)
	{

		auto ui = BuildRenderItem(objectCBIndex, gc::UI_SPRITE_DATA[i].geoName, gc::UI_SPRITE_DATA[i].subGeoName, "uiMat");
		mRitemLayer[(int)RenderLayer::UI].emplace_back(ui.get());
		mAllRitems.push_back(std::move(ui));

		spriteCtrl[i].Init(this, offset++, gc::UI_SPRITE_DATA[i].position, gc::UI_SPRITE_DEFAULT_DISPLAY[i]);
	}

	// char lines sprites
	{

		// points display ritems 
		offset = mAllRitems.size();
		for (size_t i = 0; i < gc::UI_LINE_1_LEN; i++)
		{
			Vector3 tempPos = gc::UI_POINTS_POS;
			tempPos.x += gc::UI_CHAR_SPACING * (float)i;

			Vector3 tempUVW = Vector2::Zero;

			auto uiChar = BuildRenderItem(objectCBIndex, gc::UI_CHAR.geoName, gc::UI_CHAR.subGeoName, "uiMat");
			XMStoreFloat4x4(&uiChar->position,  Matrix::CreateTranslation(gc::UI_CHAR.position + tempPos));
			XMStoreFloat4x4(&uiChar->texTransform,  Matrix::CreateTranslation( tempUVW));

			mRitemLayer[(int)RenderLayer::UI].emplace_back(uiChar.get());
			mAllRitems.push_back(std::move(uiChar));
		}
		
		pointsDisplay.Init(this, offset, gc::UI_LINE_1_LEN, gc::UI_POINTS_POS, gc::CHAR_PRD, gc::CHAR_PTS, 2);
		
		// time display ritems
		offset = mAllRitems.size();
		for (size_t i = 0; i < gc::UI_LINE_2_LEN; i++)
		{
			Vector3 tempPos = gc::UI_TIME_POS;
			tempPos.x += gc::UI_CHAR_SPACING * (float)i;

			Vector3 tempUVW = Vector2::Zero;

			auto uiChar = BuildRenderItem(objectCBIndex, gc::UI_CHAR.geoName, gc::UI_CHAR.subGeoName, "uiMat");
			XMStoreFloat4x4(&uiChar->position, Matrix::CreateTranslation(gc::UI_CHAR.position + tempPos));
			XMStoreFloat4x4(&uiChar->texTransform, Matrix::CreateTranslation(tempUVW));

			mRitemLayer[(int)RenderLayer::UI].emplace_back(uiChar.get());
			mAllRitems.push_back(std::move(uiChar));
		}

		timeDisplay.Init(this, offset, gc::UI_LINE_2_LEN, gc::UI_TIME_POS, gc::CHAR_COLON, gc::CHAR_TIME, 1);

		// ammo display ritems
		offset = mAllRitems.size();
		for (size_t i = 0; i < gc::UI_LINE_3_LEN; i++)
		{
			Vector3 tempPos = gc::UI_TIME_POS;
			tempPos.x += gc::UI_CHAR_SPACING * (float)i;

			Vector3 tempUVW = Vector2::Zero;

			auto uiChar = BuildRenderItem(objectCBIndex, gc::UI_CHAR.geoName, gc::UI_CHAR.subGeoName, "uiMat");
			XMStoreFloat4x4(&uiChar->position, Matrix::CreateTranslation(gc::UI_CHAR.position + tempPos));
			XMStoreFloat4x4(&uiChar->texTransform, Matrix::CreateTranslation(tempUVW));

			mRitemLayer[(int)RenderLayer::UI].emplace_back(uiChar.get());
			mAllRitems.push_back(std::move(uiChar));
		}

		ammoDisplay.Init(this, offset, gc::UI_LINE_3_LEN, gc::UI_AMMO_POS, gc::CHAR_AMM0, gc::CHAR_SPC, 0);

	}

	// word sprites
	{

		offset = mAllRitems.size();

		for (size_t i = 0; i < gc::UI_NUM_RITEM_WORD; i++)
		{
			Vector3 tempUVW = Vector2::Zero;
			tempUVW.y += gc::UI_WORD_INC * (float)i;

			auto uiWord = BuildRenderItem(objectCBIndex, gc::UI_WORD.geoName, gc::UI_WORD.subGeoName, "uiMat");


			mRitemLayer[(int)RenderLayer::UI].emplace_back(uiWord.get());
			mAllRitems.push_back(std::move(uiWord));

			wordCtrl[i].Init(this, offset++, gc::UI_WORD_INIT_POSITION[i], true, tempUVW);

		}

	}


#endif //UI_SPRITE_TOGGLE

	{
		RenderLayer gpRlayer[GeoPointIndex::COUNT]
		{
			RenderLayer::PointsGS,
			RenderLayer::PointsGS,
			RenderLayer::PointsGS,
			RenderLayer::PointsGS
		};

		for (size_t vb = 0; vb < GeoPointIndex::COUNT; vb++)
		{
			auto ri = BuildRenderItem(objectCBIndex, gc::GEO_POINT_NAME[vb].geoName, gc::GEO_POINT_NAME[vb].subGeoName, "Tentacle", D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			mGeoPointsRitems[vb] = ri.get();
			mRitemLayer[(int)gpRlayer[vb]].push_back(ri.get());
			mAllRitems.push_back(std::move(ri));
		}

		

	}
}

/// <summary>
/// Draw render items, we may use different structures which would require different functions
/// </summary>
void Application::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialBuffer->Resource();


	//Draw items block here
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		if (ri->shouldRender == false)
			continue;

		cmdList->IASetVertexBuffers(0, 1, &ri->geometry->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->geometry->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->objectCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->material->MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

}

// loads obj objects into mGeometries
// eg: BuildObjGeometry("Data/Models/object.obj", "objectGeo", "object");// loads obj
void Application::BuildObjGeometry(const std::string& filepath, const std::string& meshName, const std::string& subMeshName)
{
	struct VertexConversion
	{
		Vertex operator()(const objl::Vertex& v)
		{
			return
			{
				{v.Position.X,v.Position.Y,v.Position.Z},
				{v.Normal.X,v.Normal.Y,v.Normal.Z},
				{v.TextureCoordinate.X,-v.TextureCoordinate.Y} // flipped Y coor from blender
			};
		}
	};

	struct IndexConversion
	{
		int operator()(const unsigned int& i)
		{
			return static_cast<int>(i);
		}
	};

	struct MeshConvertion
	{
		std::vector<Vertex> vertices;
		std::vector<std::int32_t> indices;

		void operator()(const objl::Mesh& mesh)
		{
			std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(vertices), VertexConversion());
			std::transform(mesh.Indices.begin(), mesh.Indices.end(), std::back_inserter(indices), IndexConversion());
		}
	};

	objl::Loader loader;

	bool loadout = loader.LoadFile(filepath);

	if (loadout)
	{
		MeshConvertion meshConvert;

		// converts vertex and index formats from objl to local
		std::for_each(loader.LoadedMeshes.begin(), loader.LoadedMeshes.end(), std::ref(meshConvert));

		const UINT vbByteSize = (UINT)meshConvert.vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)meshConvert.indices.size() * sizeof(std::int32_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = meshName;

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), meshConvert.vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), meshConvert.indices.data(), ibByteSize);

		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), meshConvert.vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), meshConvert.indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R32_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry submesh;
		submesh.IndexCount = (UINT)meshConvert.indices.size();
		submesh.StartIndexLocation = 0;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs[subMeshName] = submesh;

		mGeometries[geo->Name] = std::move(geo);
	}
	else
	{
		//file not found
		assert(false);
	}
}

void Application::BuildEnemyObjects()
{
	mobBox.resize(gc::NUM_GEO_POINTS[GeoPointIndex::BOSS] + gc::NUM_GEO_POINTS[GeoPointIndex::ENEMY]);
	particleAnims.resize(gc::NUM_GEO_POINTS[GeoPointIndex::PARTICLE]);
	mobAnims.resize(gc::NUM_GEO_POINTS[GeoPointIndex::ENEMY]);
	mobs.resize(gc::NUM_GEO_POINTS[GeoPointIndex::ENEMY]);

	bossStats.Setup(&mGeoPoints.at(GeoPointIndex::ENEMY).at(0), mCamera, &mobBox.at(0));//Possibly move this somewhere else in order to setup the geometry

	for (size_t i = 0; i < gc::NUM_GEO_POINTS[GeoPointIndex::ENEMY]; i++)
	{
		assert(i < gc::NUM_GEO_POINTS[GeoPointIndex::ENEMY]);

		mobs.at(i).Setup(&mGeoPoints.at(GeoPointIndex::ENEMY).at(i), mCamera, &mobBox.at(1 + i));

		mobAnims.at(i).SetAnimation(&gc::ANIM_DATA[gc::AnimIndex::ENEMY_IDLE], &mGeoPoints.at(GeoPointIndex::ENEMY).at(i).TexRect);
	}
	assert(mobs.size() == mobAnims.size());

	// particle
	for (size_t i = 0; i < gc::NUM_GEO_POINTS[GeoPointIndex::PARTICLE]; i++)
	{
		particleAnims.at(i).SetAnimation(&gc::ANIM_DATA[gc::AnimIndex::PARTICLE_PURPLE], &mGeoPoints.at(GeoPointIndex::PARTICLE).at(i).TexRect);
	}
}

void Application::Shoot()
{
	if (currentGun.CanShoot())
	{
		mGameAudio.Play("PlayerShoot", nullptr, false, mAudioVolume, RandomPitchValue());

		if (!infiniteActive) //Check To See If InfAmmo Is Active, If It Is Don't Decrease Ammo
		{
			currentGun.Shoot(); //Just Decreases Ammo Count
		}

		XMFLOAT4X4 P = mCamera->GetProj4x4f();
		float vx = (+2.0f * (mClientWidth / 2) / mClientWidth - 1.0f) / P(0, 0);
		float vy = (-2.0f * (mClientHeight / 2) / mClientHeight + 1.0f) / P(1, 1);

		XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
		XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);
		XMMATRIX V = mCamera->GetView();
		XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);

		for (size_t i = 0; i < mobBox.size(); i++)
		{
			// boss/enemy
			bool isBoss = i == 0;
			size_t pointIndex = (isBoss) ? i : i - 1;
			size_t bbIndex = i;

			Point* p = (i == 0) ? &mGeoPoints.at(GeoPointIndex::BOSS).at(0) : &mGeoPoints.at(GeoPointIndex::ENEMY).at(pointIndex);


			XMMATRIX W = Matrix::CreateTranslation(p->Pos);
			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);

			XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);

			rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
			rayDir = XMVector3TransformNormal(rayDir, toLocal);

			rayDir = XMVector3Normalize(rayDir);

			float tmin = 0.0f;

			// boss collision logic
			if (isBoss)
			{
				if (mobBox.at(bbIndex).Intersects(rayOrigin, rayDir, tmin))
				{
					if (gameplayState == GameplayState::Playing)
					{
						points += mMainPassCB.TotalTime * gc::POINTS_MULTI;
					}
					particleCtrl.Explode(&mGeoPoints[GeoPointIndex::PARTICLE], { bossStats.posX, bossStats.posY, bossStats.posZ }, 3.0f);
					
					if (!quadActive) //Normal Damage Calculations
					{
						if (bossStats.DealDamage(currentGun.GetDamage()))
						{
							p->Billboard = BillboardType::NONE;
							points += mMainPassCB.TotalTime * gc::POINTS_MULTI;
						}
					}
					else //Quad Damage Calculations
					{
						if (bossStats.DealDamage(currentGun.GetDamage() * 4))
						{
							p->Billboard = BillboardType::NONE;
							points += mMainPassCB.TotalTime * gc::POINTS_MULTI;
						}
					}

				}
			}
			// mob collision logic
			else
			{
				// mobs
				if (mobBox.at(bbIndex).Intersects(rayOrigin, rayDir, tmin))
				{
					mGameAudio.Play("EnemyTakeDamage", nullptr, false, mAudioVolume, RandomPitchValue());
					particleCtrl.Explode(&mGeoPoints[GeoPointIndex::PARTICLE], { mobs.at(pointIndex).posX, mobs.at(pointIndex).posY , mobs.at(pointIndex).posZ }, 2.0f);

					if (mobs.at(pointIndex).DealDamage(currentGun.GetDamage()))
					{
						p->Billboard = BillboardType::NONE;
						points += mMainPassCB.TotalTime * gc::POINTS_MULTI;
					}
				}
			}

		}


	}
}

void Application::CheckCameraCollision()
{

	Vector3 pos = mCamera->GetPosition();
	pos = ApplyTerrainHeight(pos, terrainParam);
	pos.y += 1.5f;

	mCamera->SetPosition(pos);


	int counter = -1;
	for (auto ri : mRitemLayer[(int)RenderLayer::AmmoBox])
	{
		counter++;

		auto geo = ri->geometry;
		if (ri->shouldRender == false)
			continue;

		if(ammoBox[counter].Contains(mCamera->GetPosition()))
		{
			mGameAudio.Play("Pickup", nullptr, false, mAudioVolume, RandomPitchValue());

			ri->shouldRender = false;
			ri->NumFramesDirty = gNumFrameResources;
			currentGun.AddAmmo(ammoBoxClass[counter].Consume());
		}
	}

	counter = -1;
	for (auto ri : mRitemLayer[(int)RenderLayer::HealthBox])
	{
		counter++;

		auto geo = ri->geometry;
		if (ri->shouldRender == false)
			continue;

		if (healthBox[counter].Contains(mCamera->GetPosition()))
		{
			mGameAudio.Play("PickupHealth", nullptr, false, mAudioVolume, RandomPitchValue());

			ri->shouldRender = false;
			ri->NumFramesDirty = gNumFrameResources;
			playerHealth.current += (healthBoxClass[counter].Consume() * 0.01f) / playerHealth.maximum;
		}
	}

	counter = -1;
	for (auto ri : mRitemLayer[(int)RenderLayer::ShieldBox])
	{
		counter++;

		auto geo = ri->geometry;
		if (ri->shouldRender == false)
			continue;

		if (shieldBox[counter].Contains(mCamera->GetPosition()))
		{
			mGameAudio.Play("PickupHealth", nullptr, false, mAudioVolume, RandomPitchValue());

			ri->shouldRender = false;
			ri->NumFramesDirty = gNumFrameResources;
			//Shield - Protect From 1 Hit For N Seconds
			shieldBoxClass[counter].Consume();
		}
	}

	counter = -1;
	for (auto ri : mRitemLayer[(int)RenderLayer::SpeedBox])
	{
		counter++;

		auto geo = ri->geometry;
		if (ri->shouldRender == false)
			continue;

		if (speedBox[counter].Contains(mCamera->GetPosition()))
		{
			mGameAudio.Play("PickupHealth", nullptr, false, mAudioVolume, RandomPitchValue());

			ri->shouldRender = false;
			ri->NumFramesDirty = gNumFrameResources;
			//Speed - Move 2x Speed For N Seconds
			speedBoxClass[counter].Consume();
		}
	}

	counter = -1;
	for (auto ri : mRitemLayer[(int)RenderLayer::QuadBox])
	{
		counter++;

		auto geo = ri->geometry;
		if (ri->shouldRender == false)
			continue;

		if (quadBox[counter].Contains(mCamera->GetPosition()))
		{
			mGameAudio.Play("PickupHealth", nullptr, false, mAudioVolume, RandomPitchValue());

			ri->shouldRender = false;
			ri->NumFramesDirty = gNumFrameResources;
			//Quad Damage - x4 Damage For N Seconds
			quadActive = true;
			//Start Timer Here \[T]/
			quadBoxClass[counter].Consume();
		}
	}

	counter = -1;
	for (auto ri : mRitemLayer[(int)RenderLayer::InfiniteBox])
	{
		counter++;

		auto geo = ri->geometry;
		if (ri->shouldRender == false)
			continue;

		if (infiniteBox[counter].Contains(mCamera->GetPosition()))
		{
			mGameAudio.Play("PickupHealth", nullptr, false, mAudioVolume, RandomPitchValue());

			ri->shouldRender = false;
			ri->NumFramesDirty = gNumFrameResources;
			//Infinite Ammo - Unlimited Ammo + No Reload For N Seconds
			infiniteActive = true;
			//Start Timer Here \[T]/
			infiniteBoxClass[counter].Consume();
		}
	}

}

void Application::PlayFootAudio(float dt)
{
	footStepTimer += dt;
	if (footStepTimer >= footStepInterval)
	{
		footStepTimer = 0;
		mGameAudio.Play("PlayerFootstep", nullptr, false, mAudioVolume, RandomPitchValue());
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Application::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Application::GetSpriteGpuDescHandle(const std::string& textureName)
{
	// get gpu start
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	//find offset distance
	const int distance = (int)std::distance(mTextures.begin(), mTextures.find(textureName));

	//offset desc
	hGpuDescriptor.Offset(distance, mCbvSrvUavDescriptorSize);

	return hGpuDescriptor;
}

float Application::GetGameTime() const
{
	return mTimer.TotalTime();
}

const UINT Application::GetCbvSrvDescriptorSize() const
{
	return mCbvSrvDescriptorSize;
}