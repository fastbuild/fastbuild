clear

# Include paths
INCLUDEPATHS="-I ~/p4/Code -I`pwd` -I`pwd`/../External/LZ4/lz4-r127"

if [[ $OSTYPE == darwin* ]]; then
	DEFINES="-DDEBUG -D__OSX__ -D__APPLE__"
	LINKEROPTIONS=
    OPTIONS="-x c++ -c -g -O0 -Wall -Wfatal-errors -std=c++11"
else
	DEFINES="-DDEBUG -D__LINUX__"
	LINKEROPTIONS=-pthread
    OPTIONS="-c -g -O0 -Wall -Wfatal-errors -std=c++11 -Wno-invalid-offsetof"
fi

mkdir -p ../tmp

#if false; then

# Core
echo Core - compiling...
mkdir -p ../tmp/Core
mkdir -p ../tmp/Core/Env
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Env/Env.cpp -o ../tmp/Core/Env/Env.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Env/Assert.cpp -o ../tmp/Core/Env/Assert.o
mkdir -p ../tmp/Core/FileIO
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/FileIO/ConstMemoryStream.cpp -o ../tmp/Core/FileIO/ConstMemoryStream.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/FileIO/FileIO.cpp -o ../tmp/Core/FileIO/FileIO.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/FileIO/FileStream.cpp -o ../tmp/Core/FileIO/FileStream.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/FileIO/IOStream.cpp -o ../tmp/Core/FileIO/IOStream.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/FileIO/MemoryStream.cpp -o ../tmp/Core/FileIO/MemoryStream.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/FileIO/PathUtils.cpp -o ../tmp/Core/FileIO/PathUtils.o
mkdir -p ../tmp/Core/Math
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Math/Mat44.cpp -o ../tmp/Core/Math/Mat44.o
mkdir -p ../tmp/Core/Math/MurmurHash3
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Math/CRC32.cpp -o ../tmp/Core/Math/CRC32.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Math/Random.cpp -o ../tmp/Core/Math/Random.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Math/MurmurHash3/MurmurHash3.cpp -o ../tmp/Core/Math/MurmurHash3/MurmurHash3.o
mkdir -p ../tmp/Core/Mem
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Mem/Mem.cpp -o ../tmp/Core/Mem/Mem.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Mem/MemPoolBlock.cpp -o ../tmp/Core/Mem/MemPoolBlock.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Mem/MemTracker.cpp -o ../tmp/Core/Mem/MemTracker.o
mkdir -p ../tmp/Core/Network
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Network/Network.cpp -o ../tmp/Core/Network/Network.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Network/NetworkStartupHelper.cpp -o ../tmp/Core/Network/NetworkStartupHelper.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Network/TCPConnectionPool.cpp -o ../tmp/Core/Network/TCPConnectionPool.o
mkdir -p ../tmp/Core/Process
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Process/Mutex.cpp -o ../tmp/Core/Process/Mutex.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Process/Process.cpp -o ../tmp/Core/Process/Process.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Process/SharedMemory.cpp -o ../tmp/Core/Process/SharedMemory.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Process/SystemMutex.cpp -o ../tmp/Core/Process/SystemMutex.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Process/Thread.cpp -o ../tmp/Core/Process/Thread.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Process/Semaphore.cpp -o ../tmp/Core/Process/Semaphore.o
mkdir -p ../tmp/Core/Profile
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Profile/ProfileManager.cpp -o ../tmp/Core/Profile/ProfileManager.o
mkdir -p ../tmp/Core/Strings
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Strings/AString.cpp -o ../tmp/Core/Strings/AString.o
mkdir -p ../tmp/Core/Time
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Time/Timer.cpp -o ../tmp/Core/Time/Timer.o
mkdir -p ../tmp/Core/Tracing
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Tracing/Tracing.cpp -o ../tmp/Core/Tracing/Tracing.o
mkdir -p ../tmp/Core/Reflection
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/RefObject.cpp -o ../tmp/Core/Reflection/RefObject.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/ReflectionIter.cpp -o ../tmp/Core/Reflection/ReflectionIter.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/ReflectionInfo.cpp -o ../tmp/Core/Reflection/ReflectionInfo.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/ReflectedProperty.cpp -o ../tmp/Core/Reflection/ReflectedProperty.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/PropertyType.cpp -o ../tmp/Core/Reflection/PropertyType.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/Object.cpp -o ../tmp/Core/Reflection/Object.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/Container.cpp -o ../tmp/Core/Reflection/Container.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/BindReflection.cpp -o ../tmp/Core/Reflection/BindReflection.o
mkdir -p ../tmp/Core/Reflection/Serialization
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/Serialization/TextWriter.cpp -o ../tmp/Core/Reflection/Serialization/TextWriter.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/Serialization/TextReader.cpp -o ../tmp/Core/Reflection/Serialization/TextReader.o
mkdir -p ../tmp/Core/Reflection/MetaData
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/MetaData/MetaData.cpp -o ../tmp/Core/Reflection/MetaData/MetaData.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/MetaData/Meta_Range.cpp -o ../tmp/Core/Reflection/MetaData/Meta_Range.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/MetaData/Meta_Path.cpp -o ../tmp/Core/Reflection/MetaData/Meta_Path.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/MetaData/Meta_Optional.cpp -o ../tmp/Core/Reflection/MetaData/Meta_Optional.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/Reflection/MetaData/Meta_File.cpp -o ../tmp/Core/Reflection/MetaData/Meta_File.o
echo Core - archiving...
pushd ../tmp/Core/ > /dev/null
ar rcs libCore.a Env/Env.o Env/Assert.o FileIO/ConstMemoryStream.o FileIO/FileIO.o FileIO/FileStream.o FileIO/IOStream.o FileIO/MemoryStream.o FileIO/PathUtils.o Math/Mat44.o Math/CRC32.o Math/Random.o Math/MurmurHash3/MurmurHash3.o Mem/Mem.o Mem/MemPoolBlock.o Mem/MemTracker.o Network/Network.o Network/NetworkStartupHelper.o Network/TCPConnectionPool.o Process/Mutex.o Process/Process.o Process/SharedMemory.o Process/SystemMutex.o Process/Thread.o Process/Semaphore.o Profile/ProfileManager.o Strings/AString.o Time/Timer.o Tracing/Tracing.o Reflection/RefObject.o Reflection/ReflectionIter.o Reflection/ReflectionInfo.o Reflection/ReflectedProperty.o Reflection/PropertyType.o Reflection/Object.o Reflection/Container.o Reflection/BindReflection.o Reflection/Serialization/TextWriter.o Reflection/Serialization/TextReader.o Reflection/MetaData/MetaData.o Reflection/MetaData/Meta_Range.o Reflection/MetaData/Meta_Path.o Reflection/MetaData/Meta_Optional.o Reflection/MetaData/Meta_File.o
popd > /dev/null

