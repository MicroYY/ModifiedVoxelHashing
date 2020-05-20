





#define NOMINMAX

#include <iostream>
#include <Windows.h>

#include <stddef.h>

#include <GL/glew.h>

/*Depending on the SDL version you are using, you may have to include SDL2/SDL.h or directly SDL.h (2.0.7)*/
#include <SDL.h>
#include <SDL_syswm.h>



#include <opencv2/opencv.hpp>
#include <Extras/OVR_Math.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

//#include <sl/Camera.hpp>

#include "Shader.hpp"	
#include "CUDARGBDSensor.h"



void __VR_runner(uchar4* image, bool isHostMem, unsigned int width, unsigned int height);
