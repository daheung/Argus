#ifndef __MATH_MATRIX__
#define __MATH_MATRIX__

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Math/Vector3D.h"
#include "Core/Public/Math/Vector4D.h"

typedef union FMatrix
{
	alignas(16) float32 Data[16];
#if defined (CORE_USE_SIMD)
	/** Used for SIMD operations. */
	alignas(16) FVector4D Rows[4];
#endif
} FMatrix;

/**
 * @brief Creates and returns an identity matrix:
 *
 * {
 *     {1, 0, 0, 0},
 *     {0, 1, 0, 0},
 *     {0, 0, 1, 0},
 *     {0, 0, 0, 1}
 * }
 *
 * @return A new identity matrix
 */
NODISCARD FORCEINLINE FMatrix MatIdentity()
{
	FMatrix OutMatrix = { 0 };
	OutMatrix.Data[0] = 1.0f;
	OutMatrix.Data[5] = 1.0f;
	OutMatrix.Data[10] = 1.0f;
	OutMatrix.Data[15] = 1.0f;
	return OutMatrix;
}

/**
 * @brief Returns the result of multiplying MatrixA and MatrixB.
 *
 * @param MatrixA The first matrix to be multiplied.
 * @param MatrixB The second matrix to be multiplied.
 * @return The result of the matrix multiplication.
 */
NODISCARD FORCEINLINE FMatrix MatMultiplication(FMatrix MatrixA, FMatrix MatrixB)
{
	FMatrix OutMatrix = MatIdentity();
	const float32* MatrixAPtr = MatrixA.Data;
	const float32* MatrixBPtr = MatrixB.Data;
	float32* Dest = OutMatrix.Data;

	for (uint32 I = 0; I < 4; ++I)
	{
		for (uint32 J = 0; J < 4; ++J)
		{
			*Dest =
				MatrixAPtr[0] * MatrixBPtr[0 + J] +
				MatrixAPtr[1] * MatrixBPtr[4 + J] +
				MatrixAPtr[2] * MatrixBPtr[8 + J] +
				MatrixAPtr[3] * MatrixBPtr[12 + J];
			Dest++;
		}
		MatrixAPtr += 4;
	}

	return OutMatrix;
}

/**
 * @brief Creates and returns and orthographic projection matrix. Typically used to
 * render flat or 2D scenes.
 *
 * @param Left	The left side of the view frustum.
 * @param Right The right side of the view frustum.
 * @param Bottom The bottom side of the view frustum.
 * @param Top The top side of the view frustum.
 * @param NearClip The near clipping place distance.
 * @param FarClip The far clipping place distance.
 * @return A new orthographic projection matrix.
 */
NODISCARD FORCEINLINE FMatrix MatOrthographic(float32 Left, float32 Right, float32 Bottom, float32 Top, float32 NearClip, float32 FarClip)
{
	FMatrix OutMatrix = MatIdentity();

	const float32 LeftRight = 1.0f / (Left - Right);
	const float32 BottomTop = 1.0f / (Bottom - Top);
	const float32 NearFar = 1.0f / (NearClip - FarClip);

	OutMatrix.Data[0] = -2.0f * LeftRight;
	OutMatrix.Data[5] = -2.0f * BottomTop;
	OutMatrix.Data[10] = -2.0f * NearFar;

	OutMatrix.Data[12] = (Left + Right) * LeftRight;
	OutMatrix.Data[13] = (Bottom + Top) * BottomTop;
	OutMatrix.Data[14] = (NearClip + FarClip) * NearFar;

	return OutMatrix;
}

/**
 * @brief Creates and returns a perspective matrix. Typically used to render 3d scenes.
 *
 * @param FovRadians The field of view in radians.
 * @param AspectRatio The aspect ratio.
 * @param NearClip The near clipping plane distance.
 * @param FarClip The far clipping plane distance.
 * @return A new perspective matrix.
 */
