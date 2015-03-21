#line 1 "c:\\p4\\code\\Tools\\FBuild\\FBuildTest\\Tests\\TestIncludeParser.cpp"





#line 1 "C:\\p4\\Code\\TestFramework/UnitTest.h"


#pragma once



#line 1 "c:\\p4\\code\\testframework\\UnitTestManager.h"


#pragma once





#line 1 "C:\\p4\\Code\\Core/Env/Assert.h"


#pragma once





#line 1 "C:\\p4\\Code\\Core/Env/Types.h"


#pragma once



typedef unsigned char		uint8_t;
typedef signed char			int8_t;
typedef unsigned short		uint16_t;
typedef signed short		int16_t;
typedef unsigned int		uint32_t;
typedef signed int			int32_t;
typedef unsigned long long	uint64_t;
typedef signed long long	int64_t;




















    

    




#line 43 "C:\\p4\\Code\\Core/Env/Types.h"


#line 46 "C:\\p4\\Code\\Core/Env/Types.h"
#line 10 "C:\\p4\\Code\\Core/Env/Assert.h"




	

	
	












	
	

	
	





	template<bool> struct compile_time_assert_failure;
	template<> struct compile_time_assert_failure<true>{};

	class AssertHandler
	{
	public:
		static void SetThrowOnAssert( bool throwOnAssert )
		{
			s_ThrowOnAssert = throwOnAssert;
		}
		static bool Failure( const char * message, 
							 const char * file,  
							 const int line );

		static bool s_ThrowOnAssert;
	};

























#line 82 "C:\\p4\\Code\\Core/Env/Assert.h"


#line 85 "C:\\p4\\Code\\Core/Env/Assert.h"
#line 10 "c:\\p4\\code\\testframework\\UnitTestManager.h"




class UnitTest;



class UnitTestManager
{
public:
	UnitTestManager();
	~UnitTestManager();

	
	bool RunTests( const char * testGroup = nullptr );

	
	


		static		  UnitTestManager &	Get();
	#line 33 "c:\\p4\\code\\testframework\\UnitTestManager.h"
	static inline bool					IsValid() { return ( s_Instance != 0 ); }

	
	static void RegisterTestGroup( UnitTest * testGroup );
	static void DeRegisterTestGroup( UnitTest * testGroup );

	
	void TestBegin( const char * testName );
	void TestEnd();

	
	static bool AssertFailure( const char * message, const char * file, uint32_t line );

private:
	uint32_t	m_TestsRun;
	uint32_t	m_TestsPassed;
	const char * m_CurrentTestName;

	static UnitTestManager * s_Instance;
	static UnitTest * s_FirstTest; 
};


#line 57 "c:\\p4\\code\\testframework\\UnitTestManager.h"
#line 8 "C:\\p4\\Code\\TestFramework/UnitTest.h"





class UnitTest
{
protected:
	explicit		UnitTest() { m_NextTestGroup = nullptr; }
	inline virtual ~UnitTest() {}

	virtual void RunTests() = 0;
	virtual const char * GetName() const = 0;
	virtual bool RunThisTestOnly() const { return false; }

private:
	friend class UnitTestManager;
	UnitTest * m_NextTestGroup;
};



























































#line 87 "C:\\p4\\Code\\TestFramework/UnitTest.h"
#line 7 "c:\\p4\\code\\Tools\\FBuild\\FBuildTest\\Tests\\TestIncludeParser.cpp"

#line 1 "C:\\p4\\Code\\Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"



#pragma once





#line 1 "C:\\p4\\Code\\Core/Containers/Array.h"


#pragma once





#line 1 "C:\\p4\\Code\\Core/Containers/Sort.h"


#pragma once









class AscendingCompare
{
public:
	template < class T >
	inline bool operator () ( const T & a, const T & b ) const
	{
		return ( a < b );
	}
};



class AscendingCompareDeref
{
public:
	template < class T >
	inline bool operator () ( const T & a, const T & b ) const
	{
		return ( ( *a ) < ( *b ) );
	}
};



template < class T, class COMPARE >
void ShellSort( T * begin, T * end, const COMPARE & compare )
{
	size_t numItems = end - begin;
	size_t increment = 3;
	while ( increment > 0 )
	{
		for ( size_t i=0; i < numItems; i++ )
		{
			size_t j = i;
			T temp( begin[ i ] );
			while ( ( j >= increment ) && ( compare( temp, begin[ j - increment ]  ) ) )
			{
				begin[ j ] = begin[ j - increment ];
				j = j - increment;
			}
			begin[ j ] = temp;
		}
		if ( increment / 2 != 0 )
		{
			increment = increment / 2 ;
		}
		else if ( increment == 1 )
		{
			increment = 0;
		}
		else
		{
			increment = 1;
		}
	}    
}


#line 72 "C:\\p4\\Code\\Core/Containers/Sort.h"
#line 10 "C:\\p4\\Code\\Core/Containers/Array.h"


#line 1 "C:\\p4\\Code\\Core/Math/Conversions.h"


#pragma once









class Math
{
public:
	static inline uint16_t RoundUp( uint16_t value, uint16_t alignment )
	{
		return ( value + alignment - 1) & ~( alignment - 1 );
	}
	static inline uint32_t RoundUp( uint32_t value, uint32_t alignment )
	{
		return ( value + alignment - 1) & ~( alignment - 1 );
	}
	static inline uint64_t RoundUp( uint64_t value, uint64_t alignment )
	{
		return ( value + alignment - 1) & ~( alignment - 1 );
	}
	template <class T>
	static inline T Max( T a, T b )
	{
		return ( a > b ) ? a : b;
	}
	template <class T>
	static inline T Min( T a, T b )
	{
		return ( a < b ) ? a : b;
	}
};


#line 42 "C:\\p4\\Code\\Core/Math/Conversions.h"
#line 13 "C:\\p4\\Code\\Core/Containers/Array.h"

#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"

#pragma once



#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

#pragma once




#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

#pragma once



#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"















#pragma once




#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


















#line 20 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 25 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 26 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 27 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

























#line 53 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 57 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"










#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"













#pragma once







































































































































#line 151 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"



#line 155 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"






































#line 194 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"


#line 197 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"

#line 199 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"





#line 205 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"



#line 209 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"






#line 216 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"











#line 228 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"








#line 237 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"
#line 238 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"































































































































































































































































































































































































































































































#pragma region Input Buffer SAL 1 compatibility macros




































































































































































































































































































































































































































































                                                




                                                

















































































































































































































































































































#pragma endregion Input Buffer SAL 1 compatibility macros

















































































#line 1564 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"






























#line 1595 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"
























#line 1620 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"












#line 1633 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"






































#line 1672 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"


























































































































#line 1795 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"






































































































#line 1898 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"








































































































































































#line 2067 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"





































































































#line 2169 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"



















































































































































































































#line 2381 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"
extern "C" {




#line 2387 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"



































































































































































































































#line 2615 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    

    
    

#line 2654 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"









































































































































































































































#line 2888 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"









#line 2898 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"


    
    
#line 2903 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"






#line 2910 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"
#line 2911 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"






#line 2918 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"
#line 2919 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"











#line 2931 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"

































#line 2965 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"






















}
#line 2989 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"

#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\ConcurrencySal.h"


















#pragma once


extern "C" {
#line 24 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\ConcurrencySal.h"
















































































































































































































































#line 265 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\ConcurrencySal.h"



#line 269 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\ConcurrencySal.h"


















































































#line 352 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\ConcurrencySal.h"


}
#line 356 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\ConcurrencySal.h"

#line 358 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\ConcurrencySal.h"
#line 2991 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\sal.h"


#line 68 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#pragma pack(push,8)

#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"












#pragma once






#line 21 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"







#pragma pack(push,8)


extern "C" {
#line 33 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"










#line 44 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"
#line 45 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"





typedef __w64 unsigned int   uintptr_t;
#line 52 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"

#line 54 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"





typedef char *  va_list;
#line 61 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"

#line 63 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"





#line 69 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"







#line 77 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"


#line 80 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"













#line 94 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"












































#line 139 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"


}
#line 143 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"

#pragma pack(pop)

#line 147 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\vadefs.h"
#line 75 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


extern "C" {
#line 79 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





#line 85 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 90 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 95 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







#line 103 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



















#line 123 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


#line 126 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 128 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 129 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 130 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




















#line 151 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"









#line 161 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 162 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 164 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"










#line 175 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 177 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 178 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





#line 184 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 186 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 191 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 193 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 195 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 196 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





#line 202 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 204 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 205 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 210 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 212 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 213 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 218 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 220 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
  
#line 222 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 223 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"










#line 234 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 235 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 242 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 243 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 250 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

















#line 268 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 273 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"








#line 282 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 289 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 290 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





#line 296 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 303 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 304 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 311 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 312 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 317 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


#line 320 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 322 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 323 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 324 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





#line 330 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"










#line 341 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 343 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 344 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 345 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"








#line 354 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





#line 360 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 367 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 368 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 370 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
















#line 387 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 388 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 392 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 399 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 400 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 407 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 414 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 416 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 417 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




  




#line 427 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 428 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



  
  




#line 438 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 439 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




   


#line 447 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 452 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 453 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



  




#line 462 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 463 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



  




#line 472 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 473 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 477 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





#line 483 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 488 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 490 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 491 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





typedef __w64 unsigned int   size_t;
#line 498 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 500 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



typedef size_t rsize_t;

#line 506 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 507 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





typedef __w64 int            intptr_t;
#line 514 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 516 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"














typedef __w64 int            ptrdiff_t;
#line 532 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 534 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







typedef unsigned short wint_t;
typedef unsigned short wctype_t;

#line 545 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


















typedef int errno_t;
#line 565 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


typedef __w64 long __time32_t;   

#line 570 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


typedef __int64 __time64_t;     

#line 575 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





typedef __time64_t time_t;      
#line 582 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 584 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







#line 592 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 593 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 597 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 599 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 604 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 606 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 607 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"





#line 613 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 617 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"




#line 622 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 624 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 625 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"










  void __cdecl _invalid_parameter(  const wchar_t *,   const wchar_t *,   const wchar_t *, unsigned int, uintptr_t);



#line 640 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

 __declspec(noreturn)
void __cdecl _invoke_watson(  const wchar_t *,   const wchar_t *,   const wchar_t *, unsigned int, uintptr_t);



  
#line 648 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"












#line 661 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


































#line 696 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


































































































































































#line 859 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 860 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"









































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 1926 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"















































































































































#line 2070 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"
#line 2071 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

struct threadlocaleinfostruct;
struct threadmbcinfostruct;
typedef struct threadlocaleinfostruct * pthreadlocinfo;
typedef struct threadmbcinfostruct * pthreadmbcinfo;
struct __lc_time_data;

typedef struct localeinfo_struct
{
    pthreadlocinfo locinfo;
    pthreadmbcinfo mbcinfo;
} _locale_tstruct, *_locale_t;


typedef struct localerefcount {
        char *locale;
        wchar_t *wlocale;
        int *refcount;
        int *wrefcount;
} locrefcount;

typedef struct threadlocaleinfostruct {
        int refcount;
        unsigned int lc_codepage;
        unsigned int lc_collate_cp;
        unsigned int lc_time_cp;
        locrefcount lc_category[6];
        int lc_clike;
        int mb_cur_max;
        int * lconv_intl_refcount;
        int * lconv_num_refcount;
        int * lconv_mon_refcount;
        struct lconv * lconv;
        int * ctype1_refcount;
        unsigned short * ctype1;
        const unsigned short * pctype;
        const unsigned char * pclmap;
        const unsigned char * pcumap;
        struct __lc_time_data * lc_time_curr;
        wchar_t * locale_name[6];
} threadlocinfo;

#line 2114 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"


}
#line 2118 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 2122 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 2124 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 2128 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 2130 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 2134 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 2136 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"






#line 2143 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"



#line 2147 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#pragma pack(pop)

#line 2151 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"

#line 22 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"














#pragma once

#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 18 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"















#line 34 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"









































#line 76 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"






#line 83 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"
#line 84 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"




#line 89 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"
#line 90 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"


#line 93 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\limits.h"
#line 23 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"





#pragma pack(push,8)


extern "C" {
#line 33 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"







#line 41 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
#line 42 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"










typedef int (__cdecl * _onexit_t)(void);



#line 57 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"



#line 61 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"




#line 66 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"


#line 69 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"






typedef struct _div_t {
        int quot;
        int rem;
} div_t;

typedef struct _ldiv_t {
        long quot;
        long rem;
} ldiv_t;

typedef struct _lldiv_t {
        long long quot;
        long long rem;
} lldiv_t;


#line 92 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"










#pragma pack(4)
typedef struct {
    unsigned char ld[10];
} _LDOUBLE;
#pragma pack()













#line 121 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

typedef struct {
        double x;
} _CRT_DOUBLE;

typedef struct {
    float f;
} _CRT_FLOAT;





typedef struct {
        


        long double x;
} _LONGDOUBLE;



#pragma pack(4)
typedef struct {
    unsigned char ld12[12];
} _LDBL12;
#pragma pack()


#line 151 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"



















 extern int __mb_cur_max;



#line 175 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
 int __cdecl ___mb_cur_max_func(void);
 int __cdecl ___mb_cur_max_l_func(_locale_t);
#line 178 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"





































typedef void (__cdecl *_purecall_handler)(void);


 _purecall_handler __cdecl _set_purecall_handler(  _purecall_handler _Handler);
 _purecall_handler __cdecl _get_purecall_handler(void);
#line 221 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"


extern "C++"
{




#line 230 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
}
#line 232 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"



typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t);


 _invalid_parameter_handler __cdecl _set_invalid_parameter_handler(  _invalid_parameter_handler _Handler);
 _invalid_parameter_handler __cdecl _get_invalid_parameter_handler(void);
#line 241 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"




 extern int * __cdecl _errno(void);


errno_t __cdecl _set_errno(  int _Value);
errno_t __cdecl _get_errno(  int * _Value);
#line 251 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

 unsigned long * __cdecl __doserrno(void);


