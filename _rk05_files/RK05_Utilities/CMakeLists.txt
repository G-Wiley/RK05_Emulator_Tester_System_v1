project(RK05Utilities)
add_executable(RK05Simh2Bin Source/RK05Simh2Bin.cpp Source/RK05Util.cpp)
add_executable(RK05Bin2Simh Source/RK05Bin2Simh.cpp Source/RK05Util.cpp)
add_executable(RK05BinInfo Source/RK05BinInfo.cpp Source/RK05Util.cpp)
add_executable(RK05BinRelabel Source/RK05BinRelabel.cpp Source/RK05Util.cpp)