NODISCARD FORCEINLINE FMatrix MatPerspective(float32 FovRadians, float32 AspectRatio, float32 NearClip, float32 FarClip)
{
	FMatrix OutMatrix = { 0 };
	const float32 HalfTanFov = CoreTan(FovRadians * 0.5f);

	OutMatrix.Data[0] = 1.0f / (AspectRatio * HalfTanFov);
	OutMatrix.Data[5] = 1.0f / HalfTanFov;
	OutMatrix.Data[10] = -((FarClip + NearClip) / (FarClip - NearClip));
	OutMatrix.Data[11] = -1.0f;
	OutMatrix.Data[14] = -((2.0f * FarClip * NearClip) / (FarClip - NearClip));
	return OutMatrix;
}

/** @brief Creates and returns a look-at matrix, or a matrix looking
 * at target from the perspective of position.
 *
 * @param Position The position of the matrix.
 * @param Target The position to "Look at".
 * @return A matrix looking at target from the perspective of position.
 */
NODISCARD FORCEINLINE FMatrix MatLookAt(FVector3D Position, FVector3D Target, FVector3D Up)
{
	FMatrix OutMatrix = { 0 };
	FVector3D AxisZ = { 0 };
	AxisZ.X = Target.X - Position.X;
	AxisZ.Y = Target.Y - Position.Y;
	AxisZ.Z = Target.Z - Position.Z;
	AxisZ = NormalizeVector3D(AxisZ);

	FVector3D AxisX = NormalizeVector3D(CreateCrossVector3D(AxisZ, Up));
	FVector3D AxisY = CreateCrossVector3D(AxisX, AxisZ);

	OutMatrix.Data[0] = AxisX.X;
	OutMatrix.Data[1] = AxisY.X;
	OutMatrix.Data[2] = -AxisZ.X;
	OutMatrix.Data[3] = 0.0f;
	OutMatrix.Data[4] = AxisX.Y;
	OutMatrix.Data[5] = AxisY.Y;
	OutMatrix.Data[6] = -AxisZ.Y;
	OutMatrix.Data[7] = 0.0f;
	OutMatrix.Data[8] = AxisX.Z;
	OutMatrix.Data[9] = AxisY.Z;
	OutMatrix.Data[10] = -AxisZ.Z;
	OutMatrix.Data[11] = 0.0f;
	OutMatrix.Data[12] = -DotVector3D(AxisX, Position);
	OutMatrix.Data[13] = -DotVector3D(AxisY, Position);
	OutMatrix.Data[14] = DotVector3D(AxisZ, Position);
	// TODO: Check this.
	OutMatrix.Data[15] = 1.0f;
	return OutMatrix;
}

/**
 * @brief Returns a transposed copy of the provided matrix(rows -> columns)
 *
 * @param Matrix The matrix to be transposed.
 * @return A transposed copy of the provided matrix;
 */
NODISCARD FORCEINLINE FMatrix MatTransposed(FMatrix Matrix)
{
	FMatrix OutMatrix = MatIdentity();
	OutMatrix.Data[0] = Matrix.Data[0];
	OutMatrix.Data[1] = Matrix.Data[4];
	OutMatrix.Data[2] = Matrix.Data[8];
	OutMatrix.Data[3] = Matrix.Data[12];
	OutMatrix.Data[4] = Matrix.Data[1];
	OutMatrix.Data[5] = Matrix.Data[5];
	OutMatrix.Data[6] = Matrix.Data[9];
	OutMatrix.Data[7] = Matrix.Data[13];
	OutMatrix.Data[8] = Matrix.Data[2];
	OutMatrix.Data[9] = Matrix.Data[6];
	OutMatrix.Data[10] = Matrix.Data[10];
	OutMatrix.Data[11] = Matrix.Data[14];
	OutMatrix.Data[12] = Matrix.Data[3];
	OutMatrix.Data[13] = Matrix.Data[7];
	OutMatrix.Data[14] = Matrix.Data[11];
	OutMatrix.Data[15] = Matrix.Data[15];
	return OutMatrix;
}

/**
 * @brief Creates and returns an inverse of the provided matrix.
 *
 * @param Matrix The matrix to be inverted.
 * @return A inverted copy of the provided matrix.
 */
