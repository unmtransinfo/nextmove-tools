/////////////////////////////////////////////////////////////////////////////
/// fuzzymatch_molnames.cpp
///   reference file of names is compiled into program
///   input SDF file with synonyms for each molecule
///   for each synonym determine if match (1) exact (2) fuzzy or (3) not.
///   
/// Fuzzy name matching is problematic with systematic chemical names,
/// which often differ in meaning with one character, e.g. "glutamic acid"
/// vs. "glutaric acid".  Hence one approach is to only use fuzzy matching
/// for names where name2struct fails.
///   
///   
///   Jeremy J Yang
///   10 Feb 2012
/////////////////////////////////////////////////////////////////////////////
#include "openeye.h"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include "CaffeineFix.h"
namespace CaffeineFix {
#include "data/drug_synonyms.h"
}

#include "oeplatform.h"
#include "oesystem.h"
#include "oechem.h"
#include "oeiupac.h"

using namespace OESystem;
using namespace OEChem;
using namespace OEIUPAC;

using namespace std;

static string progname;
static unsigned int verbose=0;
static string ifile="";
static string ofile=""; 
static string sdtags_str=""; 
static bool debug=false;

void Help(string msg)
{
  cerr << msg << endl
    <<"  "<<progname<<" [options]"             <<endl
                                               <<endl
    <<"    options:"                           <<endl
    <<"     -in INFILE ...... SD file with synonyms"                       <<endl
    <<"     -out OUTFILE .... SD file, annotated"                       <<endl
    <<"     -sdtags TAGS .... SD datatags for synonyms (comma delimited)"                       <<endl
    <<"     -v .............. verbose"                <<endl
    <<"     -vv ............. very verbose"                <<endl
    <<"     -vvv ............ very very verbose"                <<endl
    <<"     -h .............. help"                   <<endl
                                               <<endl;
  exit(1);
}

/////////////////////////////////////////////////////////////////////////////
void ProcessArgs(int argc,char *argv[])
{
  string a;
  progname=*argv;
  if (argc==1) Help("");
  for (++argv;--argc;++argv)
  {
    a=*argv;
    if (a=="-in")
    {
      if (--argc==0) Help("ERROR: "+a+" requires value.");
      ifile=*++argv;
    }
    else if (a=="-out")
    {
      if (--argc==0) Help("ERROR: "+a+" requires value.");
      ofile=*++argv;
    }
    else if (a=="-sdtags")
    {
      if (--argc==0) Help("ERROR: "+a+" requires value.");
      sdtags_str=*++argv;
    }
    else if (a=="-debug") debug=true;
    else if (a=="-v") verbose=1;
    else if (a=="-vv") verbose=2;
    else if (a=="-vvv") verbose=3;
    else if (a=="-h") Help("");
    else Help("ERROR: Bad option");
  }
}

/////////////////////////////////////////////////////////////////////////////
string str_trim(string str) /// Remove leading and trailing whitespace.
{
  unsigned int p;
  while (1)
  {
    if (str.empty()) break;
    p=str.find_first_of(" \t\n");
    if (p!=0) break;
    str=str.substr(1,str.length()-1);
  }
  while (1)
  {
    if (str.empty()) break;
    p=str.find_last_of(" \t\n");
    if (p!=0) break;
    str=str.substr(0,str.length()-1);
  }
  return str;
}

/////////////////////////////////////////////////////////////////////////////
string str_lower(string str) /// Lower case
{
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}
/////////////////////////////////////////////////////////////////////////////
vector<string> str_split(string str,string delims)
{
  vector<string> words;
  unsigned int p=0,q;
  q=str.find_first_of(delims);
  do {
    string word=(str.substr(p,q-p));
    word=str_trim(word);
    if (!word.empty())
      words.push_back(word);
    p=q+1;
    if (p>=str.length()) break;
    q=str.find_first_of(delims,p);
    if (q>str.length()) q=str.length();
  } while (q<=str.length());
  return words;
}

