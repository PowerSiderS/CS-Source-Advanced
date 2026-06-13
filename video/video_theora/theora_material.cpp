//=============================================================================
// Theora video material implementation
// Decodes Ogg/Theora/Vorbis and uploads frames to a MaterialSystem texture.
//=============================================================================

#include "theora_material.h"

#include "filesystem.h"
#include "tier1/strtools.h"
#include "tier1/KeyValues.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "materialsystem/itexture.h"
#include "vtf/vtf.h"
#include "pixelwriter.h"
#include "tier2/tier2.h"
#include "platform.h"

#if defined( USE_SDL ) && !defined( ANDROID )
#include "SDL.h"
#endif

#include "tier0/memdbgon.h"

// ---------------------------------------------------------------------------
// Custom ITextureRegenerator that owns a raw RGBA buffer
// ---------------------------------------------------------------------------
class CTheoraTextureRegen : public ITextureRegenerator
{
public:
    CTheoraTextureRegen( int w, int h )
        : m_nWidth( w ), m_nHeight( h )
    {
        m_Buffer.SetCount( w * h * 4 );
        V_memset( m_Buffer.Base(), 0, m_Buffer.Count() );
    }

    void UpdateBuffer( const uint8 *pSrc, int stride )
    {
        for ( int y = 0; y < m_nHeight; y++ )
        {
            V_memcpy( &m_Buffer[ y * m_nWidth * 4 ], pSrc + y * stride, m_nWidth * 4 );
        }
    }

    virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect ) OVERRIDE
    {
        CPixelWriter writer;
        writer.SetPixelMemory( pVTFTexture->Format(),
                               pVTFTexture->ImageData(),
                               pVTFTexture->RowSizeInBytes( 0 ) );

        for ( int y = 0; y < m_nHeight; y++ )
        {
            writer.Seek( 0, y );
            const uint8 *pRow = &m_Buffer[ y * m_nWidth * 4 ];
            for ( int x = 0; x < m_nWidth; x++, pRow += 4 )
            {
                writer.WritePixel( pRow[0], pRow[1], pRow[2], pRow[3] );
            }
        }
    }

    virtual void Release() OVERRIDE {}

private:
    int              m_nWidth, m_nHeight;
    CUtlVector<uint8> m_Buffer;
};

