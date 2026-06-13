//=============================================================================
// Theora video subsystem
// Supports .ogv files (Ogg container, Theora video, Vorbis audio)
// Cross-platform: Windows, Linux (ARM / AArch64 / x86)
//=============================================================================

#ifndef THEORA_VIDEO_H
#define THEORA_VIDEO_H

#ifdef _WIN32
#pragma once
#endif

#include "theora_common.h"
#include "video/ivideoservices.h"
#include "videosubsystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IFileSystem;
class IMaterialSystem;

extern IFileSystem     *g_pFileSystem;
extern IMaterialSystem *materials;

//-----------------------------------------------------------------------------
// CTheoraVideoSubSystem
//-----------------------------------------------------------------------------
class CTheoraVideoSubSystem : public CTier2AppSystem< IVideoSubSystem >
{
    typedef CTier2AppSystem< IVideoSubSystem > BaseClass;

public:
    CTheoraVideoSubSystem();
    ~CTheoraVideoSubSystem();

    // IAppSystem
    virtual bool            Connect( CreateInterfaceFn factory ) OVERRIDE;
    virtual void            Disconnect() OVERRIDE;
    virtual void           *QueryInterface( const char *pInterfaceName ) OVERRIDE;
    virtual InitReturnVal_t Init() OVERRIDE;
    virtual void            Shutdown() OVERRIDE;

    // IVideoSubSystem
    virtual VideoSystem_t           GetSystemID() OVERRIDE;
    virtual VideoSystemStatus_t     GetSystemStatus() OVERRIDE;
    virtual VideoSystemFeature_t    GetSupportedFeatures() OVERRIDE;
    virtual const char             *GetVideoSystemName() OVERRIDE;

    virtual bool            InitializeVideoSystem( IVideoCommonServices *pCommonServices ) OVERRIDE;
    virtual bool            ShutdownVideoSystem() OVERRIDE;

    virtual VideoResult_t   VideoSoundDeviceCMD( VideoSoundDeviceOperation_t operation, void *pDevice, void *pData = nullptr ) OVERRIDE;

    virtual int                     GetSupportedFileExtensionCount() OVERRIDE;
    virtual const char             *GetSupportedFileExtension( int num ) OVERRIDE;
    virtual VideoSystemFeature_t    GetSupportedFileExtensionFeatures( int num ) OVERRIDE;

    virtual VideoResult_t   PlayVideoFileFullScreen( const char *filename, void *mainWindow,
                                                     int windowWidth, int windowHeight,
                                                     int desktopWidth, int desktopHeight,
                                                     bool windowed, float forcedMinTime,
                                                     VideoPlaybackFlags_t playbackFlags ) OVERRIDE;

    virtual IVideoMaterial *CreateVideoMaterial( const char *pMaterialName,
                                                 const char *pVideoFileName,
                                                 VideoPlaybackFlags_t flags ) OVERRIDE;
    virtual VideoResult_t   DestroyVideoMaterial( IVideoMaterial *pVideoMaterial ) OVERRIDE;

    virtual IVideoRecorder *CreateVideoRecorder() OVERRIDE;
    virtual VideoResult_t   DestroyVideoRecorder( IVideoRecorder *pRecorder ) OVERRIDE;

    virtual VideoResult_t   CheckCodecAvailability( VideoEncodeCodec_t codec ) OVERRIDE;
    virtual VideoResult_t   GetLastResult() OVERRIDE;

    static const VideoSystemFeature_t DEFAULT_FEATURE_SET;

private:
    bool                    InitTheora();
    void                    ShutdownTheora();

    bool                    m_bTheoraInitialized;
    VideoResult_t           m_LastResult;
    VideoSystemStatus_t     m_CurrentStatus;
    VideoSystemFeature_t    m_AvailableFeatures;
    IVideoCommonServices   *m_pCommonServices;
};

#endif // THEORA_VIDEO_H
