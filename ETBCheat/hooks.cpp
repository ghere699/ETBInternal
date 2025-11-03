#include "pch.h"
#include "hooks.h"
#include "minhook/MinHook.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include "gui.h"
#include "features.h"
#include "Instances.hpp"
#include <vector>
#include <algorithm>
#include <unordered_set>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- Globals ---
static bool g_ShowMenu = true;
static HWND g_GameWindow = NULL;
static WNDPROC oWndProc = NULL;
static ptrdiff_t g_commandQueueOffset = 0;

// --- D3D12 Globals for ImGui ---
static UINT g_numFrames = 0;
static ID3D12Device* g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
static ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
static std::vector<ID3D12CommandAllocator*> g_frameCommandAllocators;
static std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> g_mainRenderTargetDescriptors;
static UINT g_frameIndex = 0;

// --- Synchronization ---
static ID3D12Fence* g_pFence = nullptr;
static HANDLE g_fenceEvent = NULL;
static UINT64 g_fenceLastSignaledValue = 0;
static std::vector<UINT64> g_frameFenceValues;

// --- Function Typedefs ---
typedef HRESULT(APIENTRY* Present_t)(IDXGISwapChain3*, UINT, UINT);
static Present_t oPresent = nullptr;
//static decltype(&ShowCursor) oShowCursor = nullptr;

static ProcessEvent_t oProcessEvent = nullptr;

// --- Helper Function for Synchronization ---
void WaitForFence(UINT64 fenceValue)
{
	if (g_pFence->GetCompletedValue() < fenceValue)
	{
		g_pFence->SetEventOnCompletion(fenceValue, g_fenceEvent);
		WaitForSingleObject(g_fenceEvent, INFINITE);
	}
}

LRESULT WINAPI hkWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Toggle the menu on INSERT key press.
	if (msg == WM_KEYUP && wParam == VK_INSERT)
	{
		g_ShowMenu = !g_ShowMenu;
		ImGui::GetIO().MouseDrawCursor = g_ShowMenu;
	}

	if (g_ShowMenu)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	}

	return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

HRESULT APIENTRY hkPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool bInitialized = false;

	if (!bInitialized)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&g_pd3dDevice))) {
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			DXGI_FORMAT backBufferFormat = sd.BufferDesc.Format;
			g_numFrames = sd.BufferCount;
			g_GameWindow = sd.OutputWindow;
			std::cout << "[+] SwapChain has " << g_numFrames << " buffers." << std::endl;

			g_frameCommandAllocators.resize(g_numFrames);
			g_mainRenderTargetDescriptors.resize(g_numFrames);
			g_frameFenceValues.resize(g_numFrames, 0);

			D3D12_DESCRIPTOR_HEAP_DESC srv_desc = {};
			srv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srv_desc.NumDescriptors = 1;
			srv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (FAILED(g_pd3dDevice->CreateDescriptorHeap(&srv_desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap))))
				return oPresent(pSwapChain, SyncInterval, Flags);

			D3D12_DESCRIPTOR_HEAP_DESC rtv_desc = {};
			rtv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtv_desc.NumDescriptors = g_numFrames;
			rtv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			rtv_desc.NodeMask = 1;
			if (FAILED(g_pd3dDevice->CreateDescriptorHeap(&rtv_desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap))))
				return oPresent(pSwapChain, SyncInterval, Flags);

			SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT i = 0; i < g_numFrames; i++)
			{
				g_mainRenderTargetDescriptors[i] = rtvHandle;
				rtvHandle.ptr += rtvDescriptorSize;
				ID3D12Resource* pBackBuffer = nullptr;
				pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
				g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptors[i]);
				if (pBackBuffer) pBackBuffer->Release();
			}

			for (UINT i = 0; i < g_numFrames; i++)
			{
				if (FAILED(g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameCommandAllocators[i]))))
					return oPresent(pSwapChain, SyncInterval, Flags);
			}

			if (FAILED(g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameCommandAllocators[0], NULL, IID_PPV_ARGS(&g_pd3dCommandList))) ||
				FAILED(g_pd3dCommandList->Close()))
				return oPresent(pSwapChain, SyncInterval, Flags);

			// Create synchronization objects
			if (FAILED(g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence))))
				return oPresent(pSwapChain, SyncInterval, Flags);

			g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			if (g_fenceEvent == NULL)
				return oPresent(pSwapChain, SyncInterval, Flags);

			// Initialize ImGui
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.MouseDrawCursor = g_ShowMenu;

			io.Fonts->AddFontDefault();
			io.Fonts->Build();

			ImGui::StyleColorsDark();
			ImGui_ImplWin32_Init(g_GameWindow);
			ImGui_ImplDX12_Init(g_pd3dDevice, g_numFrames, backBufferFormat, g_pd3dSrvDescHeap,
				g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
				g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

			oWndProc = (WNDPROC)SetWindowLongPtr(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
			bInitialized = true;
			std::cout << "[+] ImGui Initialized!" << std::endl;
			std::cout << "[+] -> Press INSERT to toggle menu." << std::endl;
		}
		else
		{
			return oPresent(pSwapChain, SyncInterval, Flags);
		}
	}

	// Get the command queue from the swapchain
	ID3D12CommandQueue* pCommandQueue = nullptr;
	if (g_commandQueueOffset > 0)
	{
		pCommandQueue = *reinterpret_cast<ID3D12CommandQueue**>(reinterpret_cast<uintptr_t>(pSwapChain) + g_commandQueueOffset);
	}
	if (!pCommandQueue) return oPresent(pSwapChain, SyncInterval, Flags);

	// Start a new ImGui frame
	ImGui_ImplDX12_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
	if (g_ShowMenu)
	{
		GUI::Render();
	}
	ImGui::Render();

	// Wait for the previous frame to finish before reusing the command allocator.
	g_frameIndex = pSwapChain->GetCurrentBackBufferIndex();
	WaitForFence(g_frameFenceValues[g_frameIndex]);

	// Get the command allocator for the current frame
	ID3D12CommandAllocator* commandAllocator = g_frameCommandAllocators[g_frameIndex];
	commandAllocator->Reset();

	// Record ImGui rendering commands
	g_pd3dCommandList->Reset(commandAllocator, nullptr);
	g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptors[g_frameIndex], FALSE, nullptr);
	g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
	g_pd3dCommandList->Close();

	// Execute the command list
	pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

	// Signal the fence with the new value for this frame.
	// This value will be used in a future frame to wait for completion.
	g_fenceLastSignaledValue++;
	pCommandQueue->Signal(g_pFence, g_fenceLastSignaledValue);
	g_frameFenceValues[g_frameIndex] = g_fenceLastSignaledValue;

	return oPresent(pSwapChain, SyncInterval, Flags);
}

