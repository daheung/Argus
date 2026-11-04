#ifndef __MATH_VECTOR_2D__
#define __MATH_VECTOR_2D__

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/Assertion.h"

#pragma warning(push)
#pragma warning(disable: 4201)

typedef struct FVector2D
{
	/** An array of x, y */
	union
	{
		alignas(4) float32 Elements[2];
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
		};
	};
} FVector2D;

#pragma warning(pop)

/**
 * @brief Creates and returns a new 2-element vector using the supplied values.
 *
 * @param X The x value.
 * @param Y The y value.
 * @return A new 2-element vector.
 */
NODISCARD FORCEINLINE FVector2D CreateVector2D(float32 X, float32 Y)
{
	FVector2D OutVector = { 0 };
	OutVector.X = X;
	OutVector.Y = Y;
	return OutVector;
}

/**
 * @brief Create and returns 2-component vector with all components set to 0.0f.
 */
NODISCARD FORCEINLINE FVector2D CreateZeroVector2D()
{
	FVector2D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = 0.0f;
	return OutVector;
}

/**
 * @brief Create and returns 2-component vector with all components set to 1.0f.
 */
NODISCARD FORCEINLINE FVector2D CreateOneVector2D()
{
	FVector2D OutVector = { 0 };
	OutVector.X = 1.0f;
	OutVector.Y = 1.0f;
	return OutVector;
}

/**
 * @brief Create and returns 2-component vector pointing up (0, 1).
 */
NODISCARD FORCEINLINE FVector2D CreateUpVector2D()
{
	FVector2D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = 1.0f;
	return OutVector;
}

/**
 * @brief Create and returns 2-component vector pointing down (0, -1).
 */
NODISCARD FORCEINLINE FVector2D CreateDownVector2D()
{
	FVector2D OutVector = { 0 };
	OutVector.X = 0.0f;
	OutVector.Y = -1.0f;
	return OutVector;
}

/**
 * @brief Create and returns 2-component vector pointing left (-1, 0).
 */
NODISCARD FORCEINLINE FVector2D CreateLeftVector2D()
{
	FVector2D OutVector = { 0 };
	OutVector.X = -1.0f;
	OutVector.Y = 0.0f;
	return OutVector;
}

/**
 * @brief Create and returns 2-component vector pointing right (1, 0).
 */
NODISCARD FORCEINLINE FVector2D CreateRightVector2D()
{
	FVector2D OutVector = { 0 };
	OutVector.X = 1.0f;
	OutVector.Y = 0.0f;
	return OutVector;
}

/**
 * @brief Adds VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector2D AddVector2D(FVector2D VectorA, FVector2D VectorB)
{
	FVector2D OutVector = { 0 };
	OutVector.X = VectorA.X + VectorB.X;
	OutVector.Y = VectorA.Y + VectorB.Y;
	return OutVector;
}

/**
 * @brief Subtracts VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector2D SubVector2D(FVector2D VectorA, FVector2D VectorB)
{
	FVector2D OutVector = { 0 };
	OutVector.X = VectorA.X - VectorB.X;
	OutVector.Y = VectorA.Y - VectorB.Y;
	return OutVector;
}

/**
 * @brief Multiplies VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector2D MulVector2D(FVector2D VectorA, FVector2D VectorB)
{
	FVector2D OutVector = { 0 };
	OutVector.X = VectorA.X * VectorB.X;
	OutVector.Y = VectorA.Y * VectorB.Y;
	return OutVector;
}

/**
 * @brief Divides VectorA to VectorB and returns a copy of the result.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @return The resulting vector.
 */
NODISCARD FORCEINLINE FVector2D DivVector2D(FVector2D VectorA, FVector2D VectorB)
{
	CORE_ASSERT((VectorB.X == 0) || (VectorB.Y == 0));
	FVector2D OutVector = { 0 };
	OutVector.X = VectorA.X / VectorB.X;
	OutVector.Y = VectorA.Y / VectorB.Y;
	return OutVector;
}

/**
 * @brief Returns the squared length of the provided vector.
 *
 * @param Vector The vector to retrieve the squared length of.
 * @return The squared length.
 */
NODISCARD FORCEINLINE float32 LengthSquaredVector2D(FVector2D Vector)
{
	return (Vector.X * Vector.X) + (Vector.Y * Vector.Y);
}

/**
 * @brief Returns the length of the provided vector.
 *
 * @param Vector The vector to retrieve the length of.
 * @return The length.
 */
NODISCARD FORCEINLINE float32 LengthVector2D(FVector2D Vector)
{
	return CoreSqrt(LengthSquaredVector2D(Vector));
}

/**
 * @brief Normalizes the provided vector in place to a unit vector.
 *
 * @param Vector A pointer to the vector to be normalized.
 */
NODISCARD FORCEINLINE void NormalizeVector2DPtr(FVector2D* const Vector)
{
	CORE_ASSERT(Vector != NULL);
	const float32 Length = LengthVector2D(*Vector);

	CORE_ASSERT(Length != 0);
	Vector->X /= Length;
	Vector->Y /= Length;
}

/**
 * @brief Returns a normalized copy of the supplied vector.
 *
 * @param Vector The vector to be normalized.
 * @return A normalized copy of the supplied vector
 */
NODISCARD FORCEINLINE FVector2D NormalizeVector2D(FVector2D Vector)
{
	NormalizeVector2DPtr(&Vector);
	return Vector;
}

/**
 * @brief Compares all elements of VectorA and VectorB and ensures the difference
 * is less than tolerance.
 *
 * @param VectorA The first vector.
 * @param VectorB The second vector.
 * @param Tolerance The difference tolerance. Typically LUMORA_FLOAT_EPSILON or similar.
 * @return True if within tolerance; otherwise false.
 */
NODISCARD FORCEINLINE bool8 CompareVector2D(FVector2D VectorA, FVector2D VectorB, float32 Tolerance)
{
	if (CoreAbs(VectorA.X - VectorB.X) > Tolerance)
	{
		return FALSE;
	}

	if (CoreAbs(VectorA.Y - VectorB.Y) > Tolerance)
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
NODISCARD FORCEINLINE float32 DistanceVector2D(FVector2D VectorA, FVector2D VectorB)
{
	FVector2D OutVector = { 0 };
	OutVector.X = VectorA.X - VectorB.X;
	OutVector.Y = VectorA.X - VectorB.Y;
	return LengthVector2D(OutVector);
}

#endif