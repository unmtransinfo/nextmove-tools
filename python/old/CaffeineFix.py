# Copyright (c) 2010 NextMove Software, Inc.

import drug_synonyms as CaffeineFixDictionary

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

