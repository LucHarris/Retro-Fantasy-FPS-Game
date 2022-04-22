#include "AudioSystem.h"
#include <algorithm>
#include <iterator>

//
// Base Audio
//

DirectX::SOUND_EFFECT_INSTANCE_FLAGS BaseAudioChannel::GetInstanceFlags(DirectX::AudioEmitter* emitter)
{
	DirectX::SOUND_EFFECT_INSTANCE_FLAGS iflags = DirectX::SoundEffectInstance_Default;

	//if emitter provided then 3d sound filter applied 
	if (emitter)
		iflags = iflags | DirectX::SoundEffectInstance_Use3D;

	return iflags;
}

BaseAudioChannel::BaseAudioChannel(DirectX::AudioListener* listener)
	:
	mCacheLimit(1),
	pListener(listener),
	mType(AUDIO_CHANNEL_TYPE::SFX)
{

}

void BaseAudioChannel::Pause()
{
	mAudioEngine->Suspend();
}

void BaseAudioChannel::Resume()
{
	mAudioEngine->Resume();
}

void BaseAudioChannel::SetFade(float secs)
{
	mFadeInSecs = abs(secs);
}

void BaseAudioChannel::SetCacheLimit(size_t limit)
{
	mCacheLimit = limit;
}

void BaseAudioChannel::LoadSound(const std::string& soundName, const std::wstring& filename)
{
	//Loads a sound for the engine
	auto sound = std::make_unique<DirectX::SoundEffect>(mAudioEngine.get(), filename.c_str());
	assert(sound);
	mSounds[soundName] = std::move(sound);
}

AUDIO_CHANNEL_TYPE BaseAudioChannel::GetType()
{
	return mType;
}

void BaseAudioChannel::SetChannelVolume(float volume, bool increment)
{
	if (increment)
	{
		volume += mAudioEngine->GetMasterVolume();
	}

	// set limits
	if (volume < 0.0f)
	{
		volume = 0.0f;
	}
	else
	{
		if (volume > 1.0f)
		{
			volume = 1.0f;
		}
	}

	mAudioEngine->SetMasterVolume(volume);
}

const float BaseAudioChannel::GetChannelVolume()
{
	return mAudioEngine->GetMasterVolume();
}

//
// SFX
//

SfxChannel::SfxChannel(DirectX::AudioListener* listener)
	:
	BaseAudioChannel(listener),
	mForceAudio(true)
{
	mType = AUDIO_CHANNEL_TYPE::SFX;
	mCacheLimit = 1;
}

void SfxChannel::Init()
{

	DirectX::AUDIO_ENGINE_FLAGS eflags = DirectX::AudioEngine_Default | DirectX::AudioEngine_EnvironmentalReverb | DirectX::AudioEngine_ReverbUseFilters;

#ifdef _DEBUG
	eflags = eflags | DirectX::AudioEngine_Debug;
#endif

	//Setup engine for sfx
	mAudioEngine = std::make_unique<DirectX::AudioEngine>(eflags, nullptr, nullptr, AudioCategory_GameEffects);

}

void SfxChannel::Update(float gt)
{
	if (mAudioEngine->Update())
	{
		//Remove audio that has stopped
		mCache.remove_if([](const auto& c) {
			return c->GetState() == DirectX::SoundState::STOPPED;
			});
	}
	else
	{
		// No audio device is active
		if (mAudioEngine->IsCriticalError())
		{
			assert(false);
		}
	}

}

void SfxChannel::Play(const std::string& soundName, DirectX::AudioEmitter* emitter, bool loop, float volume, float pitch, float pan)
{
	bool play = true;

	if (mCache.size() >= max(mCacheLimit, 1))
	{
		if (mForceAudio)
			//if(mCache.size() > 0)
			mCache.pop_back();
		else
			play = false;
	}

	if (play)
	{
		assert(mSounds[soundName]);

		DirectX::SOUND_EFFECT_INSTANCE_FLAGS iflags = GetInstanceFlags(emitter);

		mCache.push_front(std::move(mSounds[soundName]->CreateInstance(iflags)));

		mCache.front()->Play(false); //no looping

		if (emitter)
		{
			//3D
			mCache.front()->Apply3D(*pListener, *emitter);
		}
		else
		{
			//Non-3D
			mCache.front()->SetVolume(volume);
			mCache.front()->SetPitch(pitch);
			mCache.front()->SetPan(pan);
		}
	}
	else
	{
		std::string str = "Warning - Unable to play '" + soundName + "' due to insufficent cache\n";
		OutputDebugStringA(str.c_str());
	}






}

