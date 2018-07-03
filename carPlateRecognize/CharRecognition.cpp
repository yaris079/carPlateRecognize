#include <QMessageBox>
#include <qdebug.h>
#include "CharRecognition.h"

#pragma execution_character_set("utf-8")

CharRecognition::CharRecognition()
{

}

QString CharRecognition::recognizeChar(IplImage **srcIplImage)
{
    IplImage *src = 0;
    char templatePath[256] = { 0 };
    static QString recognizeResult;
    recognizeResult.clear();

    for (int x = 0; x<CHARS_LENGHT; x++) {
        src = srcIplImage[x];

        if (!src) {
            QMessageBox::warning(NULL, "警告", "您的字符不存在");
            return QString("");
        }

        int r_result[45];
        cvThreshold(src, src, 100, 255, CV_THRESH_BINARY);

        IplImage *src1 = 0;
        int i = 0;
        /*
        0,1,2,3,4,5,6,7,8,9[0-9]
        a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z(i, o 不可用)[10-33]
        [京][沪][津][渝][冀][晋][蒙][辽][吉][黑][苏][浙][皖][闽][赣][鲁][豫][鄂][湘][粤][桂][琼][川][贵][云][藏][陕][甘][青][宁][新][34-64]
        因为数据不够，仅有部分省份模板
		*/

        for (i = 0; i < 45; i++) {
            // If you use Windows, please uncomment this following code, and set the correct path of template floder.
            sprintf(templatePath, "./imgs/template/%d.jpg", i);

            // If you use Mac OS X, please uncomment this following code, and set the correct path of template floder.
            //sprintf(templatePath, "/Users/ifpelset/Develop/Code/PC/Qt/SimpleLPR/imgs/template/%d.jpg", i);
            src1 = cvLoadImage(templatePath, 0);

            if (!src1) {
                QMessageBox::warning(NULL, "Can not load file", templatePath);
                return QString("");
            }

            cvThreshold(src1, src1, 100, 255, CV_THRESH_BINARY);
            r_result[i] = myGetResult_R(src, src1);
            cvReleaseImage(&src1);
        }

        int best = 0, best_value;
        best_value = r_result[0];

        for (i = 0; i < 45; i++)
        if (r_result[i] < best_value) {
            best_value = r_result[i];
            best = i;
        }

        if (best >=0 && best <= 9) {
            recognizeResult += QString::number(best);
        } else if (best >= 10 && best <= 33) {
            // 17 h 18 j   n 22 p 23
            if (best >= 23) { // + 2
                recognizeResult += char(best+2+55);
            }
            else if (best < 23 && best >= 18) { // +1
                recognizeResult += char(best + 1 + 55);
            }
            else { // +0
                recognizeResult += char(best + 55);
            }
        } else {
            switch (best) {
            case 34:
                recognizeResult += "沪";
                break;
            case 35:
                recognizeResult += "苏";
                break;
            case 36:
                recognizeResult += "浙";
                break;
            case 37:
                recognizeResult += "辽";
                break;
			case 38:
				recognizeResult += "川";
				break;
			case 39:
				recognizeResult += "吉";
				break;
			case 40:
				recognizeResult += "琼";
				break;
			case 41:
				recognizeResult += "京";
				break;
			case 42:
				recognizeResult += "皖";
				break;
			case 43:
				recognizeResult += "冀";
				break;
			case 44:
				recognizeResult += "豫";
				break;
            default:
                // 剩余的中文字暂时不处理
                break;
            }
        }
        // 最后释放资源，一共七次，指针数组中引用的资源
        cvReleaseImage(&src);
    }

    return recognizeResult;
}

void CharRecognition::myHorizontal(IplImage *src, int *r_sum)
{
    int i, j;
    int counter = 0;;//各行白点个数
    for (j = 0; j<src->height; j++) {
        counter = 0;
        for (i = 0; i<src->width; i++) {
            if (S(src, i, j) == 255)
                counter++;
        }
        r_sum[j] += counter;
    }
}

void CharRecognition::myVertical(IplImage *src, int *l_sum)
{
    int i, j;
    int counter = 0;//各列白点个数
    for (i = 0; i<src->width; i++) {
        counter = 0;
        for (j = 0; j<src->height; j++) {
            if (S(src, i, j) == 255)counter++;
        }
        l_sum[i] += counter;
    }
}

int CharRecognition::mySub(int *r_poniter1, int *r_poniter2, int leng)//各列或行白点个数差的绝对值之和
{
    int i = 0, sum = 0;
    for (i = 0; i<leng; i++)
        sum += abs(r_poniter1[i] - r_poniter2[i]);
    return sum;
}

int CharRecognition::myGetResult_R(IplImage *src1, IplImage *src2)//
{
    // 这里必须要初始化，不然会造成值非常大，int越界
	int l_sum1[90] = { 0 }, l_sum2[90] = { 0 };
    int r_sum1[90] = { 0 }, r_sum2[90] = { 0 };
    int r1,r2,result = 0;
    myHorizontal(src1, r_sum1);
    myVertical(src1, l_sum1);
    myHorizontal(src2, r_sum2);
    myVertical(src2, l_sum2);
	//计算各行白点个数差的绝对值之和
    r1 = mySub(r_sum1, r_sum2, 90);
	r2 = mySub(l_sum1, l_sum2, 90);
	result = (r1 + r2) / 2;
    return result;
}
