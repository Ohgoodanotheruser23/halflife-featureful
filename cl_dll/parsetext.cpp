#include "parsetext.h"

void SkipSpaceCharacters(const char* const text, int& i, const int length)
{
	while (i<length && (text[i] == ' ' || text[i] == '\r' || text[i] == '\n'))
	{
		++i;
	}
}

void SkipSpaces(const char* const text, int& i, const int length)
{
	while (i<length && text[i] == ' ')
	{
		++i;
	}
}

void ConsumeNonSpaceCharacters(const char* const text, int& i, const int length)
{
	while(i<length && text[i] != ' ' && text[i] != '\n' && text[i] != '\r')
	{
		++i;
	}
}

void ConsumeLine(const char* const text, int& i, const int length)
{
	while(i<length && text[i] != '\n' && text[i] != '\r')
	{
		++i;
	}
}