// ---------------------------------------------------------------------------
// Helper: clamp byte
// ---------------------------------------------------------------------------
static inline uint8 ClampByte( int v )
{
    return (uint8)( v < 0 ? 0 : ( v > 255 ? 255 : v ) );
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
CTheoraVideoMaterial::CTheoraVideoMaterial() :
    m_PlaybackFlags( VideoPlaybackFlags::NO_PLAYBACK_OPTIONS ),
    m_LastResult( VideoResult::SUCCESS ),
    m_bInitialized( false ),
    m_bPlaying( false ),
    m_bPaused( false ),
    m_bLooping( false ),
    m_bFinished( false ),
    m_bNewFrameReady( false ),
    m_flVideoDuration( 0.0 ),
    m_flCurrentTime( 0.0 ),
    m_flLastUpdateTime( 0.0 ),
    m_nCurrentFrame( 0 ),
    m_nTotalFrames( 0 ),
    m_hFile( FILESYSTEM_INVALID_HANDLE ),
    m_bVideoStreamFound( false ),
    m_bAudioStreamFound( false ),
    m_nVideoSerial( 0 ),
    m_nAudioSerial( 0 ),
    m_pTheoraSetup( nullptr ),
    m_pTheoraCtx( nullptr ),
    m_bTheoraHeaderDone( false ),
    m_bVorbisHeaderDone( false ),
    m_bVorbisDspReady( false ),
    m_bHasAudio( false ),
    m_bAudioMuted( false ),
    m_fVolume( 1.0f ),
    m_nVideoWidth( 0 ),
    m_nVideoHeight( 0 ),
    m_nTexWidth( 0 ),
    m_nTexHeight( 0 )
{
#ifdef ANDROID
    m_pSLEngineObj    = nullptr;
    m_pSLEngine       = nullptr;
    m_pSLOutputMixObj = nullptr;
    m_pSLPlayerObj    = nullptr;
    m_pSLPlayerPlay   = nullptr;
    m_pSLBufferQueue  = nullptr;
    m_pSLVolume       = nullptr;
    m_SLFreeCount     = kSLAudioBuffers;
    m_SLWriteIdx      = 0;
    m_bSLOpen         = false;
#elif defined( USE_SDL )
    m_nAudioDeviceID = 0;
    m_bSDLAudioOpen  = false;
#endif
}

CTheoraVideoMaterial::~CTheoraVideoMaterial()
{
    Destroy();
}

// ---------------------------------------------------------------------------
// ReadMoreData — feed data from the file into ogg_sync_state
// ---------------------------------------------------------------------------
bool CTheoraVideoMaterial::ReadMoreData()
{
    if ( m_hFile == FILESYSTEM_INVALID_HANDLE )
        return false;

    char *pBuf = ogg_sync_buffer( &m_OggSync, cTheoraOggBufferSize );
    if ( !pBuf )
        return false;

    int nRead = (int) g_pFullFileSystem->Read( pBuf, cTheoraOggBufferSize, m_hFile );
    if ( nRead <= 0 )
        return false;

    ogg_sync_wrote( &m_OggSync, nRead );
    return true;
}

// ---------------------------------------------------------------------------
// Init — open file, parse Ogg headers, initialise decoders, create texture
// ---------------------------------------------------------------------------
bool CTheoraVideoMaterial::Init( const char *pMaterialName, const char *pFileName,
                                  VideoPlaybackFlags_t flags )
{
    m_FileName      = pFileName;
    m_PlaybackFlags = flags;

    // Build a unique decorated name for this instance
    static int s_nUID = 0;
    int uid = ++s_nUID;
    char uniqueName[256];
    Q_snprintf( uniqueName, sizeof( uniqueName ), "%s_%04d", pMaterialName, uid );
    m_MaterialName = uniqueName;

    // Open file
    m_hFile = g_pFullFileSystem->Open( pFileName, "rb", "GAME" );
    if ( m_hFile == FILESYSTEM_INVALID_HANDLE )
    {
        Msg( "Theora: cannot open '%s'\n", pFileName );
        m_LastResult = VideoResult::VIDEO_FILE_NOT_FOUND;
        return false;
    }

    // Init Ogg sync
    ogg_sync_init( &m_OggSync );

    // Init Theora info / comment structs 
    th_info_init( &m_TheoraInfo );
    th_comment_init( &m_TheoraComment );
    m_pTheoraSetup = nullptr;

    // Init Vorbis info / comment structs
    vorbis_info_init( &m_VorbisInfo );
    vorbis_comment_init( &m_VorbisComment );

    //--------------------------------------------------------------------------
    // Phase 1: read Ogg pages until we have all headers for both streams
    //--------------------------------------------------------------------------
    int nTheoraHeaders  = 0;
    int nVorbisHeaders  = 0;
    ogg_page   page;
    ogg_packet pkt;

    while ( true )
    {
        // Try to get a page from the sync buffer
        while ( ogg_sync_pageout( &m_OggSync, &page ) != 1 )
        {
            if ( !ReadMoreData() )
                goto headers_done; // EOF before all headers — partial init is OK
        }

        int serial = ogg_page_serialno( &page );

        if ( ogg_page_bos( &page ) )
        {
            // Beginning-of-stream page — register new logical stream
            ogg_stream_state testStream;
            ogg_stream_init( &testStream, serial );
            ogg_stream_pagein( &testStream, &page );
            ogg_stream_packetout( &testStream, &pkt );

            if ( !m_bVideoStreamFound && th_packet_isheader( &pkt ) )
            {
                if ( !m_bVideoStreamFound )
                {
                    ogg_stream_init( &m_VideoStream, serial );
                    m_bVideoStreamFound = true;
                    m_nVideoSerial      = serial;
                    ogg_stream_pagein( &m_VideoStream, &page );
                    nTheoraHeaders = 1;
                    th_decode_headerin( &m_TheoraInfo, &m_TheoraComment, &m_pTheoraSetup, &pkt );
                }
                ogg_stream_clear( &testStream );
            }
            else if ( !m_bAudioStreamFound && pkt.bytes >= 7 &&
                      V_memcmp( pkt.packet, "\x01vorbis", 7 ) == 0 )
            {
                ogg_stream_init( &m_AudioStream, serial );
                m_bAudioStreamFound = true;
                m_nAudioSerial      = serial;
                ogg_stream_pagein( &m_AudioStream, &page );
                nVorbisHeaders = 1;
                vorbis_synthesis_headerin( &m_VorbisInfo, &m_VorbisComment, &pkt );
                ogg_stream_clear( &testStream );
            }
            else
            {
                ogg_stream_clear( &testStream );
            }
            continue;
        }

        // Not a BOS page — dispatch to the right stream
        if ( m_bVideoStreamFound && serial == m_nVideoSerial )
            ogg_stream_pagein( &m_VideoStream, &page );
        else if ( m_bAudioStreamFound && serial == m_nAudioSerial )
            ogg_stream_pagein( &m_AudioStream, &page );

        // Read remaining Theora headers (need 3 total)
        if ( m_bVideoStreamFound && nTheoraHeaders < 3 )
        {
            while ( ogg_stream_packetout( &m_VideoStream, &pkt ) > 0 )
            {
                int ret = th_decode_headerin( &m_TheoraInfo, &m_TheoraComment,
                                              &m_pTheoraSetup, &pkt );
                if ( ret > 0 ) nTheoraHeaders++;
            }
        }

        // Read remaining Vorbis headers (need 3 total)
        if ( m_bAudioStreamFound && nVorbisHeaders < 3 )
        {
            while ( ogg_stream_packetout( &m_AudioStream, &pkt ) > 0 )
            {
                int ret = vorbis_synthesis_headerin( &m_VorbisInfo, &m_VorbisComment, &pkt );
                if ( ret == 0 ) nVorbisHeaders++;
            }
        }

        if ( ( !m_bVideoStreamFound || nTheoraHeaders >= 3 ) &&
             ( !m_bAudioStreamFound || nVorbisHeaders >= 3 ) )
            break;
    }

headers_done:

    if ( !m_bVideoStreamFound || nTheoraHeaders < 3 )
    {
        Msg( "Theora: '%s' does not contain a valid Theora video stream\n", pFileName );
        m_LastResult = VideoResult::VIDEO_ERROR_OCCURED;
        return false;
    }

    // Allocate Theora decoder
    m_pTheoraCtx = th_decode_alloc( &m_TheoraInfo, m_pTheoraSetup );
    if ( !m_pTheoraCtx )
    {
        Msg( "Theora: th_decode_alloc failed for '%s'\n", pFileName );
        m_LastResult = VideoResult::VIDEO_ERROR_OCCURED;
        return false;
    }
    m_bTheoraHeaderDone = true;

    m_nVideoWidth  = m_TheoraInfo.pic_width;
    m_nVideoHeight = m_TheoraInfo.pic_height;
    m_nTexWidth    = TheoraComputeNextPOT( m_nVideoWidth );
    m_nTexHeight   = TheoraComputeNextPOT( m_nVideoHeight );

    // Frame rate
    if ( m_TheoraInfo.fps_denominator > 0 )
        m_FrameRate.SetFPS( (float) m_TheoraInfo.fps_numerator /
                            (float) m_TheoraInfo.fps_denominator );
    else
        m_FrameRate.SetFPS( 30.0f );

    // Allocate scratch RGBA buffer (full texture size)
    m_RGBABuffer.SetCount( m_nTexWidth * m_nTexHeight * 4 );
    V_memset( m_RGBABuffer.Base(), 0, m_RGBABuffer.Count() );

    //--------------------------------------------------------------------------
    // Initialise audio decoder
    //--------------------------------------------------------------------------
    if ( m_bAudioStreamFound && nVorbisHeaders >= 3 )
    {
        if ( vorbis_synthesis_init( &m_VorbisDsp, &m_VorbisInfo ) == 0 )
        {
            vorbis_block_init( &m_VorbisDsp, &m_VorbisBlock );
            m_bVorbisDspReady = true;

#ifdef ANDROID
            if ( slCreateEngine( &m_pSLEngineObj, 0, nullptr, 0, nullptr, nullptr ) == SL_RESULT_SUCCESS &&
                 (*m_pSLEngineObj)->Realize( m_pSLEngineObj, SL_BOOLEAN_FALSE ) == SL_RESULT_SUCCESS &&
                 (*m_pSLEngineObj)->GetInterface( m_pSLEngineObj, SL_IID_ENGINE, &m_pSLEngine ) == SL_RESULT_SUCCESS )
            {
                SLresult r = (*m_pSLEngine)->CreateOutputMix( m_pSLEngine, &m_pSLOutputMixObj, 0, nullptr, nullptr );
                if ( r == SL_RESULT_SUCCESS )
                    r = (*m_pSLOutputMixObj)->Realize( m_pSLOutputMixObj, SL_BOOLEAN_FALSE );

                if ( r == SL_RESULT_SUCCESS )
                {
                    SLDataLocator_AndroidSimpleBufferQueue locBufQ = {
                        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                        (SLuint32)kSLAudioBuffers
                    };
                    SLDataFormat_PCM fmt = {
                        SL_DATAFORMAT_PCM,
                        (SLuint32)m_VorbisInfo.channels,
                        (SLuint32)( m_VorbisInfo.rate * 1000 ), // milliHz
                        SL_PCMSAMPLEFORMAT_FIXED_16,
                        SL_PCMSAMPLEFORMAT_FIXED_16,
                        ( m_VorbisInfo.channels == 2 )
                            ? ( SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT )
                            : SL_SPEAKER_FRONT_CENTER,
                        SL_BYTEORDER_LITTLEENDIAN
                    };
                    SLDataSource audioSrc = { &locBufQ, &fmt };
                    SLDataLocator_OutputMix locMix = { SL_DATALOCATOR_OUTPUTMIX, m_pSLOutputMixObj };
                    SLDataSink   audioSnk = { &locMix, nullptr };

                    const SLInterfaceID ids[]  = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME };
                    const SLboolean     reqs[] = { SL_BOOLEAN_TRUE,                 SL_BOOLEAN_FALSE };

                    r = (*m_pSLEngine)->CreateAudioPlayer( m_pSLEngine, &m_pSLPlayerObj,
                                                           &audioSrc, &audioSnk, 2, ids, reqs );
                    if ( r == SL_RESULT_SUCCESS )
                        r = (*m_pSLPlayerObj)->Realize( m_pSLPlayerObj, SL_BOOLEAN_FALSE );

                    if ( r == SL_RESULT_SUCCESS )
                    {
                        (*m_pSLPlayerObj)->GetInterface( m_pSLPlayerObj, SL_IID_PLAY,
                                                         &m_pSLPlayerPlay );
                        (*m_pSLPlayerObj)->GetInterface( m_pSLPlayerObj,
                                                         SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                                         &m_pSLBufferQueue );
                        (*m_pSLPlayerObj)->GetInterface( m_pSLPlayerObj, SL_IID_VOLUME,
                                                         &m_pSLVolume );

                        (*m_pSLBufferQueue)->RegisterCallback( m_pSLBufferQueue,
                                                               SLBufferCallback, this );
                        // Start paused; StartVideo() will unpause
                        (*m_pSLPlayerPlay)->SetPlayState( m_pSLPlayerPlay, SL_PLAYSTATE_PAUSED );

                        m_bSLOpen   = true;
                        m_bHasAudio = true;
                        Msg( "Theora: OpenSL ES audio OK (%dHz %dch)\n",
                             m_VorbisInfo.rate, m_VorbisInfo.channels );
                    }
                    else
                    {
                        Msg( "Theora: OpenSL ES CreateAudioPlayer failed (%d)\n", (int)r );
                    }
                }
            }
            if ( !m_bSLOpen )
                Msg( "Theora: OpenSL ES init failed — video-only playback\n" );

#elif defined( USE_SDL )
            SDL_AudioSpec desired, obtained;
            V_memset( &desired, 0, sizeof( desired ) );
            desired.freq     = m_VorbisInfo.rate;
            desired.format   = AUDIO_F32SYS;
            desired.channels = (uint8) m_VorbisInfo.channels;
            desired.samples  = 4096;
            desired.callback = nullptr; // push model

            m_nAudioDeviceID = SDL_OpenAudioDevice( nullptr, 0, &desired, &obtained, 0 );
            if ( m_nAudioDeviceID > 0 )
            {
                m_bSDLAudioOpen = true;
                m_bHasAudio     = true;
            }
            else
            {
                Msg( "Theora: SDL_OpenAudioDevice failed: %s (video-only playback)\n",
                     SDL_GetError() );
            }
#endif
        }
        else
        {
            Msg( "Theora: vorbis_synthesis_init failed — no audio\n" );
        }
    }

    //--------------------------------------------
    // Create procedural texture + material
    //--------------------------------------------
    char texName[256];
    Q_snprintf( texName, sizeof( texName ), "_rt_theora_%s", uniqueName );

    ITexture *pTex = materials->CreateProceduralTexture(
        texName, TEXTURE_GROUP_OTHER,
        m_nTexWidth, m_nTexHeight,
        IMAGE_FORMAT_RGBA8888,
        TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP |
        TEXTUREFLAGS_NOLOD  | TEXTUREFLAGS_PROCEDURAL );

    if ( !pTex )
    {
        Msg( "Theora: failed to create procedural texture\n" );
        m_LastResult = VideoResult::VIDEO_ERROR_OCCURED;
        return false;
    }
    m_Texture.Init( pTex );
    pTex->SetTextureRegenerator( nullptr );

    // Create an UnlitGeneric material that references this texture
    KeyValues *pKV = new KeyValues( "UnlitGeneric" );
    pKV->SetString( "$basetexture", texName );
    pKV->SetInt( "$translucent", 1 );
    pKV->SetInt( "$vertexcolor", 1 );
    pKV->SetInt( "$vertexalpha", 1 );

    m_Material.Init( materials->CreateMaterial( uniqueName, pKV ) );

    m_bInitialized = true;
    m_LastResult   = VideoResult::SUCCESS;
    return true;
}

