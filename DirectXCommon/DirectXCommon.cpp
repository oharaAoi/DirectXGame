#include "DirectXCommon.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

#include "ImGuiManager.h"

DirectXCommon* DirectXCommon::GetInstacne(){
	static DirectXCommon instance;
	return &instance;
}

void DirectXCommon::Finalize() {
	transformationMatrixResorce_->Release();
	vertexResourceSprite_->Release();

	dsvDescriptorHeap_->Release();
	depthStencilResource_->Release();

	textureResource_->Release();
	srvHeap_->Release();

	wvpResource_->Release();
	materialResource_->Release();
	vertexResource_->Release();
	graphicsPipelineState_->Release();
	rootSigneture_->Release();
	if (errorBlob_) {
		errorBlob_->Release();
	}
	pixelShaderBlob_->Release();
	vertexShaderBlob_->Release();
	includeHandler_->Release();
	dxcCompiler_->Release();
	dxcUtils_->Release();

	//
	CloseHandle(fenceEvent_);
	fence_->Release();
	swapChainResources_[0]->Release();
	swapChainResources_[1]->Release();
	rtvDescriptorHeap_->Release();
	intermediateResource_->Release();
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

	winApp_->Finalize();

	// 
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}
}

/*=============================================================================================================================
	初期化
=============================================================================================================================*/
void DirectXCommon::Initialize(WinApp* win, int32_t backBufferWidth, int32_t backBufferHeight){
	assert(win);
	winApp_ = win;
	kClientWidth_ = backBufferWidth;
	kClientHeight_ = backBufferHeight;

	// -----------------------------------
	bufferCount_ = 2;

	// -----------------------------------
	transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	transformSprite_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	// -----------------------------------

	// DirectXの初期化
	InitializeDXGDevice();
	// 画面を青くするための初期化
	InitializeScreen();
	// CPUとGPUの同期を取るための
	CreateFence();
	// DXC(DirectXShaderCompilerの初期化)
	InitializeDXC();
	// PSO(PipelineStateObject)の生成
	CreatePSO();
	// 頂点データの生成
	CreateVertexResource();
	// spriteの生成
	CreateSprite();

	// texture
	CreateTexture();

	// texture
	intermediateResource_ = UploadTextureData(textureResource_, mipImage_, device_, commandList_);
}

/*=============================================================================================================================
	DXGTデバイス初期化
=============================================================================================================================*/
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

/*=============================================================================================================================
	DXCの初期化
=============================================================================================================================*/
void DirectXCommon::InitializeDXC() {
	HRESULT hr = S_FALSE;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));

	// includeに対応するための設定
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}

/*=============================================================================================================================
	画面の色を変えるための準備
=============================================================================================================================*/
void DirectXCommon::InitializeScreen() {
	// コマンドを生成
	CreateCommand();
	// スワップチェーンの生成
	SwapChainCreate();
	// DescriptorHeapの生成
	CreateRTVHeap();
	// ---------------------

	// RTV(RenderTargetView)を作る
	CreateRTV();
	srvHeap_ = CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// ---------------------
	// 深度
	depthStencilResource_ = CreateDepthStencilTextureResource(device_, kClientWidth_, kClientHeight_);

	// DSV用のヒープでディスクリプタの数は1。Shaderには振れないのでfalse
	dsvDescriptorHeap_ = CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	// 先頭にDsvを作る
	device_->CreateDepthStencilView(depthStencilResource_, &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());

}

/*=============================================================================================================================
	画面の色を変える(コマンドを積み込む)
=============================================================================================================================*/
void DirectXCommon::BeginFrame(){
	HRESULT hr = S_FALSE;
	// コマンドを積み込んで確定させる ----------------------------
	// これから書き込む府バックバッファのインデックスを取得
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// 完璧な画面クリア 01_02 -----------------------
	// TransitionBarrierの設定
	// 今回のバリアはTransition
	barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier_.Transition.pResource = swapChainResources_[backBufferIndex];
	// 遷移前(現在)のResourceState
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	// 遷移後のResourveState
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// TransitionBArrierを張る
	commandList_->ResourceBarrier(1, &barrier_);
	// -------------------------------------------

	// 描画先のRTVを設定する
	//commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, nullptr);

	// DSVの設定
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);

	// 指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);

	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	ID3D12DescriptorHeap* heaps[] = { srvHeap_ };
	commandList_->SetDescriptorHeaps(1, heaps);

	// -----------------------------------------------------------------

}

