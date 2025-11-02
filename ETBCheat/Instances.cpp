// --- START OF FILE Instances.cpp ---

#include "Instances.hpp"

namespace Instances
{
	// --- Private cached pointers ---
	// These variables store pointers to frequently used objects to avoid searching for them repeatedly.
	// They are kept within this .cpp file to hide them from external access. Direct access should be through the getter functions.
	namespace
	{
		UWorld* GWorld = nullptr;
		APlayerController* GPlayerController = nullptr;
		ULocalPlayer* GLocalPlayer = nullptr;
		APawn* GLocalPawn = nullptr;

		// Maps for storing static objects for fast lookups.
		std::map<std::string, UClass*> GStaticClasses;
		std::map<std::string, UFunction*> GStaticFunctions;
	}

	// --- Internal Helper Functions ---

	// Scans the GObjects array and populates the static maps.
	void MapObjects()
	{
		GStaticClasses.clear();
		GStaticFunctions.clear();

		for (int i = 0; i < UObject::GObjects->Num(); ++i)
		{
			UObject* obj = UObject::GObjects->GetByIndex(i);
			if (!obj) continue;

			// Check if the object is a UClass
			if (obj->IsA(UClass::StaticClass()))
			{
				GStaticClasses.emplace(obj->GetFullName(), static_cast<UClass*>(obj));
			}
			// Check if the object is a UFunction
			else if (obj->IsA(UFunction::StaticClass()))
			{
				GStaticFunctions.emplace(obj->GetFullName(), static_cast<UFunction*>(obj));
			}
		}
		std::cout << "[Instances] Mapped " << GStaticClasses.size() << " classes and " << GStaticFunctions.size() << " functions." << std::endl;
	}

	// --- Public Functions ---

	void Initialize()
	{
		MapObjects();
		Update(); // Perform an initial update to cache pointers.
		std::cout << "[Instances] Initialized successfully." << std::endl;
	}

	void Update()
	{
		// The order of acquisition is important here.
		GWorld = UWorld::GetWorld();
		if (!GWorld)
		{
			// If world is null, everything else probably is too.
			GLocalPlayer = nullptr;
			GPlayerController = nullptr;
			GLocalPawn = nullptr;
			return;
		}

		UGameInstance* gameInstance = GWorld->OwningGameInstance;
		if (gameInstance && gameInstance->LocalPlayers.IsValidIndex(0))
		{
			GLocalPlayer = gameInstance->LocalPlayers[0];
		}
		else
		{
			GLocalPlayer = nullptr;
		}

		if (GLocalPlayer)
		{
			GPlayerController = GLocalPlayer->PlayerController;
		}
		else
		{
			GPlayerController = nullptr;
		}

		if (GPlayerController)
		{
			GLocalPawn = GPlayerController->K2_GetPawn();
		}
		else
		{
			GLocalPawn = nullptr;
		}
	}

	UWorld* GetWorld()
	{
		return GWorld;
	}

	APlayerController* GetPlayerController()
	{
		return GPlayerController;
	}

	ULocalPlayer* GetLocalPlayer()
	{
		return GLocalPlayer;
	}

	APawn* GetLocalPawn()
	{
		return GLocalPawn;
	}

	UClass* FindStaticClass(const std::string& className)
	{
		if (GStaticClasses.count(className))
		{
			return GStaticClasses[className];
		}
		return nullptr;
	}

	UFunction* FindStaticFunction(const std::string& functionName)
	{
		if (GStaticFunctions.count(functionName))
		{
			return GStaticFunctions[functionName];
		}
		return nullptr;
	}

} // namespace Instances

// --- END OF FILE Instances.cpp ---