// ---------------------------------------------------------------------------
// Destroy — free all resources
// ---------------------------------------------------------------------------
void CTheoraVideoMaterial::Destroy()
{
    if ( !m_bInitialized )
        return;

#ifdef ANDROID
    if ( m_bSLOpen )
    {
        m_bSLOpen = false; // prevent callback side-effects during teardown
        if ( m_pSLBufferQueue )
            (*m_pSLBufferQueue)->Clear( m_pSLBufferQueue ); // flush + no more callbacks
        if ( m_pSLPlayerPlay )
            (*m_pSLPlayerPlay)->SetPlayState( m_pSLPlayerPlay, SL_PLAYSTATE_STOPPED );
        if ( m_pSLPlayerObj )
        {
            (*m_pSLPlayerObj)->Destroy( m_pSLPlayerObj );
            m_pSLPlayerObj   = nullptr;
            m_pSLPlayerPlay  = nullptr;
            m_pSLBufferQueue = nullptr;
            m_pSLVolume      = nullptr;
        }
        if ( m_pSLOutputMixObj )
        {
            (*m_pSLOutputMixObj)->Destroy( m_pSLOutputMixObj );
            m_pSLOutputMixObj = nullptr;
        }
        if ( m_pSLEngineObj )
        {
            (*m_pSLEngineObj)->Destroy( m_pSLEngineObj );
            m_pSLEngineObj = nullptr;
            m_pSLEngine    = nullptr;
        }
        m_SLAccum.Purge();
    }
#elif defined( USE_SDL )
    if ( m_bSDLAudioOpen )
    {
        SDL_CloseAudioDevice( m_nAudioDeviceID );
        m_nAudioDeviceID = 0;
        m_bSDLAudioOpen  = false;
    }
#endif

    if ( m_bVorbisDspReady )
    {
        vorbis_block_clear( &m_VorbisBlock );
        vorbis_dsp_clear( &m_VorbisDsp );
        m_bVorbisDspReady = false;
    }

    if ( m_bAudioStreamFound )
    {
        vorbis_comment_clear( &m_VorbisComment );
        vorbis_info_clear( &m_VorbisInfo );
        ogg_stream_clear( &m_AudioStream );
    }

    if ( m_pTheoraCtx )
    {
        th_decode_free( m_pTheoraCtx );
        m_pTheoraCtx = nullptr;
    }
    if ( m_pTheoraSetup )
    {
        th_setup_free( m_pTheoraSetup );
        m_pTheoraSetup = nullptr;
    }
    if ( m_bTheoraHeaderDone )
    {
        th_comment_clear( &m_TheoraComment );
        th_info_clear( &m_TheoraInfo );
    }
    if ( m_bVideoStreamFound )
    {
        ogg_stream_clear( &m_VideoStream );
    }

    ogg_sync_clear( &m_OggSync );

    if ( m_hFile != FILESYSTEM_INVALID_HANDLE )
    {
        g_pFullFileSystem->Close( m_hFile );
        m_hFile = FILESYSTEM_INVALID_HANDLE;
    }

    m_Material.Shutdown();
    m_Texture.Shutdown();

    m_bInitialized = false;
}

