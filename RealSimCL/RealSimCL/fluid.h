#pragma once

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <omp.h>
#include <iostream>
#include <fstream>


#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#ifdef WIN32
#include "glew.h"
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"
#include "freeglut_ext.h"
#include "glui.h"

#include "cl.h"
#include "cl_gl.h"


enum Projections
{
	ORTHO,
	PERSP
};

enum ButtonVals
{
	GO,
	PAUSE,
	RESET,
	QUIT
};


//
// structs we will need later:

struct xyzw
{
	float x, y, z, w;
};

struct rgba
{
	float r, g, b, a;
};

//
// function prototypes:
//

inline
float
SQR(float x)
{
	return x * x;
}

void	Animate();
void	Buttons(int);
void	Display();
void	DoRasterString(float, float, float, char *);
void	DoStrokeString(float, float, float, float, char *);
void	InitCL();
void	InitGlui();
void	InitGraphics();
void	InitLists();
bool	IsCLExtensionSupported(const char *);
void	Keyboard(unsigned char, int, int);
void	MouseButton(int, int, int, int);
void	MouseMotion(int, int);
void	PrintCLError(cl_int, char *, FILE *fp = stderr);
void	Quit();
float	Ranf(float, float);
void	Reset();
void	ResetParticles();
void	Resize(int, int);
void	Visibility(int);

int		Simulate(int argc, char **argv);