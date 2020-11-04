/* normalize.cpp
 * Chemical Named Entity Extraction
 * NextMove Software
 * Version 1.0, September 2010
 */
#include <string.h>
#include <stdio.h>

#include <string>

#ifdef LEXICHEM
#include "oeiupac.h"
#endif

#include "normalize.h"
#include "engine.h"


static void AppendChar(std::string &str, int val)
{
  switch (val)
  {
  case 9:
    str += ' ';
    break;

  case 13:
    break;

  default:
    if (val >= 0x80)
    {
      char buffer[32];
      sprintf(buffer,"\\u%04x",val);
      str += buffer;
    }
    else str += val;
  }
}


static std::string NormalizeBuffer8(const unsigned char *ptr, unsigned int len)
{
  const unsigned char *end = ptr + len;
  std::string result;

  for (;;)
  {
    if (ptr >= end)
      return result;

    switch (*ptr)
    {
    case 0:
      return result;

    case 9:
      result += ' ';
      ptr++;
      break;

    case 13:
      if (ptr[1] == 10)
        ptr += 2;
      else ptr++;
      result += '\n';
      break;

    case '&':
      // printf("%12.12s\n",ptr);
      if (ptr[1]=='a')
      {
        if (ptr[2]=='m')
        {
          if (ptr[3]=='p' && ptr[4]==';')
          {
            // printf("%12.12s\n",ptr);
            if (ptr[5]=='#' && (ptr[6]=='x' || ptr[6]=='X'))
            {
              if (ptr[7]=='2')
              {
                if (ptr[8]=='0')
                {
                  if (ptr[9]=='1')
                  {
                    if (ptr[10]=='4' && ptr[11]==';')
                    {
                      // Unicode U+2014 is 'EM DASH'
                      result += '-';
                      ptr += 12;
                      break;
                    }
                    else if ((ptr[10]=='c' || ptr[10]=='C') && ptr[11]==';')
                    {
                      // Unicode U+201C is 'LEFT DOUBLE QUOTATION MARK'
                      result += '"';
                      ptr += 12;
                      break;
                    }
                    else if ((ptr[10]=='d' || ptr[10]=='D') && ptr[11]==';')
                    {
                      // Unicode U+201C is 'RIGHT DOUBLE QUOTATION MARK'
                      result += '"';
                      ptr += 12;
                      break;
                    }
                  }
                  else if (ptr[9]=='3')
                  {
                    if (ptr[10]=='2' && ptr[11]==';')
                    {
                      // Unicode U+2032 is 'PRIME'
                      result += '\'';
                      ptr += 12;
                      break;
                    }
                    else if (ptr[10]=='3' && ptr[11]==';')
                    {
                      // Unicode U+2033 is 'DOUBLE PRIME'
                      result += '\'';
                      ptr += 12;
                      break;
                    }
                  }
                }
                else if (ptr[8]=='2')
                {
                  if (ptr[9]=='1')
                  {
                    if (ptr[10]=='2' && ptr[11]==';')
                    {
                      // Unicode U+2212 is 'MINUS SIGN'
                      result += '-';
                      ptr += 12;
                      break;
                    }
                  }
                }
              }
            }
            else if (ptr[5]=='a' && ptr[6]=='m' && ptr[7]=='p' && ptr[8]==';')
            {
              result += '&';
              ptr += 9;
              break;
            }
            else if (ptr[5]=='l' && ptr[6]=='t' && ptr[7]==';')
            {
              result += '<';
              ptr += 8;
              break;
            }
            else if (ptr[5]=='g' && ptr[6]=='t' && ptr[7]==';')
            {
              result += '>';
              ptr += 8;
              break;
            }
            // printf("%12.12s\n",ptr);
            result += '&';
            ptr += 5;
            break;
          }
        }
        else if (ptr[2]=='g')
        {
          if (ptr[3]=='r' && ptr[4]=='a' && ptr[5]=='v' &&
              ptr[6]=='e' && ptr[7]==';')
          {
            result += 'a';
            ptr += 8;
            break;
          }
        }
      }
      else if (ptr[1]=='e')
      {
        if (ptr[2]=='a')
        {
          if (ptr[3]=='c' && ptr[4]=='u' && ptr[5]=='t' &&
              ptr[6]=='e' && ptr[7]==';')
          {
            result += 'e';
            ptr += 8;
            break;
          }
        }
        else if (ptr[2]=='g')
        {
          if (ptr[3]=='r' && ptr[4]=='a' && ptr[5]=='v' &&
              ptr[6]=='e' && ptr[7]==';')
          {
            result += 'e';
            ptr += 8;
            break;
          }
        }
      }
      else if (ptr[1]=='g')
      {
        if (ptr[2]=='t' && ptr[3]==';')
        {
          result += '>';
          ptr += 4;
          break;
        }
      }
      else if (ptr[1]=='l')
      {
        if (ptr[2]=='t' && ptr[3]==';')
        {
          result += '<';
          ptr += 4;
          break;
        }
      }
      else if (ptr[1]=='#')
      {
        if (ptr[2]=='0' && ptr[3]=='3')
        {
          if (ptr[4]=='4' && ptr[5]==';')
          {
            result += '"';
            ptr += 6;
            break;
          }
          else if (ptr[4]=='9' && ptr[5]==';')
          {
            result += '\'';
            ptr += 6;
            break;
          }
        }
      }
      else if (ptr[1]=='E')
      {
        if (ptr[2]=='a')
        {
          if (ptr[3]=='c' && ptr[4]=='u' && ptr[5]=='t' &&
              ptr[6]=='e' && ptr[7]==';')
          {
            result += 'E';
            ptr += 8;
            break;
          }
        }
        else if (ptr[2]=='g')
        {
          if (ptr[3]=='r' && ptr[4]=='a' && ptr[5]=='v' &&
              ptr[6]=='e' && ptr[7]==';')
          {
            result += 'E';
            ptr += 8;
            break;
          }
        }
      }
      // printf("%12.12s\n",ptr);
      result += '&';
      ptr++;
      break;

    case '<':
      if (ptr[1]=='b' && ptr[2]=='r' && ptr[3]=='/' && ptr[4]=='>')
      {
        result += ' ';
        ptr += 5;
        break;
      }
      result += '<';
      ptr++;
      break;

    case 0xad:  // Adobe hyphen
      result += '-';
      ptr++;
      break;

    default:
      if (*ptr >= 0x80)
      {
        int val;

        if (((ptr[0] & 0xf0) == 0xe0) &&
            ((ptr[1] & 0xc0) == 0x80) &&
            ((ptr[2] & 0xc0) == 0x80))
        {
          val = ((ptr[0] & 0x0f) << 12) |
                ((ptr[1] & 0x3f) << 6)  |
                (ptr[2] & 0x3f);
          ptr += 3;
        }
        else if (((ptr[0] & 0xe0) == 0xc0) &&
                 ((ptr[1] & 0xc0) == 0x80))
        {
          val = ((ptr[0] & 0x1f) << 6) | (ptr[1] & 0x3f);
          ptr += 2;
        }
        else val = *ptr++;
        AppendChar(result,val);
      }
      else result += *ptr++;
    }
  }
}


