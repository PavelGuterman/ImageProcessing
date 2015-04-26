#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>
#include <iostream>
#include <Windows.h>
#include <windows.h>
#include <string>
#include <fstream>
#include <vector>

#define MAX_KERNEL 11
using namespace std;
using namespace cv;

vector<string> getAllImages(char*);
void Puzzle(vector<string>, char*);
Mat VecHist(Mat);
//Mat Concat(Mat, Mat);



int main(int argc, char *argv[])
{
	vector<string> images;
	printf("%d\n", argc);
	if (argc != 2)
	{
		printf("You need to provide a path to the images location \n");
		exit(1);
	}

	printf("%s\n", argv[1]);
	images = getAllImages(argv[1]);
	Puzzle(images, argv[1]);


	return 0;
}

vector<string>	getAllImages(char* path)
{
	string line;
	vector<string> images;
	int imgSize = 0;
	char *cmd = (char*)malloc(1 + strlen("dir /b ") + strlen(path));
	strcpy(cmd, "dir /b \"");
	strcat(cmd, path);
	strcat(cmd, "\" >test.txt");
	system(cmd);

	ifstream imgList("test.txt");
	if (imgList.is_open())
	{

		while (getline(imgList, line))
		{
			cout << line << '\n';
			images.push_back(line);
			imgSize += 1;

		}
		imgList.close();
	}

	return images;
}


