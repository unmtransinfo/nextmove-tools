#!/usr/bin/env python
#############################################################################
### nextmove_utils.py - CaffeineFix and LeadMine
#############################################################################
import sys,os,re
import subprocess,argparse


NM_HOME = "/home/app/nextmove"
NM_COMPILE_JAR=NM_HOME+'/leadmine-3.13/bin/compilecfx.jar'

NM_DICT_DIR="/home/jjyang/lobo_nextmove/data"
sys.path.append(NM_DICT_DIR)


#############################################################################
#From CaffeineFix.py
def is_correct(s):
  l = len(s)
  if l == 0:
    return False
  ch = s[0]
  state = 0
  i = 1
  while True:
    if CaffeineFixDictionary.ch(state) == ch:
      if i == l:
        return CaffeineFixDictionary.valid(state)
      state = CaffeineFixDictionary.down(state)
      ch = s[i]
      i = i+1
    else:
      state = CaffeineFixDictionary.across(state)
      if state == 0:
        return False

def is_prefix(s):
  l = len(s)
  if l == 0:
    return True
  ch = s[0]
  state = 0
  i = 1
  while True:
    if CaffeineFixDictionary.ch(state) == ch:
      if i == l:
        return True
      state = CaffeineFixDictionary.down(state)
      ch = s[i]
      i = i+1
    else:
      state = CaffeineFixDictionary.across(state)
      if state == 0:
        return False

#############################################################################
def CheckWord(qry, verbose):
  if is_correct(qry):
    print '"%s" is correct'%qry
  else:
    print '"%s" is wrong'%qry
  if is_prefix(qry):
    print '"%s" is a prefix'%qry
  else:
    print '"%s" is not a prefix'%qry

#############################################################################
def CompileDict(userdict, ofile, verbose):
  cmd_args = [JAVA, '-jar', NM_COMPILE_JAR, userdict, ofile]
  if verbose:
    print >>sys.stderr, 'COMMAND: %s'%(' '.join(cmd_args))
  ok = subprocess.call(cmd_args)
  if verbose:
    print >>sys.stderr, 'RESULT: rval = %s'%str(ok)

#############################################################################
def DecompileDict(idict, lang, ofile, verbose):
  lang_arg = '-j' if lang == 'java' else '-p'
  if verbose:
    print >>sys.stderr, 'COMMAND: %s %s %s %s'%(NM_DECOMPILE, lang_arg, idict, ofile)
  ok = subprocess.call([NM_DECOMPILE, lang_arg, idict, ofile])
  if verbose:
    print >>sys.stderr, 'RESULT: rval = %s'%str(ok)

#############################################################################
if __name__=='__main__':

  parser = argparse.ArgumentParser(
        description='NextMove utility',
        epilog='')
  ops = ['info', 'compileDict', 'decompileDict2Python',
	'checkWord', 'suggestWord' ]
  parser.add_argument("op",choices=ops,help='operation')
  parser.add_argument("--q",help="query")
  parser.add_argument("--userdict",dest="userdict",help="input user dictionary (TXT)")
  parser.add_argument("--idict",dest="idictfile",help="input compiled dictionary (CFX)")
  parser.add_argument("--o",dest="ofile",help="output file")
  parser.add_argument("--nmax",type=int)
  parser.add_argument("-v","--verbose",action="count")

  args = parser.parse_args()


  if args.op == 'info':
    parser.error('DEBUG: Not yet implemented.')

  elif args.op == 'compileDict':
    if not args.userdict or not args.ofile:
      parser.error('compileDict requires --userdict and --o.')
      parser.exit()
    CompileDict(args.userdict, args.ofile, args.verbose)

  elif args.op == 'decompileDict2Python':
    if not args.idictfile:
      parser.error('decompileDict2Python requires --idict.')
      parser.exit()
    DecompileDict(args.idictfile, 'python', args.ofile, args.verbose)

  elif args.op == 'checkWord':
    if not args.q:
      parser.error('checkWord requires --q.')
      parser.exit()

    import meddra_llt_dict as CaffeineFixDictionary

    CheckWord(args.q, args.verbose)

  else:
    parser.error('No operation specified.')
    parser.print_help()