/////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  ProcessArgs(argc,argv);

  oemolistream ims;
  if (!ims.open(ifile.c_str()))
    OEThrow.Fatal("Unable to open \"%s\".\n",ifile.c_str());

  oemolostream oms;
  if (!oms.open(ofile.c_str()))
    OEThrow.Fatal("Unable to open \"%s\".\n",ofile.c_str());

  vector<string> sdtags = str_split(sdtags_str,",");

  for (vector<string>::iterator tag=sdtags.begin();tag!=sdtags.end();++tag)
    cerr << "DEBUG: sdtag: " << *tag << endl;

  //map<string, vector<int> > unames;

  OEPlatform::oeosstream oeerrs;
  OEThrow.SetOutputStream(oeerrs);

  OEGraphMol mol;
  OEGraphMol mol_from_name;
  int n_mol=0;
  int n_name=0;
  int n_name2struct=0;
  int n_exactmatch=0;
  int n_nocasematch=0;
  int n_fuzzymatch=0;
  while (OEReadMolecule(ims,mol))
  {
    ++n_mol;
    string smi;
    OECreateSmiString(smi,mol);
    if (verbose) cerr << "DEBUG:\t[" << n_mol << "] " << mol.GetTitle() << " " << smi <<endl;
    bool ok_name2struct=false;
    bool ok_exactmatch=false;
    bool ok_nocasematch=false;
    bool ok_fuzzymatch=false;
    for (vector<string>::iterator tag=sdtags.begin();tag!=sdtags.end();++tag)
    {
      string names_str=OEGetSDData(mol,*tag);
      vector<string> names = str_split(names_str,";");
      for (vector<string>::iterator name=names.begin();name!=names.end();++name)
      {
        ++n_name;
        if (verbose>1) cerr << "\tname: (" << *tag << ") \"" << *name << "\"" ;

        mol_from_name.Clear();
        ok_name2struct=OEParseIUPACName(mol_from_name,(*name).c_str());
        if (ok_name2struct)
        {
          if (verbose>1) cerr << "\t(PARSED OK BY OEIUPAC, n_atoms = " << mol_from_name.NumAtoms() << ")" <<endl;
          ++n_name2struct;
          continue;
        }
        ok_exactmatch=CaffeineFix::cmatch(CaffeineFix::fsm,*name);
        if (ok_exactmatch)
        {
          if (verbose) cerr << "\t(EXACT MATCH)" <<endl;
          ++n_exactmatch;
          continue;
        }
        string fuzz = CaffeineFix::suggest(CaffeineFix::fsm,*name);
        if (!fuzz.empty())
        ok_nocasematch=(str_lower(*name).compare(str_lower(fuzz))==0);
        if (ok_nocasematch)
        {
          if (verbose) cerr << "\t(NOCASE MATCH, \"" << *name << "\" ~ \"" << fuzz << "\")" <<endl;
          ++n_nocasematch;
          continue;
        }
        ok_fuzzymatch=(!fuzz.empty() && !ok_nocasematch);
        if (ok_fuzzymatch)
        {
          if (verbose) cerr << "\t(FUZZY MATCH, \"" << *name << "\" ~ \"" << fuzz << "\")" <<endl;
          ++n_fuzzymatch;
          //fprintf(stderr,"DEBUG: fuzz[0]=%d\n",(int)fuzz[0]);
          continue;
        }
        if (!ok_exactmatch && !ok_fuzzymatch && verbose>1) cerr << "\t(NO MATCH)" <<endl;
      }
      if (ok_exactmatch) break;
      if (ok_fuzzymatch) break;
    }
    if (!ok_exactmatch && !ok_fuzzymatch && verbose) cerr << "\t(NO MATCH)" <<endl;

    string errtxt=oeerrs.str();
    if (!errtxt.empty() && verbose>2) { cerr << errtxt <<endl; }
    oeerrs.clear();
  }

  OEThrow.SetOutputStream(OEPlatform::oeerr);

  cerr << progname << ": n_mol = " << n_mol <<endl;
  cerr << progname << ": n_name = " << n_name <<endl;
  cerr << progname << ": n_name2struct = " << n_name2struct <<endl;
  cerr << progname << ": n_exactmatch = " << n_exactmatch <<endl;
  cerr << progname << ": n_nocasematch = " << n_nocasematch <<endl;
  cerr << progname << ": n_fuzzymatch = " << n_fuzzymatch <<endl;

  oms.close();
  ims.close();
  return 0;
}
