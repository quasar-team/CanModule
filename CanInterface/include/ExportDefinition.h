#ifndef EXPORT_DEFINITION_H_
#define EXPORT_DEFINITION_H_

/**
* If building dll, visual studio needs to know which functions are externally visible
*/
#ifdef _WINDOWS
	#define SHARED_LIB_EXPORT_DEFN __declspec(dllexport)
#else
	#define SHARED_LIB_EXPORT_DEFN // nothing for linux
#endif //_WINDOWS

#endif /* EXPORT_DEFINITION_H_ */