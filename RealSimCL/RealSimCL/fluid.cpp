#include "fluid.h"
#include <iostream>
#include <string>

// using namespace std;


const char *WINDOWTITLE = { "RealSimCL" };
const char *GLUITITLE = { "Panel" };


const int NUM_PARTICLES = 512 * 16;		// 1k - 1k * 2^12
const int LOCAL_SIZE = 256;				// 8 - 512
std::string CL_FILE_NAME;
std::string CL_BINARY_NAME;
const float box_size = 50.f;		// l x w x h
const int GLUITRUE = { true };
const int GLUIFALSE = { false };

#define ESCAPE		0x1b

const int INIT_WINDOW_SIZE = { 800 };		// window size in pixels

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };

const int LEFT = { 4 };
const int MIDDLE = { 2 };
const int RIGHT = { 1 };

const float BACKCOLOR[] = { 0., 0., 0., 0. };

size_t GlobalWorkSize[3] = { NUM_PARTICLES, 1, 1 };
size_t LocalWorkSize[3] = { LOCAL_SIZE,    1, 1 };

// non-constant global variables:

int	ActiveButton;		// current button that is down
GLUI *Glui;			// instance of glui window
int	GluiWindow;		// the glut id for the glui window
int	MainWindow;		// window id for main graphics window
int	Paused;
GLfloat	RotMatrix[4][4];	// set by glui rotation widget
GLuint	SphereList;
int	WhichProjection;	// ORTHO or PERSP
int	Xmouse, Ymouse;		// mouse values
float	Xrot, Yrot;		// rotation angles in degrees
float	TransXYZ[3];		// set by glui translation widgets

double	ElapsedTime;
int		ShowPerformance;
float	max_performance = -1.;

int frame = 0;

GLuint			hPobj;
GLuint			hCobj;
cl_mem			dPobj;
cl_mem			dCobj;
struct xyzw *hVel;

struct xyzw solid_points[NUM_PARTICLES];
struct xyzw solid_normals[NUM_PARTICLES / 2];

float *Density;
struct xyzw *pressure;
struct xyzw *viscosity;
float gravity = 9.8f;
float mass = 1.f;
cl_mem			dVel;
cl_mem			dDensity;
cl_command_queue	CmdQueue;
cl_device_id		Device;
cl_kernel		Kernel;
cl_platform_id		Platform;
cl_program		Program;
cl_platform_id		PlatformID;


int Simulate(int argc, char **argv)
{
	std::string root;
	std::getline(std::cin, root);
	CL_FILE_NAME = root + "/RealSimCL/Debug/particles.cl";
	CL_BINARY_NAME = root + "/RealSimCL/Debug/particles.nv";

	float y_scale;
	scanf("%f", &y_scale);

	for (int i = NUM_PARTICLES / 2; i < NUM_PARTICLES; i++) {
		scanf("%f %f %f", &solid_points[i].x, &solid_points[i].y, &solid_points[i].z);
		solid_points[i].w = 1.;
		//printf("%f, %f, %f, %f\n", points[i].x, points[i].y, points[i].z, points[i].w);
	}
	for (int i = 0; i < NUM_PARTICLES / 2; i++) {
		scanf("%f %f %f", &solid_normals[i].x, &solid_normals[i].y, &solid_normals[i].z);
		solid_normals[i].w = 1.;
		//printf("%f, %f, %f, %f\n", normals[i].x, normals[i].y, normals[i].z, normals[i].w);
	}
	printf("%f\n", y_scale);
	glutInit(&argc, argv);
	InitGraphics();
	InitLists();
	InitCL();
	Reset();
	//InitGlui();
	GLUI_Master.set_glutIdleFunc(Animate);
	//glutHideWindow();
	//float x = 0;
	//int counter = 0;
	//while (std::cin >> x) {
	//	printf("%f ", x);
	//	counter++;
	//}
	//std::cout << counter << std::endl;
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
	return 0;
}

