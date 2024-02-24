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
/// DiscriptorHeapの作成
/// </summary>
/// <param name="device"></param>
/// <param name="heapType"></param>
/// <param name="numDescriptor"></param>
/// <param name="shaderVisible"></param>
/// <returns></returns>
ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptor, bool shaderVisible);

/// <summary>
/// 
/// </summary>
/// <param name="device"></param>
/// <param name="sizeInBytes"></param>
/// <returns></returns>
ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

/// <summary>
/// ログを出す
/// </summary>
/// <param name="message"></param>
void Log(const std::string& message);