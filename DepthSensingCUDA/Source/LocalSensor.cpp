#include "stdafx.h"

#include <fstream>
#include <iostream>
#include "cuda_runtime.h"

#include "LocalSensor.h"

#ifdef LOCAL_SENSOR


LocalSensor::LocalSensor()
{
	colorWidth = 640;
	colorHeight = 480;

	depthWidth = 640;
	depthHeight = 480;


	RGBDSensor::init(depthWidth, depthHeight, depthWidth, depthHeight, 1);

	initializeDepthIntrinsics(518.52905273437500000, 518.34594726562500000, 319.55499267578125000, 239.06936645507812500);
	initializeDepthExtrinsics(mat4f::identity());

	initializeColorIntrinsics(518.52905273437500000, 518.34594726562500000, 319.55499267578125000, 239.06936645507812500);
	initializeColorExtrinsics(mat4f::identity());

	frameNum = 1;
	path = "D:/Code/TestingCode/data/mynt/stone";
	pose = (float*)malloc(sizeof(float) * 7);
}

LocalSensor::~LocalSensor()
{

}

HRESULT LocalSensor::createFirstConnected()
{
	return S_OK;
}

HRESULT LocalSensor::processDepth()
{
	std::cout << frameNum << "\n";

	float* depth = getDepthFloat();

	depthMat = cv::imread(path + "/depth/" + std::to_string(frameNum) + ".png", -1);
	//cv::imshow("depth", depthMat);
	//cv::waitKey(1);


	/*for (int i = 0; i < 640 * 480; i++)
		std::cout << ud[i] << std::endl;;*/

	ushort* pixel = (ushort*)depthMat.data;
	for (int i = 0; i < depthHeight; i++) {
		for (size_t j = 0; j < depthWidth; j++) {
			unsigned int index = i * depthWidth + j;
			depth[index] = pixel[index] * 0.001f;
		}
	}

	//for (int i = 0; i < depthHeight; i++) {
	//	ushort* ud = depthMat.ptr<ushort>(i);
	//	for (int j = 0; j < depthWidth; j++) {
	//		const ushort d = ud[j];
	//		unsigned int index = i * depthWidth + j;
	//		if (d == 0)
	//		{
	//			depth[index] == -std::numeric_limits<float>::infinity();
	//		}
	//		else {
	//			depth[index] = (float)d * 0.001f;
	//		}
	//		//std::cout << index << " " << depth[index] << std::endl;
	//	}
	//}

	/*
	for (int i = 0; i < depthHeight * depthWidth; i++)
	{
		const ushort& d = ud[i];
		if (d == 0)
		{
			depth[i] == -std::numeric_limits<float>::infinity();
		}
		else {
			depth[i] = (float)d * 0.001f;
		}
	}
	*/

	/*
	int ii = 0;
	for (int i = 0; i < depthHeight; i++) {
		for (int j = 0; j < depthWidth; j++) {
			unsigned int index = i * depthWidth + j;
			const ushort& d = ud[ii];
			if (d == 0)
			{
				depth[i] == -std::numeric_limits<float>::infinity();
			}
			else {
				depth[i] = (float)d * 0.001f;
			}
			ii++;
		}
	}
	*/
	return S_OK;
}

HRESULT LocalSensor::processColor()
{
	rgbMat = cv::imread(path + "/rgb/" + std::to_string(frameNum) + ".png", -1);
	//rgbMat = cv::imread("D:\\TestingCode\\data\\kinect\\rgb\\1.png", -1);
	//cv::imshow("rgb", rgbMat);
	//cv::waitKey(1);

	uchar* rgb = (uchar*)rgbMat.data;
	for (int i = 0; i < colorHeight; i++) {
		for (size_t j = 0; j < colorWidth; j++) {
			unsigned int index = i * colorWidth + j;
			m_colorRGBX[index].z = rgb[index * 3 + 0];
			m_colorRGBX[index].y = rgb[index * 3 + 1];
			m_colorRGBX[index].x = rgb[index * 3 + 2];
			m_colorRGBX[index].w = 255;
		}
	}
	if (GlobalAppState::get().s_dataWithPose) {
		std::ifstream in(path + "/pose/" + std::to_string(frameNum) + ".txt", std::ios::in);

		for (int i = 0; i < 7; i++) {
			std::string str;
			std::getline(in, str);
			pose[i] = atof(str.c_str());
		}
		float q0 = pose[3];
		float q1 = pose[0];
		float q2 = pose[1];
		float q3 = pose[2];

		m_rigidTransform._m00 = 1.f - 2.f * q2 * q2 - 2.f * q3 * q3;
		m_rigidTransform._m01 = 2.f * q1 * q2 - 2.f * q0 * q3;
		m_rigidTransform._m02 = 2.f * q1 * q3 + 2.f * q0 * q2;
		m_rigidTransform._m03 = pose[4];

		m_rigidTransform._m10 = 2 * q1 * q2 + 2 * q0 * q3;
		m_rigidTransform._m11 = 1 - 2 * q1 * q1 - 2 * q3 * q3;
		m_rigidTransform._m12 = 2 * q2 * q3 - 2 * q0 * q1;
		m_rigidTransform._m13 = pose[5];

		m_rigidTransform._m20 = 2 * q1 * q3 - 2 * q0 * q2;
		m_rigidTransform._m21 = 2 * q2 * q3 + 2 * q0 * q1;
		m_rigidTransform._m22 = 1 - 2 * q1 * q1 - 2 * q2 * q2;
		m_rigidTransform._m23 = pose[6];

		m_rigidTransform._m30 = 0;
		m_rigidTransform._m31 = 0;
		m_rigidTransform._m32 = 0;
		m_rigidTransform._m33 = 1;

		//std::cout << m_rigidTransform << "\n";
	}

	frameNum++;
	if (frameNum == 1641)
	{
		frameNum = 1;
	}

	return S_OK;
}

#endif // LOCAL_SENSOR
