#ifndef __DICM_H__
#define __DICM_H__

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "dicom.h"

#define MAJOR 0
#define MINOR 1
#define PATCH 0

#define ERROR -1
#define ERROR_BIG_ENDIAN -2
#define STR_REPR_BINARY "<binary data>"
#define STR_REPR_TOO_MUCH_DATA "<too much data>"
#define MAX_LOADED_TAG 4096

#define TYPE_OF(TAG, VR) !strncmp(TAG->vr, VR, 2)

#define PRINT_TAG(fd, tag) \
  printf("(0x%04X, 0x%04X) %.2s (%u) [%s]\n", tag.group, \
         tag.element, tag.vr, tag.datasize, \
         tag_data_to_string(&tag, (char *) tag.data, NULL));

typedef struct file_s {
  int16_t fd;
  ssize_t size;
  uint8_t *content;
  char    *filename;
} file_t;

typedef struct implicit_tag_s {
  uint16_t group;
  uint16_t element;
  uint16_t datasize;
  void     *data;
} implicit_tag_t;

extern const uint8_t g_implicit_tag_size;

typedef struct explicit_tag_s {
  uint16_t group;
  uint16_t element;
  char     vr[2];
  uint16_t datasize;
  void     *data;
} explicit_tag_t;

extern const uint8_t g_explicit_tag_size;

typedef struct double_length_explicit_tag_s {
  uint16_t group;
  uint16_t element;
  char     vr[2];
  char     reserved;
  uint32_t datasize;
  void     *data;
} double_length_explicit_tag_t;

extern const uint8_t g_double_length_explicit_tag_size;

typedef double_length_explicit_tag_t tag_t;

// Cf DICOM standard Part 6 Chapt 7
typedef struct dicom_meta_s {
  size_t file_meta_information_group_length;
  char *file_meta_information_version;
  char media_storage_sop_class_uid[UID_MAX_SIZE];
  char media_storage_sop_instance_uid[UID_MAX_SIZE];
  transfer_syntax_t transfer_syntax;
  char implementation_class_uid[UID_MAX_SIZE];
  char implementation_version_name[MAX_SHORT_STR_SIZE];
  char source_application_entity_title[MAX_SHORT_STR_SIZE];
  char sending_application_entity_title[MAX_SHORT_STR_SIZE];
  char receiving_application_entity_title[MAX_SHORT_STR_SIZE];
  char private_information_creator_uid[MAX_SHORT_STR_SIZE];
  char *private_information;
} dicom_meta_t;

int8_t load_file(char *filename, file_t *file);
int8_t close_file(file_t *file);
ssize_t check_preamble(file_t *file, ssize_t offset);
ssize_t check_header(file_t *file, ssize_t offset);
char *tag_data_to_string(tag_t *tag, void *data, size_t *length);
void get_vr(implicit_tag_t *implicit_tag, char vr[2]);
ssize_t decode_explicit_tag(file_t *file, ssize_t offset, tag_t *tag);
ssize_t decode_implicit_tag(file_t *file, ssize_t offset, tag_t *tag);
ssize_t decode_meta_data(file_t *file, ssize_t offset, dicom_meta_t *dicom_meta);
int8_t decode_n_tags(file_t *file, ssize_t offset, dicom_meta_t *dicom_meta,
                     tag_t *tags, size_t maxtags);
uint8_t is_double_length_vr(char *s);
uint8_t is_valid_vr(char *s);
uint8_t is_dicom(file_t *file);
tag_t *get_tag(tag_t *tags, uint32_t number);
void *get_tag_data(tag_t *tags, uint32_t number);
char *trim(char *s, char *output);

#endif // __DICM_H__