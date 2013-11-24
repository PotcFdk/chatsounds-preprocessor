#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "24";
	static const char MONTH[] = "11";
	static const char YEAR[] = "2013";
	static const char UBUNTU_VERSION_STYLE[] = "13.11";
	
	//Software Status
	static const char STATUS[] = "Beta";
	static const char STATUS_SHORT[] = "b";
	
	//Standard Version Type
	static const long MAJOR = 0;
	static const long MINOR = 3;
	static const long BUILD = 5;
	static const long REVISION = 10;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 388;
	#define RC_FILEVERSION 0,3,5,10
	#define RC_FILEVERSION_STRING "0, 3, 5, 10\0"
	static const char FULLVERSION_STRING[] = "0.3.5.10";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 2;
	

}
#endif //VERSION_H
