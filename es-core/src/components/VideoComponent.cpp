#include "components/VideoComponent.h"
#include "Renderer.h"
#include "ThemeData.h"
#include "Util.h"
#include "resources/ShaderI420.h"
#ifdef WIN32
#include <codecvt>
#endif

#define FADE_TIME_MS	200

libvlc_instance_t*		VideoComponent::mVLC = NULL;

// VLC prepares to render a video frame.
static void *lock(void *data, void **p_pixels) {
    struct VideoContext *c = (struct VideoContext *)data;
    SDL_LockMutex(c->mutex);
	p_pixels[0] = c->bands[0];
	p_pixels[1] = c->bands[1];
	p_pixels[2] = c->bands[2];
    return NULL; // Picture identifier, not needed here.
}

// VLC just rendered a video frame.
static void unlock(void *data, void *id, void *const *p_pixels) {
    struct VideoContext *c = (struct VideoContext *)data;
    SDL_UnlockMutex(c->mutex);
    c->dataAvail = true;
}

// VLC wants to display a video frame.
static void display(void *data, void *id) {
    //Data to be displayed
}

VideoComponent::VideoComponent(Window* window) :
	GuiComponent(window),
	mStaticImage(window),
	mMediaPlayer(nullptr),
	mVideoHeight(0),
	mVideoWidth(0),
	mStartDelayed(false),
	mIsPlaying(false),
	mShowing(false)
{
	memset(&mContext, 0, sizeof(mContext));

	// Setup the default configuration
	mConfig.showSnapshotDelay 		= false;
	mConfig.showSnapshotNoVideo		= false;
	mConfig.startDelay				= 0;

	// Make sure VLC has been initialised
	setupVLC();
}

VideoComponent::~VideoComponent()
{
	// Stop any currently running video
	stopVideo();
}

void VideoComponent::setOrigin(float originX, float originY)
{
	mOrigin << originX, originY;

	// Update the embeded static image
	mStaticImage.setOrigin(originX, originY);
}

Eigen::Vector2f VideoComponent::getCenter() const
{
	return Eigen::Vector2f(mPosition.x() - (getSize().x() * mOrigin.x()) + getSize().x() / 2,
		mPosition.y() - (getSize().y() * mOrigin.y()) + getSize().y() / 2);
}

void VideoComponent::onSizeChanged()
{
	// Update the embeded static image
	mStaticImage.onSizeChanged();
}

bool VideoComponent::setVideo(std::string path)
{
	// Convert the path into a format VLC can understand
	boost::filesystem::path fullPath = getCanonicalPath(path);
	fullPath.make_preferred().native();

	// Check that it's changed
	if (fullPath == mVideoPath)
		return !path.empty();

	// Store the path
	mVideoPath = fullPath;

	// If the file exists then set the new video
	if (!fullPath.empty() && ResourceManager::getInstance()->fileExists(fullPath.generic_string()))
	{
		// Return true to show that we are going to attempt to play a video
		return true;
	}
	// Return false to show that no video will be displayed
	return false;
}

void VideoComponent::setImage(std::string path)
{
	// Check that the image has changed
	if (path == mStaticImagePath)
		return;
	
	mStaticImage.setImage(path);
	// Make the image stretch to fill the video region
	mStaticImage.setSize(getSize());
	mFadeIn = 0.0f;
	mStaticImagePath = path;
}

void VideoComponent::setDefaultVideo()
{
	setVideo(mConfig.defaultVideoPath);
}

void VideoComponent::setOpacity(unsigned char opacity)
{
	mOpacity = opacity;
	// Update the embeded static image
	mStaticImage.setOpacity(opacity);
}

