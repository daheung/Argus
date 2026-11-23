#pragma once

enum class EPoolType
{
    Paged,
    NonPaged,
};

enum class EContainerLockType : uint8
{
	Wait,
	Spinlock
};