/*=============================================================================================================================
	画面の色を変える(コマンドをキックする)
=============================================================================================================================*/
void DirectXCommon::EndFrame(){
	HRESULT hr = S_FALSE;

	// ------------------------------------------------------------------
	// 画面に各処理はすべて終わり、画面に移すので状態を遷移
	// 今回はRenderTargetからPresentにする
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// TransitionBariierを張る
	commandList_->ResourceBarrier(1, &barrier_);

	// ------------------------------------------------------------------
	// コマンドリストの内容を確定させる
	hr = commandList_->Close();
	assert(SUCCEEDED(hr));
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

/*=============================================================================================================================
	FenceとEventの生成
=============================================================================================================================*/
void DirectXCommon::CreateFence() {
	HRESULT hr = S_FALSE;
	fenceValue_ = 0;
	hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	// Fenceのsignalを待つためのイベントを作成する
	fenceEvent_ = CreateEvent(NULL, false, false, NULL);
	assert(fenceEvent_ != nullptr);
}

/*=============================================================================================================================
	 描画するコマンドを積む
=============================================================================================================================*/
void DirectXCommon::DrawCall() {
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);
	// RootSignatureを設定。PSOに設定しているけど別途設定が必要
	commandList_->SetGraphicsRootSignature(rootSigneture_);
	commandList_->SetPipelineState(graphicsPipelineState_);
	commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
	// 形状を設定。PSOに設定しているものとはまた別。
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 02_01 -------------------------
	// マテリアルCBufferの場所を設定
	commandList_->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
	// 02_02 -------------------------
	commandList_->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	// -------------------------------
	commandList_->SetGraphicsRootDescriptorTable(2, srvHandleGPU_);
	// 
	// 描画
	commandList_->DrawInstanced(6, 1, 0, 0);
}

void DirectXCommon::SpriteDraw() {
	commandList_->IASetVertexBuffers(0, 1, &vertexBufferViewSprite_);
	commandList_->SetGraphicsRootConstantBufferView(1, transformationMatrixResorce_->GetGPUVirtualAddress());
	commandList_->DrawInstanced(6, 1, 0, 0);
}

//=============================================================================================================================
//	PSOの生成
//=============================================================================================================================
void DirectXCommon::CreatePSO() {
	HRESULT hr = S_FALSE;

	// ---------------------------------------------------------------------------------
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//
	inputElementDescs[1].SemanticName = "TEXCORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);
	// ----------------------------------------------------------------------------------

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// ----------------------------------------------------------------------------------

	ShaderCompile();
	rootSigneture_ = CreateRootSignature();
	graphicsPipelineStateDesc_.pRootSignature = rootSigneture_;
	graphicsPipelineStateDesc_.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc_.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() };
	graphicsPipelineStateDesc_.PS = { pixelShaderBlob_->GetBufferPointer(), pixelShaderBlob_->GetBufferSize() };
	graphicsPipelineStateDesc_.BlendState = SetBlendState();
	graphicsPipelineStateDesc_.RasterizerState = SetRasterizerState();

	graphicsPipelineStateDesc_.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 書き込むRTVの情報
	graphicsPipelineStateDesc_.NumRenderTargets = 1;
	graphicsPipelineStateDesc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定
	graphicsPipelineStateDesc_.SampleDesc.Count = 1;
	graphicsPipelineStateDesc_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// 実際に生成
	graphicsPipelineState_ = nullptr;
	hr = device_->CreateGraphicsPipelineState(&graphicsPipelineStateDesc_, IID_PPV_ARGS(&graphicsPipelineState_));
	assert(SUCCEEDED(hr));
}

