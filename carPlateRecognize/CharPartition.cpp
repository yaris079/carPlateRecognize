#include <QMessageBox>
#include <qdebug.h>
#include "CharPartition.h"

#pragma execution_character_set("utf-8")

CharPartition::CharPartition()
{

}

IplImage **CharPartition::partChar(IplImage *srcImage)
{
    IplImage *pImgSrc = NULL;
    IplImage *pImgResize = NULL;        //归一化为高90，宽409的图像

    pImgSrc = srcImage;
    if (!pImgSrc) {
        QMessageBox::warning(NULL, "警告", "你的车牌定位的源图片有问题");
        return NULL;
    }

    int nWidth = 409, nHeight = 90;

    pImgResize = cvCreateImage(cvSize(nWidth, nHeight), IPL_DEPTH_8U, 1);//归一化

    // 其实就是将图像缩放成409*90
    cvResize(pImgSrc, pImgResize, CV_INTER_LINEAR); //线性插值

    // WIDTH 34 分隔符区域宽度
    // 409 整个车牌的宽度
    int l_sum[409] = { 0 }, modle[WIDTH] = { 0 };
    int sum = 0, i = 0, j = 0;
    int begin = 0, end = 0;//分隔符开始与结束位置

    // 模板的值为(3, 3, 15, 15， ... 15, 15, 3, 3)
    modle[0] = MOD_MIN * 255;
    modle[1] = MOD_MIN * 255;
    modle[WIDTH - 1] = MOD_MIN * 255;
    modle[WIDTH - 2] = MOD_MIN * 255;

    for (i = 2; i<WIDTH - 2; i++)
        modle[i] = MOD_MAX * 255; // 模板除了首尾都是15

    for (i = 0; i<pImgResize->width; i++) {
        sum = 0;
        for (j = 0; j<pImgResize->height; j++) {
            sum = S(pImgResize, i, j) + sum;//灰度之和
        }
        l_sum[i] = sum;
    }

	int l_max_value = l_sum[0];//寻找最大峰值
	for (j = 0; j<pImgResize->width - 1; j++)
		if (l_max_value<l_sum[j]) {
			l_max_value = l_sum[j];
		}
	//画出垂直投影图像
	CvScalar s;
	IplImage* pImgCol = cvCreateImage(cvSize(pImgResize->width * 2, pImgResize->height), 8, 3);
	//初始化背景颜色
	for (int i = 0; i < pImgResize->height; i++) // 图像高度
	{
		for (int j = 0; j < pImgResize->width * 2; j++) // 图像宽度
		{
			s.val[0] = 0; // B
			s.val[1] = 0; // G
			s.val[2] = 0; // R
			cvSet2D(pImgCol, i, j, s); //图像赋值
		}
	}
	for (j = 0; j < pImgResize->width; j++) {
		cvRectangle(pImgCol,
			cvPoint(j * 2, pImgResize->height),
			cvPoint(j * 2 + 1, pImgResize->height - (pImgResize->height*l_sum[j] / l_max_value)),
			CV_RGB(255, 255, 255));
	}

    // 查找分隔符区域
    bool ret = findSeparator(modle, l_sum, &begin, &end);
    if (!ret) {
        QMessageBox::warning(NULL, "警告", "未找到分隔符，请选择其他图片");
        return NULL;
    }

    int min_value = 0, min_col = 0;//最小值及其所在列，用于寻找字符之间的空隙
    int oneright = 0, oneleft = 0, twoleft = 0, threeright = 0, fourleft = 0, fourright = 0, fiveleft = 0, fiveright = 0, sixleft = 0, sixright = 0, sevenleft = 0, sevenright = 0;
	
	//**归一化后pImgResize->height=90**
    i = begin - (int)((45 / 90.0)*pImgResize->height) - 1; // 第二个字符的左边界
    min_value = l_sum[i];
    min_col = i;

    // 在第一个字符与第二个字符之间的空隙去搜索
    for (; i >= begin - (int)((57 / 90.0)*pImgResize->height) - 1; i--) {
        if (min_value>l_sum[i]) {
            min_value = l_sum[i];
            min_col = i;
        }
    }

    // 从最小值往右搜索第二个字符的左边界
    for (i = min_col; i<min_col + (int)((12 / 90.0)*pImgResize->height); i++) {
        if (l_sum[i] > 3 * 255) {
            twoleft = i;
            break;
        }
    }

    if (i >= min_col + (int)((12 / 90.0)*pImgResize->height)) {
        twoleft = begin - (int)((51 / 90.0)*pImgResize->height);
    }

    // 从最小值往左搜索第一个字符的右边界，搜索12个单位
    for (i = min_col; i>min_col - (int)((12 / 90.0)*pImgResize->height); i--) {
        if (l_sum[i]>3 * 255) {
            oneright = i; break;
        }
    }
	//未找到明确边界，指定
    if (i <= min_col - (int)((12 / 90.0)*pImgResize->height)) {
        oneright = begin - (int)((51 / 90.0)*pImgResize->height);
    }

    // 预留宽度5 用来搜索第一个字符左边界
    i = oneright - (int)((40 / 90.0)*pImgResize->height) - 1;
    min_value = l_sum[i];
    min_col = i;

    for (; i >= oneright - (int)((52 / 90.0)*pImgResize->height) - 1; i--) {
        if (min_value>l_sum[i]) {
            min_value = l_sum[i];
            min_col = i;
        }
    }

    // 向右搜索第一个字符左边界
    for (i = min_col; i<min_col + (int)((12 / 90.0)*pImgResize->height); i++) {
        if (l_sum[i]>3 * 255) {
            oneleft = i;
            break;
        }
    }

    if (i >= min_col + (int)((12 / 90.0)*pImgResize->height)) {
        oneleft = oneright - (int)((51 / 90.0)*pImgResize->height);
    }

    if (oneleft<0) {
        oneleft = 0;
    }

    i = end + (int)((45 / 90.0)*pImgResize->height) + 1;//第三个字符右边界
    min_value = l_sum[i];
    min_col = i;
	// 在第三个字符与第四个字符之间的空隙去搜索
    for (; i <= end + (int)((57 / 90.0)*pImgResize->height); i++) {
        if (min_value>l_sum[i]) {
            min_value = l_sum[i];
            min_col = i;
        }
    }

    // 搜索第三个字符右边界
    for (i = min_col; i>min_col - (int)((12 / 90.0)*pImgResize->height); i--) {
        if (l_sum[i]>3 * 255) {
            threeright = i;
            break;
        }
    }
    if (i <= min_col - (int)((12 / 90.0)*pImgResize->height)) {
        threeright = end + (51 / 90)*pImgResize->height;
    }

    // 第四个字符左边界
    for (i = min_col; i<min_col + (int)((12 / 90.0)*pImgResize->height); i++) {
        if (l_sum[i]>3 * 255) {
            fourleft = i;
            break;
        }
    }
    if (i >= min_col + (int)((12 / 90.0)*pImgResize->height)) {
        fourleft = end + (51 / 90)*pImgResize->height;
    }
	
    i = fourleft + (int)((45 / 90.0)*pImgResize->height) + 1;
    min_value = l_sum[i];
    min_col = i;
    for (; i <= fourleft + (int)((57 / 90.0)*pImgResize->height); i++) {
        if (min_value>l_sum[i]) {
            min_value = l_sum[i];
            min_col = i;
        }
    }
	// 第四个字符右边界
    for (i = min_col; i>min_col - (int)((12 / 90.0)*pImgResize->height); i--) {
        if (l_sum[i]>3 * 255) {
            fourright = i; break;
        }
    }
    if (i <= min_col - (int)((12 / 90.0)*pImgResize->height)) {
        fourright = fourleft + (int)((51 / 90.0)*pImgResize->height);
    }
	// 第五个字符左边界
    for (i = min_col; i<min_col + (int)((12 / 90.0)*pImgResize->height); i++) {
        if (l_sum[i]>3 * 255) {
            fiveleft = i; break;
        }
    }
    if (i >= min_col + (int)((12 / 90.0)*pImgResize->height)) {
        fiveleft = fourleft + (int)((51 / 90.0)*pImgResize->height);
    }

    i = fiveleft + (int)((45 / 90.0)*pImgResize->height) + 1;
    min_value = l_sum[i];
    min_col = i;
    for (; i <= fiveleft + (int)((57 / 90.0)*pImgResize->height); i++) {
        if (min_value>l_sum[i]) {
            min_value = l_sum[i];
            min_col = i;
        }
    }
	// 第五个字符右边界
    for (i = min_col; i>min_col - (int)((12 / 90.0)*pImgResize->height); i--) {
        if (l_sum[i]>3 * 255) {
            fiveright = i; break;
        }
    }
    if (i <= min_col - (int)((12 / 90.0)*pImgResize->height)) {
        fiveright = fiveleft + (int)((51 / 90.0)*pImgResize->height);
    }

	// 第六个字符左边界
    for (i = min_col; i<min_col + (int)((12 / 90.0)*pImgResize->height); i++) {
        if (l_sum[i]>3 * 255) {
            sixleft = i;
            break;
        }
    }
    if (i >= min_col + (int)((12 / 90.0)*pImgResize->height)) {
        sixleft = fiveleft + (int)((51 / 90.0)*pImgResize->height);
    }

    i = sixleft + (int)((45 / 90.0)*pImgResize->height) + 1;
    min_value = l_sum[i];
    min_col = i;
    for (; i <= sixleft + (int)((57 / 90.0)*pImgResize->height); i++) {
        if (min_value>l_sum[i]) {
            min_value = l_sum[i];
            min_col = i;
        }
    }
	// 第六个字符右边界
    for (i = min_col; i>min_col - (int)((12 / 90.0)*pImgResize->height); i--) {
        if (l_sum[i]>3 * 255) {
            sixright = i;
            break;
        }
    }
    if (i <= min_col - (int)((12 / 90.0)*pImgResize->height)) {
        sixright = sixleft + (int)((51 / 90.0)*pImgResize->height);
    }

	// 第七个字符左边界
    for (i = min_col; i<min_col + (int)((12 / 90.0)*pImgResize->height); i++) {
        if (l_sum[i]>3 * 255) {
            sevenleft = i; break;
        }
    }
    if (i >= min_col + (int)((12 / 90.0)*pImgResize->height)) {
        sevenleft = sixleft + (int)((51 / 90.0)*pImgResize->height);
    }
    i = sevenleft + (int)((45 / 90.0)*pImgResize->height) + 1;
    min_value = l_sum[i];
    min_col = i;
    for (; (i <= sevenleft + (int)((57 / 90.0)*pImgResize->height)) && i<pImgResize->width; i++) {
        if (min_value>l_sum[i]) {
            min_value = l_sum[i];
            min_col = i;
        }
    }
	// 第七个字符右边界
    for (i = min_col; i>min_col - (int)((12 / 90.0)*pImgResize->height); i--) {
        if (l_sum[i]>3 * 255) {
            sevenright = i;
            break;
        }
    }
    if (i <= min_col - (int)((12 / 90.0)*pImgResize->height)) {
        sevenright = sevenleft + (int)((51 / 90.0)*pImgResize->height);
    }
    if (sevenright>pImgResize->width - 1) {
        sevenright = pImgResize->width - 1;
    }

    static IplImage *pImgChar[7] = {NULL};

    pImgChar[0] = cvCreateImage(cvSize(oneright - oneleft + 1, nHeight), IPL_DEPTH_8U, 1);
    pImgChar[1] = cvCreateImage(cvSize(begin - twoleft + 2, nHeight), IPL_DEPTH_8U, 1);
    pImgChar[2] = cvCreateImage(cvSize(threeright - end + 2, nHeight), IPL_DEPTH_8U, 1);
    pImgChar[3] = cvCreateImage(cvSize(fourright - fourleft + 2, nHeight), IPL_DEPTH_8U, 1);
    pImgChar[4] = cvCreateImage(cvSize(fiveright - fiveleft + 2, nHeight), IPL_DEPTH_8U, 1);
    pImgChar[5] = cvCreateImage(cvSize(sixright - sixleft + 2, nHeight), IPL_DEPTH_8U, 1);
    pImgChar[6] = cvCreateImage(cvSize(sevenright - sevenleft + 1, nHeight), IPL_DEPTH_8U, 1);

    CvRect ROI_rect1;
    ROI_rect1.x = oneleft;
    ROI_rect1.y = 0;
    ROI_rect1.width = oneright - oneleft + 1;
    ROI_rect1.height = pImgResize->height;
    cvSetImageROI(pImgResize, ROI_rect1);
    cvCopy(pImgResize, pImgChar[0], NULL); //获取第1个字符
    cvResetImageROI(pImgResize);

    ROI_rect1.x = twoleft - 1;
    ROI_rect1.y = 0;
    ROI_rect1.width = begin - twoleft + 2;
    ROI_rect1.height = pImgResize->height;
    cvSetImageROI(pImgResize, ROI_rect1);
    cvCopy(pImgResize, pImgChar[1], NULL); //获取第2个字符
    cvResetImageROI(pImgResize);

    ROI_rect1.x = end - 1;
    ROI_rect1.y = 0;
    ROI_rect1.width = threeright - end + 2;
    ROI_rect1.height = pImgResize->height;
    cvSetImageROI(pImgResize, ROI_rect1);
    cvCopy(pImgResize, pImgChar[2], NULL); //获取第3个字符
    cvResetImageROI(pImgResize);


    ROI_rect1.x = fourleft - 1;
    ROI_rect1.y = 0;
    ROI_rect1.width = fourright - fourleft + 2;
    ROI_rect1.height = pImgResize->height;
    cvSetImageROI(pImgResize, ROI_rect1);
    cvCopy(pImgResize, pImgChar[3], NULL); //获取第4个字符
    cvResetImageROI(pImgResize);

    ROI_rect1.x = fiveleft - 1;
    ROI_rect1.y = 0;
    ROI_rect1.width = fiveright - fiveleft + 2;
    ROI_rect1.height = pImgResize->height;
    cvSetImageROI(pImgResize, ROI_rect1);
    cvCopy(pImgResize, pImgChar[4], NULL); //获取第5个字符
    cvResetImageROI(pImgResize);

    ROI_rect1.x = sixleft - 1;
    ROI_rect1.y = 0;
    ROI_rect1.width = sixright - sixleft + 2;
    ROI_rect1.height = pImgResize->height;
    cvSetImageROI(pImgResize, ROI_rect1);
    cvCopy(pImgResize, pImgChar[5], NULL); //获取第6个字符
    cvResetImageROI(pImgResize);

    ROI_rect1.x = sevenleft - 1;
    ROI_rect1.y = 0;
    ROI_rect1.width = sevenright - sevenleft + 1;
    ROI_rect1.height = pImgResize->height;
    cvSetImageROI(pImgResize, ROI_rect1);
    cvCopy(pImgResize, pImgChar[6], NULL); //获取第7个字符
    cvResetImageROI(pImgResize);
	
	//在二值图中画出边界线
	cvLine(pImgResize, cvPoint(oneleft, 0), cvPoint(oneleft, pImgResize->height), cvScalar(188, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(oneright, 0), cvPoint(oneright, pImgResize->height), cvScalar(188, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(twoleft, 0), cvPoint(twoleft, pImgResize->height), cvScalar(255, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(begin, 0), cvPoint(begin, pImgResize->height), cvScalar(188, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(end, 0), cvPoint(end, pImgResize->height), cvScalar(255, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(threeright, 0), cvPoint(threeright, pImgResize->height), cvScalar(188, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(fourleft, 0), cvPoint(fourleft, pImgResize->height), cvScalar(255, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(fourright, 0), cvPoint(fourright, pImgResize->height), cvScalar(188, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(fiveleft, 0), cvPoint(fiveleft, pImgResize->height), cvScalar(255, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(fiveright, 0), cvPoint(fiveright, pImgResize->height), cvScalar(188, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(sixleft, 0), cvPoint(sixleft, pImgResize->height), cvScalar(255, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(sixright, 0), cvPoint(sixright, pImgResize->height), cvScalar(188, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(sevenleft, 0), cvPoint(sevenleft, pImgResize->height), cvScalar(255, 0, 0), 1, 8, 0);
	cvLine(pImgResize, cvPoint(sevenright, 0), cvPoint(sevenright, pImgResize->height), cvScalar(188, 0, 0), 1, 8, 0);


	cvSaveImage("./imgs/result/pImgChar.jpg", pImgResize);
	cvSaveImage("./imgs/result/pImgCol.jpg", pImgCol);
	cvSaveImage("./imgs/result/pImgChar1.jpg", pImgChar[0]);
	cvSaveImage("./imgs/result/pImgChar2.jpg", pImgChar[1]);
	cvSaveImage("./imgs/result/pImgChar3.jpg", pImgChar[2]);
	cvSaveImage("./imgs/result/pImgChar4.jpg", pImgChar[3]);
	cvSaveImage("./imgs/result/pImgChar5.jpg", pImgChar[4]);
	cvSaveImage("./imgs/result/pImgChar6.jpg", pImgChar[5]);
	cvSaveImage("./imgs/result/pImgChar7.jpg", pImgChar[6]);

    // 释放资源
    cvReleaseImage(&pImgSrc);
	cvReleaseImage(&pImgCol);
    cvReleaseImage(&pImgResize);

    return pImgChar;
}

//寻找车牌中的分隔符·
bool CharPartition::findSeparator(int templ[], int sum[], int *begin, int *end)
{
    int m = WIDTH; // 34  分隔符总的宽度是2×D+R=34 mm
    int i, j, g;
    int subre[WIDTH] = { 0 };

    // 分隔符区域搜索算法
    while (m > 7) { // m为分隔符宽度，为34，若给的范围太宽，逐渐减小宽度
        // START(57)->END(181)  扩大范围，设定搜索范围在第2个字符的起始位置到第3个字符的结束位置
        // templ模板为 (3, 3, 15, 15, ... 15, 15, 3, 3)
        // sum垂直投影，像素值的和
        for (i = START; i + m < END; i++) {
            for (j = 0, g = i; j < m; j++, g++) {
                subre[j] = templ[j] - sum[g];				
            }

            j = 0;
            while (subre[j]>0 && j<m)
                j++;

            // 如果序列subre中的每个元素都大于0，则认为找到了分隔符区域
            if (j == m) {
                *begin = i;
                *end = *begin + m; // 分隔符区域为[L+i, L+i+m]
                return true;
            }
        }

        // 没有找到分隔符区域  缩小模板
        templ[m - 3] = templ[m - 1];
        templ[m - 4] = templ[m - 2];
        m--;
    }

    return false;
}

