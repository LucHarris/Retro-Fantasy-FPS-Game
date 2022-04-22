#include "SpriteSystem.h"
#include "Application.h" // access to Application through pointer
#include "ResourceUploadBatch.h"
#include <CommonStates.h>
#include "Constants.h"

#ifndef UI_SPRITE_RENDER_TOGGLE
#define	UI_SPRITE_RENDER_TOGGLE 0
#endif // !UI_SPRITE_RENDER_TOGGLE

#ifndef UI_TEXT_RENDER_TOGGLE
#define	UI_TEXT_RENDER_TOGGLE 0
#endif // !UI_SPRITE_RENDER_TOGGLE


bool Sprite::Initialise(Application* const app, const std::string& textureName, bool centreOrigin)
{
   texture = app->GetSpriteGpuDescHandle(textureName);

   if (centreOrigin)
   {
	   origin.x = 0.5f * (float)(sourceRectangle.right - sourceRectangle.left);
	   origin.y = 0.5f * (float)(sourceRectangle.bottom - sourceRectangle.top);
   }

   return true;
}

bool UI::Initialise(Application* const app)
{
	//todo assign textures to sprites, init rects
	for (auto& s : sprites)
	{
		s.Initialise(app, "terrainTex", false); // todo change tex name
		s.visible = true;
	}

	//todo init text array properties
	for (auto& t : texts)
	{
		t.visible = true;
		t.string = "TEXTS";
	}

	texts.at(0).string = "";
	texts.at(0).visible = true;
	texts.at(0).fontIndex = 0;

	return true;
}

void SpriteSystem::Initialise(Application* const app,ID3D12Device* device, ID3D12CommandQueue* commandQueue, UINT srvDescSize, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilFormat, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	const UINT size = app->GetCbvSrvDescriptorSize();

	graphicsMemory = std::make_unique<DirectX::GraphicsMemory>(device);

	DirectX::ResourceUploadBatch resourceUpload(device);

	resourceUpload.Begin();

	{
		DirectX::RenderTargetState rtState(backBufferFormat, depthStencilFormat);
		DirectX::SpriteBatchPipelineStateDescription sbPipelineDesc(rtState, &CommonStates::NonPremultiplied, nullptr, nullptr);

		spriteBatch = std::make_unique<DirectX::SpriteBatch>(device, resourceUpload, sbPipelineDesc);

		size_t fontIndex = 0;

		spriteFont.at(fontIndex++) = std::make_unique<DirectX::SpriteFont>(
			device,
			resourceUpload,
			L"Data/Fonts/arial_16.spritefont",
			cpuHandle,
			gpuHandle
			);

		// todo add more textures
		// 
		// cpuHandle.Offset(size);
		// gpuHandle.Offset(size);
		// 
		// spriteFont.at(fontIndex++) = std::make_unique<DirectX::SpriteFont>( ...... see above ..... )

		assert(fontIndex == NUM_FONT);
	}

	auto uploadResourceEnd = resourceUpload.End(commandQueue);
	uploadResourceEnd.wait();
}

void UI::Update(Application* const app, float dt)
{
	// todo put screenspace text/sprite updates here
	texts.at(TEXT_TIMER).string = "Total Time" + std::to_string(app->GetGameTime());
}

void SpriteSystem::Draw(ID3D12GraphicsCommandList* commandList, const D3D12_VIEWPORT& viewport, const UI& ui)
{
	// start
	{
		spriteBatch->SetViewport(viewport);
		spriteBatch->Begin(commandList);
	}

	// body
	
	{
#if UI_SPRITE_RENDER_TOGGLE
		// render sprites
		for (auto s : ui.sprites)
		{
			if (s.visible)
			{
				if (s.destinationRectangle.left < 0)
				{
					spriteBatch->Draw(
						s.texture,
						s.textureSize,
						s.position,
						&s.sourceRectangle,
						s.color,
						s.rotation,
						s.origin,
						s.scale,
						s.effects
					);
				}
				else
				{
					spriteBatch->Draw(
						s.texture,
						s.textureSize,
						s.destinationRectangle,
						&s.sourceRectangle,
						s.color,
						s.rotation,
						s.origin,
						s.effects
					);
				}
			}

		}
#endif

#if UI_TEXT_RENDER_TOGGLE
		// render text on top of sprites
		for (auto t : ui.texts)
		{
			if (t.visible)
			{
				const char* string = t.string.c_str();

				const DirectX::SimpleMath::Vector2 origin = (t.center) ?
					spriteFont.at(t.fontIndex)->MeasureString(string, true) * 0.5f : //center 
					t.origin;															// predefined origin

				spriteFont.at(t.fontIndex)->DrawString(spriteBatch.get(), string, t.position, t.color, t.rotation, origin, t.scale);
			}
		}
#endif

	}
	
	// end
	{
		spriteBatch->End();
	}

}