//=============================================================================================================================
//	VertexResourceの生成
//=============================================================================================================================
void DirectXCommon::CreateVertexResource(){
	vertexResource_ = CreateBufferResource(device_, sizeof(VertexData) * 6);

	// VertexBufferViewを作成する ------------------------------------------------------------------------
	// リソースの先頭のアドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点当たりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// Resourceにデータを書き込む -------------------------------------------------------------------------
	VertexData* vertexData = nullptr;
	// 書き込む溜めのアドレスを取得
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 1枚目 ------------------------------------------
	// 左下
	vertexData[0].pos = { -0.5f, -0.5f, 0.0f, 1.0f };
	vertexData[0].texcord = { 0.0f, 1.0f };
	// 上
	vertexData[1].pos = { 0.0f, 0.5f, 0.0f, 1.0f };
	vertexData[1].texcord = { 0.5f, 0.0f };
	// 右下
	vertexData[2].pos = { 0.5f, -0.5f, 0.0f, 1.0f };
	vertexData[2].texcord = { 1.0f, 1.0f };
	// 2枚目 ------------------------------------------
	// 左下
	vertexData[3].pos = { -0.5f, -0.5f, 0.5f, 1.0f };
	vertexData[3].texcord = { 0.0f, 1.0f };
	// 上
	vertexData[4].pos = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexData[4].texcord = { 0.5f, 0.0f };
	// 右下
	vertexData[5].pos = { 0.5f, -0.5f, -0.5f, 1.0f };
	vertexData[5].texcord = { 1.0f, 1.0f };

	// 02_01 -----------------------------------------------------------------------------------------
	materialResource_ = CreateBufferResource(device_, sizeof(Vector4));
	// マテリアルにデータを書き込む
	Vector4* materialData = nullptr;
	// 書き込むためのアドレスを取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 赤
	*materialData = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	// 02_02 -----------------------------------------------------------------------------------------
	wvpResource_ = CreateBufferResource(device_, sizeof(Matrix4x4));

	Matrix4x4* wvpData = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	transform_.rotate.y += 0.03f;
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scalel, transform_.rotate, transform_.translate);
	*wvpData = worldMatrix;

	// ViewportとScissor ------------------------------------------------------------------------------
	// ビューポート
	// クライアント領域のサイズと一緒にして画面全体を表示
	viewport_.Width = static_cast<float>(kClientWidth_);
	viewport_.Height = static_cast<float>(kClientHeight_);
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	// シザー矩形
	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect_.left = 0;
	scissorRect_.right = static_cast<LONG>(kClientWidth_);
	scissorRect_.top = 0;
	scissorRect_.bottom = static_cast<LONG>(kClientHeight_);
}

//=============================================================================================================================
//	VertexResourceSpriteの生成
//=============================================================================================================================
void DirectXCommon::CreateSprite() {
	// sprite用の頂点リソースを作る ---------------------------------------------------------------------
	vertexResourceSprite_ = CreateBufferResource(device_, sizeof(VertexData) * 6);

	// 頂点バッファビューを作る
	// 先頭のアドレスから
	vertexBufferViewSprite_.BufferLocation = vertexResourceSprite_->GetGPUVirtualAddress();
	// 使用するサイズは頂点6つ分
	vertexBufferViewSprite_.SizeInBytes = sizeof(VertexData) * 6;
	// 1頂点当たり
	vertexBufferViewSprite_.StrideInBytes = sizeof(VertexData);

	/// 頂点データを設定する ---------------------------------------------------------------------
	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	// 1枚目
	vertexDataSprite[0].pos = { 0.0f, 360.0f, 0.0f, 1.0f };
	vertexDataSprite[0].texcord = { 0.0f, 1.0f };
	vertexDataSprite[1].pos = { 0.0f, 0.0f, 0.0f, 1.0f, };
	vertexDataSprite[1].texcord = { 0.0f, 0.0f };
	vertexDataSprite[2].pos = { 640.0f, 360.0f, 0.0f, 1.0f };
	vertexDataSprite[2].texcord = { 1.0f, 1.0f };

	// 2枚目
	vertexDataSprite[3].pos = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexDataSprite[3].texcord = { 0.0f, 0.0f };
	vertexDataSprite[4].pos = { 640.0f, 0.0f, 0.0f, 1.0f, };
	vertexDataSprite[4].texcord = { 1.0f, 0.0f };
	vertexDataSprite[5].pos = { 640.0f, 360.0f, 0.0f, 1.0f };
	vertexDataSprite[5].texcord = { 1.0f, 1.0f };

	// Transform ---------------------------------------------------------------------
	transformationMatrixResorce_ = CreateBufferResource(device_, sizeof(Matrix4x4));
	// アドレスを取得
	transformationMatrixResorce_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
	// 単位行列を書き込んで置く
	*transformationMatrixData_ = MakeIdentity4x4();

}