void SfxChannel::ForceAudio(bool force)
{
	mForceAudio = force;
}

//
// Music 
//

MusicChannel::MusicChannel(DirectX::AudioListener* listener)
	:
	BaseAudioChannel(listener)
{
	mType = AUDIO_CHANNEL_TYPE::MUSIC;
	mCacheLimit = 2; // Only front and back
}

void MusicChannel::Init()
{
	DirectX::AUDIO_ENGINE_FLAGS eflags = DirectX::AudioEngine_Default;

#ifdef _DEBUG
	eflags = eflags | DirectX::AudioEngine_Debug;
#endif
	//Setup engine for music and ambience
	mAudioEngine = std::make_unique<DirectX::AudioEngine>(eflags, nullptr, nullptr, AudioCategory_GameMedia); // AudioCategory_GameMedia

	assert(mCache.size() == 0);

	std::generate_n(std::back_inserter(mCache), mCacheLimit, []()
		{

			return std::unique_ptr<DirectX::SoundEffectInstance>{};
		});

}

void MusicChannel::Update(float gt)
{


	if (mAudioEngine->Update())
	{
		assert(mCache.size() == mCacheLimit);

		//Increase volume for cache
		mNormalisedVolume += gt / mFadeInSecs;

		//Limit normalised volume
		if (mNormalisedVolume > 1.0f)
			mNormalisedVolume = 1.0f;
		else if (mNormalisedVolume < 0.0f)
			mNormalisedVolume = 0.0f;

		//Apply volume
		if (mCache.front())
			mCache.front()->SetVolume(mNormalisedVolume);
		if (mCache.back())
			mCache.back()->SetVolume((1.0f - mNormalisedVolume)); //compliment

	}
	else
	{
		if (mAudioEngine->IsCriticalError())
		{
			assert(false);
		}
	}



}

void MusicChannel::Play(const std::string& soundName, DirectX::AudioEmitter* emitter, bool loop, float volume, float pitch, float pan)
{
	assert(mSounds[soundName]);

	// Doesn't fade into the same track
	if (soundName != mFrontAudio)
	{
		if (soundName.size() >= MusicChannel::MAX_AUDIONAME)
		{
			// music name too long
			assert(false);
		}
		
		strcpy_s(mFrontAudio, soundName.c_str());

		SwapCache(); //Current front audio swapped with last

		DirectX::SOUND_EFFECT_INSTANCE_FLAGS iflags = GetInstanceFlags(emitter);

		//New instance to first cache element
		mCache.front() = std::move(mSounds[soundName]->CreateInstance(iflags));

		mCache.front()->Play(loop);

		//Set properties
		if (emitter && pListener)
		{
			mCache.front()->Apply3D(*pListener, *emitter);
		}
		else
		{
			mCache.front()->SetVolume(mNormalisedVolume);
			mCache.front()->SetPitch(pitch);
			mCache.front()->SetPan(pan);
		}
	}



}

void MusicChannel::SwapCache()
{
	mCache.front().swap(mCache.back());
	mNormalisedVolume = 1.0f - mNormalisedVolume;//swap audio volume 
}


//
// Game Audio 
//

bool AudioSystem::ValidChannel(const std::string& name)
{
	if (mChannels.count(name) == 1)
	{
		return true;
	}
	else
	{
		std::string str = "Error - SoundEngine '" + name + "' does not exist.\n";
		OutputDebugStringA(str.c_str());
		return false;
	}
}

bool AudioSystem::ValidKeys(const std::string& channelName, const std::string& soundName)
{
	bool b = false;

	if (mkeys.count(soundName) == 1)
	{
		if (mkeys[soundName] == channelName)
		{
			b = true;
		}
		else
		{
			std::string str = "Error - Sound '" + soundName + "' is not contained within '" + channelName + "'. Contained in '" + mkeys[soundName] + "'.\n";
			OutputDebugStringA(str.c_str());
		}
	}
	else
	{
		std::string str = "Error - Sound '" + soundName + "' does not exist.\n ";
		OutputDebugStringA(str.c_str());
	}
	assert(b);
	return b;
}

AudioSystem::~AudioSystem()
{
	OutputDebugStringA("~AudioSystem   \n");
	mChannels.clear();
}

void AudioSystem::Init()
{
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		assert(false);
	}
	mCone = X3DAudioDefault_DirectionalCone;

}

