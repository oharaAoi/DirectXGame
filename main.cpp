#include "Window/WinApp.h"
#include "DirectXCommon/DirectXCommon.h"
#include "Function/Convert.h"
#include "Camera.h"
#include <memory>

// ログを表示するための関数
void Log(const std::string& message);

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

	// camera ----------------------------------------
	std::unique_ptr<Camera> camera = std::make_unique<Camera>();
	camera->Init();

	//==========================================================
	//	メインループ
	//==========================================================
	// ゲームの処理
	while (sWinApp->ProcessMessage()) {
		sDirectX->BeginFrame();

		sDirectX->CreateWVPResource(camera->GetVpMatrix());
		// 三角形の描画
		sDirectX->DrawCall();

		sDirectX->EndFrame();
	}

	/*sWinApp->Finalize();*/
	return 0;
}

/*=====================================================================================
	↓関数定義をこれより下で行う
======================================================================================*/

/*=================================================================================
	logを表示する関数
==================================================================================*/
void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}