#pragma once

#include <string>
#include <array>
#include <SpriteFont.h>
#include "SimpleMath.h"
#include <GraphicsMemory.h>
#include "Common/d3dx12.h" //  CD3DX12_CPU_DESCRIPTOR_HANDLE and CD3DX12_GPU_DESCRIPTOR_HANDLE


#ifndef NUM_FONT
#define NUM_FONT 1u
#endif // !FONT_SIZE


class Application;

// floating point number display (unsigned)
class UICharLine
{

	// offset in renderitem vector
	uint32_t ritemStart = 0;
	// number of chars in array
	uint32_t len = 0;
	// char index
	int decimalSymbol;
	// char index
	int postfixSymbol;
	int decimalPlace;

	uint32_t decimalIndex;
	uint32_t postfixIndex;
public:
	void Init(Application* const app, uint32_t ritemOffset, uint32_t length, const DirectX::SimpleMath::Vector3& pos, int decimalSymbol = 11, int postfixSymbol = 12, int decimalPlace = 2);
	void Update(Application* const app, float dt, float dislpayValue);
};



class UISprite
{
public:
	enum { OFF, ON, COUNT };
private:
	uint32_t ritemIndex = 0;
public:
	DirectX::SimpleMath::Vector3 pos[COUNT];
	// call when setting up ritems
	// uwv for 'words' only
	void Init(Application* const app, uint32_t ritemOffset, const DirectX::SimpleMath::Vector3& pos, bool visible = true, const DirectX::SimpleMath::Vector3& uvw = DirectX::SimpleMath::Vector3::Zero);
	// call before UpdateObjectBuffer
	void SetDisplay(Application* const app, bool display);

	void SetXScale(Application* const app, float scale, float dt);
	
};













// depricated
struct Text
{
	std::string string = "";
	DirectX::SimpleMath::Vector2 position = { 0,0 };
	DirectX::SimpleMath::Vector4 color = { 0.0f,0.0f,0.0f,1.0f };
	float rotation = 0.0f;
	DirectX::SimpleMath::Vector2 origin = { 0,0 };
	float scale = 1.0f;
	size_t fontIndex = 0;
	bool center = false;
	bool visible = false;

	Text() = default;
	Text(const Text&) = default;
};
// depricated
struct Sprite
{
	std::string textureName; //lookup gpu handle
	D3D12_GPU_DESCRIPTOR_HANDLE texture;
	DirectX::XMUINT2 textureSize = DirectX::XMUINT2(512, 512);
	DirectX::XMFLOAT2 position = { 0.0f,0.0f };
	RECT destinationRectangle = { -1,-1,-1,-1 }; //ignore if left = -1
	RECT sourceRectangle = { 0,0,512,512 };
	DirectX::SimpleMath::Vector4 color = DirectX::Colors::White;
	float rotation = 0.0f;
	float scale = 1.0f;
	DirectX::XMFLOAT2 origin = DirectX::XMFLOAT2(0.0f, 0.0f);
	DirectX::SpriteEffects effects = DirectX::SpriteEffects::SpriteEffects_None;
	bool visible = false;

	Sprite() = default;
	Sprite(const Sprite&) = default;

	bool Initialise( Application* const app, const std::string& textureName, bool centreOrigin = false);

};


// container for text and sprites
// depricated
struct UI
{
	// index for text/spr containers
	enum {
		TEXT_TIMER = 0,
		//TEXT_SOMETHING
		SPR_IMAGE = 0
	};
	std::array<Sprite, 0> sprites;
	std::array<Text, 1> texts;

	// define and initial text and sprite elements
	bool Initialise(Application* const app);

	void Update(Application* const app, float dt);

};

// manages rendering of screen space sprites and text 
// depricated
class SpriteSystem
{
private:
	//required for draw begin/end
	std::unique_ptr<DirectX::GraphicsMemory> graphicsMemory;
	std::unique_ptr<DirectX::SpriteBatch> spriteBatch;
	std::array<std::unique_ptr<DirectX::SpriteFont>, NUM_FONT> spriteFont;
public:
	SpriteSystem() = default;
	void Initialise(
		Application* const app,
		ID3D12Device* device, 
		ID3D12CommandQueue* commandQueue, 
		UINT srvDescSize, 
		DXGI_FORMAT backBufferFormat, 
		DXGI_FORMAT depthStencilFormat, 
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle, 
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle);


	// draw all visible elements for UI 
	void Draw(
		ID3D12GraphicsCommandList* commandList,
		const D3D12_VIEWPORT& viewport, 
		const UI& ui
	);


};