void AudioSystem::CreateChannel(const std::string& name, const AUDIO_CHANNEL_TYPE& type)
{
	assert(name.length() > 0);

	if (!mChannels[name])
	{
		switch (type)
		{
		case SFX:	mChannels[name] = std::make_unique<SfxChannel>(&mListener); break;
		case MUSIC:	mChannels[name] = std::make_unique<MusicChannel>(&mListener); break;
		default:			assert(false);
		}

		mChannels[name]->Init();
	}
	else
	{
		assert(false);
	}
}

void AudioSystem::Update(float gt, const DirectX::XMFLOAT3& camPos, const DirectX::XMFLOAT3& camForward, const DirectX::XMFLOAT3& camUp)
{
	//Camera as listener
	mListener.SetPosition(camPos);
	mListener.SetOrientation(camForward, camUp);

	for (auto& engine : mChannels)
	{
		engine.second->Update(gt);
	}
}

void AudioSystem::Play(const std::string& soundName, DirectX::AudioEmitter* emitter, bool loop, float volume, float pitch, float pan)
{
	if (mkeys.count(soundName) == 1)
	{
		mChannels[mkeys[soundName]]->Play(soundName, emitter, loop, volume, pitch, pan);
	}
	else
	{
		std::string msg = "Error - Audio cannot play: " + soundName + "\n";
		OutputDebugStringA(msg.c_str());
		assert(false);
	}


}

void AudioSystem::Pause(const std::string& channelName)
{
	if (ValidChannel(channelName))
		mChannels[channelName]->Pause();
}

void AudioSystem::Resume(const std::string& channelName)
{
	if (ValidChannel(channelName))
		mChannels[channelName]->Resume();
}

void AudioSystem::PauseAll()
{
	std::for_each(mChannels.begin(), mChannels.end(), [](auto& e)
		{
			e.second->Pause();
		});
}

void AudioSystem::ResumeAll()
{
	std::for_each(mChannels.begin(), mChannels.end(), [](auto& e)
		{
			e.second->Resume();
		});
}

void AudioSystem::LoadSound(const std::string& channelName, const std::string& soundName, const std::wstring& filename)
{
	std::string debug = "";


	//Check sound doesnt already exist
	if (mkeys.count(soundName) == 0)
	{
		//Create an SFX engine if one doesn't exist
		if (!ValidChannel(channelName))
		{
			CreateChannel(channelName, SFX);
			OutputDebugStringA("Warning - Engine not created. Default SFX_Engine created");
		}

		//Adds keys to map
		mkeys[soundName] = channelName;

		//Add audio to engine
		mChannels[channelName]->LoadSound(soundName, filename);

		std::string debug = "Success - Audio loaded: " + soundName + "on engine " + channelName + "\n";
	}
	else
	{
		std::string debug = "Warning - Audio already loaded: " + soundName + "on engine " + channelName + "\n";
	}

	OutputDebugStringA(debug.c_str());
}

void AudioSystem::SetCacheSize(const std::string& name, size_t limit)
{
	assert(ValidChannel(name));

	switch (mChannels[name]->GetType())
	{
	case AUDIO_CHANNEL_TYPE::SFX:	mChannels[name]->SetCacheLimit(limit); break;
	case AUDIO_CHANNEL_TYPE::MUSIC:	OutputDebugStringA("Warning - Cannot change cache size of music audio engines.\n"); break;

	}
}

void AudioSystem::ForceAudio(const std::string& name, bool force)
{
	if (ValidChannel(name))
	{
		switch (mChannels[name]->GetType())
		{
		case AUDIO_CHANNEL_TYPE::SFX:	mChannels[name]->ForceAudio(force); break;
		case AUDIO_CHANNEL_TYPE::MUSIC:	OutputDebugStringA("Warning - No forcing audio for music audio engines.\n"); break;
		}
	}

}

void AudioSystem::SetFade(const std::string& channelName, float secs)
{
	if (ValidChannel(channelName))
	{

		switch (mChannels[channelName]->GetType())
		{
			case AUDIO_CHANNEL_TYPE::MUSIC:	mChannels[channelName]->SetFade(secs); break;
			case AUDIO_CHANNEL_TYPE::SFX:	OutputDebugStringA("Warning - SFX audio engines do not support fading.\n"); break;
		}

	}
}

void AudioSystem::SetChannelVolume(const std::string& channelName, float volume, bool increment)
{
	if (ValidChannel(channelName))
	{
		mChannels[channelName]->SetChannelVolume(volume, increment);
	}
}

void AudioSystem::SetVolume(float volume, bool increment)
{
	for (auto& e : mChannels)
	{
		e.second->SetChannelVolume(volume, increment);
	}
}

float RandomPitchValue()
{
	return static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - 0.5f;;
}