void Hooks::Initialize()
{
	// Create a dummy device and swapchain to get the VTable addresses
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX12 Dummy", NULL };
	RegisterClassEx(&wc);
	HWND hWnd = CreateWindow(wc.lpszClassName, NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
	IDXGIFactory4* pFactory; CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&pFactory);
	ID3D12Device* pDummyDevice; D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&pDummyDevice);
	D3D12_COMMAND_QUEUE_DESC cqDesc = {}; cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ID3D12CommandQueue* pDummyCommandQueue; pDummyDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pDummyCommandQueue));
	DXGI_SWAP_CHAIN_DESC sd = {}; sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	IDXGISwapChain* pDummySwapChain; pFactory->CreateSwapChain(pDummyCommandQueue, &sd, &pDummySwapChain);

	uintptr_t* pSwapChainVTable = *(uintptr_t**)pDummySwapChain;

	// Find the CommandQueue offset by scanning the dummy swapchain object
	for (ptrdiff_t i = 0; i < 0x200; i += sizeof(void*))
	{
		ID3D12CommandQueue* pCandidate = *reinterpret_cast<ID3D12CommandQueue**>((uintptr_t)pDummySwapChain + i);
		if (pCandidate == pDummyCommandQueue) {
			g_commandQueueOffset = i;
			std::cout << "[+] Found CommandQueue offset: 0x" << std::hex << g_commandQueueOffset << std::dec << std::endl;
			break;
		}
	}

	void* pPresentAddress = (void*)pSwapChainVTable[8]; // Present is at index 8 for IDXGISwapChain3
	pDummySwapChain->Release(); pDummyCommandQueue->Release(); pDummyDevice->Release();
	pFactory->Release(); DestroyWindow(hWnd); UnregisterClass(wc.lpszClassName, wc.hInstance);

	if (g_commandQueueOffset == 0) { std::cout << "[-] Failed to find CommandQueue offset!" << std::endl; return; }
	if (MH_Initialize() != MH_OK) { std::cout << "[-] MinHook init failed!" << std::endl; return; }
	if (MH_CreateHook(pPresentAddress, &hkPresent, reinterpret_cast<void**>(&oPresent)) != MH_OK) { std::cout << "[-] CreateHook failed!" << std::endl; return; }
	UWorld* world = UWorld::GetWorld();
	if (world && world->OwningGameInstance)
	{
		uintptr_t** VTable = *(uintptr_t***)world->OwningGameInstance;

		void* pProcessEventAddr = (void*)VTable[Offsets::ProcessEventIdx];

		if (MH_CreateHook(pProcessEventAddr, &Hooks::hkProcessEvent, reinterpret_cast<void**>(&oProcessEvent)) != MH_OK)
		{
			std::cout << "[-] CreateHook (ProcessEvent) failed!" << std::endl;
		}
		else
		{
			std::cout << "[+] ProcessEvent hook created successfully!" << std::endl;
		}
	}
	else
	{
		std::cout << "[-] Failed to get UGameInstance to hook ProcessEvent." << std::endl;
	}
	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) { std::cout << "[-] EnableHook failed!" << std::endl; return; }
	std::cout << "[+] Hooks initialized successfully!" << std::endl;
}

