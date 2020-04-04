cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("C:\\Program Files\\Lockheed Martin\\Prepar3D v4 SDK 4.5.13.32097\\inc\\SimConnect")
link_directories("C:\\Program Files\\Lockheed Martin\\Prepar3D v4 SDK 4.5.13.32097\\lib\\SimConnect")

add_executable (ThrottleCtrl "ThrottleCtrl.cpp" "ThrottleCtrl.h")

target_link_libraries(ThrottleCtrl user32.lib AdvApi32.lib Shell32.lib SimConnect.lib)
