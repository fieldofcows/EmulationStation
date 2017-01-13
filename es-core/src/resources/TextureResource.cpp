#include "resources/TextureResource.h"
#include "Log.h"
#include "platform.h"
#include GLHEADER
#include "ImageIO.h"
#include "Renderer.h"
#include "Util.h"
#include "Settings.h"

TextureDataManager		TextureResource::sTextureDataManager;
std::map< TextureResource::TextureKeyType, std::weak_ptr<TextureResource> > TextureResource::sTextureMap;
std::set<TextureResource*> 	TextureResource::sAllTextures;

TextureResource::TextureResource(const std::string& path, bool tile)
{
	// Create a texture data object for this texture
	std::shared_ptr<TextureData> data = sTextureDataManager.add(this, tile);
	if (!path.empty())
		data->initFromPath(path);
	sAllTextures.insert(this);
}

TextureResource::~TextureResource()
{
	sTextureDataManager.remove(this);
	sAllTextures.erase(sAllTextures.find(this));
}

void TextureResource::initFromPixels(const unsigned char* dataRGBA, size_t width, size_t height)
{
	std::shared_ptr<TextureData> data = sTextureDataManager.get(this);
	data->releaseVRAM();
	data->releaseRAM();
	data->initFromRGBA(dataRGBA, width, height);
}

void TextureResource::initFromMemory(const char* data, size_t length)
{
	std::shared_ptr<TextureData> textureData = sTextureDataManager.get(this);
	textureData->releaseVRAM();
	textureData->releaseRAM();
	textureData->initImageFromMemory((const unsigned char*)data, length);
}

const Eigen::Vector2i TextureResource::getSize() const
{
	Eigen::Vector2i ret;
	std::shared_ptr<TextureData> data = sTextureDataManager.get(this);
	ret << data->width(), data->height();
	return ret;
}

bool TextureResource::isTiled() const
{
	std::shared_ptr<TextureData> data = sTextureDataManager.get(this);
	return data->tiled();
}

void TextureResource::bind()
{
	std::shared_ptr<TextureData> data = sTextureDataManager.get(this);
	data->uploadAndBind();
}

std::shared_ptr<TextureResource> TextureResource::get(const std::string& path, bool tile)
{
	std::shared_ptr<ResourceManager>& rm = ResourceManager::getInstance();

	const std::string canonicalPath = getCanonicalPath(path);
	if(canonicalPath.empty())
	{
		std::shared_ptr<TextureResource> tex(new TextureResource("", tile));
		rm->addReloadable(tex); //make sure we get properly deinitialized even though we do nothing on reinitialization
		return tex;
	}

	TextureKeyType key(canonicalPath, tile);
	auto foundTexture = sTextureMap.find(key);
	if(foundTexture != sTextureMap.end())
	{
		if(!foundTexture->second.expired())
			return foundTexture->second.lock();
	}

	// need to create it
	std::shared_ptr<TextureResource> tex;
	tex = std::shared_ptr<TextureResource>(new TextureResource(key.first, tile));
	std::shared_ptr<TextureData> data = sTextureDataManager.get(tex.get());
	data->load();

	// is it an SVG?
	if(key.first.substr(key.first.size() - 4, std::string::npos) != ".svg")
	{
		// Probably not. Add it to our map. We don't add SVGs because 2 svgs might be rasterized at different sizes
		sTextureMap[key] = std::weak_ptr<TextureResource>(tex);
	}
	return tex;
}

// For scalable source images in textures we want to set the resolution to rasterize at
void TextureResource::rasterizeAt(size_t width, size_t height)
{
	std::shared_ptr<TextureData> data = sTextureDataManager.get(this);
	data->setSourceSize((float)width, (float)height);
	data->load();
}

Eigen::Vector2f TextureResource::getSourceImageSize() const
{
	Eigen::Vector2f ret;
	std::shared_ptr<TextureData> data = sTextureDataManager.get(this);
	ret << data->sourceWidth(), data->sourceHeight();
	return ret;
}

bool TextureResource::isInitialized() const
{
	return true;
}

size_t TextureResource::getMemUsage() const
{
	return 0;
}

size_t TextureResource::getTotalMemUsage()
{
	return 0;
}

size_t TextureResource::getTotalTextureSize()
{
	return 0;
}


TextureResourceCache::TextureResourceCache()
{
}

TextureResourceCache::~TextureResourceCache()
{
}

void TextureResourceCache::add(TextureResource* tex)
{
	// If it's in the cache then we want to remove it from it's current location
	remove(tex);
	// Add it to the front of the cache
	mCacheEntries.push_front(tex);
	mCacheLookup[tex] = mCacheEntries.begin();
}

void TextureResourceCache::remove(TextureResource* tex)
{
	// Find the entry in the list
	auto it = mCacheLookup.find(tex);
	if (it != mCacheLookup.end())
	{
		// Remove the list entry
		mCacheEntries.erase((*it).second);
		// And the lookup
		mCacheLookup.erase(it);
	}
}

TextureResource* TextureResourceCache::purgeOldest()
{
	// Pop the oldest one off the back
	TextureResource* tex = nullptr;
	if (mCacheEntries.size() > 0)
	{
		tex = mCacheEntries.back();
		mCacheEntries.pop_back();
		// Remove it from the lookup as well
		auto it = mCacheLookup.find(tex);
		if (it != mCacheLookup.end())
			mCacheLookup.erase(it);
	}
	return tex;
}
