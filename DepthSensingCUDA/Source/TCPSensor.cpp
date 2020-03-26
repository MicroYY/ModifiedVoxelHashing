#include "stdafx.h"

#include <fstream>
#include "cuda_runtime.h"

#include "TCPSensor.h"


#ifdef TCP_SENSOR


TCPSensor::TCPSensor()
{
#ifdef HD
	colorWidth = 1280;
	colorHeight = 720;

	depthWidth = 1280;
	depthHeight = 720;


	RGBDSensor::init(depthWidth, depthHeight, depthWidth, depthHeight, 1);

	initializeDepthIntrinsics(1039.90344238281250000, 1039.85351562500000000, 640.06604003906250000, 357.32925415039062500);
	initializeDepthExtrinsics(mat4f::identity());

	initializeColorIntrinsics(1039.90344238281250000, 1039.85351562500000000, 640.06604003906250000, 357.32925415039062500);
	initializeColorExtrinsics(mat4f::identity());

	bufSize = colorHeight * colorWidth * 2 * 3;
	colorMapUchar = (unsigned char*)malloc(sizeof(unsigned char) * colorWidth * colorHeight * 3);
	depthMapUchar = (unsigned char*)malloc(sizeof(unsigned char) * depthWidth * depthHeight * 3);
	recvImg = (char*)malloc(sizeof(unsigned char) * bufSize);

	frameNum = 0;
	start = false;
	image = cv::Mat(1440, 1280, CV_8UC3);
	recvPose = (char*)malloc(sizeof(unsigned char) * 28);

#else
	colorWidth = 640;
	colorHeight = 480;

	depthWidth = 640;
	depthHeight = 480;


	RGBDSensor::init(depthWidth, depthHeight, depthWidth, depthHeight, 1);

	//initializeDepthIntrinsics(518.52905273437500000, 518.34594726562500000, 319.55499267578125000, 239.06936645507812500);
	initializeDepthIntrinsics(554.254691191187, 554.254691191187, 320.5, 240.5);
	initializeDepthExtrinsics(mat4f::identity());

	//initializeColorIntrinsics(518.52905273437500000, 518.34594726562500000, 319.55499267578125000, 239.06936645507812500);
	initializeColorIntrinsics(554.254691191187, 554.254691191187, 320.5, 240.5);
	initializeColorExtrinsics(mat4f::identity());

	bufSize = colorHeight * colorWidth * 2 * 3;
	colorMapUchar = (unsigned char*)malloc(sizeof(unsigned char) * colorWidth * colorHeight * 3);
	depthMapUchar = (unsigned char*)malloc(sizeof(unsigned char) * depthWidth * depthHeight * 3);
	recvImg = (char*)malloc(sizeof(unsigned char) * bufSize);

	recvImgRGB = (uchar*)recvImg;
	recvImgDepth = (uchar*)recvImg + bufSize / 2;

	frameNum = 0;
	//image = cv::Mat(960, 640, CV_8UC3);
	recvPose = (char*)malloc(sizeof(unsigned char) * 28);
#endif // 720P

	start = false;
	distance = 0;
	lastTranslation[0] = 0; lastTranslation[1] = 0; lastTranslation[2] = 0;
}

TCPSensor::~TCPSensor()
{
	closesocket(clientSocket);
	WSACleanup();
	free(colorMapUchar);

	free(depthMapUchar);
	free(recvImg);

}

