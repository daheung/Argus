#ifndef __MATH_VECTOR_3D__
#define __MATH_VECTOR_3D__

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/Assertion.h"

#pragma warning(push)
#pragma warning(disable: 4201)

typedef struct FVector3D
{
	union
	{
		/** An array of x, y, z */
		float32 Elements[3];

		struct
		{
			union
			{
				/** The first element. */
				float32 X, R, S, U;
			};
			union
			{
				/** The second element. */
				float32 Y, G, T, V;
			};
			union
			{
				/** The third element. */
				float32 Z, B, P, W;
			};
		};
	};
} FVector3D;

#pragma warning(pop)

/**
 * @brief Creates and returns a new 3-element vector using the supplied values.
 *
 * @param X The x value.
 * @param Y The y value.
 * @param Z The z value.
 * @return A new 3-element vector.
 */
NODISCARD FORCEINLINE FVector3D CreateVector3D(float32 X, float32 Y, float32 Z)
{
	FVector3D OutVector = { 0 };
	OutVector.X = X;
	OutVector.Y = Y;
	OutVector.Z = Z;
	return OutVector;
}

/**
 * @brief Create and returns 3-component vector with all components set to 0.0f.
 */
NODISCARD FORCEINLINE FVector3D CreateZeroVector3D()
{
	FVector3D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = 0.0f;
	OutVector.Z = 0.0f;
	return OutVector;
}

/**
 * @brief Create and returns 3-component vector with all components set to 1.0f.
 */
NODISCARD FORCEINLINE FVector3D CreateOneVector3D()
{
	FVector3D OutVector = { 0 };
	OutVector.X = 1.0f;
	OutVector.Y = 1.0f;
	OutVector.Z = 1.0f;
	return OutVector;
}

/**
 * @brief Create and returns 3-component vector pointing up (0, 1, 0).
 */
NODISCARD FORCEINLINE FVector3D CreateUpVector3D()
{
	FVector3D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = 1.0f;
	OutVector.Z = 0.0f;
	return OutVector;
}

/**
 * @brief Create and returns 3-component vector pointing down (0, -1, 0).
 */
NODISCARD FORCEINLINE FVector3D CreateDownVector3D()
{
	FVector3D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = -1.0f;
	OutVector.Z = 0.0f;
	return OutVector;
}

/**
 * @brief Create and returns 3-component vector pointing left (-1, 0, 0).
 */
NODISCARD FORCEINLINE FVector3D CreateLeftVector3D()
{
	FVector3D OutVector = { 0 };
	OutVector.X = -1.0f;
	OutVector.Y = 0.0f;
	OutVector.Z = 0.0f;
	return OutVector;
}

/**
 * @brief Create and returns 3-component vector pointing right (1, 0, 0).
 */
NODISCARD FORCEINLINE FVector3D CreateRightVector3D()
{
	FVector3D OutVector = { 0 };
	OutVector.X = 1.0f;
	OutVector.Y = 0.0f;
	OutVector.Z = 0.0f;
	return OutVector;
}

/**
 * @brief Create and returns 3-component vector pointing forward (0, 0, -1).
 */
NODISCARD FORCEINLINE FVector3D CreateForwardVector3D()
{
	FVector3D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = 0.0f;
	OutVector.Z = -1.0f;
	return OutVector;
}

/**
 * @brief Create and returns 3-component vector pointing back (0, 0, 1).
 */
NODISCARD FORCEINLINE FVector3D CreateBackVector3D()
{
	FVector3D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = 0.0f;
	OutVector.Z = 1.0f;
	return OutVector;
}

/**
 * @brief Adds VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector3D AddVector3D(FVector3D VectorA, FVector3D VectorB)
{
	FVector3D OutVector = { 0 };
	OutVector.X = VectorA.X + VectorB.X;
	OutVector.Y = VectorA.Y + VectorB.Y;
	OutVector.Z = VectorA.Z + VectorB.Z;
	return OutVector;
}

/**
 * @brief Subtracts VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector3D SubVector3D(FVector3D VectorA, FVector3D VectorB)
{
	FVector3D OutVector = { 0 };
	OutVector.X = VectorA.X - VectorB.X;
	OutVector.Y = VectorA.Y - VectorB.Y;
	OutVector.Z = VectorA.Z - VectorB.Z;
	return OutVector;
}

/**
 * @brief Multiplies VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector3D MulVector3D(FVector3D VectorA, FVector3D VectorB)
{
	FVector3D OutVector = { 0 };
	OutVector.X = VectorA.X * VectorB.X;
	OutVector.Y = VectorA.Y * VectorB.Y;
	OutVector.Z = VectorA.Z * VectorB.Z;
	return OutVector;
}

/**
 * @brief Divides VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector3D DivVector3D(FVector3D VectorA, FVector3D VectorB)
{
	CORE_ASSERT((VectorB.X == 0) || (VectorB.Y == 0) || (VectorB.Z == 0));
	FVector3D OutVector = { 0 };
	OutVector.X = VectorA.X / VectorB.X;
	OutVector.Y = VectorA.Y / VectorB.Y;
	OutVector.Z = VectorA.Z / VectorB.Z;
	return OutVector;
}

/**
 * @brief Multiplies all elements of vectorA by scalar and returns a copy of the value.
 *
 * @param VectorA The vector to be multiplied.
 * @param Scalar The scalar value.
 * @return A copy of the resulting vector.
 */
