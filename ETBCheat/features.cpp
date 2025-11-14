#include "pch.h"
#include "features.h"
#include "Instances.hpp"

FString CharToFString(const char* str)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	if (len == 0) return FString();
	wchar_t* wstr = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);
	FString fstr(wstr);
	delete[] wstr;
	return fstr;
}

ABPCharacter_Demo_C* Features::GetPawn()
{
	APawn* pawn = Instances::GetLocalPawn();
	if (pawn && pawn->IsA(ABPCharacter_Demo_C::StaticClass()))
	{
		return static_cast<ABPCharacter_Demo_C*>(pawn);
	}
	return nullptr;
}

void Features::OnPawnChange(ABPCharacter_Demo_C* newPawn) //basically if (new_level) pawnreset so we update the features state
{
	if (newPawn == g_lastKnownPawn)
		return;

	g_lastKnownPawn = newPawn;

	if (!newPawn)
		return;

	g_lastStaminaState = !g_infiniteStaminaEnabled;

	g_originalSpeedSaved = false;
	g_lastSpeedHackState = !g_speedHackEnabled;
}

template<typename T>
void SpawnItemByType(ABPCharacter_Demo_C* PlayerCharacter, int count)
{
	if (!PlayerCharacter) {
		return;
	}

	UClass* ItemClass = T::StaticClass();
	if (!ItemClass) {
		return;
	}

	T* DefaultItem = static_cast<T*>(ItemClass->DefaultObject);
	if (!DefaultItem) {
		return;
	}

	FName ItemID = DefaultItem->ID;

	for (int i = 0; i < count; ++i)
	{
		PlayerCharacter->DropItem_SERVER(ItemID);
	}
}

void Features::ItemSpawner(int selectedItemIndex, int spawnCount)
{
	ABPCharacter_Demo_C* PlayerCharacter = Features::GetPawn();

	if (!PlayerCharacter) {
		return;
	}
	switch (selectedItemIndex)
	{
	case 0: SpawnItemByType<ABP_Item_AlmondWater_C>(PlayerCharacter, spawnCount); break;
	case 1: SpawnItemByType<ABP_Juice_C>(PlayerCharacter, spawnCount); break;
	case 2: SpawnItemByType<ABP_EnergyBar_C>(PlayerCharacter, spawnCount); break;
	case 3: SpawnItemByType<ABP_Item_Flashlight_C>(PlayerCharacter, spawnCount); break;
	case 4: SpawnItemByType<ABP_Item_Chainsaw_C>(PlayerCharacter, spawnCount); break;
	case 5: SpawnItemByType<ABP_Item_BugSpray_C>(PlayerCharacter, spawnCount); break;
	case 6: SpawnItemByType<ABP_Liquid_Pain_C>(PlayerCharacter, spawnCount); break;
	case 7: SpawnItemByType<ABP_Item_Knife_C>(PlayerCharacter, spawnCount); break;
	case 8: SpawnItemByType<ABP_Item_Crowbar_C>(PlayerCharacter, spawnCount); break;
	case 9: SpawnItemByType<ABP_FlareGun_C>(PlayerCharacter, spawnCount); break;
	case 10: SpawnItemByType<ABP_MothJelly_C>(PlayerCharacter, spawnCount); break;
	case 11: SpawnItemByType<ABP_Rope_C>(PlayerCharacter, spawnCount); break;
	case 12: SpawnItemByType<ABP_Item_AlmondBottle_C>(PlayerCharacter, spawnCount); break;
	case 13: SpawnItemByType<ABP_Item_Firework_C>(PlayerCharacter, spawnCount); break;
	case 14: SpawnItemByType<ABP_Item_Ticket_C>(PlayerCharacter, spawnCount); break;
	case 15: SpawnItemByType<ABP_Diving_Helmet_C>(PlayerCharacter, spawnCount); break;
	case 16: SpawnItemByType<ABP_Item_Camera_C>(PlayerCharacter, spawnCount); break;
	case 17: SpawnItemByType<ABP_Item_Glowstick_Red_C>(PlayerCharacter, spawnCount); break;
	case 18: SpawnItemByType<ABP_Item_Glowstick_Blue_C>(PlayerCharacter, spawnCount); break;
	case 19: SpawnItemByType<ABP_Item_Glowstick_Yellow_C>(PlayerCharacter, spawnCount); break;
	case 20: SpawnItemByType<ABP_Thermometer_C>(PlayerCharacter, spawnCount); break;
	case 21: SpawnItemByType<ABP_WalkieTalkie_C>(PlayerCharacter, spawnCount); break;
	case 22: SpawnItemByType<ABP_DroppedItem_LiDAR_C>(PlayerCharacter, spawnCount); break;
	}
}

