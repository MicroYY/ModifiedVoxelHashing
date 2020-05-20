



#include "stdafx.h"

#include "windows.h"



#include "VR.h"



#define MAX_CHAR 128

GLchar* OVR_ZED_VS =
"#version 330 core\n \
			layout(location=0) in vec3 in_vertex;\n \
			layout(location=1) in vec2 in_texCoord;\n \
			uniform uint isLeft; \n \
			out vec2 b_coordTexture; \n \
			void main()\n \
			{\n \
				if (isLeft == 1U)\n \
				{\n \
					b_coordTexture = in_texCoord;\n \
					gl_Position = vec4(in_vertex.x, in_vertex.y, in_vertex.z,1);\n \
				}\n \
				else \n \
				{\n \
					b_coordTexture = vec2(1.0 - in_texCoord.x, in_texCoord.y);\n \
					gl_Position = vec4(-in_vertex.x, in_vertex.y, in_vertex.z,1);\n \
				}\n \
			}";

GLchar* OVR_ZED_FS =
"#version 330 core\n \
			uniform sampler2D u_textureZED; \n \
			in vec2 b_coordTexture;\n \
			out vec4 out_color; \n \
			void main()\n \
			{\n \
				out_color = vec4(texture(u_textureZED, b_coordTexture).bgr,1); \n \
			}";


