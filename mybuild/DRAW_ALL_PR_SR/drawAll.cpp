#include <opencv2/core/utility.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include<vector>
#include<algorithm>
using namespace std;
using namespace cv;

int Threshold = 100, axis_height = 512, axis_width = 512, height_shift = 100, width_shift = 100;//坐标框体大小


vector<float> dist;
vector<double> overlap;

template <class Type>
Type stringToNum(const string& str)
{
	istringstream iss(str);
	Type num;
	iss >> num;
	return num;
}

Point getCenterPoint(Rect rect)
{
	Point cpt;
	cpt.x = rect.x + cvRound(rect.width / 2.0);
	cpt.y = rect.y + cvRound(rect.height / 2.0);

	return cpt;
}

float getDistance(Point pointO, Point pointA)
{
	double distance;
	distance = powf((pointO.x - pointA.x), 2) + powf((pointO.y - pointA.y), 2);
	distance = sqrtf(distance);
	return distance;
}


void culPrecision(Rect2d Rgt, Rect2d Rresult)
{

	Rect2d intersection = Rgt & Rresult;

	float percent = intersection.area() / (Rgt.area() + Rresult.area() - intersection.area());
	overlap.push_back(percent);
	dist.push_back(getDistance(getCenterPoint(Rgt), getCenterPoint(Rresult)));
}