void Puzzle(vector<string> images, char *path)
{
	Mat puzzle;
	vector<Mat> iList;
	vector<Mat> imgEdgeHist;

	//add images to list 
	for (int i = 0; i < images.size(); ++i)
	{
		String strPath(path);
		strPath += "\\";
		strPath += images[i];
		iList.push_back(imread(strPath, 0));
		if (!iList[i].data)
		{
			cout << "Error" + strPath << endl;
			exit(1);
		}
	}


	//creat tmp vector of matrices for horizotal concat
	vector<Mat> hList;
	int lineIndex = 0;
	int imgWidth = iList[0].cols;
	int imgHeight = iList[0].rows;
	hList.push_back(iList[0]);
	iList.erase(iList.begin());

	//Find best to imges to concat by there hist on edges
	while (!iList.empty())
	{
		Mat curImage = hList[lineIndex];
		bool concatRight = false;
		bool concatLeft = false;

		Mat blurImage = curImage.clone();
		for (int i = 1; i < MAX_KERNEL; i = i + 2)
		{

			GaussianBlur(curImage, blurImage, Size(i, i), 0, 0);

		}

		//Get Edge vector of curent Image
		Mat leftVec(blurImage, (Rect(0, 0, 3, imgHeight)));
		Mat rightVec(blurImage, (Rect(imgWidth - 3, 0, 3, imgHeight)));

		//try Detecet surf FEATURES
		/*int minHessian = 400;
		SurfFeatureDetector detector(minHessian);
		vector<KeyPoint> key1, key2;

		detector.detect(leftVec, key1);
		detector.detect(rightVec, key2);

		SurfDescriptorExtractor extractor;
		Mat descriptor1, descriptor2;
		extractor.compute(leftVec, key1, descriptor1);
		extractor.compute(leftVec, key2, descriptor2);
		FlannBasedMatcher matcher;
		vector<DMatch> matches;
		matcher.match(descriptor1, descriptor2, matches);
		double max_dist = 0; double min_dist = 100;
		for (int i = 0; i < descriptor1.rows; i++)
		{
		double dist = matches[i].distance;
		if (dist < min_dist) min_dist = dist;
		if (dist > max_dist) max_dist = dist;
		}
		vector< DMatch > good_matches;

		for (int i = 0; i < descriptor1.rows; i++)
		{
		if (matches[i].distance <= max(2 * min_dist, 0.02))
		{
		good_matches.push_back(matches[i]);
		}
		}
		for (int i = 0; i < (int)good_matches.size(); i++)
		{
		printf("-- Good Match [%d] Keypoint 1: %d  -- Keypoint 2: %d  \n", i, good_matches[i].queryIdx, good_matches[i].trainIdx);
		}*/


		//GET HISTOGRAM OF VECTORS
		Mat leftHistOfCurImg = VecHist(leftVec);
		Mat rightHistOfCurImg = VecHist(rightVec);

		double bestMatch = -1;
		int bestImgFitt = 0;

		//scan hist vectors to find best match
		for (int i = 0; i < iList.size(); ++i)
		{
			Mat potentialImg = iList[i];

			Mat blurPotentialImage = potentialImg.clone();
			for (size_t i = 1; i < MAX_KERNEL; i = i + 2)
			{

				GaussianBlur(potentialImg, blurPotentialImage, Size(i, i), 0, 0);

			}

			//Get Edge vector of potential Image to concat
			Mat leftVecOfPotential(blurPotentialImage, (Rect(0, 0, 3, imgHeight)));
			Mat rightVecOfPotential(blurPotentialImage, (Rect(imgWidth - 3, 0, 3, imgHeight)));

			Mat leftHistOfPotential = VecHist(leftVec);
			Mat rightHistOfPotential = VecHist(rightVec);

			Mat diffRight = rightVec == leftVecOfPotential;
			Mat diffLeft = leftVec == rightVecOfPotential;

			double diffRightSum = sum(diffRight)[0];
			double diffLeftSum = sum(diffLeft)[0];


			//Mark potantial concat of curImage to left of second one
			if (bestMatch <= diffRightSum)
			{

				bestMatch = diffRightSum;
				bestImgFitt = i;
				concatLeft = true;
				concatRight = false;


			}
			if (bestMatch <= diffLeftSum)

			{
				bestMatch = diffLeftSum;
				bestImgFitt = i;
				concatRight = true;
				concatLeft = false;
			}
		}

		//define witch side shold be concat
		if (concatLeft)
		{
			cout << "left" << endl;
			hconcat(hList[lineIndex], iList[bestImgFitt], hList[lineIndex]);
		}
		if (concatRight)
		{
			cout << "right" << endl;
			hconcat(iList[bestImgFitt], hList[lineIndex], hList[lineIndex]);
		}

		iList.erase(iList.begin() + bestImgFitt);

		/*namedWindow("im", WINDOW_AUTOSIZE);
		imshow("im", hList[lineIndex]);
		waitKey(0);*/
		//check if the image line full
		if (hList[lineIndex].cols == floor(sqrt(images.size())) * imgWidth && !iList.empty())
		{
			lineIndex++;
			hList.push_back(iList[0]);
			iList.erase(iList.begin());
		}
	}

	//Horizontal Concat of lines
	Mat fullImage;
	int index = 0;
	int width = hList[0].cols;
	int height = hList[0].rows;
	fullImage.push_back(hList[0]);
	hList.erase(hList.begin());

	while (!hList.empty())
	{
		Mat curImg = fullImage;
		bool concatTop = false;
		bool concatBottom = false;
		double bestMatch = -1;
		int bestImg = 0;

		Mat blurImg = curImg.clone();
		for (int i = 1; i < MAX_KERNEL; i = i + 2)
		{

			GaussianBlur(curImg, blurImg, Size(i, i), 0, 0);

		}

		Mat topVec(blurImg, (Rect(0, 0, imgWidth, 3)));
		Mat bottomVec(blurImg, (Rect(0, imgHeight - 3, imgWidth, 3)));

		Mat topHist = VecHist(topVec);
		Mat bottomHist = VecHist(bottomVec);

		for (int i = 0; i < hList.size(); ++i)
		{
			Mat potentialImg = hList[i];

			Mat blurpotentialImage = potentialImg.clone();
			for (int i = 1; i < MAX_KERNEL; i = i + 2)
			{

				GaussianBlur(potentialImg, blurpotentialImage, Size(i, i), 0, 0);

			}

			Mat topVecOfPotential(blurpotentialImage, (Rect(0, 0, imgWidth, 3)));
			Mat bottomVecOfPotential(blurpotentialImage, (Rect(0, imgHeight - 3, imgWidth, 3)));

			Mat topHistOfPotential = VecHist(topVecOfPotential);
			Mat bottomHistOfPotential = VecHist(bottomVecOfPotential);

			Mat diffTop = topVec == bottomVecOfPotential;
			Mat diffBottom = bottomVec == topVecOfPotential;

			double diffTopSum = sum(diffTop)[0];
			double diffBottomSum = sum(diffBottom)[0];

			//Mark potantial concat of curImage to bottom of second one
			if (bestMatch <= diffTopSum)
			{
				bestMatch = diffTopSum;
				bestImg = i;
				concatBottom = true;
				concatTop = false;
			}

			//Mark potantial concat of curImage to top of second one
			if (bestMatch <= diffBottomSum)
			{
				bestMatch = diffBottomSum;
				bestImg = i;
				concatTop = true;
				concatBottom = false;
			}

			if (concatTop)
			{
				vconcat(fullImage, hList[bestImg], fullImage);
			}

			if (concatBottom)
			{
				vconcat(hList[bestImg], fullImage, fullImage);
			}


			hList.erase(hList.begin() + bestImg);
		}

	}
	namedWindow("im", WINDOW_AUTOSIZE);
	imshow("im", fullImage);
	waitKey(0);

}


Mat VecHist(Mat vec)
{
	Mat v_Hist = Mat::zeros(1, 256, CV_64F);
	for (int j = 0; j <= vec.rows*vec.cols - 1; ++j)
	{
		v_Hist.at<double>((int)vec.at<uchar>(j)) += 1;
	}

	return v_Hist;
}


