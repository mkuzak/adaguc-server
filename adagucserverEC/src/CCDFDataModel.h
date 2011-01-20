#ifndef CCDFDATAMODEL_H
#define CCDFDATAMODEL_H
#include <stdio.h>
#include <vector>
#include <iostream>
#include "CDebugger.h"
#include "CTypes.h"
#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

typedef int CDFType;

#define CDF_NONE    0 /* Unknown */
#define CDF_BYTE    1 /* signed 1 byte integer */
#define CDF_CHAR    2 /* ISO/ASCII character */
#define CDF_SHORT   3 /* signed 2 byte integer */
#define CDF_INT     4 /* signed 4 byte integer */
#define CDF_FLOAT   5 /* single precision floating point number */
#define CDF_DOUBLE  6 /* double precision floating point number */
#define CDF_UBYTE   7 /* unsigned 1 byte int */
#define CDF_USHORT  8 /* unsigned 2-byte int */
#define CDF_UINT    9 /* unsigned 4-byte int */

namespace CDF{
  //Allocates data for an array, provide type, the empty array and length
  // Data must be freed by using free()
  int allocateData(CDFType type,void **p,size_t length);
  //Copies data from one array to another and performs type conversion
  //Destdata must be a pointer to an empty array with non-void type
  class DataCopier{
    public:
      template <class T>
          int copy(T *destdata,void *sourcedata,CDFType sourcetype,size_t length){
        if(sourcetype==CDF_CHAR||sourcetype==CDF_BYTE)for(size_t t=0;t<length;t++){destdata[t]=((char*)sourcedata)[t];}
        if(sourcetype==CDF_CHAR||sourcetype==CDF_UBYTE)for(size_t t=0;t<length;t++){destdata[t]=((unsigned char*)sourcedata)[t];}
        if(sourcetype==CDF_SHORT)for(size_t t=0;t<length;t++){destdata[t]=((short*)sourcedata)[t];}
        if(sourcetype==CDF_USHORT)for(size_t t=0;t<length;t++){destdata[t]=((unsigned short*)sourcedata)[t];}
        if(sourcetype==CDF_INT)for(size_t t=0;t<length;t++){destdata[t]=((int*)sourcedata)[t];}
        if(sourcetype==CDF_UINT)for(size_t t=0;t<length;t++){destdata[t]=((unsigned int*)sourcedata)[t];}
        if(sourcetype==CDF_FLOAT)for(size_t t=0;t<length;t++){destdata[t]=((float*)sourcedata)[t];}
        if(sourcetype==CDF_DOUBLE)for(size_t t=0;t<length;t++){destdata[t]=((double*)sourcedata)[t];}
        return 0;
          }
  };
  //dataCopier.copy should work
  static DataCopier dataCopier;

  //Puts the CDF name of the type in the array with name (CDF_FLOAT, CDF_INT, etc...)
  void getCDFDataTypeName(char *name,size_t maxlen,int type);
  //Puts the C name of the type in the array with name (float, int, etc...)
  void getCDataTypeName(char *name,size_t maxlen,int type);
  //Returns the number of bytes needed for a single element of this datatype
  int getTypeSize(CDFType type);
  
  
  class Attribute{
    public:
      void setName(const char *value){
        name.copy(value);
      }

