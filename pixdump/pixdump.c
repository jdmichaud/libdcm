#define __USE_XOPEN_EXTENDED

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "../libdcm/dicom.h"
#include "../libdcm/dcm.h"
#include "../libdcm/data-dictionary.h"

void usage(char **argv) {
  fprintf(stderr, "usage: %s <FILE>\n", argv[0]);
}

int8_t write_image_raw_8bits(uint16_t width, uint16_t height, uint8_t bits_allocated,
                             uint8_t bits_stored, void *data, char *filename) {
  int8_t of = creat(filename, O_CREAT);
  if (of < 0) {
    perror(filename);
    return ERROR;
  }
  uint8_t *shifted = malloc(sizeof (uint8_t) * width * height);
  uint16_t mask = (1 << bits_stored) - 1;
  for (ssize_t i = 0; i < width * height; ++i) {
    shifted[i] = (((uint16_t *) data)[i] & mask) << (bits_allocated - bits_stored) >> 8;
  }
  write(of, shifted, width * height);
  free(shifted);
  close(of);
  return 0;
}

int8_t write_image_raw(uint16_t width, uint16_t height, uint8_t bits_allocated,
                       uint8_t bits_stored, void *data, char *filename) {
  int8_t of = creat(filename, O_CREAT);
  if (of < 0) {
    perror(filename);
    return ERROR;
  }
  uint16_t *shifted = malloc(sizeof (uint16_t) * width * height);
  uint16_t mask = (1 << bits_stored) - 1;
  for (ssize_t i = 0; i < width * height; ++i) {
    shifted[i] = (((uint16_t *) data)[i] & mask) << (bits_allocated - bits_stored);
  }
  write(of, shifted, width * height * (bits_allocated >> 3));
  free(shifted);
  close(of);
  return 0;
}

int8_t write_image_ppm(uint16_t width, uint16_t height, uint8_t bits_allocated,
                       uint8_t bits_stored, void *data, char *filename) {
  int8_t of = creat(filename, O_CREAT);
  if (of < 0) {
    perror(filename);
    return ERROR;
  }
  // Print ppm header
  dprintf(of, "P5\n%u %u\n%u\n", width, height, (1 << bits_allocated) - 1);
  // Shift due to bits_stored
  uint16_t *shifted = malloc(sizeof (uint16_t) * width * height);
  uint16_t mask = (1 << bits_stored) - 1;
  for (ssize_t i = 0; i < width * height; ++i) {
    uint16_t v = (((uint16_t *) data)[i] & mask) << (bits_allocated - bits_stored);
    shifted[i] = ((char *) &v)[0] << 8 | ((char *) &v)[1];
  }
  write(of, shifted, width * height * (bits_allocated >> 3));
  free(shifted);
  // Simple write
  // write(of, data, width * height * (bits_allocated >> 3));
  close(of);
  return 0;
}

int8_t output(file_t *file, dicom_meta_t *dicom_meta, tag_t *tags) {
  ssize_t row = 0;
  ssize_t column = 0;
  ssize_t high_bit = 0;
  ssize_t bits_stored = 0;
  ssize_t bits_allocated = 0;
  get_tag_data_int(tags, ROW_TAG, &row);
  get_tag_data_int(tags, COLUMN_TAG, &column);
  get_tag_data_int(tags, BITS_STORED_TAG, &bits_stored);
  get_tag_data_int(tags, BITS_ALLOCATED_TAG, &bits_allocated);
  get_tag_data_int(tags, HIGH_BIT_TAG, &high_bit);
  printf("high bit: %lu\n", high_bit);
  tag_t *pixeldata = get_tag(tags, PIXEL_DATA_TAG);
  if (pixeldata == NULL) {
    fprintf(stderr, "error: could not retrieve pixels\n");
    return ERROR;
  }
  char *dot = strrchr(file->filename, '.');
  char *output_filename;
  if (dot == NULL) dot = &file->filename[strlen(file->filename)];
  output_filename = malloc(sizeof (char) * (dot - file->filename + 9));
  printf("row:%lu\n", row);
  printf("column:%lu\n", column);
  printf("pixeldata->datasize:%u\n", pixeldata->datasize);
  ssize_t nbframes = pixeldata->datasize / (row * column * (bits_allocated >> 3));
  printf("nbframes %li\n", nbframes);
  for (ssize_t i = 0; i < nbframes; ++i) {
    sprintf(output_filename, "%.*s_%03li.ppm", (int) (dot - file->filename),
      file->filename, i);
    printf("writing %s\n", output_filename);
    write_image_ppm(column, row, bits_allocated, bits_stored,
      &((uint8_t *) pixeldata->data)[(row * column * (bits_allocated >> 3)) * i],
      output_filename);
  }
  free(output_filename);
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
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    usage(argv);
    return ERROR;
  }
  int8_t ret = parse_file(argv[1]);
  return ret;
}
