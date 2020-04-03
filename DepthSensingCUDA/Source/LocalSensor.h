#pragma once

#include <opencv2/opencv.hpp>

#include "GlobalAppState.h"
#include "MatrixConversion.h"

#ifdef LOCAL_SENSOR

#include "RGBDSensor.h"

class LocalSensor :public RGBDSensor
{
public:

	LocalSensor();

	~LocalSensor();

	HRESULT createFirstConnected();

	HRESULT processDepth();

	HRESULT processColor();

	std::string getSensorName() const {
		return "LocalSensor";
	}

	mat4f getRigidTransform(int offset) const {
		return m_rigidTransform;
	}


private:
	unsigned int colorWidth, colorHeight, depthWidth, depthHeight;

	unsigned int frameNum;


	std::string path;
	mat4f				m_rigidTransform;
	float* pose;

	cv::Mat rgbMat;
	cv::Mat depthMat;
};




#endif // LOCAL_SENSOR
