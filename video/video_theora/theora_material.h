//=============================================================================
// Theora video material - IVideoMaterial implementation
// Decodes an .ogv file (Theora + Vorbis in Ogg container) frame-by-frame
// and uploads each frame to a VGUI/MaterialSystem texture.
//=============================================================================

#ifndef THEORA_MATERIAL_H
#define THEORA_MATERIAL_H

#ifdef _WIN32
#pragma once
#endif

#include "theora_common.h"
#include "video/ivideoservices.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "tier1/utlstring.h"
#include "filesystem.h"

// Ogg / Theora / Vorbis headers
#include "ogg/ogg.h"
#include "theora/theoradec.h"
#include "vorbis/codec.h"

#if defined( USE_SDL ) && !defined( ANDROID )
#include "SDL.h"
#endif

#ifdef ANDROID
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#endif

//-----------------------------------------------------------------------------
// CTheoraVideoMaterial
//-----------------------------------------------------------------------------
class CTheoraVideoMaterial : public IVideoMaterial
{
public:
    CTheoraVideoMaterial();
    ~CTheoraVideoMaterial();

    // Initialise from a file path; returns false on failure
    bool            Init( const char *pMaterialName, const char *pFileName, VideoPlaybackFlags_t flags );
    void            Destroy();

    // IVideoMaterial
    virtual const char         *GetVideoFileName() OVERRIDE;
    virtual VideoResult_t       GetLastResult() OVERRIDE;
    virtual VideoFrameRate_t   &GetVideoFrameRate() OVERRIDE;

    virtual bool    HasAudio() OVERRIDE;
    virtual bool    SetVolume( float fVolume ) OVERRIDE;
    virtual float   GetVolume() OVERRIDE;
    virtual void    SetMuted( bool bMuteState ) OVERRIDE;
    virtual bool    IsMuted() OVERRIDE;
    virtual VideoResult_t SoundDeviceCommand( VideoSoundDeviceOperation_t operation, void *pDevice = nullptr, void *pData = nullptr ) OVERRIDE;

    virtual bool    IsVideoReadyToPlay() OVERRIDE;
    virtual bool    IsVideoPlaying() OVERRIDE;
    virtual bool    IsNewFrameReady() OVERRIDE;
    virtual bool    IsFinishedPlaying() OVERRIDE;

    virtual bool    StartVideo() OVERRIDE;
    virtual bool    StopVideo() OVERRIDE;
    virtual void    SetLooping( bool bLoopVideo ) OVERRIDE;
    virtual bool    IsLooping() OVERRIDE;
    virtual void    SetPaused( bool bPauseState ) OVERRIDE;
    virtual bool    IsPaused() OVERRIDE;

    virtual float   GetVideoDuration() OVERRIDE;
    virtual int     GetFrameCount() OVERRIDE;
    virtual bool    SetFrame( int FrameNum ) OVERRIDE;
    virtual int     GetCurrentFrame() OVERRIDE;
    virtual bool    SetTime( float flTime ) OVERRIDE;
    virtual float   GetCurrentVideoTime() OVERRIDE;

    virtual bool    Update() OVERRIDE;

    virtual IMaterial  *GetMaterial() OVERRIDE;
    virtual void        GetVideoTexCoordRange( float *pMaxU, float *pMaxV ) OVERRIDE;
    virtual void        GetVideoImageSize( int *pWidth, int *pHeight ) OVERRIDE;

private:
    // Ogg reading helpers
    bool    ReadMoreData();
    bool    GetNextPacket( int streamIndex, ogg_packet *pPacket );
    void    QueueAudioPacket( ogg_packet *pPacket );

    // YUV -> RGBA conversion helpers
    void    ConvertYCbCrToRGBA( th_ycbcr_buffer yuv, uint8 *pDst, int dstStride );

    // Texture upload
    void    UploadFrameToTexture( th_ycbcr_buffer yuv );

    // State
    CUtlString          m_FileName;
    CUtlString          m_MaterialName;       // unique decorated name used internally
    VideoPlaybackFlags_t m_PlaybackFlags;
    VideoResult_t       m_LastResult;
    VideoFrameRate_t    m_FrameRate;

    bool    m_bInitialized;
    bool    m_bPlaying;
    bool    m_bPaused;
    bool    m_bLooping;
    bool    m_bFinished;
    bool    m_bNewFrameReady;

    // Timing
    double  m_flVideoDuration;      // seconds (estimated)
    double  m_flCurrentTime;        // seconds elapsed
    double  m_flLastUpdateTime;     // platform time at last Update()
    int     m_nCurrentFrame;
    int     m_nTotalFrames;

    // File handle
    FileHandle_t    m_hFile;

    // Ogg sync / stream state
    ogg_sync_state      m_OggSync;
    ogg_stream_state    m_VideoStream;
    ogg_stream_state    m_AudioStream;
    bool                m_bVideoStreamFound;
    bool                m_bAudioStreamFound;
    int                 m_nVideoSerial;
    int                 m_nAudioSerial;

    // Theora decode state
    th_info         m_TheoraInfo;
    th_comment      m_TheoraComment;
    th_setup_info  *m_pTheoraSetup;
    th_dec_ctx     *m_pTheoraCtx;
    bool            m_bTheoraHeaderDone;

    // Vorbis decode state
    vorbis_info         m_VorbisInfo;
    vorbis_comment      m_VorbisComment;
    vorbis_dsp_state    m_VorbisDsp;
    vorbis_block        m_VorbisBlock;
    bool                m_bVorbisHeaderDone;
    bool                m_bVorbisDspReady;
    bool                m_bHasAudio;

    // Audio
    bool    m_bAudioMuted;
    float   m_fVolume;

#ifdef ANDROID
    // OpenSL ES audio backend (Android only — SDL allows only one audio device)
    static void SLBufferCallback( SLAndroidSimpleBufferQueueItf q, void *pCtx );

    static const int kSLAudioBuffers = 4;
    static const int kSLBufferFrames = 4096;   // audio frames per buffer slot
    static const int kSLMaxChannels  = 2;

    SLObjectItf                   m_pSLEngineObj;
    SLEngineItf                   m_pSLEngine;
    SLObjectItf                   m_pSLOutputMixObj;
    SLObjectItf                   m_pSLPlayerObj;
    SLPlayItf                     m_pSLPlayerPlay;
    SLAndroidSimpleBufferQueueItf m_pSLBufferQueue;
    SLVolumeItf                   m_pSLVolume;

    // Ring buffer: kSLAudioBuffers slots, each kSLBufferFrames * kSLMaxChannels int16 samples
    int16_t          m_SLBufs[kSLAudioBuffers][kSLBufferFrames * kSLMaxChannels];
    volatile int     m_SLFreeCount;   // number of free (not in-flight) slots — atomic via __sync_*
    int              m_SLWriteIdx;    // ring write pointer (main thread only)
    bool             m_bSLOpen;

    CUtlVector<int16_t> m_SLAccum;   // accumulates partial Vorbis packet output until chunk is full
#elif defined( USE_SDL )
    SDL_AudioDeviceID   m_nAudioDeviceID;
    bool                m_bSDLAudioOpen;
#endif

    // Texture / material
    CMaterialReference  m_Material;
    CTextureReference   m_Texture;
    int                 m_nVideoWidth;
    int                 m_nVideoHeight;
    int                 m_nTexWidth;
    int                 m_nTexHeight;
    CUtlVector<uint8>   m_RGBABuffer;   // scratch RGBA buffer for YUV conversion
};

#endif // THEORA_MATERIAL_H
