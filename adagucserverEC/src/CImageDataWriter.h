#ifndef CImageDataWriter_H
#define CImageDataWriter_H
#include "CStopWatch.h"
#include "CIBaseDataWriterInterface.h"
#include "CImgWarpNearestNeighbour.h"
#include "CImgWarpBilinear.h"
#include "CImgWarpBoolean.h"
#include "CMyCURL.h"

class CImageDataWriter: public CBaseDataWriterInterface{
  private:
    CImageWarper imageWarper;
    int requestType;
    int status;
    int mode;
    int animation;
    int nrImagesAdded;
    CT::string getFeatureInfoResult;
    CT::string getFeatureInfoHeader;
    DEF_ERRORFUNCTION();
    int createLegend(CDataSource *sourceImage,CDrawImage *drawImage);
    int warpImage(CDataSource *sourceImage,CDrawImage *drawImage);
    CDrawImage drawImage;
    CServerParams *srvParam;
    enum RenderMethodEnum { nearest, bilinear, contour, vector, shaded,contourshaded,vectorcontour,vectorcontourshaded,nearestcontour,bilinearcontour};
    
    float shadeInterval,contourIntervalL,contourIntervalH;
    float textScaleFactor,textOffsetFactor;//To display pressure in pa to hpa etc...
    int smoothingFilter;
    RenderMethodEnum renderMethod;
    RenderMethodEnum getRenderMethodFromString(CT::string *renderMethodString);
    double convertValue(CDFType type,void *data,size_t p);
    void setValue(CDFType type,void *data,size_t ptr,double pixel);
    int _setTransparencyAndBGColor(CServerParams *srvParam,CDrawImage* drawImage);
    float getValueForColorIndex(CDataSource *dataSource,int index);
    int getColorIndexForValue(CDataSource *dataSource,float value);
    int drawCascadedWMS(const char *service,const char *layers,bool transparent);
  public:
    static const char *RenderMethodStringList;
    CImageDataWriter();
    int createLegend(CDataSource *sourceImage);
    int getFeatureInfo(CDataSource *sourceImage,int dX,int dY);
    int createAnimation();
    void setDate(const char *date);
    int calculateData(std::vector <CDataSource*> &dataSources);
    
    // Virtual functions
    int init(CServerParams *srvParam, CDataSource *dataSource,int nrOfBands);
    int addData(std::vector <CDataSource*> &dataSources);
    int end();
    int initializeLegend(CServerParams *srvParam,CDataSource *dataSource);
};

#endif