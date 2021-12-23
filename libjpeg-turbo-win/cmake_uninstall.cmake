# This code is from the CMake FAQ

if (NOT EXISTS "C:/Users/rudi/Desktop/libjpeg-turbo-2.1.2/install_manifest.txt")
  message(FATAL_ERROR "Cannot find install manifest: \"C:/Users/rudi/Desktop/libjpeg-turbo-2.1.2/install_manifest.txt\"")
endif(NOT EXISTS "C:/Users/rudi/Desktop/libjpeg-turbo-2.1.2/install_manifest.txt")

file(READ "C:/Users/rudi/Desktop/libjpeg-turbo-2.1.2/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
list(REVERSE files)
foreach (file ${files})
  message(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
    if (EXISTS "$ENV{DESTDIR}${file}")
      execute_process(
        COMMAND "C:/Program Files (x86)/CMake/bin/cmake.exe" -E remove "$ENV{DESTDIR}${file}"
        OUTPUT_VARIABLE rm_out
        RESULT_VARIABLE rm_retval
      )
    if(NOT ${rm_retval} EQUAL 0)
      message(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
    endif (NOT ${rm_retval} EQUAL 0)
  else (EXISTS "$ENV{DESTDIR}${file}")
    message(STATUS "File \"$ENV{DESTDIR}${file}\" does not exist.")
  endif (EXISTS "$ENV{DESTDIR}${file}")
endforeach(file)