// ---------------------------------------------------------------------------
// IVideoMaterial — identification / state
// ---------------------------------------------------------------------------
const char *CTheoraVideoMaterial::GetVideoFileName()  { return m_FileName.Get(); }
VideoResult_t CTheoraVideoMaterial::GetLastResult()   { return m_LastResult; }
VideoFrameRate_t &CTheoraVideoMaterial::GetVideoFrameRate() { return m_FrameRate; }

bool  CTheoraVideoMaterial::HasAudio()          { return m_bHasAudio; }
bool  CTheoraVideoMaterial::IsMuted()           { return m_bAudioMuted; }
float CTheoraVideoMaterial::GetVolume()         { return m_fVolume; }
bool  CTheoraVideoMaterial::IsVideoReadyToPlay(){ return m_bInitialized && !m_bPlaying && !m_bFinished; }
bool  CTheoraVideoMaterial::IsVideoPlaying()    { return m_bPlaying; }
bool  CTheoraVideoMaterial::IsNewFrameReady()   { return m_bNewFrameReady; }
bool  CTheoraVideoMaterial::IsFinishedPlaying() { return m_bFinished; }
bool  CTheoraVideoMaterial::IsLooping()         { return m_bLooping; }
bool  CTheoraVideoMaterial::IsPaused()          { return m_bPaused; }

