// Report
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Report.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/FBuildStats.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args
#include <string.h>
#if defined( __WINDOWS__ )
    #include <windows.h> // for ::GetTimeFormat
#endif

// Globals
//------------------------------------------------------------------------------
uint32_t g_ReportNodeColors[Node::NUM_NODE_TYPES] = {
	0x000000, // PROXY_NODE (never seen)
	0xFFFFFF, // COPY_NODE
	0xAAAAAA, // DIRECTORY_LIST_NODE
	0x000000, // EXEC_NODE
	0x888888, // FILE_NODE
	0x88FF88, // LIBRARY_NODE
	0xFF8888, // OBJECT_NODE
	0x228B22, // ALIAS_NODE
	0xFFFF88, // EXE_NODE
	0x88AAFF, // UNITY_NODE
	0x88CCFF,	// CS_NODE
	0xFFAAFF, // TEST_NODE
	0xDDA0DD, // COMPILER_NODE
	0xFFCC88, // DLL_NODE
	0xFFFFFF, // VCXPROJ_NODE
	0x444444, // OBJECT_LIST_NODE
	0x000000, // COPY_DIR_NODE (never seen)
	0xFF3030, // REMOVE_DIR_NODE
	0x77DDAA, // SLN_NODE
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
Report::Report()
	: m_LibraryStats( 512, true )
	, m_NumPieCharts( 0 )
{
	// Compile time check to ensure color vector is in sync
	CTASSERT( sizeof( g_ReportNodeColors ) / sizeof (uint32_t) == Node::NUM_NODE_TYPES );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Report::~Report()
{
	const LibraryStats * const * end = m_LibraryStats.End();
	for ( LibraryStats ** it=m_LibraryStats.Begin(); it != end; ++it )
	{
		FDELETE *it;
	}
}

// Generate
//------------------------------------------------------------------------------
void Report::Generate( const FBuildStats & stats )
{
	Timer t;

	// pre-allocate a large string for output
	m_Output.SetReserved( MEGABYTE );
	m_Output.SetLength( 0 );

	// generate some common data used in reporting
	GetLibraryStats( stats );

	// build the report
	CreateHeader();

	CreateTitle();

	CreateOverview( stats );

	DoCPUTimeByType( stats );
	DoCacheStats( stats );
	DoCPUTimeByLibrary();
	DoCPUTimeByItem( stats );

	DoIncludes();

	CreateFooter();

	// patch in time take
	const float time = t.GetElapsed();
	AStackString<> timeTakenBuffer;
	stats.FormatTime( time, timeTakenBuffer );
	char * placeholder = m_Output.Find( "^^^^    " );
	memcpy( placeholder, timeTakenBuffer.Get(), timeTakenBuffer.GetLength() );
	
}

// Save
//------------------------------------------------------------------------------
void Report::Save() const
{
	FileStream f;
	if ( f.Open( "report.html", FileStream::WRITE_ONLY ) )
	{
		f.Write( m_Output.Get(), m_Output.GetLength() );
	}
}

// CreateHeader
//------------------------------------------------------------------------------
void Report::CreateHeader()
{
	const char * header = 
		"<!doctype html>\n"
		"<style type=\"text/css\">\n"
		"body{\n"
		"font-family:arial;\n"
		"width:1000px;\n"
		"margin:0 auto;\n"
		"font-size:13px\n"
		"}\n"
		"th{\n"
		"background-color:#e0e0e0;\n"
		"padding:4px;\n"
		"height:15px;\n"
		"text-align:left;\n"
		"border-top-left-radius:5px;\n"
		"-moz-border-radius-topleft:5px;\n"
		"border-top-right-radius:5px;\n"
		"-moz-border-radius-topright:5px;\n"
		"font-weight:bold;\n"
		"}\n"
		"tr:nth-child(odd){\n"
		"background-color:#f0f0f0;\n"
		"}\n"
		"h1{\n"
		"background-color:#b0b0b0;\n"
		"border-radius:5px;\n"
		"padding:4px;\n"
		"height:40px;\n"
		"text-align:left;\n"
		"font-weight:bold;\n"
		"}\n"
		"h2{\n"
		"background-color:#d0d0d0;\n"
		"border-radius:5px;\n"
		"padding:4px;\n"
		"height:27px;\n"
		"text-align:left;\n"
		"font-weight:bold;\n"
		"}\n"
		"table{\n"
		"  padding-left:10px;\n"
		"}\n"
		"td\n"
		"word-break:break-all;\n"
		"padding:3px;\n"
		"}\n"
		".perc{\n"
		"color:#aaaaaa;\n"
		"font-size:10px\n"
		"}\n"
		"</style>\n"

		"<html>\n"
		"<head>\n"
		"<meta charset=\"UTF-8\" />\n"
		"<title>FASTBuild Report</title>\n"
		"</head>\n"
		"<body>\n"
		"<script type=\"text/javascript\">\n"
		"function getTotal(data)\n"
		"{\n"
		" var total = 0;\n"
		" for (var j = 0; j < data.length; j++)\n"
		" {\n"
		"  total += (typeof data[j] == 'number') ? data[j] : 0;\n"
		" }\n"
		" return total;\n"
		"}\n"
		"function plotData(canvasName,myData,myLabels,myColor, units)\n"
		"{\n"
		" var canvas;\n"
		" var ctx;\n"
		" var lastend = 0;\n"
		" var myTotal = getTotal(myData);\n"
		  
		" canvas = document.getElementById(canvasName);\n"
		" ctx = canvas.getContext(\"2d\");\n"
		" ctx.clearRect(0, 0, canvas.width, canvas.height);\n"
		  
		" ctx.fillStyle = \"black\";\n"
		" ctx.beginPath();\n"
		" ctx.moveTo(70,70);\n"
		" ctx.arc(70,70,66,0,(Math.PI*2),false);\n"
		" ctx.lineTo(70,70);\n"
		" ctx.fill();\n"
		  
		" for (var i = 0; i < myData.length; i++)\n"
		" {\n"
		"  ctx.fillStyle = myColor[i];\n"
		"  ctx.beginPath();\n"
		"  ctx.moveTo(70,70);\n"
		"  ctx.arc(70,70,65,lastend,lastend+(Math.PI*2*(myData[i]/myTotal)),false);\n"
		"  ctx.lineTo(70,70);\n"
		"  ctx.fill();\n"
		"  lastend += Math.PI*2*(myData[i]/myTotal);\n"
		" }\n"

		// legend
		" for (var i = 0; i < myData.length; i++)\n"
		" {\n"
		"  var y = 20 + ( i * 25 );\n"

		// color box
		"  ctx.fillStyle=\"#888888\";\n"
		"  ctx.fillRect(200-1,y-15-1,22,22)\n"
		"  ctx.fillStyle=myColor[ i ];\n"
		"  ctx.fillRect(200,y-15,20,20)\n"

		// text
		"  ctx.fillStyle = \"black\";\n"
		"  ctx.font = \"16px Arial\";\n"
		"  ctx.fillText(myLabels[ i ], 230, y );\n"

		// value
		"  var perc = ( 100.0 * myData[ i ] / myTotal ).toFixed(1);\n"
		"  ctx.fillText( perc + \"% (\" + myData[ i ] + units + \")\", 330, y );\n"
		" }\n"
		"}\n"

		// toggleTable
		"function toggleTable(tableName)\n"
		"{\n"
		" var table = document.getElementById(tableName);\n"
		" table.style.display = (table.style.display == \"table\") ? \"none\" : \"table\";\n"
		"}\n"

		"</script>\n";
	m_Output += header;
}

// CreateTitle
//------------------------------------------------------------------------------
void Report::CreateTitle()
{

	Write( "<h1>FASTBuild Report</h1>\n" );
}

// CreateOverview
//------------------------------------------------------------------------------
void Report::CreateOverview( const FBuildStats & stats )
{
	DoSectionTitle( "Overview", "overview" );

	AStackString<> buffer;

	DoTableStart();

	// Headings
	Write( "<tr><th width=150>Item</th><th>Details</th></tr>\n" );

	// Full command line
    AStackString<> commandLineBuffer;
    Env::GetCmdLine( commandLineBuffer );
    #if defined( __WINDOWS__ )
        const char * exeExtension = strstr( commandLineBuffer.Get(), ".exe\"" );
        const char * commandLine = exeExtension ? ( exeExtension + 5 ) : commandLineBuffer.Get(); // skip .exe + closing quote
    #else
        const char * commandLine = commandLineBuffer.Get();
    #endif
	Write( "<tr><td width=80>Cmd Line Options</td><td>%s</td></tr>", commandLine );

	// Target
	AStackString<> targets;
	const Node * rootNode = stats.GetRootNode();
	if (rootNode->GetType() != Node::PROXY_NODE)
	{
		targets = rootNode->GetName();
	}
	else
	{
		const Dependencies & childNodes = rootNode->GetStaticDependencies();
		size_t num = childNodes.GetSize();
		for ( size_t i=0; i<num; ++i )
		{
			if ( i != 0 )
			{
				targets += ", ";
			}
			const Node * child = childNodes[ i ].GetNode();
			targets += child->GetName();
		}
	}
	Write( "<tr><td>Target(s)</td><td>%s</td></tr>\n", targets.Get() );

	// Result
	const bool buildOK = ( stats.GetRootNode()->GetState() == Node::UP_TO_DATE );
	Write( "<tr><td>Result</td><td>%s</td></tr>\n", buildOK ? "OK" : "FAILED" );

	// Real Time
	float totalBuildTime = stats.m_TotalBuildTime;
	stats.FormatTime( totalBuildTime, buffer );
	Write( "<tr><td>Time</td><td>%s</td></tr>\n", buffer.Get() );

	// Local CPU Time 
	float totalLocalCPUInSeconds = (float)( (double)stats.m_TotalLocalCPUTimeMS / (double)1000 );
	stats.FormatTime( totalLocalCPUInSeconds, buffer );
	float localRatio = ( totalLocalCPUInSeconds / totalBuildTime );
	Write( "<tr><td>CPU Time</td><td>%s (%2.1f:1)</td></tr>\n", buffer.Get(), localRatio );

	// version info
	Write( "<tr><td>Version</td><td>%s %s</td></tr>\n", FBUILD_VERSION_STRING, FBUILD_VERSION_PLATFORM );

	// report time
	char timeBuffer[ 256 ];
	#if defined( __WINDOWS__ )
		VERIFY( ::GetTimeFormat( LOCALE_NAME_USER_DEFAULT,		// LCID Locale
								 TIME_FORCE24HOURFORMAT,		// DWORD dwFlags
								 nullptr,						// SYSTEMTIME *lpTime
								 nullptr,						// LPCTSTR lpFormat,
								 timeBuffer,					// LPTSTR lpTimeStr,
								 256 ) );						// int cchTime
	#elif defined( __APPLE__ )
		timeBuffer[ 0 ] = '\000'; // TODO:MAC Implement GetTimeFormat in Report
	#elif defined( __LINUX__ )
		timeBuffer[ 0 ] = '\000'; // TODO:LINUX Implement GetTimeFormat in Report
	#else
		#error Unknown platform
	#endif

	char dateBuffer[ 256 ];
	#if defined( __WINDOWS__ )
		VERIFY( ::GetDateFormat( LOCALE_NAME_USER_DEFAULT,		// LCID Locale
								 DATE_LONGDATE,					// DWORD dwFlags
								 nullptr,						// SYSTEMTIME *lpTime
								 nullptr,						// LPCTSTR lpFormat,
								 dateBuffer,					// LPTSTR lpTimeStr,
								 256 ) );						// int cchTime
	#elif defined( __APPLE__ )
		dateBuffer[ 0 ] = '\000'; // TODO:MAC Implement GetDateFormat in Report
	#elif defined( __LINUX__ )
		dateBuffer[ 0 ] = '\000'; // TODO:LINUX Implement GetDateFormat in Report
	#else
		#error Unknown platform
	#endif

	// NOTE: leave space to patch in time taken later "^^^^                          "
	Write( "<tr><td>Report Generated</td><td>^^^^                         - %s %s</td></tr>\n", dateBuffer, timeBuffer );

	DoTableStop();
}

// DoCacheStats
//------------------------------------------------------------------------------
void Report::DoCacheStats( const FBuildStats & stats )
{
	(void)stats;

	DoSectionTitle( "Cache Stats", "cacheStats" );

	const FBuildOptions & options = FBuild::Get().GetOptions();
	if ( options.m_UseCacheRead || options.m_UseCacheWrite )
	{
		// avoid writing useless table
		uint32_t totalOutOfDateItems( 0 );
		uint32_t totalCacheable( 0 );
		uint32_t totalCacheHits( 0 );
		const LibraryStats * const * end = m_LibraryStats.End();
		for ( LibraryStats ** it = m_LibraryStats.Begin(); it != end; ++it )
		{
			const LibraryStats & ls = *( *it );
			totalOutOfDateItems += ls.objectCount_OutOfDate;
			totalCacheable += ls.objectCount_Cacheable;
			totalCacheHits += ls.objectCount_CacheHits;
		}
		if ( totalOutOfDateItems == 0 )
		{
			Write( "No cacheable items were built.\n" );
			return;
		}
		uint32_t totalCacheMisses( totalCacheable - totalCacheHits );

		Array< PieItem > pieItems( 3, false );
		pieItems.Append(PieItem("Uncacheable", (float)(totalOutOfDateItems - totalCacheable), 0xFF8888));
		pieItems.Append(PieItem("Cache Miss", (float)totalCacheMisses, 0xFFCC88));
		pieItems.Append(PieItem("Cache Hit", (float)totalCacheHits, 0x88FF88));
		DoPieChart(pieItems, "");

		DoTableStart();

		// Headings
		Write( "<tr><th>Library</th><th style=\"width:70px;\">Items</th><th style=\"width:90px;\">Out-of-Date</th><th style=\"width:90px;\">Cacheable</th><th style=\"width:70px;\">Hits</th><th style=\"width:70px;\">Misses</th><th style=\"width:60px;\">Stores</th></tr>\n" );

		size_t numOutput( 0 );

		// items
		for ( LibraryStats ** it = m_LibraryStats.Begin(); it != end; ++it )
		{
			const LibraryStats & ls = *( *it );
			const char * libraryName = ls.library->GetName().Get();

			// total items in library
			const uint32_t items = ls.objectCount;

			// out of date items
			const uint32_t	outOfDateItems		= ls.objectCount_OutOfDate;
			if ( outOfDateItems == 0 )
			{
				continue; // skip library if nothing was done
			}
			const float		outOfDateItemsPerc	= ( (float)outOfDateItems / (float)items ) * 100.0f;

			// cacheable
			const uint32_t	cItems		= ls.objectCount_Cacheable;
			const float		cItemsPerc	= ( (float)cItems / (float)outOfDateItems ) * 100.0f;

			// hits
			const uint32_t	cHits		= ls.objectCount_CacheHits;
			const float		cHitsPerc	= ( cItems > 0 ) ? ( (float)cHits / (float)cItems ) * 100.0f : 0.0f;

			// misses
			const uint32_t	cMisses		= ( cItems - cHits );
			const float		cMissesPerc	= ( cMisses > 0 ) ? 100.0f - cHitsPerc : 0.0f;

			// stores
			const uint32_t	cStores		= ls.objectCount_CacheStores;

			// start collapsable section
			if ( numOutput == 10 )
			{
				DoToggleSection();
			}

			Write( ( numOutput == 10 ) ? "<tr></tr><tr><td>%s</td><td style=\"width:70px;\">%u</td><td style=\"width:90px;\">%u <font class='perc'>(%2.1f%%)</font></td><td style=\"width:90px;\">%u <font class='perc'>(%2.1f%%)</font></td><td style=\"width:70px;\">%u <font class='perc'>(%2.1f%%)</font></td><td style=\"width:70px;\">%u <font class='perc'>(%2.1f%%)</font></td><td style=\"width:60px;\">%u</td></tr>\n"
									   : "<tr><td>%s</td><td>%u</td><td>%u <font class='perc'>(%2.1f%%)</font></td><td>%u <font class='perc'>(%2.1f%%)</font></td><td>%u <font class='perc'>(%2.1f%%)</font></td><td>%u <font class='perc'>(%2.1f%%)</font></td><td>%u</td></tr>\n", 
						libraryName, 
						items,
						outOfDateItems, outOfDateItemsPerc,
						cItems, cItemsPerc,
						cHits, cHitsPerc,
						cMisses, cMissesPerc,
						cStores  );
			numOutput++;
		}

		DoTableStop();

		if ( numOutput > 10 )
		{
			Write( "</details>\n" );
		}
	}
	else
	{
		Write( "Cache not used.\n" );
	}
}

// DoCPUTimeByType
//------------------------------------------------------------------------------
void Report::DoCPUTimeByType( const FBuildStats & stats )
{
	DoSectionTitle( "CPU Time by Node Type", "cpuTimeByNodeType" );

	// Summary Pie Chart
	Array< PieItem > items( 32, true );
	 
	for ( size_t i=0; i < (size_t)Node::NUM_NODE_TYPES; ++i )
	{
		const FBuildStats::Stats & nodeStats = stats.GetStatsFor( (Node::Type)i );
		if ( nodeStats.m_NumProcessed == 0 )
		{
			continue;
		}

		// label
		const char * typeName = Node::GetTypeName( Node::Type( i ) );
		const float value = (float)( (double)nodeStats.m_ProcessingTimeMS / (double)1000 );
		const uint32_t color = g_ReportNodeColors[ i ];

		PieItem item( typeName, value, color, (void *)i );
		items.Append( item );
	}

	items.Sort();

	// pie chart
	DoPieChart( items, " s" );

	// table
	DoTableStart();
	Write( "<tr><th width=80>Type</th><th width=80>Time</th><th width=80>Processed</th><th width=80>Built</th><th width=80>Cache Hits</th></tr>\n" );
	for ( size_t i=0; i < items.GetSize(); ++i )
	{
		Node::Type type = (Node::Type)(size_t)items[ i ].userData;
		const FBuildStats::Stats & nodeStats = stats.GetStatsFor( type );
		if ( nodeStats.m_NumProcessed == 0 )
		{
			continue;
		}

		const char * typeName = Node::GetTypeName( type );
		const float value = (float)( (double)nodeStats.m_ProcessingTimeMS / (double)1000 );
		const uint32_t processed = nodeStats.m_NumProcessed;
		const uint32_t built = nodeStats.m_NumBuilt;
		const uint32_t cacheHits = nodeStats.m_NumCacheHits;

		Write( "<tr><td>%s</td><td>%2.3fs</td><td>%u</td><td>%u</td>", 
					typeName, 
					value, 
					processed, 
					built, 
					cacheHits );
		if ( type == Node::OBJECT_NODE )
		{
			// cacheable
			Write( "<td>%u</td></tr>\n", cacheHits );
		}
		else
		{
			// non-cacheable
			Write( "<td>-</td></tr>\n" );
		}
	}
	DoTableStop();
}

// DoCPUTimeByItem
//------------------------------------------------------------------------------
void Report::DoCPUTimeByItem( const FBuildStats & stats )
{
	DoSectionTitle( "CPU Time by Item", "cpuTimeByItem" );

	DoTableStart();

	// Headings
	Write( "<tr><th style=\"width:100px;\">Time</th><th style=\"width:100px;\">Type</th><th>Name</th></tr>\n" );

	size_t numOutput = 0;

	// Result
	const Array< const Node * > & nodes = stats.GetNodesByTime();
	for ( const Node ** it = nodes.Begin();
		  it != nodes.End();
		  ++ it )
	{
		const Node * node = *it;
		float time = ( (float)node->GetProcessingTime() * 0.001f ); // ms to s
		const char * type = node->GetTypeName();
		const char * name = node->GetName().Get();

		// start collapsable section
		if ( numOutput == 10 )
		{
			DoToggleSection( (uint32_t)nodes.GetSize() - 10 );
		}

		Write( ( numOutput == 10 ) ? "<tr></tr><tr><td style=\"width:100px;\">%2.3fs</td><td style=\"width:100px;\">%s</td><td>%s</td></tr>\n"
								   : "<tr><td>%2.3fs</td><td>%s</td><td>%s</td></tr>\n", time, type, name );
		numOutput++;
	}

	DoTableStop();

	if ( numOutput > 10 )
	{
		Write( "</details>\n" );
	}
}

// DoCPUTimeByLibrary
//------------------------------------------------------------------------------
void Report::DoCPUTimeByLibrary()
{
	DoSectionTitle( "CPU Time by Library", "cpuTimeByLibrary" );

	DoTableStart();

	// total
	uint32_t total = 0;
	const LibraryStats * const *  end = m_LibraryStats.End();
	for ( LibraryStats ** it = m_LibraryStats.Begin(); it != end; ++it )
	{
		total += ( *it )->cpuTimeMS;
	}
	if ( total == 0 )
	{
		Write( "No libraries built.\n" );
		return;
	}

	// Headings
	Write( "<tr><th style=\"width:80px;\">Time</th><th style=\"width:50px;\">%%</th><th style=\"width:70px;\">Obj Built</th><th style=\"width:50px;\">Type</th><th>Name</th></tr>\n" );

	const float totalS = (float)( (double)total * 0.001 );
	size_t numOutput( 0 );
	// Result
	for ( LibraryStats ** it = m_LibraryStats.Begin(); it != end; ++it )
	{
		const LibraryStats & ls = *( *it );
		if ( ls.cpuTimeMS == 0 )
		{
			continue;
		}

		// start collapsable section
		if ( numOutput == 10 )
		{
			DoToggleSection();
		}

		const uint32_t objCount = ls.objectCount_OutOfDate;
		const float time = ( (float)ls.cpuTimeMS * 0.001f ); // ms to s
		const float perc = (float)( (double)time / (double)totalS * 100 );
		const char * type = ls.library->GetTypeName();
		switch ( ls.library->GetType() )
		{
			case Node::LIBRARY_NODE: type = "Static"; break;
			case Node::DLL_NODE: type = "DLL"; break;
			case Node::CS_NODE: type = "C# DLL"; break;
			case Node::OBJECT_LIST_NODE: type = "ObjectList"; break;
			default: break;
		}
		const char * name = ls.library->GetName().Get();
		Write( ( numOutput == 10 ) ? "<tr></tr><tr><td style=\"width:80px;\">%2.3fs</td><td style=\"width:50px;\">%2.1f</td><td style=\"width:70px;\">%u</td><td style=\"width:50px;\">%s</td><td>%s</td></tr>\n"
								   : "<tr><td>%2.3fs</td><td>%2.1f</td><td>%u</td><td>%s</td><td>%s</td></tr>\n",
										time, perc, objCount, type, name );
		numOutput++;
	}

	DoTableStop();

	if ( numOutput > 10 )
	{
		Write( "</details>\n" );
	}
}

// DoIncludes
//------------------------------------------------------------------------------
PRAGMA_DISABLE_PUSH_MSVC( 6262 ) // warning C6262: Function uses '262212' bytes of stack
void Report::DoIncludes()
{
	DoSectionTitle( "Includes", "includes" );

	size_t numLibsOutput = 0;

	// build per-library stats
	const LibraryStats * const * end = m_LibraryStats.End();
	for ( LibraryStats ** it = m_LibraryStats.Begin(); it != end; ++it )
	{
		if ( ( *it )->objectCount_OutOfDate == 0 )
		{
			continue;
		}

		// get all the includes for this library
		const Node * library = ( *it )->library;
		IncludeStatsMap incStatsMap;
		GetIncludeFilesRecurse( incStatsMap, library );

		// flatten and sort by usage
		Array< const IncludeStats * > incStats( 10 * 1024, true );
		incStatsMap.Flatten( incStats );
		incStats.SortDeref();

		Write( "<h3>%s</h3>\n", library->GetName().Get() );
		numLibsOutput++;

		if ( incStats.GetSize() == 0 )
		{
			Write( "No inludes.\n" );
			continue;
		}

		DoTableStart();
		Write( "<tr><th style=\"width:80px;\">Objects</th><th style=\"width:80px;\">Included</td><th style=\"width:60px;\">PCH</th><th>Name</th></tr>\n" );

		const uint32_t numObjects = ( *it )->objectCount;

		// output
		const size_t numIncludes = incStats.GetSize();
		size_t numOutput = 0;
		for ( size_t i=0; i<numIncludes; ++i )
		{
			const IncludeStats & s = *incStats[ i ];
			const char * fileName = s.node->GetName().Get();
			const uint32_t included = s.count;
			const bool inPCH = s.inPCH;

			// start collapsable section
			if ( numOutput == 10 )
			{
				DoToggleSection( numIncludes - 10 );
			}

			Write( ( numOutput == 10 ) ? "<tr></tr><tr><td style=\"width:80px;\">%u</td><td style=\"width:80px;\">%u</td><td style=\"width:60px;\">%s</td><td>%s</td></tr>\n"
									   : "<tr><td>%u</td><td>%u</td><td>%s</td><td>%s</td></tr>\n", 
						numObjects, 
						included, 
						inPCH ? "YES" : "no", 
						fileName );
			numOutput++;
		}

		DoTableStop();

		// end collpsable section
		if ( numOutput > 10 )
		{
			Write( "</details>\n" );
		}
	}

	DoTableStop();

	if ( numLibsOutput == 0 )
	{
		Write( "No libraries built.\n" );
	}
}
PRAGMA_DISABLE_POP_MSVC // warning C6262: Function uses '262212' bytes of stack

// DoPieChart
//------------------------------------------------------------------------------
void Report::DoPieChart( const Array< PieItem > & items, const char * units )
{
	AStackString<> buffer;

	uint32_t height = Math::Max< uint32_t >( 140, 40 + 25 * (uint32_t)items.GetSize() );

	m_NumPieCharts++;

	Write( "<section>\n" );
	Write( "<div>\n" );
	Write( "<canvas id=\"canvas%u\" width=\"500\" height=\"%u\">\n", m_NumPieCharts, height );
	Write( "HTML5 Canvas support required.\n" );
	Write( "</canvas>\n" );
	Write( "</div>\n" );

	Write( "<script type=\"text/javascript\">\n" );
	Write( "	var myData = [" );
	for ( size_t i=0; i<items.GetSize(); ++i )
	{
		if ( i > 0 )
		{
			Write( "," );
		}
		buffer.Format( "%2.3f", items[ i ].value );
		Write( buffer.Get() );
	}
	Write( "];\n" );
	Write( "	var myLabels = [" );
	for ( size_t i=0; i<items.GetSize(); ++i )
	{
		if ( i > 0 )
		{
			Write( "," );
		}
		Write( "\"%s\"", items[ i ].label );
	}
	Write( "];\n" );
	Write( "	var myColors = [" );
	for ( size_t i=0; i<items.GetSize(); ++i )
	{
		if ( i > 0 )
		{
			Write( "," );
		}
		Write( "\"#%x\"", items[ i ].color );
	}
	Write( "];\n" );

	Write( "	plotData(\"canvas%u\",myData,myLabels,myColors,\"%s\");\n", m_NumPieCharts, units );
	Write( "</script>\n" );
	Write( "</section>\n" );
}

// CreateFooter
//------------------------------------------------------------------------------
void Report::CreateFooter()
{
	const char * footer = 
		"<br><br><br>\n"
		"</body>\n"
		"</html>\n";
	m_Output += footer;
}

// DoSectionTitle
//------------------------------------------------------------------------------
void Report::DoSectionTitle( const char * sectionName, const char * sectionId )
{
	Write( "<h2 id=\"%s\">%s</h2>\n", sectionId, sectionName );
}

// DoTableStart
//------------------------------------------------------------------------------
void Report::DoTableStart( int width, const char * id, bool hidden )
{
	AStackString<> output;
	output.Format( "<table width=%u", width );
	if ( id )
	{
		output += " id=\"";
		output += id;
		output += "\"";
	}
	if ( hidden )
	{
		output += " style=\"display:none;\"";
	}
	output += ">\n";
	Write( output.Get() );
}

// DoTableStop
//------------------------------------------------------------------------------
void Report::DoTableStop()
{
	Write( "</table>\n" );
}

// DoToggleSection
//------------------------------------------------------------------------------
void Report::DoToggleSection( size_t numMore )
{
	static int tableId = 0;
	++tableId;
	AStackString<> tableIdStr;
	tableIdStr.Format( "table%u", tableId );

	DoTableStop();
	AStackString<> more;
	if ( numMore )
	{
		more.Format( "%u ", (uint32_t)numMore );
	}
	Write( "<a href='javascript:toggleTable(\"%s\");'>%sMore...</a>\n", tableIdStr.Get(), more.Get() );
	DoTableStart( DEFAULT_TABLE_WIDTH, tableIdStr.Get(), true ); // hide table
}

// Write
//------------------------------------------------------------------------------
void Report::Write( const char * fmtString, ... )
{
	AStackString< 1024 > tmp;

	va_list args;
	va_start(args, fmtString);
	tmp.VFormat( fmtString, args );
	va_end( args );

	// resize output buffer in large chunks to prevent re-sizing
	if ( m_Output.GetLength() + tmp.GetLength() > m_Output.GetReserved() )
	{
		m_Output.SetReserved( m_Output.GetReserved() + MEGABYTE );
	}

	m_Output += tmp;
}

// GetLibraryStats
//------------------------------------------------------------------------------
void Report::GetLibraryStats(  const FBuildStats & stats )
{
	// gather library stats, sorted by CPU cost
	GetLibraryStatsRecurse( m_LibraryStats, stats.GetRootNode(), nullptr );
	m_LibraryStats.SortDeref();
}

// GetLibraryStatsRecurse
//------------------------------------------------------------------------------
void Report::GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Node * node, LibraryStats * currentLib ) const
{
	// skip nodes we've already seen
	if ( node->GetStatFlag( Node::STATS_REPORT_PROCESSED ) )
	{
		return;
	}
	node->SetStatFlag( Node::STATS_REPORT_PROCESSED );

	Node::Type type = node->GetType();

	// object?
	if ( type == Node::OBJECT_NODE )
	{
		if ( currentLib == nullptr )
		{
			ASSERT( false ); // should not be possible with a correctly formed dep graph
			return;
		}

		currentLib->objectCount++;

		bool cacheHit = node->GetStatFlag( Node::STATS_CACHE_HIT );
		bool cacheMiss = node->GetStatFlag( Node::STATS_CACHE_MISS );
		if ( cacheHit || cacheMiss )
		{
			currentLib->objectCount_Cacheable++;

			if ( cacheHit )
			{
				currentLib->objectCount_CacheHits++;
			}
			if ( node->GetStatFlag( Node::STATS_CACHE_STORE ) )
			{
				currentLib->objectCount_CacheStores++;
			}
		}

		if ( cacheHit || cacheMiss || node->GetStatFlag( Node::STATS_BUILT ) )
		{
			currentLib->objectCount_OutOfDate++;
			currentLib->cpuTimeMS += node->GetProcessingTime();
		}

		return; // Stop recursing at Objects
	}	

	bool isLibrary = false;
	switch ( type )
	{
		case Node::DLL_NODE:		isLibrary = true; break;
		case Node::LIBRARY_NODE:	isLibrary = true; break;
		case Node::OBJECT_LIST_NODE: isLibrary = true; break;
		case Node::CS_NODE:
		{
			isLibrary = node->GetName().EndsWithI( ".dll" ); // TODO:C - robustify this (could have an aribtrary extension)
			break;
		}
		default: break; // not a library
	}

	if ( isLibrary )
	{
		currentLib = FNEW( LibraryStats );
		currentLib->library = node;
		currentLib->cpuTimeMS = 0;
		currentLib->objectCount = 0;
		currentLib->objectCount_OutOfDate = 0;
		currentLib->objectCount_Cacheable = 0;
		currentLib->objectCount_CacheHits = 0;
		currentLib->objectCount_CacheStores = 0;

		// count time for library/dll itself
		if ( node->GetStatFlag( Node::STATS_BUILT ) )
		{
			currentLib->cpuTimeMS += node->GetProcessingTime();
		}

		libStats.Append( currentLib );

		// recurse into this new lib
	}

	// Dependencies
	GetLibraryStatsRecurse( libStats, node->GetPreBuildDependencies(), currentLib );
	GetLibraryStatsRecurse( libStats, node->GetStaticDependencies(), currentLib );
	GetLibraryStatsRecurse( libStats, node->GetDynamicDependencies(), currentLib );
}

// GetLibraryStatsRecurse
//------------------------------------------------------------------------------
void Report::GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Dependencies & dependencies, LibraryStats * currentLib ) const
{
	const Dependency * const end = dependencies.End();
	for ( const Dependency * it = dependencies.Begin(); it != end; ++it )
	{
		GetLibraryStatsRecurse( libStats, it->GetNode(), currentLib );
	}
}

