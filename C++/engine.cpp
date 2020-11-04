/* engine.cpp

	Modified from engine.cpp in CaffeineFix, Version 1.1, October 2010.
	Modified for use with medical terms (e.g. MedDRA) rather than molecule names.

	ProcessBuffer calls NormalizeBuffer, ProcessEntity
	ProcessEntity corrects the entity if possible.
	
	Jeremy J Yang
	13 Jul 2011
 *
 * Original code by Roger Sayle, NextMove Software
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "engine.h"
#include "normalize.h"
#include "output.h"


typedef struct {
    unsigned char ch;
    unsigned char state;
    unsigned int down;
    unsigned int across;
  } FSMType;

#include "atomic.h"
#include "casnum.h"
#include "dict_nms.h"
#include "element.h"
#include "generic.h"
#include "grammar.h"
#include "noise.h"
#include "polymer.h"
#include "prefix.h"
#include "regnum.h"
#include "white.h"

#undef USE_SPELL_CORRECTION
// #define USE_SPELL_CORRECTION
#define MAX_CORRECTION_DIST  1

static unsigned char CookChar(unsigned char ch)
{
#if 1
  if (ch>='A' && ch<='Z')
    return ch + 32;
  if (ch=='\n' || ch=='\t')
    return ' ';
#endif
  return ch;
}


/*===========================*/
/*  Exact Matching Routines  */
/*===========================*/

bool ExactMatchFSM(const unsigned char *beg,
                   const unsigned char *end,
                   const FSMType *fsm)
{
  unsigned int state = 0;
  const unsigned char *ptr = beg;
  unsigned int ch = CookChar(*ptr++);

  for (;;)
  {
    if (fsm[state].ch == ch)
    {
      if (ptr == end)
        return fsm[state].state != 0;
      state = fsm[state].down;
      ch = CookChar(*ptr++);
    }
    else state = fsm[state].across;
    if (state == 0)
      return false;
  }
}


bool ExactPrefixFSM(const unsigned char *beg,
                    const unsigned char *end,
                    const FSMType *fsm)
{
  unsigned int state = 0;
  const unsigned char *ptr = beg;
  unsigned int ch = CookChar(*ptr++);

  for (;;)
  {
    if (fsm[state].ch == ch)
    {
      if (ptr == end)
        return true;
      state = fsm[state].down;
      ch = CookChar(*ptr++);
    }
    else state = fsm[state].across;
    if (state == 0)
      return false;
  }
}


/*===============================*/
/*  Chemistry Matching Routines  */
/*===============================*/

struct Stack {
  unsigned int top;
  unsigned char stack[32];

  Stack() : top(0) {}

  bool empty() { return top == 0; }

  bool fail(unsigned char ch)
  {
    switch (ch)
    {
    case '(':
      if (top == sizeof(stack))
        return true;
      stack[top++] = ')';
      break;

    case '[':
      if (top == sizeof(stack))
        return true;
      stack[top++] = ']';
      break;

    case '{':
      if (top == sizeof(stack))
        return true;
      stack[top++] = '}';
      break;

    case ')':
      if (top == 0 || stack[top-1] != ')')
        return true;
      top--;
      break;

    case ']':
      if (top == 0 || stack[top-1] != ']')
        return true;
      top--;
      break;

    case '}':
      if (top == 0 || stack[top-1] != '}')
        return true;
      top--;
      break;
    }
    return false;
  }
};


bool ExactChemMatchFSM(const unsigned char *beg,
                       const unsigned char *end,
                       const FSMType *fsm)
{
  unsigned int state = 0;
  const unsigned char *ptr = beg;
  unsigned int ch = CookChar(*ptr++);
  Stack stack;

  for (;;)
  {
    if (fsm[state].ch == ch)
    {
      if (stack.fail(ch))
        return false;
      if (ptr == end)
        return (fsm[state].state!=0) && stack.empty();
      state = fsm[state].down;
      ch = CookChar(*ptr++);
    }
    else state = fsm[state].across;
    if (state == 0)
      return false;
  }
}


bool ExactChemPrefixFSM(const unsigned char *beg,
                        const unsigned char *end,
                        const FSMType *fsm)
{
  unsigned int state = 0;
  const unsigned char *ptr = beg;
  unsigned int ch = CookChar(*ptr++);
  Stack stack;

  for (;;)
  {
    if (fsm[state].ch == ch)
    {
      if (stack.fail(ch))
        return false;
      if (ptr == end)
        return true;
      state = fsm[state].down;
      ch = CookChar(*ptr++);
    }
    else state = fsm[state].across;
    if (state == 0)
      return false;
  }
}


/*================================*/
/*  Spelling Correction Routines  */
/*================================*/

