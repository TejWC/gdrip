/**********************************************************************
 * This file is part of the GNU Dental Radiograph Image Program, also
 * known as "gdrip."
 *
 * gdrip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gdrip.  If not, see <http://www.gnu.org/licenses/>.
 *********************************************************************/

#include <QProgressDialog>
#include <QDebug>

#include "math.h"

#include "imageprocessor.h"

ImageProcessor::ImageProcessor(QObject *parent) :
    QObject(parent)
{
}

QImage ImageProcessor::equalizeHistogram(QImage input) {
    QImage returnMe(input);

    QVector<float> occ = ImageProcessor::findOccurrences(input);
    QVector<float> pdf;
    float currentpdf =0;
    for(int i=0;i<occ.count();i++) {
        currentpdf += occ.value(i);
        pdf.append(currentpdf);
    }

    for(int x=0;x<input.width();x++) {
        for(int y=0;y<input.height();y++) {
            int originalVal = qRed(input.pixel(x,y));
            float pdfVal = pdf.value(originalVal);
            int newVal = (int) (255 * pdfVal);
            returnMe.setPixel(x,y,newVal);
        }
    }
    return returnMe;
}

QVector<float> ImageProcessor::findOccurrences(QImage input) {
    QVector<float> returnMe;
    returnMe.fill(0,256);
    float single = 1.0 / (input.width() * input.height());

    for(int x=0;x<input.width();x++) {
        for(int y=0;y<input.height();y++) {
            float newVal = returnMe.value(qRed(input.pixel(x,y)))+single;
            returnMe.replace(qRed(input.pixel(x,y)),newVal);
        }
    }

    return returnMe;
}

QImage ImageProcessor::thresholdImage(QImage input, int cutoff) {

//    QVector<int> occLineVals = regValsBezier(p0x,p0y,p2x,p2y,bestBezX,bestBezY,3,img);
//    int sum=0;
//    foreach(int val, occLineVals) {
//        sum+=val;
//    }
//    float avgRegVals = ((float)sum) / occLineVals.count();
//    float devSum =0;
//    foreach(int val, occLineVals) {
//        float dev = val-avgRegVals;
//        devSum += (dev *dev);
//    }
//    float stdDev =(float)(sqrt(devSum / (occLineVals.count()-1)));

//    qDebug()<<"Avg: " <<avgRegVals;
//    qDebug()<<"StDev: " <<stdDev;

//    float thres = avgRegVals + (3 * stdDev);
    //now show that threshold
    QImage returnMe(input);
    for(int x=0;x<input.width();x++) {
        for(int y=0;y<input.height();y++) {
            int val = qRed(input.pixel(x,y));
            if(val > cutoff) {
                returnMe.setPixel(x,y,255);
            } else {
                returnMe.setPixel(x,y,0);
            }
            //returnMe.setPixel(x,y,qRgb(val,val,val));
        }
    }
    return returnMe;
}

QVector<int> ImageProcessor::findOcculsion(QImage input) {
    int radius = (int) (.1 * input.height());
    //qDebug()<<"radius: " <<radius;

    //left side
    int bestLeftY=0;
    int bestLeftYval=INT_MAX;
    for(int currentY=radius;currentY<input.height()-radius;currentY++) {
        int sum =0;
        foreach(int x,ImageProcessor::regionVals(0,currentY,radius,input)) {
            sum+=x;
        }

        if(sum < bestLeftYval) {
            bestLeftY = currentY;
            bestLeftYval = sum;
        }
    }

    //QPainter painter(&input);
    //painter.fillRect(0,bestLeftY,square,square,QColor(255,0,0));

    //right side
    int bestRightY=0;
    int bestRightYval=INT_MAX;
    for(int currentY=radius;currentY<input.height()-radius;currentY++) {
        int sum =0;
        foreach(int x,ImageProcessor::regionVals(input.width()-1,currentY,radius,input)) {
            sum+=x;
        }
        //qDebug()<<"Sum was: " <<sum;

        if(sum < bestRightYval) {
            bestRightY = currentY;
            bestRightYval = sum;
        }
    }
    //painter.fillRect(input.width()-1-square,bestRightY,square,square,QColor(255,0,0));

    //now for the ugly part: making the (Quadratic) bezier curve

    int p0x = 0;
    int p0y = bestLeftY;

    int p2x = input.width() -1;
    int p2y = bestRightY;

    int bestBezX=0;
    int bestBezY=0;
    int bestBezVal = INT_MAX;
    QProgressDialog process(tr("Calculating different bezier curves...."),
                            QString(),0,input.width()*input.height());
    process.setWindowTitle(tr("Calculating curves"));
    process.setWindowModality(Qt::WindowModal);
    for(int x=0;x<input.width();x++) {
        for(int y=0;y<input.height();y++) {
            int val = ImageProcessor::computeBezierSum(
                        p0x,p0y,p2x,p2y,x,y,bestBezVal,input);
            if(val < bestBezVal) {
                bestBezX = x;
                bestBezY = y;
                bestBezVal = val;
            }
            process.setValue(process.value() +1);
        }
    }
    process.setValue(input.width()*input.height());

    QVector<int> returnMe;
    returnMe.append(p0x); //0
    returnMe.append(p0y);//1
    returnMe.append(bestBezX);//2
    returnMe.append(bestBezY);//3
    returnMe.append(p2x);//4
    returnMe.append(p2y);//5

    return returnMe;
}