bool CTheoraVideoMaterial::SetVolume( float fVolume )
{
    m_fVolume = fVolume;
#ifdef ANDROID
    // OpenSL ES volume is in millibels; just store the float, applied per-sample in QueueAudioPacket
#endif
    return true;
}

void CTheoraVideoMaterial::SetMuted( bool bMuteState )
{
    m_bAudioMuted = bMuteState;
#ifdef ANDROID
    if ( m_bSLOpen && m_pSLVolume )
        (*m_pSLVolume)->SetMute( m_pSLVolume, bMuteState ? SL_BOOLEAN_TRUE : SL_BOOLEAN_FALSE );
#elif defined( USE_SDL )
    if ( m_bSDLAudioOpen )
        SDL_PauseAudioDevice( m_nAudioDeviceID, bMuteState ? 1 : 0 );
#endif
}

VideoResult_t CTheoraVideoMaterial::SoundDeviceCommand( VideoSoundDeviceOperation_t operation,
                                                         void *pDevice, void *pData )
{
    return VideoResult::OPERATION_NOT_SUPPORTED;
}

void CTheoraVideoMaterial::SetLooping( bool bLoopVideo ) { m_bLooping = bLoopVideo; }

void CTheoraVideoMaterial::SetPaused( bool bPauseState )
{
    m_bPaused = bPauseState;
#ifdef ANDROID
    if ( m_bSLOpen && m_pSLPlayerPlay )
        (*m_pSLPlayerPlay)->SetPlayState( m_pSLPlayerPlay,
            bPauseState ? SL_PLAYSTATE_PAUSED : SL_PLAYSTATE_PLAYING );
#elif defined( USE_SDL )
    if ( m_bSDLAudioOpen )
        SDL_PauseAudioDevice( m_nAudioDeviceID, bPauseState ? 1 : 0 );
#endif
}