void VideoComponent::render(const Eigen::Affine3f& parentTrans)
{
	Eigen::Affine3f trans = parentTrans * getTransform();
	GuiComponent::renderChildren(trans);

	Renderer::setMatrix(trans);
	
    // Handle the case where the video is delayed
    handleStartDelay();

    // Handle looping of the video
    handleLooping();

	if (mIsPlaying && mContext.valid && mContext.dataAvail)
	{
		// Setup the shader to translate from RGB to I420
		ShaderI420* shader = dynamic_cast<ShaderI420*>(ResourceManager::getInstance()->shader(ResourceManager::SHADER_I420));
		shader->textures(0, 1, 2);

		glEnable(GL_TEXTURE_2D);

		// Setup the textures that have come from the VLC frame
		SDL_LockMutex(mContext.mutex);
		// Y is full width and height, U and V are half width and height
		GLint widths[3] = { (GLint)mVideoWidth, (GLint)mVideoWidth / 2, (GLint)mVideoWidth / 2 };
		GLint heights[3] = { (GLint)mVideoHeight, (GLint)mVideoHeight / 2, (GLint)mVideoHeight / 2 };
		for (int i = 2; i >= 0; --i)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, mContext.textures[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, widths[i], heights[i], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mContext.bands[i]);
		}
		SDL_UnlockMutex(mContext.mutex);

		// Set the fade in the shader
		shader->fadeIn(mFadeIn);

		// Shader expects vertices and texture coords
        glVertexAttribPointer(ShaderI420::ATTRIBUTE_VERTEX, 2, GL_FLOAT, 0, sizeof(VideoVertex), &mVertices[0].pos);
        glEnableVertexAttribArray(ShaderI420::ATTRIBUTE_VERTEX);
        glVertexAttribPointer(ShaderI420::ATTRIBUTE_TEXCOORD, 2, GL_FLOAT, 0, sizeof(VideoVertex), &mVertices[0].tex);
        glEnableVertexAttribArray(ShaderI420::ATTRIBUTE_TEXCOORD);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisable(GL_TEXTURE_2D);
		shader->endUse();
	}
	else
	{
		// This is the case where the video is not currently being displayed. Work out
		// if we need to display a static image
		if ((mConfig.showSnapshotNoVideo && mVideoPath.empty()) || (mStartDelayed && mConfig.showSnapshotDelay))
		{
			// Display the static image instead
			mStaticImage.setOpacity((unsigned char)(mFadeIn * 255.0f));
			mStaticImage.render(parentTrans);
		}
	}

}

void VideoComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	using namespace ThemeFlags;

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "video");
	if(!elem)
	{
		return;
	}

	Eigen::Vector2f scale = getParent() ? getParent()->getSize() : Eigen::Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	if ((properties & POSITION) && elem->has("pos"))
	{
		Eigen::Vector2f denormalized = elem->get<Eigen::Vector2f>("pos").cwiseProduct(scale);
		setPosition(Eigen::Vector3f(denormalized.x(), denormalized.y(), 0));
	}

	if ((properties & ThemeFlags::SIZE) && elem->has("size"))
	{
		setSize(elem->get<Eigen::Vector2f>("size").cwiseProduct(scale));
	}

	// position + size also implies origin
	if (((properties & ORIGIN) || ((properties & POSITION) && (properties & ThemeFlags::SIZE))) && elem->has("origin"))
		setOrigin(elem->get<Eigen::Vector2f>("origin"));

	if(elem->has("default"))
		mConfig.defaultVideoPath = elem->get<std::string>("default");

	if((properties & ThemeFlags::DELAY) && elem->has("delay"))
		mConfig.startDelay = (unsigned)(elem->get<float>("delay") * 1000.0f);

	if (elem->has("showSnapshotNoVideo"))
		mConfig.showSnapshotNoVideo = elem->get<bool>("showSnapshotNoVideo");

	if (elem->has("showSnapshotDelay"))
		mConfig.showSnapshotDelay = elem->get<bool>("showSnapshotDelay");

	// Update the embeded static image
	mStaticImage.setPosition(getPosition());
	mStaticImage.setMaxSize(getSize());
	mStaticImage.setSize(getSize());
}

std::vector<HelpPrompt> VideoComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> ret;
	ret.push_back(HelpPrompt("a", "select"));
	return ret;
}

