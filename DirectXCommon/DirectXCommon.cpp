#include "DirectXCommon.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

DirectXCommon* DirectXCommon::GetInstacne(){
	static DirectXCommon instance;
	return &instance;
}

/*==========================================================================
	初期化
==========================================================================*/
void DirectXCommon::Initialize(WinApp* win){
	assert(win);
	winApp_ = win;

	InitializeDXGDevice();
	InitializeScreen();

	// FenceとEventの生成 -------------------------------
	HRESULT hr = S_FALSE;
	fenceValue_ = 0;
	hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	// Fenceのsignakを待つためのイベントを作成する
	fenceEvent_ = CreateEvent(NULL, false, false, NULL);
	assert(fenceEvent_ != nullptr);
}

DirectXCommon::~DirectXCommon(){

	CloseHandle(fenceEvent_);
	fence_->Release();
	rtcDescriptorHeap_->Release();
	swapChainResources_[0]->Release();
	swapChainResources_[1]->Release();
	swapChain_->Release();
	commandList_->Release();
	commandAllocator_->Release();
	commandQueue_->Release();
	device_->Release();
	useAdapter_->Release();
	dxgiFactory_->Release();
#ifdef _DEBUG
	debugController_->Release();
#endif
	
	// 
	IDXGIDebug1* debug;

	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}
}

/*==========================================================================
	DXGTデバイス初期化
==========================================================================*/
void DirectXCommon::InitializeDXGDevice(){

#ifdef _DEBUG

	debugController_ = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
		// デバックレイヤーを有効化する
		debugController_->EnableDebugLayer();
		// さらにGPU側でもチェック
		debugController_->SetEnableGPUBasedValidation(TRUE);
	}

#endif

	dxgiFactory_ = nullptr;
	// HRESULはwidows系のエラーコードであり
	// 関数が成功したかどうかをSUCCEEDEマクロ判定で判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));

	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	// どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	// --------------------------------------------------------------------------------
	// 使用するアダプタ用の変数。最初にnullptrを入れておく
	useAdapter_ = nullptr;
	// 良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter_)) != DXGI_ERROR_NOT_FOUND; ++i) {

		// アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter_->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr)); // 取得出来なかった時用
		// ソフトウェアのアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプタの情報の情報をログに出力.wstrinfgの方
			Log(ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
			break;
		}

		useAdapter_ = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
	}

	// 適切なアダプタが見つからなかったら軌道できない
	assert(useAdapter_ != nullptr);

	// ------------------------------------------------------------------------------
	device_ = nullptr;
	// 機能レベルとログ出力四の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
	};

	const char* featureLevelString[] = { "12.2", "12.1", "12.0" };

	// 高い順に生成できるか試していく
	for (size_t levels = 0; levels < _countof(featureLevels); ++levels) {
		// 採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter_, featureLevels[levels], IID_PPV_ARGS(&device_));

		// 指定した機能レベルでデバイスが生成出来たか確認
		if (SUCCEEDED(hr)) {
			// 生成できたためログ出力を行ってループを抜ける
			Log((std::format("FeatureLevel : {}\n", featureLevelString[levels])));
			break;
		}
	}

	// デバイスの生成がうまく行かなかった時
	assert(device_ != nullptr);
	Log("complete create D3D12Device\n");

#ifdef _DEBUG

	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// ヤバいエラージに止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// 抑制するメッセージのID ------------------------------
		D3D12_MESSAGE_ID denyIds[] = {
			// Windows11でのDXGIデバックレイヤーとDX12デバックレイヤーの相互作用にバグによるメッセージ
			// https://stackoverflow.com.questions/69805245/directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//指定してメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);

		// 解放
		infoQueue->Release();
	}

#endif // DEBUG

}

