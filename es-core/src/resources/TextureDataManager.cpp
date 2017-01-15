#include "resources/TextureDataManager.h"
#include "Settings.h"

TextureDataManager::TextureDataManager()
{
	unsigned char data[5 * 5 * 4];
	mBlank = std::shared_ptr<TextureData>(new TextureData(false));
	for (int i = 0; i < (5 * 5); ++i)
	{
		data[i*4] = (i % 2) * 255;
		data[i*4+1] = (i % 2) * 255;
		data[i*4+2] = (i % 2) * 255;
		data[i*4+3] = 255;
	}
	mBlank->initFromRGBA(data, 5, 5);
	mLoader = new TextureLoader;
}

TextureDataManager::~TextureDataManager()
{
	delete mLoader;
}

std::shared_ptr<TextureData> TextureDataManager::add(const TextureResource* key, bool tiled)
{
	remove(key);
	std::shared_ptr<TextureData> data(new TextureData(tiled));
	mTextures.push_front(data);
	mTextureLookup[key] = mTextures.begin();
	return data;
}

void TextureDataManager::remove(const TextureResource* key)
{
	// Find the entry in the list
	auto it = mTextureLookup.find(key);
	if (it != mTextureLookup.end())
	{
		// Remove the list entry
		mTextures.erase((*it).second);
		// And the lookup
		mTextureLookup.erase(it);
	}
}

std::shared_ptr<TextureData> TextureDataManager::get(const TextureResource* key)
{
	// If it's in the cache then we want to remove it from it's current location and
	// move it to the top
	std::shared_ptr<TextureData> tex;
	auto it = mTextureLookup.find(key);
	if (it != mTextureLookup.end())
	{
		tex = *(*it).second;
		// Remove the list entry
		mTextures.erase((*it).second);
		// Put it at the top
		mTextures.push_front(tex);
		// Store it back in the lookup
		mTextureLookup[key] = mTextures.begin();

		// Make sure it's loaded or queued for loading
		load(tex);
	}
	return tex;
}

void TextureDataManager::bind(const TextureResource* key)
{
	std::shared_ptr<TextureData> tex = get(key);
	bool bound = false;
	if (tex != nullptr)
		bound = tex->uploadAndBind();
	if (!bound)
		mBlank->uploadAndBind();
}

size_t TextureDataManager::getTotalSize()
{
	size_t total = 0;
	for (auto tex : mTextures)
		total += tex->width() * tex->height() * 4;
	return total;
}

size_t TextureDataManager::getCommittedSize()
{
	size_t total = 0;
	for (auto tex : mTextures)
		total += tex->getVRAMUsage();
	return total;
}

void TextureDataManager::load(std::shared_ptr<TextureData> tex)
{
	// See if it's already loaded
	if (tex->isLoaded())
		return;
	// Not loaded. Make sure there is room
	size_t size = getCommittedSize();
	size_t max_texture = (size_t)Settings::getInstance()->getInt("MaxVRAM") * 1024 * 1024;

	for (auto it = mTextures.rbegin(); it != mTextures.rend(); ++it)
	{
		if (size < max_texture)
			break;
		size -= (*it)->getVRAMUsage();
		(*it)->releaseVRAM();
		(*it)->releaseRAM();
	}
	mLoader->load(tex);
	//tex->load();
}

TextureLoader::TextureLoader() : mThread(&TextureLoader::threadProc, this), mExit(false)
{
}

TextureLoader::~TextureLoader()
{
	mExit = true;
	mEvent.notify_one();
	mThread.join();
}

void TextureLoader::threadProc()
{
	while (!mExit)
	{
		std::shared_ptr<TextureData> textureData;
		{
			// Wait for an event to say there is something in the queue
			std::unique_lock<std::mutex> lock(mMutex);
			mEvent.wait(lock);
			if (!mTextureDataQ.empty())
			{
				textureData = mTextureDataQ.back();
				mTextureDataQ.pop_back();
			}
		}
		// Queue has been released here but we might have a texture to process
		while (textureData)
		{
			textureData->load();

			// See if there is another item in the queue
			textureData = nullptr;
			std::unique_lock<std::mutex> lock(mMutex);
			if (!mTextureDataQ.empty())
			{
				textureData = mTextureDataQ.back();
				mTextureDataQ.pop_back();
			}
		}
	}
}

void TextureLoader::load(std::shared_ptr<TextureData> textureData)
{
	// Make sure it's not already loaded
	if (!textureData->isLoaded())
	{
		mTextureDataQ.push_front(textureData);
		mEvent.notify_one();
	}
}