void VideoComponent::setupContext()
{
	if (!mContext.valid)
	{
		// Create the YUV buffers for our video
		mContext.mutex = SDL_CreateMutex();
		mContext.numBands = 3;
		mContext.bands[0] = new unsigned char[mVideoWidth*mVideoHeight];
		mContext.bands[1] = new unsigned char[mVideoWidth*mVideoHeight / 4];
		mContext.bands[2] = new unsigned char[mVideoWidth*mVideoHeight / 4];

		glGenTextures(mContext.numBands, mContext.textures);

		for (int i = 0; i < mContext.numBands; ++i)
		{
			glBindTexture(GL_TEXTURE_2D, mContext.textures[i]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		float tex_offs_x = 0.0f;
		float tex_offs_y = 0.0f;
		float x, y;
		float x2;
		float y2;

		x = -(float)mSize.x() * mOrigin.x();
		y = -(float)mSize.y() * mOrigin.y();
		x2 = x+mSize.x();
		y2 = y+mSize.y();

		// We need two triangles to cover the rectangular area
		mVertices[0].pos[0] = x; 			mVertices[0].pos[1] = y;
		mVertices[1].pos[0] = x; 			mVertices[1].pos[1] = y2;
		mVertices[2].pos[0] = x2;			mVertices[2].pos[1] = y;

		mVertices[3].pos[0] = x2;			mVertices[3].pos[1] = y;
		mVertices[4].pos[0] = x; 			mVertices[4].pos[1] = y2;
		mVertices[5].pos[0] = x2;			mVertices[5].pos[1] = y2;

		// Texture coordinates
		mVertices[0].tex[0] = -tex_offs_x; 			mVertices[0].tex[1] = -tex_offs_y;
		mVertices[1].tex[0] = -tex_offs_x; 			mVertices[1].tex[1] = 1.0f + tex_offs_y;
		mVertices[2].tex[0] = 1.0f + tex_offs_x;		mVertices[2].tex[1] = -tex_offs_y;

		mVertices[3].tex[0] = 1.0f + tex_offs_x;		mVertices[3].tex[1] = -tex_offs_y;
		mVertices[4].tex[0] = -tex_offs_x;			mVertices[4].tex[1] = 1.0f + tex_offs_y;
		mVertices[5].tex[0] = 1.0f + tex_offs_x;		mVertices[5].tex[1] = 1.0f + tex_offs_y;

		// Colours - use this to fade the video in and out
		for (int i = 0; i < (4 * 6); ++i) {
			if ((i%4) < 3)
				mVertices[i / 4].colour[i % 4] = mFadeIn;
			else
				mVertices[i / 4].colour[i % 4] = 1.0f;
		}


		mContext.valid = true;
		mContext.dataAvail = false;
	}
}

void VideoComponent::freeContext()
{
	if (mContext.valid)
	{
		for (unsigned i = 0; i < mContext.numBands; ++i)
			delete[] mContext.bands[i];
		glDeleteTextures(mContext.numBands, mContext.textures);
		SDL_DestroyMutex(mContext.mutex);
		mContext.valid = false;
		mContext.dataAvail = false;
	}
}

void VideoComponent::setupVLC()
{
	// If VLC hasn't been initialised yet then do it now
	if (!mVLC)
	{
		const char* args[] = { "--quiet" };
		mVLC = libvlc_new(sizeof(args) / sizeof(args[0]), args);
	}
}

void VideoComponent::handleStartDelay()
{
	// Only play if any delay has timed out
	if (mStartDelayed)
	{
		if (mStartTime > SDL_GetTicks())
		{
			// Timeout not yet completed
			return;
		}
		// Completed
		mStartDelayed = false;
		// Clear the playing flag so startVideo works
		mIsPlaying = false;
		startVideo();
	}
}

void VideoComponent::handleLooping()
{
	if (mIsPlaying && mMediaPlayer)
	{
		libvlc_state_t state = libvlc_media_player_get_state(mMediaPlayer);
		if (state == libvlc_Ended)
		{
			//libvlc_media_player_set_position(mMediaPlayer, 0.0f);
			libvlc_media_player_set_media(mMediaPlayer, mMedia);
			libvlc_media_player_play(mMediaPlayer);
		}
	}
}

extern "C" unsigned setup(void **opaque, char *chroma, unsigned *width, unsigned *height, unsigned *pitches,unsigned *lines)
{
	pitches[0] = *width;
	pitches[1] = *width / 2;
	pitches[2] = *width / 2;
	lines[0] = *height;
	lines[1] = *height / 2;
	lines[2] = *height / 2;
	return 1;
}

extern "C" void cleanup(void *opaque)
{
}

void VideoComponent::startVideo()
{
	if (!mIsPlaying) {
		mVideoWidth = 0;
		mVideoHeight = 0;

#ifdef WIN32
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wton;
		std::string path = wton.to_bytes(mVideoPath.c_str());
#else
		std::string path(mVideoPath.c_str());
#endif
		// Make sure we have a video path
		if (mVLC && (path.size() > 0))
		{
			// Set the video that we are going to be playing so we don't attempt to restart it
			mPlayingVideoPath = mVideoPath;

			// Open the media
			mMedia = libvlc_media_new_path(mVLC, path.c_str());
			if (mMedia)
			{
				unsigned 	track_count;
				// Get the media metadata so we can find the aspect ratio
				libvlc_media_parse(mMedia);
				libvlc_media_track_t** tracks;
				track_count = libvlc_media_tracks_get(mMedia, &tracks);
				for (unsigned track = 0; track < track_count; ++track)
				{
					if (tracks[track]->i_type == libvlc_track_video)
					{
						mVideoWidth = tracks[track]->video->i_width;
						mVideoHeight = tracks[track]->video->i_height;
						break;
					}
				}
				libvlc_media_tracks_release(tracks, track_count);

				// Make sure we found a valid video track
				if ((mVideoWidth > 0) && (mVideoHeight > 0))
				{
					setupContext();

					// Setup the media player
					mMediaPlayer = libvlc_media_player_new_from_media(mMedia);
					libvlc_media_player_play(mMediaPlayer);
					libvlc_video_set_callbacks(mMediaPlayer, lock, unlock, display, (void*)&mContext);
					//libvlc_video_set_format(mMediaPlayer, "RGBA", (int)mVideoWidth, (int)mVideoHeight, (int)mVideoWidth * 4);
					libvlc_video_set_format_callbacks(mMediaPlayer, setup, cleanup);

					// Update the playing state
					mIsPlaying = true;
					mFadeIn = 0.0f;
				}
			}
		}
	}
}

void VideoComponent::startVideoWithDelay()
{
	// If not playing then either start the video or initiate the delay
	if (!mIsPlaying)
	{
		// Set the video that we are going to be playing so we don't attempt to restart it
		mPlayingVideoPath = mVideoPath;

		if (mConfig.startDelay == 0)
		{
			// No delay. Just start the video
			mStartDelayed = false;
			startVideo();
		}
		else
		{
			// Configure the start delay
			mStartDelayed = true;
			mFadeIn = 0.0f;
			mStartTime = SDL_GetTicks() + mConfig.startDelay;
		}
		mIsPlaying = true;
	}
}

void VideoComponent::stopVideo()
{
	mIsPlaying = false;
	mStartDelayed = false;
	// Release the media player so it stops calling back to us
	if (mMediaPlayer)
	{
		libvlc_media_player_stop(mMediaPlayer);
		libvlc_media_player_release(mMediaPlayer);
		libvlc_media_release(mMedia);
		mMediaPlayer = NULL;
		freeContext();
	}
}

void VideoComponent::update(int deltaTime)
{
	manageState();

	// If the video start is delayed and there is less than the fade time then set the image fade
	// accordingly
	if (mStartDelayed)
	{
		Uint32 ticks = SDL_GetTicks();
		if (mStartTime > ticks) 
		{
			Uint32 diff = mStartTime - ticks;
			if (diff < FADE_TIME_MS) 
			{
				mFadeIn = (float)diff / (float)FADE_TIME_MS;
				return;
			}
		}
	}
	// If the fade in is less than 1 then increment it
	if (mFadeIn < 1.0f)
	{
		mFadeIn += deltaTime / (float)FADE_TIME_MS;
		if (mFadeIn > 1.0f)
			mFadeIn = 1.0f;
	}
	GuiComponent::update(deltaTime);
}

void VideoComponent::manageState()
{
	// We will only show if the component is on display
	bool show = mShowing;

	// See if we're already playing
	if (mIsPlaying)
	{
		// If we are not on display then stop the video from playing
		if (!show)
		{
			stopVideo();
		}
		else
		{
			if (mVideoPath != mPlayingVideoPath)
			{
				// Path changed. Stop the video. We will start it again below because
				// mIsPlaying will be modified by stopVideo to be false
				stopVideo();
			}
		}
	}
	// Need to recheck variable rather than 'else' because it may be modified above
	if (!mIsPlaying)
	{
		// If we are on display then see if we should start the video
		if (show && !mVideoPath.empty())
		{
			startVideoWithDelay();
		}
	}
}

void VideoComponent::onShow()
{
	mShowing = true;
	manageState();
}

void VideoComponent::onHide()
{
	mShowing = false;
	manageState();
}


