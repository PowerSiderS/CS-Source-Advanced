//=============================================================================
// Theora video subsystem implementation
//=============================================================================

#include "theora_video.h"
#include "theora_material.h"

#include "filesystem.h"
#include "tier0/icommandline.h"
#include "tier1/strtools.h"
#include "tier1/utllinkedlist.h"
#include "tier1/KeyValues.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "materialsystem/itexture.h"
#include "vtf/vtf.h"
#include "pixelwriter.h"
#include "tier2/tier2.h"
#include "platform.h"

#if defined( USE_SDL )
#include "SDL.h"
#endif

#include "tier0/memdbgon.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
static CTheoraVideoSubSystem g_TheoraSystem;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTheoraVideoSubSystem, IVideoSubSystem,
                                   VIDEO_SUBSYSTEM_INTERFACE_VERSION, g_TheoraSystem );

// ---------------------------------------------------------------------------
// Supported file extensions
// ---------------------------------------------------------------------------
static VideoFileExtensionInfo_t s_TheoraExtensions[] =
{
    { ".ogv", VideoSystem::THEORA, VideoSystemFeature::FULL_PLAYBACK },
    { ".ogg", VideoSystem::THEORA, VideoSystemFeature::FULL_PLAYBACK },
};
static const int s_TheoraExtensionCount = ARRAYSIZE( s_TheoraExtensions );

const VideoSystemFeature_t CTheoraVideoSubSystem::DEFAULT_FEATURE_SET =
    VideoSystemFeature::FULL_PLAYBACK;

// ---------------------------------------------------------------------------
// CTheoraVideoSubSystem
// ---------------------------------------------------------------------------
CTheoraVideoSubSystem::CTheoraVideoSubSystem() :
    m_bTheoraInitialized( false ),
    m_LastResult( VideoResult::SUCCESS ),
    m_CurrentStatus( VideoSystemStatus::NOT_INITIALIZED ),
    m_AvailableFeatures( CTheoraVideoSubSystem::DEFAULT_FEATURE_SET ),
    m_pCommonServices( nullptr )
{
}

CTheoraVideoSubSystem::~CTheoraVideoSubSystem()
{
    ShutdownTheora();
}

// ---------------------------------------------------------------------------
// IAppSystem
// ---------------------------------------------------------------------------
bool CTheoraVideoSubSystem::Connect( CreateInterfaceFn factory )
{
    if ( !BaseClass::Connect( factory ) )
        return false;

    if ( g_pFullFileSystem == nullptr || materials == nullptr )
    {
        Msg( "Theora video subsystem: failed to connect — missing filesystem or materials\n" );
        return false;
    }
    return true;
}

void CTheoraVideoSubSystem::Disconnect()
{
    BaseClass::Disconnect();
}

void *CTheoraVideoSubSystem::QueryInterface( const char *pInterfaceName )
{
    if ( pInterfaceName && V_strncmp( pInterfaceName, VIDEO_SUBSYSTEM_INTERFACE_VERSION,
                                      Q_strlen( VIDEO_SUBSYSTEM_INTERFACE_VERSION ) + 1 ) == 0 )
    {
        return (IVideoSubSystem *) this;
    }
    return nullptr;
}

InitReturnVal_t CTheoraVideoSubSystem::Init()
{
    InitReturnVal_t nRetVal = BaseClass::Init();
    if ( nRetVal != INIT_OK )
        return nRetVal;
    return INIT_OK;
}

void CTheoraVideoSubSystem::Shutdown()
{
    ShutdownTheora();
    BaseClass::Shutdown();
}

// ---------------------------------------------------------------------------
// IVideoSubSystem — identification
// ---------------------------------------------------------------------------
VideoSystem_t CTheoraVideoSubSystem::GetSystemID()
{
    return VideoSystem::THEORA;
}

VideoSystemStatus_t CTheoraVideoSubSystem::GetSystemStatus()
{
    return m_CurrentStatus;
}

