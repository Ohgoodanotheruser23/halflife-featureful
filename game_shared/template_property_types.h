#pragma once
#ifndef TEMPLATE_VALUE_TYPES_H
#define TEMPLATE_VALUE_TYPES_H

struct Color
{
	Color(): r(0), g(0), b(0) {}
	Color(int red, int green, int blue): r(red), g(green), b(blue) {}
	int r;
	int g;
	int b;
};

template <typename N>
struct NumberRange
{
	NumberRange(): min(), max() {}
	NumberRange(N mini, N maxi): min(mini), max(maxi) {}
	NumberRange(N val): min(val), max(val) {}
	N min;
	N max;
};

typedef NumberRange<float> FloatRange;
typedef NumberRange<int> IntRange;

#endif
