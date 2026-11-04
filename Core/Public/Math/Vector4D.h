#ifndef __MATH_VECTOR_4D__
#define __MATH_VECTOR_4D__

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"

#pragma warning(push)
#pragma warning(disable: 4201)

typedef struct FVector4D
{
	union
	{
#if defined(CORE_USE_SIMD)
		/** Used for SIMD operations. */
		alignas(16) __m128 Data;
#endif
		/** An array of x, y, z, w */
		alignas(16) float32 Elements[4];

		struct
		{
			union
			{
				/** The first element. */
				float32 X, R, S;
			};
			union
			{
				/** The second element. */
				float32 Y, G, T;
			};
			union
			{
				/** The third element. */
				float32 Z, B, P;
			};
			union
			{
				/** The fourth element. */
				float32 W, A, Q;
			};
		};
	};
} FVector4D;

#pragma warning(pop)

/**
 * @brief Creates and returns a new 3-element vector using the supplied values.
 *
 * @param X The x value.
 * @param Y The y value.
 * @param Z The z value.
 * @param W The W value.
 * @return A new 3-element vector.
 */
NODISCARD FORCEINLINE FVector4D CreateVector4D(float32 X, float32 Y, float32 Z, float32 W)
{
#if defined(CORE_USE_SIMD)
	FVector4D OutVector = { 0 };
	OutVector.Data = _mm_setr_ps(X, Y, Z, W);
	return OutVector;
#else
	FVector4D OutVector = { 0 };
	OutVector.X = X;
	OutVector.Y = Y;
	OutVector.Z = Z;
	OutVector.W = W;
	return OutVector;
#endif
}

/**
 * @brief Create and returns 4-component vector with all components set to 0.0f.
 */
NODISCARD FORCEINLINE FVector4D CreateZeroVector4D()
{
	FVector4D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = 0.0f;
	OutVector.Z = 0.0f;
	OutVector.W = 0.0f;
	return OutVector;
}

/**
 * @brief Create and returns 4-component vector with all components set to 1.0f.
 */
NODISCARD FORCEINLINE FVector4D CreateOneVector4D()
{
	FVector4D OutVector = { 0 };
	OutVector.X = 1.0f;
	OutVector.Y = 1.0f;
	OutVector.Z = 1.0f;
	OutVector.W = 1.0f;
	return OutVector;
}

/**
 * @brief Adds VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector4D AddVector4D(FVector4D VectorA, FVector4D VectorB)
{
	FVector4D OutVector = { 0 };
	OutVector.X = VectorA.X + VectorB.X;
	OutVector.Y = VectorA.Y + VectorB.Y;
	OutVector.Z = VectorA.Z + VectorB.Z;
	OutVector.W = VectorA.W + VectorB.W;
	return OutVector;
}

/**
 * @brief Subtracts VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector4D SubVector4D(FVector4D VectorA, FVector4D VectorB)
{
	FVector4D OutVector = { 0 };
	OutVector.X = VectorA.X - VectorB.X;
	OutVector.Y = VectorA.Y - VectorB.Y;
	OutVector.Z = VectorA.Z - VectorB.Z;
	OutVector.W = VectorA.W - VectorB.W;
	return OutVector;
}

/**
 * @brief Multiplies VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector4D MulVector4D(FVector4D VectorA, FVector4D VectorB)
{
	FVector4D OutVector = { 0 };
	OutVector.X = VectorA.X * VectorB.X;
	OutVector.Y = VectorA.Y * VectorB.Y;
	OutVector.Z = VectorA.Z * VectorB.Z;
	OutVector.W = VectorA.W * VectorB.W;
	return OutVector;
}

/**
 * @brief Divides VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector4D DivVector4D(FVector4D VectorA, FVector4D VectorB)
{
	CORE_ASSERT((VectorB.X == 0) || (VectorB.Y == 0) || (VectorB.Z == 0) || (VectorB.W == 0));
	FVector4D OutVector = { 0 };
	OutVector.X = VectorA.X / VectorB.X;
	OutVector.Y = VectorA.Y / VectorB.Y;
	OutVector.Z = VectorA.Z / VectorB.Z;
	OutVector.W = VectorA.W / VectorB.W;
	return OutVector;
}

/**
 * @brief Returns the squared length of the provided vector.
 *
 * @param Vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
NODISCARD FORCEINLINE float32 LengthSquaredVector4D(FVector4D Vector)
{
	return (Vector.X * Vector.X) + (Vector.Y * Vector.Y) + (Vector.Z * Vector.Z) + (Vector.W * Vector.W);
}

/**
 * @brief Returns the length of the provided vector.
 *
 * @param Vector The vector to retrieve the length of.
 * @return The length.
 */
NODISCARD FORCEINLINE float32 LengthVector4D(FVector4D Vector)
{
	return CoreSqrt(LengthSquaredVector4D(Vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 *
 * @param Vector A pointer to the vector to be normalized.
 */
NODISCARD FORCEINLINE void NormalizeVector4DPtr(FVector4D* const Vector)
{
	CORE_ASSERT(Vector != NULL);
	const float32 Length = LengthVector4D(*Vector);

	CORE_ASSERT(Length != 0);
	Vector->X /= Length;
	Vector->Y /= Length;
	Vector->Z /= Length;
	Vector->W /= Length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 *
 * @param Vector The vector to be normalized.
 * @return A normalized copy of the supplied vector
 */
NODISCARD FORCEINLINE FVector4D NormalizeVector4D(FVector4D Vector)
{
	NormalizeVector4DPtr(&Vector);
	return Vector;
}

NODISCARD FORCEINLINE float32 DotVector4DByF32(
	float32 a0, float32 a1, float32 a2, float32 a3,
	float32 b0, float32 b1, float32 b2, float32 b3,
	float32 p
) {
	p =
		a0 * b0 +
		a1 * b1 +
		a2 * b2 +
		a3 * b3;
	return p;
}

#endif 
