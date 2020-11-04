/*=================================================*/
/* Copyright (c) 2010,2011 NextMove Software, Inc. */
/*=================================================*/
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

#include "CaffeineFix.h"
namespace CaffeineFix {
#include "../include/dict.h"
}


static char *inpname;
static char *outname;


static void CheckWord(const std::string &str, std::ostream &ofs)
{
  if (!CaffeineFix::match(CaffeineFix::fsm,str))
  {
    std::string tmp;

    ofs << str << std::endl;
    if (CaffeineFix::prefix(CaffeineFix::fsm,str))
    {
      tmp = CaffeineFix::complete(CaffeineFix::fsm,str);
      if (!tmp.empty())
        ofs << "  .. complete: " << tmp << std::endl;
      else
        ofs << "  .. prefix: Y" << std::endl;
    }
    else
      ofs << "  .. prefix: N" << std::endl;
    tmp = CaffeineFix::suggest(CaffeineFix::fsm,str);
    if (!tmp.empty())
      ofs << "  .. suggest: " << tmp << std::endl;
  }
}


static void ProcessFile(std::istream &ifs, std::ostream &ofs)
{
  std::string line;

  while (getline(ifs,line))
  {
    char ch = line[0];
    if (ch == '#'  || ch == '\0' ||
        ch == '\n' || ch == '\r' ||
        ch == '\t')
      continue;

    CheckWord(line,ofs);
  }
}


static void DisplayUsage(void)
{
  std::cerr << "usage:  TestCaffeineFix2 [<infile> [<outfile>]]" << std::endl;
  std::exit(1);
}


static void ProcessCommandLine(int argc, char *argv[])
{
  char *ptr;
  int i, j;

  inpname = (char*)0;
  outname = (char*)0;

  j = 0;
  for (i=1; i<argc; i++)
  {
    ptr = argv[i];
    if (ptr[0]=='-' && ptr[1])
    {
      DisplayUsage();
    }
    else switch( j++ )
    {
    case 0:  inpname = ptr;  break;
    case 1:  outname = ptr;  break;
    default: DisplayUsage();
    }
  }

  if (!j)
    DisplayUsage();
}


int main(int argc, char *argv[])
{
  std::ifstream ifs;
  std::ofstream ofs;
  std::istream *ifp;
  std::ostream *ofp;

  ProcessCommandLine(argc,argv);

  if(inpname && inpname[0]!='-')
  {
    ifs.open(inpname);
    if(!ifs)
    {
      std::cerr << "Error: Unable to open input file!" << std::endl;
      std::exit(1);
    }
    ifp = &ifs;
  }
  else ifp = &std::cin;

  if(outname && outname[0]!='-')
  {
    ofs.open(outname);
    if(!ofs)
    {
      std::cerr << "Error: Unable to create output file!" << std::endl;
      std::exit(1);
    }
    ofp = &ofs;
  }
  else ofp = &std::cout;

  ProcessFile(*ifp,*ofp);

  ofs.close();
  ifs.close();
  return 0;
}

