#include "Window/WinApp.h"
#include "DirectXCommon/DirectXCommon.h"
#include "Function/Convert.h"

// ログを表示するための関数
void Log(const std::string& message);

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
	sDirectX->Initialize(sWinApp);
	

	/*==========================================================
		メインループ
	==========================================================*/
	MSG msg{};
	while (msg.message != WM_QUIT) {
		// windowにメッセージが来てたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			// ゲームの処理
			sDirectX->BeginFrame();
			sDirectX->EndFrame();
		}
	}

	return 0;
}

/*===================================================================================== =
	↓関数定義をこれより下で行う
====================================================================================== = */

/*=================================================================================
	logを表示する関数
==================================================================================*/
void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}