void Animate()
{
	cl_int  status;
	double time0, time1;

	// acquire the vertex buffers from opengl:

	glutSetWindow(MainWindow);
	glFinish();

	status = clEnqueueAcquireGLObjects(CmdQueue, 1, &dPobj, 0, NULL, NULL);
	PrintCLError(status, (char *)"clEnqueueAcquireGLObjects (1): ");
	status = clEnqueueAcquireGLObjects(CmdQueue, 1, &dCobj, 0, NULL, NULL);
	PrintCLError(status, (char *)"clEnqueueAcquireGLObjects (2): ");

	float *tmp = new float[4 * NUM_PARTICLES];
	int err = clEnqueueReadBuffer(CmdQueue, dPobj, CL_TRUE, 0, sizeof(float) * 4 * NUM_PARTICLES, tmp, 0, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to read output array! %d\n", err);
		exit(1);
	}
	for (int i = 0; i < 4 * (NUM_PARTICLES / 2); i += 4) {
		printf("%f, %f, %f\n", tmp[i], tmp[i + 1], tmp[i + 2]);
	}
	//printf("%d: %f, %f, %f, %f\n", frame, tmp[0], tmp[1], tmp[2], tmp[3]);


	if (ShowPerformance)
		time0 = omp_get_wtime();

	// 11. enqueue the Kernel object for execution:

	cl_event wait;
	status = clEnqueueNDRangeKernel(CmdQueue, Kernel, 1, NULL, GlobalWorkSize, LocalWorkSize, 0, NULL, &wait);
	PrintCLError(status, (char *)"clEnqueueNDRangeKernel: ");

	if (ShowPerformance)
	{
		status = clWaitForEvents(1, &wait);
		PrintCLError(status, (char *)"clWaitForEvents: ");
		time1 = omp_get_wtime();
		ElapsedTime = time1 - time0;
	}

	clFinish(CmdQueue);
	status = clEnqueueReleaseGLObjects(CmdQueue, 1, &dCobj, 0, NULL, NULL);
	PrintCLError(status, (char *)"clEnqueueReleaseGLObjects (1): ");
	status = clEnqueueReleaseGLObjects(CmdQueue, 1, &dPobj, 0, NULL, NULL);
	PrintCLError(status, (char *)"clEnqueueReleaseGLObjects (2): ");


	glutSetWindow(MainWindow);
	glutPostRedisplay();

	if (++frame == 800) {
		glutLeaveMainLoop();
	}
}




//
// glui buttons callback:
//

void Buttons(int id)
{
	char m1[128] = "clEneueueWriteBuffer: ";
	cl_int status;
	switch (id)
	{
	case GO:
		GLUI_Master.set_glutIdleFunc(Animate);
		break;

	case PAUSE:
		Paused = !Paused;
		if (Paused)
			GLUI_Master.set_glutIdleFunc(NULL);
		else
			GLUI_Master.set_glutIdleFunc(Animate);
		break;

	case RESET:
		Reset();
		ResetParticles();
		status = clEnqueueWriteBuffer(CmdQueue, dVel, CL_FALSE, 0, 4 * sizeof(float) * NUM_PARTICLES, hVel, 0, NULL, NULL);
		PrintCLError(status, m1);
		status = clEnqueueWriteBuffer(CmdQueue, dDensity, CL_FALSE, 0, sizeof(float) * NUM_PARTICLES, Density, 0, NULL, NULL);
		PrintCLError(status, m1);
		GLUI_Master.set_glutIdleFunc(NULL);
		Glui->sync_live();
		glutSetWindow(MainWindow);
		glutPostRedisplay();
		break;

	case QUIT:
		Quit();
		break;

	default:
		fprintf(stderr, "Don't know what to do with Button ID %d\n", id);
	}

}



//
// draw the complete scene:
//

