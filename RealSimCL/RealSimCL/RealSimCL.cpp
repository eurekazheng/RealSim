#include "RealSimCL.h"


RealSimCL::RealSimCL() {}

RealSimCL::~RealSimCL() {}

void RealSimCL::simulate(int argc, char **argv)
{
	Simulate(argc, argv);
}

int main(int argc, char **argv)
{
	RealSimCL RealSimCL;
	RealSimCL.simulate(argc, argv);
	return 0;
}