void ImageProcessor::drawBezier(int p0x, int p0y, int p2x,
                                int p2y, int p1x, int p1y, QPainter *img) {
    for(float t = 0;t<1;t+= 0.0001) {
        float onemt = 1-t;

        float x = (onemt*onemt*p0x) +
                (2 * onemt * t *p1x) +
                (t * t *p2x);

        float y = (onemt*onemt*p0y) +
                (2 * onemt * t *p1y) +
                (t * t *p2y);

        img->fillRect((int)x,(int)y,1,1,QColor(0,255,0));
        //img.setPixel((int)x,(int)y,qRgb(0,255,0));
    }

//    qDebug()<<"p0x: " << p0x;
//    qDebug()<<"p0y: " << p0y;
//    qDebug()<<"p2x: " << p2x;
//    qDebug()<<"p2y: " << p2y;

}

QImage ImageProcessor::drawOcculsion(QImage input) {
    QVector<int> occ = ImageProcessor::findOcculsion(input);
    QImage returnMe(input);
    returnMe = returnMe.convertToFormat(QImage::Format_RGB32);
    QPainter p(&returnMe);
    ImageProcessor::drawBezier(
                occ.at(0),
                occ.at(1),
                occ.at(4),
                occ.at(5),
                occ.at(2),
                occ.at(3),
                &p
                );
    return returnMe;
}

int ImageProcessor::computeBezierSum(int p0x, int p0y, int p2x,
                                  int p2y, int p1x, int p1y, int best, QImage img) {
    int returnMe=0;
    float div = 1.0/ img.width() * 2;
    for(float t = 0;t<1;t+= div) {
        float onemt = 1-t;

        float x = (onemt * onemt*p0x) +
                (2 * onemt * t *p1x) +
                (t * t *p2x);

        float y = (onemt * onemt*p0y) +
                (2 * onemt * t *p1y) +
                (t * t *p2y);

        int val = qRed(img.pixel((int)x,(int)y));
        //returnMe += (val*val) ;
        returnMe += (val) ;

        if(returnMe > best) {
            return returnMe;
        }
    }

    return returnMe;
}

QVector<int> ImageProcessor::regValsBezier(int p0x, int p0y, int p2x,
                                           int p2y, int p1x, int p1y, int r, QImage img) {
    QVector<int> returnMe;
    float div = 1.0/ img.width() * 2;
    for(float t = 0;t<1;t+= div) {
        float onemt = 1-t;

        float x = (onemt * onemt*p0x) +
                (2 * onemt * t *p1x) +
                (t * t *p2x);

        float y = (onemt * onemt*p0y) +
                (2 * onemt * t *p1y) +
                (t * t *p2y);

        returnMe << regionVals((int)x,(int)y,r,img);
    }
    return returnMe;
}

QVector<int> ImageProcessor::regionVals(int startX, int startY, int r, QImage img) {
    QVector<int> vals;
    for(int x= startX - r; x < (startX + r); x++) {
        for(int y = startY-r; y< (startY+r); y++) {
            if(img.valid(x,y)) {
                //int val = qRed(img.pixel(x,y));
                //vals.append(val*val*val);
                vals.append(qRed(img.pixel(x,y)));
            }
        }
    }

    return vals;
}

