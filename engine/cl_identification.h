//========= CS-Source-Advanced ============//
//
// cl_identification.h — hardware-based persistent unique ID
//
// Generates a stable per-device ID from hardware fingerprints
// (drive serials, BIOS serial, MAC addresses, machine-id).
// Output format: STEAM_ID_<16 uppercase hex chars>
//
// No cvar, no user input required. Survives file deletion.
// Ported and adapted from xash3d-fwgs identification.c
//=========================================//

#ifndef CL_IDENTIFICATION_H
#define CL_IDENTIFICATION_H

#ifdef _WIN32
#pragma once
#endif

// Call once during engine client init (before GetNetworkIDString is used)
void ID_Init( void );

// Returns the unique ID string, format: "STEAM_ID_XXXXXXXXXXXXXXXX"
// Buffer must be at least 32 bytes. Thread-safe after ID_Init completes.
const char *ID_GetUniqueIDString( void );

#endif // CL_IDENTIFICATION_H