void Display()
{
	glutSetWindow(MainWindow);
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_FLAT);
	GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
	GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = (vx - v) / 2;
	GLint yb = (vy - v) / 2;
	glViewport(xl, yb, v, v);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (WhichProjection == ORTHO)
		glOrtho(-300., 300., -300., 300., 0.1, 2000.);
	else
		gluPerspective(50., 1., 0.1, 2000.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0., 50., 500., 0., 200., 0., 0., 1., 0.);
	glTranslatef((GLfloat)TransXYZ[0], (GLfloat)TransXYZ[1], (GLfloat)TransXYZ[2]);
	glRotatef((GLfloat)Yrot, 0., 1., 0.);
	glRotatef((GLfloat)Xrot, 1., 0., 0.);
	glMultMatrixf((const GLfloat *)RotMatrix);
	glScalef(1.f, 1.f, 1.f);

	glDisable(GL_FOG);

	// ****************************************
	// Here is where you draw the current state of the particles:
	// ****************************************

	glBindBuffer(GL_ARRAY_BUFFER, hPobj);
	glVertexPointer(4, GL_FLOAT, 0, (void *)0);
	glEnableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, hCobj);
	glColorPointer(4, GL_FLOAT, 0, (void *)0);
	glEnableClientState(GL_COLOR_ARRAY);

	glPointSize(3.);
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
	glPointSize(1.);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glCallList(SphereList);

	if (ShowPerformance)
	{
		char str[128];

		if (max_performance < (float)NUM_PARTICLES / ElapsedTime / 1000000000.) {
			max_performance = (float)NUM_PARTICLES / ElapsedTime / 1000000000.;
			if (max_performance == INFINITY) max_performance = -1.;
		}



		sprintf(str, "Current %6.4f GigaParticles/Sec   MAX %6.4f GigaParticles/Sec", (float)NUM_PARTICLES / ElapsedTime / 1000000000., (float)max_performance);
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0., 100., 0., 100.);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glColor3f(1., 1., 1.);
		DoRasterString(5., 95., 0., str);
	}

	glutSwapBuffers();
	glFlush();
}



//
// use glut to display a string of characters using a raster font:
//

void DoRasterString(float x, float y, float z, char *s)
{
	char c;			// one character to print

	glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
	for (; (c = *s) != '\0'; s++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
	}
}



//
// use glut to display a string of characters using a stroke font:
//

void DoStrokeString(float x, float y, float z, float ht, char *s)
{
	char c;			// one character to print

	glPushMatrix();
	glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
	float sf = ht / (119.05f + 33.33f);
	glScalef((GLfloat)sf, (GLfloat)sf, (GLfloat)sf);
	for (; (c = *s) != '\0'; s++)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
	}
	glPopMatrix();
}


//
// initialize the opencl stuff:
//