errno_t __cdecl _set_doserrno(  unsigned long _Value);
errno_t __cdecl _get_doserrno(  unsigned long * _Value);


 __declspec(deprecated("This function or variable may be unsafe. Consider using " "strerror" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) char ** __cdecl __sys_errlist(void);


 __declspec(deprecated("This function or variable may be unsafe. Consider using " "strerror" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) int * __cdecl __sys_nerr(void);




















#line 284 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"




 extern int __argc;          
 extern char ** __argv;      
 extern wchar_t ** __wargv;  







#line 299 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

#line 301 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"





 extern char ** _environ;    
 extern wchar_t ** _wenviron;    

#line 310 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

__declspec(deprecated("This function or variable may be unsafe. Consider using " "_get_pgmptr" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  extern char * _pgmptr;      
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_get_wpgmptr" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  extern wchar_t * _wpgmptr;  
























#line 338 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

errno_t __cdecl _get_pgmptr(  char ** _Value);
errno_t __cdecl _get_wpgmptr(  wchar_t ** _Value);



#line 345 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

__declspec(deprecated("This function or variable may be unsafe. Consider using " "_get_fmode" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  extern int _fmode;          



#line 351 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
#line 352 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

 errno_t __cdecl _set_fmode(  int _Mode);
 errno_t __cdecl _get_fmode(  int * _PMode);





#line 361 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
extern "C++"
{
template <typename _CountofType, size_t _SizeOfArray>
char (*__countof_helper( _CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];

}
#line 368 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
#line 369 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"







 __declspec(noreturn) void __cdecl exit(  int _Code);

 __declspec(noreturn) void __cdecl _exit(  int _Code);
 __declspec(noreturn) void __cdecl abort(void);
#line 381 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

 unsigned int __cdecl _set_abort_behavior(  unsigned int _Flags,   unsigned int _Mask);

int       __cdecl abs(  int _X);
long      __cdecl labs(  long _X);
long long __cdecl llabs(  long long _X);

        __int64    __cdecl _abs64(__int64);




















#line 410 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"















#line 426 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
        int    __cdecl atexit(void (__cdecl *)(void));
#line 428 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
   double  __cdecl atof(  const char *_String);
   double  __cdecl _atof_l(  const char *_String,   _locale_t _Locale);
    int    __cdecl atoi(  const char *_Str);
   int    __cdecl _atoi_l(  const char *_Str,   _locale_t _Locale);
   long   __cdecl atol(  const char *_Str);
   long   __cdecl _atol_l(  const char *_Str,   _locale_t _Locale);
   long long __cdecl atoll(  const char *_Str);
   long long __cdecl _atoll_l(  const char *_Str,   _locale_t _Locale);



   void * __cdecl bsearch_s(  const void * _Key,   const void * _Base,
          rsize_t _NumOfElements,   rsize_t _SizeOfElements,
          int (__cdecl * _PtFuncCompare)(void *, const void *, const void *), void * _Context);
#line 443 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
   void * __cdecl bsearch(  const void * _Key,   const void * _Base,
          size_t _NumOfElements,   size_t _SizeOfElements,
          int (__cdecl * _PtFuncCompare)(const void *, const void *));


 void __cdecl qsort_s(  void * _Base,
          rsize_t _NumOfElements,   rsize_t _SizeOfElements,
          int (__cdecl * _PtFuncCompare)(void *, const void *, const void *), void *_Context);
#line 452 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
 void __cdecl qsort(  void * _Base,
          size_t _NumOfElements,   size_t _SizeOfElements,
          int (__cdecl * _PtFuncCompare)(const void *, const void *));
#line 456 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
          unsigned short __cdecl _byteswap_ushort(  unsigned short _Short);
          unsigned long  __cdecl _byteswap_ulong (  unsigned long _Long);
          unsigned __int64 __cdecl _byteswap_uint64(  unsigned __int64 _Int64);
   div_t  __cdecl div(  int _Numerator,   int _Denominator);


   __declspec(deprecated("This function or variable may be unsafe. Consider using " "_dupenv_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) char * __cdecl getenv(  const char * _VarName);

  errno_t __cdecl getenv_s(  size_t * _ReturnSize,   char * _DstBuf,   rsize_t _DstSize,   const char * _VarName);
#line 466 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl getenv_s(  size_t * _ReturnSize, char (&_Dest)[_Size],   const char * _VarName) throw() { return getenv_s(_ReturnSize, _Dest, _Size, _VarName); } }



#line 471 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

  errno_t __cdecl _dupenv_s(    char **_PBuffer,   size_t * _PBufferSizeInBytes,   const char * _VarName);



#line 477 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
#line 478 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

  errno_t __cdecl _itoa_s(  int _Value,   char * _DstBuf,   size_t _Size,   int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _itoa_s(  int _Value, char (&_Dest)[_Size],   int _Radix) throw() { return _itoa_s(_Value, _Dest, _Size, _Radix); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_itoa_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _itoa( int _Value,   char *_Dest,  int _Radix);
  errno_t __cdecl _i64toa_s(  __int64 _Val,   char * _DstBuf,   size_t _Size,   int _Radix);
 __declspec(deprecated("This function or variable may be unsafe. Consider using " "_i64toa_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) char * __cdecl _i64toa(  __int64 _Val,     char * _DstBuf,   int _Radix);
  errno_t __cdecl _ui64toa_s(  unsigned __int64 _Val,   char * _DstBuf,   size_t _Size,   int _Radix);
 __declspec(deprecated("This function or variable may be unsafe. Consider using " "_ui64toa_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) char * __cdecl _ui64toa(  unsigned __int64 _Val,     char * _DstBuf,   int _Radix);
   __int64 __cdecl _atoi64(  const char * _String);
   __int64 __cdecl _atoi64_l(  const char * _String,   _locale_t _Locale);
   __int64 __cdecl _strtoi64(  const char * _String,     char ** _EndPtr,   int _Radix);
   __int64 __cdecl _strtoi64_l(  const char * _String,     char ** _EndPtr,   int _Radix,   _locale_t _Locale);
   unsigned __int64 __cdecl _strtoui64(  const char * _String,     char ** _EndPtr,   int _Radix);
   unsigned __int64 __cdecl _strtoui64_l(  const char * _String,     char ** _EndPtr,   int  _Radix,   _locale_t _Locale);
   ldiv_t __cdecl ldiv(  long _Numerator,   long _Denominator);
   lldiv_t __cdecl lldiv(  long long _Numerator,   long long _Denominator);

extern "C++"
{
    inline long abs(long _X) throw()
    {
        return labs(_X);
    }
    inline long long abs(long long _X) throw()
    {
        return llabs(_X);
    }
    inline ldiv_t div(long _A1, long _A2) throw()
    {
        return ldiv(_A1, _A2);
    }
    inline lldiv_t div(long long _A1, long long _A2) throw()
    {
        return lldiv(_A1, _A2);
    }
}
#line 515 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
  errno_t __cdecl _ltoa_s(  long _Val,   char * _DstBuf,   size_t _Size,   int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ltoa_s(  long _Value, char (&_Dest)[_Size],   int _Radix) throw() { return _ltoa_s(_Value, _Dest, _Size, _Radix); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_ltoa_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _ltoa( long _Value,   char *_Dest,  int _Radix);
   int    __cdecl mblen(    const char * _Ch,   size_t _MaxCount);
   int    __cdecl _mblen_l(    const char * _Ch,   size_t _MaxCount,   _locale_t _Locale);
   size_t __cdecl _mbstrlen(  const char * _Str);
   size_t __cdecl _mbstrlen_l(  const char *_Str,   _locale_t _Locale);
   size_t __cdecl _mbstrnlen(  const char *_Str,   size_t _MaxCount);
   size_t __cdecl _mbstrnlen_l(  const char *_Str,   size_t _MaxCount,   _locale_t _Locale);
 int    __cdecl mbtowc(    wchar_t * _DstCh,     const char * _SrcCh,   size_t _SrcSizeInBytes);
 int    __cdecl _mbtowc_l(    wchar_t * _DstCh,     const char * _SrcCh,   size_t _SrcSizeInBytes,   _locale_t _Locale);
  errno_t __cdecl mbstowcs_s(  size_t * _PtNumOfCharConverted,   wchar_t * _DstBuf,   size_t _SizeInWords,   const char * _SrcBuf,   size_t _MaxCount );
extern "C++" { template <size_t _Size> inline errno_t __cdecl mbstowcs_s(  size_t * _PtNumOfCharConverted,   wchar_t (&_Dest)[_Size],   const char * _Source,   size_t _MaxCount) throw() { return mbstowcs_s(_PtNumOfCharConverted, _Dest, _Size, _Source, _MaxCount); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "mbstowcs_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  size_t __cdecl mbstowcs( wchar_t *_Dest,  const char * _Source,  size_t _MaxCount);

  errno_t __cdecl _mbstowcs_s_l(  size_t * _PtNumOfCharConverted,   wchar_t * _DstBuf,   size_t _SizeInWords,   const char * _SrcBuf,   size_t _MaxCount,   _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _mbstowcs_s_l(  size_t * _PtNumOfCharConverted, wchar_t (&_Dest)[_Size],   const char * _Source,   size_t _MaxCount,   _locale_t _Locale) throw() { return _mbstowcs_s_l(_PtNumOfCharConverted, _Dest, _Size, _Source, _MaxCount, _Locale); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_mbstowcs_s_l" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  size_t __cdecl _mbstowcs_l(  wchar_t *_Dest,   const char * _Source,   size_t _MaxCount,   _locale_t _Locale);

   int    __cdecl rand(void);


#line 538 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

  int    __cdecl _set_error_mode(  int _Mode);

 void   __cdecl srand(  unsigned int _Seed);
   double __cdecl strtod(  const char * _Str,     char ** _EndPtr);
   double __cdecl _strtod_l(  const char * _Str,     char ** _EndPtr,   _locale_t _Locale);
   long   __cdecl strtol(  const char * _Str,     char ** _EndPtr,   int _Radix );
   long   __cdecl _strtol_l(  const char *_Str,     char **_EndPtr,   int _Radix,   _locale_t _Locale);
   long long  __cdecl strtoll(  const char * _Str,     char ** _EndPtr,   int _Radix );
   long long  __cdecl _strtoll_l(  const char * _Str,     char ** _EndPtr,   int _Radix,   _locale_t _Locale );
   unsigned long __cdecl strtoul(  const char * _Str,     char ** _EndPtr,   int _Radix);
   unsigned long __cdecl _strtoul_l(const char * _Str,     char **_EndPtr,   int _Radix,   _locale_t _Locale);
   unsigned long long __cdecl strtoull(  const char * _Str,     char ** _EndPtr,   int _Radix);
   unsigned long long __cdecl _strtoull_l(  const char * _Str,     char ** _EndPtr,   int _Radix,   _locale_t _Locale);
   long double __cdecl strtold(  const char * _Str,     char ** _EndPtr);
   long double __cdecl _strtold_l(  const char * _Str,     char ** _EndPtr,   _locale_t _Locale);
   float __cdecl strtof(  const char * _Str,     char ** _EndPtr);
   float __cdecl _strtof_l(  const char * _Str,     char ** _EndPtr,   _locale_t _Locale);




 int __cdecl system(  const char * _Command);
#line 562 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
#line 563 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

  errno_t __cdecl _ultoa_s(  unsigned long _Val,   char * _DstBuf,   size_t _Size,   int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ultoa_s(  unsigned long _Value, char (&_Dest)[_Size],   int _Radix) throw() { return _ultoa_s(_Value, _Dest, _Size, _Radix); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_ultoa_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _ultoa( unsigned long _Value,   char *_Dest,  int _Radix);
 __declspec(deprecated("This function or variable may be unsafe. Consider using " "wctomb_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) int    __cdecl wctomb(  char * _MbCh,   wchar_t _WCh);
 __declspec(deprecated("This function or variable may be unsafe. Consider using " "_wctomb_s_l" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) int    __cdecl _wctomb_l(    char * _MbCh,   wchar_t _WCh,   _locale_t _Locale);

  errno_t __cdecl wctomb_s(  int * _SizeConverted,   char * _MbCh,   rsize_t _SizeInBytes,   wchar_t _WCh);
#line 572 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
  errno_t __cdecl _wctomb_s_l(  int * _SizeConverted,   char * _MbCh,   size_t _SizeInBytes,   wchar_t _WCh,   _locale_t _Locale);
  errno_t __cdecl wcstombs_s(  size_t * _PtNumOfCharConverted,   char * _Dst,   size_t _DstSizeInBytes,   const wchar_t * _Src,   size_t _MaxCountInBytes);
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcstombs_s(  size_t * _PtNumOfCharConverted,   char (&_Dest)[_Size],   const wchar_t * _Source,   size_t _MaxCount) throw() { return wcstombs_s(_PtNumOfCharConverted, _Dest, _Size, _Source, _MaxCount); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "wcstombs_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  size_t __cdecl wcstombs( char *_Dest,  const wchar_t * _Source,  size_t _MaxCount);
  errno_t __cdecl _wcstombs_s_l(  size_t * _PtNumOfCharConverted,   char * _Dst,   size_t _DstSizeInBytes,   const wchar_t * _Src,   size_t _MaxCountInBytes,   _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcstombs_s_l(  size_t * _PtNumOfCharConverted,   char (&_Dest)[_Size],   const wchar_t * _Source,   size_t _MaxCount,   _locale_t _Locale) throw() { return _wcstombs_s_l(_PtNumOfCharConverted, _Dest, _Size, _Source, _MaxCount, _Locale); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wcstombs_s_l" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  size_t __cdecl _wcstombs_l(  char *_Dest,   const wchar_t * _Source,   size_t _MaxCount,   _locale_t _Locale);

























#line 605 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"


































#line 640 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
        __declspec(noalias) __declspec(restrict)    void * __cdecl calloc(   size_t _Count,    size_t _Size);
                     __declspec(noalias)                                                                             void   __cdecl free(    void * _Memory);
        __declspec(noalias) __declspec(restrict)                              void * __cdecl malloc(   size_t _Size);
 
       __declspec(noalias) __declspec(restrict)                           void * __cdecl realloc(    void * _Memory,    size_t _NewSize);
 
       __declspec(noalias) __declspec(restrict)                       void * __cdecl _recalloc(    void * _Memory,    size_t _Count,    size_t _Size);
                     __declspec(noalias)                                                                             void   __cdecl _aligned_free(    void * _Memory);
       __declspec(noalias) __declspec(restrict)                              void * __cdecl _aligned_malloc(   size_t _Size,   size_t _Alignment);
       __declspec(noalias) __declspec(restrict)                              void * __cdecl _aligned_offset_malloc(   size_t _Size,   size_t _Alignment,   size_t _Offset);
 
       __declspec(noalias) __declspec(restrict)                              void * __cdecl _aligned_realloc(    void * _Memory,    size_t _NewSize,   size_t _Alignment);
 
       __declspec(noalias) __declspec(restrict)                       void * __cdecl _aligned_recalloc(    void * _Memory,    size_t _Count,    size_t _Size,   size_t _Alignment);
 
       __declspec(noalias) __declspec(restrict)                              void * __cdecl _aligned_offset_realloc(    void * _Memory,    size_t _NewSize,   size_t _Alignment,   size_t _Offset);
 
       __declspec(noalias) __declspec(restrict)                       void * __cdecl _aligned_offset_recalloc(    void * _Memory,    size_t _Count,    size_t _Size,   size_t _Alignment,   size_t _Offset);
                                                    size_t __cdecl _aligned_msize(  void * _Memory,   size_t _Alignment,   size_t _Offset);


















#line 678 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

#line 680 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"





  errno_t __cdecl _itow_s (  int _Val,   wchar_t * _DstBuf,   size_t _SizeInWords,   int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _itow_s(  int _Value, wchar_t (&_Dest)[_Size],   int _Radix) throw() { return _itow_s(_Value, _Dest, _Size, _Radix); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_itow_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _itow( int _Value,   wchar_t *_Dest,  int _Radix);
  errno_t __cdecl _ltow_s (  long _Val,   wchar_t * _DstBuf,   size_t _SizeInWords,   int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ltow_s(  long _Value, wchar_t (&_Dest)[_Size],   int _Radix) throw() { return _ltow_s(_Value, _Dest, _Size, _Radix); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_ltow_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _ltow( long _Value,   wchar_t *_Dest,  int _Radix);
  errno_t __cdecl _ultow_s (  unsigned long _Val,   wchar_t * _DstBuf,   size_t _SizeInWords,   int _Radix);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ultow_s(  unsigned long _Value, wchar_t (&_Dest)[_Size],   int _Radix) throw() { return _ultow_s(_Value, _Dest, _Size, _Radix); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_ultow_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _ultow( unsigned long _Value,   wchar_t *_Dest,  int _Radix);
   double __cdecl wcstod(  const wchar_t * _Str,     wchar_t ** _EndPtr);
   double __cdecl _wcstod_l(  const wchar_t *_Str,     wchar_t ** _EndPtr,   _locale_t _Locale);
   long   __cdecl wcstol(  const wchar_t *_Str,     wchar_t ** _EndPtr, int _Radix);
   long   __cdecl _wcstol_l(  const wchar_t *_Str,     wchar_t **_EndPtr, int _Radix,   _locale_t _Locale);
   long long  __cdecl wcstoll(  const wchar_t *_Str,     wchar_t **_EndPtr, int _Radix);
   long long  __cdecl _wcstoll_l(  const wchar_t *_Str,     wchar_t **_EndPtr, int _Radix,   _locale_t _Locale);
   unsigned long __cdecl wcstoul(  const wchar_t *_Str,     wchar_t ** _EndPtr, int _Radix);
   unsigned long __cdecl _wcstoul_l(  const wchar_t *_Str,     wchar_t **_EndPtr, int _Radix,   _locale_t _Locale);
   unsigned long long __cdecl wcstoull(  const wchar_t *_Str,     wchar_t ** _EndPtr, int _Radix);
   unsigned long long __cdecl _wcstoull_l(  const wchar_t *_Str,     wchar_t ** _EndPtr, int _Radix,   _locale_t _Locale);
   long double __cdecl wcstold(  const wchar_t * _Str,     wchar_t ** _EndPtr);
   long double __cdecl _wcstold_l(  const wchar_t * _Str,     wchar_t ** _EndPtr,   _locale_t _Locale);
   float __cdecl wcstof(  const wchar_t * _Str,     wchar_t ** _EndPtr);
   float __cdecl _wcstof_l(  const wchar_t * _Str,     wchar_t ** _EndPtr,   _locale_t _Locale);



   __declspec(deprecated("This function or variable may be unsafe. Consider using " "_wdupenv_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) wchar_t * __cdecl _wgetenv(  const wchar_t * _VarName);
  errno_t __cdecl _wgetenv_s(  size_t * _ReturnSize,   wchar_t * _DstBuf,   size_t _DstSizeInWords,   const wchar_t * _VarName);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wgetenv_s(  size_t * _ReturnSize, wchar_t (&_Dest)[_Size],   const wchar_t * _VarName) throw() { return _wgetenv_s(_ReturnSize, _Dest, _Size, _VarName); } }




#line 719 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

  errno_t __cdecl _wdupenv_s(    wchar_t **_Buffer,   size_t *_BufferSizeInWords,   const wchar_t *_VarName);



#line 725 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"



 int __cdecl _wsystem(  const wchar_t * _Command);
#line 730 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

#line 732 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

   double __cdecl _wtof(  const wchar_t *_Str);
   double __cdecl _wtof_l(  const wchar_t *_Str,   _locale_t _Locale);
   int __cdecl _wtoi(  const wchar_t *_Str);
   int __cdecl _wtoi_l(  const wchar_t *_Str,   _locale_t _Locale);
   long __cdecl _wtol(  const wchar_t *_Str);
   long __cdecl _wtol_l(  const wchar_t *_Str,   _locale_t _Locale);
   long long __cdecl _wtoll(  const wchar_t *_Str);
   long long __cdecl _wtoll_l(  const wchar_t *_Str,   _locale_t _Locale);

  errno_t __cdecl _i64tow_s(  __int64 _Val,   wchar_t * _DstBuf,   size_t _SizeInWords,   int _Radix);
 __declspec(deprecated("This function or variable may be unsafe. Consider using " "_i64tow_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) wchar_t * __cdecl _i64tow(  __int64 _Val,     wchar_t * _DstBuf,   int _Radix);
  errno_t __cdecl _ui64tow_s(  unsigned __int64 _Val,   wchar_t * _DstBuf,   size_t _SizeInWords,   int _Radix);
 __declspec(deprecated("This function or variable may be unsafe. Consider using " "_ui64tow_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) wchar_t * __cdecl _ui64tow(  unsigned __int64 _Val,     wchar_t * _DstBuf,   int _Radix);
   __int64   __cdecl _wtoi64(  const wchar_t *_Str);
   __int64   __cdecl _wtoi64_l(  const wchar_t *_Str,   _locale_t _Locale);
   __int64   __cdecl _wcstoi64(  const wchar_t * _Str,     wchar_t ** _EndPtr,   int _Radix);
   __int64   __cdecl _wcstoi64_l(  const wchar_t * _Str,     wchar_t ** _EndPtr,   int _Radix,   _locale_t _Locale);
   unsigned __int64  __cdecl _wcstoui64(  const wchar_t * _Str,     wchar_t ** _EndPtr,   int _Radix);
   unsigned __int64  __cdecl _wcstoui64_l(  const wchar_t *_Str ,     wchar_t ** _EndPtr,   int _Radix,   _locale_t _Locale);


#line 755 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"













#line 769 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

   char * __cdecl _fullpath(  char * _FullPath,   const char * _Path,   size_t _SizeInBytes);





#line 777 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

  errno_t __cdecl _ecvt_s(  char * _DstBuf,   size_t _Size,   double _Val,   int _NumOfDights,   int * _PtDec,   int * _PtSign);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _ecvt_s(char (&_Dest)[_Size],   double _Value,   int _NumOfDigits,   int * _PtDec,   int * _PtSign) throw() { return _ecvt_s(_Dest, _Size, _Value, _NumOfDigits, _PtDec, _PtSign); } }
   __declspec(deprecated("This function or variable may be unsafe. Consider using " "_ecvt_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) char * __cdecl _ecvt(  double _Val,   int _NumOfDigits,   int * _PtDec,   int * _PtSign);
  errno_t __cdecl _fcvt_s(  char * _DstBuf,   size_t _Size,   double _Val,   int _NumOfDec,   int * _PtDec,   int * _PtSign);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _fcvt_s(char (&_Dest)[_Size],   double _Value,   int _NumOfDigits,   int * _PtDec,   int * _PtSign) throw() { return _fcvt_s(_Dest, _Size, _Value, _NumOfDigits, _PtDec, _PtSign); } }
   __declspec(deprecated("This function or variable may be unsafe. Consider using " "_fcvt_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) char * __cdecl _fcvt(  double _Val,   int _NumOfDec,   int * _PtDec,   int * _PtSign);
 errno_t __cdecl _gcvt_s(  char * _DstBuf,   size_t _Size,   double _Val,   int _NumOfDigits);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _gcvt_s(char (&_Dest)[_Size],   double _Value,   int _NumOfDigits) throw() { return _gcvt_s(_Dest, _Size, _Value, _NumOfDigits); } }
 __declspec(deprecated("This function or variable may be unsafe. Consider using " "_gcvt_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.")) char * __cdecl _gcvt(  double _Val,   int _NumOfDigits,     char * _DstBuf);

   int __cdecl _atodbl(  _CRT_DOUBLE * _Result,   char * _Str);
   int __cdecl _atoldbl(  _LDOUBLE * _Result,   char * _Str);
   int __cdecl _atoflt(  _CRT_FLOAT * _Result,   const char * _Str);
   int __cdecl _atodbl_l(  _CRT_DOUBLE * _Result,   char * _Str,   _locale_t _Locale);
   int __cdecl _atoldbl_l(  _LDOUBLE * _Result,   char * _Str,   _locale_t _Locale);
   int __cdecl _atoflt_l(  _CRT_FLOAT * _Result,   const char * _Str,   _locale_t _Locale);
          unsigned long __cdecl _lrotl(  unsigned long _Val,   int _Shift);
          unsigned long __cdecl _lrotr(  unsigned long _Val,   int _Shift);
  errno_t   __cdecl _makepath_s(  char * _PathResult,   size_t _SizeInWords,   const char * _Drive,   const char * _Dir,   const char * _Filename,
          const char * _Ext);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _makepath_s(char (&_Path)[_Size],   const char * _Drive,   const char * _Dir,   const char * _Filename,   const char * _Ext) throw() { return _makepath_s(_Path, _Size, _Drive, _Dir, _Filename, _Ext); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_makepath_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  void __cdecl _makepath(  char *_Path,  const char * _Drive,  const char * _Dir,  const char * _Filename,  const char * _Ext);












#line 813 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"












#line 826 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"
        _onexit_t __cdecl _onexit(  _onexit_t _Func);
#line 828 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"



 void __cdecl perror(  const char * _ErrMsg);
#line 833 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

#pragma warning (push)
#pragma warning (disable:6540) 


   int    __cdecl _putenv(  const char * _EnvString);
  errno_t __cdecl _putenv_s(  const char * _Name,   const char * _Value);
#line 841 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

        unsigned int __cdecl _rotl(  unsigned int _Val,   int _Shift);
        unsigned __int64 __cdecl _rotl64(  unsigned __int64 _Val,   int _Shift);
        unsigned int __cdecl _rotr(  unsigned int _Val,   int _Shift);
        unsigned __int64 __cdecl _rotr64(  unsigned __int64 _Val,   int _Shift);
#pragma warning (pop)


 errno_t __cdecl _searchenv_s(  const char * _Filename,   const char * _EnvVar,   char * _ResultPath,   size_t _SizeInBytes);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _searchenv_s(  const char * _Filename,   const char * _EnvVar, char (&_ResultPath)[_Size]) throw() { return _searchenv_s(_Filename, _EnvVar, _ResultPath, _Size); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_searchenv_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  void __cdecl _searchenv( const char * _Filename,  const char * _EnvVar,   char *_ResultPath);
#line 853 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

__declspec(deprecated("This function or variable may be unsafe. Consider using " "_splitpath_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  void   __cdecl _splitpath(  const char * _FullPath,     char * _Drive,     char * _Dir,     char * _Filename,     char * _Ext);
  errno_t  __cdecl _splitpath_s(  const char * _FullPath,
                  char * _Drive,   size_t _DriveSize,
                  char * _Dir,   size_t _DirSize,
                  char * _Filename,   size_t _FilenameSize,
                  char * _Ext,   size_t _ExtSize);
extern "C++" { template <size_t _DriveSize, size_t _DirSize, size_t _NameSize, size_t _ExtSize> inline errno_t __cdecl _splitpath_s(  const char *_Dest,   char (&_Drive)[_DriveSize],   char (&_Dir)[_DirSize],   char (&_Name)[_NameSize],   char (&_Ext)[_ExtSize]) throw() { return _splitpath_s(_Dest, _Drive, _DriveSize, _Dir, _DirSize, _Name, _NameSize, _Ext, _ExtSize); } }

 void   __cdecl _swab(    char * _Buf1,     char * _Buf2, int _SizeInBytes);








#line 872 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

   wchar_t * __cdecl _wfullpath(  wchar_t * _FullPath,   const wchar_t * _Path,   size_t _SizeInWords);



#line 878 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

  errno_t __cdecl _wmakepath_s(  wchar_t * _PathResult,   size_t _SIZE,   const wchar_t * _Drive,   const wchar_t * _Dir,   const wchar_t * _Filename,
          const wchar_t * _Ext);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wmakepath_s(wchar_t (&_ResultPath)[_Size],   const wchar_t * _Drive,   const wchar_t * _Dir,   const wchar_t * _Filename,   const wchar_t * _Ext) throw() { return _wmakepath_s(_ResultPath, _Size, _Drive, _Dir, _Filename, _Ext); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wmakepath_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  void __cdecl _wmakepath(  wchar_t *_ResultPath,  const wchar_t * _Drive,  const wchar_t * _Dir,  const wchar_t * _Filename,  const wchar_t * _Ext);


 void __cdecl _wperror(  const wchar_t * _ErrMsg);
#line 887 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"


   int    __cdecl _wputenv(  const wchar_t * _EnvString);
  errno_t __cdecl _wputenv_s(  const wchar_t * _Name,   const wchar_t * _Value);
 errno_t __cdecl _wsearchenv_s(  const wchar_t * _Filename,   const wchar_t * _EnvVar,   wchar_t * _ResultPath,   size_t _SizeInWords);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wsearchenv_s(  const wchar_t * _Filename,   const wchar_t * _EnvVar, wchar_t (&_ResultPath)[_Size]) throw() { return _wsearchenv_s(_Filename, _EnvVar, _ResultPath, _Size); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wsearchenv_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  void __cdecl _wsearchenv( const wchar_t * _Filename,  const wchar_t * _EnvVar,   wchar_t *_ResultPath);
#line 895 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wsplitpath_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  void   __cdecl _wsplitpath(  const wchar_t * _FullPath,     wchar_t * _Drive,     wchar_t * _Dir,     wchar_t * _Filename,     wchar_t * _Ext);
 errno_t __cdecl _wsplitpath_s(  const wchar_t * _FullPath,
                  wchar_t * _Drive,   size_t _DriveSize,
                  wchar_t * _Dir,   size_t _DirSize,
                  wchar_t * _Filename,   size_t _FilenameSize,
                  wchar_t * _Ext,   size_t _ExtSize);
extern "C++" { template <size_t _DriveSize, size_t _DirSize, size_t _NameSize, size_t _ExtSize> inline errno_t __cdecl _wsplitpath_s(  const wchar_t *_Path,   wchar_t (&_Drive)[_DriveSize],   wchar_t (&_Dir)[_DirSize],   wchar_t (&_Name)[_NameSize],   wchar_t (&_Ext)[_ExtSize]) throw() { return _wsplitpath_s(_Path, _Drive, _DriveSize, _Dir, _DirSize, _Name, _NameSize, _Ext, _ExtSize); } }


#line 906 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"



__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using " "SetErrorMode" " instead. See online help for details."))  void __cdecl _seterrormode(  int _Mode);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using " "Beep" " instead. See online help for details."))  void __cdecl _beep(  unsigned _Frequency,   unsigned _Duration);
__declspec(deprecated("This function or variable has been superceded by newer library or operating system functionality. Consider using " "Sleep" " instead. See online help for details."))  void __cdecl _sleep(  unsigned long _Duration);
#line 913 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

















#line 931 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

#pragma warning(push)
#pragma warning(disable: 4141) 
  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_ecvt" ". See online help for details.")) __declspec(deprecated("This function or variable may be unsafe. Consider using " "_ecvt_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl ecvt(  double _Val,   int _NumOfDigits,   int * _PtDec,   int * _PtSign);
  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_fcvt" ". See online help for details.")) __declspec(deprecated("This function or variable may be unsafe. Consider using " "_fcvt_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl fcvt(  double _Val,   int _NumOfDec,   int * _PtDec,   int * _PtSign);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_gcvt" ". See online help for details.")) __declspec(deprecated("This function or variable may be unsafe. Consider using " "_fcvt_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))           char * __cdecl gcvt(  double _Val,   int _NumOfDigits,     char * _DstBuf);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_itoa" ". See online help for details.")) __declspec(deprecated("This function or variable may be unsafe. Consider using " "_itoa_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))           char * __cdecl itoa(  int _Val,     char * _DstBuf,   int _Radix);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_ltoa" ". See online help for details.")) __declspec(deprecated("This function or variable may be unsafe. Consider using " "_ltoa_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))           char * __cdecl ltoa(  long _Val,     char * _DstBuf,   int _Radix);


  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_putenv" ". See online help for details."))  int    __cdecl putenv(  const char * _EnvString);
#line 943 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_swab" ". See online help for details."))                                                                            void   __cdecl swab(  char * _Buf1,  char * _Buf2,   int _SizeInBytes);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_ultoa" ". See online help for details.")) __declspec(deprecated("This function or variable may be unsafe. Consider using " "_ultoa_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))         char * __cdecl ultoa(  unsigned long _Val,     char * _Dstbuf,   int _Radix);
#pragma warning(pop)
_onexit_t __cdecl onexit(  _onexit_t _Func);


#line 951 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"


}

#line 956 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

#pragma pack(pop)

#line 960 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stdlib.h"

#line 7 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"
#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\cstddef"

#pragma once


#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

#pragma once




#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xkeycheck.h"

#pragma once






 
















































































#line 91 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xkeycheck.h"

  


















































































#line 252 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xkeycheck.h"
 #line 253 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xkeycheck.h"

#line 255 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xkeycheck.h"

#line 257 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xkeycheck.h"
#line 258 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xkeycheck.h"





#line 8 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 9 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

#pragma pack(push,8)




































#line 48 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

#line 50 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"




























#line 79 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

#line 81 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
#line 82 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

		





#line 90 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
#line 91 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

		


		




		

 
  

 

#line 108 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

 
  
 #line 112 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"



 
  
 #line 118 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"










































	
	






		
			
		

#line 173 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
	#line 174 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

	
	




		
			
		

#line 186 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
	#line 187 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

	
	
		
	



#line 196 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

#line 198 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"






	
		#pragma detect_mismatch("_MSC_VER", "1800")
	#line 207 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

	
		#pragma detect_mismatch("_ITERATOR_DEBUG_LEVEL", "2")
	#line 211 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

	
		

#line 216 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
			#pragma detect_mismatch("RuntimeLibrary", "MTd_StaticDebug")
		



#line 222 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
	#line 223 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
#line 224 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
#line 225 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"








	

#line 236 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
		
	#line 238 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
#line 239 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"




#line 244 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"




#line 249 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

#line 251 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
#line 252 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"



#line 256 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"











#line 268 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"


 
#line 272 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

 
 

 









 









 









 

 









 









 




 





 













#line 354 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"











#line 366 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"













#pragma once








#line 24 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"











    
    



#line 41 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"



#line 45 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"
    

    

#line 50 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"
#line 51 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"



    
#line 56 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"

#pragma comment(lib, "libcpmt" "d" "")






#line 65 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"

#line 67 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"

#line 69 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\use_ansi.h"
#line 368 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"



#line 372 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"







#line 380 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"


 















 
  

#line 402 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
   
  #line 404 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
 #line 405 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

 












 
















 
  

#line 440 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
   
  #line 442 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
 #line 443 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

 
  

#line 448 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
   
  #line 450 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
 #line 451 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"


 
  





#line 461 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

   


#line 466 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
    
   #line 468 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

  #line 470 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
 #line 471 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

 

#line 475 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

 
  

#line 480 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
   


     
   #line 485 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
  #line 486 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
 #line 487 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

 


























  
   
  #line 518 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
 #line 519 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

		

 
  
  
  




  
  
  

  







   
   
   
  #line 546 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

  
  
  
  

 












#line 566 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

 
namespace std {
typedef bool _Bool;
}
 #line 572 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

		





		






typedef __int64 _Longlong;
typedef unsigned __int64 _ULonglong;

		







 
#line 599 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

		
 
#line 603 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
 
  
typedef unsigned short char16_t;
typedef unsigned int char32_t;
 #line 608 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
 #line 609 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

		
		






 
namespace std {
enum _Uninitialized
	{	
	_Noinit
	};

		

#pragma warning(push)
#pragma warning(disable:4412)
class  _Lockit
	{	
public:
 

  
















#line 652 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
	__thiscall _Lockit();	
	explicit __thiscall _Lockit(int);	
	__thiscall ~_Lockit() throw ();	
  #line 656 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

    static  void __cdecl _Lockit_ctor(int);
    static  void __cdecl _Lockit_dtor(int);

private:
    static  void __cdecl _Lockit_ctor(_Lockit *);
    static  void __cdecl _Lockit_ctor(_Lockit *, int);
    static  void __cdecl _Lockit_dtor(_Lockit *);

public:
	 _Lockit(const _Lockit&) = delete;
	_Lockit&  operator=(const _Lockit&) = delete;

private:
	int _Locktype;

  











#line 685 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
	};

 



































































  



  


  



  


  
 #line 771 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

class  _Init_locks
	{	
public:
 
      










#line 788 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
    __thiscall _Init_locks();
	__thiscall ~_Init_locks() throw ();
  #line 791 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"

private:
    static  void __cdecl _Init_locks_ctor(_Init_locks *);
    static  void __cdecl _Init_locks_dtor(_Init_locks *);

 







#line 805 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
	};

#pragma warning(pop)
}
 #line 810 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"





		

 void __cdecl _Atexit(void (__cdecl *)(void));

typedef int _Mbstatet;
typedef unsigned long _Uint32t;




 
 

 
 #pragma pack(pop)
#line 831 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"
#line 832 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\yvals.h"





#line 6 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\cstddef"







 #line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stddef.h"














#pragma once




#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 21 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stddef.h"


extern "C" {
#line 25 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stddef.h"











namespace std { typedef decltype(__nullptr) nullptr_t; }
using ::std::nullptr_t;
#line 39 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stddef.h"


















#line 58 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stddef.h"









#line 68 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stddef.h"

 extern unsigned long  __cdecl __threadid(void);

 extern uintptr_t __cdecl __threadhandle(void);


}
#line 76 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stddef.h"

#line 78 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\stddef.h"
#line 14 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\cstddef"
#line 15 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\cstddef"

 
namespace std {
using :: ptrdiff_t; using :: size_t;
}
 #line 21 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\cstddef"

 
namespace std {
typedef double max_align_t;	
}
 #line 27 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\cstddef"
#line 28 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\cstddef"





#line 8 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"
#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\initializer_list"

#pragma once





 #pragma pack(push,8)
 #pragma warning(push,3)
 
 

namespace std {
		
template<class _Elem>
	class initializer_list
	{	
public:
	typedef _Elem value_type;
	typedef const _Elem& reference;
	typedef const _Elem& const_reference;
	typedef size_t size_type;

	typedef const _Elem* iterator;
	typedef const _Elem* const_iterator;

	initializer_list() throw ()
		: _First(0), _Last(0)
		{	
		}

	initializer_list(const _Elem *_First_arg,
		const _Elem *_Last_arg) throw ()
		: _First(_First_arg), _Last(_Last_arg)
		{	
		}

	const _Elem *begin() const throw ()
		{	
		return (_First);
		}

	const _Elem *end() const throw ()
		{	
		return (_Last);
		}

	size_t size() const throw ()
		{	
		return ((size_t)(_Last - _First));
		}

private:
	const _Elem *_First;
	const _Elem *_Last;
	};

		
template<class _Elem> inline
	const _Elem *begin(initializer_list<_Elem> _Ilist) throw ()
	{	
	return (_Ilist.begin());
	}

		
template<class _Elem> inline
	const _Elem *end(initializer_list<_Elem> _Ilist) throw ()
	{	
	return (_Ilist.end());
	}
}

 
 #pragma warning(pop)
 #pragma pack(pop)
#line 77 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\initializer_list"
#line 78 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\initializer_list"





#line 9 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xtr1common"

#pragma once





 #pragma pack(push,8)
 #pragma warning(push,3)
 
 

namespace std {
	
template<class _T1,
	class _Ret>
	struct unary_function;

	
template<class _T1,
	class _T2,
	class _Ret>
	struct binary_function;

	
struct _Nil
	{	
	};
static _Nil _Nil_obj;

	
template<class _Ty,
	_Ty _Val>
	struct integral_constant
	{	
	static const _Ty value = _Val;

	typedef _Ty value_type;
	typedef integral_constant<_Ty, _Val> type;

	operator value_type() const
		{	
		return (value);
		}
	};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

	
template<bool>
	struct _Cat_base
		: false_type
	{	
	};

template<>
	struct _Cat_base<true>
		: true_type
	{	
	};

	
template<bool _Test,
	class _Ty = void>
	struct enable_if
	{	
	};

template<class _Ty>
	struct enable_if<true, _Ty>
	{	
	typedef _Ty type;
	};

	
template<bool _Test,
	class _Ty1,
	class _Ty2>
	struct conditional
	{	
	typedef _Ty2 type;
	};

template<class _Ty1,
	class _Ty2>
	struct conditional<true, _Ty1, _Ty2>
	{	
	typedef _Ty1 type;
	};

	
template<class _Ty1, class _Ty2>
	struct is_same
		: false_type
	{	
	};

template<class _Ty1>
	struct is_same<_Ty1, _Ty1>
		: true_type
	{	
	};

	
template<class _Ty>
	struct remove_const
	{	
	typedef _Ty type;
	};

template<class _Ty>
	struct remove_const<const _Ty>
	{	
	typedef _Ty type;
	};

template<class _Ty>
	struct remove_const<const _Ty[]>
	{	
	typedef _Ty type[];
	};

template<class _Ty, unsigned int _Nx>
	struct remove_const<const _Ty[_Nx]>
	{	
	typedef _Ty type[_Nx];
	};

	
template<class _Ty>
	struct remove_volatile
	{	
	typedef _Ty type;
	};

template<class _Ty>
	struct remove_volatile<volatile _Ty>
	{	
	typedef _Ty type;
	};

template<class _Ty>
	struct remove_volatile<volatile _Ty[]>
	{	
	typedef _Ty type[];
	};

template<class _Ty, unsigned int _Nx>
	struct remove_volatile<volatile _Ty[_Nx]>
	{	
	typedef _Ty type[_Nx];
	};

	
template<class _Ty>
	struct remove_cv
	{	
	typedef typename remove_const<typename remove_volatile<_Ty>::type>::type
		type;
	};

	
template<class _Ty>
	struct _Is_integral
		: false_type
	{	
	};

template<>
	struct _Is_integral<bool>
		: true_type
	{	
	};

template<>
	struct _Is_integral<char>
		: true_type
	{	
	};

template<>
	struct _Is_integral<unsigned char>
		: true_type
	{	
	};

template<>
	struct _Is_integral<signed char>
		: true_type
	{	
	};

 
template<>
	struct _Is_integral<wchar_t>
		: true_type
	{	
	};
 #line 201 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xtr1common"

template<>
	struct _Is_integral<unsigned short>
		: true_type
	{	
	};

template<>
	struct _Is_integral<signed short>
		: true_type
	{	
	};

template<>
	struct _Is_integral<unsigned int>
		: true_type
	{	
	};

template<>
	struct _Is_integral<signed int>
		: true_type
	{	
	};

template<>
	struct _Is_integral<unsigned long>
		: true_type
	{	
	};

template<>
	struct _Is_integral<signed long>
		: true_type
	{	
	};

 











#line 251 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xtr1common"

 
template<>
	struct _Is_integral<__int64>
		: true_type
	{	
	};

template<>
	struct _Is_integral<unsigned __int64>
		: true_type
	{	
	};
 #line 265 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xtr1common"

	
template<class _Ty>
	struct is_integral
		: _Is_integral<typename remove_cv<_Ty>::type>
	{	
	};

	
template<class _Ty>
	struct _Is_floating_point
		: false_type
	{	
	};

template<>
	struct _Is_floating_point<float>
		: true_type
	{	
	};

template<>
	struct _Is_floating_point<double>
		: true_type
	{	
	};

template<>
	struct _Is_floating_point<long double>
		: true_type
	{	
	};

	
template<class _Ty>
	struct is_floating_point
		: _Is_floating_point<typename remove_cv<_Ty>::type>
	{	
	};

template<class _Ty>
	struct _Is_numeric
		: _Cat_base<is_integral<_Ty>::value
			|| is_floating_point<_Ty>::value>
	{	
	};

	
template<class _Ty>
	struct remove_reference
	{	
	typedef _Ty type;
	};

template<class _Ty>
	struct remove_reference<_Ty&>
	{	
	typedef _Ty type;
	};

template<class _Ty>
	struct remove_reference<_Ty&&>
	{	
	typedef _Ty type;
	};

	
template<class _Tgt,
	class _Src>
	struct _Copy_cv
	{	
	typedef typename remove_reference<_Tgt>::type _Tgtx;
	typedef _Tgtx& type;
	};

template<class _Tgt,
	class _Src>
	struct _Copy_cv<_Tgt, const _Src>
	{	
	typedef typename remove_reference<_Tgt>::type _Tgtx;
	typedef const _Tgtx& type;
	};

template<class _Tgt,
	class _Src>
	struct _Copy_cv<_Tgt, volatile _Src>
	{	
	typedef typename remove_reference<_Tgt>::type _Tgtx;
	typedef volatile _Tgtx& type;
	};

template<class _Tgt,
	class _Src>
	struct _Copy_cv<_Tgt, const volatile _Src>
	{	
	typedef typename remove_reference<_Tgt>::type _Tgtx;
	typedef const volatile _Tgtx& type;
	};

template<class _Tgt,
	class _Src>
	struct _Copy_cv<_Tgt, _Src&>
	{	
	typedef typename _Copy_cv<_Tgt, _Src>::type type;
	};

	
struct _Wrap_int
	{	
	_Wrap_int(int)
		{	
		}
	};

template<class _Ty>
	struct _Identity
	{	
	typedef _Ty type;
	};


































		
template<class _Ty>
	struct _Has_result_type
		{ template<class _Uty> static auto _Fn(int, _Identity<typename _Uty::result_type> * = 0, _Identity<typename _Uty::result_type> * = 0, _Identity<typename _Uty::result_type> * = 0) -> true_type; template<class _Uty> static auto _Fn(_Wrap_int) -> false_type; typedef decltype(_Fn<_Ty>(0)) type; };
}
 
 #pragma warning(pop)
 #pragma pack(pop)
#line 427 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xtr1common"
#line 428 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xtr1common"





#line 11 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

 #pragma pack(push,8)
 #pragma warning(push,3)
 
 

 
  
  
  
 #line 22 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

namespace std {
		
 
 
 #line 28 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

 
 
 
 
 

 
 

  
  

  











#line 55 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"
   
   
  #line 58 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

 




















#line 81 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

		


		
 
 

		
template<class _Ty> inline
	_Ty *addressof(_Ty& _Val) throw ()
	{	
	return (reinterpret_cast<_Ty *>(
		(&const_cast<char&>(
		reinterpret_cast<const volatile char&>(_Val)))));
	}

		

template<bool,
	class _Ty1,
	class _Ty2>
	struct _If
	{	
	typedef _Ty2 type;
	};

template<class _Ty1,
	class _Ty2>
	struct _If<true, _Ty1, _Ty2>
	{	
	typedef _Ty1 type;
	};

template<class _Ty>
	struct _Always_false
	{	
	static const bool value = false;
	};

		
		
template<class _Arg,
	class _Result>
	struct unary_function
	{	
	typedef _Arg argument_type;
	typedef _Result result_type;
	};

		
template<class _Arg1,
	class _Arg2,
	class _Result>
	struct binary_function
	{	
	typedef _Arg1 first_argument_type;
	typedef _Arg2 second_argument_type;
	typedef _Result result_type;
	};

		
template<class _Ty = void>
	struct plus
		: public binary_function<_Ty, _Ty, _Ty>
	{	
	_Ty operator()(const _Ty& _Left, const _Ty& _Right) const
		{	
		return (_Left + _Right);
		}
	};

		
template<class _Ty = void>
	struct minus
		: public binary_function<_Ty, _Ty, _Ty>
	{	
	_Ty operator()(const _Ty& _Left, const _Ty& _Right) const
		{	
		return (_Left - _Right);
		}
	};

		
template<class _Ty = void>
	struct multiplies
		: public binary_function<_Ty, _Ty, _Ty>
	{	
	_Ty operator()(const _Ty& _Left, const _Ty& _Right) const
		{	
		return (_Left * _Right);
		}
	};

		
template<class _Ty = void>
	struct equal_to
		: public binary_function<_Ty, _Ty, bool>
	{	
	bool operator()(const _Ty& _Left, const _Ty& _Right) const
		{	
		return (_Left == _Right);
		}
	};

		
template<class _Ty = void>
	struct less
		: public binary_function<_Ty, _Ty, bool>
	{	
	bool operator()(const _Ty& _Left, const _Ty& _Right) const
		{	
		return (_Left < _Right);
		}
	};

		
template<>
	struct plus<void>
	{	
	template<class _Ty1,
		class _Ty2>
		auto operator()(_Ty1&& _Left, _Ty2&& _Right) const
		-> decltype(static_cast<_Ty1&&>(_Left)
			+ static_cast<_Ty2&&>(_Right))
		{	
		return (static_cast<_Ty1&&>(_Left)
			+ static_cast<_Ty2&&>(_Right));
		}
	};

		
template<>
	struct minus<void>
	{	
	template<class _Ty1,
		class _Ty2>
		auto operator()(_Ty1&& _Left, _Ty2&& _Right) const
		-> decltype(static_cast<_Ty1&&>(_Left)
			- static_cast<_Ty2&&>(_Right))
		{	
		return (static_cast<_Ty1&&>(_Left)
			- static_cast<_Ty2&&>(_Right));
		}
	};

		
template<>
	struct multiplies<void>
	{	
	template<class _Ty1,
		class _Ty2>
		auto operator()(_Ty1&& _Left, _Ty2&& _Right) const
		-> decltype(static_cast<_Ty1&&>(_Left)
			* static_cast<_Ty2&&>(_Right))
		{	
		return (static_cast<_Ty1&&>(_Left)
			* static_cast<_Ty2&&>(_Right));
		}
	};

		
template<>
	struct equal_to<void>
	{	
	template<class _Ty1,
		class _Ty2>
		auto operator()(_Ty1&& _Left, _Ty2&& _Right) const
		-> decltype(static_cast<_Ty1&&>(_Left)
			== static_cast<_Ty2&&>(_Right))
		{	
		return (static_cast<_Ty1&&>(_Left)
			== static_cast<_Ty2&&>(_Right));
		}
	};

		
template<>
	struct less<void>
	{	
	template<class _Ty1,
		class _Ty2>
		auto operator()(_Ty1&& _Left, _Ty2&& _Right) const
		-> decltype(static_cast<_Ty1&&>(_Left)
			< static_cast<_Ty2&&>(_Right))
		{	
		return (static_cast<_Ty1&&>(_Left)
			< static_cast<_Ty2&&>(_Right));
		}
	};


}



namespace std {
	
inline size_t _Hash_seq(const unsigned char *_First, size_t _Count)
	{	
 




#line 287 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"
	static_assert(sizeof(size_t) == 4, "This code is for 32-bit size_t.");
	const size_t _FNV_offset_basis = 2166136261U;
	const size_t _FNV_prime = 16777619U;
 #line 291 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

	size_t _Val = _FNV_offset_basis;
	for (size_t _Next = 0; _Next < _Count; ++_Next)
		{	
		_Val ^= (size_t)_First[_Next];
		_Val *= _FNV_prime;
		}

 



#line 304 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"
	static_assert(sizeof(size_t) == 4, "This code is for 32-bit size_t.");
 #line 306 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

	return (_Val);
	}

	
template<class _Kty>
	struct _Bitwise_hash
		: public unary_function<_Kty, size_t>
	{	
	size_t operator()(const _Kty& _Keyval) const
		{	
		return (_Hash_seq((const unsigned char *)&_Keyval, sizeof (_Kty)));
		}
	};

	
template<class _Kty>
	struct hash
		: public _Bitwise_hash<_Kty>
	{	
	static const bool _Value = __is_enum(_Kty);
	static_assert(_Value,
		"The C++ Standard doesn't provide a hash for this type.");
	};
template<>
	struct hash<bool>
		: public _Bitwise_hash<bool>
	{	
	};

template<>
	struct hash<char>
		: public _Bitwise_hash<char>
	{	
	};

template<>
	struct hash<signed char>
		: public _Bitwise_hash<signed char>
	{	
	};

template<>
	struct hash<unsigned char>
		: public _Bitwise_hash<unsigned char>
	{	
	};

 











#line 367 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

 
template<>
	struct hash<wchar_t>
		: public _Bitwise_hash<wchar_t>
	{	
	};
 #line 375 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

template<>
	struct hash<short>
		: public _Bitwise_hash<short>
	{	
	};

template<>
	struct hash<unsigned short>
		: public _Bitwise_hash<unsigned short>
	{	
	};

template<>
	struct hash<int>
		: public _Bitwise_hash<int>
	{	
	};

template<>
	struct hash<unsigned int>
		: public _Bitwise_hash<unsigned int>
	{	
	};

template<>
	struct hash<long>
		: public _Bitwise_hash<long>
	{	
	};

template<>
	struct hash<unsigned long>
		: public _Bitwise_hash<unsigned long>
	{	
	};

template<>
	struct hash<long long>
		: public _Bitwise_hash<long long>
	{	
	};

template<>
	struct hash<unsigned long long>
		: public _Bitwise_hash<unsigned long long>
	{	
	};

template<>
	struct hash<float>
		: public _Bitwise_hash<float>
	{	
	typedef float _Kty;
	typedef _Bitwise_hash<_Kty> _Mybase;

	size_t operator()(const _Kty& _Keyval) const
		{	
		return (_Mybase::operator()(
			_Keyval == 0 ? 0 : _Keyval)); 
		}
	};

template<>
	struct hash<double>
		: public _Bitwise_hash<double>
	{	
	typedef double _Kty;
	typedef _Bitwise_hash<_Kty> _Mybase;

	size_t operator()(const _Kty& _Keyval) const
		{	
		return (_Mybase::operator()(
			_Keyval == 0 ? 0 : _Keyval)); 
		}
	};

template<>
	struct hash<long double>
		: public _Bitwise_hash<long double>
	{	
	typedef long double _Kty;
	typedef _Bitwise_hash<_Kty> _Mybase;

	size_t operator()(const _Kty& _Keyval) const
		{	
		return (_Mybase::operator()(
			_Keyval == 0 ? 0 : _Keyval)); 
		}
	};

template<class _Ty>
	struct hash<_Ty *>
		: public _Bitwise_hash<_Ty *>
	{	
	};
}
#line 473 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

namespace std {
namespace tr1 {	
using ::std:: hash;
}	
}

  














#line 496 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

 

  




















  #line 521 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"

 















#line 539 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"




























 
 #pragma warning(pop)
 #pragma pack(pop)
#line 571 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"
#line 572 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\xstddef"





#line 8 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

 #pragma pack(push,8)
 #pragma warning(push,3)
 
 










namespace std {

  


  



  



}

 

 #line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\eh.h"












#pragma once

#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 16 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\eh.h"








#pragma pack(push,8)







typedef void (__cdecl *terminate_function)();
typedef void (__cdecl *terminate_handler)();
typedef void (__cdecl *unexpected_function)();
typedef void (__cdecl *unexpected_handler)();





#line 42 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\eh.h"








struct _EXCEPTION_POINTERS;

typedef void (__cdecl *_se_translator_function)(unsigned int, struct _EXCEPTION_POINTERS*);
#line 54 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\eh.h"

 __declspec(noreturn) void __cdecl terminate(void);
 __declspec(noreturn) void __cdecl unexpected(void);

 int __cdecl _is_exception_typeof(  const type_info &_Type,   struct _EXCEPTION_POINTERS * _ExceptionPtr);



 terminate_function __cdecl set_terminate(  terminate_function _NewPtFunc);
extern "C"  terminate_function __cdecl _get_terminate(void);
 unexpected_function __cdecl set_unexpected(  unexpected_function _NewPtFunc);
extern "C"  unexpected_function __cdecl _get_unexpected(void);
#line 67 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\eh.h"



 _se_translator_function __cdecl _set_se_translator(  _se_translator_function _NewPtFunc);
#line 72 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\eh.h"
 bool __cdecl __uncaught_exception();









#pragma pack(pop)
#line 84 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\eh.h"
#line 85 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\eh.h"
#line 41 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"
 #line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"














#pragma once




#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 21 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"





#pragma pack(push,8)


extern "C" {
#line 31 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"







#line 39 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"





#line 45 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"













typedef struct _heapinfo {
        int * _pentry;
        size_t _size;
        int _useflag;
        } _HEAPINFO;

#line 65 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"



































#line 101 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"







































#line 141 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"


 int     __cdecl _resetstkoflw (void);
#line 145 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"



 unsigned long __cdecl _set_malloc_crt_max_wait(  unsigned long _NewValue);







#line 157 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"

       void *  __cdecl _expand(  void * _Memory,   size_t _NewSize);
   size_t  __cdecl _msize(  void * _Memory);




#line 165 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"

    void *          __cdecl _alloca(  size_t _Size);


  int     __cdecl _heapwalk(  _HEAPINFO * _EntryInfo);
 intptr_t __cdecl _get_heap_handle(void);
#line 172 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"


   int     __cdecl _heapadd(  void * _Memory,   size_t _Size);
   int     __cdecl _heapchk(void);
   int     __cdecl _heapmin(void);
 int     __cdecl _heapset(  unsigned int _Fill);
 size_t  __cdecl _heapused(size_t * _Used, size_t * _Commit);
#line 180 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"













#line 194 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"

typedef char __static_assert_t[ (sizeof(unsigned int) <= 8) ];


#pragma warning(push)
#pragma warning(disable:6540)
__inline void *_MarkAllocaS(   void *_Ptr, unsigned int _Marker)
{
    if (_Ptr)
    {
        *((unsigned int*)_Ptr) = _Marker;
        _Ptr = (char*)_Ptr + 8;
    }
    return _Ptr;
}

__inline int _MallocaIsSizeInRange(size_t size)
{
    return size + 8 > size;
}
#pragma warning(pop)
#line 216 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"









#line 226 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"









#line 236 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"










#line 247 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"






__pragma(warning(push))
__pragma(warning(disable: 6014))
__declspec(noalias) __inline void __cdecl _freea(    void * _Memory)
{
    unsigned int _Marker;
    if (_Memory)
    {
        _Memory = (char*)_Memory - 8;
        _Marker = *(unsigned int *)_Memory;
        if (_Marker == 0xDDDD)
        {
            free(_Memory);
        }






#line 273 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"
    }
}
__pragma(warning(pop))
#line 277 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"
#line 278 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"
#line 279 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"




#line 284 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"





}
#line 291 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"

#pragma pack(pop)

#line 295 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\malloc.h"
#line 42 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"
 #line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"














#pragma once




#line 1 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\crtdefs.h"







































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































#line 21 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"


extern "C" {
#line 25 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"




#line 30 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

















 void *  __cdecl _memccpy(   void * _Dst,   const void * _Src,   int _Val,   size_t _MaxCount);
   const void *  __cdecl memchr(   const void * _Buf ,   int _Val,   size_t _MaxCount);
   int     __cdecl _memicmp(  const void * _Buf1,   const void * _Buf2,   size_t _Size);
   int     __cdecl _memicmp_l(  const void * _Buf1,   const void * _Buf2,   size_t _Size,   _locale_t _Locale);
  int     __cdecl memcmp(  const void * _Buf1,   const void * _Buf2,   size_t _Size);

 

void *  __cdecl memcpy(  void * _Dst,   const void * _Src,   size_t _Size);

 errno_t  __cdecl memcpy_s(  void * _Dst,   rsize_t _DstSize,   const void * _Src,   rsize_t _MaxCount);





















#line 80 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"










#line 91 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
#line 92 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
         
        
        void *  __cdecl memset(  void * _Dst,   int _Val,   size_t _Size);



__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_memccpy" ". See online help for details."))  void * __cdecl memccpy(  void * _Dst,   const void * _Src,   int _Val,   size_t _Size);
  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_memicmp" ". See online help for details."))  int __cdecl memicmp(  const void * _Buf1,   const void * _Buf2,   size_t _Size);
#line 101 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

#line 103 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
#line 104 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

  errno_t __cdecl _strset_s(  char * _Dst,   size_t _DstSize,   int _Value);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strset_s(  char (&_Dest)[_Size],   int _Value) throw() { return _strset_s(_Dest, _Size, _Value); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_strset_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _strset( char *_Dest,  int _Value);

  errno_t __cdecl strcpy_s(  char * _Dst,   rsize_t _SizeInBytes,   const char * _Src);
#line 111 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strcpy_s(  char (&_Dest)[_Size],   const char * _Source) throw() { return strcpy_s(_Dest, _Size, _Source); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "strcpy_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl strcpy( char *_Dest,  const char * _Source);

  errno_t __cdecl strcat_s(  char * _Dst,   rsize_t _SizeInBytes,   const char * _Src);
#line 116 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strcat_s(char (&_Dest)[_Size],   const char * _Source) throw() { return strcat_s(_Dest, _Size, _Source); } }

__declspec(deprecated("This function or variable may be unsafe. Consider using " "strcat_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl strcat( char *_Dest,  const char * _Source);
#line 120 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
  int     __cdecl strcmp(  const char * _Str1,   const char * _Str2);
  size_t  __cdecl strlen(  const char * _Str);
  


size_t  __cdecl strnlen(  const char * _Str,   size_t _MaxCount);

  static __inline


size_t  __cdecl strnlen_s(  const char * _Str,   size_t _MaxCount)
{
    return (_Str==0) ? 0 : strnlen(_Str, _MaxCount);
}
#line 135 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

  errno_t __cdecl memmove_s(  void * _Dst,   rsize_t _DstSize,   const void * _Src,   rsize_t _MaxCount);
#line 138 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

  void *  __cdecl memmove(  void * _Dst,   const void * _Src,   size_t _Size);




#line 145 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

   char *  __cdecl _strdup(  const char * _Src);



#line 151 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

   const char *  __cdecl strchr(  const char * _Str,   int _Val);
   int     __cdecl _stricmp(   const char * _Str1,    const char * _Str2);
   int     __cdecl _strcmpi(   const char * _Str1,    const char * _Str2);
   int     __cdecl _stricmp_l(   const char * _Str1,    const char * _Str2,   _locale_t _Locale);
   int     __cdecl strcoll(   const char * _Str1,    const  char * _Str2);
   int     __cdecl _strcoll_l(   const char * _Str1,    const char * _Str2,   _locale_t _Locale);
   int     __cdecl _stricoll(   const char * _Str1,    const char * _Str2);
   int     __cdecl _stricoll_l(   const char * _Str1,    const char * _Str2,   _locale_t _Locale);
   int     __cdecl _strncoll  (  const char * _Str1,   const char * _Str2,   size_t _MaxCount);
   int     __cdecl _strncoll_l(  const char * _Str1,   const char * _Str2,   size_t _MaxCount,   _locale_t _Locale);
   int     __cdecl _strnicoll (  const char * _Str1,   const char * _Str2,   size_t _MaxCount);
   int     __cdecl _strnicoll_l(  const char * _Str1,   const char * _Str2,   size_t _MaxCount,   _locale_t _Locale);
   size_t  __cdecl strcspn(   const char * _Str,    const char * _Control);
  __declspec(deprecated("This function or variable may be unsafe. Consider using " "_strerror_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char *  __cdecl _strerror(  const char * _ErrMsg);
  errno_t __cdecl _strerror_s(  char * _Buf,   size_t _SizeInBytes,   const char * _ErrMsg);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strerror_s(char (&_Buffer)[_Size],   const char * _ErrorMessage) throw() { return _strerror_s(_Buffer, _Size, _ErrorMessage); } }
  __declspec(deprecated("This function or variable may be unsafe. Consider using " "strerror_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char *  __cdecl strerror(  int);

  errno_t __cdecl strerror_s(  char * _Buf,   size_t _SizeInBytes,   int _ErrNum);
#line 172 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strerror_s(char (&_Buffer)[_Size],   int _ErrorMessage) throw() { return strerror_s(_Buffer, _Size, _ErrorMessage); } }
  errno_t __cdecl _strlwr_s(  char * _Str,   size_t _Size);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strlwr_s(  char (&_String)[_Size]) throw() { return _strlwr_s(_String, _Size); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_strlwr_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _strlwr( char *_String);
  errno_t __cdecl _strlwr_s_l(  char * _Str,   size_t _Size,   _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strlwr_s_l(  char (&_String)[_Size],   _locale_t _Locale) throw() { return _strlwr_s_l(_String, _Size, _Locale); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_strlwr_s_l" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _strlwr_l(  char *_String,   _locale_t _Locale);

  errno_t __cdecl strncat_s(  char * _Dst,   rsize_t _SizeInBytes,   const char * _Src,   rsize_t _MaxCount);
#line 182 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strncat_s(  char (&_Dest)[_Size],   const char * _Source,   size_t _Count) throw() { return strncat_s(_Dest, _Size, _Source, _Count); } }
#pragma warning(push)
#pragma warning(disable:6059)

__declspec(deprecated("This function or variable may be unsafe. Consider using " "strncat_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl strncat(  char *_Dest,   const char * _Source,   size_t _Count);
#pragma warning(pop)
   int     __cdecl strncmp(  const char * _Str1,   const char * _Str2,   size_t _MaxCount);
   int     __cdecl _strnicmp(  const char * _Str1,   const char * _Str2,   size_t _MaxCount);
   int     __cdecl _strnicmp_l(  const char * _Str1,   const char * _Str2,   size_t _MaxCount,   _locale_t _Locale);

  errno_t __cdecl strncpy_s(  char * _Dst,   rsize_t _SizeInBytes,   const char * _Src,   rsize_t _MaxCount);
#line 194 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl strncpy_s(char (&_Dest)[_Size],   const char * _Source,   size_t _Count) throw() { return strncpy_s(_Dest, _Size, _Source, _Count); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "strncpy_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl strncpy(    char *_Dest,   const char * _Source,   size_t _Count);
  errno_t __cdecl _strnset_s(  char * _Str,   size_t _SizeInBytes,   int _Val,   size_t _MaxCount);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strnset_s(  char (&_Dest)[_Size],   int _Val,   size_t _Count) throw() { return _strnset_s(_Dest, _Size, _Val, _Count); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_strnset_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _strnset(  char *_Dest,   int _Val,   size_t _Count);
   const char *  __cdecl strpbrk(  const char * _Str,   const char * _Control);
   const char *  __cdecl strrchr(  const char * _Str,   int _Ch);
 char *  __cdecl _strrev(  char * _Str);
   size_t  __cdecl strspn(  const char * _Str,   const char * _Control);
     const char *  __cdecl strstr(  const char * _Str,   const char * _SubStr);
  __declspec(deprecated("This function or variable may be unsafe. Consider using " "strtok_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char *  __cdecl strtok(  char * _Str,   const char * _Delim);

   char *  __cdecl strtok_s(  char * _Str,   const char * _Delim,     char ** _Context);
#line 208 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
  errno_t __cdecl _strupr_s(  char * _Str,   size_t _Size);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strupr_s(  char (&_String)[_Size]) throw() { return _strupr_s(_String, _Size); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_strupr_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _strupr( char *_String);
  errno_t __cdecl _strupr_s_l(  char * _Str,   size_t _Size, _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _strupr_s_l(  char (&_String)[_Size], _locale_t _Locale) throw() { return _strupr_s_l(_String, _Size, _Locale); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_strupr_s_l" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  char * __cdecl _strupr_l(  char *_String,   _locale_t _Locale);
  size_t  __cdecl strxfrm (    char * _Dst,   const char * _Src,   size_t _MaxCount);
  size_t  __cdecl _strxfrm_l(    char * _Dst,   const char * _Src,   size_t _MaxCount,   _locale_t _Locale);


extern "C++" {


  inline char * __cdecl strchr(  char * _Str,   int _Ch)
        { return (char*)strchr((const char*)_Str, _Ch); }
  inline char * __cdecl strpbrk(  char * _Str,   const char * _Control)
        { return (char*)strpbrk((const char*)_Str, _Control); }
  inline char * __cdecl strrchr(  char * _Str,   int _Ch)
        { return (char*)strrchr((const char*)_Str, _Ch); }
    inline char * __cdecl strstr(  char * _Str,   const char * _SubStr)
        { return (char*)strstr((const char*)_Str, _SubStr); }
#line 230 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"


  inline void * __cdecl memchr(  void * _Pv,   int _C,   size_t _N)
        { return (void*)memchr((const void*)_Pv, _C, _N); }
#line 235 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
}
#line 237 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"






#line 244 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_strdup" ". See online help for details."))  char * __cdecl strdup(  const char * _Src);



#line 250 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"


  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_strcmpi" ". See online help for details."))  int __cdecl strcmpi(  const char * _Str1,   const char * _Str2);
  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_stricmp" ". See online help for details."))  int __cdecl stricmp(  const char * _Str1,   const char * _Str2);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_strlwr" ". See online help for details."))  char * __cdecl strlwr(  char * _Str);
  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_strnicmp" ". See online help for details."))  int __cdecl strnicmp(  const char * _Str1,   const char * _Str,   size_t _MaxCount);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_strnset" ". See online help for details."))  char * __cdecl strnset(  char * _Str,   int _Val,   size_t _MaxCount);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_strrev" ". See online help for details."))  char * __cdecl strrev(  char * _Str);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_strset" ". See online help for details."))         char * __cdecl strset(  char * _Str,   int _Val);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_strupr" ". See online help for details."))  char * __cdecl strupr(  char * _Str);

#line 262 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"









#line 272 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

   wchar_t * __cdecl _wcsdup(  const wchar_t * _Str);



#line 278 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"


  errno_t __cdecl wcscat_s(  wchar_t * _Dst,   rsize_t _SizeInWords,   const wchar_t * _Src);
#line 282 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcscat_s(wchar_t (&_Dest)[_Size],   const wchar_t * _Source) throw() { return wcscat_s(_Dest, _Size, _Source); } }

__declspec(deprecated("This function or variable may be unsafe. Consider using " "wcscat_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl wcscat( wchar_t *_Dest,  const wchar_t * _Source);
#line 286 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
 

 const wchar_t * __cdecl wcschr(  const wchar_t * _Str, wchar_t _Ch);
   int __cdecl wcscmp(  const wchar_t * _Str1,   const wchar_t * _Str2);

  errno_t __cdecl wcscpy_s(  wchar_t * _Dst,   rsize_t _SizeInWords,   const wchar_t * _Src);
#line 293 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcscpy_s(wchar_t (&_Dest)[_Size],   const wchar_t * _Source) throw() { return wcscpy_s(_Dest, _Size, _Source); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "wcscpy_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl wcscpy( wchar_t *_Dest,  const wchar_t * _Source);
   size_t __cdecl wcscspn(  const wchar_t * _Str,   const wchar_t * _Control);
   size_t __cdecl wcslen(  const wchar_t * _Str);
  


size_t __cdecl wcsnlen(  const wchar_t * _Src,   size_t _MaxCount);

  static __inline


size_t __cdecl wcsnlen_s(  const wchar_t * _Src,   size_t _MaxCount)
{
    return (_Src == 0) ? 0 : wcsnlen(_Src, _MaxCount);
}
#line 310 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

  errno_t __cdecl wcsncat_s(  wchar_t * _Dst,   rsize_t _SizeInWords,   const wchar_t * _Src,   rsize_t _MaxCount);
#line 313 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcsncat_s(  wchar_t (&_Dest)[_Size],   const wchar_t * _Source,   size_t _Count) throw() { return wcsncat_s(_Dest, _Size, _Source, _Count); } }
#pragma warning(push)
#pragma warning(disable:6059)
__declspec(deprecated("This function or variable may be unsafe. Consider using " "wcsncat_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl wcsncat(  wchar_t *_Dest,   const wchar_t * _Source,   size_t _Count);
#pragma warning(pop)
   int __cdecl wcsncmp(  const wchar_t * _Str1,   const wchar_t * _Str2,   size_t _MaxCount);

  errno_t __cdecl wcsncpy_s(  wchar_t * _Dst,   rsize_t _SizeInWords,   const wchar_t * _Src,   rsize_t _MaxCount);
#line 322 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
extern "C++" { template <size_t _Size> inline errno_t __cdecl wcsncpy_s(wchar_t (&_Dest)[_Size],   const wchar_t * _Source,   size_t _Count) throw() { return wcsncpy_s(_Dest, _Size, _Source, _Count); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "wcsncpy_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl wcsncpy(    wchar_t *_Dest,   const wchar_t * _Source,   size_t _Count);
   const wchar_t * __cdecl wcspbrk(  const wchar_t * _Str,   const wchar_t * _Control);
   const wchar_t * __cdecl wcsrchr(  const wchar_t * _Str,   wchar_t _Ch);
   size_t __cdecl wcsspn(  const wchar_t * _Str,   const wchar_t * _Control);
   

 const wchar_t * __cdecl wcsstr(  const wchar_t * _Str,   const wchar_t * _SubStr);
  __declspec(deprecated("This function or variable may be unsafe. Consider using " "wcstok_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl wcstok(  wchar_t * _Str,   const wchar_t * _Delim);

   wchar_t * __cdecl wcstok_s(  wchar_t * _Str,   const wchar_t * _Delim,     wchar_t ** _Context);
#line 334 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
  __declspec(deprecated("This function or variable may be unsafe. Consider using " "_wcserror_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _wcserror(  int _ErrNum);
  errno_t __cdecl _wcserror_s(  wchar_t * _Buf,   size_t _SizeInWords,   int _ErrNum);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcserror_s(wchar_t (&_Buffer)[_Size],   int _Error) throw() { return _wcserror_s(_Buffer, _Size, _Error); } }
  __declspec(deprecated("This function or variable may be unsafe. Consider using " "__wcserror_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl __wcserror(  const wchar_t * _Str);
  errno_t __cdecl __wcserror_s(  wchar_t * _Buffer,   size_t _SizeInWords,   const wchar_t * _ErrMsg);
extern "C++" { template <size_t _Size> inline errno_t __cdecl __wcserror_s(wchar_t (&_Buffer)[_Size],   const wchar_t * _ErrorMessage) throw() { return __wcserror_s(_Buffer, _Size, _ErrorMessage); } }

   int __cdecl _wcsicmp(  const wchar_t * _Str1,   const wchar_t * _Str2);
   int __cdecl _wcsicmp_l(  const wchar_t * _Str1,   const wchar_t * _Str2,   _locale_t _Locale);
   int __cdecl _wcsnicmp(  const wchar_t * _Str1,   const wchar_t * _Str2,   size_t _MaxCount);
   int __cdecl _wcsnicmp_l(  const wchar_t * _Str1,   const wchar_t * _Str2,   size_t _MaxCount,   _locale_t _Locale);
  errno_t __cdecl _wcsnset_s(  wchar_t * _Dst,   size_t _SizeInWords,   wchar_t _Val,   size_t _MaxCount);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcsnset_s(  wchar_t (&_Dst)[_Size], wchar_t _Val,   size_t _MaxCount) throw() { return _wcsnset_s(_Dst, _Size, _Val, _MaxCount); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wcsnset_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _wcsnset(  wchar_t *_Str, wchar_t _Val,   size_t _MaxCount);
 wchar_t * __cdecl _wcsrev(  wchar_t * _Str);
  errno_t __cdecl _wcsset_s(  wchar_t * _Dst,   size_t _SizeInWords,   wchar_t _Value);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcsset_s(  wchar_t (&_Str)[_Size], wchar_t _Val) throw() { return _wcsset_s(_Str, _Size, _Val); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wcsset_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _wcsset(  wchar_t *_Str, wchar_t _Val);

  errno_t __cdecl _wcslwr_s(  wchar_t * _Str,   size_t _SizeInWords);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcslwr_s(  wchar_t (&_String)[_Size]) throw() { return _wcslwr_s(_String, _Size); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wcslwr_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _wcslwr( wchar_t *_String);
  errno_t __cdecl _wcslwr_s_l(  wchar_t * _Str,   size_t _SizeInWords,   _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcslwr_s_l(  wchar_t (&_String)[_Size],   _locale_t _Locale) throw() { return _wcslwr_s_l(_String, _Size, _Locale); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wcslwr_s_l" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _wcslwr_l(  wchar_t *_String,   _locale_t _Locale);
  errno_t __cdecl _wcsupr_s(  wchar_t * _Str,   size_t _Size);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcsupr_s(  wchar_t (&_String)[_Size]) throw() { return _wcsupr_s(_String, _Size); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wcsupr_s" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _wcsupr( wchar_t *_String);
  errno_t __cdecl _wcsupr_s_l(  wchar_t * _Str,   size_t _Size,   _locale_t _Locale);
extern "C++" { template <size_t _Size> inline errno_t __cdecl _wcsupr_s_l(  wchar_t (&_String)[_Size],   _locale_t _Locale) throw() { return _wcsupr_s_l(_String, _Size, _Locale); } }
__declspec(deprecated("This function or variable may be unsafe. Consider using " "_wcsupr_s_l" " instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details."))  wchar_t * __cdecl _wcsupr_l(  wchar_t *_String,   _locale_t _Locale);
  size_t __cdecl wcsxfrm(    wchar_t * _Dst,   const wchar_t * _Src,   size_t _MaxCount);
  size_t __cdecl _wcsxfrm_l(    wchar_t * _Dst,   const wchar_t *_Src,   size_t _MaxCount,   _locale_t _Locale);
   int __cdecl wcscoll(  const wchar_t * _Str1,   const wchar_t * _Str2);
   int __cdecl _wcscoll_l(  const wchar_t * _Str1,   const wchar_t * _Str2,   _locale_t _Locale);
   int __cdecl _wcsicoll(  const wchar_t * _Str1,   const wchar_t * _Str2);
   int __cdecl _wcsicoll_l(  const wchar_t * _Str1,   const wchar_t *_Str2,   _locale_t _Locale);
   int __cdecl _wcsncoll(  const wchar_t * _Str1,   const wchar_t * _Str2,   size_t _MaxCount);
   int __cdecl _wcsncoll_l(  const wchar_t * _Str1,   const wchar_t * _Str2,   size_t _MaxCount,   _locale_t _Locale);
   int __cdecl _wcsnicoll(  const wchar_t * _Str1,   const wchar_t * _Str2,   size_t _MaxCount);
   int __cdecl _wcsnicoll_l(  const wchar_t * _Str1,   const wchar_t * _Str2,   size_t _MaxCount,   _locale_t _Locale);




extern "C++" {
 

        inline wchar_t * __cdecl wcschr(  wchar_t *_Str, wchar_t _Ch)
        {return ((wchar_t *)wcschr((const wchar_t *)_Str, _Ch)); }
  inline wchar_t * __cdecl wcspbrk(  wchar_t *_Str,   const wchar_t *_Control)
        {return ((wchar_t *)wcspbrk((const wchar_t *)_Str, _Control)); }
  inline wchar_t * __cdecl wcsrchr(  wchar_t *_Str,   wchar_t _Ch)
        {return ((wchar_t *)wcsrchr((const wchar_t *)_Str, _Ch)); }
   

        inline wchar_t * __cdecl wcsstr(  wchar_t *_Str,   const wchar_t *_SubStr)
        {return ((wchar_t *)wcsstr((const wchar_t *)_Str, _SubStr)); }
}
#line 394 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"
#line 395 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"






#line 402 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcsdup" ". See online help for details."))  wchar_t * __cdecl wcsdup(  const wchar_t * _Str);



#line 408 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"





  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcsicmp" ". See online help for details."))  int __cdecl wcsicmp(  const wchar_t * _Str1,   const wchar_t * _Str2);
  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcsnicmp" ". See online help for details."))  int __cdecl wcsnicmp(  const wchar_t * _Str1,   const wchar_t * _Str2,   size_t _MaxCount);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcsnset" ". See online help for details."))  wchar_t * __cdecl wcsnset(  wchar_t * _Str,   wchar_t _Val,   size_t _MaxCount);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcsrev" ". See online help for details."))  wchar_t * __cdecl wcsrev(  wchar_t * _Str);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcsset" ". See online help for details."))  wchar_t * __cdecl wcsset(  wchar_t * _Str, wchar_t _Val);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcslwr" ". See online help for details."))  wchar_t * __cdecl wcslwr(  wchar_t * _Str);
__declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcsupr" ". See online help for details."))  wchar_t * __cdecl wcsupr(  wchar_t * _Str);
  __declspec(deprecated("The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: " "_wcsicoll" ". See online help for details."))  int __cdecl wcsicoll(  const wchar_t * _Str1,   const wchar_t * _Str2);

#line 423 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"


#line 426 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"








}
#line 436 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

#line 438 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\string.h"

#line 43 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

 

#line 47 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

 





























#line 79 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

 namespace std {





 
#line 88 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

class  exception
	{   
public:
	  exception();
	 explicit  exception(const char * const &);
	  exception(const char * const &, int);
	  exception(const exception&);
	 exception&  operator=(const exception&);
	 virtual  ~exception() throw ();
	 virtual const char *  what() const;

private:
	 void  _Copy_str(const char *);
	 void  _Tidy();

	const char * _Mywhat;
	bool _Mydofree;
	};

















































































using ::set_terminate; using ::terminate_handler; using ::terminate; using ::set_unexpected; using ::unexpected_handler; using ::unexpected;

typedef void (__cdecl *_Prhand)(const exception&);

 bool __cdecl uncaught_exception();


inline terminate_handler __cdecl get_terminate()
	{	
	return (_get_terminate());
	}

inline unexpected_handler __cdecl get_unexpected()
	{	
	return (_get_unexpected());
	}
#line 205 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

}

 




















































































































#line 326 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"


namespace std {


#line 332 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

		
class bad_exception : public exception
	{	
public:
	 bad_exception(const char *_Message = "bad exception")
		throw ()
		: exception(_Message)
		{	
		}

	virtual  ~bad_exception() throw ()
		{	
		}

 





#line 354 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

	};

		
class bad_alloc : public exception
	{	
public:
	 bad_alloc() throw ()
		: exception("bad allocation", 1)
		{	
		}

	virtual  ~bad_alloc() throw ()
		{	
		}

private:
	friend class bad_array_new_length;

	 bad_alloc(const char *_Message) throw ()
		: exception(_Message, 1)
		{	
		}

 





#line 385 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

	};

		
class bad_array_new_length
	: public bad_alloc
	{	
public:

	bad_array_new_length() throw ()
		: bad_alloc("bad array new length")
		{	
		}
	};


}









#line 412 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"

 void __cdecl __ExceptionPtrCreate(  void* );
 void __cdecl __ExceptionPtrDestroy(  void* );
 void __cdecl __ExceptionPtrCopy(  void*,   const void* );
 void __cdecl __ExceptionPtrAssign(  void*,   const void* );
 bool __cdecl __ExceptionPtrCompare(  const void*,   const void*);
 bool __cdecl __ExceptionPtrToBool(  const void*);
 void __cdecl __ExceptionPtrSwap(  void*,   void*);

 void __cdecl __ExceptionPtrCurrentException(  void*);
 void __cdecl __ExceptionPtrRethrow(  const void*);
 void __cdecl __ExceptionPtrCopyException(  void*,   const void*,   const void*);

namespace std {

class exception_ptr
	{
public:
	exception_ptr()
		{
		__ExceptionPtrCreate(this);
		}
	exception_ptr(nullptr_t)
		{
		__ExceptionPtrCreate(this);
		}
	~exception_ptr() throw ()
		{
		__ExceptionPtrDestroy(this);
		}
	exception_ptr(const exception_ptr& _Rhs)
		{
		__ExceptionPtrCopy(this, &_Rhs);
		}
	exception_ptr& operator=(const exception_ptr& _Rhs)
		{
		__ExceptionPtrAssign(this, &_Rhs);
		return *this;
		}
	exception_ptr& operator=(nullptr_t)
		{
		exception_ptr _Ptr;
		__ExceptionPtrAssign(this, &_Ptr);
		return *this;
		}

	typedef exception_ptr _Myt;

	explicit operator bool() const throw ()
		{
		return __ExceptionPtrToBool(this);
		}

	void _RethrowException() const
		{
		__ExceptionPtrRethrow(this);
		}

	static exception_ptr _Current_exception()
		{
		exception_ptr _Retval;
		__ExceptionPtrCurrentException(&_Retval);
		return _Retval;
		}
	static exception_ptr _Copy_exception(  void* _Except,   const void* _Ptr)
		{
		exception_ptr _Retval = 0;
		if (!_Ptr)
			{
			
			return _Retval;
			}
		__ExceptionPtrCopyException(&_Retval, _Except, _Ptr);
		return _Retval;
		}
private:
	void* _Data1;
	void* _Data2;
	};

inline void swap(exception_ptr& _Lhs, exception_ptr& _Rhs)
	{
	__ExceptionPtrSwap(&_Lhs, &_Rhs);
	}

inline bool operator==(const exception_ptr& _Lhs, const exception_ptr& _Rhs)
	{
	return __ExceptionPtrCompare(&_Lhs, &_Rhs);
	}

inline bool operator==(nullptr_t, const exception_ptr& _Rhs)
	{
	return !_Rhs;
	}

inline bool operator==(const exception_ptr& _Lhs, nullptr_t)
	{
	return !_Lhs;
	}

inline bool operator!=(const exception_ptr& _Lhs, const exception_ptr& _Rhs)
	{
	return !(_Lhs == _Rhs);
	}

inline bool operator!=(nullptr_t _Lhs, const exception_ptr& _Rhs)
	{
	return !(_Lhs == _Rhs);
	}

inline bool operator!=(const exception_ptr& _Lhs, nullptr_t _Rhs)
	{
	return !(_Lhs == _Rhs);
	}

inline exception_ptr current_exception()
	{
	return exception_ptr::_Current_exception();
	}

inline void rethrow_exception(  exception_ptr _P)
	{
	_P._RethrowException();
	}

template <class _E> void *__GetExceptionInfo(_E);

template<class _E> exception_ptr make_exception_ptr(_E _Except)
	{
	return exception_ptr::_Copy_exception(::std:: addressof(_Except), __GetExceptionInfo(_Except));
	}
}







 
 #pragma warning(pop)
 #pragma pack(pop)

#line 556 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"
#line 557 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\exception"





#line 7 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"

 #pragma pack(push,8)
 #pragma warning(push,3)
 

  








#line 22 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"

namespace std {

		
 




typedef void (__cdecl * new_handler) ();
#line 33 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"
 #line 34 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"

 
struct nothrow_t
	{	
	};

extern const nothrow_t nothrow;	
 #line 42 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"

		
 new_handler __cdecl set_new_handler(  new_handler)
	throw ();	

 new_handler __cdecl get_new_handler()
	throw ();	
}

		
void __cdecl operator delete(void *) throw ();
#pragma warning (suppress: 4985)
    void *__cdecl operator new(size_t _Size) throw (...);

 
  
inline void *__cdecl operator new(size_t, void *_Where) throw ()
	{	
	return (_Where);
	}

inline void __cdecl operator delete(void *, void *) throw ()
	{	
	}
 #line 67 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"

 
  
inline void *__cdecl operator new[](size_t, void *_Where) throw ()
	{	
	return (_Where);
	}

inline void __cdecl operator delete[](void *, void *) throw ()
	{	
	}
 #line 79 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"

void __cdecl operator delete[](void *) throw ();	

    void *__cdecl operator new[](size_t _Size)
	throw (...);	

 
  
    void *__cdecl operator new(size_t _Size, const ::std:: nothrow_t&)
	throw ();

    void *__cdecl operator new[](size_t _Size, const ::std:: nothrow_t&)
	throw ();	

void __cdecl operator delete(void *, const ::std:: nothrow_t&)
	throw ();	

void __cdecl operator delete[](void *, const ::std:: nothrow_t&)
	throw ();	
 #line 99 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"


 
using ::std:: new_handler;
 #line 104 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"

 
 #pragma warning(pop)
 #pragma pack(pop)

#line 110 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"
#line 111 "C:\\p4\\External\\SDK\\VS12.0\\VC\\include\\new"





#line 15 "C:\\p4\\Code\\Core/Containers/Array.h"




template < class T >
class Array
{
public:
	explicit Array();
	explicit Array( const Array< T > & other );
	explicit Array( size_t initialCapacity, bool resizeable = false );
	~Array();

	void Destruct();

	
	typedef	T *			Iter;
	typedef const T *	ConstIter;
	Iter		Begin()	const	{ return m_Begin; }
	Iter		End() const		{ return m_End; }
	inline T &			operator [] ( size_t index )		{ do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( index < GetSize() ) ) { if ( AssertHandler::Failure( "index < GetSize()", "C:\\p4\\Code\\Core/Containers/Array.h", 35 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); return m_Begin[ index ]; }
	inline const T &	operator [] ( size_t index ) const	{ do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( index < GetSize() ) ) { if ( AssertHandler::Failure( "index < GetSize()", "C:\\p4\\Code\\Core/Containers/Array.h", 36 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); return m_Begin[ index ]; }
	inline T &			Top()		{ do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( m_Begin < m_End ) ) { if ( AssertHandler::Failure( "m_Begin < m_End", "C:\\p4\\Code\\Core/Containers/Array.h", 37 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); return m_End[ -1 ]; }
	inline const T &	Top() const	{ do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( m_Begin < m_End ) ) { if ( AssertHandler::Failure( "m_Begin < m_End", "C:\\p4\\Code\\Core/Containers/Array.h", 38 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); return m_End[ -1 ]; }

	
	void SetCapacity( size_t capacity );
	void SetSize( size_t size );
	void Clear();
	void Swap( Array< T > & other );

	
	void Sort() { ShellSort( m_Begin, m_End, AscendingCompare() ); }
	void SortDeref() { ShellSort( m_Begin, m_End, AscendingCompareDeref() ); }
	template < class COMPARER >
	void Sort( const COMPARER & comp ) { ShellSort( m_Begin, m_End, comp ); }

	
	template < class U >
	T * Find( const U & obj ) const;
	template < class U >
	T * FindDeref( const U & obj ) const;

	
	void Append( const T & item );
	void Append( const Array< T > & other );
	void Pop();
	void PopFront(); 
	void Erase( T * const iter );

	Array & operator = ( const Array< T > & other );

	
	inline bool		IsAtCapacity() const	{ return ( m_End == m_MaxEnd ) && ( m_Resizeable == false ); }
	inline size_t	GetCapacity() const		{ return ( m_MaxEnd - m_Begin ); }
	inline size_t	GetSize() const			{ return ( m_End - m_Begin ); }
	inline bool		IsEmpty() const			{ return ( m_Begin == m_End ); }

private:
	void Grow();
	inline T * Allocate( size_t numElements ) const;
	inline void Deallocate( T * ptr ) const;

	T * m_Begin;
	T * m_End;
	T * m_MaxEnd;
	bool m_Resizeable;
};



template < class T >
Array< T >::Array()
	: m_Begin( nullptr )
	, m_End( nullptr )
	, m_MaxEnd( nullptr )
	, m_Resizeable( true )
{
}



template < class T >
Array< T >::Array( const Array< T > & other )
{
	new (this) Array( other.GetSize(), true );
	*this = other;
}



template < class T >
Array< T >::Array( size_t initialCapacity, bool resizeable )
{
	if ( initialCapacity )
	{
		
			m_Resizeable = true; 
		#line 114 "C:\\p4\\Code\\Core/Containers/Array.h"
		m_Begin = Allocate( initialCapacity );
		m_End = m_Begin;
		m_MaxEnd = m_Begin + initialCapacity;
	}
	else
	{
		m_Begin = nullptr;
		m_End = nullptr;
		m_MaxEnd = nullptr;
	}
	m_Resizeable = resizeable;
}



template < class T >
Array< T >::~Array()
{
	Destruct();
}



template < class T >
void Array< T >::Destruct()
{
	T * iter = m_Begin;
	while ( iter < m_End )
	{
		iter->~T();
		iter++;
	}
	Deallocate( m_Begin );
	m_Begin = nullptr;
	m_End = nullptr;
	m_MaxEnd = nullptr;
}



template < class T >
void Array< T >::SetCapacity( size_t capacity )
{
	if ( capacity == GetCapacity() )
	{
		return;
	}

	T * newMem = Allocate( capacity );

	
	
	size_t itemsToKeep = Math::Min( capacity, GetSize() );
	T * src = m_Begin;
	T * dst = newMem;
	T * keepEnd = m_Begin + itemsToKeep;
	while ( src < m_End )
	{
		if ( src < keepEnd )
		{
			new ( dst ) T( *src );
		}
		src->~T();
		src++;
		dst++;
	}

	
	Deallocate( m_Begin );

	
	m_Begin = newMem;
	m_End = newMem + itemsToKeep;
	m_MaxEnd = newMem + capacity;
}



template < class T >
void Array< T >::SetSize( size_t size )
{
	size_t oldSize = GetSize();

	
	if ( oldSize == size )
	{
		return;
	}

	
	if ( size < oldSize )
	{
		
		T * item = m_Begin + size;
		T * end = m_End;
		while ( item < end )
		{
			item->~T();
			item++;
		}
		m_End = m_Begin + size;
		return;
	}

	

	
	if ( size > GetCapacity() )
	{
		SetCapacity( size );
	}

	
	T * item = m_End;
	T * newEnd = m_Begin + size;
	while( item < newEnd )
	{
		new ( item ) T;
		item++;
	}
	m_End = newEnd;
}



template < class T >
void Array< T >::Clear()
{
	
	T * src = m_Begin;
	while ( src < m_End )
	{
		src->~T();
		src++;
	}

	
	m_End = m_Begin;
}



template < class T >
void Array< T >::Swap( Array< T > & other )
{
	T * tmpBegin = m_Begin;
	T * tmpEnd = m_End;
	T * tmpMaxEnd = m_MaxEnd;
	bool tmpResizeable = m_Resizeable;
	m_Begin = other.m_Begin;
	m_End = other.m_End;
	m_MaxEnd = other.m_MaxEnd;
	m_Resizeable = other.m_Resizeable;
	other.m_Begin = tmpBegin;
	other.m_End = tmpEnd;
	other.m_MaxEnd = tmpMaxEnd;
	other.m_Resizeable = tmpResizeable;
}



template < class T >
template < class U >
T * Array< T >::Find( const U & obj ) const
{
	T * pos = m_Begin;
	T * end = m_End;
	while ( pos < end )
	{
		if ( *pos == obj )
		{
			return pos;
		}
		pos++;
	}
	return nullptr;
}



template < class T >
template < class U >
T * Array< T >::FindDeref( const U & obj ) const
{
	T * pos = m_Begin;
	T * end = m_End;
	while ( pos < end )
	{
		if ( *(*pos) == obj )
		{
			return pos;
		}
		pos++;
	}
	return nullptr;
}



template < class T >
void Array< T >::Append( const T & item )
{
	if ( m_End == m_MaxEnd )
	{
		Grow();
	}
	new ( m_End ) T( item );
	m_End++;
}



template < class T >
void Array< T >::Append( const Array< T > & other  )
{
	auto end = other.End();
	for ( auto it = other.Begin(); it != end; ++it )
	{
		Append( *it );
	}	
}



template < class T >
void Array< T >::Pop()
{
	do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( m_Begin < m_End ) ) { if ( AssertHandler::Failure( "m_Begin < m_End", "C:\\p4\\Code\\Core/Containers/Array.h", 341 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); 

	T * it = --m_End;
	it->~T();
	(void)it; 
}



template < class T >
void Array< T >::PopFront()
{
	do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( m_Begin < m_End ) ) { if ( AssertHandler::Failure( "m_Begin < m_End", "C:\\p4\\Code\\Core/Containers/Array.h", 353 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); 

	
	T * dst = m_Begin;
	T * src = m_Begin + 1;
	while ( src < m_End )
	{
		*dst = *src;
		dst++;
		src++;
	}

	
	dst->~T();

	m_End--;
}



template < class T >
void Array< T >::Erase( T * const iter )
{
	do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( iter < m_End ) ) { if ( AssertHandler::Failure( "iter < m_End", "C:\\p4\\Code\\Core/Containers/Array.h", 376 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop));

	T * dst = iter;
	T * last = ( m_End - 1 );
	while ( dst < last )
	{
		*dst = *(dst + 1);
		dst++;
	}
	dst->~T();
	m_End = last;
}



template < class T >
Array< T > & Array< T >::operator = ( const Array< T > & other )
{
	Clear();

	
	const size_t otherSize = other.GetSize();
	if ( GetCapacity() < otherSize )
	{
		Deallocate( m_Begin );
		m_Begin = Allocate( otherSize );
		m_MaxEnd = m_Begin + otherSize;
	}

	m_End = m_Begin + otherSize;
	T * dst = m_Begin;
	T * src = other.m_Begin;
	const T * end = m_End;
	while ( dst < end )
	{
		new ( dst ) T( *src );
		dst++;
		src++;
	}
	
	return *this;
}



template < class T >
void Array< T >::Grow()
{
	do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( m_Resizeable ) ) { if ( AssertHandler::Failure( "m_Resizeable", "C:\\p4\\Code\\Core/Containers/Array.h", 424 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop));

	
	size_t currentCapacity = GetCapacity();
	size_t size = GetSize();
	size_t newCapacity = ( currentCapacity + ( currentCapacity >> 1 ) + 1 );
	T * newMem = Allocate( newCapacity );

	T * src = m_Begin;
	T * dst = newMem;
	while ( src < m_End )
	{
		new ( dst ) T( *src );
		src->~T();
		dst++;
		src++;
	}
	Deallocate( m_Begin );
	m_Begin = newMem;
	m_End = ( newMem ) + size;
	m_MaxEnd = ( newMem ) + newCapacity;
}



template < class T >
T * Array< T >::Allocate( size_t numElements ) const
{
	do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( m_Resizeable == true ) ) { if ( AssertHandler::Failure( "m_Resizeable == true", "C:\\p4\\Code\\Core/Containers/Array.h", 452 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop));




	return static_cast< T * >( _aligned_malloc( sizeof( T ) * numElements, __alignof( T ) ) );
#line 459 "C:\\p4\\Code\\Core/Containers/Array.h"
}



template < class T >
void Array< T >::Deallocate( T * ptr ) const
{




	_aligned_free( ptr );
#line 472 "C:\\p4\\Code\\Core/Containers/Array.h"
}


#line 476 "C:\\p4\\Code\\Core/Containers/Array.h"
#line 11 "C:\\p4\\Code\\Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"
#line 1 "C:\\p4\\Code\\Core/Strings/AString.h"


#pragma once











class AString
{
public:
	explicit AString();
	explicit AString( uint32_t reserve );
	explicit AString( const AString & string );
	explicit AString( const char * string );
	explicit AString( const char * start, const char * end );
	~AString();

	inline uint32_t		GetLength() const	{ return m_Length; }
	inline uint32_t		GetReserved() const { return ( m_ReservedAndFlags & RESERVED_MASK ); }
	inline bool			IsEmpty() const		{ return ( m_Length == 0 ); }

	
	inline char *		Get()				{ return m_Contents; }
	inline const char * Get() const			{ return m_Contents; }
	inline char *		GetEnd()			{ return ( m_Contents + m_Length ); }
	inline const char *	GetEnd() const		{ return ( m_Contents + m_Length ); }
	inline char &		operator [] ( uint32_t index )		 { do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( index < m_Length ) ) { if ( AssertHandler::Failure( "index < m_Length", "C:\\p4\\Code\\Core/Strings/AString.h", 34 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); return m_Contents[ index ]; }
	inline const char & operator [] ( uint32_t index )  const { do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( index < m_Length ) ) { if ( AssertHandler::Failure( "index < m_Length", "C:\\p4\\Code\\Core/Strings/AString.h", 35 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); return m_Contents[ index ]; }

	
	static const AString & GetEmpty() { return s_EmptyAString; }

	
	inline AString & operator = ( const char * string ) { Assign( string ); return *this; }
	inline AString & operator = ( const AString & string ) { Assign( string ); return *this; }
	void Assign( const char * string );
	void Assign( const char * start, const char * end );
	void Assign( const AString & string );
	void Clear();

	
	void SetLength( uint32_t len );

	
	AString & operator += ( char c );
	AString & operator += ( const char * string );
	AString & operator += ( const AString & string );
	void Append( const char * string, size_t len );

	
	bool operator == ( const char * other ) const;
	bool operator == ( const AString & other ) const;
	inline bool operator != ( const AString & other ) const { return !(*this == other ); }
	int32_t CompareI( const AString & other ) const;
	inline bool operator < ( const AString & other ) const { return ( CompareI( other ) < 0 ); }

	inline bool MemoryMustBeFreed() const { return ( ( m_ReservedAndFlags & MEM_MUST_BE_FREED_FLAG ) == MEM_MUST_BE_FREED_FLAG ); }

	void Format( const char * fmtString, ... );
	void VFormat( const char * fmtString, va_list arg );

	void Tokenize( Array< AString > & tokens ) const;

	
	uint32_t Replace( char from, char to, uint32_t maxReplaces = 0 );
	uint32_t Replace( const char * from, const char * to, uint32_t maxReplaces = 0 );
	void ToLower();

	
	const char *	Find( char c, const char * startPos = nullptr ) const;
	char *			Find( char c, char * startPos = nullptr ) { return const_cast< char *>( ((const AString *)this)->Find( c, startPos ) ); }
	const char *	Find( const char * subString ) const;
	char *			Find( const char * subString ) { return const_cast< char *>( ((const AString *)this)->Find( subString ) ); }
	const char *	FindI( const char * subString ) const;
	const char *	FindLast( char c ) const;
	char *			FindLast( char c ) { return const_cast< char *>( ((const AString *)this)->FindLast( c ) ); }
	bool			EndsWith( char c ) const;
	bool			EndsWith( const char * string ) const;
	bool			EndsWithI( const char * other ) const;
	bool			EndsWithI( const AString & other ) const;
	bool			BeginsWith( char c ) const;
	bool			BeginsWith( const char * string ) const;
	bool			BeginsWith( const AString & string ) const;
	bool			BeginsWithI( const char * string ) const;
	bool			BeginsWithI( const AString & string ) const;

	
	static void Copy( const char * src, char * dst, size_t len );
	static size_t StrLen( const char * string );
	static int32_t StrNCmp( const char * a, const char * b, size_t num );
	static int32_t StrNCmpI( const char * a, const char * b, size_t num );

protected:
	enum { MEM_MUST_BE_FREED_FLAG	= 0x00000001 };
	enum { RESERVED_MASK			= 0xFFFFFFFE };

	inline void SetReserved( uint32_t reserved, bool mustFreeMemory )
	{ 
		do { __pragma(warning(push)) __pragma(warning(disable:4127)) if ( !( ( reserved & MEM_MUST_BE_FREED_FLAG ) == 0 ) ) { if ( AssertHandler::Failure( "( reserved & MEM_MUST_BE_FREED_FLAG ) == 0", "C:\\p4\\Code\\Core/Strings/AString.h", 106 ) ) { __debugbreak(); } } } while ( false ); __pragma(warning(pop)); 
		m_ReservedAndFlags = ( reserved ^ ( mustFreeMemory ? MEM_MUST_BE_FREED_FLAG : 0 ) );
	}
	__declspec( noinline ) void Grow( uint32_t newLen );		
	__declspec( noinline ) void GrowNoCopy( uint32_t newLen ); 

	char *		m_Contents;			
	uint32_t	m_Length;			
	uint32_t	m_ReservedAndFlags;	

	static const char * const   s_EmptyString;
	static const AString	s_EmptyAString;
};


#line 122 "C:\\p4\\Code\\Core/Strings/AString.h"
#line 12 "C:\\p4\\Code\\Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"



class CIncludeParser
{
public:
	explicit CIncludeParser();
	~CIncludeParser();

	bool ParseMSCL_Output( const char * compilerOutput, size_t compilerOutputSize );
	bool ParseMSCL_Preprocessed( const char * compilerOutput, size_t compilerOutputSize );
	bool ParseGCC_Preprocessed( const char * compilerOutput, size_t compilerOutputSize );

	const Array< AString > & GetIncludes() const { return m_Includes; }

	
	void SwapIncludes( Array< AString > & includes );

private:
	bool Parse( const char * compilerOutput,
				size_t compilerOutputSize,
				const char * startOfLineString,
				bool quoted );

	Array< AString > m_Includes;
};


#line 41 "C:\\p4\\Code\\Tools/FBuild/FBuildCore/Helpers/CIncludeParser.h"
#line 9 "c:\\p4\\code\\Tools\\FBuild\\FBuildTest\\Tests\\TestIncludeParser.cpp"



class TestIncludeParser : public UnitTest
{
private:
	virtual void RunTests(); virtual const char * GetName() const; virtual bool RunThisTestOnly() const { return true; }

	void TestMSVCPreprocessedOutput() const;
	void TestMSVCShowIncludesOutput() const;
	void TestGCCPreprocessedOutput() const;
};



class RegisterTestsForTestIncludeParser { public: RegisterTestsForTestIncludeParser() { m_Test = new TestIncludeParser; UnitTestManager::RegisterTestGroup( m_Test ); } ~RegisterTestsForTestIncludeParser() { UnitTestManager::DeRegisterTestGroup( m_Test ); delete m_Test; } TestIncludeParser * m_Test; } s_RegisterTestsForTestIncludeParser; const char * TestIncludeParser::GetName() const { return "TestIncludeParser"; } void TestIncludeParser::RunTests() { UnitTestManager & utm = UnitTestManager::Get(); (void)utm;
	utm.TestBegin( "TestMSVCPreprocessedOutput" ); TestMSVCPreprocessedOutput(); utm.TestEnd();;
	utm.TestBegin( "TestMSVCShowIncludesOutput" ); TestMSVCShowIncludesOutput(); utm.TestEnd();;
	utm.TestBegin( "TestGCCPreprocessedOutput" ); TestGCCPreprocessedOutput(); utm.TestEnd();;
}



void TestIncludeParser::TestMSVCPreprocessedOutput() const
{
}



void TestIncludeParser::TestMSVCShowIncludesOutput() const
{
}



void TestIncludeParser::TestGCCPreprocessedOutput() const
{
}


