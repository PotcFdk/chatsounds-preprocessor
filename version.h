#ifndef VERSION_H
#define VERSION_H

#ifndef DISTRIBUTION_VERSION
#define DISTRIBUTION_VERSION alpha1
#endif

namespace Version{
	
	//Date Version Types
	static const char DATE[] = "07";
	static const char MONTH[] = "07";
	static const char YEAR[] = "2017";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 0;
	static const long BUILD  = 0;
	static const long REVISION  = 0;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 949;

}
#endif //VERSION_H
