## How To Set Up
TogetherMake uses VCPKG, you can install it by cloning it or manually downloading it from Github:

https://github.com/microsoft/vcpkg

"Install VCPKG.bat" will do this (using Git) but it can be very slow.

Put VCPKG in a folder called "vcpkg" next to Server and Client folders.

Then run the "Install Packages.bat" file which will download and build all required libraries.

If you have VCPKG installed system wide then you might need to change the include and library directories for both projects