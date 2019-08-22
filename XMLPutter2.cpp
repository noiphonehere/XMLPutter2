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
*/

#include "XMLPutter2.h"
#include <string.h>
#include <stdio.h>

int DynamicStructure::totalStructSize()
{
  if(fields.size() == 0) return 0;
  return fields.back().offset + fields.back().size;
}

void DynamicStructure::freeFields()
{
  int i;
  for(i=0; i<fields.size(); i++) {
    delete [] fields[i].pInt; //Doesn't matter what type
    fields[i].pInt = NULL;
    fields[i].structCount = 0;
    if(fields[i].type == SUBSTRUCTURE) fields[i].pDynStruct->freeFields();
  }
}

void DynamicStructure::allocFields()
{
  int i;
  for(i=0; i<fields.size(); i++) {
    switch(fields[i].type) {

    case INT:
    case SUBSTRUCTURE_COUNT:
      fields[i].pInt = new int[fields[i].count];
      memset(fields[i].pInt, 0, sizeof(int)*fields[i].count);
      break;

    case FLOAT:
      fields[i].pFloat = new float[fields[i].count];
      memset(fields[i].pFloat, 0, sizeof(float)*fields[i].count);
      break;

    case CHAR:
      fields[i].pChar = new char[fields[i].count];
      memset(fields[i].pChar, 0, sizeof(char)*fields[i].count);
      break;

    case SUBSTRUCTURE:
      fields[i].pDynStruct->allocFields();
      fields[i].ppLocalStore = new unsigned char*[1];
      *(fields[i].ppLocalStore) = 0;
      fields[i].structCount = 0;
      break;

    default:
      break; //Should not happen, but quiet the compiler
    }
  }
}

sField *DynamicStructure::findFieldDesc(const char *fieldName, FieldType force)
{
  int i;
  for(i=0;i<fields.size();i++) {
    if(!strcmp(fieldName, fields[i].name))
      if(force == NONE || fields[i].type == force)
      return &(fields[i]);
  }
  return NULL;
}

void DynamicStructure::processFieldData(sField *field, xmlChar *data)
{
  int i, valLen;
  char *dPtr = (char *)data, *dTmp;

  switch(field->type) {
  case INT:
    for(i=0;i<field->count;i++) {
      sscanf(dPtr, "%d%n", &(field->pInt[i]), &valLen); //Read one int

      dTmp = strchr(dPtr, ' '); //Find out where the next space is
      if(dTmp) dPtr = dTmp; //If there is one, move up to it
      else dPtr += valLen; //Else, move to the end of the last read
    }
    break;

  case FLOAT:
    for(i=0;i<field->count;i++) {
      sscanf(dPtr, "%f%n", &(field->pFloat[i]), &valLen);

      dTmp = strchr(dPtr, ' ');
      if(dTmp) dPtr = dTmp;
      else dPtr += valLen;
    }
    break;

  case CHAR:
    strncpy(field->pChar, (const char *)data, field->count-1);
    break;
  default:
    break;
  }
}

void DynamicStructure::processSubstructures(xmlNodePtr children)
{
  sField *ssField; //The parent structure field representing the substructure
  sField *ssCountField; //Field representing count of substructures
  DynamicStructure *ss; //The dynamic substructure
  unsigned char *localStore; //Memory for the static substructure
  unsigned char *cLocalStore; //Pointer to struct list

  //Walk the linked list of substructures
  for(;children;children = children->next) {
    //Find the field for the substructure
    ssField = findFieldDesc((const char *)(children->name), SUBSTRUCTURE);
    ssCountField = findFieldDesc((const char*)(children->name), SUBSTRUCTURE_COUNT);
    //If it doesn't exist, just go to the next node
    if(!ssField) continue;

    ssField->structCount++; //Increment struct count
    ss = ssField->pDynStruct; //Get the dynamic structure of the field

    ss->allocFields(); //Allocate the substructure's fields
    ss->fillFields(children->properties); //Fill the fields with values
    ss->processSubstructures(children->children);
    localStore = ss->getStaticStruct(); //Get a static structure
    ss->freeFields(); //Free the dynamic fields

    cLocalStore = new unsigned char[ssField->structCount*ss->totalStructSize()];

    memcpy(cLocalStore, 
	   *(ssField->ppLocalStore),
	   ss->totalStructSize()*(ssField->structCount-1)
	   );

    memcpy(
	   cLocalStore + ( ss->totalStructSize()*(ssField->structCount-1) ),
	   localStore,
	   ss->totalStructSize()
	   );

    delete [] *(ssField->ppLocalStore);
    *(ssField->ppLocalStore) = cLocalStore;

    //Update the substructure count (if applicable)
    if(ssCountField) *(ssCountField->pInt) = ssField->structCount;
  }
}

