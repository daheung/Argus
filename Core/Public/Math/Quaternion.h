#ifndef __MATH_QUATERNION__
#define __MATH_QUATERNION__

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Math/Math.h"
#include "Core/Public/Math/Vector2D.h"
#include "Core/Public/Math/Vector3D.h"
#include "Core/Public/Math/Vector4D.h"
#include "Core/Public/Math/Matrix.h"


typedef FVector4D FQuaternion;

NODISCARD FORCEINLINE FQuaternion QuatIdentity()
{
	FQuaternion OutQuat = { 0.0f };
	OutQuat.W = 1.0f;
	return OutQuat;
}

NODISCARD FORCEINLINE float32 NormalQuat(FQuaternion Quat)
{
	return CoreSqrt(
		Quat.X * Quat.X +
		Quat.Y * Quat.Y +
		Quat.Z * Quat.Z +
		Quat.W * Quat.W
	);
}

NODISCARD FORCEINLINE FQuaternion NormalizeQuat(FQuaternion Quat)
{
	float32 Normal = NormalQuat(Quat);
	FQuaternion OutQuat = QuatIdentity();
	OutQuat.X = Quat.X / Normal;
	OutQuat.Y = Quat.Y / Normal;
	OutQuat.Z = Quat.Z / Normal;
	OutQuat.W = Quat.W / Normal;
	return OutQuat;
}

NODISCARD FORCEINLINE FQuaternion ConjugateQuat(FQuaternion Quat)
{
	FQuaternion OutQuat = QuatIdentity();
	OutQuat.X = -Quat.X;
	OutQuat.Y = -Quat.Y;
	OutQuat.Z = -Quat.Z;
	OutQuat.W = Quat.W;
	return OutQuat;
}

NODISCARD FORCEINLINE FQuaternion InverseQuat(FQuaternion Quat)
{
	return NormalizeQuat(ConjugateQuat(Quat));
}

NODISCARD FORCEINLINE FQuaternion MulQuat(FQuaternion QuatA, FQuaternion QuatB)
{
	FQuaternion OutQuat = { 0 };

	OutQuat.X = QuatA.X * QuatB.W +
		QuatA.Y * QuatB.Z +
		QuatA.Z * QuatB.Y +
		QuatA.W * QuatB.X;

	OutQuat.Y = QuatA.X * QuatB.Z +
		QuatA.Y * QuatB.W +
		QuatA.Z * QuatB.X +
		QuatA.W * QuatB.Y;

	OutQuat.Z = QuatA.X * QuatB.Y +
		QuatA.Y * QuatB.X +
		QuatA.Z * QuatB.W +
		QuatA.W * QuatB.Z;

	OutQuat.W = QuatA.X * QuatB.X +
		QuatA.Y * QuatB.Y +
		QuatA.Z * QuatB.Z +
		QuatA.W * QuatB.W;

	return OutQuat;
}

NODISCARD FORCEINLINE float32 DotQuat(FQuaternion QuatA, FQuaternion QuatB)
{
	return QuatA.X * QuatB.X +
		QuatA.Y * QuatB.Y +
		QuatA.Z * QuatB.Z +
		QuatA.W * QuatB.W;
}

/**
 * @brief Creates a rotation matrix from the given quaternion.
 *
 * @param q The quaternion to be used.
 * @return A rotation matrix.
 */
NODISCARD FORCEINLINE FMatrix QuatToMat(FQuaternion Quat) {
	FMatrix OutMatrix = MatIdentity();

	// https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix

	FQuaternion NormQuat = NormalizeQuat(Quat);

	OutMatrix.Data[0] = 1.0f - 2.0f * NormQuat.Y * NormQuat.Y - 2.0f * NormQuat.Z * NormQuat.Z;
	OutMatrix.Data[1] = 2.0f * NormQuat.X * NormQuat.Y - 2.0f * NormQuat.Z * NormQuat.W;
	OutMatrix.Data[2] = 2.0f * NormQuat.X * NormQuat.Z + 2.0f * NormQuat.Y * NormQuat.W;

	OutMatrix.Data[4] = 2.0f * NormQuat.X * NormQuat.Y + 2.0f * NormQuat.Z * NormQuat.W;
	OutMatrix.Data[5] = 1.0f - 2.0f * NormQuat.X * NormQuat.X - 2.0f * NormQuat.Z * NormQuat.Z;
	OutMatrix.Data[6] = 2.0f * NormQuat.Y * NormQuat.Z - 2.0f * NormQuat.X * NormQuat.W;

	OutMatrix.Data[8] = 2.0f * NormQuat.X * NormQuat.Z - 2.0f * NormQuat.Y * NormQuat.W;
	OutMatrix.Data[9] = 2.0f * NormQuat.Y * NormQuat.Z + 2.0f * NormQuat.X * NormQuat.W;
	OutMatrix.Data[10] = 1.0f - 2.0f * NormQuat.X * NormQuat.X - 2.0f * NormQuat.Y * NormQuat.Y;

	return OutMatrix;
}

/**
 * @brief Calculates a rotation matrix based on the quaternion and the passed in
 * center point.
 *
 * @param Quat The quaternion.
 * @param Center The center point.
 * @return A rotation matrix.
 */