NODISCARD FORCEINLINE FMatrix MatInverse(FMatrix Matrix) {
	const float32* MatrixPtr = Matrix.Data;

	float32 t0 = MatrixPtr[10] * MatrixPtr[15];
	float32 t1 = MatrixPtr[14] * MatrixPtr[11];
	float32 t2 = MatrixPtr[6] * MatrixPtr[15];
	float32 t3 = MatrixPtr[14] * MatrixPtr[7];
	float32 t4 = MatrixPtr[6] * MatrixPtr[11];
	float32 t5 = MatrixPtr[10] * MatrixPtr[7];
	float32 t6 = MatrixPtr[2] * MatrixPtr[15];
	float32 t7 = MatrixPtr[14] * MatrixPtr[3];
	float32 t8 = MatrixPtr[2] * MatrixPtr[11];
	float32 t9 = MatrixPtr[10] * MatrixPtr[3];
	float32 t10 = MatrixPtr[2] * MatrixPtr[7];
	float32 t11 = MatrixPtr[6] * MatrixPtr[3];
	float32 t12 = MatrixPtr[8] * MatrixPtr[13];
	float32 t13 = MatrixPtr[12] * MatrixPtr[9];
	float32 t14 = MatrixPtr[4] * MatrixPtr[13];
	float32 t15 = MatrixPtr[12] * MatrixPtr[5];
	float32 t16 = MatrixPtr[4] * MatrixPtr[9];
	float32 t17 = MatrixPtr[8] * MatrixPtr[5];
	float32 t18 = MatrixPtr[0] * MatrixPtr[13];
	float32 t19 = MatrixPtr[12] * MatrixPtr[1];
	float32 t20 = MatrixPtr[0] * MatrixPtr[9];
	float32 t21 = MatrixPtr[8] * MatrixPtr[1];
	float32 t22 = MatrixPtr[0] * MatrixPtr[5];
	float32 t23 = MatrixPtr[4] * MatrixPtr[1];

	FMatrix OutMatrix = { 0 };
	float32* o = OutMatrix.Data;

	o[0] = (t0 * MatrixPtr[5] + t3 * MatrixPtr[9] + t4 * MatrixPtr[13]) -
		(t1 * MatrixPtr[5] + t2 * MatrixPtr[9] + t5 * MatrixPtr[13]);
	o[1] = (t1 * MatrixPtr[1] + t6 * MatrixPtr[9] + t9 * MatrixPtr[13]) -
		(t0 * MatrixPtr[1] + t7 * MatrixPtr[9] + t8 * MatrixPtr[13]);
	o[2] = (t2 * MatrixPtr[1] + t7 * MatrixPtr[5] + t10 * MatrixPtr[13]) -
		(t3 * MatrixPtr[1] + t6 * MatrixPtr[5] + t11 * MatrixPtr[13]);
	o[3] = (t5 * MatrixPtr[1] + t8 * MatrixPtr[5] + t11 * MatrixPtr[9]) -
		(t4 * MatrixPtr[1] + t9 * MatrixPtr[5] + t10 * MatrixPtr[9]);

	float32 Determinant = (MatrixPtr[0] * o[0] + MatrixPtr[4] * o[1] + MatrixPtr[8] * o[2] + MatrixPtr[12] * o[3]);

	// Check for singular matrix (determinant near zero)
	if (CoreAbs(Determinant) < 1e-6f) {
		// Return identity matrix if the determinant is close to zero (singular matrix)
		return MatIdentity();
	}

	const float32 InvDeterminant = 1.0f / Determinant;

	o[0] = InvDeterminant * o[0];
	o[1] = InvDeterminant * o[1];
	o[2] = InvDeterminant * o[2];
	o[3] = InvDeterminant * o[3];

	o[4] = InvDeterminant * ((t1 * MatrixPtr[4] + t2 * MatrixPtr[8] + t5 * MatrixPtr[12]) -
		(t0 * MatrixPtr[4] + t3 * MatrixPtr[8] + t4 * MatrixPtr[12]));
	o[5] = InvDeterminant * ((t0 * MatrixPtr[0] + t7 * MatrixPtr[8] + t8 * MatrixPtr[12]) -
		(t1 * MatrixPtr[0] + t6 * MatrixPtr[8] + t9 * MatrixPtr[12]));
	o[6] = InvDeterminant * ((t3 * MatrixPtr[0] + t6 * MatrixPtr[4] + t11 * MatrixPtr[12]) -
		(t2 * MatrixPtr[0] + t7 * MatrixPtr[4] + t10 * MatrixPtr[12]));
	o[7] = InvDeterminant * ((t4 * MatrixPtr[0] + t9 * MatrixPtr[4] + t10 * MatrixPtr[8]) -
		(t5 * MatrixPtr[0] + t8 * MatrixPtr[4] + t11 * MatrixPtr[8]));
	o[8] = InvDeterminant * ((t12 * MatrixPtr[7] + t15 * MatrixPtr[11] + t16 * MatrixPtr[15]) -
		(t13 * MatrixPtr[7] + t14 * MatrixPtr[11] + t17 * MatrixPtr[15]));
	o[9] = InvDeterminant * ((t13 * MatrixPtr[3] + t18 * MatrixPtr[11] + t21 * MatrixPtr[15]) -
		(t12 * MatrixPtr[3] + t19 * MatrixPtr[11] + t20 * MatrixPtr[15]));
	o[10] = InvDeterminant * ((t14 * MatrixPtr[3] + t19 * MatrixPtr[7] + t22 * MatrixPtr[15]) -
		(t15 * MatrixPtr[3] + t18 * MatrixPtr[7] + t23 * MatrixPtr[15]));
	o[11] = InvDeterminant * ((t17 * MatrixPtr[3] + t20 * MatrixPtr[7] + t23 * MatrixPtr[11]) -
		(t16 * MatrixPtr[3] + t21 * MatrixPtr[7] + t22 * MatrixPtr[11]));
	o[12] = InvDeterminant * ((t14 * MatrixPtr[10] + t17 * MatrixPtr[14] + t13 * MatrixPtr[6]) -
		(t16 * MatrixPtr[14] + t12 * MatrixPtr[6] + t15 * MatrixPtr[10]));
	o[13] = InvDeterminant * ((t20 * MatrixPtr[14] + t12 * MatrixPtr[2] + t19 * MatrixPtr[10]) -
		(t18 * MatrixPtr[10] + t21 * MatrixPtr[14] + t13 * MatrixPtr[2]));
	o[14] = InvDeterminant * ((t18 * MatrixPtr[6] + t23 * MatrixPtr[14] + t15 * MatrixPtr[2]) -
		(t22 * MatrixPtr[14] + t14 * MatrixPtr[2] + t19 * MatrixPtr[6]));
	o[15] = InvDeterminant * ((t22 * MatrixPtr[10] + t16 * MatrixPtr[2] + t21 * MatrixPtr[6]) -
		(t20 * MatrixPtr[6] + t23 * MatrixPtr[10] + t17 * MatrixPtr[2]));

	return OutMatrix;
}

