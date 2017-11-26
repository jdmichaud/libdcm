#define __USE_XOPEN_EXTENDED

#include <stdio.h>
#include <sys/mman.h>

#include "dicom.h"
#include "dcm.h"
#include "data-dictionary.h"

void usage(char **argv) {
  fprintf(stderr, "usage: %s <FILE>\n", argv[0]);
}

int8_t output(file_t *file, dicom_meta_t *dicom_meta, tag_t *tags) {
  return 0;
}

int32_t parse_file(char *path) {
  file_t file;
  ssize_t offset = 0;
  tag_t tags[MAX_LOADED_TAG];
  memset(&file, 0, sizeof (file_t));
  memset(&tags, 0, sizeof (tag_t) * MAX_LOADED_TAG);
  if (load_file(path, &file) != ERROR) {
    if (!is_dicom(&file)) {
      fprintf(stderr, "error: %s does not appear to be a dicom file\n",
              file.filename);
      return ERROR;
    }
    dicom_meta_t dicom_meta;
    offset = check_preamble(&file, 0);
    offset = check_header(&file, offset);
    offset = decode_meta_data(&file, offset, &dicom_meta);
    if (offset == ERROR_BIG_ENDIAN) {
      fprintf(stderr, "error: %s: big endian not supported\n", file.filename);
      return ERROR;
    }
    size_t tag_offset = 0;
    offset = decode_n_tags(&file, offset, &dicom_meta, tags, &tag_offset,
                           MAX_LOADED_TAG);
    output(&file, &dicom_meta, tags);
    munmap(file.content, file.size);
    close_file(&file);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    usage(argv);
    return ERROR;
  }
  int8_t ret = parse_file(argv[1]);
  return ret;
}
