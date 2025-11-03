#include "pch.h"
#include "gui.h"
#include "features.h"
#include "imgui/imgui.h"

static const char* g_itemNames[] = {
	"Almond Water",
	"Juice",
	"Energy Bar",
	"Flashlight",
	"Chainsaw",
	"Bug Spray",
	"Liquid Pain",
	"Knife",
	"Crowbar",
	"Flare Gun",
	"Moth Jelly",
	"Rope",
	"Almond Bottle",
	"Firework",
	"Ticket",
	"Diving Helmet",
	"Camera",
	"Glowstick (Red)",
	"Glowstick (Blue)",
	"Glowstick (Yellow)",
	"Thermometer",
	"Walkie-Talkie",
	"Lidar"
};

void GUI::Render()
{
	if (ImGui::Begin("Escape The Backrooms Internal"))
	{

		ImGui::Text("Item Spawner");
		ImGui::Combo("Item", &Features::g_selectedItemIndex, g_itemNames, IM_ARRAYSIZE(g_itemNames));
		ImGui::SliderInt("Amount", &Features::g_spawnCount, 1, 50);

		if (ImGui::Button("Spawn Selected Item"))
		{
			Features::ItemSpawner(Features::g_selectedItemIndex, Features::g_spawnCount);
		}

		ImGui::Separator();

		ImGui::Checkbox("Infinite Stamina", &Features::g_infiniteStaminaEnabled);
		ImGui::Checkbox("Infinite Sanity", &Features::g_infiniteSanityEnabled);

		ImGui::Text("Player Features (Host Only)");

		ImGui::Checkbox("Speedhack", &Features::g_speedHackEnabled);

		if (Features::g_speedHackEnabled)
		{
			ImGui::SliderFloat("Speed", &Features::g_speedValue, 100.f, 1600.0f, "%.0f");
		}

		ImGui::Checkbox("Fly", &Features::g_flyHackEnabled);
		if (Features::g_flyHackEnabled)
		{
			ImGui::SliderFloat("Fly Speed", &Features::g_flySpeed, 500.f, 10000.0f, "%.0f");
		}

		//ImGui::Checkbox("Unlock Max Players to 100", &Features::g_unlockPlayersEnabled);

		ImGui::Text("Chat Spammer");
		ImGui::Checkbox("Enable##ChatSpam", &Features::g_chatSpammerEnabled);
		if (Features::g_chatSpammerEnabled)
		{
			ImGui::InputText("Message##ChatSpam", Features::g_chatSpamMessage, IM_ARRAYSIZE(Features::g_chatSpamMessage));
			ImGui::SliderInt("Delay (ms)##ChatSpam", &Features::g_chatSpamDelay, 100, 5000);
		}


		if (ImGui::Button("Teleport All To Me"))
		{
			Features::TeleportAllPlayersToMe();
		}

		if (ImGui::Button("Kill All Players"))
		{
			Features::KillAllPlayers();
		}

		if (ImGui::Button("Unload"))
		{
			Features::g_Unload = true;
		}
	}
	ImGui::End();
}