// ↓初期化に関するメンバ関数 ---------------------------------------------------------------------------------------------------------------------------
//ID3D12Resource* DirectXCommon::CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
//	HRESULT hr = S_FALSE;
//	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
//	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
//	// 頂点リソースの設定
//	D3D12_RESOURCE_DESC vertexResourceDesc = {};
//	// バッファリソース。テクスチャの場合はまた別の設定をする
//	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//	vertexResourceDesc.Width = sizeInBytes * 3;
//	// バッファの場合がこれらは1にする決まり
//	vertexResourceDesc.Height = 1;
//	vertexResourceDesc.DepthOrArraySize = 1;
//	vertexResourceDesc.MipLevels = 1;
//	vertexResourceDesc.SampleDesc.Count = 1;
//	// バッファの場合はこれにする決まり
//	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//	// 実際に頂点リソースを作る
//	ID3D12Resource* vertexResource = nullptr;
//	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
//		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
//	assert(SUCCEEDED(hr));
//
//	return vertexResource;
//}

/// <summary>
/// コマンドを生成
/// </summary>
void DirectXCommon::CreateCommand() {
	HRESULT hr = S_FALSE;
	// GPUに命令を投げてくれる人　--------------------------
	// コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));

	// CommandAllocatorの生成 --------------------------------
	// コマンドアロケータを生成する
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する ----------------------------
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_, nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));
}

/// <summary>
/// スワップチェーンを生成する
/// </summary>
void DirectXCommon::SwapChainCreate() {
	HRESULT hr = S_FALSE;
	swapChain_ = nullptr;
	swapChainDesc_.Width = kClientWidth_;							// 画面の幅。ウィンドウのクライアント領域を同じ物にしておく
	swapChainDesc_.Height = kClientHeight_;							// 画面の高さ。ウィンドウのクライアント領域を同じ物にしておく
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// 色の形式
	swapChainDesc_.SampleDesc.Count = 1;								// マルチサンプルしない
	swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 描画のターゲットとして利用する
	swapChainDesc_.BufferCount = 2;									// ダブルバッファ
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// モニタに移したら、中身を破棄
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_, winApp_->GetHwnd(), &swapChainDesc_, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain_));
	assert(SUCCEEDED(hr));
}

