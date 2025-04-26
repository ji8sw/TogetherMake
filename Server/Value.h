/* Value
	Existing library by Ji9sw
*/

#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cctype>

//#define VALUEDEBUG
constexpr char SPLITTER = ':';

#ifdef VALUEDEBUG
#include <iostream>
#endif

namespace Value
{
	struct Value
	{
		std::string Key = "";
		std::string value = "";
		std::vector<std::string> followers;
		bool Valid = false;

		std::string ToRawString(char Delimiter = SPLITTER) const
		{
#ifdef VALUEDEBUG
			std::cout << "ToString Result: " << (Key + Delimiter + value) << "\n";
#endif
			return Key + Delimiter + value;
		}

		bool ToBool()
		{
			return value == "true" ? true : false;
		}

		int ToInt()
		{
			try
			{
				int Result = stoi(value);
				return Result;
			}
			catch (...) { return 0; }
		}

		float ToFloat()
		{
			try
			{
				char* End;
				float Result = std::strtof(value.c_str(), &End);
				if (*End == '\0')
					return Result;

				return 0.0f;
			}
			catch (...) { return 0.0f; }
		}


		static void FindSplitterPositions(const std::string& Raw, size_t Start, std::vector<size_t>& Positions)
		{
			size_t Position = Raw.find(SPLITTER, Start);
			if (Position != std::string::npos)
			{
				Positions.push_back(Position);

#ifdef VALUEDEBUG
				std::cout << "Found splitter position: " << Position << "\n";
#endif

				FindSplitterPositions(Raw, Position + 1, Positions);
			}
		}

		static void ExtractBetweenSplitters(const std::string& Raw, const std::vector<size_t>& Positions, std::vector<std::string>& Results, size_t Start = 0)
		{
			if (Positions.empty()) return;

			for (size_t Index = 0; Index < Positions.size(); ++Index)
			{
#ifdef VALUEDEBUG
				std::cout << "Found follower: " << Raw.substr(Start, Positions[Index] - Start) << "\n";
#endif
				Results.push_back(Raw.substr(Start, Positions[Index] - Start));
				Start = Positions[Index] + 1;
			}
			Results.push_back(Raw.substr(Start));
		}


		Value(std::string Raw = "", bool RemoveWhitespace = false)
		{
			if (RemoveWhitespace) Raw.erase(std::remove_if(Raw.begin(), Raw.end(), ::isspace), Raw.end());

			if (!Raw.empty())
			{
				std::vector<size_t> Positions;
				FindSplitterPositions(Raw, 0, Positions);
				if (Positions.empty()) return;
				ExtractBetweenSplitters(Raw, Positions, followers);

#ifdef VALUEDEBUG
				for (const auto& follower : followers)
				{
					std::cout << "follower: " << follower << std::endl;
				}
#endif

				if (Positions[0] != std::string::npos)
				{
					this->Key = Raw.substr(0, Positions[0]);
					this->value = Raw.substr(Positions[0] + 1);
					this->Valid = true;
				}

				ExtractBetweenSplitters(Raw, Positions, followers, Positions[0]);
			}
		}
	};

	Value FromLine(std::string& Line, bool RemoveWhitespace = false)
	{
		size_t Position = Line.find(SPLITTER);
		Value NewValue(Line, RemoveWhitespace);

		return NewValue;
	}

	class Set
	{
	public:
		std::vector<Value> List;

		// Retrieves new Values from the file and appends to the list of Values
		bool AppendFrom(const std::string& FileName, bool RemoveWhitespace = false, bool AllowInvalid = false)
		{
			std::ifstream File(FileName);

			if (!File.is_open())
				return false;

			std::string Line;
			while (std::getline(File, Line))
			{
#ifdef VALUEDEBUG
				std::cout << "Constructing from: " << Line << std::endl;
#endif
				Value NewValue = FromLine(Line, RemoveWhitespace);
				if (!NewValue.Valid && AllowInvalid) continue;
				List.push_back(NewValue);
			}

			return true;
		}

		std::string ToString(bool AllowInvalid = true)
		{
			std::string Result = "";

			for (Value Val : List)
			{
				if (!Val.Valid && !AllowInvalid) continue;
				Result += Val.ToRawString() + '\n';
			}

			return Result;
		}

		bool SaveTo(std::string FileName, bool AllowInvalid = true)
		{
			std::ofstream File(FileName);

			if (!File.is_open())
				return false;

#ifdef VALUEDEBUG
			std::cout << "Saving: " << ToString(AllowInvalid) << "\n";
#endif

			File << ToString(AllowInvalid);

			return true;
		}

		void Clear() { List.clear(); }
		void Add(Value ToAdd) { List.push_back(ToAdd); }

		Value* Get(const std::string& Key)
		{
			for (int Index = 0; Index < List.size(); Index++)
				if (List[Index].Key == Key) return &List[Index];
			Value Val = Value{};
			return &Val;
		}

		void Remove(const std::string& Key)
		{
			for (int Index = 0; Index < List.size(); Index++)
				if (List[Index].Key == Key) List.erase(List.begin() + Index);
		}

		bool Contains(const std::string& Key) const
		{
			return std::any_of(List.begin(), List.end(), [&Key](const Value& Val)
				{
					return Val.Key == Key;
				});
		}

		Set(const std::string& FileName = "", bool RemoveWhitespace = false, bool AllowInvalid = false)
		{
			if (!FileName.empty())
				AppendFrom(FileName, RemoveWhitespace, AllowInvalid);
		}
	};
}