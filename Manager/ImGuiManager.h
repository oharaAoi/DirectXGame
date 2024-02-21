#pragma once
#include "DirectXCommon/DirectXCommon.h"
#include "Function/DirectXUtils.h"
#include "Externals/ImGui/imgui.h"
#include "Externals/ImGui/imgui_impl_dx12.h"
#include "Externals/ImGui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class ImGuiManager{
public: // 静的メンバ関数

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static ImGuiManager* GetInstacne();

	static bool ImGuiHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


public:

	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	const ImGuiManager& operator=(const ImGuiManager&) = delete;

	void Init(WinApp* winApp, DirectXCommon* dxCommon);

	void Finalize();

	void Begin();

	void End();

	void Draw();

private:

	WinApp* winApp_;
	DirectXCommon* dxCommon_;

	ID3D12DescriptorHeap* srvHeap_;

};

