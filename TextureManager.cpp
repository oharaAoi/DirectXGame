#include "TextureManager.h"

TextureManager* TextureManager::GetInstacne(){
	static TextureManager instance;
	return &instance;
}

//=============================================================================================================================
//	内容
//=============================================================================================================================
void TextureManager::Initialize(DirectXCommon* dxCommon){
	assert(dxCommon);
	dxCommon_ = dxCommon; 

	mipImage_ = LoadTextrue("Resource/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImage_.GetMetadata();
	textureResource_ = CreateTextureResource(dxCommon->GetDevice(), metadata);
	UploadTextureData(textureResource_, mipImage_);

	// ------------------------------------------------------------
	// metadataを元にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// ------------------------------------------------------------
	dxCommon->SetDescTable();
	// 生成
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource_, &srvDesc, dxCommon->GetSrvHandleCPU());
}

void TextureManager::Finalize(){
	textureResource_->Release();
}

//=============================================================================================================================
//	関数
//=============================================================================================================================
DirectX::ScratchImage TextureManager::LoadTextrue(const std::string& filePath){
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

ID3D12Resource* TextureManager::CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata){
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
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	// resourceを生成する
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,					// Heapの設定
		D3D12_HEAP_FLAG_NONE,				// Heapの特殊の設定
		&desc,								// Resourceの設定
		D3D12_RESOURCE_STATE_GENERIC_READ,	// 初回のResourceState Textureは木基本読むだけ
		nullptr,							// clear最適地。使わない
		IID_PPV_ARGS(&resource)				// 作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));
	
	return resource;
}

void TextureManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages){
	// meta情報を取得
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	// 全MipMapについて
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		// MioMapLevelを指定して各Imageを取得
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
		// Textureに転送
		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel),
			nullptr,				// 全領域へのコピー
			img->pixels,			// 元データアドレス
			UINT(img->rowPitch),	// 1ラインサイズ
			UINT(img->slicePitch)	// 1枚サイズ
		);
		assert(SUCCEEDED(hr));
	}
}
