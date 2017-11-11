#ifndef __DATA_DICTIONARY_H__
#define __DATA_DICTIONARY_H__

#include <stdint.h>

typedef struct tag_definition_s {
  uint16_t group;
  uint16_t element;
  char     vr[9];
  char     vm[5];
  char     name[128];
} tag_definition_t;

extern const tag_definition_t g_tag_definitions[];

#endif // __DATA_DICTIONARY_H__