// cksum.hh
// Randal A. Koene, 20041221

#ifndef __CKSUM_HH
#define __CKSUM_HH

#include "dil2al.hh"
//#include <config.h>
#include <stdio.h>
#include <sys/types.h>

#if !defined UINT_FAST32_MAX && !defined uint_fast32_t
# define uint_fast32_t unsigned int
#endif

uint_fast32_t cksum(const char *buf, unsigned long length);

class CRC_File {
  String filename;
  String crcfilename;
  uint_fast32_t crc;
  long filelength;
  uint_fast32_t storedcrc;
  long storedfilelength;
public:
  CRC_File(String fname, String crcfname);
  CRC_File(String fname);
  uint_fast32_t Cached_CRC() { return crc; }
  long Cached_File_Length() { return filelength; }
  uint_fast32_t Cached_Stored_CRC() { return storedcrc; }
  long Cached_Stored_File_Length() { return storedfilelength; }
  bool Refresh_CRC();
  bool Get_Stored_CRC();
  bool Write();
  bool IsValid();
};

#endif
