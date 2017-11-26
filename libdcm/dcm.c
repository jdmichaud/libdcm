#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

#include "data-dictionary.h"
#include "dicom.h"
#include "dcm.h"

const uint8_t g_implicit_tag_size = sizeof (uint16_t) * 4;
const uint8_t g_explicit_tag_size = sizeof (uint16_t) * 3 + sizeof (char) * 2;
const uint8_t g_double_length_explicit_tag_size =
  sizeof (uint16_t) * 3 + sizeof (char) * 2 + sizeof (uint32_t);

int8_t load_file(char *filename, file_t *file) {
  // Open the file
  file->fd = open(filename, O_RDONLY);
  if (file->fd < 0) {
    perror(filename);
    return ERROR;
  }
  // probe its size
  file->size = lseek(file->fd, 0, SEEK_END);
  if (file->size < 0) {
    perror("lseek");
    close(file->fd);
    return ERROR;
  }
  // Be kind, rewind
  lseek(file->fd, 0, SEEK_SET);
  // Load the file
  file->content = mmap(NULL, file->size, PROT_READ, MAP_SHARED, file->fd, 0);
  if (file == NULL) {
    perror("mmap");
    close(file->fd);
    return ERROR;
  }
  file->filename = filename;
  return 0;
}

int8_t close_file(file_t *file) {
  close(file->fd);
  return 0;
}

ssize_t check_preamble(file_t *file, ssize_t offset) {
  // Check presence of preamble
  explicit_tag_t *tag;
  tag = (explicit_tag_t *) &(file->content[offset]);
  // We assume if there is a tag, it is either a meta-tag or a 0008 group
  if (tag->group == 0x0002 || tag->group == 0x0008)
    // There is no preamble
    return offset;
  return offset + PREAMBLE_LENGTH;
}

ssize_t check_header(file_t *file, ssize_t offset) {
  // Check that magic word
  if (strncmp(MAGIC_WORD, (char *) &(file->content[offset]), 4))
    // Didn't find it? Start at the beginning and hope for the best
    return offset;
  return offset + 4; // length of "DICM"
}

char *tag_data_to_string(tag_t *tag, void *data, size_t *length) {
  // TODO: Better solution
  static char decimal[21]; // len(2^64) + 1
  if (TYPE_OF(tag, "UI") ||
      TYPE_OF(tag, "SH") ||
      TYPE_OF(tag, "CS") ||
      TYPE_OF(tag, "PN") ||
      TYPE_OF(tag, "LO") ||
      TYPE_OF(tag, "AE")
      ) {
    if (length) *length = tag->datasize + 1;
    return data;
  } else if (TYPE_OF(tag, "UL")) {
    snprintf(decimal, 21, "%u", *(uint32_t *) (tag->data));
    return decimal;
  } else if (TYPE_OF(tag, "US")) {
    snprintf(decimal, 21, "%u", *(uint16_t *) (tag->data));
    return decimal;
  } else if (TYPE_OF(tag, "IS")) {
    snprintf(decimal, 21, "%llu", atoll((char *) (tag->data)));
    return decimal;
  }
  if (length) *length = strlen(STR_REPR_BINARY) + 1;
  return STR_REPR_BINARY;
}

// TODO: Make it faster
void get_vr(implicit_tag_t *implicit_tag, char vr[2]) {
  if (implicit_tag->element == 0) {
    vr[0] = 'U'; vr[1] = 'L';
    return;
  }
  for (uint16_t i = 0; g_tag_definitions[i].group != 0xFFFE; ++i) {
    // printf("(%04X, %04X) (%04X, %04X)\n",
    // implicit_tag->group, implicit_tag->element,
    // g_tag_definitions[i].group, g_tag_definitions[i].element);
    if (g_tag_definitions[i].group == implicit_tag->group &&
        g_tag_definitions[i].element == implicit_tag->element) {
      vr[0] = g_tag_definitions[i].vr[0];
      vr[1] = g_tag_definitions[i].vr[1];
      return;
    }
  }
  vr[0] = 'U'; vr[1] = 'N';
}

ssize_t decode_explicit_tag(file_t *file, ssize_t offset, tag_t *tag) {
  if (offset + g_explicit_tag_size > file->size) return 0;
  explicit_tag_t *explicit_tag;
  explicit_tag = (explicit_tag_t *) &(file->content[offset]);
  tag->group = explicit_tag->group;
  tag->element = explicit_tag->element;
  tag->vr[0] = explicit_tag->vr[0]; tag->vr[1] = explicit_tag->vr[1];
  if (is_double_length_vr(tag->vr)) {
    double_length_explicit_tag_t *dl_explicit_tag;
    dl_explicit_tag = (double_length_explicit_tag_t *) &(file->content[offset]);
    tag->datasize = dl_explicit_tag->datasize;
    tag->data =
      (void *) &(file->content[offset + g_double_length_explicit_tag_size]);
    return g_double_length_explicit_tag_size;
  }
  tag->datasize = explicit_tag->datasize;
  tag->data = (void *) &(file->content[offset + g_explicit_tag_size]);
  return g_explicit_tag_size;
}