#define CORRECTION DictCorrection
#define FSM fsm
#include "correct_ext.cpp"
#undef CORRECTION
#undef FSM

// #define CORRECTION ChemCorrection
// #define FSM iupac_fsm
// #define CHEMISTRY
// #include "correct_ext.cpp"
// #undef CORRECTION
// #undef CHEMISTRY
// #undef FSM

// #define CORRECTION FragCorrection
// #define FSM prefix_fsm
// #define CHEMISTRY
// #include "correct_ext.cpp"
// #undef CORRECTION
// #undef CHEMISTRY
// #undef FSM


/* Find next token termination character.  */
static const unsigned char *NextEnd(const unsigned char *ptr)
{
  for(;;)
  {
    switch (*ptr)
    {
    case 0:
    case '\t':  // 0x09
    case '\n':  // 0x0a
    case '\r':  // 0x0d
    case ' ':   // 0x20
    case '!':   // 0x21
    case '"':   // 0x22
    case '#':   // 0x23
    case '%':   // 0x25
    case '&':   // 0x26
    case '\'':  // 0x27
    case '(':   // 0x28
    case ')':   // 0x29
    case ',':   // 0x2c
    case '.':   // 0x2e
    case '/':   // 0x2f
    case ':':   // 0x3a
    case ';':   // 0x3b
    case '<':   // 0x3c
    case '=':   // 0x3d
    case '>':   // 0x3e
    case '?':   // 0x3f
    case '@':   // 0x40
    case '[':   // 0x5b
    case '\\':  // 0x5c
    case ']':   // 0x5d
    case '^':   // 0x5e
    case '`':   // 0x60
    case '{':   // 0x7b
    case '|':   // 0x7c
    case '}':   // 0x7d
      return ptr;

    default:
      ptr++;
    }
  }
}


/* Subset of termination characters that can appear in a token.  */
bool ExtendEnd(unsigned char ch)
{
  switch (ch)
  {
  case '\t':  // 0x09
  case '\n':  // 0x0a
  case '\r':  // 0x0d
  case ' ':   // 0x20
  case '"':   // 0x22
  case '$':   // 0x24
  case '&':   // 0x26
  case '\'':  // 0x27
  case '(':   // 0x28
  case ')':   // 0x29
  case '+':   // 0x2b
  case ',':   // 0x2c
  case '-':   // 0x2d
  case '.':   // 0x2e
  case '0':   // 0x30
  case '1':   // 0x31
  case '2':   // 0x32
  case '3':   // 0x33
  case '4':   // 0x34
  case '5':   // 0x35
  case '6':   // 0x36
  case '7':   // 0x37
  case '8':   // 0x38
  case '9':   // 0x39
  case ';':   // 0x3b
  case '<':   // 0x3c
  case '>':   // 0x3e
  case '[':   // 0x5b
  case ']':   // 0x5d
  case '^':   // 0x5e
  case '{':   // 0x7b
  case '}':   // 0x7d
  case '~':   // 0x7e
    return true;
  }
  return false;
}


typedef struct {
  const FSMType *fsm;
  bool chem;
  int kind;
} DictOrder;