# Core tests
echo CoreTest - compiling...
mkdir -p ../tmp/Core/CoreTest/Tests
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestAtomic.cpp -o ../tmp/Core/CoreTest/Tests/TestAtomic.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestAString.cpp -o ../tmp/Core/CoreTest/Tests/TestAString.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestFileIO.cpp -o ../tmp/Core/CoreTest/Tests/TestFileIO.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestHash.cpp -o ../tmp/Core/CoreTest/Tests/TestHash.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestMemPoolBlock.cpp -o ../tmp/Core/CoreTest/Tests/TestMemPoolBlock.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestMutex.cpp -o ../tmp/Core/CoreTest/Tests/TestMutex.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestPathUtils.cpp -o ../tmp/Core/CoreTest/Tests/TestPathUtils.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestTCPConnectionPool.cpp -o ../tmp/Core/CoreTest/Tests/TestTCPConnectionPool.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestTimer.cpp -o ../tmp/Core/CoreTest/Tests/TestTimer.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/TestMain.cpp -o ../tmp/Core/CoreTest/TestMain.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestReflection.cpp -o ../tmp/Core/CoreTest/Tests/TestReflection.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Core/CoreTest/Tests/TestSemaphore.cpp -o ../tmp/Core/CoreTest/Tests/TestSemaphore.o
echo CoreTest - archiving...
pushd ../tmp/Core/CoreTest > /dev/null
ar rcs libCoreTest.a Tests/TestAtomic.o Tests/TestAString.o Tests/TestFileIO.o Tests/TestHash.o Tests/TestMemPoolBlock.o Tests/TestMutex.o Tests/TestPathUtils.o Tests/TestTCPConnectionPool.o Tests/TestTimer.o TestMain.o Tests/TestReflection.o Tests/TestSemaphore.o
popd > /dev/null

# TestFramework
echo TestFramework - compiling...
mkdir -p ../tmp/TestFramework
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./TestFramework/UnitTestManager.cpp -o ../tmp/TestFramework/UnitTestManager.o
echo TestFramework - archiving...
pushd ../tmp/TestFramework > /dev/null
ar rcs libTestFramework.a UnitTestManager.o
popd > /dev/null