void InitCL()
{
	// see if we can even open the opencl Kernel Program
	// (no point going on if we can't):

	FILE *fp = fopen(CL_FILE_NAME.c_str(), "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open OpenCL source file '%s'\n", CL_FILE_NAME);
		return;
	}

	// 2. allocate the host memory buffers:

	cl_int status;		// returned status from opencl calls
				// test against CL_SUCCESS

	// get the platform id:

	status = clGetPlatformIDs(1, &Platform, NULL);
	PrintCLError(status, (char *)"clGetPlatformIDs: ");

	// get the device id:

	status = clGetDeviceIDs(Platform, CL_DEVICE_TYPE_GPU, 1, &Device, NULL);
	PrintCLError(status, (char *)"clGetDeviceIDs: ");


	// since this is an opengl interoperability program,
	// check if the opengl sharing extension is supported,
	// (no point going on if it isn't):
	// (we need the Device in order to ask, so can't do it any sooner than here)

	if (IsCLExtensionSupported("cl_khr_gl_sharing"))
	{
		//fprintf(stderr, "cl_khr_gl_sharing is supported.\n");
		//printf("NUM_PARTICLES = %d\nLOCAL_SIZE = %d\n", NUM_PARTICLES, LOCAL_SIZE);
	}
	else
	{
		fprintf(stderr, "cl_khr_gl_sharing is not supported -- sorry.\n");
		return;
	}



	// 3. create an opencl context based on the opengl context:

	cl_context_properties props[] =
	{
		CL_GL_CONTEXT_KHR,		(cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR,			(cl_context_properties)wglGetCurrentDC(),
		CL_CONTEXT_PLATFORM,		(cl_context_properties)Platform,
		0
	};

	cl_context Context = clCreateContext(props, 1, &Device, NULL, NULL, &status);
	PrintCLError(status, (char *)"clCreateContext: ");

	// 4. create an opencl command queue:

	CmdQueue = clCreateCommandQueue(Context, Device, 0, &status);
	if (status != CL_SUCCESS)
		fprintf(stderr, "clCreateCommandQueue failed\n");

	// create array and the opengl vertex array buffer and color array buffer:

	// velocity
	delete[] hVel;
	hVel = new struct xyzw[NUM_PARTICLES];
	// Density
	delete[] Density;
	Density = new float[NUM_PARTICLES];

	glGenBuffers(1, &hPobj);
	glBindBuffer(GL_ARRAY_BUFFER, hPobj);
	glBufferData(GL_ARRAY_BUFFER, 4 * NUM_PARTICLES * sizeof(float), NULL, GL_STATIC_DRAW);

	glGenBuffers(1, &hCobj);
	glBindBuffer(GL_ARRAY_BUFFER, hCobj);
	glBufferData(GL_ARRAY_BUFFER, 4 * NUM_PARTICLES * sizeof(float), NULL, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);	// unbind the buffer

	// fill those arrays and buffers:

	ResetParticles();

	// 5. create the opencl version of the opengl buffers:

	dPobj = clCreateFromGLBuffer(Context, 0, hPobj, &status);
	PrintCLError(status, (char *)"clCreateFromGLBuffer (1)");

	dCobj = clCreateFromGLBuffer(Context, 0, hCobj, &status);
	PrintCLError(status, (char *)"clCreateFromGLBuffer (2)");

	// 5. create the opencl version of the velocity/Density array:

	dVel = clCreateBuffer(Context, CL_MEM_READ_WRITE, 4 * sizeof(float) * NUM_PARTICLES, NULL, &status);
	PrintCLError(status, (char *)"clCreateBuffer: ");

	dDensity = clCreateBuffer(Context, CL_MEM_READ_WRITE, sizeof(float) * NUM_PARTICLES, NULL, &status);
	PrintCLError(status, (char *)"clCreateBuffer: ");

	// 6. enqueue the command to write the data from the host buffers to the Device buffers:

	status = clEnqueueWriteBuffer(CmdQueue, dVel, CL_FALSE, 0, 4 * sizeof(float) * NUM_PARTICLES, hVel, 0, NULL, NULL);
	PrintCLError(status, (char *)"clEneueueWriteBuffer: ");

	status = clEnqueueWriteBuffer(CmdQueue, dDensity, CL_FALSE, 0, sizeof(float) * NUM_PARTICLES, Density, 0, NULL, NULL);
	PrintCLError(status, (char *)"clEneueueWriteBuffer: ");

	// 7. read the Kernel code from a file:

	fseek(fp, 0, SEEK_END);
	size_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *clProgramText = new char[fileSize + 1];		// leave room for '\0'
	size_t n = fread(clProgramText, 1, fileSize, fp);
	clProgramText[fileSize] = '\0';
	fclose(fp);

	// create the text for the Kernel Program:

	char *strings[1];
	strings[0] = clProgramText;
	Program = clCreateProgramWithSource(Context, 1, (const char **)strings, NULL, &status);
	if (status != CL_SUCCESS)
		fprintf(stderr, "clCreateProgramWithSource failed\n");
	delete[] clProgramText;

	// 8. compile and link the Kernel code:

	const char *options = { "" };
	status = clBuildProgram(Program, 1, &Device, options, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		size_t size;
		clGetProgramBuildInfo(Program, Device, CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
		cl_char *log = new cl_char[size];
		clGetProgramBuildInfo(Program, Device, CL_PROGRAM_BUILD_LOG, size, log, NULL);
		fprintf(stderr, "clBuildProgram failed:\n%s\n", log);
		delete[] log;
	}


	// 9. create the Kernel object:

	Kernel = clCreateKernel(Program, "Particle", &status);
	PrintCLError(status, (char *)"clCreateKernel failed: ");


	// 10. setup the arguments to the Kernel object:

	status = clSetKernelArg(Kernel, 0, sizeof(cl_mem), &dPobj);
	PrintCLError(status, (char *)"clSetKernelArg (1): ");

	status = clSetKernelArg(Kernel, 1, sizeof(cl_mem), &dVel);
	PrintCLError(status, (char *)"clSetKernelArg (2): ");

	status = clSetKernelArg(Kernel, 2, sizeof(cl_mem), &dCobj);
	PrintCLError(status, (char *)"clSetKernelArg (3): ");

	status = clSetKernelArg(Kernel, 3, sizeof(cl_mem), &dDensity);
	PrintCLError(status, (char *)"clSetKernelArg (4): ");
}


//
// initialize the glui window:
//

void InitGlui()
{
	glutInitWindowPosition(INIT_WINDOW_SIZE + 50, 0);
	Glui = GLUI_Master.create_glui((char *)GLUITITLE);
	Glui->add_statictext((char *)GLUITITLE);
	Glui->add_separator();
	Glui->add_checkbox((char *)"Perspective", &WhichProjection);
	Glui->add_checkbox((char *)"Show Performance", &ShowPerformance);

	GLUI_Panel *panel = Glui->add_panel((char *)"Object Transformation");

	GLUI_Rotation *rot = Glui->add_rotation_to_panel(panel, (char *)"Rotation", (float *)RotMatrix);
	rot->set_spin(1.0);

	Glui->add_column_to_panel(panel, FALSE);
	GLUI_Translation *trans = Glui->add_translation_to_panel(panel, (char *)"Trans XY", GLUI_TRANSLATION_XY, &TransXYZ[0]);
	trans->set_speed(1.1f);

	Glui->add_column_to_panel(panel, FALSE);
	trans = Glui->add_translation_to_panel(panel, (char *)"Trans Z", GLUI_TRANSLATION_Z, &TransXYZ[2]);
	trans->set_speed(1.1f);

	panel = Glui->add_panel((char *)"", FALSE);
	Glui->add_button_to_panel(panel, (char *)"Go !", GO, (GLUI_Update_CB)Buttons);
	Glui->add_column_to_panel(panel, FALSE);
	Glui->add_button_to_panel(panel, (char *)"Pause", PAUSE, (GLUI_Update_CB)Buttons);
	Glui->add_column_to_panel(panel, FALSE);
	Glui->add_button_to_panel(panel, (char *)"Reset", RESET, (GLUI_Update_CB)Buttons);
	Glui->add_column_to_panel(panel, FALSE);
	Glui->add_button_to_panel(panel, (char *)"Quit", QUIT, (GLUI_Update_CB)Buttons);

	Glui->set_main_gfx_window(MainWindow);
	GLUI_Master.set_glutIdleFunc(NULL);
}



//
// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions
//

void InitGraphics()
{
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(INIT_WINDOW_SIZE, INIT_WINDOW_SIZE);

	MainWindow = glutCreateWindow(WINDOWTITLE);
	glutSetWindowTitle(WINDOWTITLE);
	glClearColor(BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3]);


	// setup the callback routines:

	glutSetWindow(MainWindow);
	glutDisplayFunc(Display);
	glutReshapeFunc(Resize);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseMotion);
	glutVisibilityFunc(Visibility);

