#include "CDrawImage.h"

const char *CDrawImage::className="CDrawImage";
CDrawImage::CDrawImage(){
    dImageCreated=0;
    dPaletteCreated=0;
    _bEnableTransparency=false;
    _bEnableTrueColor=false;
    _bEnableAGG=false;
    dNumImages = 0;
    //for(int j=0;j<256;j++)_colors[j]=-1;
    Geo = new CGeoParams;
//    _bEnableAGG=true;_bEnableTrueColor=true;
    aggBytebuf=NULL;
    rbuf=NULL;
    renRGBA=NULL;
    ren256=NULL;
    ras=NULL;
    
    //Freetype
    library=NULL;
    face=NULL;
    TTFFontLocation = "/usr/X11R6/lib/X11/fonts/truetype/verdana.ttf";
    
}

CDrawImage::~CDrawImage(){
  if(_bEnableAGG==false){
    if(dPaletteCreated==1){
      for(int j=0;j<256;j++)if(_colors[j]!=-1)gdImageColorDeallocate(image,_colors[j]);
    }
  }
  if(dImageCreated==1){
    if(_bEnableAGG==false){
      gdImageDestroy(image);
    };  
  }
  delete aggBytebuf;aggBytebuf=NULL;
  delete rbuf;rbuf=NULL;
  delete renRGBA; renRGBA=NULL;
  delete ren256; ren256=NULL;
  delete ras; ras=NULL;
  delete Geo; Geo=NULL;
  
  if(library!=NULL){
    FT_Done_FreeType(library);library=NULL;face=NULL;
  }
}


int CDrawImage::renderFont(FT_Bitmap *bitmap,int left,int top,agg::rgba8 color){
  for(int y=0;y<bitmap->rows;y++){
    for(int x=0;x<bitmap->width;x++){
      size_t p=(x+y*bitmap->width);
      if(bitmap->buffer[p]!=0){
        agg::rgba8 c= renRGBA->pixel( x+left,  y+top);
        int alpha=bitmap->buffer[p];
        alpha=(alpha*color.a)/256;
        int alphab=256-alpha;
        unsigned int tr=c.r*alphab+color.r*alpha;
        unsigned int tg=c.g*alphab+color.g*alpha;
        unsigned int tb=c.b*alphab+color.b*alpha;
        tr/=256;tg/=256;tb/=256;
        if(tr>255)tr=255;if(tg>255)tg=255;if(tb>255)tb=255;
        renRGBA->pixel( x+left,  y+top, agg::rgba8(tr,tg,tb,255));
      }
    }
  }
  return 0;
}

