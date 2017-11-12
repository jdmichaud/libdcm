//valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all
#define __USE_XOPEN_EXTENDED

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

#include "dicom.h"
#include "dcm.h"
#include "data-dictionary.h"

#define ERROR -1

typedef struct path_s {
  char *path;
  struct path_s *next;
} path_t;

void usage(char **argv) {
  fprintf(stderr, "usage: %s [FILE|DIRECTORY ...]\n", argv[0]);
}

int8_t output(file_t *file, dicom_meta_t *dicom_meta, tag_t *tags) {
  char *studyUid = "";
  char *seriesUid = "";
  char sopInstanceUid[64] = { 0 };
  if (dicom_meta->media_storage_sop_instance_uid[0]) {
    memcpy(sopInstanceUid, dicom_meta->media_storage_sop_instance_uid, 64);
  } else {
    tag_t *sopInstanceUidTag = get_tag(tags, SOP_INSTANCE_UID);
    if (sopInstanceUidTag == NULL) {
      fprintf(stderr, "error: SOP instance UID not found in dataset %s\n",
              file->filename);
    } else {
      memcpy(sopInstanceUid, (char *) sopInstanceUidTag->data,
        sopInstanceUidTag->datasize);
      sopInstanceUid[sopInstanceUidTag->datasize] = 0;
    }
  }
  tag_t *studyUidTag = get_tag(tags, STUDY_INSTANCE_UID);
  if (studyUidTag == NULL) {
    fprintf(stderr, "error: study UID not found in dataset %s\n",
            file->filename);
  } else studyUid = (char *) studyUidTag->data;
  tag_t *seriesUidTag = get_tag(tags, SERIES_INSTANCE_UID);
  if (seriesUidTag == NULL) {
    fprintf(stderr, "error: series UID not found in dataset %s\n",
            file->filename);
  } else seriesUid = (char *) seriesUidTag->data;

  printf(
    "{\"filename\":\"%s\",\"MediaStorageSOPInstanceUID\":\"%.64s\","
    "\"StudyInstanceUID\":\"%.64s\",\"SeriesInstanceUID\":\"%.64s\"}",
    file->filename, sopInstanceUid, studyUid, seriesUid);
  return 0;
}

int32_t parse_files(int32_t nfiles, path_t *path) {
  int32_t success = nfiles;
  int8_t first_file = 1;
  for (path_t *p = path; p; p = p->next) {
    file_t file;
    ssize_t offset = 0;
    tag_t tags[MAX_LOADED_TAG];
    memset(&file, 0, sizeof (file_t));
    memset(&tags, 0, sizeof (tag_t) * MAX_LOADED_TAG);
    load_file(p->path, &file);
    if (!is_dicom(&file)) {
      if (nfiles == 1)
        fprintf(stderr, "error: %s does not appear to be a dicom file\n",
                file.filename);
      if (nfiles == 1) return ERROR;
      success--;
    } else {
      dicom_meta_t dicom_meta;
      offset = check_preamble(&file, 0);
      offset = check_header(&file, offset);
      offset = decode_meta_data(&file, offset, &dicom_meta);
      if (offset == ERROR_BIG_ENDIAN) {
        fprintf(stderr, "error: %s: big endian not supported\n", file.filename);
        success--;
      } else {
        if (first_file) {
          first_file = 0;
          if (nfiles > 1) printf("[");
        } else {
          if (nfiles > 1) printf(",");
        }
        offset = decode_n_tags(&file, offset, &dicom_meta, tags, MAX_LOADED_TAG);
        if (output(&file, &dicom_meta, tags) == ERROR) success--;
      }
    }
    munmap(file.content, file.size);
    close_file(&file);
  }
  if (nfiles > 1) printf("]");
  return success;
}

int8_t add_path(path_t **paths, char *path) {
  path_t *new = malloc(sizeof (path_t));
  if (new == NULL) {
    perror("malloc");
    return ERROR;
  }
  new->path = (char *) strdup(path);
  if (new->path == NULL) {
    perror("strdup");
    return ERROR;
  }
  new->next = NULL;
  if (*paths == NULL) {
    *paths = new;
  } else {
    path_t *last = *paths;
    while (last->next != NULL) {
      last = last->next;
    }
    last->next = new;
  }
  return 0;
}

void free_paths(path_t **paths) {
  while (*paths != NULL) {
    path_t *tmp = (*paths)->next;
    free((*paths)->path);
    free(*paths);
    *paths = tmp;
  }
}

int32_t count_paths(path_t *paths) {
  int32_t c = 0;
  for (path_t *p = paths; p; ++c, p = p->next);
  return c;
}

int32_t generate_path(char *arg, path_t **paths) {
  struct stat buf;
  if (stat(arg, &buf)) {
    perror(arg);
  } else {
    if (S_ISDIR(buf.st_mode)) {
      DIR *dir;
      struct dirent entry;
      struct dirent *result;
      if ((dir = opendir(arg)) == NULL) {
        perror(arg);
      } else {
        int ret;
        for (ret = readdir_r(dir, &entry, &result);
             result != NULL && ret == 0;
             ret = readdir_r(dir, &entry, &result)) {
          if (!strcmp(result->d_name, ".") || !strcmp(result->d_name, ".."))
            continue;
          char path[1024];
          snprintf(path, 1024, "%s/%s", arg, result->d_name);
          generate_path(path, paths);
        }
        if (ret != 0)
          perror(arg);
        else
          closedir(dir);
      }
    } else add_path(paths, arg);
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(argv);
    return ERROR;
  }
  path_t *paths = NULL;
  for (int32_t i = 1; i < argc; ++i)
    generate_path(argv[i], &paths);
  int32_t nfiles = count_paths(paths);
  int8_t ret = !(nfiles == parse_files(nfiles, paths));
  free_paths(&paths);
  return ret;
}
