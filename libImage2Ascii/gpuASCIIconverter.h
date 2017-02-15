#pragma once

#ifdef GPUASCIICONVERTERDLL_EXPORT
	#define GPUASCIICONVERTERDLL_API __declspec(dllexport) 
#else
	#define GPUASCIICONVERTERDLL_API __declspec(dllimport) 
#endif

namespace img2ascii 
{

	class GPUASCIICONVERTERDLL_API gpuASCIIconverter
	{
	public:
		gpuASCIIconverter();
		~gpuASCIIconverter();
	};

}