# FBuildApp
echo FBuildApp - compiling...
mkdir -p ../tmp/Tools/FBuild/FBuildApp
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildApp/Main.cpp -o ../tmp/Tools/FBuild/FBuildApp/Main.o
echo FBuildApp - archiving...
pushd ../tmp/Tools/FBuild/FBuildApp > /dev/null
ar rcs libFBuildApp.a Main.o
popd > /dev/null

# FBuildCore
echo FBuildCore - compiling...
mkdir -p ../tmp/Tools/FBuild/FBuildCore
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Error.cpp -o ../tmp/Tools/FBuild/FBuildCore/Error.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/FBuild.cpp -o ../tmp/Tools/FBuild/FBuildCore/FBuild.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/FBuildOptions.cpp -o ../tmp/Tools/FBuild/FBuildCore/FBuildOptions.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/FLog.cpp -o ../tmp/Tools/FBuild/FBuildCore/FLog.o
mkdir -p ../tmp/Tools/FBuild/FBuildCore/BFF
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/BFFIterator.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/BFFIterator.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/BFFParser.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/BFFParser.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/BFFStackFrame.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/BFFStackFrame.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/BFFVariable.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/BFFVariable.o
mkdir -p ../tmp/Tools/FBuild/FBuildCore/BFF/Functions
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/Function.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/Function.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionAlias.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionAlias.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionCompiler.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionCompiler.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionCopy.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionCopy.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionCopyDir.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionCopyDir.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionCSAssembly.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionCSAssembly.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionDLL.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionDLL.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionExec.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionExec.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionExecutable.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionExecutable.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionForEach.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionForEach.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionLibrary.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionLibrary.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionObjectList.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionObjectList.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionPrint.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionPrint.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionSettings.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionSettings.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionTest.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionTest.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionUnity.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionUnity.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionUsing.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionUsing.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/BFF/Functions/FunctionVCXProject.cpp -o ../tmp/Tools/FBuild/FBuildCore/BFF/Functions/FunctionVCXProject.o
mkdir -p ../tmp/Tools/FBuild/FBuildCore/Cache
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Cache/Cache.cpp -o ../tmp/Tools/FBuild/FBuildCore/Cache/Cache.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Cache/CachePlugin.cpp -o ../tmp/Tools/FBuild/FBuildCore/Cache/CachePlugin.o
mkdir -p ../tmp/Tools/FBuild/FBuildCore/Graph
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/AliasNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/AliasNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/CompilerNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/CompilerNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/CopyDirNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/CopyDirNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/CopyNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/CopyNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/CSNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/CSNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/Dependencies.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/Dependencies.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/DirectoryListNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/DirectoryListNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/DLLNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/DLLNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/ExecNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/ExecNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/ExeNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/ExeNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/FileNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/FileNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/LibraryNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/LibraryNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/LinkerNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/LinkerNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/Node.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/Node.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/NodeGraph.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/NodeGraph.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/NodeProxy.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/NodeProxy.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/ObjectListNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/ObjectListNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/ObjectNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/ObjectNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/TestNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/TestNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/UnityNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/UnityNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/VCXProjectNode.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/VCXProjectNode.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Graph/MetaData/Meta_Name.cpp -o ../tmp/Tools/FBuild/FBuildCore/Graph/Meta_Name.o
mkdir -p ../tmp/Tools/FBuild/FBuildCore/Helpers
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Helpers/Args.cpp -o ../tmp/Tools/FBuild/FBuildCore/Helpers/Args.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Helpers/CIncludeParser.cpp -o ../tmp/Tools/FBuild/FBuildCore/Helpers/CIncludeParser.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Helpers/Compressor.cpp -o ../tmp/Tools/FBuild/FBuildCore/Helpers/Compressor.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Helpers/FBuildStats.cpp -o ../tmp/Tools/FBuild/FBuildCore/Helpers/FBuildStats.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Helpers/Report.cpp -o ../tmp/Tools/FBuild/FBuildCore/Helpers/Report.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Helpers/ResponseFile.cpp -o ../tmp/Tools/FBuild/FBuildCore/Helpers/ResponseFile.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Helpers/ToolManifest.cpp -o ../tmp/Tools/FBuild/FBuildCore/Helpers/ToolManifest.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.cpp -o ../tmp/Tools/FBuild/FBuildCore/Helpers/VSProjectGenerator.o
mkdir -p ../tmp/Tools/FBuild/FBuildCore/Protocol
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Protocol/Client.cpp -o ../tmp/Tools/FBuild/FBuildCore/Protocol/Client.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Protocol/Protocol.cpp -o ../tmp/Tools/FBuild/FBuildCore/Protocol/Protocol.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/Protocol/Server.cpp -o ../tmp/Tools/FBuild/FBuildCore/Protocol/Server.o
mkdir -p ../tmp/Tools/FBuild/FBuildCore/WorkerPool
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/WorkerPool/Job.cpp -o ../tmp/Tools/FBuild/FBuildCore/WorkerPool/Job.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/WorkerPool/JobQueue.cpp -o ../tmp/Tools/FBuild/FBuildCore/WorkerPool/JobQueue.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.cpp -o ../tmp/Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerage.cpp -o ../tmp/Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerage.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.cpp -o ../tmp/Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildCore/WorkerPool/WorkerThreadRemote.cpp -o ../tmp/Tools/FBuild/FBuildCore/WorkerPool/WorkerThreadRemote.o
echo FBuildCore - archiving...
pushd ../tmp/Tools/FBuild/FBuildCore > /dev/null
ar rcs libFBuildCore.a Error.o FBuild.o FBuildOptions.o FLog.o BFF/BFFIterator.o BFF/BFFParser.o BFF/BFFStackFrame.o BFF/BFFVariable.o BFF/Functions/Function.o BFF/Functions/FunctionAlias.o BFF/Functions/FunctionCompiler.o BFF/Functions/FunctionCopy.o BFF/Functions/FunctionCopyDir.o BFF/Functions/FunctionCSAssembly.o BFF/Functions/FunctionDLL.o BFF/Functions/FunctionExec.o BFF/Functions/FunctionExecutable.o BFF/Functions/FunctionForEach.o BFF/Functions/FunctionLibrary.o BFF/Functions/FunctionObjectList.o BFF/Functions/FunctionPrint.o BFF/Functions/FunctionSettings.o BFF/Functions/FunctionTest.o BFF/Functions/FunctionUnity.o BFF/Functions/FunctionUsing.o BFF/Functions/FunctionVCXProject.o Cache/Cache.o Cache/CachePlugin.o Graph/AliasNode.o Graph/CompilerNode.o Graph/CopyDirNode.o Graph/CopyNode.o Graph/CSNode.o Graph/Dependencies.o Graph/DirectoryListNode.o Graph/DLLNode.o Graph/ExecNode.o Graph/ExeNode.o Graph/FileNode.o Graph/LibraryNode.o Graph/LinkerNode.o Graph/Node.o Graph/NodeGraph.o Graph/NodeProxy.o Graph/ObjectListNode.o Graph/ObjectNode.o Graph/TestNode.o Graph/UnityNode.o Graph/VCXProjectNode.o Helpers/Args.o Helpers/CIncludeParser.o Helpers/Compressor.o Helpers/FBuildStats.o Helpers/Report.o Helpers/ResponseFile.o Helpers/ToolManifest.o Helpers/VSProjectGenerator.o Protocol/Client.o Protocol/Protocol.o Protocol/Server.o WorkerPool/Job.o WorkerPool/JobQueue.o WorkerPool/JobQueueRemote.o WorkerPool/WorkerBrokerage.o WorkerPool/WorkerThread.o WorkerPool/WorkerThreadRemote.o Graph/Meta_Name.o
popd > /dev/null

