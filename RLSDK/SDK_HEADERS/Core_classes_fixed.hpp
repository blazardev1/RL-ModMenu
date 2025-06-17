#pragma once

#include "Engine_classes.hpp"
#include "Core_classes.hpp"

// Forward declarations
UCLASS()
class UScriptGroup_ORS;

// Class Core.Group_ORS
// 0x00D0 (0x0068 - 0x0138)
class UGroup_ORS : public UScriptGroup_ORS
{
public:
	uint8_t                                           UnknownData00[0xD0];                           // 0x0068 (0x00D0) MISSED OFFSET

public:
	static UClass* StaticClass()
	{
		static UClass* uClassPointer = nullptr;

		if (!uClassPointer)
		{
			uClassPointer = UObject::FindClass("Class Core.Group_ORS");
		}

		return uClassPointer;
	}
};
