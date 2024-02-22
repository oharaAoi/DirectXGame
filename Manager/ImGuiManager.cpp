#include "ImGuiManager.h"

#include "Window/WinApp.h"

ImGuiManager* ImGuiManager::GetInstacne(){
	static ImGuiManager instance;
	return &instance;
}

bool ImGuiManager::ImGuiHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam){
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	return false;
}

void ImGuiManager::Init(WinApp* winApp, DirectXCommon* dxCommon){

	assert(winApp);
	assert(dxCommon);

	winApp_ = winApp;
	dxCommon_ = dxCommon;

	srvHeap_ = dxCommon_->GetSRVHeap();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp->GetHwnd());
	ImGui_ImplDX12_Init(
		dxCommon->GetDevice(),
		dxCommon->GetBufferCount(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		srvHeap_,
		srvHeap_->GetCPUDescriptorHandleForHeapStart(),
		srvHeap_->GetGPUDescriptorHandleForHeapStart()
	);

}

void ImGuiManager::Finalize(){
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	srvHeap_->Release();
}

void ImGuiManager::Begin(){
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::End(){
	ImGui::Render();
}

void ImGuiManager::Draw(){
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon_->GetCommandList());
}