      Attribute(){
        data=NULL;
        length=0;
      }
      ~Attribute(){
        if(data!=NULL)free(data);data=NULL;
      }
      CDFType type;
      CT::string name;
      size_t length;
      void *data;
      int setData(Attribute *attribute){
        this->setData(attribute->type,attribute->data,attribute->size());
        return 0;
      }
      int setData(CDFType type,const void *dataToSet,size_t dataLength){
        if(data!=NULL)free(data);data=NULL;
        length=dataLength;
        this->type=type;
        allocateData(type,&data,length);
        if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE)memcpy(data,dataToSet,length);
        if(type==CDF_SHORT||type==CDF_USHORT)memcpy(data,dataToSet,length*sizeof(short));
        if(type==CDF_INT||type==CDF_UINT)memcpy(data,dataToSet,length*sizeof(int));
        if(type==CDF_FLOAT)memcpy(data,dataToSet,length*sizeof(float));
        if(type==CDF_DOUBLE){memcpy(data,dataToSet,length*sizeof(double));}
        return 0;
      }
      int setData(const char*dataToSet){
        if(data!=NULL)free(data);data=NULL;
        length=strlen(dataToSet);
        this->type=CDF_CHAR;
        allocateData(type,&data,length+1);
        if(type==CDF_CHAR){
          memcpy(data,dataToSet,length);//TODO support other data types as well
          ((char*)data)[length]='\0';
        }
        return 0;
      }
      template <class T>
      int getData(T *dataToGet,size_t getlength){
        if(data==NULL)return 0;
        if(getlength>length)getlength=length;
        dataCopier.copy(dataToGet,data,type,getlength);
        return getlength;
      }
      int getDataAsString(CT::string *out){
        out->copy("");
        if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE){out->copy((const char*)data,length);return 0;}
        if(type==CDF_INT||type==CDF_UINT)for(size_t n=0;n<length;n++)out->printconcat(" %d",((int*)data)[n]);
        if(type==CDF_SHORT||type==CDF_USHORT)for(size_t n=0;n<length;n++)out->printconcat(" %ds",((short*)data)[n]);
        if(type==CDF_FLOAT)for(size_t n=0;n<length;n++)out->printconcat(" %ff",((float*)data)[n]);
        if(type==CDF_DOUBLE)for(size_t n=0;n<length;n++)out->printconcat(" %fdf",((double*)data)[n]);
        return 0;
      }
      size_t size(){
        return length;
      }
  };
  class Dimension{
    public:
      CT::string name;
      size_t length;
      size_t getSize(){
        return length;
      }
      int id;
      void setName(const char *value){
        name.copy(value);
      }
  };
  class Variable{
    public:
      void *cdfReaderPointer;
      DEF_ERRORFUNCTION();
      Variable(){
        isDimension=false;
        data = NULL;
        currentSize=0;
        type=CDF_CHAR;
        cdfReaderPointer=NULL;
      }
      ~Variable(){
        for(size_t j=0;j<attributes.size();j++){if(attributes[j]!=NULL){delete attributes[j];attributes[j]=NULL;}}
        if(data!=NULL){free(data);data=NULL;}
      }
      std::vector<Attribute *> attributes;
      std::vector<Dimension *> dimensionlinks;
      CDFType type;
      CT::string name;
      CT::string orgName;
      void setName(const char *value){
        name.copy(value);
        //TODO Implement this correctly in readvariabledata....
        if(orgName.length()==0)orgName.copy(value);
      }
      int id;
      size_t currentSize;
      void setSize(size_t size){
        currentSize = size;
      }
      size_t getSize(){
        return currentSize;
      }
      void *data;
      bool isDimension;
      Attribute * getAttribute(const char *name){
        for(size_t j=0;j<attributes.size();j++){
          if(attributes[j]->name.equals(name)){
            return attributes[j];
          }
        }
        return NULL;
      }
      Dimension * getDimension(const char *name){
        for(size_t j=0;j<dimensionlinks.size();j++){
          if(dimensionlinks[j]->name.equals(name)){
            return dimensionlinks[j];
          }
        }
        return NULL;
      }
      int getDimensionIndex(const char *name){
        for(size_t j=0;j<dimensionlinks.size();j++){
          if(dimensionlinks[j]->name.equals(name)){
            return j;
          }
        }
        return -1;
      }
      int addAttribute(Attribute *attr){
        attributes.push_back(attr);
        return 0;
      }
      int removeAttribute(const char *name){
        for(size_t j=0;j<attributes.size();j++){
          if(attributes[j]->name.equals(name)){
            delete attributes[j];attributes[j]=NULL;
            attributes.erase(attributes.begin()+j);
          }
        }
        return 0;
      }
      int setAttribute(const char *attrName,CDFType attrType,const void *attrData,size_t attrLen){
        Attribute *attr=getAttribute(attrName);
        if(attr==NULL){
          attr = new Attribute();
          attr->name.copy(attrName);
          addAttribute(attr);
        }
        attr->type=attrType;
        attr->setData(attrType,attrData,attrLen);
        return 0;
      }
      int setAttributeText(const char *attrName,const char *attrString,size_t strLen){
        size_t attrLen=strLen+1;
        char *attrData=new char[attrLen];
        memcpy(attrData,attrString,strLen);attrData[strLen]='\0';
        int retStat = setAttribute(attrName,CDF_CHAR,attrData,attrLen);
        delete[] attrData;
        return retStat;
      }
      int setAttributeText(const char *attrName,const char *attrString){
        size_t attrLen=strlen(attrString);
        char *attrData=new char[attrLen];
        memcpy(attrData,attrString,attrLen);
        int retStat = setAttribute(attrName,CDF_CHAR,attrData,attrLen);
        delete[] attrData;
        return retStat;
      }

      int readData(CDFType type);
      int setData(CDFType type,const void *dataToSet,size_t dataLength){
        if(data!=NULL)free(data);data=NULL;
        currentSize=dataLength;
        this->type=type;
        allocateData(type,&data,currentSize);
        if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE)memcpy(data,dataToSet,currentSize);
        if(type==CDF_SHORT||type==CDF_USHORT)memcpy(data,dataToSet,currentSize*sizeof(short));
        if(type==CDF_INT||type==CDF_UINT)memcpy(data,dataToSet,currentSize*sizeof(int));
        if(type==CDF_FLOAT)memcpy(data,dataToSet,currentSize*sizeof(float));
        if(type==CDF_DOUBLE){memcpy(data,dataToSet,currentSize*sizeof(double));}
        return 0;
      }
  };

}
class CDFObject:public CDF::Variable{
  private:
    char *NCMLVarName;
    CDFType ncmlTypeToCDFType(const char *type){
      if(strncmp("String",type,6)==0)return CDF_CHAR;
      if(strncmp("byte",type,4)==0)return CDF_BYTE;
      if(strncmp("char",type,4)==0)return CDF_CHAR;
      if(strncmp("short",type,5)==0)return CDF_SHORT;
      if(strncmp("int",type,3)==0)return CDF_INT;
      if(strncmp("float",type,5)==0)return CDF_FLOAT;
      if(strncmp("double",type,6)==0)return CDF_DOUBLE;
      return CDF_DOUBLE;
    }
    void putNCMLAttributes(xmlNode * a_node){
      xmlNode *cur_node = NULL;
      for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE&&cur_node->name!=NULL){
            //Variable elements
            if(strncmp("variable",(char*)cur_node->name,8)==0){
              NCMLVarName=NULL;
              if(cur_node->properties->name!=NULL){
                if(cur_node->properties->children->content!=NULL){
                  xmlAttr*node=cur_node->properties;
                  char * pszOrgName=NULL,*pszName=NULL,*pszType=NULL;
                  while(node!=NULL){
                    if(strncmp("name",(char*)node->name,4)==0)
                      pszName=(char*)node->children->content;
                    if(strncmp("orgName",(char*)node->name,7)==0)
                      pszOrgName=(char*)node->children->content;
                    if(strncmp("type",(char*)node->name,4)==0)
                      pszType=(char*)node->children->content;
                    node=node->next;
                  }
                  //Rename a variable
                  if(pszOrgName!=NULL&&pszName!=NULL){
                    CDF::Variable *var= getVariable(pszOrgName);
                    if(var!=NULL){
                      var->name.copy(pszName);
                    }
                  }
                  if(pszName!=NULL){
                    NCMLVarName=pszName;
                    CDF::Variable *var= getVariable(pszName);
                    if(var==NULL&&pszOrgName==NULL){
                      var = new CDF::Variable();
                      var->type=CDF_CHAR;
                      if(pszType!=NULL){
                        var->type=ncmlTypeToCDFType(pszType);
                      }
                      var->name.copy(pszName);
                      addVariable(var);
                    }
                  }
                }
              }
            }
            //Remove elements
            if(strncmp("remove",(char*)cur_node->name,6)==0){
              if(cur_node->properties->name!=NULL){
                xmlAttr*node=cur_node->properties;
                char * pszType=NULL,*pszName=NULL;
                while(node!=NULL){
                  if(strncmp("name",(char*)node->name,4)==0)
                    pszName=(char*)node->children->content;
                  if(strncmp("type",(char*)node->name,4)==0)
                    pszType=(char*)node->children->content;
                  node=node->next;
                }
                //Check what the parentname of this attribute is:
                const char *attributeParentVarName = NULL;
                if(cur_node->parent){
                  if(cur_node->parent->properties){
                    xmlAttr *tempnode = cur_node->parent->properties;
                    while(tempnode!=NULL){
                      if(strncmp("name",(char*)tempnode->name,4)==0){
                        attributeParentVarName=(char*)tempnode->children->content;
                        break;
                      }
                      tempnode=tempnode->next;
                    }
                  }
                }
                if(pszType!=NULL&&pszName!=NULL){
                  if(strncmp(pszType,"variable",8)==0){
                    removeVariable(pszName);
                  }
                  //Check wether we want to remove an attribute
                  if(strncmp(pszType,"attribute",9)==0){
                    if(attributeParentVarName!=NULL){
                      CDF::Variable *var= getVariable(attributeParentVarName);
                      if(var!=NULL){
                        var->removeAttribute(pszName);
                      }
                    }else {
                      //Remove a global attribute
                      removeAttribute(pszName);
                    }
                  }
                }
              }
            }
            //Attribute elements
            if(strncmp("attribute",(char*)cur_node->name,9)==0){
              if(NCMLVarName==NULL)NCMLVarName=(char*)"NC_GLOBAL";
              if(NCMLVarName!=NULL){
                if(cur_node->properties->name!=NULL){
                  xmlAttr*node=cur_node->properties;
                  char * pszAttributeType=NULL,*pszAttributeName=NULL,*pszAttributeValue=NULL;
                  char * pszOrgName=NULL;
                  while(node!=NULL){
                    if(strncmp("name",(char*)node->name,4)==0)
                      pszAttributeName=(char*)node->children->content;
                    if(strncmp("type",(char*)node->name,4)==0)
                      pszAttributeType=(char*)node->children->content;
                    if(strncmp("value",(char*)node->name,5)==0)
                      pszAttributeValue=(char*)node->children->content;
                    if(strncmp("orgName",(char*)node->name,7)==0)
                      pszOrgName=(char*)node->children->content;
                    node=node->next;
                  }
                  if(pszAttributeName!=NULL){
                    //Rename an attribute
                    if(pszOrgName!=NULL){
                      CDF::Variable *var = getVariable(NCMLVarName);
                      if(var!=NULL){
                        CDF::Attribute *attr=var->getAttribute(pszOrgName);
                        if(attr!=NULL){
                          attr->name.copy(pszAttributeName);
                        }
                      }
                    }else{
                      //Add an attribute
                      if(pszAttributeType!=NULL&&pszAttributeValue!=NULL){
                        CDF::Variable *var = getVariable(NCMLVarName);
                        if(var == NULL){
                          var = new CDF::Variable();
                          var->name.copy(NCMLVarName);
                          addVariable(var);
                        }
                        CDFType attrType = ncmlTypeToCDFType(pszAttributeType);
                        if(strncmp("String",pszAttributeType,6)==0){
                          var->setAttribute(pszAttributeName,
                                            attrType,
                                            pszAttributeValue,
                                            strlen(pszAttributeValue));
                        }else{
                          size_t attrLen=0;
                          CT::string t=pszAttributeValue;
                          CT::string *t2=t.split(",");
                          attrLen=t2->count;
                          double values[attrLen];
                          for(size_t attrN=0;attrN<attrLen;attrN++){
                            values[attrN]=atof(t2[attrN].c_str());
                            CDBDebug("%f",values[attrN]);
                          }
                          delete[] t2;
                          //if(attrLen==3)exit(2);
                          
                          //double value=atof(pszAttributeValue);
                          CDF::Attribute *attr = new CDF::Attribute();
                          attr->name.copy(pszAttributeName);
                          var->addAttribute(attr);
                          attr->type=attrType;
                          CDF::allocateData(attrType,&attr->data,attrLen);
                          for(size_t attrN=0;attrN<attrLen;attrN++){
                            if(attrType==CDF_BYTE)((char*)attr->data)[attrN]=(char)values[attrN];
                            if(attrType==CDF_UBYTE)((unsigned char*)attr->data)[attrN]=(unsigned char)values[attrN];
                            if(attrType==CDF_CHAR)((char*)attr->data)[attrN]=(char)values[attrN];
                            if(attrType==CDF_SHORT)((short*)attr->data)[attrN]=(short)values[attrN];
                            if(attrType==CDF_USHORT)((unsigned short*)attr->data)[attrN]=(unsigned short)values[attrN];
                            if(attrType==CDF_INT)((int*)attr->data)[attrN]=(int)values[attrN];
                            if(attrType==CDF_UINT)((unsigned int*)attr->data)[attrN]=(unsigned int)values[attrN];
                            if(attrType==CDF_FLOAT)((float*)attr->data)[attrN]=(float)values[attrN];
                            if(attrType==CDF_DOUBLE)((double*)attr->data)[attrN]=(double)values[attrN];
                          }
                          attr->length=attrLen;
                        }
                      }
                    }
                  }
                }
              }
            }
        }
        putNCMLAttributes(cur_node->children);
      }
    }
  public:
    DEF_ERRORFUNCTION();
    CDFObject(){
      name.copy("NC_GLOBAL");
    }
    ~CDFObject(){
      for(size_t j=0;j<dimensions.size();j++){delete dimensions[j];dimensions[j]=NULL;}
      for(size_t j=0;j<variables.size();j++){delete variables[j];variables[j]=NULL;}
      //for(size_t j=0;j<attributes.size();j++){delete attributes[j];attributes[j]=NULL;}
    }
    std::vector<CDF::Dimension *> dimensions;
    //std::vector<CDF::Attribute *> attributes;
    std::vector<CDF::Variable *> variables;
    CT::string name;
    CDF::Variable * getVariable(const char *name){
      if(strncmp("NC_GLOBAL",name,9)==0){
        return this;
      }
      for(size_t j=0;j<variables.size();j++){
        if(variables[j]->name.equals(name)){
          return variables[j];
        }
      }
      return NULL;
    }
    int addVariable(CDF::Variable *var){
      var->id = variables.size();
      variables.push_back(var);
      return 0;
    }
    int removeVariable(const char *name){
      for(size_t j=0;j<variables.size();j++){
        if(variables[j]->name.equals(name)){
          delete variables[j];variables[j]=NULL;
          variables.erase(variables.begin()+j);
        }
      }
      return 0;
    }
    int removeDimension(const char *name){
      for(size_t j=0;j<dimensions.size();j++){
        if(dimensions[j]->name.equals(name)){
          delete dimensions[j];dimensions[j]=NULL;
          dimensions.erase(dimensions.begin()+j);
        }
      }
      return 0;
    }
    int addDimension(CDF::Dimension *dim){
      dim->id = dimensions.size();
      dimensions.push_back(dim);
      return 0;
    }
    CDF::Dimension * getDimension(const char *name){
      for(size_t j=0;j<dimensions.size();j++){
        if(dimensions[j]->name.equals(name)){
          return dimensions[j];
        }
      }
      return NULL;
    }
    int applyNCMLFile(const char * ncmlFileName){
      //The following ncml features have been implemented:
      // add a variable with <variable name=... type=.../>
      // add a attribute with <attribute name=... type=.../>
      // remove a variable with <remove name="..." type="variable"/>
      // remove a attribute with <remove name="..." type="attribute"/>
      // rename a variable with <variable name="LavaFlow" orgName="TDCSO2" />
      // rename a attribute with <attribute name="LavaFlow" orgName="TDCSO2" />
      int errorRaised=0;
      // Read the XML file and put the attributes into the data model
      xmlDoc *doc = NULL;
      NCMLVarName =NULL;
      xmlNode *root_element = NULL;
      LIBXML_TEST_VERSION;
      doc = xmlReadFile(ncmlFileName, NULL, 0);
      if (doc == NULL) {
        CDBError("Could not parse file \"%s\"", ncmlFileName);
        return 1;
      }
      root_element = xmlDocGetRootElement(doc);
      putNCMLAttributes(root_element);
      xmlFreeDoc(doc);
      xmlCleanupParser();
      if(errorRaised==1)return 1;
      return 0;
    }
};
namespace CDF{
  void dump(CDFObject* cdfObject,CT::string* dumpString);
  void _dumpPrintAttributes(const char *variableName, std::vector<CDF::Attribute *>attributes,CT::string *dumpString);
};

class CDFReader{
  public:
    CDFReader(CDFObject *cdfObject){
      this->cdfObject=cdfObject;
    }
    virtual ~CDFReader(){}
    CDFObject *cdfObject;
    virtual int open(const char *fileName) = 0;
    virtual int close() = 0;
    virtual int readVariableData(CDF::Variable *var, CDFType type) = 0;
    //Allocates and reads the variable data
    virtual int readVariableData(CDF::Variable *var,CDFType type,size_t *start,size_t *count,ptrdiff_t  *stride) = 0;
};

#endif