static const unsigned char *ProcessEntity(const unsigned char *beg)
{
#define MAXDICT 10
   static const DictOrder order[MAXDICT] = {
     { noise_fsm,   false, KIND_NOISE   },  /* 0 */
     { element_fsm, false, KIND_ELEMENT },  /* 1 */
     { iupac_fsm,   true,  KIND_MOL     },  /* 2 */
     { polymer_fsm, false, KIND_POLYMER },  /* 3 */
     { regnum_fsm,  false, KIND_REGNUM  },  /* 4 */
     { casnum_fsm,  false, KIND_CASNUM  },  /* 5 */
     { fsm,    false, KIND_DICT    },  /* 6 */
     { atomic_fsm,  false, KIND_ATOMIC  },  /* 7 */
     { prefix_fsm,  true,  KIND_PREFIX  },  /* 8 */
     { generic_fsm, false, KIND_GENERIC }   /* 9 */
      };
//#define MAXDICT 1
//  static const DictOrder order[MAXDICT] = {
//    { fsm,    false, KIND_DICT    }
//      };

  const unsigned char *end = NextEnd(beg+1);
  const unsigned char *best_end = 0;
  bool flag[MAXDICT];
  int best_kind = 0;
  bool found, cont;
  int i;

  cont = false;
  found = false;
  for (i=0; i<MAXDICT; i++)
  {
    if (order[i].chem)
    {
      if (ExactChemPrefixFSM(beg,end,order[i].fsm))
      {
        flag[i] = true;
        cont = true;
        if (!found && ExactChemMatchFSM(beg,end,order[i].fsm))
        {
          best_kind = order[i].kind;
          best_end = end;
          found = true;
        }
      }
      else flag[i] = false;
    }
    else /* !order[i].chem */
    {
      if (ExactPrefixFSM(beg,end,order[i].fsm))
      {
        flag[i] = true;
        cont = true;
        if (!found && ExactMatchFSM(beg,end,order[i].fsm))
        {
          best_kind = order[i].kind;
          best_end = end;
          found = true;
        }
      }
      else flag[i] = false;
    }
  }

  while (cont && ExtendEnd(*end))
  {
    cont = false;
    found = false;
    end = NextEnd(end+1);
    for (i=0; i<MAXDICT; i++)
      if (flag[i])
      {
        if (order[i].chem)
        {
          if (ExactChemPrefixFSM(beg,end,order[i].fsm))
          {
            cont = true;
            if (!found && ExactChemMatchFSM(beg,end,order[i].fsm))
            {
              best_kind = order[i].kind;
              best_end = end;
              found = true;
            }
          }
          else flag[i] = false;
        }
        else /* !order[i].chem */
        {
          if (ExactPrefixFSM(beg,end,order[i].fsm))
          {
            cont = true;
            if (!found && ExactMatchFSM(beg,end,order[i].fsm))
            {
              best_kind = order[i].kind;
              best_end = end;
              found = true;
            }
          }
          else flag[i] = false;
        }
      }
  }

#ifdef USE_SPELL_CORRECTION
  int best_score;

  if (best_kind)
  {
    if (best_kind!=KIND_PREFIX && best_kind!=KIND_ATOMIC)
    {
      RecordEntity((const char*)beg,(const char*)best_end,best_kind);
      return best_end;
    }
    best_score = (int)(best_end - beg);
    if (best_score < 8)
      best_score = 8;
  }
  else best_score = 8;

  const unsigned char *fix_end = 0;
  bool dict_cont = true;
  bool chem_cont = true;
  bool frag_cont = true;
  int best_dist = -1;
  int fix_kind = 0;
  std::string fix;

  end = NextEnd(beg+1);
  for(;;)
  {
    bool ambig = false;

    if (dict_cont)
    {
      DictCorrection dc(beg,end,MAX_CORRECTION_DIST);
      int dist = dc.Closest(MAX_CORRECTION_DIST);  // RAS
      int len = (int)(end-beg);
      if (dist != -1)
      {
        if (len-dist > best_score)
        {
          best_score = len - dist;
          fix_kind = KIND_DICT;
          best_dist = dist;
          fix = dc.result;
          fix_end = end;
        }
      }
      dict_cont = dc.prefix;
    }
    if (chem_cont)
    {
      ChemCorrection cc(beg,end,MAX_CORRECTION_DIST);
      int dist = cc.Closest(MAX_CORRECTION_DIST);
      int len = (int)(end-beg);
#if 0
      printf("C \"");
      for (i=0; i<len; i++)
        printf("%c",beg[i]);
      printf("\" -> dist=%d score=%d fix=%s prefix=%c\n",  // RAS
             dist,len-dist,cc.result.c_str(),cc.prefix?'Y':'N');
#endif
      if (dist != -1)
      {
        if (len-dist > best_score)
        {
          best_score = (int)len - dist;
          fix_kind = KIND_MOL;
          best_dist = dist;
          fix = cc.result;
          fix_end = end;
        }
      }
      else if (cc.count)
        ambig = true;
      chem_cont = cc.prefix;
      // Avoid "methylene, ethylene"
      if (dist!=-1 && *end == ',' && (end[1]==' '||end[1]=='\n'))
        chem_cont = false;
    }
    if (frag_cont)
    {
      FragCorrection fc(beg,end,MAX_CORRECTION_DIST);
      int dist = fc.Closest(MAX_CORRECTION_DIST);
      int len = (int)(end-beg);
#if 0
      printf("F \"");
      for (i=0; i<len; i++)
        printf("%c",beg[i]);
      printf("\" -> dist=%d score=%d fix=%s prefix=%c\n",  // RAS
             dist,len-dist,fc.result.c_str(),fc.prefix?'Y':'N');
#endif
      if (dist != -1 && !ambig)
      {
        if (len-dist > best_score)
        {
          best_score = (int)len - dist;
          fix_kind = KIND_PREFIX;
          best_dist = dist;
          fix = fc.result;
          fix_end = end;
        }
      }
      frag_cont = fc.prefix;
      // Avoid "fluoro, chloro" -> "fluoro-chloro"
      if (*end == ',' && (end[1]==' '||end[1]=='\n'))
        frag_cont = false;
    }
    if (!chem_cont && !dict_cont && !frag_cont)
      break;
    if (*end == '\0')
      break;
    end = NextEnd(end+1);
  }

  if (fix_kind && fix.size() >= 10 && !ExactMatchFSM(beg,fix_end,white_fsm))
  {
    RecordCorrection((const char*)beg,(const char*)fix_end,
                     fix_kind,best_dist,fix);
    return fix_end;
  }
#endif  // USE_SPELL_CORRECTION

  if (best_kind)
  {
    RecordEntity((const char*)beg,(const char*)best_end,best_kind);
    return best_end;
  }

  if (*beg == '&')
    beg++;

  while (beg < end)
  {
    switch (*beg)
    {
    case '\0':  // 0x00
    case '&':   // 0x26
    case '<':   // 0x3c
      return beg;

    case '\t':  // 0x09
    case '\n':  // 0x0a
    case '\r':  // 0x0d
    case ' ':   // 0x20
    case '"':   // 0x22
    case '\'':  // 0x27
    case '(':   // 0x28
    case ')':   // 0x29
    case '/':   // 0x2f
    case '-':   // 0x2d
    case ':':   // 0x3a
    case ';':   // 0x3b
    case '>':   // 0x3e
    case '[':   // 0x5b
    case ']':   // 0x5d
    case '{':   // 0x7b
    case '}':   // 0x7d
      return beg+1;
    }
    beg++;
  }
  return end;
}


