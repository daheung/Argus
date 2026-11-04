#ifndef __MATH_VECTOR__
#define __MATH_VECTOR__

#include "Core/Public/Math/Vector2D.h"
#include "Core/Public/Math/Vector3D.h"
#include "Core/Public/Math/Vector4D.h"
#include "Core/Public/Math/Matrix.h"

/**
 * @brief Returns a new FVector3D containing the x, y, and z components of the
 * supplied FVector4, essentially dropping the w component.
 *
 * @param Vector The 4-component vector to extract from.
 * @return A new FVector3D
 */
NODISCARD FORCEINLINE FVector3D Vector4DTo3D(FVector4D Vector)
{
	FVector3D OutVector = { 0 };
	OutVector.X = Vector.X;
	OutVector.Y = Vector.Y;
	OutVector.Z = Vector.Z;
	return OutVector;
}

/**
 * @brief Returns a new FVector4D using as the x, y and z components and w for W param.
 *
 * @param Vector The 3-component vector.
 * @param W The w component.
 * @return A new FVector4D
 */
NODISCARD FORCEINLINE FVector4D Vector3DTo4D(FVector3D Vector, float32 W)
{
#if defined(LUSE_SIMD)
	FVector4D OutVector = { 0 };
	OutVector.Data = _mm_setr_ps(Vector.X, Vector.Y, Vector.Z, W);
	return OutVector;
#else
	FVector4D OutVector = { 0 };
	OutVector.X = Vector.X;
	OutVector.Y = Vector.Y;
	OutVector.Z = Vector.Z;
	OutVector.W = W;
	return OutVector;
#endif
}

#endif