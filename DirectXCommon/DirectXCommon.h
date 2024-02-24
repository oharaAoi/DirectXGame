#pragma once
// DirectX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>
#include <DirectXTex.h>
#include <d3dx12.h>

// dxc
#include <dxcapi.h>
#include <vector>

#include "Function/Convert.h"
#include "Function/DirectXUtils.h"
#include "Window/WinApp.h"

// lib
#include "VertexData.h"
#include "MyMatrix.h"
#include "Transform.h"

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

	UINT GetBufferCount() const {return bufferCount_;}

	ID3D12Device* GetDevice() const { return device_; }

	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_; }

	ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap_; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandleCPU() const { return srvHandleCPU_; }
 
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(WinApp* win, int32_t backBufferWidth, int32_t backBufferHeight);

	void Finalize();


private: // メンバ変数

	// ウィンドウズアプリケーション管理
	WinApp* winApp_;
	int32_t kClientWidth_;
	int32_t kClientHeight_;

	ID3D12Debug1* debugController_ = nullptr;
	IDXGIFactory7* dxgiFactory_ = nullptr;
	IDXGIAdapter4* useAdapter_ = nullptr;
	ID3D12Device* device_ = nullptr;
	ID3D12CommandQueue* commandQueue_ = nullptr;
	ID3D12CommandAllocator* commandAllocator_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	IDXGISwapChain4* swapChain_ = nullptr;
	ID3D12DescriptorHeap* rtvDescriptorHeap_ = nullptr;
	ID3D12Resource* swapChainResources_[2] = { nullptr };
	ID3D12Fence* fence_ = nullptr;
	IDxcUtils* dxcUtils_ = nullptr;
	IDxcCompiler3* dxcCompiler_ = nullptr;
	IDxcIncludeHandler* includeHandler_ = nullptr;
	IDxcBlob* vertexShaderBlob_ = nullptr;
	IDxcBlob* pixelShaderBlob_ = nullptr;
	ID3DBlob* errorBlob_ = nullptr;
	ID3D12RootSignature* rootSigneture_ = nullptr;
	ID3D12PipelineState* graphicsPipelineState_ = nullptr;
	ID3D12Resource* vertexResource_ = nullptr;
	ID3D12Resource* materialResource_ = nullptr;
	ID3D12Resource* wvpResource_;

	ID3D12DescriptorHeap* srvHeap_ = nullptr;

	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc_;
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

	//
	UINT bufferCount_;

	//
	kTransform transform_;

	// 
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_;

	DirectX::ScratchImage mipImage_;
	ID3D12Resource* textureResource_ = nullptr;
	ID3D12Resource* intermediateResource_ = nullptr;

	// 深度
	ID3D12Resource* depthStencilResource_ = nullptr;

	ID3D12DescriptorHeap* dsvDescriptorHeap_ = nullptr;
	
public: // メンバ関数
	DirectXCommon() = default;
	~DirectXCommon() = default;
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
	void CreateRTVHeap();

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

	void SetDescTable();

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

	/// <summary>
	/// 
	/// </summary>
	void CreateWVPResource(const Matrix4x4& vpMatrix);

	void CreateTexture();

public: // textureにかかわるもの

	/// <summary>
	/// Textrueデータを読む
	/// </summary>
	/// <returns></returns>
	DirectX::ScratchImage LoadTextrue(const std::string& filePath);

	/// <summary>
	/// Textureを読んで使えるようにする流れ
	/// </summary>
	/// <param name="device"></param>
	/// <param name="metaData"></param>
	/// <returns></returns>
	ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metaData);

	/// <summary>
	/// TextureResourceにデータを転送する
	/// </summary>
	/// <param name="texture"></param>
	/// <param name="mipImages"></param>
	[[nodiscard]]
	ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);


	void Log(const std::string& message);
};

