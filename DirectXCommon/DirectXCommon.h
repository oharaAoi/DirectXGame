#pragma once
// DirectX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>

// dxc
#include <dxcapi.h>
#include <vector>

#include "Function/Convert.h"
#include "Window/WinApp.h"
#include "Vector4.h"

/// <summary>
/// DirectX汎用
/// </summary>
class DirectXCommon{
public: // メンバ関数

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static DirectXCommon* GetInstacne();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* win, int32_t backBufferWidth, int32_t backBufferHeight);


private: // メンバ変数

	// ウィンドウズアプリケーション管理
	WinApp* winApp_;
	int32_t kClientWidth_;
	int32_t kClientHeight_;

	ID3D12Debug1* debugController_ = nullptr;
	ID3D12Device* device_ = nullptr;
	IDXGIFactory7* dxgiFactory_ = nullptr;
	IDXGIAdapter4* useAdapter_ = nullptr;
	IDXGISwapChain4* swapChain_ = nullptr;
	ID3D12CommandQueue* commandQueue_ = nullptr;
	ID3D12CommandAllocator* commandAllocator_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	ID3D12Resource* swapChainResources_[2] = { nullptr };
	ID3D12DescriptorHeap* rtcDescriptorHeap_ = nullptr;
	ID3D12Fence* fence_ = nullptr;
	IDxcUtils* dxcUtils_ = nullptr;
	IDxcCompiler3* dxcCompiler_ = nullptr;
	ID3D12RootSignature* rootSigneture_ = nullptr;
	IDxcIncludeHandler* includeHandler_ = nullptr;
	ID3D12PipelineState* graphicsPipelineState_ = nullptr;
	IDxcBlob* vertexShaderBlob_ = nullptr;
	IDxcBlob* pixelShaderBlob_ = nullptr;
	ID3DBlob* errorBlob_ = nullptr;
	ID3D12Resource* vertexResource_ = nullptr;
	ID3D12Resource* materialResource_ = nullptr;

	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];
	D3D12_RESOURCE_BARRIER barrier_;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_;
	D3D12_BLEND_DESC blendDesc_;
	D3D12_RASTERIZER_DESC rasterizerDesc_;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};
	
public: // メンバ関数
	DirectXCommon() = default;
	~DirectXCommon();
	DirectXCommon(const DirectXCommon&) = delete;
	const DirectXCommon& operator=(const DirectXCommon&) = delete;

	/// <summary>
	/// DXGTデバイス初期化
	/// </summary>
	void InitializeDXGDevice();

	/// <summary>
	/// 画面を青くするための初期化
	/// </summary>
	void InitializeScreen();

	/// <summary>
	/// DXCの初期化
	/// </summary>
	void InitializeDXC();

	void BeginFrame();
	void EndFrame();

	/// <summary>
	/// 描画するコマンドを積む
	/// </summary>
	void DrawCall();

public: // メンバ関数(関数内の細かい関数)

	/// <summary>
	/// CPUとGPUの同期を取る
	/// </summary>
	void CreateFence();

	/// <summary>
	/// コマンドを生成する
	/// </summary>
	void CreateCommand();

	/// <summary>
	/// スワップチェーンを生成する
	/// </summary>
	void SwapChainCreate();

	/// <summary>
	/// ディスクリプタヒープの生成
	/// </summary>
	void CreateDescriptorHeap();

	/// <summary>
	/// レンダーターゲットビューの生成
	/// </summary>
	void CreateRTV();

	/// <summary>
	/// PSOの生成
	/// </summary>
	void CreatePSO();

	/// <summary>
	/// 頂点データの生成
	/// </summary>
	void CreateVertexResource();

	/// <summary>
	/// BufferResourceを作る関数
	/// </summary>
	ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

	/// <summary>
	/// RootSignatureの生成
	/// </summary>
	ID3D12RootSignature* CreateRootSignature();

	/// <summary>
	/// InoutLayoutの設定
	/// </summary>
	D3D12_INPUT_LAYOUT_DESC SetInputLayoutDesc();

	/// <summary>
	/// BlendStateの設定
	/// </summary>
	D3D12_BLEND_DESC SetBlendState();

	/// <summary>
	/// RasterizerStateの設定
	/// </summary>
	D3D12_RASTERIZER_DESC SetRasterizerState();

	/// <summary>
	/// CompileShader
	/// </summary>
	/// <param name=""></param>
	IDxcBlob* CompilerShader(
		const std::wstring& filePath,
		const wchar_t* profile,
		IDxcUtils* dxcUtils,
		IDxcCompiler3* dxcCompiler,
		IDxcIncludeHandler* includeHandler
	);

	/// <summary>
	/// Shaderをコンパイルする
	/// </summary>
	void ShaderCompile();

	void Log(const std::string& message);
};

