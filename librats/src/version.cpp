#include "version.h"
#include <iostream>
#include <iomanip>
#include "rats_export.h"

namespace librats {
    namespace version {
        
        RATS_API void rats_print_version_info() {
            std::cout << "Version: " << STRING << std::endl;
            std::cout << "Git: " << GIT_DESCRIBE << std::endl;
            std::cout << "Build: " << BUILD << std::endl;
        }
        
        RATS_API void rats_print_header() {
            std::cout << ASCII_HEADER << std::endl;
            std::cout << "           Version: " << std::left << std::setw(10) << STRING 
                      << "  Build: " << BUILD << std::endl;
            std::cout << "           Git: " << GIT_DESCRIBE << std::endl;
            std::cout << "        ========================================        " << std::endl;
            std::cout << std::endl;
        }
    }
}
