/* MedTermsCorrect.cpp

	Based on extract.cpp in the NextMove Extract release, Version 1.2, October 2010.
	Modified to overwrite the files with corrected terms. 
	Modified for use with medical terms (e.g. MedDRA) rather than molecule names.
	
	Jeremy J Yang
	13 Jul 2011

	ProcessDirFilename or ProcessDirectory calls ProcessFilename
	ProcessFilename calls ProcessFile
	ProcessFile calls ProcessContent
	ProcessContent calls: ProcessZip, ProcessGZip, ProcessTar, or ProcessText
	ProcessText calls ProcessBuffer (in engine.cpp)
	ProcessBuffer calls NormalizeBuffer, ProcessEntity (in engine.cpp)
	ProcessEntity corrects the entity if possible. (in engine.cpp)

 * 
 * Original code by Roger Sayle, NextMove Software
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <dirent.h>

#include <zlib.h>


#include "engine.h"
#include "output.h"


static unsigned long bytes_read;
static unsigned long bytes_mined;
static unsigned long files_mined;
static unsigned long files_read;
static unsigned long n_corrections;
static int lang;


/* Forward prototype */
static void ProcessContent(const char*, const unsigned char*, unsigned int);


static unsigned int ReadShort(const unsigned char *ptr)
{
  return (ptr[1]<<8) | ptr[0];
}


static unsigned int ReadLong(const unsigned char *ptr)
{
  return (ptr[3]<<24) | (ptr[2]<<16) | (ptr[1]<<8) | ptr[0];
}


/* Strip directories and last file extension.  */
static void GetPNFromFilename(char *pn, const char *fname)
{
  const char *src;
  char *dst = pn;
  char *ptr;

  for (src=fname; *src; src++)
  {
    if (*src != '/')
      *dst++ = *src;
    else dst = pn;
  }
  *dst = '\0';

  for (ptr=pn; *ptr; ptr++)
    if (*ptr=='.')
    {
      *ptr = '\0';
      break;
    }
}


/* *.nam, *.nam.gz,
   *.tar, *.tar.gz, *.tgz,
   *.txt, *.txt.gz,
   *.xml, *.xml.gz,
   *.zip */
static int MatchFilename(const char *fname)
{
  const char *ptr;

  for (ptr=fname; *ptr; ptr++)
    if (ptr[0]=='.')
    {
      switch (ptr[1])
      {
      case 'n':
        if (ptr[2]=='a' && ptr[3]=='m')
        {
          if (!ptr[4])
            return 1;
          if (ptr[4]=='.' && ptr[5]=='g' && ptr[6]=='z' && !ptr[7])
            return 1;
        }
        break;

      case 't':
        if (ptr[2]=='x' && ptr[3]=='t')
        {
          if (!ptr[4])
            return 1;
          if (ptr[4]=='.' && ptr[5]=='g' && ptr[6]=='z' && !ptr[7])
            return 1;
        }
        else if (ptr[2]=='a' && ptr[3]=='r')
        {
          if (!ptr[4])
            return 1;
          if (ptr[4]=='.' && ptr[5]=='g' && ptr[6]=='z' && !ptr[7])
            return 1;
        }
        else if (ptr[2]=='g' && ptr[3]=='z' && !ptr[4])
          return 1;
        break;

      case 'x':
        if (ptr[2]=='m' && ptr[3]=='l')
        {
          if (!ptr[4])
            return 1;
          if (ptr[4]=='.' && ptr[5]=='g' && ptr[6]=='z' && !ptr[7])
            return 1;
        }
        break;

      case 'z':
        if (ptr[2]=='i' && ptr[3]=='p' && !ptr[4])
          return 1;
        break;

      case 'S':
        if (ptr[2]=='G' && ptr[3]=='M' && !ptr[4])
          return 1;
        break;

      case 'T':
        if (ptr[2]=='X' && ptr[3]=='T' && !ptr[4])
          return 1;
        break;

      case 'X':
        if (ptr[2]=='M' && ptr[3]=='L' && !ptr[4])
          return 1;
        break;

      case 'Z':
        if (ptr[2]=='I' && ptr[3]=='P' && !ptr[4])
          return 1;
        break;
      }
    }
  return 0;
}