bool CTheoraVideoMaterial::StartVideo()
{
    if ( !m_bInitialized || m_bPlaying )
        return false;

    m_bPlaying        = true;
    m_bFinished       = false;
    m_bPaused         = false;
    m_flCurrentTime   = 0.0;
    m_nCurrentFrame   = 0;
    m_flLastUpdateTime = Plat_FloatTime();

#ifdef ANDROID
    if ( m_bSLOpen && m_pSLPlayerPlay && !m_bAudioMuted )
        (*m_pSLPlayerPlay)->SetPlayState( m_pSLPlayerPlay, SL_PLAYSTATE_PLAYING );
#elif defined( USE_SDL )
    if ( m_bSDLAudioOpen && !m_bAudioMuted )
        SDL_PauseAudioDevice( m_nAudioDeviceID, 0 );
#endif

    return true;
}

bool CTheoraVideoMaterial::StopVideo()
{
    m_bPlaying  = false;
    m_bFinished = true;
#ifdef ANDROID
    if ( m_bSLOpen && m_pSLPlayerPlay )
        (*m_pSLPlayerPlay)->SetPlayState( m_pSLPlayerPlay, SL_PLAYSTATE_PAUSED );
#elif defined( USE_SDL )
    if ( m_bSDLAudioOpen )
        SDL_PauseAudioDevice( m_nAudioDeviceID, 1 );
#endif
    return true;
}

// ---------------------------------------------------------------------------
// Position queries
// ---------------------------------------------------------------------------
float CTheoraVideoMaterial::GetVideoDuration()  { return (float) m_flVideoDuration; }
int   CTheoraVideoMaterial::GetFrameCount()     { return m_nTotalFrames; }
int   CTheoraVideoMaterial::GetCurrentFrame()   { return m_nCurrentFrame; }
float CTheoraVideoMaterial::GetCurrentVideoTime(){ return (float) m_flCurrentTime; }

bool CTheoraVideoMaterial::SetFrame( int FrameNum ) { return false; }
bool CTheoraVideoMaterial::SetTime( float flTime )  { return false; }

// ---------------------------------------------------------------------------
// YUV (YCbCr 4:2:0) → RGBA conversion — BT.601 full-range
// ---------------------------------------------------------------------------
void CTheoraVideoMaterial::ConvertYCbCrToRGBA( th_ycbcr_buffer yuv, uint8 *pDst, int dstStride )
{
    const int xOff = m_TheoraInfo.pic_x;
    const int yOff = m_TheoraInfo.pic_y;
    const int w    = m_nVideoWidth;
    const int h    = m_nVideoHeight;

    for ( int y = 0; y < h; y++ )
    {
        const uint8 *pY  = yuv[0].data + ( y + yOff ) * yuv[0].stride + xOff;
        const uint8 *pCb = yuv[1].data + ( ( y + yOff ) / 2 ) * yuv[1].stride + xOff / 2;
        const uint8 *pCr = yuv[2].data + ( ( y + yOff ) / 2 ) * yuv[2].stride + xOff / 2;

        uint8 *pRow = pDst + y * dstStride;

        for ( int x = 0; x < w; x++ )
        {
            int Y  = pY[x]         - 16;
            int Cb = pCb[ x / 2 ] - 128;
            int Cr = pCr[ x / 2 ] - 128;

            // BT.601 coefficients (scaled x256)
            int R = ( 298 * Y              + 409 * Cr + 128 ) >> 8;
            int G = ( 298 * Y - 100 * Cb   - 208 * Cr + 128 ) >> 8;
            int B = ( 298 * Y + 516 * Cb              + 128 ) >> 8;

            pRow[0] = ClampByte( R );
            pRow[1] = ClampByte( G );
            pRow[2] = ClampByte( B );
            pRow[3] = 255;
            pRow   += 4;
        }
    }
}

