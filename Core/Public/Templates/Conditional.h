#pragma once

template <bool Test, typename T1, typename T2> 
struct TConditional 
{
};

template <typename T1, typename T2> struct TConditional<true , T1, T2> { using Type = T1; };
template <typename T1, typename T2> struct TConditional<false, T1, T2> { using Type = T2; };