static void ProcessText(const char *fname,
			const unsigned char *buffer,
                        unsigned int len)
{
  char pn[1024];
  GetPNFromFilename(pn,fname);
  //fprintf(stderr,"DEBUG: pn=\"%s\"\n",pn);

  ProcessBuffer((const char*)buffer,len,lang);  //engine.cpp

  bytes_mined += len;
  files_mined++;
}


static void InflateContent(const char *fname,
                           const unsigned char *ptr,
                           unsigned int clen,
                           unsigned int ulen)
{
  unsigned char *buffer;
  z_stream strm;
  int err;

  buffer = (unsigned char*)malloc(ulen+1);
  if (!buffer)
  {
    fprintf(stderr,"Warning: Unable to allocate %u bytes!\n",ulen);
    return;
  }

  memset(&strm,0,sizeof(strm));
  strm.next_in = (unsigned char*)ptr;
  strm.avail_in = clen;
  strm.total_in = 0;

  strm.next_out = buffer;
  strm.avail_out = ulen;
  strm.total_out = 0;

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  err = inflateInit2(&strm,-MAX_WBITS);
  if (err == Z_OK)
  {
    err = inflate(&strm,Z_FINISH);
    if (strm.avail_out == 0)
    {
      buffer[ulen] = '\0';
      ProcessContent(fname,buffer,ulen);
    }
    else fputs("Warning: zlib decompression failed\n",stderr);

    inflateEnd(&strm);
  }
  free(buffer);
}


static int ProcessGZip(const char *fname,
                       const unsigned char *buffer,
                       unsigned int len)
{
  const unsigned char *ptr;
  unsigned int clen,ulen;
  unsigned char flags;

  if (len < 18)
    return 0;

  ptr = buffer;
  if (ptr[0]!=0x1f || ptr[1]!=0x8b || ptr[2]!=0x08)
    return 0;

  flags = ptr[3];

  /* Assume the gzip isn't concatenated */
  ulen = ReadLong(ptr+(len-4));

  ptr = buffer+10;
  if (flags & 4)
  {
    unsigned int xlen = ReadShort(ptr);
    ptr += xlen+2;
  }

  if (flags & 8)
  {
    while(*ptr) ptr++;
    ptr++;
  }

  if (flags & 16)
  {
    while (*ptr) ptr++;
    ptr++;
  }

  if (flags & 2)
    ptr += 2;

  clen = len - ((ptr-buffer)+8);
  InflateContent(fname,ptr,clen,ulen);
  return 1;
}


static unsigned int FindZipDirectory(const unsigned char *buffer,
                                     unsigned int len)
{
  const unsigned char *ptr;
  unsigned int offset;
  unsigned int i;

  if (len < 23)
    return 0;

  offset = len - 22;
  ptr = buffer + offset;
  if (ptr[0]==0x50 && ptr[1]==0x4b && ptr[2]==0x05 &&
      ptr[3]==0x06 && ptr[20]==0x00 && ptr[21]==0x00)
    return offset;

  /* Consider increasing comment lengths! */
  for (i=1; i<65536; i++)
  {
    if (offset == 0)
      return 0;
    offset--;
    ptr = buffer + offset;
    if (ptr[0]==0x50 && ptr[1]==0x4b && ptr[2]==0x05 && ptr[3]==0x06)
    {
      /* Found signature, check comment length */
      if (ptr[20]==(i&0xff) && ptr[21]==(i>>8))
        return offset;
      return 0;
    }
  }
  return 0;
}


