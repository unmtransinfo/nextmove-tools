#!/usr/bin/python

import CaffeineFix

def CheckWord(str):
  if CaffeineFix.is_correct(str):
    print str + " is correct"
  else:
    print str + " is wrong"
    if CaffeineFix.is_prefix(str):
      print str + " is a prefix"
    else:
      print str + " is not a prefix"


CheckWord("benzene");
CheckWord("benzne");
CheckWord("benz");