// ---------------------------------------------------------------------------
// UploadFrameToTexture — convert YUV, write into the procedural texture
// ---------------------------------------------------------------------------
void CTheoraVideoMaterial::UploadFrameToTexture( th_ycbcr_buffer yuv )
{
    if ( !m_Texture.IsValid() )
        return;

    int stride = m_nTexWidth * 4;
    ConvertYCbCrToRGBA( yuv, m_RGBABuffer.Base(), stride );

    ITexture *pTex = m_Texture;
    CTheoraTextureRegen regen( m_nTexWidth, m_nTexHeight );
    regen.UpdateBuffer( m_RGBABuffer.Base(), stride );
    pTex->SetTextureRegenerator( &regen );
    pTex->Download();
    pTex->SetTextureRegenerator( nullptr );
}

// ---------------------------------------------------------------------------
// OpenSL ES buffer-queue callback
// ---------------------------------------------------------------------------
#ifdef ANDROID
void CTheoraVideoMaterial::SLBufferCallback(
    SLAndroidSimpleBufferQueueItf /*q*/, void *pCtx )
{
    CTheoraVideoMaterial *pThis = static_cast<CTheoraVideoMaterial *>( pCtx );
    if ( pThis->m_bSLOpen )
        __sync_fetch_and_add( &pThis->m_SLFreeCount, 1 );
}
#endif

// ---------------------------------------------------------------------------
// QueueAudioPacket — decode a Vorbis packet and push PCM to the audio device
// ---------------------------------------------------------------------------
void CTheoraVideoMaterial::QueueAudioPacket( ogg_packet *pPacket )
{
    if ( !m_bVorbisDspReady || m_bAudioMuted )
        return;

    if ( vorbis_synthesis( &m_VorbisBlock, pPacket ) != 0 )
        return;

    vorbis_synthesis_blockin( &m_VorbisDsp, &m_VorbisBlock );

    float **ppPcm;
    int nSamples;
    while ( ( nSamples = vorbis_synthesis_pcmout( &m_VorbisDsp, &ppPcm ) ) > 0 )
    {
        int nChans = m_VorbisInfo.channels;

#ifdef ANDROID
        if ( m_bSLOpen && m_pSLBufferQueue )
        {
            for ( int i = 0; i < nSamples; i++ )
            {
                for ( int c = 0; c < nChans && c < kSLMaxChannels; c++ )
                {
                    float s = ppPcm[c][i] * m_fVolume;
                    s = s < -1.0f ? -1.0f : ( s > 1.0f ? 1.0f : s );
                    m_SLAccum.AddToTail( (int16_t)( s * 32767.0f ) );
                }
            }

            // Drain the accumulation buffer in full-chunk increments
            const int chunkSamples = kSLBufferFrames * nChans;
            while ( m_SLAccum.Count() >= chunkSamples )
            {
                // Only enqueue if we have a free buffer slot
                if ( __sync_fetch_and_add( &m_SLFreeCount, 0 ) <= 0 )
                    break; // all slots busy — drop oldest chunk to stay in sync

                __sync_fetch_and_sub( &m_SLFreeCount, 1 );
                V_memcpy( m_SLBufs[m_SLWriteIdx],
                          m_SLAccum.Base(),
                          chunkSamples * sizeof( int16_t ) );
                (*m_pSLBufferQueue)->Enqueue( m_pSLBufferQueue,
                                              m_SLBufs[m_SLWriteIdx],
                                              chunkSamples * sizeof( int16_t ) );
                m_SLWriteIdx = ( m_SLWriteIdx + 1 ) % kSLAudioBuffers;
                m_SLAccum.RemoveMultiple( 0, chunkSamples );
            }
        }

#elif defined( USE_SDL )
        // SDL Path
        if ( m_bSDLAudioOpen )
        {
            CUtlVector<float> buf;
            buf.SetCount( nSamples * nChans );
            for ( int i = 0; i < nSamples; i++ )
            {
                for ( int c = 0; c < nChans; c++ )
                {
                    float s = ppPcm[c][i] * m_fVolume;
                    buf[ i * nChans + c ] = s < -1.0f ? -1.0f : ( s > 1.0f ? 1.0f : s );
                }
            }
            SDL_QueueAudio( m_nAudioDeviceID, buf.Base(),
                            nSamples * nChans * sizeof( float ) );
        }
#endif

        vorbis_synthesis_read( &m_VorbisDsp, nSamples );
    }
}