static void ProcessZipContent(const char *fname, const unsigned char *ptr)
{
  unsigned int clen,ulen;
  unsigned int method;
  unsigned int n,m;

  if (ptr[0]!=0x50 || ptr[1]!=0x4b || ptr[2]!=0x03 || ptr[3]!=0x04)
  {
    fprintf(stderr,"Warning: Zip file local header incorrect!\n");
    return;
  }

  method = ReadShort(ptr+8);
  clen = ReadLong(ptr+18);
  ulen = ReadLong(ptr+22);
  n = ReadShort(ptr+26);
  m = ReadShort(ptr+28);

  if (method == 8)
    InflateContent(fname,ptr+(n+m+30),clen,ulen);
  else if (method == 0)
  {
    if (clen == ulen)
      ProcessContent(fname,ptr+(n+m+30),ulen);
    else
      fprintf(stderr,"Warning: Zip store method length mismatch!\n");
  }
  else
    fprintf(stderr,"Warning: Zip Compression method %u unsupported!\n",method);
}


static int ProcessZip(const unsigned char *buffer,
                      unsigned int len)
{
  const unsigned char *ptr;
  unsigned int i,count;
  unsigned int offset;

  offset = FindZipDirectory(buffer,len);
  if (!offset) return 0;

  ptr = buffer + offset;
  count = ReadShort(ptr+8);
  offset = ReadLong(ptr+16);

  if (ReadShort(ptr+4) != 0 || ReadShort(ptr+6) != 0)
  {
    fprintf(stderr,"Warning: Multiple part zip files not supported!\n");
    return 1;
  }
  if (ReadShort(ptr+10) != count)
  {
    fprintf(stderr,"Warning: Central directory record count mismatch!\n");
    return 1;
  }

  for (i=0; i<count; i++)
  {
    unsigned long foffset;
    unsigned int n,m,k;

    ptr = buffer + offset;
    if (ptr[0]!=0x50 || ptr[1]!=0x4b || ptr[2]!=0x01 || ptr[3]!=0x02)
    {
      fprintf(stderr,"Warning: Central directory signature mismatch!\n");
      return 1;
    }

    n = ReadShort(ptr+28);
    m = ReadShort(ptr+30);
    k = ReadShort(ptr+32);
    foffset = ReadLong(ptr+42);

    if (ptr[n+45] != '/')
    {
      char *fname = (char*)malloc(n+1);
      memcpy(fname,ptr+46,n);
      fname[n] = '\0';
      if (MatchFilename((const char*)fname))
        ProcessZipContent(fname,buffer+foffset);
      free(fname);
    }

    offset += (n+m+k+46);
  }
  return 1;
}


static int IsZeroBlock(const unsigned char *ptr)
{
  unsigned int i;
  for (i=0; i<512; i++)
    if (ptr[i])
      return 0;
  return 1;
}


static int ProcessTar(const unsigned char *ptr,
                      unsigned int len)
{
  unsigned int fsize;
  int result = 0;
  int i;

  for (;;)
  {
    if (len < 512)
      return result;
    if (ptr[257]!='u' || ptr[258]!='s' || ptr[259]!='t' ||
        ptr[260]!='a' || ptr[261]!='r' || ptr[135]!='\0')
    {
      if (!result)
        return 0;
      if (!IsZeroBlock(ptr))
        fprintf(stderr,"Warning: Tar file appears to be corrupted!\n");
      return 1;
    }

    fsize = 0;
    for (i=0; i<11; i++)
    {
      char ch = ptr[i+124];
      if (ch<'0' || ch>'7')
        return 1;
      fsize = (fsize*8) + (ch-'0');
    }

    if (fsize)
    {
      if (fsize+512>len)
        return 1;
      if (ptr[156] == '0')
      {
        const char *fname = (const char*)ptr;
        if (MatchFilename(fname))
          ProcessContent(fname,ptr+512,fsize);
      }
      if (fsize & 511)
        fsize += 512 - (fsize & 511);
    }
    if (fsize+512 >= len)
      return 1;
    len -= (fsize+512);
    ptr += (fsize+512);
    result = 1;
  }
}