VideoSystemFeature_t CTheoraVideoSubSystem::GetSupportedFeatures()
{
    return m_AvailableFeatures;
}

const char *CTheoraVideoSubSystem::GetVideoSystemName()
{
    return "Theora";
}

// ---------------------------------------------------------------------------
// IVideoSubSystem — setup & shutdown
// ---------------------------------------------------------------------------
bool CTheoraVideoSubSystem::InitTheora()
{
    if ( m_bTheoraInitialized )
        return true;

#if defined( USE_SDL )
    if ( SDL_InitSubSystem( SDL_INIT_AUDIO ) != 0 )
    {
        Msg( "Theora: SDL audio init failed: %s (audio will be silent)\n", SDL_GetError() );
        // Non-fatal — video still works
    }
#endif

    m_bTheoraInitialized = true;
    m_CurrentStatus      = VideoSystemStatus::OK;
    Msg( "Theora video subsystem initialised\n" );
    return true;
}

void CTheoraVideoSubSystem::ShutdownTheora()
{
    if ( !m_bTheoraInitialized )
        return;

    m_bTheoraInitialized = false;
    m_CurrentStatus      = VideoSystemStatus::NOT_INITIALIZED;
}

bool CTheoraVideoSubSystem::InitializeVideoSystem( IVideoCommonServices *pCommonServices )
{
    m_pCommonServices = pCommonServices;
    return InitTheora();
}

bool CTheoraVideoSubSystem::ShutdownVideoSystem()
{
    ShutdownTheora();
    return true;
}

VideoResult_t CTheoraVideoSubSystem::VideoSoundDeviceCMD( VideoSoundDeviceOperation_t operation,
                                                           void *pDevice, void *pData )
{
    return VideoResult::OPERATION_NOT_SUPPORTED;
}

// ---------------------------------------------------------------------------
// IVideoSubSystem — file extensions
// ---------------------------------------------------------------------------
int CTheoraVideoSubSystem::GetSupportedFileExtensionCount()
{
    return s_TheoraExtensionCount;
}

const char *CTheoraVideoSubSystem::GetSupportedFileExtension( int num )
{
    if ( num < 0 || num >= s_TheoraExtensionCount )
        return nullptr;
    return s_TheoraExtensions[num].m_FileExtension;
}

VideoSystemFeature_t CTheoraVideoSubSystem::GetSupportedFileExtensionFeatures( int num )
{
    if ( num < 0 || num >= s_TheoraExtensionCount )
        return VideoSystemFeature::NO_FEATURES;
    return s_TheoraExtensions[num].m_VideoFeatures;
}

