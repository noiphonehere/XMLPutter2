/*
Copyright (c) 2013, 2019 Carlo Retta

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

--- end of license text ---

XMLPutter2
----------
Uses libXML2 to read data from an XML document and place it into an
arbitrary C struct.
*/

#ifndef XMLPUTTER2_H
#define XMLPUTTER2_H

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <vector>

class DynamicStructure;
class XMLPutter2;

enum FieldType {
  INT,
  FLOAT,
  CHAR,
  SUBSTRUCTURE,
  SUBSTRUCTURE_COUNT,
  NONE
};

struct sField {
  char name[25];

  unsigned int offset;
  unsigned int size;

  FieldType type;
  unsigned int count;

  union {
    int *pInt;
    float *pFloat;
    char *pChar;
    unsigned char **ppLocalStore;
  };
  DynamicStructure *pDynStruct;
  unsigned int structCount;
};

class DynamicStructure
{
  //Node Name
  char name[25];

  //Structure stuff
  std::vector<sField> fields;
  bool isFinalized;

  //Functions
 public:
  friend class XMLPutter2; //FIX
  DynamicStructure();
  ~DynamicStructure();
  int totalStructSize();
  void freeFields();
  void allocFields();

  void addField(const char *name, FieldType type, unsigned int count, DynamicStructure *ds = NULL);
  void finalizeStruct();
  sField *findFieldDesc(const char *fieldName, FieldType force = NONE);
  void processFieldData(sField *field, xmlChar *data);
  void processSubstructures(xmlNodePtr children);
  void setNodeName(const char *name);
  const char *getNodeName();
  void fillFields(xmlAttrPtr attr);
  unsigned char *getStaticStruct();
};
class XMLPutter2
{
  //libXML2 Stuff
  xmlDocPtr doc;
  xmlNodePtr root;
  xmlNodePtr currentNode;

  //Structure
  DynamicStructure *base;

  //Functions
 public:
  XMLPutter2();
  ~XMLPutter2();
  void setFile(const char *fileName);
  void setRootElement(const char *rootName);
  void setBaseStructure(DynamicStructure *s);

  unsigned char *readOne();
};

#endif //XMLPUTTER2_h
