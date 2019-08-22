# XMLPutter2 #

This code uses libXML2 to read data from an XML file and place it into a C struct.

This is essentially a copy of some very old (~2013) code that I wrote.
There are very few source code comments, it uses printf where it
ought not to, and the code is overall quite messy and probably buggy.
You have been warned.


## How to Use ##
You probably shouldn't use the code as-is, but if you really want to, here's how:

Your project needs to be set up to use libXML2. Then, you just need to make sure
XMLPutter2.h is in your include paths and that XMLPutter2.cpp gets compiled.

There is likely no point in turning it into a shared library (.so, .dll, etc).
The code is so small that it would just add extra clutter. However, there should
not be anything stopping you from compiling it as one if you so desire.

Loading something in code essentially looks as follows:

	XMLPutter2 p;
	p.setFile("myFile.xml");
	p.setRootElement("myrootelement");
	
	DynamicStructure ds;
	ds.setNodeName("MyNodeName");
	ds.addField("myfield", INT, 1); // 1 is number of elements, > 1 for array
	ds.finalizeStruct();
	
	p.setBaseStructure(&ds);
	
	myStruct *s = (myStruct*)p.readOne();


This will process a document like such:

```xml
	<?xml version="1.0"?>
	<myrootelement>
		<MyNodeName myfield="2"/>
	</myrootelement>
```


## License ##
Licensed under the Expat license: https://directory.fsf.org/wiki/License:Expat
