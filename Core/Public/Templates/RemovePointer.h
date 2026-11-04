#pragma once

template <typename T> struct TRemovePointer		{ using Type = T; };
template <typename T> struct TRemovePointer<T*> { using Type = T; };