int main()
{
	string listPath = "G:/otb/list.txt";
	String resultRootPath = "G:/otb/";
	String gtRootPath = "G:/otb/";
	String drawRootPath = "G:/otb/_drawing/";

	fstream listFile;
	listFile.open(listPath, ios::in);

	vector<Point> drawPoint_PR(50);
	vector<Point> drawPoint_SR(100);

	int listLength = 0;

	while (!listFile.eof())
	{
		listLength++;

		string sequenceName;
		getline(listFile, sequenceName);
		cout << sequenceName << endl;

		string resultPath = resultRootPath + sequenceName + "/_result.txt";
		cout << resultPath << endl;
		string gtPath = gtRootPath + sequenceName + "/groundtruth_rect.txt";
		cout << gtPath << endl;

		fstream resultFile;
		resultFile.open(resultPath, ios::in);
		if (!resultFile)
		{
			cout << "error: could not open sequence result file: " << resultPath << endl;
			system("pause");
		}

		fstream gtFile;
		gtFile.open(gtPath, ios::in);
		if (!gtFile)
		{
			cout << "error: could not open sequence gt file: " << gtPath << endl;
			system("pause");
		}


		while (!resultFile.eof() && !gtFile.eof())
		{
			int counter = 1;
			string result;
			string gt;

			getline(resultFile, result);
			getline(gtFile, gt);

			float resultBuf[4];
			float gtBuf[4];

			sscanf_s(result.c_str(), "%f,%f,%f,%f", &resultBuf[0], &resultBuf[1], &resultBuf[2], &resultBuf[3]);
			//cout << resultBuf[0] << "  ";

			//OTB的gt数据处理
			sscanf_s(gt.c_str(), "%f,%f,%f,%f", &gtBuf[0], &gtBuf[1], &gtBuf[2], &gtBuf[3]);

			//vot的gt数据处理
			/*float x1 = -1.f, x2 = -1.f, x3 = -1.f, x4 = -1.f;
			float y1 = -1.f, y2 = -1.f, y3 = -1.f, y4 = -1.f;
			sscanf_s(gt.c_str(), "%f,%f,%f,%f,%f,%f,%f,%f", &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4);
			gtBuf[0] = min(min(x1, x2), min(x3, x4));
			gtBuf[1] = min(min(y1, y2), min(y3, y4));
			float xmax = max(max(x1, x2), max(x3, x4));
			float ymax = max(max(y1, y2), max(y3, y4));
			gtBuf[2] = xmax - gtBuf[0];
			gtBuf[3] = ymax - gtBuf[1];*/

			//cout << gtBuf[0];

			Rect2d Rresult = Rect2d(result[0], result[1], result[2], result[3]);
			Rect2d Rgt = Rect2d(gt[0], gt[1], gt[2], gt[3]);
			culPrecision(Rgt, Rresult);

			counter++;
		}

		//计算Precision rate
		float PreRate[51];
		for (int i = 0; i <= 50; i++)
		{
			float ctr = 0;
			for (int j = 0; j < dist.size(); j++)
			{
				if (dist[j] >= i)
					ctr++;
			}
			PreRate[i] = ctr / dist.size();
			//			cout << PreRate[i] << endl;
		}

		//计算Success Rate
		float SucRate[101];
		for (int i = 0; i <= 100; i++)
		{
			float ctr = 0;
			for (int j = 0; j < overlap.size(); j++)
			{
				float tmp = i;
				if (overlap[j] <= tmp / 100)
					ctr++;
			}
			SucRate[i] = ctr / overlap.size();
		}

		//Precision映射

		for (int i = 0; i <= 50; i++)
		{
			Point temp = Point(axis_width * i / 50 + width_shift, axis_height*PreRate[i] + height_shift);
			drawPoint_PR[i] += temp;
		}

		//success映射

		for (int i = 0; i <= 100; i++)
		{
			Point temp = Point(axis_width * i /100 + width_shift, axis_height*SucRate[i] + height_shift);
			drawPoint_SR[i] += temp;
		}

	}


	//连线
	Mat img(axis_width + width_shift * 2, axis_height + height_shift * 2, CV_8UC3, Scalar(255, 255, 255));//precision图
	Mat img2(axis_width + width_shift * 2, axis_height + height_shift * 2, CV_8UC3, Scalar(255, 255, 255));//success图
	for (int i = 0; i < drawPoint_PR.size() - 1; i++)
	{
		line(img, drawPoint_PR[i] / listLength, drawPoint_PR[i + 1] / listLength, Scalar(0, 0, 255), 2, 8);//连线
	}
	for (int i = 0; i < drawPoint_SR.size() - 1; i++)
	{
		line(img2, drawPoint_SR[i] / listLength, drawPoint_SR[i + 1] / listLength, Scalar(0, 0, 255), 2, 8);//连线
	}

	//坐标轴
	line(img, Point(width_shift, height_shift), Point(width_shift, height_shift + axis_height), Scalar(0, 0, 0), 2, 8);
	line(img, Point(width_shift, height_shift + axis_height), Point(width_shift + axis_width, height_shift + axis_height), Scalar(0, 0, 0), 2, 8);

	line(img2, Point(width_shift, height_shift), Point(width_shift, height_shift + axis_height), Scalar(0, 0, 0), 2, 8);
	line(img2, Point(width_shift, height_shift + axis_height), Point(width_shift + axis_width, height_shift + axis_height), Scalar(0, 0, 0), 2, 8);

	//坐标轴数值
	//precision横坐标
	for (float i = 0; i <= 5; i++)
	{
		string str = to_string((int)(i * 10));
		line(img, Point(width_shift + i / 5 * axis_width, height_shift + axis_height), Point(width_shift + i / 5 * axis_width, height_shift + axis_height * 0.98), Scalar(0, 0, 0), 2, 8);
		putText(img, str, Point(width_shift + axis_width * i / 5, height_shift * 5 / 4 + axis_height), 1, 1, Scalar(0, 0, 0));
	}
	//precision纵坐标
	for (float i = 0; i <= 10; i++)
	{
		string str = to_string((int)((10 - i) * 10)) + "%";
		line(img, Point(width_shift, height_shift + axis_height * i / 10), Point(width_shift + 0.02 * axis_width, height_shift + axis_height * i / 10), Scalar(0, 0, 0), 2, 8);
		putText(img, str, Point(width_shift*0.65, height_shift + axis_height * i / 10), 1, 1, Scalar(0, 0, 0));
	}
	//success横坐标
	for (float i = 0; i <= 10; i++)
	{
		string str = to_string((int)(i * 10)) + "%";
		line(img2, Point(width_shift + i / 10 * axis_width, height_shift + axis_height), Point(width_shift + i / 10 * axis_width, height_shift + axis_height * 0.98), Scalar(0, 0, 0), 2, 8);
		putText(img2, str, Point(width_shift + axis_width * i / 10, height_shift * 5 / 4 + axis_height), 1, 1, Scalar(0, 0, 0));
	}
	//success纵坐标
	for (float i = 0; i <= 10; i++)
	{
		string str = to_string((int)((10 - i) * 10)) + "%";
		line(img2, Point(width_shift, height_shift + axis_height * i / 10), Point(width_shift + 0.02 * axis_width, height_shift + axis_height * i / 10), Scalar(0, 0, 0), 2, 8);
		putText(img2, str, Point(width_shift*0.65, height_shift + axis_height * i / 10), 1, 1, Scalar(0, 0, 0));
	}

	putText(img, " precision rate", Point(axis_width*0.5, 1.5*height_shift + axis_height), 0, 1, Scalar(0, 0, 0));
	putText(img2, " success rate", Point(axis_width*0.5, 1.5*height_shift + axis_height), 0, 1, Scalar(0, 0, 0));
	//imshow("precision", img);
	//imshow("success", img2);
	imwrite(drawRootPath + "precision.jpg", img);
	imwrite(drawRootPath + "success.jpg", img2);

	waitKey(300);

	system("pause");
	return 0;

}