#pragma once

enum EStoredActionType
{
	MOVE_VERTEX
};

struct StoredActionData
{
	EStoredActionType Type;

	virtual StoredActionData* Execute(bool Networked = true)
	{
		return this;
	}

	virtual StoredActionData* Reverse(bool Networked = true)
	{
		return this;
	}

	template <typename T>
	inline T* As()
	{
		if (!this) return;
		return (T*)this;
	}
};