/// <summary>
/// ディスクリプタヒープの生成
/// </summary>
void DirectXCommon::CreateRTVHeap() {
	HRESULT hr = S_FALSE;

	rtvDescriptorHeap_ = CreateDescriptorHeap(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	// swapChainからResorceを引っ張ってくる -----------------------------------
	hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
	// うまく取得出来なければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
	assert(SUCCEEDED(hr));
}

/// <summary>
/// レンダーターゲットビューの生成
/// </summary>
void DirectXCommon::CreateRTV(){
	// RTVの設定
	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	// まず1つめを作る。最初のところに作る。作る場所を指定して上げる必要がある
	rtvHandles_[0] = rtvStartHandle;
	device_->CreateRenderTargetView(swapChainResources_[0], &rtvDesc_, rtvHandles_[0]);
	// 2つめのディスクリプタハンドルを得る
	rtvHandles_[1].ptr = rtvHandles_[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// 2つめを作る
	device_->CreateRenderTargetView(swapChainResources_[1], &rtvDesc_, rtvHandles_[1]);
}

/// <summary>
/// ShaderとResourceをどのように関連付けるかをしるしたオブジェクト
/// </summary>
/// <returns></returns>
ID3D12RootSignature* DirectXCommon::CreateRootSignature(){
	HRESULT hr = S_FALSE;
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// textureを読むための
	D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 02_01 -------------------------------------
	// RootParameter作成。複数設定できるので配列。
	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	// PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;					// レジスタ番号0とバインド

	// 02_02 -------------------------------------
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	// 02_04 -------------------------------------
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	descriptionRootSignature.pParameters = rootParameters;				// ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);	// 配列の長さ

	// Samplerの設定 -------------------------------------------
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob_);
	if (FAILED(hr)) {
		Log(reinterpret_cast<char*>(errorBlob_->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	ID3D12RootSignature* result = nullptr;
	hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&result));
	assert(SUCCEEDED(hr));

	return result;
}

// 今は使っていない ----------------------------------------------
/// <summary>
/// VertexShaderへ渡す頂点データがどのような物かを指定するオブジェクト
/// </summary>
/// <returns></returns>
D3D12_INPUT_LAYOUT_DESC DirectXCommon::SetInputLayoutDesc() {
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	return inputLayoutDesc;
}

/// <summary>
/// PixelShaderからの出力を画面にどのように書き込むかを設定
/// </summary>
/// <returns></returns>
D3D12_BLEND_DESC DirectXCommon::SetBlendState() {
	D3D12_BLEND_DESC blendDesc{};
	// すべての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}

/// <summary>
/// 三角形の内部をピクセル分解して,PixelShaderを起動しこの処理の設定をを行う
/// </summary>
/// <returns></returns>
D3D12_RASTERIZER_DESC DirectXCommon::SetRasterizerState() {
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	return rasterizerDesc;
}

/// <summary>
/// 
/// </summary>
void DirectXCommon::SetDescTable() {
	// SRVを作成するDescriptorHeapの場所を求める
	srvHandleCPU_ = srvHeap_->GetCPUDescriptorHandleForHeapStart();
	srvHandleGPU_ = srvHeap_->GetGPUDescriptorHandleForHeapStart();
	// 先頭はImGuiが使っている溜めその次を使う
	srvHandleCPU_.ptr += device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvHandleGPU_.ptr += device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

/// <summary>
/// Shaderをコンパイルする
/// </summary>
void  DirectXCommon::ShaderCompile() {
	vertexShaderBlob_ = CompilerShader(L"Object3D.VS.hlsl", L"vs_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
	assert(vertexShaderBlob_ != nullptr);

	pixelShaderBlob_ = CompilerShader(L"Object3D.PS.hlsl", L"ps_6_0", dxcUtils_, dxcCompiler_, includeHandler_);
	assert(pixelShaderBlob_ != nullptr);
}

/// <summary>
/// 
/// </summary>
/// <param name="filePath"></param>
/// <param name="profile"></param>
/// <param name="dxcUtils"></param>
/// <param name="dxcCompiler"></param>
/// <param name="includeHandler"></param>
/// <returns></returns>
IDxcBlob* DirectXCommon::CompilerShader(
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler) {

	// 1.-----------------------------------------------------------------------------------------
	// これからシェーダーをコンパイルする旨えおログに出す
	Log(ConvertString(std::format(L"Begin compileShader, path:{}\n", filePath, profile)));
	// hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	// 読めなかったら止める
	assert(SUCCEEDED(hr));
	DxcBuffer shaderSourceBuffer{};
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	// 2.-----------------------------------------------------------------------------------------
	LPCWSTR arguments[] = {
		filePath.c_str(),			// コンパイル対象のhlslファイル
		L"-E", L"main",				// エントリーポイントの指定。基本的にmian以外にしない
		L"-T", profile,				// shaderProfileの設定
		L"-Zi", L"-Qembed_debug",	// デバック用に情報を埋め込む
		L"-Od",						// 最適化を外して置く
		L"-Zpr",					// メモリレイアウトは行優先
	};
	// 実際にshaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,		// 読み込んだファイル
		arguments,					// コンパイルオプション
		_countof(arguments),		// コンパイルオプションの数
		includeHandler,				// includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)	// コンパイル結果
	);
	// コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	// 3.-----------------------------------------------------------------------------------------
	// 警告,エラーが出たらログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	// 4.-----------------------------------------------------------------------------------------
	// コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	// 成功したらログを出す
	Log(ConvertString(std::format(L"Compile Succeeded, path:{}\n", filePath, profile)));
	// もう使わないリソースを解放
	shaderSource->Release();
	shaderResult->Release();

	return shaderBlob;
}

/// <summary>
/// 移動用のの頂点の生成
/// </summary>
void DirectXCommon::CreateWVPResource(const Matrix4x4& vpMatrix){
	Matrix4x4* wvpData = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	transform_.rotate.y += 0.01f;
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scalel, transform_.rotate, transform_.translate);
	Matrix4x4 wvpMatrix = Multiply(worldMatrix, vpMatrix);
	*wvpData = wvpMatrix;
}

/// <summary>
/// 移動用のの頂点の生成
/// </summary>
void DirectXCommon::CreateaWVPSpriteRespirce(){
	ImGui::Begin("sprite");
	ImGui::SliderFloat3("translate", &transformSprite_.translate.x, 0, 1280);
	ImGui::End();
	Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite_.scalel, transformSprite_.rotate, transformSprite_.translate);
	Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
	Matrix4x4 projectMatrixSprite = MakeOrthograhicMatrix(0.0f, 0.0f, float(kClientWidth_), float(kClientHeight_), 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectMatrixSprite));
	*transformationMatrixData_ = worldViewProjectionMatrix;
}

// ======================================================================================================================================================
void DirectXCommon::CreateTexture() {
	mipImage_ = LoadTextrue("Resource/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImage_.GetMetadata();
	textureResource_ = CreateTextureResource(device_, metadata);
	/*UploadTextureData(textureResource_, mipImage_);*/

	// ------------------------------------------------------------
	// metadataを元にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// ------------------------------------------------------------
	// SRVを作成するDescriptorHeapの場所を求める
	srvHandleCPU_ = srvHeap_->GetCPUDescriptorHandleForHeapStart();
	srvHandleGPU_ = srvHeap_->GetGPUDescriptorHandleForHeapStart();
	// 先頭はImGuiが使っている溜めその次を使う
	srvHandleCPU_.ptr += device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvHandleGPU_.ptr += device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 生成
	device_->CreateShaderResourceView(textureResource_, &srvDesc, srvHandleCPU_);
}

DirectX::ScratchImage DirectXCommon::LoadTextrue(const std::string& filePath){
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertWString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	return mipImages;
}

ID3D12Resource* DirectXCommon::CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata){
	// metadataを元にResourceの設定
	D3D12_RESOURCE_DESC desc{};
	desc.Width = UINT(metadata.width);								// Textureの幅
	desc.Height = UINT(metadata.height);							// Textureの高さ
	desc.MipLevels = UINT16(metadata.mipLevels);					// mipmapの数
	desc.DepthOrArraySize = UINT16(metadata.arraySize);				// 奥行き　or 配列Textureの配数
	desc.Format = metadata.format;									// TextureのFormat
	desc.SampleDesc.Count = 1;										// サンプリングカウント
	desc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);	// Textureの次元数

	// 利用するheapの設定。非常に特殊な運用。exで一般的
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// resourceを生成する
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,					// Heapの設定
		D3D12_HEAP_FLAG_NONE,				// Heapの特殊の設定
		&desc,								// Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,	// 初回のResourceState Textureは木基本読むだけ
		nullptr,							// clear最適地。使わない
		IID_PPV_ARGS(&resource)				// 作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));

	return resource;
}

[[nodiscard]]
ID3D12Resource* DirectXCommon::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList){
	
	std::vector<D3D12_SUBRESOURCE_DATA> subresource;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresource);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresource.size()));
	ID3D12Resource* intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource, 0, 0, UINT(subresource.size()), subresource.data());

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	return intermediateResource;
}
// ============================================================================================

void DirectXCommon::Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}