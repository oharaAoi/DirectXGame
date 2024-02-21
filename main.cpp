#include "Window/WinApp.h"
#include "DirectXCommon/DirectXCommon.h"
#include "Function/Convert.h"
#include "Camera.h"
#include <memory>

#include "ImGuiManager.h"

static const int kWindowWidth = 1280;
static const int kWindowHeight = 720;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	// 出力ウィンドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	// windowの生成 -----------------------------------
	WinApp* sWinApp = nullptr;
	assert(!sWinApp);
	sWinApp = WinApp::GetInstance();

	sWinApp->CreateGameWindow();

	// DirectXの準備 ----------------------------------
	DirectXCommon* sDirectX = nullptr;
	assert(!sDirectX);
	sDirectX = DirectXCommon::GetInstacne();
	sDirectX->Initialize(sWinApp, kWindowWidth, kWindowHeight);

	// ImGui -----------------------------------------
	ImGuiManager* imGuiManager = nullptr;
	imGuiManager = ImGuiManager::GetInstacne();
	imGuiManager->Init(sWinApp, sDirectX);

	// camera ----------------------------------------
	std::unique_ptr<Camera> camera = std::make_unique<Camera>();
	camera->Init();

	//==========================================================
	//	メインループ
	//==========================================================
	// ゲームの処理
	while (sWinApp->ProcessMessage()) {
		imGuiManager->Begin();
		sDirectX->BeginFrame();

		sDirectX->CreateWVPResource(camera->GetVpMatrix());

		ImGui::ShowDemoWindow();
		// 三角形の描画
		sDirectX->DrawCall();

		imGuiManager->End();
		imGuiManager->Draw();
		
		sDirectX->EndFrame();
	}

	imGuiManager->Finalize();
	/*sWinApp->Finalize();*/
	return 0;
}