int CDrawImage::drawFreeTypeText(int x,int y,float angle,const char *text,agg::rgba8 color){
  //Draw text :)
  if(library==NULL){
    int status  = initializeFreeType();
    if(status != 0){
      CDBError("Unable to initialize freetype");
      return 1;
    }
  };
  
  
  int error;
  FT_GlyphSlot slot; FT_Matrix matrix; /* transformation matrix */
  //  FT_UInt glyph_index;
  FT_Vector pen; /* untransformed origin */
  int n;

  int my_target_height = 8;

  int num_chars=strlen(text);
  slot = face->glyph; /* a small shortcut */
  /* set up matrix */
  matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
  matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
  matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
  matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L ); /* the pen position in 26.6 cartesian space coordinates */
  /* start at (300,200) */ 
  pen.x = x * 64; pen.y = ( my_target_height - y ) * 64;
  for ( n = 0; n < num_chars; n++ ) { /* set transformation */  
    FT_Set_Transform( face, &matrix, &pen ); /* load glyph image into the slot (erase previous one) */  
    error = FT_Load_Char( face, text[n], FT_LOAD_RENDER ); 
    if ( error ){
      CDBError("unable toFT_Load_Char");
      return 1;
    }
    /* now, draw to our target surface (convert position) */ 
    renderFont( &slot->bitmap, slot->bitmap_left, my_target_height - slot->bitmap_top,color ); 
    /* increment pen position */  
    pen.x += slot->advance.x; pen.y += slot->advance.y; 
  }
  return 0;
}
int CDrawImage::initializeFreeType(){
  if(library!=NULL){
    CDBError("Freetype is already intialized");
    return 1;
  };
  int error = FT_Init_FreeType( &library );
  const char * fontLocation = TTFFontLocation;
  //"/usr/X11R6/lib/X11/fonts/truetype/verdana.ttf";//"/usr/share/fonts/truetype/arial.ttf";
  
  //CDBError("Unable to initialize freetype: Could not read fontfile %s",fontLocation);

  if ( error ) { 
    CDBError("an error occurred during freetype library initialization");
    if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
    return 1;
  }
  error = FT_New_Face( library,fontLocation , 0, &face ); 
  if ( error == FT_Err_Unknown_File_Format ) { 
    CDBError("the font file could be opened and read, but it appears that its font format is unsupported");
    if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
    return 1;
  } else if ( error ) {
    CDBError("Unable to initialize freetype: Could not read fontfile %s",fontLocation);
    if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
    return 1;
  }
  
  error = FT_Set_Char_Size( face, /* handle to face object */  
                            0, /* char_width in 1/64th of points */ 
                            8*64, /* char_height in 1/64th of points */ 
                            100, /* horizontal device resolution */
                            100 ); /* vertical device resolution */
  if ( error ) {
    CDBError("unable to set character size");
    if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
    return 1;
  }
  
  //drawFreeTypeText(400,400,1,"HALLO",agg::rgba8(255,255,255,200));

  return 0;
}


int CDrawImage::createImage(int _dW,int _dH){
  Geo->dWidth=_dW;
  Geo->dHeight=_dH;
  return createImage(Geo);
}

int CDrawImage::createImage(CGeoParams *_Geo){
#ifdef MEASURETIME
  StopWatch_Stop("start createImage");
#endif  
  
  if(dImageCreated==1){CDBError("createImage: image already created");return 1;}
  //CDBDebug("_bEnableAGG: %d, _bEnableTrueColor:%d",_bEnableAGG,_bEnableTrueColor);
  Geo->copy(_Geo);
  if(_bEnableAGG==true){
    size_t imageSize=0;
    if(_bEnableTrueColor){
      //Create the frame buffer
      imageSize=Geo->dWidth * Geo->dHeight * 4;
      aggBytebuf = new unsigned char[imageSize];
      
      // Create the rendering buffer 
      rbuf = new agg::rendering_buffer(aggBytebuf, Geo->dWidth,Geo->dHeight, Geo->dWidth * 4);
      // Create the renderer and the rasterizer
      renRGBA = new agg::renderer<agg::span_rgba32> (*rbuf);
    }else{
      //Create the frame buffer
      imageSize=Geo->dWidth * Geo->dHeight;
      aggBytebuf = new unsigned char[imageSize ];
      // Create the rendering buffer 
      rbuf = new agg::rendering_buffer(aggBytebuf, Geo->dWidth,Geo->dHeight, Geo->dWidth );
      // Create the renderer and the rasterizer
      ren256 = new agg::renderer<agg::span_mono8> (*rbuf);
    }
    ras = new agg::rasterizer();
    // Setup the rasterizer
    ras->gamma(1.3);
    ras->filling_rule(agg::fill_even_odd);
    if(_bEnableTrueColor){
    renRGBA->clear(agg::rgba8(210, 210, 210,0));
      /*int j=0;int i=imageSize;
      do{
        aggBytebuf[j++]=0;
    }while(j<i);*/
      /*for(size_t j=0;j<imageSize;j++){
        aggBytebuf[j]=0;
    }*/

    }else{
      ren256->clear(agg::rgba8(255, 255, 255,0));
    }
    
  }
  if(_bEnableAGG==false){
    if(_bEnableTrueColor==false){
      image = gdImageCreate(Geo->dWidth,Geo->dHeight);
    }else{
      image = gdImageCreateTrueColor(Geo->dWidth,Geo->dHeight);
      gdImageSaveAlpha( image, true );
    }
    fontConfig = (char*)"verdana"; /* fontconfig pattern */
    gdFTUseFontConfig(1);
  }
  dImageCreated=1;
#ifdef MEASURETIME
  StopWatch_Stop("image created.");
#endif  

  return 0;

}


