#include "Core/Public/Math/Math.h"
#include "Core/Public/Math/MathFwd.h"
#include "Core/Public/Memory/Memory.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static bool8 bRandomSeeded = FALSE;

/**
 * Note that these are here in order to prevent having to import the
 * entire <math.h> everywhere.
 */

CORE_C_API float32 CoreSin(float32 X)
{
    return sinf(X);
}

CORE_C_API float32 CoreCos(float32 X)
{
    return cosf(X);
}

CORE_C_API float32 CoreTan(float32 X)
{
    return tanf(X);
}

CORE_C_API float32 CoreAcos(float32 X)
{
    return acosf(X);
}

CORE_C_API float32 CoreSqrt(float32 X)
{
    return sqrtf(X);
}

CORE_C_API float32 CoreAbs(float32 X)
{
    return fabsf(X);
}

CORE_C_API int32 CoreRandom()
{
    if (!bRandomSeeded)
    {
        srand((uint32)time(NULL));
        bRandomSeeded = TRUE;
    }
    return rand();
}

CORE_C_API int32 CoreRandomInRange(int32 Min, int32 Max)
{
    if (!bRandomSeeded)
    {
        srand((uint32)time(NULL));
        bRandomSeeded = TRUE;
    }
    return (rand() % (Max - Min + 1)) + Min;
}

CORE_C_API float32 CoreFRandom()
{
    return (float32)CoreRandom() / (float32)RAND_MAX;
}

CORE_C_API float32 CoreFRandomInRange(float32 Min, float32 Max)
{
    return Min + ((float32)CoreRandom() / ((float32)RAND_MAX / (Max - Min)));
}

CORE_C_API bool8 StrToVec4D(char* String, FVector4D* OutVector)
{
    if (!String)
    {
        return FALSE;
    }

    FMemory::Memzero(OutVector, sizeof(FVector4D));
    int32 Result = sscanf_s(String, "%f %f %f %f", &OutVector->X, &OutVector->Y, &OutVector->Z, &OutVector->W);
    return Result != 1;
}

CORE_C_API bool8 StrToVec3D(char* String, FVector3D* OutVector)
{
    if (!String)
    {
        return FALSE;
    }

    FMemory::Memzero(OutVector, sizeof(FVector3D));
    int32 Result = sscanf_s(String, "%f %f %f", &OutVector->X, &OutVector->Y, &OutVector->Z);
    return Result != 1;
}

CORE_C_API bool8 StrToVec2D(char* String, FVector2D* OutVector)
{
    if (!String)
    {
        return FALSE;
    }

    FMemory::Memzero(OutVector, sizeof(FVector2D));
    int32 Result = sscanf_s(String, "%f %f", &OutVector->X, &OutVector->Y);
    return Result != 1;
}
