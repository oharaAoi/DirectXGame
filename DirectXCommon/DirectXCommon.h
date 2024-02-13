#pragma once
// DirectX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>

#include "Function/Convert.h"

#include "Window/WinApp.h"

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
	void Initialize(WinApp* win);



private: // メンバ変数

	// ウィンドウズアプリケーション管理
	WinApp* winApp_;

	ID3D12Debug1* debugController_ = nullptr;
	ID3D12Device* device_ = nullptr;
	// DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory_ = nullptr;
	IDXGIAdapter4* useAdapter_ = nullptr;

	IDXGISwapChain4* swapChain_ = nullptr;

	ID3D12CommandQueue* commandQueue_ = nullptr;
	ID3D12CommandAllocator* commandAllocator_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
	ID3D12Resource* swapChainResources_[2] = { nullptr };
	ID3D12DescriptorHeap* rtcDescriptorHeap_ = nullptr;
	ID3D12Fence* fence_ = nullptr;

	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_;
	// RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];

	
public: // メンバ関数
	DirectXCommon() = default;
	~DirectXCommon();
	DirectXCommon(const DirectXCommon&) = delete;
	const DirectXCommon& operator=(const DirectXCommon&) = delete;

	/// <summary>
	/// DXGTデバイス初期化
	/// </summary>
	void InitializeDXGDevice();

	void InitializeScreen();

	void BeginFrame();
	void EndFrame();

	void Log(const std::string& message);
};

