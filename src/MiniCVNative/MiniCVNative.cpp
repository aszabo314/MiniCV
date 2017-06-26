// MiniCVNative.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "MiniCVNative.h"

using namespace cv;
using namespace std;


//runtime identify cv::Mat type and channel count
static string type2str(int type) {
	string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:  r = "8U"; break;
	case CV_8S:  r = "8S"; break;
	case CV_16U: r = "16U"; break;
	case CV_16S: r = "16S"; break;
	case CV_32S: r = "32S"; break;
	case CV_32F: r = "32F"; break;
	case CV_64F: r = "64F"; break;
	default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	return r;
}


typedef struct {

	double FocalLength;
	Point2d PrincipalPoint;
	double Probability;
	double InlierThreshold;

} RecoverPoseConfig;

DllExport(bool) cvRecoverPoses(const RecoverPoseConfig* config, const int N, const Point2d* pa, const Point2d* pb, Matx33d& rMat1, Matx33d& rMat2, Vec3d& tVec, uint8_t* ms) {
	
	if (N < 5) return false;

	Mat E;
	Mat mask;
	
	Mat aa(N, 1, CV_64FC2, (void*)pa);
	Mat ba(N, 1, CV_64FC2, (void*)pb);

	//printf("%d vs %d (%s vs %s)\n", aa.checkVector(2), ba.checkVector(2), type2str(aa.type()).c_str(), type2str(aa.type()).c_str());

	E = findEssentialMat(aa, ba, config->FocalLength, config->PrincipalPoint, RANSAC, config->Probability, config->InlierThreshold, mask);

	for (int i = 0; i < mask.rows; i++) {
		ms[i] = mask.at<uint8_t>(i);
		//printf("fufu1 %d  ", mask.at<uint8_t>(i));
		//printf("fufu2 %d  \n", ms[i]);
	}
	if (E.rows == 3 && E.cols == 3)
	{
		decomposeEssentialMat(E, rMat1, rMat2, tVec);
		return true;
	}
	else
	{
		return false;
	}
}


DllExport(int) cvRecoverPose(const RecoverPoseConfig* config, const int N, const Point2d* pa, const Point2d* pb, Matx33d& rMat, Vec3d& tVec, uint8_t* ms) {
	vector<Point2d> a(pa, pa + N);
	vector<Point2d> b(pb, pb + N);

	Mat E;
	Mat mask;

	E = findEssentialMat(a, b, config->FocalLength, config->PrincipalPoint, RANSAC, config->Probability, config->InlierThreshold, mask);

	for (int i = 0; i < mask.rows; i++) {
		ms[i] = mask.at<uint8_t>(i);
		//printf("fufu1 %d  ", mask.at<uint8_t>(i));
		//printf("fufu2 %d  \n", ms[i]);
	}

	auto res = recoverPose(E, a, b, rMat, tVec, config->FocalLength, config->PrincipalPoint, mask);

	return res;
}

void cvCornerSubPix(const cv::Mat img, const vector<Vec2d> corners) {
	cornerSubPix(img, corners, Size(11, 11), Size(-1, -1), TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.001));
}