static void ProcessContent(const char *fname,
                           const unsigned char *ptr,
                           unsigned int len)
{
  if (ProcessZip(ptr,len))
    return;
  if (ProcessGZip(fname,ptr,len))
    return;
  if (ProcessTar(ptr,len))
    return;
  ProcessText(fname,ptr,len);
}


static void ProcessFile(const char *fname, FILE *fp)
{
  unsigned char *buffer;
  unsigned int alloc;
  unsigned int len;

  buffer = (unsigned char*)malloc(65536);
  if (!buffer)
  {
    fprintf(stderr,"Error: Unable to allocate 64K buffer!\n");
    exit(1);
  }

  alloc = 65536;
  len = 0;

  for(;;)
  {
    unsigned int chunk = fread(buffer+len,1,65536,fp);
    len += chunk;
    if (chunk != 65536)
      break;
    alloc += 65536;
    buffer = (unsigned char*)realloc(buffer,alloc);
    if (!buffer)
    {
      fprintf(stderr,"Error: Unable to reallocate %uK buffer!\n",alloc>>10);
      exit(1);
    }
  }

  if (len)
  {
    buffer[len] = '\0';
    ProcessContent(fname,buffer,len);
    bytes_read += len;
  }
  free(buffer);
  files_read++;
}


static void ProcessFilename(const char *fname)
{

  if (fname[0]!='-' || fname[1]!='\0')
  {
    FILE *fp = fopen(fname,"r");
    if (!fp)
    {
      fprintf(stderr,"Error: Unable to read file '%s'!\n",fname);
      return;
    }
    fprintf(stderr,"%s\n",fname);
    ProcessFile(fname,fp);
    fclose(fp);
  }
  else ProcessFile("stdin",stdin);
}


static void ProcessDirFilename(const char *dname, const char *bname)
{
  char fname[4096];
  const char *src;
  char *dst;

  dst = fname;
  for (src=dname; *src; src++)
    *dst++ = *src;
  if (dname[0] && dst[-1]!='/')
    *dst++ = '/';
  strcpy(dst,bname);
  ProcessFilename(fname);
}


static void ProcessDirectory(const char *dirname)
{
#ifdef DIRENT64
  struct dirent64 *dp;
#else
  struct dirent *dp;
#endif
  DIR *dir;

  dir = opendir(dirname);
  if (dir)
  {
    for (;;)
    {
#ifdef DIRENT64
      dp = readdir64(dir);
#else
      dp = readdir(dir);
#endif
      if (!dp)
        break;
      if (MatchFilename(dp->d_name))
        ProcessDirFilename(dirname,dp->d_name);
    }
    closedir(dir);
  }
  else /* not a directory, assume filename */
    ProcessFilename(dirname);
}


int main(int argc, char *argv[])
{
  bool done;
  int i;

  fputs("MedTermsCorrect - Medical terms extraction and correction\n",stderr);
  fputs("(Based on Extract v1.3  Chemical Named Entity Extraction, NextMove Software, December 2010)\n\n",stderr);

  if (argc < 2)
  {
    fprintf(stderr,"usage: MedTermsCorrect [options] <files>\n\n");
    exit(1);
  }

  done = false;
  lang = LANG_EN;

  files_read = 0;
  bytes_read = 0;
  files_mined = 0;
  bytes_mined = 0;
  n_corrections=0;

  for (i=1; i<argc; i++)
    ProcessDirectory(argv[i]);

  fprintf(stderr,"%lu bytes in %lu files read!\n",bytes_read,files_read);
  fprintf(stderr,"%lu bytes in %lu files mined!\n",bytes_mined,files_mined);
  fprintf(stderr,"corrections: %lu\n",n_corrections);
  return 0;
}

