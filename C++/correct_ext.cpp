/* correct.cpp
 * Chemical Named Entity Extraction
 * NextMove Software
 * Version 1.0, September 2010
 * Version 1.1, October 2010
 */

#ifdef MATCHES
#define SHORTCIRCUIT do {} while(0)
#else
#define SHORTCIRCUIT  if (count == 2) return
#endif

struct CORRECTION {
  const unsigned char *beg;
  const unsigned char *end;
  unsigned char *tmp;

#ifdef MATCHES
  std::set<std::string> matches;
#endif

#ifdef CHEMISTRY
  unsigned char stack[32];
  unsigned int top;
#endif

  std::string result;
  bool prefix;
  int count;

  const unsigned char *src;
  unsigned char *dst;
  int ttl;

  void Init(const unsigned char *b, const unsigned char *e, int max)
  {
    beg = b;
    end = e;

    unsigned int len = (unsigned int)(e-b)+(max+1);
    tmp = (unsigned char*)malloc(len);
  }

  CORRECTION()
  {
    tmp = (unsigned char*)0;
  }

  CORRECTION(const unsigned char *b, const unsigned char *e, int max)
  {
    Init(b,e,max);
  }

  ~CORRECTION()
  {
    if (tmp)
      free(tmp);
  }

  static unsigned char CookChar(unsigned char ch)
  {
    if (ch>='A' && ch<='Z')
      return ch + 32;
    if (ch == '\n')
      return ' ';
    return ch;
  }

  void Done()
  {
    *dst = '\0';
    // fprintf(stderr,"count=%d \"%s\"\n",count,tmp); // RAS
    if (end[-1]==' ' || end[-1]=='\t' || end[-1]=='\n')
      return;

    if (count == 0)
    {
      result = std::string((const char*)tmp);
      count = 1;
    }
    else if (count == 1)
    {
      std::string str((const char*)tmp);
      if (result != str)
      {
#ifdef MATCHES
        matches.insert(result);
        matches.insert(str);
#endif
        count = 2;
      }
    }
#ifdef MATCHES
    else matches.insert(std::string(tmp));
#endif
  }

#ifdef CHEMISTRY
  bool IsOk(unsigned char ch)
  {
    switch (ch)
    {
    case '(':
      if (top == sizeof(stack))
        return false;
      stack[top++] = ')';
      break;

    case '[':
      if (top == sizeof(stack))
        return false;
      stack[top++] = ']';
      break;

    case '{':
      if (top == sizeof(stack))
        return false;
      stack[top++] = '}';
      break;

    case ')':
      if (top == 0 || stack[top-1] != ')')
        return false;
      top--;
      break;

    case ']':
      if (top == 0 || stack[top-1] != ']')
        return false;
      top--;
      break;

    case '}':
      if (top == 0 || stack[top-1] != '}')
        return false;
      top--;
      break;
    }
    return true;
  }
#else
  bool IsOk(unsigned char)
  {
    return true;
  }
#endif

#ifdef CHEMISTRY
  void Undo(unsigned char ch)
  {
    switch (ch)
    {
    case '(':
    case '[':
    case '{':
      top--;
      break;

    case ')':
    case ']':
    case '}':
      stack[top++] = ch;
      break;
    }
  }
#endif

  bool CheckDelete(unsigned char ch)
  {
    /* Don't delete first repeated character */
    if (src+1 != end && ch == src[1])
      return false;
    if (src == beg)
    {
      switch (ch)
      {
      case '.':  // 0x2e
      case '(':  // 0028
      case '[':  // 0x5b
      case '{':  // 0x7b
        return false;
      }

      if (src+1 != end)
        switch (src[1])
        {
        case '\t':
        case '\n':
        case '\r':
        case ' ':
          return false;
        }
    }
    return true;
  }

