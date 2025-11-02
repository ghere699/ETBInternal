// --- START OF FILE Instances.hpp ---

#pragma once
#include "pch.h" // Includes SDK.hpp
#include <map>
#include <string>
#include <vector>

/*
*   The Instances namespace provides a centralized and efficient way to find and manage Unreal Engine UObject instances.
*   It is designed to replace scattered, repetitive object-finding logic.
*
*   Key Features:
*   - Caching: Frequently accessed objects like UWorld, APlayerController, and ULocalPlayer are cached to avoid repeated lookups.
*   - Templated Finders: Generic functions (GetInstanceOf, FindObject, etc.) to find any UObject type.
*   - Static Object Mapping: On initialization, it maps all UClass and UFunction objects for fast retrieval by name later.
*
*   Usage:
*   1. Call Instances::Initialize() once after the SDK's GObjects is ready.
*   2. Call Instances::Update() periodically (e.g., in a Tick or ProcessEvent hook) to keep cached pointers fresh.
*   3. Use functions like Instances::GetPlayerController() or Instances::GetInstanceOf<T>() to access game objects.
*/
namespace Instances
{
	// --- Initialization and Update ---

	// Should be called once when the hack is injected and GObjects is valid.
	// Maps all static classes and functions for faster lookups later.
	void Initialize();

	// Should be called periodically from a hook (e.g., PlayerTick) to update cached instances
	// that might change during gameplay (like entering/leaving a match).
	void Update();


	// --- Cached Instance Getters ---
	// These provide fast access to core game objects. They are updated by calling Instances::Update().

	UWorld* GetWorld();
	APlayerController* GetPlayerController();
	ULocalPlayer* GetLocalPlayer();
	APawn* GetLocalPawn();


	// --- Static Object Finders ---
	// These use the pre-mapped objects from Initialize() for very fast lookups.

	// Finds a UClass by its full name. Example: UClass* pcClass = FindStaticClass("Class Engine.PlayerController");
	UClass* FindStaticClass(const std::string& className);

	// Finds a UFunction by its full name. Example: UFunction* func = FindStaticFunction("Function Engine.Actor.ReceiveTick");
	UFunction* FindStaticFunction(const std::string& functionName);


	// --- Dynamic Object Finders (Templated) ---
	// These functions iterate through the global GObjects array.
	// NOTE: Use them thoughtfully, as iterating thousands of objects can be slow if done frequently in a loop.
	// It's often better to find an object once and store the pointer.

	// Gets the most recent, active instance of a class.
	// Example: class* classname = GetInstanceOf<class>();
	template<typename T>
	T* GetInstanceOf()
	{
		// Check if T is derived from UObject
		if (!std::is_base_of<UObject, T>::value) return nullptr;

		// Iterate backwards, as newer objects are often at the end of the array.
		for (int i = UObject::GObjects->Num() - 1; i >= 0; --i)
		{
			UObject* obj = UObject::GObjects->GetByIndex(i);

			if (!obj || !obj->IsA(T::StaticClass()))
				continue;

			// Skip the default template objects
			if (obj->GetFullName().find("Default__") != std::string::npos)
				continue;

			return static_cast<T*>(obj);
		}

		return nullptr;
	}

	// Gets all active instances of a class type.
	// Example: std::vector<class*> enemies = GetAllInstancesOf<class>();
	template<typename T>
	std::vector<T*> GetAllInstancesOf()
	{
		std::vector<T*> instances;

		if (!std::is_base_of<UObject, T>::value) return instances;

		for (int i = 0; i < UObject::GObjects->Num(); ++i)
		{
			UObject* obj = UObject::GObjects->GetByIndex(i);

			if (!obj || !obj->IsA(T::StaticClass()))
				continue;

			// Skip the default template objects
			if (obj->GetFullName().find("Default__") != std::string::npos)
				continue;

			instances.push_back(static_cast<T*>(obj));
		}

		return instances;
	}

	// Finds an object by its class and part of its name.
	template<typename T>
	T* FindObject(const std::string& objectName)
	{
		if (!std::is_base_of<UObject, T>::value) return nullptr;

		for (int i = 0; i < UObject::GObjects->Num(); ++i)
		{
			UObject* obj = UObject::GObjects->GetByIndex(i);

			if (!obj || !obj->IsA(T::StaticClass()))
				continue;

			if (obj->GetName().find(objectName) != std::string::npos)
			{
				return static_cast<T*>(obj);
			}
		}

		return nullptr;
	}


} // namespace Instances

// --- END OF FILE Instances.hpp ---