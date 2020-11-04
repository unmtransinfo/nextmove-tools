/* html.cpp
 * Chemical Named Entity Extraction
 * NextMove Software
 * Version 1.1, September 2010
 * Version 1.2, October 2010
 */
#include <string.h>
#include <stdio.h>

#include <vector>


#include "output.h"


struct Entity {
  const char *beg;
  const char *end;
  int kind,dist;

  Entity(const char *b, const char *e, int k)
    : beg(b), end(e), kind(k), dist(0) {}
  Entity(const char *b, const char *e, int k, int d)
    : beg(b), end(e), kind(k), dist(d) {}
};


static std::vector<Entity> entities;
static FILE *curr_fp = NULL;
static const char *curr_buffer;
static const char *curr_pn;


void SetOutputFile(FILE *fp)
{
  curr_fp = fp;
}


void IdentifySource(const char *pn, int)
{
  if (!curr_fp)
    curr_fp = stdout;
  curr_buffer = (const char*)0;
  curr_pn = pn;

  entities.clear();
}


void UpdatePart(int)
{
}


void RecordBuffer(const char *buffer)
{
  curr_buffer = buffer;
}


void RecordEntity(const char *beg, const char *end, int kind)
{
  entities.push_back(Entity(beg,end,kind));
}


void RecordCorrection(const char *beg, const char *end, int kind,
                      int /*dist*/, const std::string&)
{
  entities.push_back(Entity(beg,end,kind,1));
}


static void OutputString(const char *ptr)
{
  for (;;)
  {
    switch (*ptr)
    {
    case 0:
      return;

    case '<':
      fprintf(curr_fp,"&lt;");
      ptr++;
      break;

    case '>':
      fprintf(curr_fp,"&gt;");
      ptr++;
      break;

    case '&':
      fprintf(curr_fp,"&amp;");
      ptr++;
      break;

    default:
      fprintf(curr_fp,"%c",*ptr);
      ptr++;
    }
  }
}


static void OutputBuffer(const char *ptr)
{
  unsigned int count = entities.size();
  unsigned int state = 0;
  unsigned int idx = 0;

  for (;;)
  {
    if (state && ptr >= entities[idx].end)
    {
      fprintf(curr_fp,"</font>");
      state = 0;
      idx++;
    }

    if (state == 0 && idx < count && ptr >= entities[idx].beg)
    {
      const char *col = "pink";

      switch (entities[idx].kind)
      {
      case KIND_MOL:      col = "violet";      break;  /* #ee82ee */
      case KIND_DICT:     col = "#9090ff";     break;
      case KIND_REGNUM:   col = "#90b0ff";     break;
      case KIND_CASNUM:   col = "#60c0ff";     break;
      case KIND_ELEMENT:  col = "cyan";        break;  /* #00ffff */
      case KIND_PREFIX:   col = "lime";        break;  /* #00ff00 */
      case KIND_ATOMIC:   col = "#80ff00";     break;
      case KIND_POLYMER:  col = "yellow";      break;  /* #ffff00 */
      case KIND_GENERIC:  col = "orange";      break;  /* #ffa500 */
      case KIND_NOISE:    col = "#ff4500";     break;  /* orangered */
      }
      if (entities[idx].dist)
        fprintf(curr_fp,"<font style=\"background-color:%s"
                        ";color:white\">",col);
      else
        fprintf(curr_fp,"<font style=\"background-color:%s\">",col);
      state = 1;
    }

    switch (*ptr)
    {
    case 0:
      if (state)
        fprintf(curr_fp,"</font>");
      return;

    case '<':
      fprintf(curr_fp,"&lt;");
      ptr++;
      break;

    case '>':
      fprintf(curr_fp,"&gt;");
      ptr++;
      break;

    case '&':
      fprintf(curr_fp,"&amp;");
      ptr++;
      break;

    case '\\':
      if (ptr[1]=='u' && ptr[2] && ptr[3] && ptr[4] && ptr[5])
      {
        fprintf(curr_fp,"&#x%c%c%c%c;",ptr[2],ptr[3],ptr[4],ptr[5]);
        ptr += 6;
      }
      else
      {
        fprintf(curr_fp,"\\");
        ptr++;
      }
      break;

    default:
      fprintf(curr_fp,"%c",*ptr);
      ptr++;
    }
  }
}


void FinalizeSource()
{
  fprintf(curr_fp,"<html>\n");
  if (curr_pn)
  {
    fprintf(curr_fp,"<title>");
    OutputString(curr_pn);
    fprintf(curr_fp,"</title>\n");
  }
  if (curr_buffer)
  {
    fprintf(curr_fp,"<body>");
    OutputBuffer(curr_buffer);
    fprintf(curr_fp,"\n</body>\n");
  }
  fprintf(curr_fp,"</html>\n");
}