NODISCARD FORCEINLINE FMatrix QuatToRotationMatrix(FQuaternion Quat, FVector3D Center) {
	FMatrix OutMatrix = { 0 };

	float32* o = OutMatrix.Data;
	o[0] = (Quat.X * Quat.X) - (Quat.Y * Quat.Y) - (Quat.Z * Quat.Z) + (Quat.W * Quat.W);
	o[1] = 2.0f * ((Quat.X * Quat.Y) + (Quat.Z * Quat.W));
	o[2] = 2.0f * ((Quat.X * Quat.Z) - (Quat.Y * Quat.W));
	o[3] = Center.X - Center.X * o[0] - Center.Y * o[1] - Center.Z * o[2];

	o[4] = 2.0f * ((Quat.X * Quat.Y) - (Quat.Z * Quat.W));
	o[5] = -(Quat.X * Quat.X) + (Quat.Y * Quat.Y) - (Quat.Z * Quat.Z) + (Quat.W * Quat.W);
	o[6] = 2.0f * ((Quat.Y * Quat.Z) + (Quat.X * Quat.W));
	o[7] = Center.Y - Center.X * o[4] - Center.Y * o[5] - Center.Z * o[6];

	o[8] = 2.0f * ((Quat.X * Quat.Z) + (Quat.Y * Quat.W));
	o[9] = 2.0f * ((Quat.Y * Quat.Z) - (Quat.X * Quat.W));
	o[10] = -(Quat.X * Quat.X) - (Quat.Y * Quat.Y) + (Quat.Z * Quat.Z) + (Quat.W * Quat.W);
	o[11] = Center.Z - Center.X * o[8] - Center.Y * o[9] - Center.Z * o[10];

	o[12] = 0.0f;
	o[13] = 0.0f;
	o[14] = 0.0f;
	o[15] = 1.0f;

	return OutMatrix;
}

NODISCARD FORCEINLINE FQuaternion QuatFromAxisAngle(FVector3D Axis, float32 Angle, bool8 bNormalize)
{
	const float32 HalfAngle = 0.5f * Angle;
	const float32 Sin = CoreSin(HalfAngle);
	const float32 Cos = CoreCos(HalfAngle);

	FQuaternion OutQuat = { 0 };
	OutQuat.X = Sin * Axis.X;
	OutQuat.Y = Sin * Axis.Y;
	OutQuat.Z = Sin * Axis.Z;
	OutQuat.W = Cos;

	if (bNormalize)
	{
		return NormalizeQuat(OutQuat);
	}

	return OutQuat;
}

/**
 * @brief Calculates spherical linear interpolation of a given percentage
 * between two quaternions.
 *
 * @param QuatA The first quaternion.
 * @param QuatB The second quaternion.
 * @param Percentage The percentage of interpolation, typically a value from
 * 0.0f-1.0f.
 * @return An interpolated quaternion.
 */
NODISCARD FORCEINLINE FQuaternion SlerpQuat(FQuaternion QuatA, FQuaternion QuatB, float32 Percentage) {
	FQuaternion OutQuaternion = { 0 };
	// Source: https://en.wikipedia.org/wiki/Slerp
	// Only unit quaternions are valid rotations.
	// Normalize to avoid undefined behavior.
	FQuaternion A = NormalizeQuat(QuatA);
	FQuaternion B = NormalizeQuat(QuatB);

	// Compute the cosine of the angle between the two vectors.
	float32 Dot = DotQuat(A, B);

	// If the dot product is negative, slerp won't take
	// the shorter path. Note that v1 and -v1 are equivalent when
	// the negation is applied to all four components. Fix by
	// reversing one quaternion.
	if (Dot < 0.0f) {
		B.X = -B.X;
		B.Y = -B.Y;
		B.Z = -B.Z;
		B.W = -B.W;
		Dot = -Dot;
	}

	const float32 DOT_THRESHOLD = 0.9995f;
	if (Dot > DOT_THRESHOLD) {
		// If the inputs are too close for comfort, linearly interpolate
		// and normalize the result.
		OutQuaternion.X = A.X + ((B.X - A.X) * Percentage);
		OutQuaternion.Y = A.Y + ((B.Y - A.Y) * Percentage);
		OutQuaternion.Z = A.Z + ((B.Z - A.Z) * Percentage);
		OutQuaternion.W = A.W + ((B.W - A.W) * Percentage);
		return NormalizeQuat(OutQuaternion);
	}

	// Since dot is in range [0, DOT_THRESHOLD], acos is safe
	const float32 Theta0 = CoreAcos(Dot);		// theta_0 = angle between input vectors
	const float32 Theta = Theta0 * Percentage;	// theta = angle between v0 and result
	const float32 SinTheta = CoreSin(Theta);	// compute this value only once
	const float32 SinTheta0 = CoreSin(Theta0);	// compute this value only once

	const float32 Sin0 =
		CoreCos(Theta) -
		Dot * SinTheta / SinTheta0;				// == sin(theta_0 - theta) / sin(theta_0)
	const float32 Sin1 = SinTheta / SinTheta0;

	OutQuaternion.X = (A.X * Sin0) + (B.X * Sin1);
	OutQuaternion.Y = (A.Y * Sin0) + (B.Y * Sin1);
	OutQuaternion.Z = (A.Z * Sin0) + (B.Z * Sin1);
	OutQuaternion.W = (A.W * Sin0) + (B.W * Sin1);
	return OutQuaternion;
}

#endif 