DynamicStructure::DynamicStructure() : isFinalized(false)
{
  memset(name, 0, sizeof(char)*25);
}
DynamicStructure::~DynamicStructure()
{
  freeFields();
}

void DynamicStructure::setNodeName(const char *name)
{
  strncpy(this->name, name, 24);
}
const char *DynamicStructure::getNodeName()
{
  return name;
}

XMLPutter2::XMLPutter2() : doc(NULL), root(NULL), currentNode(NULL)
{
}

XMLPutter2::~XMLPutter2()
{
}

void XMLPutter2::setFile(const char *fileName)
{
  doc = xmlReadFile(fileName, NULL, 0);
  if(!doc) printf("Error opening XML document: %s\n", fileName);
  else {
    root = xmlDocGetRootElement(doc);
    if(!root) {
      printf("Error: Document has no root element: %s\n", fileName);
    }
    else {
      currentNode = root->children;
    }
  }
}

void XMLPutter2::setRootElement(const char *rootName)
{
  if(!doc) printf("You need to specify the file before specifying the root name.\n");
  else {
    if(xmlStrcmp(root->name, (xmlChar*)rootName))
      printf("Root name does not match specified one (%s != %s)! Might have specified the wrong document! Crash could occur...\n", root->name, rootName);
  }
}

void XMLPutter2::setBaseStructure(DynamicStructure *s)
{
  base = s;
}

void DynamicStructure::addField(const char *name, FieldType type, unsigned int count, DynamicStructure *ds)
{
  if(isFinalized) {
    printf("Can't add fields after finalize!\n");
    return;
  }
  sField t;
  strncpy(t.name, name, 24);
  t.type = type;
  t.count = count;
  t.offset = totalStructSize();

  int alignment = 1;

  switch(type) {
  case INT:
  case SUBSTRUCTURE_COUNT:
    t.size = count*sizeof(int);
    alignment = sizeof(int);
    break;

  case FLOAT:
    t.size = count*sizeof(float);
    alignment = sizeof(float);
    break;

  case CHAR:
    t.size = count*sizeof(char);
    alignment = sizeof(char);
    break;

  case SUBSTRUCTURE:
    t.size = sizeof(unsigned char*); //Want only one (for now)
    alignment = sizeof(unsigned char*);
    t.pDynStruct = ds;
    printf("WARNING: Substructure is experimental!\n");
    break;

  default:
    printf("Uh-oh...\n");
    t.size = count;
    break;
  }
  
  if(t.offset%alignment) {
    t.offset += alignment - (t.offset%alignment);
  }

  fields.push_back(t);
}

void DynamicStructure::finalizeStruct()
{
  isFinalized = true;
}
void DynamicStructure::fillFields(xmlAttrPtr attr)
{
  sField *currentField;
  if(!attr) printf("No attributes?!\n");
  while(attr) {
    currentField = findFieldDesc((const char *)attr->name);
    if(!currentField) {
      printf("Found unhandled field %s in document!\n", attr->name);
      attr = attr->next;
      continue;
    }

    processFieldData(currentField, attr->children->content);
    attr = attr->next;

  }
}

unsigned char *DynamicStructure::getStaticStruct()
{
  unsigned char *localStore = new unsigned char[totalStructSize()];
  int i;

  for(i=0;i<fields.size();i++) {
      memcpy(localStore+fields[i].offset, fields[i].pInt, fields[i].size);
  }
  return localStore;
}
unsigned char *XMLPutter2::readOne()
{
  while(currentNode && 
	(currentNode->type != XML_ELEMENT_NODE || 
	 strcmp((const char*)(currentNode->name), base->getNodeName() ) )
	) {
    currentNode = currentNode->next;
  }
  if(!currentNode) return NULL;

  unsigned char *localStore;
  xmlAttrPtr attr = currentNode->properties;


  base->allocFields();

  base->fillFields(attr);
  base->processSubstructures(currentNode->children);

  localStore = base->getStaticStruct();

  base->freeFields();

  currentNode = currentNode->next;
  return localStore;
}