QImage ImageProcessor::findBackground(QImage input) {
    QImage returnMe = equalizeHistogram(input);
    QVector<int> occ = ImageProcessor::findOcculsion(returnMe);

    //returnMe = returnMe.convertToFormat(QImage::Format_RGB32);
    //QPainter p(&returnMe);

    QVector<int> regVals = ImageProcessor::regValsBezier(
                occ.at(0),
                occ.at(1),
                occ.at(4),
                occ.at(5),
                occ.at(2),
                occ.at(3),
                3,
                returnMe
                );

    int sum=0;
    foreach(int i ,regVals) {
        sum+=i;
    }
    double average = ((double)sum) /  regVals.count();
    double variance=0;
    foreach(int i ,regVals) {
        int val = i-average;
        variance+= (val *val);
    }
    double standardDev =sqrt(variance / regVals.count());
    returnMe = ImageProcessor::thresholdImage(returnMe,average + (standardDev*3));
    returnMe = ImageProcessor::drawOcculsion(returnMe);

    return returnMe;
}

void ImageProcessor::drawBezierDer(int p0x, int p0y, int p2x,
                                     int p2y, int p1x, int p1y, QPainter *input) {
    input->setPen(QColor(255,0,0));

    float div = .1;
    for(float t = 0;t<1;t+= div) {
        float onemt = 1-t;

        float x = (onemt * onemt*p0x) +
                (2 * onemt * t *p1x) +
                (t * t *p2x);

        float y = (onemt * onemt*p0y) +
                (2 * onemt * t *p1y) +
                (t * t *p2y);

        float slopeX = (2*onemt*(p1x-p0x)) + (2*t*(p2x-p1x));
        float slopeY = (2*onemt*(p1y-p0y)) + (2*t*(p2y-p1y));

        float negInverse = -5 * slopeX / slopeY;

        QPoint start(x,y);
        QPoint end(x + 5,y+negInverse);

        input->drawLine(start,end);

    }
}

QImage ImageProcessor::findTeeth(QImage input) {
    QImage returnMe = equalizeHistogram(input);

    QVector<int> occ = ImageProcessor::findOcculsion(returnMe);
    returnMe = returnMe.convertToFormat(QImage::Format_RGB32);
    QPainter p(&returnMe);
    ImageProcessor::drawBezier(
                occ.at(0),
                occ.at(1),
                occ.at(4),
                occ.at(5),
                occ.at(2),
                occ.at(3),
                &p
                );
    ImageProcessor::drawBezierDer(
                occ.at(0),
                occ.at(1),
                occ.at(4),
                occ.at(5),
                occ.at(2),
                occ.at(3),
                &p
                );



    return returnMe;
}


//--------------optminal threshold (Otsu method)
//first make a probably array of the values
//    QVector<float> pi;
//    float totalPixels = img.width()*img.height();
//    float pSingle = 1.0 / totalPixels;
//    pi.fill(0,256);
//    for(int x=0;x<img.width();x++) {
//        for(int y=0;y<img.height();y++) {
//            int index = qRed(img.pixel(x,y));
//            float newVal = pi.value(index)+pSingle;
//            pi.replace(index,newVal);
//        }
//    }

//    //now find the "image average" aka ut
//    float ut =0;
//    for(int i=0;i<256;i++) {
//        ut += (i * pi.value(i) );
//    }

//    qDebug()<<"ut: "<<ut;

//    int bestK =0;
//    float bestKVal = 0;

//    for(int k=1;k<256;k++) {
//        //first calculate w(k)
//        float wk=0;
//        for(int i=0;i<k;i++) {
//            wk += pi.value(i);
//        }

//        //now calculate u(k)
//        float uk=0;
//        for(int i=k+1; i<256;i++) {
//            uk += pi.value(i);
//        }


//        //now calculate the between class variance
//        float numb = ut *  (wk - uk) * (wk - uk);
//        //float denom = wk*uk;
//        float denom = 1;
//        float kVal = (numb *numb) / denom;
//        if((kVal > bestKVal) &&
//                (denom !=  0)){
//            bestK = k;
//            bestKVal = kVal;
//        }
//    }

//    qDebug()<<bestK;
//    qDebug()<<bestKVal;