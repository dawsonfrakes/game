#ifndef STEAMWORKS_H
#define STEAMWORKS_H

#include "gametypes.h"

bool SteamAPI_Init(void);
void SteamAPI_Shutdown(void);
bool SteamAPI_RestartAppIfNecessary(u32 unOwnAppID);

#endif