void __VR_runner(uchar4* image, bool isHostMem, unsigned int width, unsigned int height)
{
	SDL_Init(SDL_INIT_VIDEO);
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result)) {
		std::cout << "ERROR: Failed to initialize libOVR" << std::endl;
		SDL_Quit();
		return;
	}


	ovrSession session;
	ovrGraphicsLuid luid;

	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		std::cout << "ERROR: Oculus Rift not detected" << std::endl;
		ovr_Shutdown();
		SDL_Quit();
		return;
	}

	int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED;
	int winWidth = width;
	int winHeight = height;
	Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	SDL_Window* window = SDL_CreateWindow("Rendering to Rift", x, y, winWidth, winHeight, flags);

	SDL_GLContext glContext = SDL_GL_CreateContext(window);

	glewInit();

	SDL_GL_SetSwapInterval(0);

	int zedWidth = width;
	int zedHeight = height;

	GLuint textureID[2];
	glGenTextures(2, textureID);
	for (size_t eye = 0; eye < 2; eye++)
	{
		glBindTexture(GL_TEXTURE_2D, textureID[eye]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, zedWidth, zedHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	cudaGraphicsResource *cimg_l;
	cudaGraphicsResource *cimg_r;
	cudaError_t  err = cudaGraphicsGLRegisterImage(&cimg_l, textureID[ovrEye_Left], GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);
	cudaError_t  err2 = cudaGraphicsGLRegisterImage(&cimg_r, textureID[ovrEye_Right], GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);
	if (err != cudaSuccess || err2 != cudaSuccess)
		std::cout << "ERROR: cannot create CUDA texture : " << err << std::endl;
	//cudaGraphicsMapResources(1, &cimg_l, 0);
	//cudaGraphicsMapResources(1, &cimg_r, 0);
	float pixel_density = 1.75f;
	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
	// Get the texture sizes of Oculus eyes
	ovrSizei textureSize0 = ovr_GetFovTextureSize(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0], pixel_density);
	ovrSizei textureSize1 = ovr_GetFovTextureSize(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1], pixel_density);
	// Compute the final size of the render buffer
	ovrSizei bufferSize;
	bufferSize.w = textureSize0.w + textureSize1.w;
	bufferSize.h = std::max(textureSize0.h, textureSize1.h);

	// Initialize OpenGL swap textures to render
	ovrTextureSwapChain textureChain = nullptr;
	// Description of the swap chain
	ovrTextureSwapChainDesc descTextureSwap = {};
	descTextureSwap.Type = ovrTexture_2D;
	descTextureSwap.ArraySize = 1;
	descTextureSwap.Width = bufferSize.w;
	descTextureSwap.Height = bufferSize.h;
	descTextureSwap.MipLevels = 1;
	descTextureSwap.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	//descTextureSwap.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	descTextureSwap.SampleCount = 1;
	descTextureSwap.StaticImage = ovrFalse;
	// Create the OpenGL texture swap chain
	result = ovr_CreateTextureSwapChainGL(session, &descTextureSwap, &textureChain);
	//std::cout << std::endl << " DV" << std::endl;;
	ovrErrorInfo errInf;
	if (OVR_SUCCESS(result)) {
		int length = 0;
		ovr_GetTextureSwapChainLength(session, textureChain, &length);
		for (int i = 0; i < length; ++i) {
			GLuint chainTexId;
			ovr_GetTextureSwapChainBufferGL(session, textureChain, i, &chainTexId);
			glBindTexture(GL_TEXTURE_2D, chainTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
	else {

		ovr_GetLastErrorInfo(&errInf);
		std::cout << "ERROR: failed creating swap texture " << errInf.ErrorString << std::endl;
		for (int eye = 0; eye < 2; eye++)
		{
			//thread_data.left_image.release();
			//thread_data.right_image.release();
			//thread_data.zed_image[eye].free();
		}
		//thread_data.cam.Close();
		//thread_data.zed.close();
		ovr_Destroy(session);
		ovr_Shutdown();
		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return;
	}

	GLuint fboID;
	glGenFramebuffers(1, &fboID);
	// Generate depth buffer of the frame buffer
	GLuint depthBuffID;
	glGenTextures(1, &depthBuffID);
	glBindTexture(GL_TEXTURE_2D, depthBuffID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLenum internalFormat = GL_DEPTH_COMPONENT24;
	GLenum type = GL_UNSIGNED_INT;
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, bufferSize.w, bufferSize.h, 0, GL_DEPTH_COMPONENT, type, NULL);

	// Create a mirror texture to display the render result in the SDL2 window
	ovrMirrorTextureDesc descMirrorTexture;
	memset(&descMirrorTexture, 0, sizeof(descMirrorTexture));
	descMirrorTexture.Width = winWidth;
	descMirrorTexture.Height = winHeight;
	descMirrorTexture.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	//descMirrorTexture.Format = OVR_FORMAT_B8G8R8_UNORM;

	ovrMirrorTexture mirrorTexture = nullptr;
	result = ovr_CreateMirrorTextureGL(session, &descMirrorTexture, &mirrorTexture);
	if (!OVR_SUCCESS(result)) {
		ovr_GetLastErrorInfo(&errInf);
		std::cout << "ERROR: Failed to create mirror texture " << errInf.ErrorString << std::endl;
	}
	GLuint mirrorTextureId;
	ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &mirrorTextureId);

	GLuint mirrorFBOID;
	glGenFramebuffers(1, &mirrorFBOID);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBOID);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	// Frame index used by the compositor, it needs to be updated each new frame
	long long frameIndex = 0;

	// FloorLevel will give tracking poses where the floor height is 0
	ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);

	// Initialize a default Pose
	ovrPosef eyeRenderPose[2];
	ovrPosef hmdToEyeOffset[2];

	// Get the Oculus view scale description
	double sensorSampleTime;

	// Create and compile the shader's sources
	Shader shader(OVR_ZED_VS, OVR_ZED_FS);

	// Compute the useful part of the ZED image
	unsigned int widthFinal = bufferSize.w / 2;
	float heightGL = 1.f;
	float widthGL = 1.f;
	if (zedWidth > 0.f) {
		unsigned int heightFinal = zedHeight * widthFinal / (float)zedWidth;
		// Convert this size to OpenGL viewport's frame's coordinates
		heightGL = (heightFinal) / (float)(bufferSize.h);
		widthGL = ((zedWidth * (heightFinal / (float)zedHeight)) / (float)widthFinal);
	}
	else {
		std::cout << "WARNING: ZED parameters got wrong values."
			"Default vertical and horizontal FOV are used.\n"
			"Check your calibration file or check if your ZED is not too close to a surface or an object."
			<< std::endl;
	}

	// Compute the Horizontal Oculus' field of view with its parameters
	float ovrFovH = (atanf(hmdDesc.DefaultEyeFov[0].LeftTan) + atanf(hmdDesc.DefaultEyeFov[0].RightTan));
	// Compute the Vertical Oculus' field of view with its parameters
	float ovrFovV = (atanf(hmdDesc.DefaultEyeFov[0].UpTan) + atanf(hmdDesc.DefaultEyeFov[0].DownTan));

	// Compute the center of the optical lenses of the headset
	float offsetLensCenterX = ((atanf(hmdDesc.DefaultEyeFov[0].LeftTan)) / ovrFovH) * 2.f - 1.f;
	float offsetLensCenterY = ((atanf(hmdDesc.DefaultEyeFov[0].UpTan)) / ovrFovV) * 2.f - 1.f;

	// Create a rectangle with the computed coordinates and push it in GPU memory
	struct GLScreenCoordinates {
		float left, up, right, down;
	} screenCoord;

	screenCoord.up = heightGL + offsetLensCenterY;
	screenCoord.down = heightGL - offsetLensCenterY;
	screenCoord.right = widthGL + offsetLensCenterX;
	screenCoord.left = widthGL - offsetLensCenterX;

	float rectVertices[12] = { -screenCoord.left, -screenCoord.up, 0, screenCoord.right, -screenCoord.up, 0, screenCoord.right, screenCoord.down, 0, -screenCoord.left, screenCoord.down, 0 };
	GLuint rectVBO[3];
	glGenBuffers(1, &rectVBO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

	float rectTexCoord[8] = { 0, 1, 1, 1, 1, 0, 0, 0 };
	glGenBuffers(1, &rectVBO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectTexCoord), rectTexCoord, GL_STATIC_DRAW);

	unsigned int rectIndices[6] = { 0, 1, 2, 0, 2, 3 };
	glGenBuffers(1, &rectVBO[2]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectVBO[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Initialize a boolean that will be used to stop the application�s loop and another one to pause/unpause rendering
	bool end = false;
	// SDL variable that will be used to store input events
	SDL_Event events;
	// This boolean is used to test if the application is focused
	bool isVisible = true;

	// Enable the shader
	glUseProgram(shader.getProgramId());
	// Bind the Vertex Buffer Objects of the rectangle that displays ZED images

	// vertices
	glEnableVertexAttribArray(Shader::ATTRIB_VERTICES_POS);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO[0]);
	glVertexAttribPointer(Shader::ATTRIB_VERTICES_POS, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectVBO[2]);
	// texture coordinates
	glEnableVertexAttribArray(Shader::ATTRIB_TEXTURE2D_POS);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO[1]);
	glVertexAttribPointer(Shader::ATTRIB_TEXTURE2D_POS, 2, GL_FLOAT, GL_FALSE, 0, 0);

	cudaGraphicsMapResources(1, &cimg_l, 0);
	cudaGraphicsMapResources(1, &cimg_r, 0);
	/*unsigned char* imageL = (unsigned char*)malloc(4 * sizeof(unsigned char) * 640 * 480);
	for (size_t i = 0; i < 640 * 480 * 4; i++)
	{
		imageL[i] = 255;
	}
	unsigned char* imageR = (unsigned char*)malloc(4 * sizeof(unsigned char) * 640 * 480);
	for (size_t i = 0; i < 640 * 480 * 4; i++)
	{
		imageR[i] = 0;
	}*/

	TCHAR szName[] = TEXT("pose");
	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(float) * 7,
		szName
	);
	float* pose = (float*)MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(float) * 7
	);


	TCHAR processName[] = TEXT("x64\\Release\\Mapping.exe");
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi;
	DWORD dwExitCode;

	if (GlobalAppState::get().s_mappingEnabled)
	{
		auto iRet = CreateProcess(processName, NULL, NULL, NULL, false, NULL, NULL, NULL, &si, &pi);
		if (iRet)
		{
			std::cout << "Process started." << std::endl
				<< "Process ID:\t"
				<< pi.dwProcessId << std::endl;
		}
		else
		{
			std::cout << "Cannot start process!" << std::endl
				<< "Error code:\t" << GetLastError() << std::endl;
		}
	}


	ovrTrackingState ts;
	while (!end) {

		// While there is an event catched and not tested
		while (SDL_PollEvent(&events)) {
			// If a key is released
			if (events.type == SDL_KEYUP) {
				// If Q -> quit the application
				if (events.key.keysym.scancode == SDL_SCANCODE_Q)
					end = true;
			}
		}

		// Get texture swap index where we must draw our frame
		GLuint curTexId;
		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(session, textureChain, &curIndex);
		ovr_GetTextureSwapChainBufferGL(session, textureChain, curIndex, &curTexId);

		// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
		hmdToEyeOffset[ovrEye_Left] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[ovrEye_Left]).HmdToEyePose;
		hmdToEyeOffset[ovrEye_Right] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[ovrEye_Right]).HmdToEyePose;

		// Get eye poses, feeding in correct IPD offset
		ovr_GetEyePoses2(session, frameIndex, ovrTrue, hmdToEyeOffset, eyeRenderPose, &sensorSampleTime);

		ts = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), ovrTrue);
		auto rotation = ts.HeadPose.ThePose.Orientation;
		auto position = ts.HeadPose.ThePose.Position;
		pose[0] = rotation.x; pose[1] = rotation.y; pose[2] = rotation.z; pose[3] = rotation.w;
		pose[4] = position.x; pose[5] = position.y; pose[6] = position.z;
		//std::cout << "rotation: " << rotation.x << " " << rotation.y << " " << rotation.z << " " << rotation.w << std::endl;
		//std::cout << "position: " << position.x << " " << position.y << " " << position.z << std::endl;



		// If the application is focused
		if (isVisible) {
			// If successful grab a new ZED image
			if (1) {
				// Update the ZED frame counter
				//thread_data.mtx.lock();

				cudaArray_t arrIm;

				cudaGraphicsSubResourceGetMappedArray(&arrIm, cimg_l, 0, 0);
				//cudaMemcpy2DToArray(arrIm, 0, 0, thread_data.zed_image[ovrEye_Left].getPtr<sl::uchar1>(sl::MEM_GPU), thread_data.zed_image[ovrEye_Left].getStepBytes(sl::MEM_GPU), thread_data.zed_image[ovrEye_Left].getWidth() * 4, thread_data.zed_image[ovrEye_Left].getHeight(), cudaMemcpyDeviceToDevice);
				//cudaMemcpy2DToArray(arrIm, 0, 0, imageL, 4 * 1280, 1280 * 4, 720, cudaMemcpyHostToDevice);
				//cudaMemcpy2DToArray(arrIm, 0, 0, g_CudaDepthSensor->getColorWithPointCloudUchar4(), g_CudaDepthSensor->getColorWidth() * 4 * 1, g_CudaDepthSensor->getColorWidth() * 4 * 1, g_CudaDepthSensor->getColorHeight(), cudaMemcpyHostToDevice);

				cudaMemcpy2DToArray(arrIm, 0, 0, image, width * 4 * 1, width * 4 * 1, height, cudaMemcpyHostToDevice);


				//cudaMemcpy2DToArray(arrIm, 0, 0, image, g_CudaDepthSensor->getColorWidth() * 4 * 1, g_CudaDepthSensor->getColorWidth() * 4 * 1, g_CudaDepthSensor->getColorHeight(), cudaMemcpyDeviceToDevice);

				cudaGraphicsSubResourceGetMappedArray(&arrIm, cimg_r, 0, 0);
				//cudaMemcpy2DToArray(arrIm, 0, 0, thread_data.zed_image[ovrEye_Right].getPtr<sl::uchar1>(sl::MEM_GPU), thread_data.zed_image[ovrEye_Right].getStepBytes(sl::MEM_GPU), thread_data.zed_image[ovrEye_Left].getWidth() * 4, thread_data.zed_image[ovrEye_Left].getHeight(), cudaMemcpyDeviceToDevice);
				cudaMemcpy2DToArray(arrIm, 0, 0, image, width * 4 * 1, width * 4 * 1, height, cudaMemcpyHostToDevice);


				//cudaMemcpy2DToArray(arrIm, 0, 0, g_CudaDepthSensor->getColorWithPointCloudUchar4(), 4 * g_CudaDepthSensor->getColorWidth() * 1, g_CudaDepthSensor->getColorWidth() * 4 * 1, g_CudaDepthSensor->getColorHeight(), cudaMemcpyHostToDevice);

				//thread_data.mtx.unlock();
				//thread_data.new_frame = false;

				// Bind the frame buffer
				glBindFramebuffer(GL_FRAMEBUFFER, fboID);
				// Set its color layer 0 as the current swap texture
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
				// Set its depth layer as our depth buffer
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffID, 0);
				// Clear the frame buffer
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glClearColor(0, 0, 0, 1);

				// Render for each Oculus eye the equivalent ZED image
				for (int eye = 0; eye < 2; eye++) {
					// Set the left or right vertical half of the buffer as the viewport
					glViewport(eye == ovrEye_Left ? 0 : bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);
					// Bind the left or right ZED image
					glBindTexture(GL_TEXTURE_2D, eye == ovrEye_Left ? textureID[ovrEye_Left] : textureID[ovrEye_Right]);
					// Bind the isLeft value
					glUniform1ui(glGetUniformLocation(shader.getProgramId(), "isLeft"), eye == ovrEye_Left ? 1U : 0U);
					// Draw the ZED image
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				}
				// Avoids an error when calling SetAndClearRenderSurface during next iteration.
				// Without this, during the next while loop iteration SetAndClearRenderSurface
				// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
				// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
				glBindFramebuffer(GL_FRAMEBUFFER, fboID);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
				// Commit changes to the textures so they get picked up frame
				ovr_CommitTextureSwapChain(session, textureChain);
			}
			// Do not forget to increment the frameIndex!
			frameIndex++;
		}

		/*
		Note: Even if we don't ask to refresh the framebuffer or if the Camera::grab()
			  doesn't catch a new frame, we have to submit an image to the Rift; it
				  needs 75Hz refresh. Else there will be jumbs, black frames and/or glitches
				  in the headset.
		 */
		ovrLayerEyeFov ld;
		ld.Header.Type = ovrLayerType_EyeFov;
		// Tell to the Oculus compositor that our texture origin is at the bottom left
		ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft; // Because OpenGL | Disable head tracking
		// Set the Oculus layer eye field of view for each view
		for (int eye = 0; eye < 2; ++eye) {
			// Set the color texture as the current swap texture
			ld.ColorTexture[eye] = textureChain;
			// Set the viewport as the right or left vertical half part of the color texture
			ld.Viewport[eye] = OVR::Recti(eye == ovrEye_Left ? 0 : bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);
			// Set the field of view
			ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
			// Set the pose matrix
			ld.RenderPose[eye] = eyeRenderPose[eye];
		}

		ld.SensorSampleTime = sensorSampleTime;

		ovrLayerHeader* layers = &ld.Header;
		// Submit the frame to the Oculus compositor
		// which will display the frame in the Oculus headset
		result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);

		if (!OVR_SUCCESS(result)) {
			ovr_GetLastErrorInfo(&errInf);
			std::cout << "ERROR: failed to submit frame " << errInf.ErrorString << std::endl;
			end = true;
		}

		if (result == ovrSuccess && !isVisible) {
			std::cout << "The application is now shown in the headset." << std::endl;
		}
		isVisible = (result == ovrSuccess);

		// This is not really needed for this application but it may be useful for an more advanced application
		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(session, &sessionStatus);
		if (sessionStatus.ShouldRecenter) {
			std::cout << "Recenter Tracking asked by Session" << std::endl;
			ovr_RecenterTrackingOrigin(session);
		}

		// Copy the frame to the mirror buffer
		// which will be drawn in the SDL2 image
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBOID);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		GLint w = winWidth;
		GLint h = winHeight;
		glBlitFramebuffer(0, h, w, 0,
			0, 0, w, h,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		// Swap the SDL2 window
		SDL_GL_SwapWindow(window);
		//std::cout << "12345" << std::endl;

	}
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &dwExitCode);
	CloseHandle(pi.hProcess);
}
