#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#ifdef MATH_UTILS_BUILDING_DLL
#define MATH_UTILS_API __declspec(dllexport)
#else 
#define MATH_UTILS_API
#endif

MATH_UTILS_API int add(int a, int b);
MATH_UTILS_API int factorial(int n);

#endif
