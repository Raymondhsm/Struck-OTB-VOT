/*
 * Struck: Structured Output Tracking with Kernels
 *
 * Code to accompany the paper:
 *   Struck: Structured Output Tracking with Kernels
 *   Sam Hare, Amir Saffari, Philip H. S. Torr
 *   International Conference on Computer Vision (ICCV), 2011
 *
 * Copyright (C) 2011 Sam Hare, Oxford Brookes University, Oxford, UK
 *
 * This file is part of Struck.
 *
 * Struck is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Struck is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Struck.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "Tracker.h"
#include "Config.h"

#include <iostream>
#include <fstream>
#include <cmath>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "vot.hpp"

using namespace std;
using namespace cv;

static const int kLiveBoxWidth = 80;
static const int kLiveBoxHeight = 80;

void rectangle(Mat& rMat, const FloatRect& rRect, const Scalar& rColour)
{
	IntRect r(rRect);
	rectangle(rMat, Point(r.XMin(), r.YMin()), Point(r.XMax(), r.YMax()), rColour);
}

int main(int argc, char* argv[])
{
	// read list file
	string sequenceName;
	string listPath = "E:/sequences/list.txt";
	fstream f;
	f.open(listPath, ios::in);
	if (!f.is_open()) {
		cout << "error: could not open list file: " << listPath << endl;
	}

	// loop the list file
	while (!f.eof())
	{
		getline(f, sequenceName);

		// read config file
		string configPath = "config.txt";
		if (argc > 1)
		{
			configPath = argv[1];
		}
		Config conf(configPath);
		conf.sequenceName = sequenceName;
		//		cout << conf << endl;

		if (conf.features.size() == 0)
		{
			cout << "error: no features specified in config" << endl;
			system("pause");
			return EXIT_FAILURE;
		}

		Tracker tracker(conf);

		//Check if --challenge was passed as an argument
		bool challengeMode = false;
		for (int i = 1; i < argc; i++) {
			if (strcmp("--challenge", argv[i]) == 0) {
				challengeMode = true;
			}
		}

		if (challengeMode) {
			//load region, images and prepare for output
			Mat frameOrig;
			Mat frame;
			VOT vot_io("region.txt", "images.txt", "output.txt");
			vot_io.getNextImage(frameOrig);
			resize(frameOrig, frame, Size(conf.frameWidth, conf.frameHeight));
			cv::Rect initPos = vot_io.getInitRectangle();
			vot_io.outputBoundingBox(initPos);
			float scaleW = (float)conf.frameWidth / frameOrig.cols;
			float scaleH = (float)conf.frameHeight / frameOrig.rows;

			FloatRect initBB_vot = FloatRect(initPos.x*scaleW, initPos.y*scaleH, initPos.width*scaleW, initPos.height*scaleH);
			tracker.Initialise(frame, initBB_vot);

			while (vot_io.getNextImage(frameOrig) == 1) {
				resize(frameOrig, frame, Size(conf.frameWidth, conf.frameHeight));

				tracker.Track(frame);
				const FloatRect& bb = tracker.GetBB();
				float x = bb.XMin() / scaleW;
				float y = bb.YMin() / scaleH;
				float w = bb.Width() / scaleW;
				float h = bb.Height() / scaleH;

				cv::Rect output = cv::Rect(x, y, w, h);

				vot_io.outputBoundingBox(output);
			}

			return 0;
		}

		ofstream outFile;
		if (conf.resultsPath != "")
		{
			conf.resultsPath = conf.resultsPath +  "/" + conf.sequenceName + "_result.txt";
			outFile.open(conf.resultsPath.c_str(), ios::out);
			if (!outFile)
			{
				cout << "error: could not open results file: " << conf.resultsPath << endl;
				system("pause");
				return EXIT_FAILURE;
			}
		}

		cout << conf << endl;

		// if no sequence specified then use the camera
		bool useCamera = (conf.sequenceName == "");

		VideoCapture cap;

		int startFrame = -1;
		int endFrame = -1;
		FloatRect initBB;
		string imgFormat;
		float scaleW = 1.f;
		float scaleH = 1.f;

		if (useCamera)
		{
			if (!cap.open(0))
			{
				cout << "error: could not start camera capture" << endl;
				system("pause");
				return EXIT_FAILURE;
			}
			startFrame = 0;
			endFrame = INT_MAX;
			Mat tmp;
			cap >> tmp;
			scaleW = (float)conf.frameWidth / tmp.cols;
			scaleH = (float)conf.frameHeight / tmp.rows;

			initBB = IntRect(conf.frameWidth / 2 - kLiveBoxWidth / 2, conf.frameHeight / 2 - kLiveBoxHeight / 2, kLiveBoxWidth, kLiveBoxHeight);
			cout << "press 'i' to initialise tracker" << endl;
		}
		else
		{
			// parse frames file
			string framesFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "/" + "frame.txt";
			ifstream framesFile(framesFilePath.c_str(), ios::in);
			if (!framesFile)
			{
				cout << "error: could not open sequence frames file: " << framesFilePath << endl;
				system("pause");
				return EXIT_FAILURE;
			}
			string framesLine;
			getline(framesFile, framesLine);
			sscanf_s(framesLine.c_str(), "%d,%d", &startFrame, &endFrame);
			if (framesFile.fail() || startFrame == -1 || endFrame == -1)
			{
				cout << "error: could not parse sequence frames file" << endl;
				system("pause");
				return EXIT_FAILURE;
			}

			imgFormat = conf.sequenceBasePath + "/" + conf.sequenceName + "/color/%08d.jpg";

			// read first frame to get size
			char imgPath[256];
			sprintf_s(imgPath, imgFormat.c_str(), startFrame);
			Mat tmp = cv::imread(imgPath, 0);
			scaleW = (float)conf.frameWidth / tmp.cols;
			scaleH = (float)conf.frameHeight / tmp.rows;

			// read init box from ground truth file
			//string gtFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "/" + "groundtruth_rect.txt";

			//vot°æ±¾
			string gtFilePath = conf.sequenceBasePath + "/" + conf.sequenceName + "/" + "groundtruth.txt";

			ifstream gtFile(gtFilePath.c_str(), ios::in);
			if (!gtFile)
			{
				cout << "error: could not open sequence gt file: " << gtFilePath << endl;
				system("pause");
				return EXIT_FAILURE;
			}
			string gtLine;
			getline(gtFile, gtLine);
			float xmin = -1.f;
			float ymin = -1.f;
			float width = -1.f;
			float height = -1.f;
			//sscanf_s(gtLine.c_str(), "%f,%f,%f,%f", &xmin, &ymin, &width, &height);*/

			//votÌØÊâgt¸Ä
			float x1 = -1.f, x2 = -1.f, x3 = -1.f, x4 = -1.f;
			float y1 = -1.f, y2 = -1.f, y3 = -1.f, y4 = -1.f;
			sscanf_s(gtLine.c_str(), "%f,%f,%f,%f,%f,%f,%f,%f", &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4);
			xmin = min(min(x1, x2), min(x3, x4));
			ymin = min(min(y1, y2), min(y3, y4));
			float xmax = max(max(x1, x2), max(x3, x4));
			float ymax = max(max(y1, y2), max(y3, y4));
			width = xmax - xmin;
			height = ymax - ymin;

			if (gtFile.fail() || xmin < 0.f || ymin < 0.f || width < 0.f || height < 0.f)
			{
				cout << "error: could not parse sequence gt file" << endl;
				system("pause");
				return EXIT_FAILURE;
			}
			initBB = FloatRect(xmin*scaleW, ymin*scaleH, width*scaleW, height*scaleH);
		}



		if (!conf.quietMode)
		{
			namedWindow("result");
		}

		Mat result(conf.frameHeight, conf.frameWidth, CV_8UC3);
		bool paused = false;
		bool doInitialise = false;
		srand(conf.seed);
		for (int frameInd = startFrame; frameInd <= endFrame; ++frameInd)
		{
			Mat frame;
			if (useCamera)
			{
				Mat frameOrig;
				cap >> frameOrig;
				resize(frameOrig, frame, Size(conf.frameWidth, conf.frameHeight));
				flip(frame, frame, 1);
				frame.copyTo(result);
				if (doInitialise)
				{
					if (tracker.IsInitialised())
					{
						tracker.Reset();
					}
					else
					{
						tracker.Initialise(frame, initBB);
					}
					doInitialise = false;
				}
				else if (!tracker.IsInitialised())
				{
					rectangle(result, initBB, CV_RGB(255, 255, 255));
				}
			}
			else
			{
				char imgPath[256];
				sprintf_s(imgPath, imgFormat.c_str(), frameInd);
				Mat frameOrig = cv::imread(imgPath, 0);
				if (frameOrig.empty())
				{
					cout << "error: could not read frame: " << imgPath << endl;
					system("pause");
					return EXIT_FAILURE;
				}
				resize(frameOrig, frame, Size(conf.frameWidth, conf.frameHeight));
				cvtColor(frame, result, CV_GRAY2RGB);

				if (frameInd == startFrame)
				{
					tracker.Initialise(frame, initBB);
				}
			}

			if (tracker.IsInitialised())
			{
				tracker.Track(frame);

				if (!conf.quietMode && conf.debugMode)
				{
					tracker.Debug();
				}

				rectangle(result, tracker.GetBB(), CV_RGB(0, 255, 0));

				if (outFile)
				{
					const FloatRect& bb = tracker.GetBB();
					outFile << bb.XMin() / scaleW << "," << bb.YMin() / scaleH << "," << bb.Width() / scaleW << "," << bb.Height() / scaleH << endl;
				}
			}

			if (!conf.quietMode)
			{
				imshow("result", result);
				int key = waitKey(paused ? 0 : 1);
				if (key != -1)
				{
					if (key == 27 || key == 113) // esc q
					{
						break;
					}
					else if (key == 112) // p
					{
						paused = !paused;
					}
					else if (key == 105 && useCamera)
					{
						doInitialise = true;
					}
				}
				if (conf.debugMode && frameInd == endFrame)
				{
					cout << "\n\nend of sequence" << endl;
					//					waitKey();
				}
			}
		}

		if (outFile.is_open())
		{
			outFile.close();
		}
	
	}
	system("pause");
	return EXIT_SUCCESS;
}
