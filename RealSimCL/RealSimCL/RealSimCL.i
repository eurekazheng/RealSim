// Name of the module should match the project name
%module RealSimCL

// Includes so SWIG knows how to handle some of the types in the C++ standard library.
%include "carrays.i"
%include "std_vector.i"
%include "std_string.i"
%include <typemaps.i>

%{
/* Put header files here or function declarations */
#include "fluid.h"
%}

// // Define relationships between C++ data and Python data.
// namespace std {
// 	%template(VecFloat) vector<float>;

// 	// In Python, use VectorPyBranch for a vector<vector<float> >
// 	%template(VectorPyBranch) vector<vector<float> >;
// }

%include "fluid.h"
