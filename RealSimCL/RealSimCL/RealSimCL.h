#pragma once
#include "fluid.h"


class RealSimCL
{
public:
	RealSimCL();
	~RealSimCL();

	void simulate(int argc, char **argv);
};