// ---------------------------------------------------------------------------
// IVideoSubSystem — playback
// ---------------------------------------------------------------------------
VideoResult_t CTheoraVideoSubSystem::PlayVideoFileFullScreen(
    const char *filename, void *mainWindow,
    int windowWidth, int windowHeight,
    int desktopWidth, int desktopHeight,
    bool windowed, float forcedMinTime,
    VideoPlaybackFlags_t playbackFlags )
{
    // Create a material, play through Update() loop, then destroy
    IVideoMaterial *pMat = CreateVideoMaterial( "theora_fullscreen", filename, playbackFlags );
    if ( !pMat )
        return m_LastResult;

    // Use back-buffer size when caller passes 0 (e.g. playvideo_test)
    if ( windowWidth <= 0 || windowHeight <= 0 )
    {
        materials->GetBackBufferDimensions( windowWidth, windowHeight );
        if ( windowWidth <= 0 )  windowWidth  = 1280;
        if ( windowHeight <= 0 ) windowHeight = 720;
    }

    pMat->StartVideo();

    // Cache tex coord range and texture dimensions once — they don't change
    float maxU = 1.0f, maxV = 1.0f;
    pMat->GetVideoTexCoordRange( &maxU, &maxV );
    int vidW = 0, vidH = 0;
    pMat->GetVideoImageSize( &vidW, &vidH );
    // Texture is POT — recover actual texture size from image size + coord range
    int texW = ( maxU > 0.0f ) ? (int)( vidW / maxU + 0.5f ) : vidW;
    int texH = ( maxV > 0.0f ) ? (int)( vidH / maxV + 0.5f ) : vidH;
    if ( texW <= 0 ) texW = 1;
    if ( texH <= 0 ) texH = 1;

    bool bAbort = false, bPause = false, bQuit = false;
    if ( m_pCommonServices )
        m_pCommonServices->InitFullScreenPlaybackInputHandler( playbackFlags, forcedMinTime, windowed );

    while ( !pMat->IsFinishedPlaying() && !bAbort )
    {
        if ( m_pCommonServices )
            m_pCommonServices->ProcessFullScreenInput( bAbort, bPause, bQuit );

        if ( bQuit ) break;

        pMat->SetPaused( bPause );
        pMat->Update();

        // Render the decoded frame fullscreen
        {
            CMatRenderContextPtr pRenderCtx( materials );
            pRenderCtx->ClearColor4ub( 0, 0, 0, 255 );
            pRenderCtx->ClearBuffers( true, false );

            // DrawScreenSpaceRectangle maps texture region [x0,y0]-[x1,y1]
            // to the destination rect.  Use texel coords (not normalised UVs).
            pRenderCtx->DrawScreenSpaceRectangle(
                pMat->GetMaterial(),
                0, 0,                           // dest x, y
                windowWidth, windowHeight,      // dest w, h
                0.0f, 0.0f,                     // src texel at top-left
                maxU * (float)( texW - 1 ),     // src texel x at right edge
                maxV * (float)( texH - 1 ),     // src texel y at bottom edge
                texW, texH );                   // full texture dimensions
        }
        materials->SwapBuffers();

#if defined( USE_SDL )
        SDL_Delay( 2 );
#endif
    }

    if ( m_pCommonServices )
        m_pCommonServices->TerminateFullScreenPlaybackInputHandler();

    DestroyVideoMaterial( pMat );
    return VideoResult::SUCCESS;
}

IVideoMaterial *CTheoraVideoSubSystem::CreateVideoMaterial( const char *pMaterialName,
                                                             const char *pVideoFileName,
                                                             VideoPlaybackFlags_t flags )
{
    if ( !m_bTheoraInitialized )
    {
        m_LastResult = VideoResult::SYSTEM_NOT_AVAILABLE;
        return nullptr;
    }

    CTheoraVideoMaterial *pMat = new CTheoraVideoMaterial();
    if ( !pMat->Init( pMaterialName, pVideoFileName, flags ) )
    {
        m_LastResult = pMat->GetLastResult();
        delete pMat;
        return nullptr;
    }

    m_LastResult = VideoResult::SUCCESS;
    return pMat;
}

VideoResult_t CTheoraVideoSubSystem::DestroyVideoMaterial( IVideoMaterial *pVideoMaterial )
{
    if ( !pVideoMaterial )
        return VideoResult::BAD_INPUT_PARAMETERS;

    CTheoraVideoMaterial *pMat = static_cast<CTheoraVideoMaterial *>( pVideoMaterial );
    pMat->Destroy();
    delete pMat;
    return VideoResult::SUCCESS;
}

IVideoRecorder *CTheoraVideoSubSystem::CreateVideoRecorder()
{
    m_LastResult = VideoResult::FEATURE_NOT_AVAILABLE;
    return nullptr;
}

VideoResult_t CTheoraVideoSubSystem::DestroyVideoRecorder( IVideoRecorder *pRecorder )
{
    return VideoResult::FEATURE_NOT_AVAILABLE;
}

VideoResult_t CTheoraVideoSubSystem::CheckCodecAvailability( VideoEncodeCodec_t codec )
{
    return VideoResult::FEATURE_NOT_AVAILABLE;
}

VideoResult_t CTheoraVideoSubSystem::GetLastResult()
{
    return m_LastResult;
}