// ---------------------------------------------------------------------------
// Update — decode the next video frame(s) that are due given elapsed time
// ---------------------------------------------------------------------------
bool CTheoraVideoMaterial::Update()
{
    if ( !m_bInitialized || !m_bPlaying || m_bPaused || m_bFinished )
        return false;

    double flNow     = Plat_FloatTime();
    double flElapsed = flNow - m_flLastUpdateTime;
    m_flLastUpdateTime = flNow;
    m_flCurrentTime   += flElapsed;

    m_bNewFrameReady = false;

    double flFrameDuration = ( m_FrameRate.GetFPS() > 0.0f )
                             ? 1.0 / m_FrameRate.GetFPS()
                             : 1.0 / 30.0;

    ogg_page   page;
    ogg_packet pkt;

    // Decode frames until we've caught up to the current time
    while ( true )
    {
        // Try to get a video packet
        while ( ogg_stream_packetout( &m_VideoStream, &pkt ) == 1 )
        {
            ogg_int64_t granulepos = pkt.granulepos;
            if ( granulepos >= 0 )
                m_pTheoraCtx && th_decode_packetin( m_pTheoraCtx, &pkt, &granulepos );
            else
                m_pTheoraCtx && th_decode_packetin( m_pTheoraCtx, &pkt, nullptr );

            th_ycbcr_buffer yuv;
            if ( th_decode_ycbcr_out( m_pTheoraCtx, yuv ) == 0 )
            {
                UploadFrameToTexture( yuv );
                m_bNewFrameReady = true;
                m_nCurrentFrame++;

                if ( m_nTotalFrames == 0 )
                    m_flVideoDuration += flFrameDuration;
            }

            // Check if we've reached the target time
            double frameTime = m_nCurrentFrame * flFrameDuration;
            if ( frameTime >= m_flCurrentTime )
                return m_bNewFrameReady;
        }

        // Drain audio packets
        while ( m_bAudioStreamFound &&
                ogg_stream_packetout( &m_AudioStream, &pkt ) == 1 )
        {
            QueueAudioPacket( &pkt );
        }

        // Need more Ogg pages
        bool bGotPage = false;
        while ( ogg_sync_pageout( &m_OggSync, &page ) == 1 )
        {
            int serial = ogg_page_serialno( &page );
            if ( serial == m_nVideoSerial )
                ogg_stream_pagein( &m_VideoStream, &page );
            else if ( m_bAudioStreamFound && serial == m_nAudioSerial )
                ogg_stream_pagein( &m_AudioStream, &page );
            bGotPage = true;
            break;
        }

        if ( !bGotPage )
        {
            // Try to read more data from the file
            if ( !ReadMoreData() )
            {
                // EOF
                if ( m_bLooping )
                {
                    g_pFullFileSystem->Seek( m_hFile, 0, FILESYSTEM_SEEK_HEAD );
                    ogg_sync_reset( &m_OggSync );
                    ogg_stream_reset( &m_VideoStream );
                    if ( m_bAudioStreamFound )
                        ogg_stream_reset( &m_AudioStream );
                    if ( m_bVorbisDspReady )
                        vorbis_synthesis_restart( &m_VorbisDsp );
                    m_nCurrentFrame = 0;
                    m_flCurrentTime = 0.0;
                    m_nTotalFrames  = m_nCurrentFrame;
                }
                else
                {
                    m_bFinished = true;
                    m_bPlaying  = false;
#ifdef ANDROID
                    if ( m_bSLOpen && m_pSLPlayerPlay )
                        (*m_pSLPlayerPlay)->SetPlayState( m_pSLPlayerPlay, SL_PLAYSTATE_PAUSED );
#elif defined( USE_SDL )
                    if ( m_bSDLAudioOpen )
                        SDL_PauseAudioDevice( m_nAudioDeviceID, 1 );
#endif
                }
                return m_bNewFrameReady;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Material / texture accessors
// ---------------------------------------------------------------------------
IMaterial *CTheoraVideoMaterial::GetMaterial()
{
    return m_Material;
}

void CTheoraVideoMaterial::GetVideoTexCoordRange( float *pMaxU, float *pMaxV )
{
    if ( m_nTexWidth > 0 && m_nTexHeight > 0 )
    {
        *pMaxU = (float) m_nVideoWidth  / (float) m_nTexWidth;
        *pMaxV = (float) m_nVideoHeight / (float) m_nTexHeight;
    }
    else
    {
        *pMaxU = *pMaxV = 1.0f;
    }
}

void CTheoraVideoMaterial::GetVideoImageSize( int *pWidth, int *pHeight )
{
    *pWidth  = m_nVideoWidth;
    *pHeight = m_nVideoHeight;
}
