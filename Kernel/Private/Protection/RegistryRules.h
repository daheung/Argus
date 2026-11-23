#pragma once

#include "Core/Public/Memory/Memory.h"
#include "Core/Public/Policy/RuleTypes.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlString.h"
#include "Kernel/Public/Containers/KrnlContainerLock.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"

//struct FRulePredicate
//{
//	Argus::Policy::EEventFieldType FieldType;
//	Argus::Policy::EEventMatch Match;
//	Argus::Policy::EEventCaseSensitivity CaseSensitivity;
//
//	Argus::Policy::EEventMatchExpPredicate PredicateRetType;
//
//	FPageString Value;
//};
//
//struct FRuleExpToken
//{
//	Argus::Policy::EEventMatchExpTokenType ExpTokenType;
//
//	union
//	{
//		Argus::Policy::EEventMatchExpPredicate PredicateType;
//
//		Argus::Policy::EEventMatchExpOperator OperatorType;
//	};
//};

struct FRule
{
	//FPageString RuleId;
	//LARGE_INTEGER RegisteredAt;
	//TPageArray<FRulePredicate> Predicates;
	//TPageArray<FRuleExpToken> ExpTokens;
};

class FRegistryRules
{
public:
	FORCEINLINE explicit FRegistryRules() noexcept
		: RegistryRules()
		, RegistryRulesLock()
	{
	}

public:
	FORCEINLINE bool RegisterRule(FRule& Rule) noexcept
	{
		const bool bIsInvalidRules = FRegistryRules::CheckRuleValid(Rule);
		if (bIsInvalidRules)
		{
			return false;
		}

		TPageArrayLock::FWriteGuard Guard(this->RegistryRulesLock);
		return this->RegistryRules.Push(Rule);
	}
	
public:
	NODISCARD static FORCEINLINE bool CheckRuleValid(const FRule& Rule) noexcept
	{
		//using namespace Argus::Policy;

		//const uint32 NumPredicate = Rule.Predicates.Num();
		//const uint32 NumExpToken  = Rule.ExpTokens.Num();

		//const uint32 NumPredInExpression = Rule.ExpTokens.FindAllByPredicate([](const FRuleExpToken& ExpToken) -> bool {
		//	return ExpToken.ExpTokenType == EEventMatchExpTokenType::Predicate;
		//});
		//if (NumPredicate != NumPredInExpression)
		//{
		//	return false;
		//}

		//bool bHasLhs = false;
		//bool bHasRhs = false;
		//for (uint32 OpIndex = 0; OpIndex < NumExpToken; ++OpIndex)
		//{
		//	const FRuleExpToken Operation = Rule.ExpTokens[OpIndex];
		//	const bool bIsPred = Operation.ExpTokenType == EEventMatchExpTokenType::Predicate;
		//	if (bIsPred) // Predicate
		//	{
		//		if (!bHasLhs)
		//		{
		//			bHasLhs = true;
		//		}
		//		else if (!bHasRhs)
		//		{
		//			bHasRhs = true;
		//		}
		//		else
		//		{
		//			return false;
		//		}
		//	}
		//	else // Operation
		//	{
		//		switch (Operation.OperatorType)
		//		{
		//		case EEventMatchExpOperator::And:
		//		case EEventMatchExpOperator::Or:
		//		case EEventMatchExpOperator::Not:
		//			break;
		//		default:
		//			return false;
		//		}
		//		if (!bHasLhs || !bHasLhs)
		//		{
		//			return false;
		//		}
		//	}
		//}
		//	
		//return true;

		UNREFERENCED_PARAMETER(Rule);
		return false;
	}

public:
	template <typename Predicate>
	NODISCARD FORCEINLINE uint32 UnregisterRules(Predicate&& Pred) noexcept
	{
		UNREFERENCED_PARAMETER(Pred);
		return 0;
	}

	template <typename Predicate>
	NODISCARD FORCEINLINE int32 FindByPredicate(Predicate&& Pred, FRule& OutRule) noexcept
	{
		TPageArrayLock::FReadGuard Guard(this->RegistryRules);
		int32 Index = this->RegistryRules.FindIndexByPredicate(Pred);
		if (Index == INDEX_NONE)
		{
			return Index;
		}

		ConstructItemsFrom<FRule>(&OutRule, &this->RegistryRules[Index], 1);
		return Index;
	}
	
private:
	TPageArray<FRule> RegistryRules;
	TPageArrayLock RegistryRulesLock;
};