ssize_t decode_implicit_tag(file_t *file, ssize_t offset, tag_t *tag)
{
  if (offset + g_implicit_tag_size > file->size) return 0;
  implicit_tag_t *implicit_tag;
  implicit_tag = (implicit_tag_t *) &(file->content[offset]);
  tag->group = implicit_tag->group;
  tag->element = implicit_tag->element;
  get_vr(implicit_tag, tag->vr);
  tag->datasize = implicit_tag->datasize;
  tag->data = (void *) &(file->content[offset + g_implicit_tag_size]);
  return g_implicit_tag_size;
}

ssize_t decode_meta_data(file_t *file, ssize_t offset, dicom_meta_t *dicom_meta)
{
  ssize_t head = offset;
  ssize_t last_step = 0;
  tag_t tag;
  memset(dicom_meta, 0, sizeof (dicom_meta_t));
  // Default transfer syntax is IMPLICIT
  // Cf DICOM standard Part 5 Chapt 10.1
  dicom_meta->transfer_syntax = IMPLICIT;
  while (head <= file->size) {
    memset(&tag, 0, sizeof (tag));
    last_step = decode_explicit_tag(file, offset, &tag);
    //PRINT_TAG(stdout, tag);
    if (tag.group != META_DATA_GROUP) break;
    switch (tag.element) {
    case 0x0010:
      if (!strncmp(TRANSFER_TYPE_IMPLICIT, (char *) tag.data, tag.datasize))
        dicom_meta->transfer_syntax = IMPLICIT;
      else if (!strncmp(TRANSFER_TYPE_EXPLICIT_LITTLE_ENDIAN,
                        (char *) tag.data, tag.datasize))
        dicom_meta->transfer_syntax = EXPLICIT_LITTLE_ENDIAN;
      else if (!strncmp(TRANSFER_TYPE_EXPLICIT_BIG_ENDIAN,
                        (char *) tag.data, tag.datasize))
        // TODO: Big endian support
        return ERROR_BIG_ENDIAN;
      else if (!strncmp(TRANSFER_TYPE_DEFLATED_EXPLICIT_BIG_ENDIAN,
                        (char *) tag.data, tag.datasize))
        // TODO: Big endian support
        return ERROR_BIG_ENDIAN;
      else {
        // In case of compression, consider the meta-data as
        // encoded in EXPLICIT_LITTLE_ENDIAN (TODO: Ref to standard)
        dicom_meta->transfer_syntax = EXPLICIT_LITTLE_ENDIAN;
      }
      break;
    case 0x0003:
      memcpy(&(dicom_meta->media_storage_sop_instance_uid), (char *) tag.data,
             tag.datasize);
      dicom_meta->media_storage_sop_instance_uid[tag.datasize] = 0;
    default:
      break;
    }
    offset += last_step + tag.datasize;
  }
  // Rewind the last tag which is not a META tag
  return offset;
}

ssize_t decode_sequence_tag(file_t *file, ssize_t offset, dicom_meta_t *dicom_meta,
                            tag_t *tags, size_t *tag_offset, size_t maxtags) {
  // Is implicit or explicit
  if (dicom_meta->transfer_syntax == IMPLICIT) {
    // If implicit, check the size
    implicit_tag_t *implicit_tag;
    implicit_tag = (implicit_tag_t *) &(file->content[offset]);
    // TODO: Manage implicit sequences with defined length
    // If the size is not 0xFFFFFFFF, seek by the size
    if (implicit_tag->datasize != 0xFFFFFFFF) {
      offset += g_implicit_tag_size + implicit_tag->datasize;
    } else {
      // Seek to the first item
      offset += g_implicit_tag_size;
    }
  } else {
    // Seek to the first item
    offset += g_double_length_explicit_tag_size;
  }
  while (1) {
    // Check first item
    implicit_tag_t *implicit_tag;
    implicit_tag = (implicit_tag_t *) &(file->content[offset]);
    // If sequence delimiter tag found, stop
    if (implicit_tag->group == (SEQUENCE_DELIMITATION_TAG >> 16) &&
        implicit_tag->element == (SEQUENCE_DELIMITATION_TAG & 0x0000FFFF)) {
      offset += g_implicit_tag_size;
      break;
    }
    // If not first item -> ERROR
    if (implicit_tag->group != (ITEM_TAG >> 16) ||
        implicit_tag->element != (ITEM_TAG & 0x0000FFFF))
      return ERROR;
    // It is a first item, skip it and start reading sequence
    offset += g_implicit_tag_size;
    offset = decode_n_tags(file, offset, dicom_meta, tags, tag_offset,
                           maxtags);
    if (offset == ERROR) return ERROR;
    implicit_tag = (implicit_tag_t *) &(file->content[offset]);
    // If item delimiter tag found, skip it
    if (implicit_tag->group == (ITEM_DELIMITATION_TAG >> 16) &&
        implicit_tag->element == (ITEM_DELIMITATION_TAG & 0x0000FFFF))
      offset += g_implicit_tag_size;
    implicit_tag = (implicit_tag_t *) &(file->content[offset]);
  }
  return offset;
}

