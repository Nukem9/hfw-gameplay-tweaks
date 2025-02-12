#include <unordered_set>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>
#include "../../ModConfiguration.h"
#include "../../ModCoreEvents.h"
#include "../Core/Player.h"
#include "../Core/Mover.h"
#include "../Core/Destructibility.h"
#include "../Core/DebugSettings.h"
#include "../Core/JobHeaderCPU.h"
#include "../Nx/NxD3DImpl.h"
#include "../Nx/NxDXGIImpl.h"
#include "EntitySpawnerWindow.h"
#include "DebugUI.h"
#include "MainMenuBar.h"

// This has to be forward declared as ImGui avoids leaking Win32 types in header files
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace HRZ2::DebugUI
{
	std::unordered_map<std::string, std::shared_ptr<Window>> m_Windows;

	std::vector<ID3D12CommandAllocator *> CommandAllocators;
	ID3D12GraphicsCommandList *CommandList;

	ID3D12DescriptorHeap *SrvDescriptorHeap;
	ID3D12DescriptorHeap *RtvDescriptorHeap;

	WNDPROC OriginalWndProc;
	bool InterceptInput;

	LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	void Initialize(NxDXGIImpl *DXGIImpl)
	{
		// Steal the device and window handle from the swap chain
		ID3D12Device *device = nullptr;
		DXGIImpl->m_DXGISwapChain->GetDevice(IID_PPV_ARGS(&device));

		HWND windowHandle = nullptr;
		DXGIImpl->m_DXGISwapChain->GetHwnd(&windowHandle);

		// Grab back buffers and create d3d resources
		auto hr = S_OK;

		const D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 16,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};

		hr |= device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&SrvDescriptorHeap));

		const D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = 8,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		};

		hr |= device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&RtvDescriptorHeap));

		CommandAllocators.resize(DXGIImpl->m_NumBuffers);

		ID3D12Resource *tempBackBuffer = nullptr;
		hr |= DXGIImpl->m_DXGISwapChain->GetBuffer(0, IID_PPV_ARGS(&tempBackBuffer));
		auto backBufferDesc = tempBackBuffer->GetDesc();
		tempBackBuffer->Release();

		for (uint32_t i = 0; i < CommandAllocators.size(); i++)
			hr |= device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocators[i]));

		hr |= device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0], nullptr, IID_PPV_ARGS(&CommandList));

		if (FAILED(hr))
			__debugbreak();

		CommandList->Close();

		// Initialize ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		auto& style = ImGui::GetStyle();
		style.FrameBorderSize = 1;
		style.ScrollbarRounding = 0;

		auto& io = ImGui::GetIO();
		io.FontGlobalScale = ModConfiguration.DebugMenuFontScale;
		io.MouseDrawCursor = false;

		// Create D3D12 resources
		ImGui_ImplWin32_Init(windowHandle);
		ImGui_ImplDX12_Init(
			device,
			DXGIImpl->m_NumBuffers,
			backBufferDesc.Format,
			SrvDescriptorHeap,
			SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(windowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc)));

		DebugUI::AddWindow(std::make_shared<MainMenuBar>());
	}

	void AddWindow(std::shared_ptr<Window> Handle)
	{
		// Immediately discard duplicate window instances
		auto id = Handle->GetId();

		if (!m_Windows.contains(id))
			m_Windows.emplace(id, Handle);
	}

	void RenderUI()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		UpdatePlayerSpecific();
		UpdateFreecam();

		// Clear window focus for every frame that isn't intercepting input
		if (!InterceptInput)
			ImGui::FocusWindow(nullptr);

		// A copy is required because Render() might create new instances and invalidate iterators
		auto currentWindows = m_Windows;

		for (auto& [name, window] : currentWindows)
		{
			// Detach windows that are pending close first
			if (window->Close())
				m_Windows.erase(name);
			else
				window->Render();
		}

		ImGui::Render();
	}

	void RenderUID3D(NxD3DImpl *D3DImpl, NxDXGIImpl *DXGIImpl)
	{
		const auto drawData = ImGui::GetDrawData();

		if (drawData->Valid && drawData->CmdListsCount > 0)
		{
			const auto bufferIndex = DXGIImpl->m_DXGISwapChain->GetCurrentBackBufferIndex();
			const auto buffer = D3DImpl->GetD3D12GameBackBuffer(bufferIndex);

			// Create a brand new RTV each frame. Tracking this in engine code is too difficult.
			const auto rtvHandle = RtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			D3DImpl->GetD3D12Device()->CreateRenderTargetView(buffer, nullptr, rtvHandle);

			// Reset command allocator, command list, then draw
			auto allocator = CommandAllocators[bufferIndex];
			allocator->Reset();

			D3D12_RESOURCE_BARRIER barrier = {
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition = {
					.pResource = buffer,
					.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
					.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
				},
			};

			CommandList->Reset(allocator, nullptr);
			CommandList->ResourceBarrier(1, &barrier);

			CommandList->SetDescriptorHeaps(1, &SrvDescriptorHeap);
			CommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
			ImGui_ImplDX12_RenderDrawData(drawData, CommandList);

			std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
			CommandList->ResourceBarrier(1, &barrier);
			CommandList->Close();

			D3DImpl->GetD3D12CommandQueue(0)->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList *const *>(&CommandList));
		}
	}

	std::optional<LRESULT> HandleMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		switch (Msg)
		{
		case WM_KEYDOWN:
		{
			if (ModConfiguration.Hotkeys.ToggleDebugUI == VK_OEM_3 && wParam == VK_OEM_8)
				wParam = VK_OEM_3; // Workaround for UK keyboard layouts

			if (wParam == ModConfiguration.Hotkeys.ToggleDebugUI)
			{
				// Toggle input blocking and the main menu bar
				MainMenuBar::ToggleVisibility();

				InterceptInput = !InterceptInput;
				ImGui::GetIO().MouseDrawCursor = InterceptInput;

				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.TogglePauseGameLogic)
			{
				MainMenuBar::TogglePauseGameLogic();
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.ToggleAIProcessing)
			{
				MainMenuBar::TogglePauseAIProcessing();
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.TogglePauseTimeOfDay)
			{
				MainMenuBar::TogglePauseTimeOfDay();
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.ToggleFreeCamera)
			{
				MainMenuBar::ToggleFreeflyCamera();
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.ToggleNoclip)
			{
				MainMenuBar::ToggleNoclip();
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.IncreaseTimescale)
			{
				MainMenuBar::AdjustTimescale(0.25f);
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.DecreaseTimescale)
			{
				MainMenuBar::AdjustTimescale(-0.25f);
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.ToggleTimescale)
			{
				MainMenuBar::ToggleTimescaleOverride();
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.SpawnEntity)
			{
				EntitySpawnerWindow::ForceSpawnEntityClick();
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.QuickSave)
			{
				MainMenuBar::ToggleQuickSave();
				return 1;
			}
			else if (wParam == ModConfiguration.Hotkeys.QuickLoad)
			{
				MainMenuBar::ToggleQuickLoad();
				return 1;
			}
		}
		break;
		}

		// Allow imgui to process it
		ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);

		if (ShouldInterceptInput())
		{
			const static std::unordered_set<UINT> blockedMessages = {
				WM_MOUSEMOVE,	WM_MOUSELEAVE,	  WM_LBUTTONDOWN, WM_LBUTTONDBLCLK, WM_RBUTTONDOWN, WM_RBUTTONDBLCLK,
				WM_MBUTTONDOWN, WM_MBUTTONDBLCLK, WM_XBUTTONDOWN, WM_XBUTTONDBLCLK, WM_LBUTTONUP,	WM_RBUTTONUP,
				WM_MBUTTONUP,	WM_XBUTTONUP,	  WM_MOUSEWHEEL,  WM_MOUSEHWHEEL,	WM_KEYDOWN,		WM_KEYUP,
				WM_SYSKEYDOWN,	WM_SYSKEYUP,	  WM_CHAR,		  WM_INPUT,
			};

			if (blockedMessages.contains(Msg))
				return 0;
		}

		// Return nullopt to indicate the game should process the message instead
		return std::nullopt;
	}

	LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		auto handled = HandleMessage(hWnd, Msg, wParam, lParam);

		if (handled)
			return handled.value();

		return CallWindowProcA(OriginalWndProc, hWnd, Msg, wParam, lParam);
	}

	bool ShouldInterceptInput()
	{
		return InterceptInput || MainMenuBar::m_FreeCamMode == MainMenuBar::FreeCamMode::Free;
	}

	void UpdatePlayerSpecific()
	{
		auto debugSettings = DebugSettings::GetInstance();
		auto player = Player::GetLocalPlayer();

		if (!debugSettings || !player || !player->m_Entity)
			return;

		if (MainMenuBar::m_EnableInfiniteClipAmmo)
			debugSettings->m_InfiniteSizeClip = true;

		if (auto destructibility = player->m_Entity->m_Destructibility)
		{
			if (MainMenuBar::m_EnableGodMode)
				destructibility->m_Invulnerable = true;

			if (MainMenuBar::m_EnableDemigodMode)
				destructibility->m_DieAtZeroHealth = false;
		}

		if (MainMenuBar::m_EnableAutoNeutralFaction)
		{
			JobHeaderCPU::SubmitCallback([]
			{
				auto& modEvents = ModCoreEvents::GetInstance();
				std::shared_lock lock(modEvents.m_CachedDataMutex);

				for (auto faction : modEvents.m_CachedAIFactions)
				{
					if (faction->m_UUID == GGUUID::Parse("057D8132-C39A-D247-80B4-E35DF0F42098"))
					{
						if (auto player = Player::GetLocalPlayer(); player && player->m_Entity)
							player->m_Entity->SetFaction(reinterpret_cast<AIFaction *>(faction));

						break;
					}
				}
			});
		}

		if (MainMenuBar::m_LODRangeModifier != std::numeric_limits<float>::max())
		{
			// Emulate RepresentationManagerGame::SetLODDistanceMultiplier
			const auto representationManager = *Offsets::Signature("48 8B 05 ? ? ? ? 48 8B 88 10 1C 00 00 48 63 80 08 1C 00 00 48 8D 14 C1")
													.AsRipRelative(7)
													.ToPointer<uintptr_t>();

			if (representationManager)
			{
				auto& gameViewArray = *reinterpret_cast<Array<uintptr_t> *>(representationManager + 0x1C08); // TODO

				for (auto gameView : gameViewArray)
					*reinterpret_cast<float *>(gameView + 0x468) = MainMenuBar::m_LODRangeModifier; // TODO
			}
		}
	}

	void UpdateFreecam()
	{
		const auto& io = ImGui::GetIO();
		const auto cameraMode = MainMenuBar::m_FreeCamMode;

		if (cameraMode == MainMenuBar::FreeCamMode::Off || !Player::GetLocalPlayer())
			return;

		// Set up the camera's rotation matrix
		float adjustYaw = 0.0f;
		float adjustPitch = 0.0f;

		if (cameraMode == MainMenuBar::FreeCamMode::Free)
		{
			// Convert mouse X/Y to yaw/pitch angles
			static float currentCursorX = 0.0f;
			static float currentCursorY = 0.0f;
			static float targetCursorX = 0.0f;
			static float targetCursorY = 0.0f;

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
			{
				targetCursorX += io.MouseDelta.x * 0.5f;
				targetCursorY += io.MouseDelta.y * 0.5f;
			}

			// Exponential decay view angle smoothing (https://stackoverflow.com/a/10228863)
			const double springiness = 60.0;
			const float mult = static_cast<float>(1.0 - std::exp(std::log(0.5) * springiness * io.DeltaTime));

			currentCursorX += (targetCursorX - currentCursorX) * mult;
			currentCursorY += (targetCursorY - currentCursorY) * mult;

			float degreesX = std::fmodf(currentCursorX, 360.0f);
			float degreesY = std::fmodf(currentCursorY, 360.0f);

			if (degreesX < 0)
				degreesX += 360.0f;

			if (degreesY < 0)
				degreesY += 360.0f;

			// Degrees to radians
			adjustYaw = degreesX * (3.14159f / 180.0f);
			adjustPitch = degreesY * (3.14159f / 180.0f);
		}

		// Scale camera velocity based on delta time
		float baseSpeed = io.DeltaTime * 5.0f;

		if (io.KeysDown[ImGuiKey_LeftShift])
			baseSpeed *= 10.0f;
		else if (io.KeysDown[ImGuiKey_LeftCtrl])
			baseSpeed /= 5.0f;

		// WSAD keys for movement
		float forwardSpeed = 0.0f;
		float strafeSpeed = 0.0f;

		if (io.KeysDown[ImGuiKey_W])
			forwardSpeed += baseSpeed;

		if (io.KeysDown[ImGuiKey_S])
			forwardSpeed -= baseSpeed;

		if (io.KeysDown[ImGuiKey_A])
			strafeSpeed -= baseSpeed;

		if (io.KeysDown[ImGuiKey_D])
			strafeSpeed += baseSpeed;

		JobHeaderCPU::SubmitCallable([forwardSpeed, strafeSpeed, adjustYaw, adjustPitch]
		{
			auto player = Player::GetLocalPlayer();
			auto camera = player ? player->GetLastActivatedCamera() : nullptr;

			if (!camera)
				return;

			auto cameraMode = MainMenuBar::m_FreeCamMode;
			auto& cameraTransform = MainMenuBar::m_FreeCamPosition;

			if (cameraMode == MainMenuBar::FreeCamMode::Free)
			{
				cameraTransform.Orientation = RotMatrix(adjustYaw, adjustPitch, 0.0f);
				cameraTransform.Position += cameraTransform.Orientation.Forward() * forwardSpeed;
				cameraTransform.Position += cameraTransform.Orientation.Right() * strafeSpeed;

				camera->SetWorldTransform(cameraTransform);
			}
			else if (cameraMode == MainMenuBar::FreeCamMode::Noclip && player->m_Entity)
			{
				cameraTransform.Orientation = camera->GetWorldTransform().Orientation;
				cameraTransform.Position += cameraTransform.Orientation.Forward() * forwardSpeed;
				cameraTransform.Position += cameraTransform.Orientation.Right() * strafeSpeed;

				player->m_Entity->m_Mover->OverrideMovement(cameraTransform, 0.0001f, false);
			}
		});
	}
}
