/*=================================================*/
/* Copyright (c) 2010,2011 NextMove Software, Inc. */
/*=================================================*/
#include <iostream>
#include <string>

#include "CaffeineFix.h"

namespace CaffeineFix {
#include "dict.h"
}


static void CheckWord(const std::string &str)
{
  if (!CaffeineFix::match(CaffeineFix::fsm,str))
  {
    std::string tmp;

    std::cout << str << " is wrong" << std::endl;
    if (CaffeineFix::prefix(CaffeineFix::fsm,str))
    {
      tmp = CaffeineFix::complete(CaffeineFix::fsm,str);
      if (!tmp.empty())
        std::cout << "  .. complete: " << tmp << std::endl;
      else
        std::cout << "  .. prefix: Y" << std::endl;
    }
    else
      std::cout << "  .. prefix: N" << std::endl;
    tmp = CaffeineFix::suggest(CaffeineFix::fsm,str);
    if (!tmp.empty())
      std::cout << "  .. suggest: " << tmp << std::endl;
  }
  else
    std::cout << str << " is correct" << std::endl;
}


int main()
{
  CheckWord("benzene");
  CheckWord("benzine");
  CheckWord("benzne");
  CheckWord("benz");
  CheckWord("ben");
  return 0;
}