// GetIncludeFilesRecurse
//------------------------------------------------------------------------------
void Report::GetIncludeFilesRecurse( IncludeStatsMap & incStats, const Node * node ) const
{
	Node::Type type = node->GetType();
	if ( type == Node::OBJECT_NODE )
	{
		// Dynamic Deps
		const Dependencies & dynamicDeps = node->GetDynamicDependencies();
		const Dependency * const end = dynamicDeps.End();
		for ( const Dependency * it = dynamicDeps.Begin(); it != end; ++it )
		{
			AddInclude( incStats, it->GetNode(), node );
		}

		return;
	}

	// Static Deps
	const Dependencies & staticDeps = node->GetStaticDependencies();
	const Dependency * end = staticDeps.End();
	for ( const Dependency * it = staticDeps.Begin(); it != end; ++it )
	{
		GetIncludeFilesRecurse( incStats, it->GetNode() );
	}

	// Dynamic Deps
	const Dependencies & dynamicDeps = node->GetDynamicDependencies();
	end = dynamicDeps.End();
	for ( const Dependency * it = dynamicDeps.Begin(); it != end; ++it )
	{
		GetIncludeFilesRecurse( incStats, it->GetNode() );
	}
}

// AddInclude
//------------------------------------------------------------------------------
void Report::AddInclude( IncludeStatsMap & incStats, const Node * node, const Node * parentNode ) const
{
	bool isHeaderInPCH = false;
	if ( parentNode->GetType() == Node::OBJECT_NODE )
	{
		const ObjectNode * obj = parentNode->CastTo< ObjectNode >();
		isHeaderInPCH = obj->IsCreatingPCH();
	}

	// check for existing
	IncludeStats * stats = incStats.Find( node );
	if ( stats == nullptr )
	{
		stats = incStats.Insert( node );
	}

	stats->count++;
	stats->inPCH |= isHeaderInPCH;
}