#ifdef WIN32
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		fprintf(stderr, "glewInit Error\n");
	}
#endif
}



//
// initialize the display lists that will not change:
//

void InitLists()
{
	SphereList = glGenLists(1);
	glNewList(SphereList, GL_COMPILE);
	// sphere
	glColor3f(.5f, .5f, .5f);
	glPushMatrix();
	glTranslatef(0.f, 0.f, 0.f);
	//glutSolidSphere(150.f, 64, 64);
	glPopMatrix();
	glEndList();
}



//
// the keyboard callback:
//

void Keyboard(unsigned char c, int x, int y)
{
	switch (c)
	{
	case 'o':
	case 'O':
		WhichProjection = ORTHO;
		break;

	case 'p':
	case 'P':
		WhichProjection = PERSP;
		break;

	case 'q':
	case 'Q':
	case ESCAPE:
		Buttons(QUIT);	// will not return here
		break;			// happy compiler

	default:
		fprintf(stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c);
	}
	Glui->sync_live();
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}



//
// called when the mouse button transitions down or up:
//

void MouseButton(int button, int state, int x, int y)
{
	int b;			// LEFT, MIDDLE, or RIGHT

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		b = LEFT;		break;

	case GLUT_MIDDLE_BUTTON:
		b = MIDDLE;		break;

	case GLUT_RIGHT_BUTTON:
		b = RIGHT;		break;

	default:
		b = 0;
		fprintf(stderr, "Unknown mouse button: %d\n", button);
	}


	// button down sets the bit, up clears the bit:

	if (state == GLUT_DOWN)
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}
}



