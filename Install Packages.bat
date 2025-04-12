cd vcpkg

vcpkg integrate install

vcpkg install enet
vcpkg install libnice
vcpkg install imgui[glfw-binding,opengl3-binding] --recurse
vcpkg install glew
vcpkg install glm