#pragma once
#include <d3d12.h>
#include <cassert>
#include <DirectXTex.h>

#include "Function/Convert.h"
#include "DirectXCommon/DirectXCommon.h"

class TextureManager{
public: // メンバ関数

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns></returns>
	static TextureManager* GetInstacne();

public:

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(const TextureManager&) = delete;
	const TextureManager& operator=(const TextureManager&) = delete;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="device"></param>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

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
	void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

private:
	DirectX::ScratchImage mipImage_;
	ID3D12Resource* textureResource_;
	
	//D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_;
	//D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_;
};

