# Struck-OTB-VOT
基于struck的github代码改版，用于测试OTB以及VOT数据集，并增加Precision Rate和Success Rate的曲线绘制。

#### 环境配置
* 项目运行需要openCV和Eigen 3的库支持。我自己跑得时候使用的是openCV 2.4.13和Eigen 3的版本，其他版本没试过，可以试一试。
* 使用vs打开mybuild/STRUCK.sln

#### 项目内路径说明
* 跑VOT和OTB数据集是需要修改项目内main.cpp中56行的listPath的路径，就是数据集中list.txt的路径，list.txt中包含数据集各个数据的文件名列表。
* 另外数据集内每个数据文件夹下需包含frames.txt，主要内容为该数据的开始帧和结束帧（如，开始为1，结束为60的frame.txt内容为 1,60）
* main.cpp中194 行需要修改数据集中的图片的路径
* 207行需要修改groundtruth的文件名
* config.txt文件中修改sequenceBasePath 为你的数据集根目录，resultsPath = 为你的输出结果目录

#### 输入输出结果说明
* groundtruth文件的规范为矩形的左上角坐标以及矩形的长宽，共四个float值
* 结果输出规范为矩形的左上角坐标以及矩形的长宽，共四个float值
* 由于VOTgroundtruth的规范为四边形的四个顶点的坐标，共8个float值，本项目中使用gt中的四边形的外接矩形作为输入数据处理。

#### 绘图说明
* DRAW_PR_SR项目可以绘制数据集每一个模块的数据的precision Rate和Success Rate
* DRAW_ALL_PR_SR项目可以绘制整个数据集的平均PR和SR曲线图
* string listPath = list.txt的路径
* String resultRootPath = result文件夹的路径
* String gtRootPath = gt文件的路径
* String drawRootPath = 绘图保存的根路径