void Features::SpeedChanger(ABPCharacter_Demo_C* PlayerCharacter)
{
	if (!PlayerCharacter || !PlayerCharacter->CharacterMovement) return;

	if (g_speedHackEnabled != g_lastSpeedHackState)
	{
		if (g_speedHackEnabled)
		{
			if (!g_originalSpeedSaved)
			{
				g_originalWalkSpeed = PlayerCharacter->WalkSpeed;
				g_originalSprintSpeed = PlayerCharacter->SprintSpeed;
				g_originalSpeedSaved = true;
			}
		}
		else
		{
			if (g_originalSpeedSaved)
			{
				PlayerCharacter->SetWalkSpeedServer(g_originalWalkSpeed);
				PlayerCharacter->SetSprintSpeedServer(g_originalSprintSpeed);
				PlayerCharacter->CharacterMovement->MaxWalkSpeed = g_originalWalkSpeed;
				PlayerCharacter->CharacterMovement->MaxAcceleration = 2048.0f;
				g_originalSpeedSaved = false;
			}
		}
		g_lastSpeedHackState = g_speedHackEnabled;
	}

	if (g_speedHackEnabled)
	{
		static ULONGLONG next_update_tick = 0;
		ULONGLONG current_tick = GetTickCount64();

		if (current_tick >= next_update_tick)
		{
			PlayerCharacter->SetWalkSpeedServer(g_speedValue);
			PlayerCharacter->SetSprintSpeedServer(g_speedValue);

			next_update_tick = current_tick + 100;
		}

		PlayerCharacter->CharacterMovement->MaxWalkSpeed = g_speedValue;
		PlayerCharacter->CharacterMovement->MaxAcceleration = g_speedValue * 10.0f;
	}

}

void Features::UnlockLobbyLimit()
{
	if (!g_unlockPlayersEnabled) return;

	static ULONGLONG next_update_tick = 0;
	ULONGLONG current_tick = GetTickCount64();

	if (current_tick < next_update_tick) {
		return;
	}

	auto createServerWidgets = Instances::GetAllInstancesOf<UW_CreateServer_C>();

	for (UW_CreateServer_C* widget : createServerWidgets)
	{
		if (widget)
		{
			widget->MaximumPlayers = 100;

			if (widget->Slider_MaxPlayers)
			{
				widget->Slider_MaxPlayers->MaxValue = 100.0f;
				widget->Slider_MaxPlayers->Value = 100.0f;
				widget->ChangeMaxPlayerSlider(100);

			}
		}
	}
	next_update_tick = current_tick + 800;

}

void Features::KillAllPlayers()
{
	APawn* localPawn = Instances::GetLocalPawn();
	if (!localPawn)
	{
		std::cout << "[-] KillAllPlayers failed: Local Pawn not found." << std::endl;
		return;
	}

	std::vector<ABPCharacter_Demo_C*> allPlayers = Instances::GetAllInstancesOf<ABPCharacter_Demo_C>();

	std::cout << "[+] Found " << allPlayers.size() << " player(s)" << std::endl;

	if (allPlayers.empty())
	{
		return;
	}

	for (ABPCharacter_Demo_C* player : allPlayers)
	{
		if (player && player != localPawn)
		{
			player->LiquidPain();
		}
	}
}

void Features::TeleportAllPlayersToMe()
{
	APawn* localPawn = Instances::GetLocalPawn();
	if (!localPawn)
	{
		std::cout << "[-] TeleportAllPlayersToMe failed: Local Pawn not found." << std::endl;
		return;
	}

	FVector myLocation = localPawn->K2_GetActorLocation();
	std::vector<ABPCharacter_Demo_C*> allPlayers = Instances::GetAllInstancesOf<ABPCharacter_Demo_C>();

	std::cout << "[+] Found " << allPlayers.size() << " player(s) to teleport." << std::endl;

	for (ABPCharacter_Demo_C* player : allPlayers)
	{
		if (player && player != localPawn)
		{
			player->K2_SetActorLocation(myLocation, false, nullptr, true);
		}
	}
}

