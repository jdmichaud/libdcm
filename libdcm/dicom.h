#ifndef __DICOM_H__
#define __DICOM_H__

#include <stdint.h>

#define PREAMBLE_LENGTH 0x80 // Cf DICOM Standard Part 10 Sect 7.1
#define MAGIC_WORD "DICM" // Cf DICOM Standard Part 10 Sect 7.1

#define UID_MAX_SIZE 64  // Cf DICOM Standard Part 5 Sect 6.2
#define MAX_SHORT_STR_SIZE 17 // Cf DICOM Standard Part 5 Sect 6.2

// Cf DICOM standard Part 6 Chapt A
typedef enum transfer_syntax_e {
  IMPLICIT, // 1.2.840.10008.1.2
  EXPLICIT_LITTLE_ENDIAN, // 1.2.840.10008.1.​2.​1
  EXPLICIT_BIG_ENDIAN, // 1.2.840.10008.1.​2.​2
  DEFLATED_EXPLICIT_BIG_ENDIAN, // 1.2.840.10008.1.2.​1.​99
} transfer_syntax_t;

#define NUMBER_OF_VR 31 // Cf DICOM standard Part 6 Sect 6.2

typedef struct vr_s {
  char name[3];
  uint8_t double_length; // Does tags of that type has a 32 bits lengths
  uint8_t string_of_character; // Does tags of that type carry a string of
                               // of characters as payload
} vr_t;

// Cf DICOM standard Part 6 Sect 6.2
static const vr_t g_valid_vrs[NUMBER_OF_VR] = {
  { "AE", 0, 1 },
  { "AS", 0, 1 },
  { "AT", 0, 0 },
  { "CS", 0, 1 },
  { "DA", 0, 1 },
  { "DS", 0, 1 },
  { "DT", 0, 1 },
  { "FL", 0, 0 },
  { "FD", 0, 0 },
  { "IS", 0, 1 },
  { "LO", 0, 1 },
  { "LT", 0, 1 },
  { "OB", 1, 0 },
  { "OD", 1, 0 },
  { "OF", 1, 0 },
  { "OL", 1, 0 },
  { "OW", 1, 0 },
  { "PN", 0, 1 },
  { "SH", 0, 1 },
  { "SL", 0, 1 },
  { "SQ", 1, 0 },
  { "SS", 0, 0 },
  { "ST", 0, 1 },
  { "TM", 0, 1 },
  { "UC", 0, 0 },
  { "UI", 0, 1 },
  { "UL", 0, 0 },
  { "UN", 1, 0 },
  { "UR", 0, 1 },
  { "US", 0, 0 },
  { "UT", 0, 1 },
};

#define TRANSFER_TYPE_IMPLICIT "1.2.840.10008.1.2"
#define TRANSFER_TYPE_EXPLICIT_LITTLE_ENDIAN "1.2.840.10008.1.2.1"
#define TRANSFER_TYPE_EXPLICIT_BIG_ENDIAN "1.2.840.10008.1.2.2"
#define TRANSFER_TYPE_DEFLATED_EXPLICIT_BIG_ENDIAN "1.2.840.10008.1.2.1.99"

#define META_DATA_GROUP 0x0002 // Cf DICOM standard Part 6 Chapt 7

#define SOP_INSTANCE_UID 0x00080018
#define STUDY_INSTANCE_UID 0x0020000d
#define SERIES_INSTANCE_UID 0x0020000e

#endif // __DICOM_H__