NODISCARD FORCEINLINE FMatrix MatTranslation(FVector3D Position)
{
	FMatrix OutMatrix = MatIdentity();
	OutMatrix.Data[12] = Position.X;
	OutMatrix.Data[13] = Position.Y;
	OutMatrix.Data[14] = Position.Z;
	//OutMatrix.Data[3] = Position.X;
	//OutMatrix.Data[7] = Position.Y;
	//OutMatrix.Data[11] = Position.Z;
	return OutMatrix;
}

/**
 * @brief Returns a scale matrix using the provided scale.
 *
 * @param Scale The 3-component scale.
 * @return A scale matrix.
 */
NODISCARD FORCEINLINE FMatrix MatScale(FVector3D Scale)
{
	FMatrix OutMatrix = MatIdentity();
	OutMatrix.Data[0] = Scale.X;
	OutMatrix.Data[5] = Scale.Y;
	OutMatrix.Data[10] = Scale.Z;
	return OutMatrix;
}

NODISCARD FORCEINLINE FMatrix MatEularX(float32 AngleRadians)
{
	FMatrix OutMatrix = MatIdentity();
	float32 Cos = CoreCos(AngleRadians);
	float32 Sin = CoreSin(AngleRadians);

	OutMatrix.Data[5] = Cos;
	OutMatrix.Data[6] = Sin;
	OutMatrix.Data[9] = -Sin;
	OutMatrix.Data[10] = Cos;
	return OutMatrix;
}

