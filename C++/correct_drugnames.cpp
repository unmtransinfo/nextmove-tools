/*=================================================*/
/* Copyright (c) 2010,2011 NextMove Software, Inc. */
/*=================================================*/
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

#include "CaffeineFix.h"
namespace CaffeineFix {
#include "data/drug_synonyms.h"
}

static unsigned int count;
static char *inpname;
static char *outname;

static void ProcessLine(const std::string &str, std::ostream &ofs)
{
  if (!CaffeineFix::cmatch(CaffeineFix::fsm,str))
  {
    std::string tmp = CaffeineFix::suggest(CaffeineFix::fsm,str);
    if (!tmp.empty())
    {
      ofs << tmp << std::endl;
      count++;
      return;
    }
  }
  ofs << str << std::endl;
}

static void ProcessFile(std::istream &ifs, std::ostream &ofs)
{
  std::string line;

  count = 0;
  while (getline(ifs,line))
  {
    char ch = line[0];
    if (ch!='#' && ch!='\t' && ch!='\0')
      ProcessLine(line,ofs);
    else ofs << line << std::endl;
  }
  if (count != 1)
    std::cerr << count << " changes" << std::endl;
  else std::cerr << "1 change" << std::endl;
}

static void DisplayUsage(void)
{
  std::cerr << "usage:  correct_drugnames [-c][-v] [<infile> [<outfile>]]" << std::endl;
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