static int IsSpace(unsigned char ch)
{
  return ch==' ' || ch=='\n';
}


static const unsigned char *SkipEntity(const unsigned char *ptr)
{
  switch (ptr[0])
  {
  case 'a':
    if (ptr[1]=='n' && IsSpace(ptr[2]))
      return ptr+3;
    if (ptr[1]=='s' && IsSpace(ptr[2]))
      return ptr+3;
    break;

  case 'i':
    if (ptr[1]=='n' && IsSpace(ptr[2]))
      return ptr+3;
    if (ptr[1]=='s' && IsSpace(ptr[2]))
      return ptr+3;
    break;

  case 'm':
    if ((ptr[1]=='l' || ptr[1]=='L') && IsSpace(ptr[2]))
      return ptr+3;
    break;

  case 'o':
    if (ptr[1]=='f' && IsSpace(ptr[2]))
      return ptr+3;
    if (ptr[1]=='n' && IsSpace(ptr[2]))
      return ptr+3;
    if (ptr[1]=='r' && IsSpace(ptr[2]))
      return ptr+3;
    break;

  case 't':
    if (ptr[1]=='o' && IsSpace(ptr[2]))
      return ptr+3;
    break;
  }

  if (IsSpace(ptr[1]))
    return ptr+2;

  if ((ptr[0]>='1' && ptr[0]<='9'))
  {
    if ((ptr[1]=='%' || ptr[1]==':') &&
        ptr[2]==' ')
      return ptr+3;
    if ((ptr[1]>='0' && ptr[1]<='9') &&
        (ptr[2]=='%' || ptr[2]==':') &&
        ptr[3]==' ')
      return ptr+4;
  }

  return (const unsigned char*)0;
}


void ProcessBuffer(const char *buffer, unsigned int len, int lang)
{
  std::string temp = NormalizeBuffer((const unsigned char*)buffer,len,lang); //normalize.cpp
  const char *start = temp.c_str();
  const unsigned char *ptr = (const unsigned char*)start;
  const unsigned char *skip;

  //RecordBuffer(start); //html.cpp

  for (;;)
  {
    // fprintf(stderr,"DEBUG: %c\n",*ptr);
    switch (*ptr)
    {
    case 0:
      return;

    /* Chemical entities don't start with these characters. */
    case '\t':  // 0x09
    case '\n':  // 0x0a
    case '\r':  // 0x0d 
    case ' ':   // 0x20
    case '!':   // 0x21
    case '"':   // 0x22
    case '\'':  // 0x27
    case ')':   // 0x29
    case '*':   // 0x2a
    case ',':   // 0x2c
    case '-':   // 0x2d
    case '/':   // 0x2f
    case ':':   // 0x3a
    case ';':   // 0x3b
    case '=':   // 0x3d
    case '>':   // 0x3e
    case '?':   // 0x3f
    case '@':   // 0x40
    case ']':   // 0x5d
    case '`':   // 0x60
    case '|':   // 0x7c
    case '}':   // 0x7d
    case '~':   // 0x7e
      ptr++;
      break;

    case '<':
      ptr++;
      break;

    default:
      skip = SkipEntity(ptr);
      if (!skip)
      {
        ptr = ProcessEntity(ptr);
      }
      else ptr = skip;
    }
  }
}

