#pragma once
#include "pch.h"

namespace Features
{
    ABPCharacter_Demo_C* GetPawn();
    void OnPawnChange(ABPCharacter_Demo_C* newPawn);
    void ItemSpawner(int selectedItemIndex, int spawnCount);
    void SpeedChanger(ABPCharacter_Demo_C* PlayerCharacter);
    void UnlockLobbyLimit(); //in development
    void KillAllPlayers();
    void TeleportAllPlayersToMe();
    void ForceEndLevel();
    void InfiniteStamina(ABPCharacter_Demo_C* PlayerCharacter);
    void InfiniteSanity(ABPCharacter_Demo_C* PlayerCharacter);
    void PlayerFly(ABPCharacter_Demo_C* PlayerCharacter);
    void ChatSpammer();

    inline int g_selectedItemIndex = 0;
    inline int g_spawnCount = 1;
    inline bool g_speedHackEnabled = false;
    inline float g_speedValue = 2500.0f;
    inline float g_originalWalkSpeed = 0.0f;
    inline float g_originalSprintSpeed = 0.0f;
    inline bool g_originalSpeedSaved = false;
    inline bool g_lastSpeedHackState = false;

    inline bool g_unlockPlayersEnabled = false;

    inline bool g_infiniteStaminaEnabled = false;
    inline bool g_lastStaminaState = false;
    inline bool g_infiniteSanityEnabled = false;
    inline ABPCharacter_Demo_C* g_lastKnownPawn = nullptr;

    inline bool g_chatSpammerEnabled = false;
    inline char g_chatSpamMessage[128] = "https://github.com/ghere699/ETBInternal";
    inline int g_chatSpamDelay = 500;

    inline bool g_flyHackEnabled = false;
    inline float g_flySpeed = 2000.0f;
    inline bool g_wasFlying = false;

    inline bool g_Unload = false;
}