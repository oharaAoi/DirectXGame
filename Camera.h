#pragma once
#include "MyMatrix.h"
#include "Transform.h"

class Camera{
private:

	kTransform cameraTransform_;

	Matrix4x4 cameraMatrix_;
	Matrix4x4 prijectionMatrix_;
	Matrix4x4 viewMatrix_;
	Matrix4x4 vpMatrix_;

public:

	Camera();
	~Camera();

	void Init();
	void Update();
	void Draw();

	/// accsser
	Matrix4x4 GetVpMatrix() const { return vpMatrix_; }
};