ssize_t decode_n_tags(file_t *file, ssize_t offset, dicom_meta_t *dicom_meta,
                     tag_t *tags, size_t *tag_offset, size_t maxtags)
{
  while (offset <= file->size && *tag_offset < maxtags) {
    ssize_t shift = 0;
    if ((shift = dicom_meta->transfer_syntax == IMPLICIT ?
        decode_implicit_tag(file, offset, &tags[*tag_offset]) :
        decode_explicit_tag(file, offset, &tags[*tag_offset])) == -1)
      return ERROR;
    // PRINT_TAG(stdout, tags[*tag_offset]);
    // TODO: Manage PixelData and other type of payloads
    if (tags[*tag_offset].group > 0x4FFE) break;
    // If end of item or end of sequence, we bailout
    if (tags[*tag_offset].group == 0xFFFE) break;
    // Sequence tag are managed by a special function
    if (tags[*tag_offset].vr[0] == 'S' && tags[*tag_offset].vr[1] == 'Q') {
      offset = decode_sequence_tag(file, offset, dicom_meta, tags, tag_offset,
                                   maxtags);
      if (offset == ERROR) return ERROR;
    } else {
      offset += shift + tags[*tag_offset].datasize;
      (*tag_offset)++;
    }
  }
  return offset;
}

uint8_t is_double_length_vr(char *s) {
  for (uint8_t i = 0; i < NUMBER_OF_VR; ++i)
    if (!strncmp(s, g_valid_vrs[i].name, 2))
      return g_valid_vrs[i].double_length;
  return 0;
}

uint8_t is_str_of_char_vr(char *s) {
  for (uint8_t i = 0; i < NUMBER_OF_VR; ++i)
    if (!strncmp(s, g_valid_vrs[i].name, 2))
      return g_valid_vrs[i].string_of_character;
  return 0;
}

uint8_t is_valid_vr(char *s) {
  for (uint8_t i = 0; i < NUMBER_OF_VR; ++i)
    if (!strncmp(s, g_valid_vrs[i].name, 2)) return 1;
  return 0;
}

uint8_t is_dicom(file_t *file) {
  // Check if DICM appears at byte 128
  if (file->size > 0x0080 &&
    !strncmp((char *) &(file->content[0x0080]), MAGIC_WORD, 4)) {
    return 1;
  }
  // Check if the first 2 bytes represent a group number of tag which are
  // usually at the beginning of a DICOM file
  if (file->size > 2 && (((uint16_t *) file->content)[0] == 0x0002 ||
    ((uint16_t *) file->content)[0] == 0x0008)) {
    // if yes, check if a known value representation is present
    if (is_valid_vr((char *) &(file->content[5]))) return 1;
    // Check if we can read a size and then read the next tag
    uint32_t datasize = ((uint32_t *) file->content)[1];
    if (file->size > 8 + datasize &&
      (((uint16_t *) file->content)[(8 + datasize) / sizeof (uint16_t)] == 0x0002 ||
       ((uint16_t *) file->content)[(8 + datasize) / sizeof (uint16_t)] == 0x0008)) {
      // We found another tag. Let's consider this a DICOM file
      return 1;
    }
  }
  return 0; // Not a DICOM file
}

tag_t *get_tag(tag_t *tags, uint32_t tagid) {
  for (ssize_t i = 0; i < MAX_LOADED_TAG
    && (tags[i].group != 0x0000 || tags[i].element != 0x0000); ++i) {
    if ((uint32_t) (tags[i].group << 16) + tags[i].element == tagid)
      return &tags[i];
  }
  return NULL;
}

void *get_tag_data(tag_t *tags, uint32_t tagid) {
  tag_t *tag = get_tag(tags, tagid);
  if (tag == NULL) return NULL;
  // We add one if we have a string of character so we can append a terminating
  // NULL character
  void *data = malloc(sizeof (char) *
                      (tag->datasize + (is_str_of_char_vr(tag->vr) ? 1 : 0)));
  if (data == NULL) {
    perror("malloc");
    return NULL;
  }
  memcpy(data, tag->data, sizeof (char) * tag->datasize);
  if (is_str_of_char_vr(tag->vr)) {
    ((char *) data)[tag->datasize] = 0;
  }
  return data;
}

char *trim(char *s, char *output) {
  if (*s == 0) return s;
  char *beg = s;
  char *end = s;
  while (*beg && isspace(*beg)) ++beg;
  while (*end) ++end;
  while ((end - 1) > beg && isspace(*(end - 1))) --end;
  if (output != NULL) s = output;
  memmove(s, beg, end - beg + 1);
  s[end - beg] = 0;
  return s;
}