# FBuildTest
echo FBuildTest - compiling...
mkdir -p ../tmp/Tools/FBuild/FBuildTest
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/TestMain.cpp -o ../tmp/Tools/FBuild/FBuildTest/TestMain.o
mkdir -p ../tmp/Tools/FBuild/FBuildTest/Tests
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/FBuildTest.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/FBuildTest.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestAlias.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestAlias.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestBFFParsing.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestBFFParsing.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestBuildAndLinkLibrary.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestBuildAndLinkLibrary.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestBuildFBuild.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestBuildFBuild.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestCachePlugin.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestCachePlugin.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestCLR.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestCLR.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestCompressor.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestCompressor.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestCopy.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestCopy.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestCSharp.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestCSharp.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestCUDA.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestCUDA.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestDistributed.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestDistributed.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestDLL.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestDLL.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestExe.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestExe.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestGraph.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestGraph.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestIncludeParser.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestIncludeParser.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestPrecompiledHeaders.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestPrecompiledHeaders.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestProjectGeneration.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestProjectGeneration.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestResources.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestResources.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestTest.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestTest.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestUnity.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestUnity.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildTest/Tests/TestVariableStack.cpp -o ../tmp/Tools/FBuild/FBuildTest/Tests/TestVariableStack.o
echo FBuildTest - archiving...
pushd ../tmp/Tools/FBuild/FBuildTest > /dev/null
ar rcs libFBuildTest.a TestMain.o Tests/FBuildTest.o Tests/TestAlias.o Tests/TestBFFParsing.o Tests/TestBuildAndLinkLibrary.o Tests/TestBuildFBuild.o Tests/TestCachePlugin.o Tests/TestCLR.o Tests/TestCompressor.o Tests/TestCopy.o Tests/TestCSharp.o Tests/TestCUDA.o Tests/TestDistributed.o Tests/TestDLL.o Tests/TestExe.o Tests/TestGraph.o Tests/TestIncludeParser.o Tests/TestPrecompiledHeaders.o Tests/TestProjectGeneration.o Tests/TestResources.o Tests/TestTest.o Tests/TestUnity.o Tests/TestVariableStack.o
popd > /dev/null

