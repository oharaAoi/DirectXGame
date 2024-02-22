#include "Object3d.hlsli"

struct TransformationMatrix{
    float4x4 WVP;
};

ConstantBuffer<TransformationMatrix> gTransfomationMatrix : register(b0);
struct VertexShaderInput{
	float4 position : POSITION0;
    float2 texcord : TEXCORD0;
  
};

VertexShaderOutput main(VertexShaderInput input){
	VertexShaderOutput output;
    output.position = mul(input.position, gTransfomationMatrix.WVP);
    output.texcord = input.texcord;
	return output;
}