  void Recurse(unsigned int state, int accept)
  {
    unsigned char ch,nch;
    bool found = false;
    unsigned int i;

    if (src >= end)
    {
      prefix = true;
#ifdef CHEMISTRY
      if (accept && top == 0)
#else
      if (accept)
#endif
        return Done();

      if (ttl > 0 && state != 0)
      {
        /* Extend truncation */
        i = state;
        do {
          nch = FSM[i].ch;
          if (IsOk(nch))
          {
            *dst++ = nch;
            ttl--;
            Recurse(FSM[i].down,FSM[i].state);
            dst--;
            ttl++;
#ifdef CHEMISTRY
            Undo(nch);
#endif
            SHORTCIRCUIT;
          }
          i = FSM[i].across;
        } while (i);
      }
      return;
    }

    if (state == 0 && accept)
    {
      if (ttl && src+1 == end)
        Done();
      return;
    }

    ch = CookChar(*src);
    i = state;
    do {
      if (FSM[i].ch == ch && IsOk(ch))
      {
        found = true;
        *dst++ = ch;
        src++;
        Recurse(FSM[i].down,FSM[i].state);
        src--;
        dst--;
#ifdef CHEMISTRY
        Undo(ch);
#endif
        SHORTCIRCUIT;
        break;
      }
      i = FSM[i].across;
    } while(i);

#if 1
    switch (*src)
    {
#if 1
    case ' ':
    case '\t':
    case '\n':
    case '-':
      if (!found && src[1]!=' ' && src[1]!='\n')
      {
        src++;
        Recurse(state,accept);
        src--;
      }
      break;
#else
    case ' ':
    case '\t':
    case '-':
      if (!found || !count)
      {
        src++;
        Recurse(state,accept);
        src--;
      }
      break;

    case '\n':
      if (src+2 != end)
      {
        src++;
        Recurse(state,accept);
        src--;
      }
      break;
#endif

    case 'l':
      i = state;
      do {
        if (FSM[i].ch == '1')
        {
          *dst++ = '1';
          src++;
          Recurse(FSM[i].down,FSM[i].state);
          src--;
          dst--;
          break;
        }
        i = FSM[i].across;
      } while(i);
      break;

    case 'I':
      i = state;
      do {
        nch = FSM[i].ch;
        if (nch == '1' || nch == 'l')
        {
          *dst++ = nch;
          src++;
          Recurse(FSM[i].down,FSM[i].state);
          src--;
          dst--;
          break;
        }
        i = FSM[i].across;
      } while(i);
      break;

    // "rn" -> "m"
    case 'r':
      if (src[1]=='n')
      {
        i = state;
        do {
          if (FSM[i].ch == 'm')
          {
            *dst++ = 'm';
            src += 2;
            Recurse(FSM[i].down,FSM[i].state);
            src -= 2;
            dst--;
            break;
          }
          i = FSM[i].across;
        } while(i);
      }
      break;

    case '1':
      i = state;
      do {
        if (FSM[i].ch == 'l')
        {
          *dst++ = 'l';
          src++;
          Recurse(FSM[i].down,FSM[i].state);
          src--;
          dst--;
          break;
        }
        i = FSM[i].across;
      } while(i);
      break;

    // "&phi;" -> "rp"
    case '&':
      if (src[1]=='p' && src[2]=='h' && src[3]=='i')
      {
        if (src+4 != end)
        {
          if (src[4]==';')
          {
            i = state;
            do {
              if (FSM[i].ch == 'r')
              {
                for (i=FSM[i].down; i; i=FSM[i].across)
                  if (FSM[i].ch == 'p')
                  {
                    *dst++ = 'r';
                    *dst++ = 'p';
                    src += 5;
                    Recurse(FSM[i].down,FSM[i].state);
                    src -= 5;
                    dst -= 2;
                    break;
                  }
                break;
              }
              i = FSM[i].across;
            } while(i);
          }
        }
        else prefix = true;
      }
      break;
    }

    SHORTCIRCUIT;
#endif

    if (ttl)
    {
#if 1
      /* Substitution */
      i = state;
      do {
        nch = FSM[i].ch;
        if (nch != ch && IsOk(nch))
        {
          *dst++ = nch;
          src++;
          ttl--;
          Recurse(FSM[i].down,FSM[i].state);
          ttl++;
          src--;
          dst--;
#ifdef CHEMISTRY
          Undo(nch);
#endif
          SHORTCIRCUIT;
        }
        i = FSM[i].across;
      } while(i);
#endif

#if 1
      /* Delete Insertion */
      if (CheckDelete(ch))
      {
        src++;
        ttl--;
        Recurse(state,accept);
        ttl++;
        src--;
        SHORTCIRCUIT;
      }
#endif

#if 1
      /* Insert deletion */
      i = state;
      do {
        nch = FSM[i].ch;
        if (nch != ch && IsOk(nch))
        {
          *dst++ = nch;
          ttl--;
          Recurse(FSM[i].down,FSM[i].state);
          dst--;
          ttl++;
#ifdef CHEMISTRY
          Undo(nch);
#endif
          SHORTCIRCUIT;
        }
        i = FSM[i].across;
      } while(i);
#endif
    } /* ttl */
  } /* Recurse */

  void Try(int dist)
  {
    // fprintf(stderr,"Try(%d)\n",dist);  // RAS
    result = "";
    prefix = false;
    count = 0;

#ifdef MATCHES
    matches.clear();
#endif

#ifdef CHEMISTRY
    top = 0;
#endif

    ttl = dist;
    src = beg;
    dst = tmp;

    Recurse(0,0);
  }

  int Count(int dist)
  {
    Try(dist);
    return count;
  }

  int Closest(int max)
  {
    for (int i=0; i<=max; i++)
    {
      Try(i);
      if (count == 1)
        return i;
      if (count)
        return -1;
    }
    return -1;
  }

  bool Prefix(int max)
  {
    for (int i=0; i<=max; i++)
    {
      Try(i);
      if (prefix)
        return true;
    }
    return false;
  }
};

