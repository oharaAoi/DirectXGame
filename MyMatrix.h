#pragma once
#include <Matrix4x4.h>
#include <Vector3.h>
#include <assert.h>
#include <cmath>

/// <summary>
/// 加算
/// </summary>
/// <param name="m1"></param>
/// <param name="m2"></param>
/// <returns></returns>
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

/// <summary>
/// 減算
/// </summary>
/// <param name="m1"></param>
/// <param name="m2"></param>
/// <returns></returns>
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

/// <summary>
/// 積
/// </summary>
/// <param name="m1"></param>
/// <param name="m2"></param>
/// <returns></returns>
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

/// <summary>
/// 逆行列
/// </summary>
/// <param name="m"></param>
/// <returns></returns>
Matrix4x4 Inverse(Matrix4x4 matrix);

void swapRows(Matrix4x4& matrix, int row1, int row2);
void scaleRow(Matrix4x4& matrix, int row, float scalar);
void addScaledRow(Matrix4x4& matrix, int targetRow, int sourceRow, float scalar);

/// <summary>
/// 転置行列
/// </summary>
/// <param name="m"></param>
/// <returns></returns>
Matrix4x4 Transpose(const Matrix4x4 matrix);

/// <summary>
/// 単位行列
/// </summary>
/// <param name="matrix"></param>
/// <returns></returns>
Matrix4x4 MakeIdentity4x4();

/// <summary>
/// 平行移動行列
/// </summary>
/// <param name="translate"></param>
/// <returns></returns>
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

/// <summary>
/// 拡縮行列
/// </summary>
/// <param name="scale"></param>
/// <returns></returns>
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

/// <summary>
/// 回転行列
/// </summary>
/// <param name="radian"></param>
/// <returns></returns>
Matrix4x4 MakeRotateXMatrix(float radian);
Matrix4x4 MakeRotateYMatrix(float radian);
Matrix4x4 MakeRotateZMatrix(float radian);
Matrix4x4 MakeRotateXYZMatrix(Vector3 radian);

/// <summary>
/// 三次元アフィン変換行列
/// </summary>
/// <param name="scale"></param>
/// <param name="rotate"></param>
/// <param name="translate"></param>
/// <returns></returns>
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

/// <summary>
/// 座標変換
/// </summary>
/// <param name="vector"></param>
/// <param name="matrix"></param>
/// <returns></returns>
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

/// <summary>
/// 正射影行列
/// </summary>
/// <param name="left"></param>
/// <param name="top"></param>
/// <param name="right"></param>
/// <param name="bottom"></param>
/// <param name="nearClip"></param>
/// <param name="farClip"></param>
/// <returns></returns>
Matrix4x4 MakeOrthograhicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

/// <summary>
/// 透視投影行列
/// </summary>
/// <param name="fovY"></param>
/// <param name="aspectRatio"></param>
/// <param name="nearClip"></param>
/// <param name="farClip"></param>
/// <returns></returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

/// <summary>
/// ビューポート変換行列
/// </summary>
/// <param name="left"></param>
/// <param name="top"></param>
/// <param name="width"></param>
/// <param name="height"></param>
/// <param name="minDepth"></param>
/// <param name="maxDepth"></param>
/// <returns></returns>
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