static std::string NormalizeBuffer16LE(const unsigned char *ptr,
                                       unsigned int len)
{
  std::string result;
  unsigned int i,val;

  for (i=0; i<len; i+=2)
  {
    val = (ptr[i+1] << 8) | ptr[i];
    AppendChar(result,val);
  }
  return result;
}


std::string NormalizeBuffer16BE(const unsigned char *ptr, unsigned int len)
{
  std::string result;
  unsigned int i,val;

  for (i=0; i<len; i+=2)
  {
    val = (ptr[i] << 8) | ptr[i+1];
    AppendChar(result,val);
  }
  return result;
}


std::string NormalizeBuffer(const unsigned char *buffer,
                            unsigned int len, int lang)
{
#if 0
  return std::string((const char*)buffer,len);
#endif
  std::string temp;

  if (buffer[0]==0xff && buffer[1]==0xfe)
    temp = NormalizeBuffer16LE(buffer+2,len-2);
  else if (buffer[0]==0xfe && buffer[1]==0xff)
    temp = NormalizeBuffer16BE(buffer+2,len-2);
  else temp = NormalizeBuffer8(buffer,len);
  // printf("%s\n\n",temp.c_str());
#ifdef LEXICHEM
  switch (lang)
  {
  case LANG_DE:  temp = OEIUPAC::OEFromGerman(temp.c_str());	break;
  case LANG_DA:  temp = OEIUPAC::OEFromDanish(temp.c_str());	break;
  case LANG_ES:  temp = OEIUPAC::OEFromSpanish(temp.c_str());	break;
  case LANG_FR:  temp = OEIUPAC::OEFromFrench(temp.c_str());	break;
  case LANG_HU:  temp = OEIUPAC::OEFromHungarian(temp.c_str());	break;
  case LANG_IT:  temp = OEIUPAC::OEFromItalian(temp.c_str());	break;
  case LANG_NL:  temp = OEIUPAC::OEFromDutch(temp.c_str());	break;
  case LANG_JA:  temp = OEIUPAC::OEFromJapanese(temp.c_str());	break;
  case LANG_PL:  temp = OEIUPAC::OEFromPolish(temp.c_str());	break;
  case LANG_RU:  temp = OEIUPAC::OEFromRussian(temp.c_str());	break;
  case LANG_SV:  temp = OEIUPAC::OEFromSwedish(temp.c_str());	break;
  case LANG_ZH:  temp = OEIUPAC::OEFromChinese(temp.c_str());	break;
  }
  // if (lang != LANG_EN) printf("%s\n\n",temp.c_str());
#endif
  return temp;
}

