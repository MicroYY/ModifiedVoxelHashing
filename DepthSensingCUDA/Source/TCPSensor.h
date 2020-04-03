#pragma once
//#include <opencv2/opencv.hpp>

#include "GlobalAppState.h"
#include "MatrixConversion.h"

#ifdef TCP_SENSOR

#include <WinSock2.h>

#include "RGBDSensor.h"

#pragma comment(lib,"ws2_32.lib")


class TCPSensor :public RGBDSensor
{
public:
	TCPSensor();

	~TCPSensor();

	HRESULT createFirstConnected();

	HRESULT processDepth();

	HRESULT processColor();

	std::string getSensorName() const {
		return "TCPSensor";
	}




	const unsigned int getFrameNum() {
		return frameNum;
	}

	mat4f getRigidTransform(int offset) const {
		return m_rigidTransform;
	}

	//mat4f getRigidTransform()
	//{
	//	//mat4f tmp;
	//	//tmp[0] = m_rigidTransform[0]; tmp[1] = m_rigidTransform[1]; tmp[2] = m_rigidTransform[2]; tmp[3] = m_rigidTransform[3];
	//	//tmp[4] = m_rigidTransform[4]; tmp[5] = m_rigidTransform[5]; tmp[6] = m_rigidTransform[6]; tmp[7] = m_rigidTransform[7];
	//	//tmp[8] = m_rigidTransform[8]; tmp[9] = m_rigidTransform[9]; tmp[10] = m_rigidTransform[10]; tmp[11] = m_rigidTransform[11];
	//	//tmp[12] = m_rigidTransform[12]; tmp[13] = m_rigidTransform[13]; tmp[14] = m_rigidTransform[14]; tmp[15] = m_rigidTransform[15];
	//	return m_rigidTransform;
	//}

private:
	void quaternion2Mat(float* quaternion);

	//void recvImg(unsigned int bufSize);

	unsigned char* colorMapUchar;

	unsigned char* depthMapUchar;

	unsigned int colorWidth, colorHeight, depthWidth, depthHeight;

	char* recvImg;
	uchar* recvImgRGB;
	uchar* recvImgDepth;

	unsigned int bufSize;

	SOCKET clientSocket;

	unsigned int frameNum;

	std::mutex mtx;

	//cv::Mat image;

	mat4f				m_rigidTransform;
	mat4f fixedRenderTransform;
	float* RT;
	char* recvPose;

	float initPose[7];

	bool start;
	double distance;
	vec3d lastTranslation;
};






#endif // TCP_SENSOR