/*==========================================================================
	画面の色を変えるための準備
==========================================================================*/
void DirectXCommon::InitializeScreen() {
	HRESULT hr = S_FALSE;
	// GPUに命令を投げてくれる人　--------------------------
	// コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	// コマンドキューの生成がうまくいかなかったので起動できない

	// CommandListの生成 ------------------------------
	// コマンドアロケータを生成する
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	// コマンドアロケータの生成がうまく行かなかったので生成できない
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_, nullptr, IID_PPV_ARGS(&commandList_));
	// コマンドリストの生成がうまく行かなかったので生成できない
	assert(SUCCEEDED(hr));

	// スワップチェーンを生成する ---------------------------------
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	swapChainDesc.Width = 1280; // 画面の幅。ウィンドウのクライアント領域を同じ物にしておく
	swapChainDesc.Height = 720; // 画面の高さ。ウィンドウのクライアント領域を同じ物にしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2; // ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニタに移したら、中身を破棄
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_, winApp_->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain_));
	assert(SUCCEEDED(hr));

	// DescriptorHeapの生成 ----------------------------------------
	// ディスクリプタヒープの生成
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescrictorHeapDesc{};
	rtvDescrictorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー用
	rtvDescrictorHeapDesc.NumDescriptors = 2; // ダブルバッファ用に2つ。多くてもかまわない
	hr = device_->CreateDescriptorHeap(&rtvDescrictorHeapDesc, IID_PPV_ARGS(&rtcDescriptorHeap_));
	// ディスクリプタヒープを作れなかったので起動できない
	assert(SUCCEEDED(hr));

	// swapChainからResorceを引っ張ってくる -----------------------------------
	hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
	// うまく取得出来なければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
	assert(SUCCEEDED(hr));

	// RTVを作る　----------------------------------------------------
	// RTVの設定
	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; 
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtcDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	// まず1つめを作る。最初のところに作る。作る場所を指定して上げる必要がある
	rtvHandles_[0] = rtvStartHandle;
	device_->CreateRenderTargetView(swapChainResources_[0], &rtvDesc_, rtvHandles_[0]);
	// 2つめのディスクリプタハンドルを得る
	rtvHandles_[1].ptr = rtvHandles_[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// 2つめを作る
	device_->CreateRenderTargetView(swapChainResources_[1], &rtvDesc_, rtvHandles_[1]);
}

/*==========================================================================
	画面の色を変える(コマンドを積み込む)
==========================================================================*/
void DirectXCommon::BeginFrame(){
	HRESULT hr = S_FALSE;
	// コマンドを積み込んで確定させる ----------------------------
	// これから書き込む府バックバッファのインデックスを取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// 完璧な画面クリア 01_02 -----------------------
	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	// 今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources_[backBufferIndex];
	// 遷移前(現在)のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	// 遷移後のResourveState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// TransitionBArrierを張る
	commandList_->ResourceBarrier(1, &barrier);
	// -------------------------------------------

	// 描画先のRTVを設定する
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, nullptr);
	// 指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

	// ------------------------------------------------------------------
	// 画面に各処理はすべて終わり、画面に移すので状態を遷移
	// 今回はRenderTargetからPresentにする
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// TransitionBariierを張る
	commandList_->ResourceBarrier(1, &barrier);

	// ------------------------------------------------------------------

	// コマンドリストの内容を確定させる
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));
}

/*==========================================================================
	画面の色を変える(コマンドをキックする)
==========================================================================*/
void DirectXCommon::EndFrame(){
	HRESULT hr = S_FALSE;
	// GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList_ };
	commandQueue_->ExecuteCommandLists(1, commandLists);
	// GPUとOSに画面の交換を行うように通知
	swapChain_->Present(1, 0);

	// 01_02 ---------------------------------------------
	// Fenceの値を更新
	fenceValue_++;
	// GPUがここまでたどりついた時に、Fenceの値を指定した値に代入するようにsignalを送る
	commandQueue_->Signal(fence_, fenceValue_);

	// Fenceの値が指定したSignal値にたどりついているか確認する
	// GetCompletedValueの初期値はFence作成時に渡した初期値
	if (fence_->GetCompletedValue() < fenceValue_) {
		// 指定下Signalにたどりついていないので、たどりつくまで松ようにイベントを設定する
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		// イベントを待つ
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// ---------------------------------------------------

	// 次フレーム用のコマンドリストを準備
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_, nullptr);
	assert(SUCCEEDED(hr));
}


void DirectXCommon::Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}