void Hooks::Shutdown()
{
	// 1. & 2. Restore original state immediately to prevent new calls
	if (oWndProc && g_GameWindow)
		SetWindowLongPtr(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)oWndProc);

	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);

	// 3. Wait for the GPU to finish any work that was in flight
	if (g_pFence && g_fenceEvent) {
		WaitForFence(g_fenceLastSignaledValue);
	}

	// 4. Shutdown ImGui and release D3D12 resources
	if (g_pd3dDevice) {
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	// Release synchronization primitives
	if (g_fenceEvent) CloseHandle(g_fenceEvent);
	if (g_pFence) g_pFence->Release();

	// Release all other D3D12 objects
	if (g_pd3dRtvDescHeap) g_pd3dRtvDescHeap->Release();
	if (g_pd3dSrvDescHeap) g_pd3dSrvDescHeap->Release();
	for (auto& allocator : g_frameCommandAllocators) { if (allocator) allocator->Release(); }
	g_frameCommandAllocators.clear();
	if (g_pd3dCommandList) g_pd3dCommandList->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();

	// 5. Uninitialize MinHook at the end
	MH_Uninitialize();
}

void Hooks::hkProcessEvent(SDK::UObject* pThis, SDK::UFunction* Function, void* Parms)
{
	if (pThis && Function)
	{
		/*
		static const std::unordered_set<std::string> exclusionList = {
			"Function W_VersionNumber.W_VersionNumber_C.Get_VersionText_Visibility_0",
			"Function W_MainMenu.W_MainMenu_C.OnlyVisibleOnXSX",
			"Function UMG.VisibilityBinding.GetValue",
			"Function W_MainMenu.W_MainMenu_C.Get_JoinGameButton_bIsEnabled_0",
			"Function BP_MenuFlickering.BP_MenuFlickering_C.Flicker__UpdateFunc",
			"Function W_MainMenu.W_MainMenu_C.Tick",
			"Function BPCharacter_Demo.BPCharacter_Demo_C.ReceiveTick",
			"Function W_MainMenu.W_MainMenu_C.GetPlatformBasedVisibility",
			"Function UMG.BoolBinding.GetValue",
			"Function W_MainMenu.W_MainMenu_C.ExecuteUbergraph_W_MainMenu",
			"Function Engine.CameraModifier.BlueprintModifyPostProcess",
			"Function Engine.CameraModifier.BlueprintModifyCamera",
			"Function UI_TheHubStats.UI_TheHubStats_C.UpdateTime",
			"Function UMG.UserWidget.OnMouseMove",
			"Function BP_MyGameInstance.BP_MyGameInstance_C.CheckAchievementQueue",
			"Function UI_Menu_ModeSelection.UI_Menu_ModeSelection_C.GetVisibilityPremiumFeatureErrorPS5",
			"Function UI_Menu_ModeSelection.UI_Menu_ModeSelection_C.GetVisibilityPremiumFeatureErrorXSX",
			"Function UI_Menu_ModeSelection.UI_Menu_ModeSelection_C.Get Visibility Online Error"
		};


		if (exclusionList.find(functionName) == exclusionList.end())
		{
			//std::cout << "[ProcessEvent] -> " << functionName << std::endl;
		}
		*/

		std::string functionName = Function->GetFullName();

		std::string lowerFunctionName = functionName;
		std::transform(lowerFunctionName.begin(), lowerFunctionName.end(), lowerFunctionName.begin(),
			[](unsigned char c) { return std::tolower(c); });


		/*
		if (lowerFunctionName.find("chat") != std::string::npos)
		{
			std::cout << "[ProcessEvent] -> " << functionName << std::endl;
		}
		*/

		if (functionName == "Function BPCharacter_Demo.BPCharacter_Demo_C.ReceiveTick")
		{
			Instances::Update();

			ABPCharacter_Demo_C* pLocalPlayer = Features::GetPawn();
			Features::OnPawnChange(pLocalPlayer);
			if (pLocalPlayer)
			{
				Features::SpeedChanger(pLocalPlayer);
				Features::InfiniteStamina(pLocalPlayer);
				Features::InfiniteSanity(pLocalPlayer);
			}
		}

		if (functionName == "Function WB_Chat.WB_Chat_C.Tick")
		{
			Instances::Update();
			//Features::UnlockLobbyLimit();

			ABPCharacter_Demo_C* pLocalPlayer = Features::GetPawn();
			if (pLocalPlayer)
			{
				Features::PlayerFly(pLocalPlayer);
			}
			Features::ChatSpammer();
		}

	}

	return oProcessEvent(pThis, Function, Parms);
}