DllExport(void) cvDoStuff(std::string* imgs, int ct, std::string* repr, int rct, std::string* oFilenames) {
	auto op = vector<Vec3d>();
	for (int x = 0; x < 7; x++) {
		for (int y = 0; y < 6; y++) {
			op.push_back(Vec3d(x, y, 0));
		}
	}

	auto size = Size();
	
	auto objectPoints = vector<vector<Vec3d>>();
	auto imagePoints = vector<vector<Vec2d>>();

	printf("Finding calibration target in %d images...\n", ct);
	for (int i = 0; i < ct; i++) {
		printf("attempt %d with name: %s\n", i, imgs[i]);
		auto color = imread(imgs[i]);
		auto img = Mat();
		cvtColor(color, img, COLOR_BGR2GRAY);
		size = Size(img.rows, img.cols);
		auto corners = vector<Vec2d>();

		bool success = findChessboardCorners(img, Size(7, 6), corners);

		if (success) {
			objectPoints.push_back(op);

			cvCornerSubPix(img, corners);

			imagePoints.push_back(corners);
			printf("Found %d target points in img No. %d\n", corners.size(), i);
		}
	}

	printf("Estimating calibration ...\n");
	auto kMat = Matx33d();
	auto dist = vector<double>();
	calibrateCamera(objectPoints, imagePoints, size, kMat, dist, noArray(), noArray());
	printf("fx = %d\n", kMat(0, 0));
	printf("fy = %d\n", kMat(1, 1));
	printf("cx = %d\n", kMat(2, 0));
	printf("cy = %d\n", kMat(2, 1));

	printf("k1 = %d\n", dist.at(0));
	printf("k2 = %d\n", dist.at(1));
	printf("p1 = %d\n", dist.at(2));
	printf("p2 = %d\n", dist.at(3));
	printf("k3 = %d\n", dist.at(4));

	printf("Undistorting %d imgs ...\n", rct);
	for (int i = 0; i < rct; i++) {
		auto img = imread(repr[i]);

		auto dst = Mat();

		undistort(img, dst, kMat, dist);

		printf("Undistorted img No. %d\n", i);

		auto fn = oFilenames[i];
		imwrite(fn, dst);
	}
	printf("finished\n");
}



DllExport(DetectorResult*) cvDetectFeatures(char* data, int width, int height, int channels, int mode = FEATURE_MODE_AKAZE) {

	int fmt;
	switch (channels)
	{
		case 1: 
			fmt = CV_8UC1;
			break;
		case 2:
			fmt = CV_8UC2;
			break;
		case 3:
			fmt = CV_8UC3;
			break;
		case 4:
			fmt = CV_8UC4;
			break;
		default:
			return nullptr;
	}

	Mat input(height, width, fmt, (void*)data);
	cv::Ptr<cv::FeatureDetector> detector;
	switch (mode)
	{
	case FEATURE_MODE_AKAZE:
		detector = cv::AKAZE::create();
		break;
	case FEATURE_MODE_ORB:
		detector = cv::ORB::create();
		break;
	case FEATURE_MODE_BRISK:
		detector = cv::BRISK::create();
		break;
	default:
		return nullptr;
	}

	Mat img;
	cv::cvtColor(input, img, CV_RGB2GRAY);

	vector<KeyPoint> points;
	vector<float> descriptors;
	detector->detectAndCompute(img, noArray(), points, descriptors);

	if (points.size() == 0)
	{
		auto res = new DetectorResult();
		res->PointCount = 0;
		res->DescriptorEntries = 0;
		res->Points = nullptr;
		res->Descriptors = nullptr;
		return res;
	}
	else
	{
		printf("descriptors: %d\n", descriptors.size());
		printf("points: %d\n", points.size());

		auto points1 = new KeyPoint2d[points.size()];
		auto descriptors1 = new float[descriptors.size()];

		for (int i = 0; i < points.size(); i++)
		{
			points1[i].angle = points[i].angle;
			points1[i].class_id = points[i].class_id;
			points1[i].octave = points[i].octave;
			points1[i].pt = points[i].pt;
			points1[i].response = points[i].response;
			points1[i].size = points[i].size;
		}

		std::copy(descriptors.begin(), descriptors.end(), descriptors1);
		//std::copy(descriptors.begin<float>(), descriptors.end<float>(), descriptors1);

		detector->clear();
		img.release();

		auto res = new DetectorResult();
		res->PointCount = (int)points.size();
		res->DescriptorEntries = (int)descriptors.size();
		res->Points = points1;
		res->Descriptors = descriptors1;

		return res;
	}
}

DllExport(void) cvFreeFeatures(DetectorResult* res)
{
	if (!res || !res->PointCount) return;

	if (res->Descriptors)
	{
		delete res->Descriptors;
		res->Descriptors = nullptr;
	}
	if (res->Points)
	{
		delete res->Points;
		res->Points = nullptr;
	}

	delete res;
}