int CDrawImage::writeAGGPng(){
  
  
#ifdef MEASURETIME
  StopWatch_Stop("start writeAGGPng.");
#endif  
  int width=Geo->dWidth, height=Geo->dHeight;
  png_structp png_ptr;
  png_infop info_ptr;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  
  if (!png_ptr){CDBError("png_create_write_struct failed");return 1;}
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr){
    png_destroy_write_struct(&png_ptr, NULL);
    CDBError("png_create_info_struct failed");
    return 1;
  }

  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during init_io");
    return 1;
  }

  png_init_io(png_ptr, stdout);

  /* write header */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during writing header");
    return 1;
  }
  /*png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
  PNG_FILTER_TYPE_DEFAULT);*/
  if(_bEnableTrueColor){
  /*png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
    PNG_FILTER_TYPE_BASE);*/
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
  
  }else{
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_NONE);
  }
  
  
  png_set_filter (png_ptr, 0, PNG_FILTER_NONE);
  png_set_compression_level (png_ptr, -1);
  
  //png_set_invert_alpha(png_ptr);
  png_write_info(png_ptr, info_ptr);
  
  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during writing bytes");
    return 1;
  }
  png_set_packing(png_ptr);
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  

  int i;
  png_bytep row_ptr = 0;
  if(_bEnableTrueColor){
    for (i = 0; i < height; i=i+1){
      row_ptr = aggBytebuf + 4 * i * width;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
  }else{
    for (i = 0; i < height; i++){
      row_ptr = aggBytebuf + 1 * i * width;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
  }
  
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  

  /* end write */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during end of write");
    return 1;
  }

  png_write_end(png_ptr, NULL);
  
#ifdef MEASURETIME
  StopWatch_Stop("end writeAGGPng.");
#endif  
  return 0;
}

int CDrawImage::printImage(){
  if(dImageCreated==0){CDBError("print: image not created");return 1;}
  
  if(_bEnableAGG==true){
    writeAGGPng();
  }
  if(_bEnableAGG==false){
    gdImagePng(image, stdout);
  }
  return 0;
}

void CDrawImage::draw_line(agg::rasterizer* ras,
                  double x1, double y1, 
                  double x2, double y2,
                  double width)
{

  double dx = x2 - x1;
  double dy = y2 - y1;
  double d = sqrt(dx*dx + dy*dy);
    
  dx = width * (y2 - y1) / d;
  dy = width * (x2 - x1) / d;

  ras->move_to_d(x1 - dx,  y1 + dy);
  ras->line_to_d(x2 - dx,  y2 + dy);
  ras->line_to_d(x2 + dx,  y2 - dy);
  ras->line_to_d(x1 + dx,  y1 - dy);
}