NODISCARD FORCEINLINE FVector3D MulScalarVector3D(FVector3D VectorA, float32 Scalar)
{
	FVector3D OutVector = { 0 };
	OutVector.X = VectorA.X * Scalar;
	OutVector.Y = VectorA.Y * Scalar;
	OutVector.Z = VectorA.Z * Scalar;
	return OutVector;
}

/**
 * @brief Returns the squared length of the provided vector.
 *
 * @param Vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
NODISCARD FORCEINLINE float32 LengthSquaredVector3D(FVector3D Vector)
{
	return (Vector.X * Vector.X) + (Vector.Y * Vector.Y) + (Vector.Z * Vector.Z);
}

/**
 * @brief Returns the length of the provided vector.
 *
 * @param Vector The vector to retrieve the length of.
 * @return The length.
 */
NODISCARD FORCEINLINE float32 LengthVector3D(FVector3D Vector)
{
	return CoreSqrt(LengthSquaredVector3D(Vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 *
 * @param Vector A pointer to the vector to be normalized.
 */
NODISCARD FORCEINLINE void NormalizeVector3DPtr(FVector3D* const Vector)
{
	CORE_ASSERT(Vector != NULL);
	const float32 Length = LengthVector3D(*Vector);

	//CORE_ASSERT(Length != 0);
	Vector->X /= Length;
	Vector->Y /= Length;
	Vector->Z /= Length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 *
 * @param Vector The vector to be normalized.
 * @return A normalized copy of the supplied vector
 */
NODISCARD FORCEINLINE FVector3D NormalizeVector3D(FVector3D Vector)
{
	NormalizeVector3DPtr(&Vector);
	return Vector;
}

/**
 * @brief Returns the dot product between the provided vectors. Typically used
 * to calculate the difference id direction.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The doc product
 */
NODISCARD FORCEINLINE float32 DotVector3D(FVector3D VectorA, FVector3D VectorB)
{
	float32 OutFloat = 0.0f;
	OutFloat += VectorA.X * VectorB.X;
	OutFloat += VectorA.Y * VectorB.Y;
	OutFloat += VectorA.Z * VectorB.Z;
	return OutFloat;
}

/**
 * @brief Calculates and returns the cross product of the supplied vectors.
 * The cross product is a new vector which is orthogonal to both provided vectors.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The cross product.
 */
NODISCARD FORCEINLINE FVector3D CreateCrossVector3D(FVector3D VectorA, FVector3D VectorB)
{
	FVector3D OutVector = { 0 };
	OutVector.X = (VectorA.Y * VectorB.Z) - (VectorA.Z * VectorB.Y);
	OutVector.Y = (VectorA.Z * VectorB.X) - (VectorA.X * VectorB.Z);
	OutVector.Z = (VectorA.X * VectorB.Y) - (VectorA.Y * VectorB.X);
	return OutVector;
}

/**
 * @brief Compares all elements of VectorA and VectorB and ensures the difference
 * is less than tolerance.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @param Tolerance The difference tolerance. Typically CORE_FLOAT_EPSILON or similar.
 * @return True if within tolerance; otherwise false.
 */
NODISCARD FORCEINLINE bool8 CompareVector3D(FVector3D VectorA, FVector3D VectorB, float32 Tolerance)
{
	if (CoreAbs(VectorA.X - VectorB.X) > Tolerance)
	{
		return FALSE;
	}

	if (CoreAbs(VectorA.Y - VectorB.Y) > Tolerance)
	{
		return FALSE;
	}

	if (CoreAbs(VectorA.Z - VectorB.Z) > Tolerance)
	{
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief Returns the distance between VectorA and VectorB.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The distance between VectorA and VectorB.
 */
NODISCARD FORCEINLINE float32 DistanceVector3D(FVector3D VectorA, FVector3D VectorB)
{
	FVector3D OutVector = { 0 };
	OutVector.X = VectorA.X - VectorB.X;
	OutVector.Y = VectorA.Y - VectorB.Y;
	OutVector.Z = VectorA.Z - VectorB.Z;
	return LengthVector3D(OutVector);
}

#endif 