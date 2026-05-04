#ifndef _UDT_TEST_UTIL_H_
#define _UDT_TEST_UTIL_H_

struct UDTUpDown{
   UDTUpDown()
   {
      // use this function to initialize the UDT library
      UDT::startup();
   }
   ~UDTUpDown()
   {
      // use this function to release the UDT library
      UDT::cleanup();
   }
};

#endif