// IncludeStatsMap (CONSTRUCTOR)
//------------------------------------------------------------------------------
Report::IncludeStatsMap::IncludeStatsMap()
	: m_Pool( sizeof( IncludeStats ), __alignof( IncludeStats ) )
{
	memset( m_Table, 0, sizeof( m_Table ) );
}

// IncludeStatsMap (DESTRUCTOR)
//------------------------------------------------------------------------------
Report::IncludeStatsMap::~IncludeStatsMap()
{
	for ( size_t i=0; i<65536; ++i )
	{
		IncludeStats * item = m_Table[ i ];
		while ( item )
		{
			IncludeStats * next = item->m_Next;
			m_Pool.Free( item );
			item = next;
		}
	}
}

// Find
//------------------------------------------------------------------------------
Report::IncludeStats * Report::IncludeStatsMap::Find( const Node * node ) const
{
	// caculate table entry
	uint32_t hash = node->GetNameCRC();
	uint32_t key = ( hash & 0xFFFF );
	IncludeStats * item = m_Table[ key ];

	// check linked list
	while ( item )
	{
		if ( item->node == node )
		{
			return item;
		}
		item = item->m_Next;
	}

	// not found
	return nullptr;
}

// Insert
//------------------------------------------------------------------------------
Report::IncludeStats * Report::IncludeStatsMap::Insert( const Node * node )
{
	// caculate table entry
	uint32_t hash = node->GetNameCRC();
	uint32_t key = ( hash & 0xFFFF );

	// insert new item
	IncludeStats * newStats = (IncludeStats *)m_Pool.Alloc( sizeof( IncludeStats ) );
	newStats->node = node;
	newStats->count = 0;
	newStats->inPCH = false;
	newStats->m_Next = m_Table[ key ];
	m_Table[ key ] = newStats;

	return newStats;
}

// Flatten
//------------------------------------------------------------------------------
void Report::IncludeStatsMap::Flatten( Array< const IncludeStats * > & stats ) const
{
	for ( size_t i=0; i<65536; ++i )
	{
		IncludeStats * item = m_Table[ i ];
		while ( item )
		{
			IncludeStats * next = item->m_Next;
			stats.Append( item );
			item = next;
		}
	}
}

//------------------------------------------------------------------------------
