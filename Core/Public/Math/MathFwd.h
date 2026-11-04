#ifndef __MATH_FORWARD__
#define __MATH_FORWARD__

#include "Core/Public/Math/Math.h"
#include "Core/Public/Math/Vector.h"
#include "Core/Public/Math/Matrix.h"
#include "Core/Public/Math/Quaternion.h"
#include "Core/Public/Math/NumericLimits.h"

STATIC_ASSERT(sizeof(FVector2D) == 8, "FVector2D must be exactly 8 bytes.");
STATIC_ASSERT(alignof(FVector2D) == 4, "FVector2D must be 4-byte aligned.");

STATIC_ASSERT(sizeof(FVector3D) == 12, "FVector3D must be exactly 12 bytes.");
STATIC_ASSERT(alignof(FVector3D) == 4, "FVector3D must be 4-byte aligned.");

STATIC_ASSERT(sizeof(FVector4D) == 16, "FVector4D must be exactly 16 bytes.");
STATIC_ASSERT(alignof(FVector4D) == 16, "FVector4D must be 16-byte aligned.");

#endif