//
// called when the mouse moves while a button is down:
//

void MouseMotion(int x, int y)
{
	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if (ActiveButton & LEFT)
	{
		Xrot += (ANGFACT * dy);
		Yrot += (ANGFACT * dx);
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}



//
// reset the transformations and the colors:
//
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene
//

void Reset()
{
	ActiveButton = 0;
	Paused = GLUIFALSE;
	ShowPerformance = GLUIFALSE;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
	TransXYZ[0] = TransXYZ[1] = TransXYZ[2] = 0.;

	RotMatrix[0][1] = RotMatrix[0][2] = RotMatrix[0][3] = 0.;
	RotMatrix[1][0] = RotMatrix[1][2] = RotMatrix[1][3] = 0.;
	RotMatrix[2][0] = RotMatrix[2][1] = RotMatrix[2][3] = 0.;
	RotMatrix[3][0] = RotMatrix[3][1] = RotMatrix[3][3] = 0.;
	RotMatrix[0][0] = RotMatrix[1][1] = RotMatrix[2][2] = RotMatrix[3][3] = 1.;
}


void ResetParticles()
{
	// set particles position
	glBindBuffer(GL_ARRAY_BUFFER, hPobj);
	struct xyzw *points = (struct xyzw *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	float	pi = 3.1415926f;
	float	azimuthal = 0.5 * pi / (float)(NUM_PARTICLES / 2);
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		if (i < NUM_PARTICLES / 2) {
			points[i].x = Ranf(-30, 30);
			points[i].y = Ranf(170., 200.);
			points[i].z = Ranf(-30, 30);
			points[i].w = 1.;
		}
		else {
			//float theta = Ranf(0.f, pi * 2.f);
			//float fi = Ranf(0.f, pi * 2.f);
			//float r = 149.f;
			//points[i].x = r * sinf(theta) * cosf(fi);
			//points[i].y = r * sinf(theta) * sinf(fi);
			//points[i].z = r * cosf(theta);
			//points[i].w = 1.;
			points[i].x = solid_points[i].x;
			points[i].y = solid_points[i].y;
			points[i].z = solid_points[i].z;
			points[i].w = solid_points[i].w;
		}

	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	// set color
	glBindBuffer(GL_ARRAY_BUFFER, hCobj);
	struct rgba *colors = (struct rgba *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		if (i < NUM_PARTICLES / 2) {
			colors[i].r = .629f;
			colors[i].g = .398f;
			colors[i].b = .969f;
			colors[i].a = 1.;
		}
		else {
			colors[i].r = solid_normals[i - NUM_PARTICLES / 2].x;
			colors[i].g = solid_normals[i - NUM_PARTICLES / 2].y;
			colors[i].b = solid_normals[i - NUM_PARTICLES / 2].z;
			colors[i].a = 1.;
		}

	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	// set velocity
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		hVel[i].x = 0.;
		hVel[i].y = 0.;
		hVel[i].z = 0.;
	}

	// set Density
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		Density[i] = 0.f;
	}
}



void Resize(int width, int height)
{
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


void Visibility(int state)
{
	if (state == GLUT_VISIBLE)
	{
		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}







// the stroke characters 'X' 'Y' 'Z' :

static float xx[] = {
		0., 1., 0., 1.
};

static float xy[] = {
		-.5, .5, .5, -.5
};

static int xorder[] = {
		1, 2, -3, 4
};


static float yx[] = {
		0., 0., -.5, .5
};

static float yy[] = {
		0., .6f, 1., 1.
};

static int yorder[] = {
		1, 2, 3, -2, 4
};


static float zx[] = {
		1., 0., 1., 0., .25, .75
};

static float zy[] = {
		.5, .5, -.5, -.5, 0., 0.
};

static int zorder[] = {
		1, 2, 3, 4, -5, 6
};



//
// exit gracefully:
//

void Quit()
{
	Glui->close();
	glutSetWindow(MainWindow);
	glFinish();
	glutDestroyWindow(MainWindow);


	// 13. clean everything up:

	clReleaseKernel(Kernel);
	clReleaseProgram(Program);
	clReleaseCommandQueue(CmdQueue);
	clReleaseMemObject(dPobj);
	clReleaseMemObject(dCobj);
	clReleaseMemObject(dDensity);

	exit(0);
}




#define TOP	2147483647.		// 2^31 - 1	

float Ranf(float low, float high)
{
	long random();		// returns integer 0 - TOP

	float r = (float)rand();
	return(low + r * (high - low) / (float)RAND_MAX);
}


bool IsCLExtensionSupported(const char *extension)
{
	// see if the extension is bogus:

	if (extension == NULL || extension[0] == '\0')
		return false;

	char *where = (char *)strchr(extension, ' ');
	if (where != NULL)
		return false;

	// get the full list of extensions:

	size_t extensionSize;
	clGetDeviceInfo(Device, CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize);
	char *extensions = new char[extensionSize];
	clGetDeviceInfo(Device, CL_DEVICE_EXTENSIONS, extensionSize, extensions, NULL);

	for (char *start = extensions; ; )
	{
		where = (char *)strstr((const char *)start, extension);
		if (where == 0)
		{
			delete[] extensions;
			return false;
		}

		char *terminator = where + strlen(extension);	// points to what should be the separator

		if (*terminator == ' ' || *terminator == '\0' || *terminator == '\r' || *terminator == '\n')
		{
			delete[] extensions;
			return true;
		}
		start = terminator;
	}

	delete[] extensions;
	return false;
}


struct errorcode
{
	cl_int		statusCode;
	const char *meaning;
}
ErrorCodes[] =
{
	{ CL_SUCCESS,				""					},
	{ CL_DEVICE_NOT_FOUND,			"Device Not Found"			},
	{ CL_DEVICE_NOT_AVAILABLE,		"Device Not Available"			},
	{ CL_COMPILER_NOT_AVAILABLE,		"Compiler Not Available"		},
	{ CL_MEM_OBJECT_ALLOCATION_FAILURE,	"Memory Object Allocation Failure"	},
	{ CL_OUT_OF_RESOURCES,			"Out of resources"			},
	{ CL_OUT_OF_HOST_MEMORY,		"Out of Host Memory"			},
	{ CL_PROFILING_INFO_NOT_AVAILABLE,	"Profiling Information Not Available"	},
	{ CL_MEM_COPY_OVERLAP,			"Memory Copy Overlap"			},
	{ CL_IMAGE_FORMAT_MISMATCH,		"Image Format Mismatch"			},
	{ CL_IMAGE_FORMAT_NOT_SUPPORTED,	"Image Format Not Supported"		},
	{ CL_BUILD_PROGRAM_FAILURE,		"Build Program Failure"			},
	{ CL_MAP_FAILURE,			"Map Failure"				},
	{ CL_INVALID_VALUE,			"Invalid Value"				},
	{ CL_INVALID_DEVICE_TYPE,		"Invalid Device Type"			},
	{ CL_INVALID_PLATFORM,			"Invalid Platform"			},
	{ CL_INVALID_DEVICE,			"Invalid Device"			},
	{ CL_INVALID_CONTEXT,			"Invalid Context"			},
	{ CL_INVALID_QUEUE_PROPERTIES,		"Invalid Queue Properties"		},
	{ CL_INVALID_COMMAND_QUEUE,		"Invalid Command Queue"			},
	{ CL_INVALID_HOST_PTR,			"Invalid Host Pointer"			},
	{ CL_INVALID_MEM_OBJECT,		"Invalid Memory Object"			},
	{ CL_INVALID_IMAGE_FORMAT_DESCRIPTOR,	"Invalid Image Format Descriptor"	},
	{ CL_INVALID_IMAGE_SIZE,		"Invalid Image Size"			},
	{ CL_INVALID_SAMPLER,			"Invalid Sampler"			},
	{ CL_INVALID_BINARY,			"Invalid Binary"			},
	{ CL_INVALID_BUILD_OPTIONS,		"Invalid Build Options"			},
	{ CL_INVALID_PROGRAM,			"Invalid Program"			},
	{ CL_INVALID_PROGRAM_EXECUTABLE,	"Invalid Program Executable"		},
	{ CL_INVALID_KERNEL_NAME,		"Invalid Kernel Name"			},
	{ CL_INVALID_KERNEL_DEFINITION,		"Invalid Kernel Definition"		},
	{ CL_INVALID_KERNEL,			"Invalid Kernel"			},
	{ CL_INVALID_ARG_INDEX,			"Invalid Argument Index"		},
	{ CL_INVALID_ARG_VALUE,			"Invalid Argument Value"		},
	{ CL_INVALID_ARG_SIZE,			"Invalid Argument Size"			},
	{ CL_INVALID_KERNEL_ARGS,		"Invalid Kernel Arguments"		},
	{ CL_INVALID_WORK_DIMENSION,		"Invalid Work Dimension"		},
	{ CL_INVALID_WORK_GROUP_SIZE,		"Invalid Work Group Size"		},
	{ CL_INVALID_WORK_ITEM_SIZE,		"Invalid Work Item Size"		},
	{ CL_INVALID_GLOBAL_OFFSET,		"Invalid Global Offset"			},
	{ CL_INVALID_EVENT_WAIT_LIST,		"Invalid Event Wait List"		},
	{ CL_INVALID_EVENT,			"Invalid Event"				},
	{ CL_INVALID_OPERATION,			"Invalid Operation"			},
	{ CL_INVALID_GL_OBJECT,			"Invalid GL Object"			},
	{ CL_INVALID_BUFFER_SIZE,		"Invalid Buffer Size"			},
	{ CL_INVALID_MIP_LEVEL,			"Invalid MIP Level"			},
	{ CL_INVALID_GLOBAL_WORK_SIZE,		"Invalid Global Work Size"		},
};

void PrintCLError(cl_int errorCode, char *prefix, FILE *fp)
{
	if (errorCode == CL_SUCCESS)
		return;

	const int numErrorCodes = sizeof(ErrorCodes) / sizeof(struct errorcode);
	const char *meaning = "";
	for (int i = 0; i < numErrorCodes; i++)
	{
		if (errorCode == ErrorCodes[i].statusCode)
		{
			meaning = ErrorCodes[i].meaning;
			break;
		}
	}

	fprintf(fp, "%s %s\n", prefix, meaning);
}
