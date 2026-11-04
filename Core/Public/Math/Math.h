#ifndef __MATH__
#define __MATH__

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"

struct FVector2D;
struct FVector3D;
struct FVector4D;

#define CORE_PI						3.14159265358959323846f

#define CORE_PI_2					(2.0f * CORE_PI)

#define CORE_HALF_PI				(0.5f * CORE_PI)

#define CORE_ONE_OVER_PI			(1.0f / CORE_PI)

#define CORE_ONE_OVER_TWO_PI		(1.0f / CORE_PI_2)

#define CORE_SQRT_TWO				1.41421356237309504880f

#define CORE_SQRT_THREE				1.73205080756887729352f

#define CORE_SQRT_ONE_OVER_TWO		0.70710678118654752440f

#define CORE_SQRT_ONE_OVER_THREE	0.57735026918962576451f

#define CORE_DEG2RAD_MULTIPLIER		(CORE_PI / 180.0f)

#define CORE_RAD2DEG_MULTIPLIER		(180.0f / CORE_PI)

/** The multiplier to convert seconds to milliseconds. */
#define CORE_SEC_TO_MS_MULTIPLIER 1000.f

/** The multiplier to convert milliseconds to seconds. */
#define CORE_MS_TO_SEC_MULTIPLIER 0.001f

/** A huge number that should be larger than any valid number used. */
#define CORE_INFINITY	1e30f

/** Smallest positive number where 1.0 + FLOAT_EPSILON != 0 */
#define CORE_FLOAT_EPSILON 1.192092896e-07f

#define CoreClamp(Value, Min, Max)		\
	((Value < Min) ? Min : (Value > Max ? Max : Value))

#define CoreMin(Lhs, Rhs)				\
	((Lhs < Rhs) ? Lhs : Rhs)

#define CoreMax(Lhs, Rhs)				\
	((Lhs > Rhs) ? Lhs : Rhs)

CORE_C_API NODISCARD float32 CoreSin(float32 X);
CORE_C_API NODISCARD float32 CoreCos(float32 X);
CORE_C_API NODISCARD float32 CoreTan(float32 X);
CORE_C_API NODISCARD float32 CoreAcos(float32 X);
CORE_C_API NODISCARD float32 CoreSqrt(float32 X);
CORE_C_API NODISCARD float32 CoreAbs(float32 X);

/** Indicates if the value if a power of 2. 0 is considering _not_ a power of 2.
 * @param Value The value to ve interpreted.
 * @returns True if the power of 2, otherwise false.
 */
NODISCARD FORCEINLINE bool8 IsPowerOf2(uint64 Value)
{
	return (Value != 0) && ((Value & (Value - 1)) == 0);
}

CORE_C_API NODISCARD int32 CoreRandom();
CORE_C_API NODISCARD int32 CoreRandomInRange(int32 Min, int32 Max);

CORE_C_API NODISCARD float32 CoreFRandom();
CORE_C_API NODISCARD float32 CoreFRandomInRange(float32 Min, float32 Max);

/**
 * @brief Converts provided degrees to radians.
 *
 * @param Degrees The degrees to be converted.
 * @return The amount in radians.
 */
NODISCARD FORCEINLINE float32 DegToRad(float32 Degrees)
{
	return Degrees * CORE_DEG2RAD_MULTIPLIER;
}

/**
 * @brief Converts provided radians to degrees.
 *
 * @param Radians The radians to be converted.
 * @return The amount in degrees.
 */
NODISCARD FORCEINLINE float32 RadToDeg(float32 Radians)
{
	return Radians * CORE_RAD2DEG_MULTIPLIER;
}

/**
 * @brief Attemps to parse a vector from the provided string.
 *
 * @param String The string to parse from. Should be space-delimited. (i.e. "1.0 2.0 3.0 4.0")
 * @param OutVector A pointer to the vector to write to.
 * @return True if parsed successfully; otherwise false.
 */
CORE_C_API bool8 StrToVec4D(char* String, struct FVector4D* OutVector);

/**
 * @brief Attemps to parse a vector from the provided string.
 *
 * @param String The string to parse from. Should be space-delimited. (i.e. "1.0 2.0 3.0")
 * @param OutVector A pointer to the vector to write to.
 * @return True if parsed successfully; otherwise false.
 */
CORE_C_API bool8 StrToVec3D(char* String, struct FVector3D* OutVector);

/**
 * @brief Attemps to parse a vector from the provided string.
 *
 * @param String The string to parse from. Should be space-delimited. (i.e. "1.0 2.0")
 * @param OutVector A pointer to the vector to write to.
 * @return True if parsed successfully; otherwise false.
 */
CORE_C_API bool8 StrToVec2D(char* String, struct FVector2D* OutVector);

#endif 