void Features::ForceEndLevel()
{
	UWorld* world = Instances::GetWorld();
	if (!world)
	{
		std::cout << "[-] ForceEndLevel failed: World not found." << std::endl;
		return;
	}

	AGameModeBase* gameModeBase = world->AuthorityGameMode;
	if (!gameModeBase)
	{
		std::cout << "[-] ForceEndLevel failed: GameMode not found." << std::endl;
		return;
	}

	if (gameModeBase->IsA(AMP_GameMode_C::StaticClass()))
	{
		AMP_GameMode_C* gameMode = static_cast<AMP_GameMode_C*>(gameModeBase);
		gameMode->EndGame();
	}
	else
	{
		std::cout << "[-] ForceEndLevel failed: Current GameMode is not MP_GameMode_C." << std::endl;
	}
}

void Features::InfiniteStamina(ABPCharacter_Demo_C* PlayerCharacter)
{
	if (!PlayerCharacter) return;

	if (g_infiniteStaminaEnabled != g_lastStaminaState)
	{
		if (g_infiniteStaminaEnabled)
		{
			PlayerCharacter->ShouldUseStamina = false;
		}
		else
		{
			PlayerCharacter->ShouldUseStamina = true;
		}
		g_lastStaminaState = g_infiniteStaminaEnabled;
	}
}

void Features::InfiniteSanity(ABPCharacter_Demo_C* PlayerCharacter)
{
	if (!g_infiniteSanityEnabled || !PlayerCharacter)
	{
		return;
	}

	AMP_PS_C* playerState = static_cast<AMP_PS_C*>(PlayerCharacter->PlayerState);
	if (!playerState)
	{
		return;
	}

	static ULONGLONG next_update_tick = 0;
	ULONGLONG current_tick = GetTickCount64();

	if (current_tick >= next_update_tick)
	{
		next_update_tick = current_tick + 1000;
		playerState->SRV_AddSanity(100.0f);
		playerState->AddSanity(100.0f);
	}
}

void Features::PlayerFly(ABPCharacter_Demo_C* PlayerCharacter)
{
	if (!PlayerCharacter) return;
	auto MovementComponent = PlayerCharacter->CharacterMovement;
	auto CapsuleComponent = PlayerCharacter->CapsuleComponent;
	if (!MovementComponent || !CapsuleComponent) return;

	if (g_flyHackEnabled)
	{
		if (!g_wasFlying)
		{
			g_wasFlying = true;
			MovementComponent->MovementMode = EMovementMode::MOVE_Flying;
			CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MovementComponent->GravityScale = 0.f;
			MovementComponent->BrakingDecelerationFlying = 2000.f;
		}

		APlayerController* PC = Instances::GetPlayerController();
		if (!PC) return;

		FRotator cameraRotation = PC->GetControlRotation();
		FVector moveDirection = { 0.f, 0.f, 0.f };

		if (GetAsyncKeyState('W') & 0x8000) moveDirection += UKismetMathLibrary::GetForwardVector(cameraRotation);
		if (GetAsyncKeyState('S') & 0x8000) moveDirection -= UKismetMathLibrary::GetForwardVector(cameraRotation);
		if (GetAsyncKeyState('D') & 0x8000) moveDirection += UKismetMathLibrary::GetRightVector(cameraRotation);
		if (GetAsyncKeyState('A') & 0x8000) moveDirection -= UKismetMathLibrary::GetRightVector(cameraRotation);
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) moveDirection.Z += 1.0f;
		if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) moveDirection.Z -= 1.0f;


		if (!moveDirection.IsZero())
		{
			moveDirection = UKismetMathLibrary::Normal(moveDirection, 0.0001f);
			FVector launchVelocity = moveDirection * g_flySpeed;

			PlayerCharacter->LaunchCharacter(launchVelocity, true, true);
		}
		else
		{
			MovementComponent->Velocity = { 0.f, 0.f, 0.f };
		}
	}
	else
	{
		if (g_wasFlying)
		{
			g_wasFlying = false;
			MovementComponent->MovementMode = EMovementMode::MOVE_Walking;
			MovementComponent->Velocity = { 0.f, 0.f, 0.f };
			CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MovementComponent->GravityScale = 1.0f;
			MovementComponent->BrakingDecelerationFlying = 0.f;
			MovementComponent->StopMovementImmediately();
		}
	}
}

void Features::ChatSpammer()
{
	if (!g_chatSpammerEnabled)
	{
		return;
	}

	static ULONGLONG next_spam_tick = 0;
	ULONGLONG current_tick = GetTickCount64();

	if (current_tick >= next_spam_tick)
	{
		UWB_Chat_C* chatWidget = Instances::GetInstanceOf<UWB_Chat_C>();
		if (chatWidget)
			chatWidget->Send_Global_Message(CharToFString(g_chatSpamMessage));
		next_spam_tick = current_tick + g_chatSpamDelay;
	}
}