void CDrawImage::drawVector(int x,int y,double direction, double strength,int color){
  double wx1,wy1,wx2,wy2,hx1,hy1,hx2,hy2,dx1,dy1;
  dx1=cos(direction)*(strength+1.25);
  dy1=sin(direction)*(strength+1.25);
  wx1=double(x)-dx1;wy1=double(y)-dy1;
  wx2=double(x)+dx1;wy2=double(y)+dy1;
  //hx1=double(x)+cos(direction-0.7f)*(strength/1.8f);hy1=double(y)+sin(direction-0.7f)*(strength/1.8f);
  //hx2=double(x)+cos(direction+0.7f)*(strength/1.8f);hy2=double(y)+sin(direction+0.7f)*(strength/1.8f);
  strength=(-3-strength);
  hx1=double(wx2)+cos(direction-0.7f)*(strength/1.8f);hy1=double(wy2)+sin(direction-0.7f)*(strength/1.8f);
  hx2=double(wx2)+cos(direction+0.7f)*(strength/1.8f);hy2=double(wy2)+sin(direction+0.7f)*(strength/1.8f);
  if(_bEnableAGG==true){
    double width=.75;
    agg::rgba8 colStr(CDIred[color], CDIgreen[color], CDIblue[color],255);
    if(_bEnableTrueColor){
      draw_line(ras,wx1,wy1,wx2,wy2,width);
      ras->render(*renRGBA, colStr);
      draw_line(ras,wx2,wy2,hx1,hy1,width);
      ras->render(*renRGBA, colStr);
      draw_line(ras,wx2,wy2,hx2,hy2,width);
      ras->render(*renRGBA, colStr);
    }else{
      draw_line(ras,wx1,wy1,wx2,wy2,width);
      ras->render(*ren256, colStr);
      draw_line(ras,wx2,wy2,hx1,hy1,width);
      ras->render(*ren256, colStr);
      draw_line(ras,wx2,wy2,hx2,hy2,width);
      ras->render(*ren256, colStr);
    }
  }else{
    gdImageLine(image, int(wx1),int(wy1),int(wx2),int(wy2),_colors[color]);
    gdImageLine(image, int(wx2),int(wy2),int(hx1),int(hy1),_colors[color]);
    gdImageLine(image, int(wx2),int(wy2),int(hx2),int(hy2),_colors[color]);
  }
}