void UICharLine::Init(
	Application* const app, 
	uint32_t ritemOffset, 
	uint32_t length, 
	const DirectX::SimpleMath::Vector3& pos,
	int decimalSymbol, 
	int postfixSymbol, 
	int decimalPlace)
{
	ritemStart = ritemOffset;
	len = length;
	this->decimalPlace = decimalPlace;
	this->postfixSymbol = postfixSymbol;
	this->decimalSymbol = decimalSymbol;
	
	auto ir = app->GetAllRItems();

	assert((size_t)(ritemStart + len-1) < ir->size());

	DirectX::SimpleMath::Vector3 tempPos = pos;

	postfixIndex = ritemStart + (len - 1);
	decimalIndex = postfixIndex - decimalPlace - 1;

	assert(ritemStart < postfixIndex);
	assert(ritemStart < decimalIndex);

	// set position 
	for (uint32_t i = 0; i < length; i++)
	{
		uint32_t rIndex = ritemStart + i;
		tempPos.x += gc::UI_CHAR_SPACING;
		XMStoreFloat4x4(&ir->at(rIndex).get()->position, DirectX::SimpleMath::Matrix::CreateTranslation(tempPos));
	}

	// set uv for decimal sym and postfix sym
	{
		tempPos = Vector3::Zero;
		tempPos.y += postfixSymbol * gc::UI_CHAR_INC;
		XMStoreFloat4x4(&ir->at(postfixIndex).get()->texTransform, DirectX::SimpleMath::Matrix::CreateTranslation(tempPos));
	}

	{
		tempPos = Vector3::Zero;
		tempPos.y += decimalSymbol * gc::UI_CHAR_INC;
		XMStoreFloat4x4(&ir->at(decimalIndex).get()->texTransform, DirectX::SimpleMath::Matrix::CreateTranslation(tempPos));
	}

}

void UICharLine::Update(Application* const app, float dt, float displayValue)
{
	auto ir = app->GetAllRItems();

	// last element
	uint32_t rIndex = ritemStart + len - 1;

	float dec = (float)-decimalPlace;

	int start = -decimalPlace;
	int end = len - decimalPlace ;

	for (int i = start; i < end; i++)
	{
		if (rIndex != decimalIndex && rIndex != postfixIndex)
		{

			float p = pow(10.0f, dec);
			float q = p * 10.0f;

			float v = fmodf( displayValue , q);
			v = v / p;
			v = max(floorf(v),0.0f);
			{
				Vector3 tempPos = Vector3::Zero;
				tempPos.y += v * gc::UI_CHAR_INC;
				XMStoreFloat4x4(
					&ir->at(rIndex).get()->texTransform, 
					DirectX::SimpleMath::Matrix::CreateTranslation(tempPos));
				ir->at(rIndex).get()->NumFramesDirty = 1;

				--rIndex;
			}

			dec = dec + 1.0f;
		}
		else
		{
			--rIndex;
		}
	}

	end = 0;

}

void UISprite::Init(
	Application* const app, 
	uint32_t ritemOffset, 
	const DirectX::SimpleMath::Vector3& onPosition, 
	bool visible, 
	const DirectX::SimpleMath::Vector3& uvw)
{
	ritemIndex = ritemOffset;

	auto ir = app->GetAllRItems();

	assert(ritemIndex < ir->size());

	pos[OFF] = gc::UI_OFF_POS;
	pos[ON] = onPosition;

	XMStoreFloat4x4(
		&ir->at(ritemIndex).get()->position,
		DirectX::SimpleMath::Matrix::CreateTranslation((visible) ? pos[ON] : pos[OFF]));

	XMStoreFloat4x4(
		&ir->at(ritemIndex).get()->texTransform,
		DirectX::SimpleMath::Matrix::CreateTranslation(uvw));
}

void UISprite::SetDisplay(Application* const app, bool visible)
{
	auto ir = app->GetAllRItems();
	assert(ritemIndex < ir->size());

	XMStoreFloat4x4(
		&ir->at(ritemIndex).get()->position, 
		DirectX::SimpleMath::Matrix::CreateTranslation((visible) ? pos[ON] : pos[OFF]));
	// set to update in objectbuffer
	ir->at(ritemIndex).get()->NumFramesDirty = 1;
}

void UISprite::SetXScale(Application* const app, float scale, float dt)
{
	auto ir = app->GetAllRItems();
	assert(ritemIndex < ir->size());

	// smooth transition
	float a = ir->at(ritemIndex).get()->position._11;
	float b = scale;
	float s = dt;
	float x = a * (1.0f - s) + b * s;

	ir->at(ritemIndex).get()->position._11 = x;
	ir->at(ritemIndex).get()->NumFramesDirty = 1;
}