NODISCARD FORCEINLINE FMatrix MatEularY(float32 AngleRadians)
{
	FMatrix OutMatrix = MatIdentity();
	float32 Cos = CoreCos(AngleRadians);
	float32 Sin = CoreSin(AngleRadians);

	OutMatrix.Data[0] = Cos;
	OutMatrix.Data[2] = -Sin;
	OutMatrix.Data[8] = Sin;
	OutMatrix.Data[10] = Cos;
	return OutMatrix;
}

NODISCARD FORCEINLINE FMatrix MatEularZ(float32 AngleRadians)
{
	FMatrix OutMatrix = MatIdentity();
	float32 Cos = CoreCos(AngleRadians);
	float32 Sin = CoreSin(AngleRadians);

	OutMatrix.Data[0] = Cos;
	OutMatrix.Data[1] = Sin;
	OutMatrix.Data[4] = -Sin;
	OutMatrix.Data[5] = Cos;
	return OutMatrix;
}

NODISCARD FORCEINLINE FMatrix MatEular(float32 XRadians, float32 YRadians, float32 ZRadians)
{
	FMatrix OutMatrix = MatIdentity();
	const FMatrix RotateX = MatEularX(XRadians);
	const FMatrix RotateY = MatEularY(YRadians);
	const FMatrix RotateZ = MatEularZ(ZRadians);

	OutMatrix = MatMultiplication(RotateX, RotateY);
	OutMatrix = MatMultiplication(OutMatrix, RotateZ);
	return OutMatrix;
}

/**
 * @brief Returns a forward vector relative to the provided matrix.
 *
 * @param Matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
NODISCARD FORCEINLINE FVector3D MatForward(FMatrix Matrix)
{
	FVector3D Forward = { 0 };
	Forward.X = -Matrix.Data[8];
	Forward.Y = -Matrix.Data[9];
	Forward.Z = -Matrix.Data[10];
	NormalizeVector3DPtr(&Forward);
	return Forward;
}

/**
 * @brief Returns a backward vector relative to the provided matrix.
 *
 * @param Matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
NODISCARD FORCEINLINE FVector3D MatBackward(FMatrix Matrix)
{
	FVector3D Backward = { 0 };
	Backward.X = Matrix.Data[8];
	Backward.Y = Matrix.Data[9];
	Backward.Z = Matrix.Data[10];
	NormalizeVector3DPtr(&Backward);
	return Backward;
}

/**
 * @brief Returns a upward vector relative to the provided matrix.
 *
 * @param Matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
NODISCARD FORCEINLINE FVector3D MatUp(FMatrix Matrix)
{
	FVector3D Up = { 0 };
	Up.X = Matrix.Data[1];
	Up.Y = Matrix.Data[5];
	Up.Z = Matrix.Data[9];
	NormalizeVector3DPtr(&Up);
	return Up;
}

/**
 * @brief Returns a downward vector relative to the provided matrix.
 *
 * @param Matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
NODISCARD FORCEINLINE FVector3D MatDown(FMatrix Matrix)
{
	FVector3D Down = { 0 };
	Down.X = -Matrix.Data[1];
	Down.Y = -Matrix.Data[5];
	Down.Z = -Matrix.Data[9];
	NormalizeVector3DPtr(&Down);
	return Down;
}

/**
 * @brief Returns a left vector relative to the provided matrix.
 *
 * @param Matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
NODISCARD FORCEINLINE FVector3D MatLeft(FMatrix Matrix)
{
	FVector3D Left = { 0 };
	Left.X = -Matrix.Data[0];
	Left.Y = -Matrix.Data[1];
	Left.Z = -Matrix.Data[2];
	NormalizeVector3DPtr(&Left);
	return Left;
}

/**
 * @brief Returns a right vector relative to the provided matrix.
 *
 * @param Matrix The matrix from which to base the vector.
 * @return A 3-component directional vector.
 */
NODISCARD FORCEINLINE FVector3D MatRight(FMatrix Matrix)
{
	FVector3D Right = { 0 };
	Right.X = Matrix.Data[0];
	Right.Y = Matrix.Data[1];
	Right.Z = Matrix.Data[2];
	NormalizeVector3DPtr(&Right);
	return Right;
}

#endif