HRESULT TCPSensor::createFirstConnected()
{
	WSADATA wsaData;
	int iRet = 0;
	iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRet != 0)
	{
		std::cout << "WSAStartup(MAKEWORD(2, 2), &wsaData) execute failed!" << std::endl;
		return S_FALSE;
	}

	if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion))
	{
		WSACleanup();
		std::cout << "WSADATA version is not correct!" << std::endl;
		return S_FALSE;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		std::cout << "clientSocket = socket(AF_INET, SOCK_STREAM, 0) execute failed!" << std::endl;
		return S_FALSE;
	}

	SOCKADDR_IN srvAddr;
	srvAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(8088);

	iRet = connect(clientSocket, (SOCKADDR*)&srvAddr, sizeof(SOCKADDR));
	if (iRet != 0)
	{
		std::cout << "connect(clientSocket, (SOCKADDR*)&srvAddr, sizeof(SOCKADDR)) execute failed!" << std::endl;
		return S_FALSE;
	}
	printf("连接已建立");


	char sendBuf = 0xa5;
	send(clientSocket, &sendBuf, 1, 0);


	return S_OK;
}


HRESULT TCPSensor::processDepth()
{
	int iRet = recv(clientSocket, recvImg, bufSize, 0);
	while (iRet != bufSize)
	{
		iRet += recv(clientSocket, &recvImg[iRet], bufSize - iRet, 0);
	}

#ifdef VINS
	iRet = recv(clientSocket, recvPose, 28, 0);
	while (iRet != 28)
	{
		iRet += recv(clientSocket, &recvPose[iRet], 28 - iRet, 0);
	}
	RT = (float*)recvPose;

	float q0 = RT[3];
	float q1 = RT[0];
	float q2 = RT[1];
	float q3 = RT[2];

	m_rigidTransform._m00 = 1.f - 2.f * q2 * q2 - 2.f * q3 * q3;
	m_rigidTransform._m01 = 2.f * q1 * q2 - 2.f * q0 * q3;
	m_rigidTransform._m02 = 2.f * q1 * q3 + 2.f * q0 * q2;
	m_rigidTransform._m03 = RT[4];

	m_rigidTransform._m10 = 2 * q1 * q2 + 2 * q0 * q3;
	m_rigidTransform._m11 = 1 - 2 * q1 * q1 - 2 * q3 * q3;
	m_rigidTransform._m12 = 2 * q2 * q3 - 2 * q0 * q1;
	m_rigidTransform._m13 = RT[5];

	m_rigidTransform._m20 = 2 * q1 * q3 - 2 * q0 * q2;
	m_rigidTransform._m21 = 2 * q2 * q3 + 2 * q0 * q1;
	m_rigidTransform._m22 = 1 - 2 * q1 * q1 - 2 * q2 * q2;
	m_rigidTransform._m23 = RT[6];

	m_rigidTransform._m30 = 0;
	m_rigidTransform._m31 = 0;
	m_rigidTransform._m32 = 0;
	m_rigidTransform._m33 = 1;

	std::cout << m_rigidTransform << "\n";
#endif // VINS



	float* depth = getDepthFloat();

	//#pragma omp parallel for num_threads(4)
	for (int i = 0; i < depthHeight; i++)
	{
		for (size_t j = 0; j < depthWidth; j++)
		{
			unsigned int index = i * depthWidth + j;
			const unsigned short& d = (recvImgDepth[index * 3 + 0] << 8) | recvImgDepth[index * 3 + 1];

			if (d == 0)
				depth[index] = -std::numeric_limits<float>::infinity();
			else {
				depth[index] = (float)d * 0.001f;
			}

		}
	}

	//#pragma omp parallel for num_threads(4)
	for (int i = 0; i < colorHeight; i++)
	{
		for (size_t j = 0; j < colorWidth; j++)
		{
			unsigned int index = i * colorWidth + j;
			m_colorRGBX[index].z = recvImgRGB[index * 3 + 0];
			m_colorRGBX[index].y = recvImgRGB[index * 3 + 1];
			m_colorRGBX[index].x = recvImgRGB[index * 3 + 2];
			m_colorRGBX[index].w = 255;
		}
	}

	return S_OK;
}

HRESULT TCPSensor::processColor()
{

	return S_OK;
}



void TCPSensor::quaternion2Mat(float* quaternion)
{


}



#endif // TCP_SENSOR