# FBuildWorker
echo FBuildWorker - compiling...
mkdir -p ../tmp/Tools/FBuild/FBuildWorker
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildWorker/Main.cpp -o ../tmp/Tools/FBuild/FBuildWorker/Main.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildWorker/FBuildWorkerOptions.cpp -o ../tmp/Tools/FBuild/FBuildWorker/FBuildWorkerOptions.o
mkdir -p ../tmp/Tools/FBuild/FBuildWorker/Worker
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildWorker/Worker/IdleDetection.cpp -o ../tmp/Tools/FBuild/FBuildWorker/Worker/IdleDetection.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildWorker/Worker/Worker.cpp -o ../tmp/Tools/FBuild/FBuildWorker/Worker/Worker.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildWorker/Worker/WorkerSettings.cpp -o ../tmp/Tools/FBuild/FBuildWorker/Worker/WorkerSettings.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ./Tools/FBuild/FBuildWorker/Worker/WorkerWindow.cpp -o ../tmp/Tools/FBuild/FBuildWorker/Worker/WorkerWindow.o
echo FBuildWorker - archiving...
pushd ../tmp/Tools/FBuild/FBuildWorker > /dev/null
ar rcs libFBuildWorker.a Main.o FBuildWorkerOptions.o Worker/IdleDetection.o Worker/Worker.o Worker/WorkerSettings.o Worker/WorkerWindow.o
popd > /dev/null

# LZ4
echo LZ4 - compiling...
mkdir -p ../tmp/LZ4
g++  $OPTIONS $INCLUDEPATHS $DEFINES ../External/LZ4/lz4-r127/lz4.c -o ../tmp/LZ4/lz4.o
g++  $OPTIONS $INCLUDEPATHS $DEFINES ../External/LZ4/lz4-r127/xxhash.c -o ../tmp/LZ4/xxhash.o
echo LZ4 - archiving...
pushd ../tmp/LZ4 > /dev/null
ar rcs libLZ4.a lz4.o
popd > /dev/null

#fi

#----- LINK -----
mkdir -p ../bin
echo CoreTest - linking...
g++ ../tmp/Core/CoreTest/libCoreTest.a ../tmp/TestFramework/libTestFramework.a ../tmp/Core/libCore.a ../tmp/xxHash/xxhash.o $LINKEROPTIONS -o ../bin/coretest
echo FBuildTest - linking...
g++ ../tmp/Tools/FBuild/FBuildTest/libFBuildTest.a ../tmp/Tools/FBuild/FBuildCore/libFBuildCore.a ../tmp/Core/libCore.a ../tmp/TestFramework/libTestFramework.a ../tmp/LZ4/libLZ4.a $LINKEROPTIONS -o ../bin/fbuildtest
echo FBuildApp - linking...
g++ ../tmp/Tools/FBuild/FBuildApp/libFBuildApp.a ../tmp/Tools/FBuild/FBuildCore/libFBuildCore.a ../tmp/Core/libCore.a ../tmp/LZ4/libLZ4.a $LINKEROPTIONS -o ../bin/fbuild
echo FBuildWorker - linking...
g++ ../tmp/Tools/FBuild/FBuildWorker/libFBuildWorker.a ../tmp/Tools/FBuild/FBuildCore/libFBuildCore.a ../tmp/Core/libCore.a ../tmp/LZ4/libLZ4.a $LINKEROPTIONS -o ../bin/fbuildworker


