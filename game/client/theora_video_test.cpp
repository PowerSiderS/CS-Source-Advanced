#include "cbase.h"
#include "cdll_client_int.h"
#include "video/ivideoservices.h"
#include "tier1/convar.h"

CON_COMMAND_F( playvideo_test, "Play an Ogg/Theora (.ogv) video file fullscreen for testing", FCVAR_DONTRECORD )
{
    if ( args.ArgC() < 2 )
    {
        Msg( "Usage: playvideo_test <path>\n"
             "  e.g. playvideo_test media/intro.ogv\n" );
        return;
    }

    if ( !g_pVideo )
    {
        Warning( "playvideo_test: video services not available\n" );
        return;
    }

    const char *pPath = args.ArgV()[1];

    // Try to resolve the file through the video services locator
    char resolvedPath[512];
    Q_strncpy( resolvedPath, pPath, sizeof( resolvedPath ) );
    VideoSystem_t system = VideoSystem::DETERMINE_FROM_FILE_EXTENSION;

    g_pVideo->LocatePlayableVideoFile(
        pPath, "GAME",
        &system, resolvedPath, sizeof( resolvedPath ),
        VideoSystemFeature::PLAY_VIDEO_FILE_FULL_SCREEN );

    Msg( "playvideo_test: playing '%s'\n", resolvedPath );

    VideoPlaybackFlags_t flags =
        VideoPlaybackFlags::CENTER_VIDEO_IN_WINDOW |
        VideoPlaybackFlags::LOCK_ASPECT_RATIO      |
        VideoPlaybackFlags::ABORT_ON_ESC;

    VideoResult_t result = g_pVideo->PlayVideoFileFullScreen(
        resolvedPath,
        nullptr,    // mainWindow — nullptr = use current game window
        0, 0,       // windowWidth/Height — 0 = auto-detect
        0, 0,       // desktopWidth/Height — 0 = auto-detect
        true,       // windowed
        0.0f,       // no forced minimum play time
        flags );

    if ( result != VideoResult::SUCCESS )
        Warning( "playvideo_test: playback failed (result=%d)\n", (int)result );
}
