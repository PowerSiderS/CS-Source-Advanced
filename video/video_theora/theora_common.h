//=============================================================================
// Theora video subsystem - shared constants
//=============================================================================

#ifndef THEORA_COMMON_H
#define THEORA_COMMON_H

#ifdef _WIN32
#pragma once
#endif

static const int cTheoraMinVideoFrameWidth  = 16;
static const int cTheoraMinVideoFrameHeight = 16;
static const int cTheoraMaxVideoFrameWidth  = 4096;
static const int cTheoraMaxVideoFrameHeight = 4096;

static const int cTheoraOggBufferSize = 65536;  // 64 KB per read

//-----------------------------------------------------------------------------
// Computes a power of two at least as big as the passed-in number
//-----------------------------------------------------------------------------
static inline int TheoraComputeNextPOT( int n )
{
    int i = 1;
    while ( i < n ) i <<= 1;
    return i;
}

#endif // THEORA_COMMON_H
