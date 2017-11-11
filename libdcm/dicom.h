#ifndef __DICOM_H__
#define __DICOM_H__

#define PREAMBLE_LENGTH 0x80 // Cf DICOM Standard Part 10 Sect 7.1
#define MAGIC_WORD  "DICM" // Cf DICOM Standard Part 10 Sect 7.1

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

// Cf DICOM standard Part 6 Sect 6.2
static const char g_valid_vrs[NUMBER_OF_VR][3] = {
  "AE", "AS", "AT", "CS", "DA", "DS", "DT", "FL", "FD", "IS", "LO", "LT", "OB",
  "OD", "OF", "OL", "OW", "PN", "SH", "SL", "SQ", "SS", "ST", "TM", "UC", "UI",
  "UL", "UN", "UR", "US", "UT",
};

#define TRANSFER_TYPE_IMPLICIT "1.2.840.10008.1.2"
#define TRANSFER_TYPE_EXPLICIT_LITTLE_ENDIAN "1.2.840.10008.1.​2.​1"
#define TRANSFER_TYPE_EXPLICIT_BIG_ENDIAN "1.2.840.10008.1.​2.​2"
#define TRANSFER_TYPE_DEFLATED_EXPLICIT_BIG_ENDIAN "1.2.840.10008.1.​2.​1.99"

#define META_DATA_GROUP 0x0002 // Cf DICOM standard Part 6 Chapt 7

#define STUDY_INSTANCE_UID 0x0020000d
#define SERIES_INSTANCE_UID 0x0020000e

#endif // __DICOM_H__