void CDrawImage::line(int x1,int y1,int x2,int y2,int color){
  if(_bEnableAGG==true){
    if(color>=0&&color<256){
      
      draw_line(ras,x1,y1,x2,y2,0.8);
      if(_bEnableTrueColor){
        ras->render(*renRGBA, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
      }else{
        ras->render(*ren256, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
      }
    
  
    }
  }else{
    gdImageLine(image, x1,y1,x2,y2,_colors[color]);
  }
}
void CDrawImage::line(int x1,int y1,int x2,int y2,float w,int color){
  if(_bEnableAGG==true){
    if(color>=0&&color<256){
      
      draw_line(ras,x1,y1,x2,y2,w);
      if(_bEnableTrueColor){
        ras->render(*renRGBA, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
      }else{
        ras->render(*ren256, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
      }
    
  
    }
  }else{
    gdImageSetThickness(image, int(w)*2);
    gdImageLine(image, x1,y1,x2,y2,_colors[color]);
  }
}

/*void CDrawImage::line(float &x1,float &y1,float &x2,float &y2,int &color){
  if(_bEnableAGG==true){
    if(color>=0&&color<256){
      draw_line(ras, x1,y1,x2,y2,.8);
      if(_bEnableTrueColor){
        ras->render(*renRGBA, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
      }else{
        ras->render(*ren256, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
      }
        
      
    }
  }else{
    gdImageLine(image, x1,y1,x2,y2,_colors[color]);
  }
}*/
void CDrawImage::setPixelIndexed(int x,int y,int color){
  if(_bEnableAGG==true){
    if(color>=0&&color<256){
      if(_bEnableTrueColor){
       renRGBA->pixel( x,  y, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
       // draw_line(ras,x,y,x+2,y+2,1);
        //ras->render(*renRGBA, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
      }else{
        ren256->pixel( x,  y, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
      }
    }
  }else{
    gdImageSetPixel(image, x,y,colors[color]);
  }
}

void CDrawImage::setPixelTrueColor(int x,int y,unsigned int color){
  if(_bEnableAGG==true){
    if(_bEnableTrueColor){
      //renRGBA->pixel( x,  y, agg::rgba8(color, color>>8, color>>16,color>>24));
      renRGBA->pixel( x,  y, agg::rgba8(color,color/256,color/(256*256),255));
       // draw_line(ras,x,y,x+2,y+2,1);
        //ras->render(*renRGBA, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
    }else{
     // ren256->pixel( x,  y, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
    }
  }else{
    gdImageSetPixel(image, x,y,color);
  }
}

map<int,int> myColorMap;
map<int,int>::iterator myColorIter;

void CDrawImage::setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b){
  if(_bEnableAGG==true){
    if(_bEnableTrueColor){
      //renRGBA->pixel( x,  y, agg::rgba8(color, color>>8, color>>16,color>>24));
      renRGBA->pixel( x,  y, agg::rgba8(r,g,b,255));
       // draw_line(ras,x,y,x+2,y+2,1);
        //ras->render(*renRGBA, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
    }else{
     // ren256->pixel( x,  y, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
    }
  }else{
    if(_bEnableTrueColor){
      gdImageSetPixel(image, x,y,r+g*256+b*65536);
    }else{
      int key = r+g*256+b*65536;
      int color;
      myColorIter=myColorMap.find(key);
      if(myColorIter==myColorMap.end()){
        color = gdImageColorClosest(image,r,g,b);
        myColorMap[key]=color;
      }else{
        color=(*myColorIter).second;
      }
      gdImageSetPixel(image, x,y,color);
      
    }
  }
}

void CDrawImage::setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b,unsigned char a){
  if(_bEnableAGG==true){
    if(_bEnableTrueColor){
      agg::rgba8 c= renRGBA->pixel( x,  y);
      int alpha=(255*a)/256;
      int alphab=256-alpha;
      unsigned int tr=c.r*alphab+r*alpha;
      unsigned int tg=c.g*alphab+g*alpha;
      unsigned int tb=c.b*alphab+b*alpha;
      tr/=256;tg/=256;tb/=256;
      if(tr>255)tr=255;if(tg>255)tg=255;if(tb>255)tb=255;
      renRGBA->pixel( x,  y, agg::rgba8(tr,tg,tb,255));
    }else{
     // ren256->pixel( x,  y, agg::rgba8(CDIred[color], CDIgreen[color], CDIblue[color],255));
    }
  }else{
    if(_bEnableTrueColor){
      gdImageSetPixel(image, x,y,r+g*256+b*65536);
    }else{
      if(_bEnableTrueColor){
        gdImageSetPixel(image, x,y,r+g*256+b*65536);
      }else{
        int key = r+g*256+b*65536;
        int color;
        myColorIter=myColorMap.find(key);
        if(myColorIter==myColorMap.end()){
          color = gdImageColorClosest(image,r,g,b);
          myColorMap[key]=color;
        }else{
          color=(*myColorIter).second;
        }
        gdImageSetPixel(image, x,y,color);
      }
    }
  }
}
void CDrawImage::setText(const char * text, size_t length,int x,int y,int color,int fontSize){
  if(_bEnableAGG==true){
    agg::rgba8 colStr(CDIred[color], CDIgreen[color], CDIblue[color],255);
    drawFreeTypeText( x, y+11, 0,text,colStr);
    //Not yet supported...
  }else{
    char *pszText=new char[length+1];
    strncpy(pszText,text,length);
    pszText[length]='\0';
    if(fontSize==-1)gdImageString (image, gdFontSmall, x,  y, (unsigned char *)pszText, color);
    if(fontSize==0)gdImageString (image, gdFontMediumBold, x,  y, (unsigned char *)pszText, color);
    if(fontSize==1)gdImageString (image, gdFontLarge, x,  y, (unsigned char *)pszText, color);
    delete[] pszText;
  }
}

void CDrawImage::setTextStroke(const char * text, size_t length,int x,int y, int fgcolor,int bgcolor, int fontSize){
  if(_bEnableAGG==true){
    //Not yet supported...
  }else{
    char *pszText=new char[length+1];
    strncpy(pszText,text,length);
    pszText[length]='\0';
  
    for(int dy=-1;dy<2;dy=dy+1)
      for(int dx=-1;dx<2;dx=dx+1)
        if(!(dx==0&&dy==0)){
      if(fontSize==-1)gdImageString (image, gdFontSmall, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
          if(fontSize==0)gdImageString (image, gdFontMediumBold, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
          if(fontSize==1)gdImageString (image, gdFontLarge, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
        }
    if(fontSize==-1)gdImageString (image, gdFontSmall, x,  y, (unsigned char *)pszText, fgcolor);
    if(fontSize==0)gdImageString (image, gdFontMediumBold, x,  y, (unsigned char *)pszText, fgcolor);
    if(fontSize==1)gdImageString (image, gdFontLarge, x,  y, (unsigned char *)pszText, fgcolor);
  
    delete[] pszText;
  }
}

void CDrawImage::drawText(int x,int y,float angle,int fonstsize,const char *text,agg::rgba8 color){
  if(_bEnableAGG==true){
    drawFreeTypeText( x, y, angle,text,color);
  }else{
    bla(text, strlen(text),angle, x, y, 240,fonstsize);
  }
}
void CDrawImage::bla(const char * text, size_t length,double angle,int x,int y,int color,int fontSize){
  char *_text = new char[strlen(text)+1];
  memcpy(_text,text,strlen(text)+1);
  
  //gdImageStringFT(NULL,&brect[0],0,fontConfig,8.0f,angle,0,0,(char*)_text);
  //gdImageFilledRectangle (image,brect[0]+x,brect[1]+y,brect[2]+x,brect[3]+y, _colors[0]);
  int tcolor=-_colors[color];
  if(_bEnableTrueColor)tcolor=-tcolor;
  gdImageStringFT(image, &brect[0], tcolor, fontConfig, 8.0f, angle,  x,  y, (char*)_text);
  delete[] _text;
}

int CDrawImage::_createStandard(){
  addColor(240,0  ,0  ,0  );
  addColor(241,32,32  ,32  );
  addColor(242,64 ,64,64  );
  addColor(243,96,96,96  );
  //addColor(244,64  ,64  ,192);
  addColor(244,64  ,64  ,255);
  addColor(245,128,128  ,255);
  addColor(246,64  ,64,192);
  addColor(247,32,32,32);
  addColor(248,0  ,0  ,0  );
  addColor(249,255,255,191);
  addColor(250,191,232,255);
  addColor(251,204,204,204);
  addColor(252,160,160,160);
  addColor(253,192,192,192);
  addColor(254,224,224,224);
  addColor(255,255,255,255);
  
  copyPalette();
  return 0;
}
int CDrawImage::createGDPalette(){
  return createGDPalette(NULL);
}
int CDrawImage::createGDPalette(CServerConfig::XMLE_Legend *legend){
  if(dImageCreated==0){CDBError("createGDPalette: image not created");return 1;}
  for(int j=0;j<255;j++){
    CDIred[j]=0;
    CDIgreen[j]=0;
    CDIblue[j]=0;
  }
  if(legend==NULL){
    return _createStandard();
  }
  if(legend->attr.type.equals("colorRange")){
    int controle=0;
    float cx;
    float rc[3];
    for(size_t j=0;j<legend->palette.size()-1&&controle<240;j++){
      if(legend->palette[j]->attr.index>255)legend->palette[j]->attr.index=255;
      if(legend->palette[j]->attr.index<0)  legend->palette[j]->attr.index=0;
      if(legend->palette[j+1]->attr.index>255)legend->palette[j+1]->attr.index=255;
      if(legend->palette[j+1]->attr.index<0)  legend->palette[j+1]->attr.index=0;
      float dif = legend->palette[j+1]->attr.index-legend->palette[j]->attr.index;
      if(dif<0.5f)dif=1;
      rc[0]=float(legend->palette[j+1]->attr.red  -legend->palette[j]->attr.red)/dif;
      rc[1]=float(legend->palette[j+1]->attr.green-legend->palette[j]->attr.green)/dif;
      rc[2]=float(legend->palette[j+1]->attr.blue -legend->palette[j]->attr.blue)/dif;

  
      for(int i=legend->palette[j]->attr.index;i<legend->palette[j+1]->attr.index&&controle<240;i++){
        if(i!=controle){CDBError("Invalid color table");return 1;}
        cx=float(controle-legend->palette[j]->attr.index);
        CDIred[controle]=int(rc[0]*cx)+legend->palette[j]->attr.red;
        CDIgreen[controle]=int(rc[1]*cx)+legend->palette[j]->attr.green;
        CDIblue[controle]=int(rc[2]*cx)+legend->palette[j]->attr.blue;
        controle++;
      }
    }
    return _createStandard();
  }
  if(legend->attr.type.equals("interval")){
    for(size_t j=0;j<legend->palette.size();j++){
      for(int i=legend->palette[j]->attr.min;i<=legend->palette[j]->attr.max;i++){
        if(i>=0&&i<240){
          CDIred[i]=legend->palette[j]->attr.red;
          CDIgreen[i]=legend->palette[j]->attr.green;
          CDIblue[i]=legend->palette[j]->attr.blue;
          if(CDIred[i]==0)CDIred[i]=1;//for transparency
        }
      }
    }
    return _createStandard();
  }
  return 1;
}

void CDrawImage::rectangle( int x1, int y1, int x2, int y2,int innercolor,int outercolor){
  if(_bEnableAGG==true){
    
    
    float w=1;
    line( x1-1, y1, x2+1, y1,w,outercolor);
    line( x2, y1, x2, y2,w,outercolor);
    line( x2+1, y2, x1-1, y2,w,outercolor);
    line( x1, y2, x1, y1,w,outercolor);
    for(int j=y1+1;j<y2;j++){
      line( x1, j, x2, j,1,innercolor);
    }
  }else{
    gdImageFilledRectangle (image,x1+1,y1+1,x2-1,y2-1, innercolor);
    gdImageRectangle (image,x1,y1,x2,y2, outercolor);
  }
}

void CDrawImage::rectangle( int x1, int y1, int x2, int y2,int outercolor){
  if(_bEnableAGG==true){
    line( x1, y1, x2, y1,1,outercolor);
    line( x2, y1, x2, y2,1,outercolor);
    line( x2, y2, x1, y2,1,outercolor);
    line( x1, y2, x1, y1,1,outercolor);
  }else{
    gdImageRectangle (image,x1,y1,x2,y2, outercolor);
  }
}

int CDrawImage::addColor(int Color,unsigned char R,unsigned char G,unsigned char B){
  CDIred[Color]=R;
  CDIgreen[Color]=G;
  CDIblue[Color]=B;
  return 0;
}

int CDrawImage::copyPalette(){
  if(_bEnableAGG==false){
    _colors[0] = gdImageColorAllocate(image,BGColorR,BGColorG,BGColorB); 
    for(int j=1;j<256;j++){
      _colors[j] = gdImageColorAllocate(image,CDIred[j],CDIgreen[j],CDIblue[j]); 
    }
  }
  for(int j=0;j<256;j++){
    colors[j]=_colors[j];
    //gdImageSetAntiAliased(image,colors[j])
    if(CDIred[j]==0&&CDIgreen[j]==0&&CDIblue[j]==0)colors[j]=_colors[0];
  }
  if(_bEnableAGG==false){
    if(_bEnableTransparency){
      gdImageColorTransparent(image,_colors[0]);
    }
  }
  return 0;
}

int CDrawImage::addImage(int delay){
  //Add the current active image:
  gdImageGifAnimAdd(image, stdout, 0, 0, 0, delay, gdDisposalRestorePrevious, NULL);
  //This image is added, so it can be destroyed.
  for(int j=0;j<256;j++)if(_colors[j]!=-1)gdImageColorDeallocate(image,_colors[j]);
  gdImageDestroy(image); 
  //Make sure a new image is available for drawing
  if(_bEnableTrueColor==false){
    image = gdImageCreate(Geo->dWidth,Geo->dHeight);
  }else{
    image = gdImageCreateTrueColor(Geo->dWidth,Geo->dHeight);
  }
  
  copyPalette();
  return 0;
}

int CDrawImage::beginAnimation(){
  gdImageGifAnimBegin(image, stdout, 1, 0);
  return 0; 
}

int CDrawImage::endAnimation(){
  gdImageGifAnimEnd(stdout);
  return 0;
}

void CDrawImage::enableTransparency(bool enable){
  _bEnableTransparency=enable;
}
void CDrawImage::setBGColor(unsigned char R,unsigned char G,unsigned char B){
  BGColorR=R;
  BGColorG=G;
  BGColorB=B;
}

void CDrawImage::setTrueColor(bool enable){
  _bEnableTrueColor=enable;
}