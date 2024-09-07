zlib:x64-windows-static                           1.3                 A compression library
zlib:x86-windows                                  1.3                 A compression library
zlib:x86-windows-static                           1.3                 A compression library
zlib:x64-windows								  1.3                 A compression library

zstd:x64-windows-static                           1.5.5#1             Zstandard - Fast real-time compression algorithm
zstd:x86-windows                                  1.5.5#1             Zstandard - Fast real-time compression algorithm
zstd:x86-windows-static                           1.5.5#1             Zstandard - Fast real-time compression algorithm
zstd:x64-windows								  1.5.5#1             Zstandard - Fast real-time compression algorithm

libjpeg-turbo:x64-windows-static                  3.0.0#1             libjpeg-turbo is a JPEG image codec that uses SI...
libjpeg-turbo:x86-windows                         3.0.0#1             libjpeg-turbo is a JPEG image codec that uses SI...
libjpeg-turbo:x86-windows-static                  3.0.0#1             libjpeg-turbo is a JPEG image codec that uses SI...
libjpeg-turbo:x64-windows  						  3.0.0#1             libjpeg-turbo is a JPEG image codec that uses SI...

liblzma:x64-windows-static                        5.4.3#1             Compression library with an API similar to that ...
liblzma:x86-windows                               5.4.3#1             Compression library with an API similar to that ...
liblzma:x86-windows-static                        5.4.3#1             Compression library with an API similar to that ...
liblzma:x64-windows                         	  5.4.3#1             Compression library with an API similar to that ...


Install All ? Or Just you need packages x64-windows-static Triplet only ? I will doing the second choice on Windows with Visual Studio 2022 and for Cmake on Windows Terminal command line Ninja (or just you need may some packages are already install, list packages installed with command "vckpg list" (see vcpkg-list-sample.txt from Visual Studio command line terminal [beware of the vckpg environment from Visual Studio which use the Manisfest mode rather that the Classic mode see that https://learn.microsoft.com/fr-fr/vcpkg/get_started/get-started-vs?pivots=shell-cmd . Sorry the only answer work quickly is Uninstall the vcpkg optional package from your Visual Studio instance.] and copy that on text file on your computer) :

Packages previously install on the process of the file cmake\readme.txt (Section: # Windows with cmake, generate Visual Studio project files)
vcpkg install zlib:x64-windows-static
vcpkg install zstd:x64-windows-static
vcpkg install libjpeg-turbo:x64-windows-static
vcpkg install liblzma:x64-windows-static
vcpkg install openssl:x64-windows-static

Packages to install (if you want to install all packages)
vcpkg install zlib:x86-windows
vcpkg install zlib:x86-windows-static
vcpkg install zlib:x64-windows
vcpkg install zlib:x64-windows-static				<- Already install if you have follow cmake\readme.txt

vcpkg install zstd:x86-windows
vcpkg install zstd:x86-windows-static
vcpkg install zstd:x64-windows
vcpkg install zstd:x64-windows-static				<- Already install if you have follow cmake\readme.txt

vcpkg install libjpeg-turbo:x86-windows
vcpkg install libjpeg-turbo:x86-windows-static
vcpkg install libjpeg-turbo:x64-windows
vcpkg install libjpeg-turbo:x64-windows-static		<- Already install if you have following cmake\readme.txt

vcpkg install liblzma:x86-windows
vcpkg install liblzma:x86-windows-static
vcpkg install liblzma:x64-windows
vcpkg install liblzma:x64-windows-static			<- Already install if you have following cmake\readme.txt


Packages to install (if you want to install x64-windows-static triplet only packages and you have insert VCPKG_DEFAULT_TRIPLET on system variables)

vcpkg install zlib									<- Already install if you have follow cmake\readme.txt

vcpkg install zstd									<- Already install if you have follow cmake\readme.txt

vcpkg install libjpeg-turbo							<- Already install if you have following cmake\readme.txt

vcpkg install liblzma								<- Already install if you have following cmake\readme.txt

vcpkg integrate install
