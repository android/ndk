//
// Copyright (C) 2013 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

// A small portable program used to dump the dynamic dependencies of a
// shared library. Requirements:
//   - Must support both 32-bit and 64-bit ELF binaries.
//   - Must support both little and big endian binaries.
//   - Must be compiled as a Unicode program on Windows.
//   - Follows Chromium coding-style guide.
//   - Single source file to make it easier to build anywhere.

//
// Work-around Windows Unicode support.
//

// Enable Windows Unicode support by default. Override this by
// setting WINDOWS_UNICODE at build time.
#if !defined(WINDOWS_UNICODE) && defined(_WIN32)
#define WINDOWS_UNICODE 1
#endif

#ifdef _WIN32
#undef UNICODE
#undef _UNICODE
#ifdef WINDOWS_UNICODE
#define UNICODE 1
#define _UNICODE 1
#endif
#include <windows.h>
#include <tchar.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#include <string>

// Define String as a typedef for either std::string or std::wstring
// depending on the platform.
#if WINDOWS_UNICODE
typedef std::wstring String;
#else
typedef std::string String;
#endif

// Use the following functions instead of their standard library equivalent.
#if !WINDOWS_UNICODE
#define TCHAR char
#define _T(x) x
#define _tgetenv   getenv
#define _tcslen    strlen
#define _tcschr    strchr
#define _tcscmp    strcmp
#define _tcsncmp   strncmp
#define _tfopen    fopen
#define _tprintf   printf
#define _vftprintf vfprintf
#define _ftprintf  fprintf
#define _tstat     stat
#define _vtprintf  vprintf
#define _stat      stat
#endif

// Use TO_STRING(xxx) to convert a C-string into the equivalent String.
#if WINDOWS_UNICODE == 1
static inline std::wstring __s2ws(const std::string& s) {
    int s_len = static_cast<int>(s.length() + 1);
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), s_len, 0, 0);
    std::wstring result(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), s_len, &result[0], len);
    return result;
}
#define TO_STRING(x) __s2ws(x)
#else
#define TO_STRING(x)  (x)
#endif

// Use TO_CONST_TCHAR(xxx) to convert a const char* to a const char/wchar*
#if WINDOWS_UNICODE == 1
#define TO_CONST_TCHAR(x) TO_STRING(x).c_str()
#else
#define TO_CONST_TCHAR(x)  (x)
#endif


//
// Start the real program now
//

#include <errno.h>
#ifdef __linux__
#include <glob.h>
#endif
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace {

// Utility functions.

enum {
  MESSAGE_FLAG_PANIC = (1 << 0),
  MESSAGE_FLAG_ERRNO = (1 << 1),
};

void vmessage(int flags, const TCHAR* fmt, va_list args) {
  int old_errno = errno;
  if (flags & MESSAGE_FLAG_PANIC)
    fprintf(stderr, "ERROR: ");

  _vftprintf(stderr, fmt, args);
  if (flags & MESSAGE_FLAG_ERRNO) {
    fprintf(stderr, ": %s", strerror(old_errno));
  }
  fprintf(stderr, "\n");

  if (flags & MESSAGE_FLAG_PANIC)
    exit(1);

  errno = old_errno;
}

void panic(const TCHAR* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vmessage(MESSAGE_FLAG_PANIC, fmt, args);
  va_end(args);
}

int g_verbose = 0;

void log_n(int n, const TCHAR* fmt, ...) {
  if (g_verbose >= n) {
    va_list args;
    va_start(args, fmt);
    _vtprintf(fmt, args);
    va_end(args);
  }
}

#define LOG_N(level,...) \
  ({ if (g_verbose >= (level)) log_n((level), __VA_ARGS__); })

#define LOG(...)  LOG_N(1,__VA_ARGS__)
#define LOG2(...) LOG_N(2,__VA_ARGS__)

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define DLOG(...) _tprintf(__VA_ARGS__)
#else
#define DLOG(...) ((void)0)
#endif

// Path utilites

// Return the position of the last directory separator in a path,
// or std::string::npos if none is found.
size_t path_last_dirsep(const String& filepath) {
#ifdef _WIN32
  size_t sep_slash = filepath.rfind(_T('/'));
  size_t sep_backslash = filepath.rfind(_T('\\'));
  size_t sep;
  if (sep_slash == std::string::npos)
    sep = sep_backslash;
  else if (sep_backslash == std::string::npos)
    sep = sep_slash;
  else
    sep = std::max(sep_slash, sep_backslash);
#else
  size_t sep = filepath.rfind(_T('/'));
#endif
  return sep;
}

// Return the directory name of a given path.
String path_dirname(const String& filepath) {
  size_t sep = path_last_dirsep(filepath);
  if (sep == std::string::npos)
    return String(_T("."));
  else if (sep == 0)
    return String(_T("/"));
  else
    return filepath.substr(0, sep);
}

// Return the basename of a given path.
String path_basename(const String& filepath) {
  size_t sep = path_last_dirsep(filepath);
  if (sep == std::string::npos)
    return filepath;
  else
    return filepath.substr(sep + 1);
}


// Reading utilities.

uint16_t get_u16_le(const uint8_t* bytes) {
  return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
}

uint32_t get_u32_le(const uint8_t* bytes) {
  return static_cast<uint32_t>(
      bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
}

uint64_t get_u64_le(const uint8_t* bytes) {
  uint64_t lo = static_cast<uint64_t>(get_u32_le(bytes));
  uint64_t hi = static_cast<uint64_t>(get_u32_le(bytes + 4));
  return lo | (hi << 32);
}

uint16_t get_u16_be(const uint8_t* bytes) {
  return static_cast<uint16_t>((bytes[0] << 8) | bytes[1]);
}

uint32_t get_u32_be(const uint8_t* bytes) {
  return static_cast<uint32_t>(
      (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
}

uint64_t get_u64_be(const uint8_t* bytes) {
  uint64_t hi = static_cast<uint64_t>(get_u32_be(bytes));
  uint64_t lo = static_cast<uint64_t>(get_u32_be(bytes + 4));
  return lo | (hi << 32);
}

// FileReader utility classes

class Reader {
public:
  Reader() {}

  virtual const uint8_t* GetBytesAt(off_t pos, size_t size) = 0;
  virtual uint16_t GetU16At(off_t pos) = 0;
  virtual uint32_t GetU32At(off_t pos) = 0;
  virtual uint64_t GetU64At(off_t pos) = 0;

  virtual ~Reader() {}
};

class FileReader : public Reader {
public:
  FileReader(FILE* file) : file_(file) {}

  virtual const uint8_t* GetBytesAt(off_t pos, size_t size) {
    if (size < kMaxBytes &&
        fseek(file_, pos, SEEK_SET) == 0 &&
        fread(buffer_, size, 1, file_) == 1) {
      return buffer_;
    } else
      return NULL;
  }

private:
  static const size_t kMaxBytes = 32;
  FILE* file_;
  uint8_t buffer_[kMaxBytes];
};

class FileLittleEndianReader : public FileReader {
public:
  FileLittleEndianReader(FILE* file) : FileReader(file) {}

  virtual uint16_t GetU16At(off_t pos) {
    const uint8_t* buf = GetBytesAt(pos, 2);
    return (buf != NULL) ? get_u16_le(buf) : 0;
  }

  virtual uint32_t GetU32At(off_t pos) {
    const uint8_t* buf = GetBytesAt(pos, 4);
    return (buf != NULL) ? get_u32_le(buf) : 0;
  }

  virtual uint64_t GetU64At(off_t pos) {
    const uint8_t* buf = GetBytesAt(pos, 8);
    return (buf != NULL) ? get_u64_le(buf) : 0ULL;
  }
};

class FileBigEndianReader : public FileReader {
public:
  FileBigEndianReader(FILE* file) : FileReader(file) {}

  virtual uint16_t GetU16At(off_t pos) {
    const uint8_t* buf = GetBytesAt(pos, 2);
    return (buf != NULL) ? get_u16_be(buf) : 0;
  }

  virtual uint32_t GetU32At(off_t pos) {
    const uint8_t* buf = GetBytesAt(pos, 4);
    return (buf != NULL) ? get_u32_be(buf) : 0;
  }

  virtual uint64_t GetU64At(off_t pos) {
    const uint8_t* buf = GetBytesAt(pos, 8);
    return (buf != NULL) ? get_u64_be(buf) : 0ULL;
  }
};

// ELF utility functions.

// The first 16 common bytes.
#define EI_NIDENT 16

#define EI_CLASS   4
#define ELFCLASS32 1
#define ELFCLASS64 2

#define EI_DATA     5
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

bool elf_ident_is_elf(const uint8_t* ident) {
  return ident[0] == 0x7f &&
         ident[1] == 'E' &&
         ident[2] == 'L' &&
         ident[3] == 'F';
}

bool elf_ident_is_big_endian(const uint8_t* ident) {
  return ident[EI_DATA] == ELFDATA2MSB;
}

bool elf_ident_is_64bits(const uint8_t* ident) {
  return ident[EI_CLASS] == ELFCLASS64;
}

#define SHT_STRTAB  3
#define SHT_DYNAMIC 6

// 32-bit ELF definitions.

class Elf32 {
public:
  typedef uint16_t Half;
  typedef uint32_t Word;
  typedef uint32_t Off;
  typedef uint32_t Addr;
  typedef int32_t Sword;

  struct Header {
    uint8_t e_ident[EI_NIDENT];
    Half    e_type;
    Half    e_machine;
    Word    e_version;
    Addr    e_entry;
    Off     e_phoff;
    Off     e_shoff;
    Word    e_flags;
    Half    e_shsize;
    Half    e_phentsize;
    Half    e_phnum;
    Half    e_shentsize;
    Half    e_shnum;
    Half    e_shstrndx;
  };

  struct Shdr {
    Word sh_name;
    Word sh_type;
    Word sh_flags;
    Addr sh_addr;
    Off  sh_offset;
    Word sh_size;
    Word sh_link;
    Word sh_info;
    Word sh_addralign;
    Word sh_entsize;
  };
};

class Elf64 {
public:
  typedef uint16_t Half;
  typedef uint64_t Off;
  typedef uint64_t Addr;
  typedef int32_t  Sword;
  typedef uint32_t Word;
  typedef uint64_t Xword;
  typedef int64_t Sxword;

  struct Header {
    uint8_t e_ident[EI_NIDENT];
    Half    e_type;
    Half    e_machine;
    Word    e_version;
    Addr    e_entry;
    Off     e_phoff;
    Off     e_shoff;
    Word    e_flags;
    Half    e_shsize;
    Half    e_phentsize;
    Half    e_phnum;
    Half    e_shentsize;
    Half    e_shnum;
    Half    e_shstrndx;
  };

  struct Shdr {
    Word  sh_name;
    Word  sh_type;
    Xword sh_flags;
    Addr  sh_addr;
    Off   sh_offset;
    Xword sh_size;
    Word  sh_link;
    Word  sh_info;
    Xword sh_addralign;
    Xword sh_entsize;
  };
};

template <class ELF>
class ElfParser {
public:
  ElfParser(Reader& reader) : reader_(reader) {}

  typedef typename ELF::Word Word;
  typedef typename ELF::Sword Sword;
  typedef typename ELF::Addr Addr;
  typedef typename ELF::Off Off;
  typedef typename ELF::Half Half;
  typedef typename ELF::Header ElfHeader;
  typedef typename ELF::Shdr Shdr;

  // Read an ELF::Word at a given position.
  Word GetWordAt(off_t pos) {
    return reader_.GetU32At(pos);
  }

  // Read an ELF::Half at a given position.
  Half GetHalfAt(off_t pos) {
    return reader_.GetU16At(pos);
  }

  // Read an ELF::Sword at a given position.
  Sword GetSwordAt(off_t pos) {
    return static_cast<Sword>(GetWordAt(pos));
  }

  // Read an ELF::Addr at a given position.
  Addr GetAddrAt(off_t pos);

  // Read an ELF::Off at a given position.
  Off GetOffAt(off_t pos) {
    return static_cast<Off>(GetAddrAt(pos));
  }

  // Helper class to iterate over the section table.
  class SectionIterator {
  public:
    explicit SectionIterator(ElfParser& parser)
      : parser_(parser) {
      table_offset_ = parser_.GetOffAt(offsetof(ElfHeader, e_shoff));
      table_count_ = parser_.GetHalfAt(offsetof(ElfHeader, e_shnum));
      table_entry_size_ = parser_.GetHalfAt(offsetof(ElfHeader, e_shentsize));
      if (table_entry_size_ < static_cast<Half>(sizeof(Shdr))) {
        // malformed binary. Ignore all sections.
        table_count_ = 0;
      }
    }

    Off NextOffset() {
      if (table_count_ == 0)
        return 0;
      Off result = table_offset_;
      table_offset_ += table_entry_size_;
      table_count_ -= 1;
      return result;
    }

    void Skip(int count) {
      while (count > 0 && table_count_ > 0) {
        table_offset_ += table_entry_size_;
        table_count_--;
        count--;
      }
    }

  private:
    ElfParser& parser_;
    Off table_offset_;
    Half table_count_;
    Half table_entry_size_;
  };

  // Return the offset of the first section of a given type, or 0 if not
  // found. |*size| will be set to the section size in bytes in case of
  // success.
  Off GetSectionOffsetByType(int table_type, Off* size) {
    SectionIterator iter(*this);
    for (;;) {
      Off table_offset = iter.NextOffset();
      if (table_offset == 0)
        break;
      Word sh_type = GetWordAt(table_offset + offsetof(Shdr, sh_type));
      if (sh_type == static_cast<Word>(table_type)) {
        *size = GetOffAt(table_offset + offsetof(Shdr, sh_size));
        return GetOffAt(table_offset + offsetof(Shdr, sh_offset));
      }
    }
    return 0;
  }

  // Return the index of the string table for the dynamic section
  // in this ELF binary. Or 0 if not found.
  int GetDynamicStringTableIndex() {
    SectionIterator iter(*this);
    for (;;) {
      Off table_offset = iter.NextOffset();
      if (table_offset == 0)
        break;
      Word sh_type = GetWordAt(table_offset + offsetof(Shdr, sh_type));
      if (sh_type == SHT_DYNAMIC)
        return GetWordAt(table_offset + offsetof(Shdr, sh_link));
    }
    return 0;
  }

  // Return the offset of a section identified by its index, or 0 in case
  // of error (bad index).
  Off GetSectionOffsetByIndex(int sec_index, Off* size) {
    SectionIterator iter(*this);
    iter.Skip(sec_index);
    Off table_offset = iter.NextOffset();
    if (table_offset != 0) {
      *size = GetOffAt(table_offset + offsetof(Shdr, sh_size));
      return GetOffAt(table_offset + offsetof(Shdr, sh_offset));
    }
    return 0;
  }

  // Return a string identified by its index and its string table
  // Address. Returns an empty string in case of error.
  String GetStringByIndex(Off str_index, int str_table_index) {
    String result;

    if (str_table_index != 0) {
      Off str_table_size = 0;
      Off str_table = GetSectionOffsetByIndex(str_table_index,
                                              &str_table_size);
      if (str_table != 0 && str_index < str_table_size) {
        str_table += str_index;
        str_table_size -= str_index;
        while (str_table_size > 0) {
          const uint8_t* p = reader_.GetBytesAt(str_table, 1);
          if (p == NULL || *p == '\0')
            break;
          result.append(1, static_cast<const char>(*p));
          str_table += 1;
          str_table_size -= 1;
        }
      }
    }
    return result;
  }

private:
  Reader& reader_;
};

template <>
Elf32::Addr ElfParser<Elf32>::GetAddrAt(off_t pos) {
  return reader_.GetU32At(pos);
}

template <>
Elf64::Addr ElfParser<Elf64>::GetAddrAt(off_t pos) {
  return reader_.GetU64At(pos);
}

// Helper class to iterate over items of a given type in the dynamic
// section. A type of 0 (SHT_NULL) means iterate over all items.
//
// Examples:
//    // Iterate over all entries in the table, find the SHT_NEEDED ones.
//    DynamicIterator<Elf32> iter(parser);
//    while (iter.GetNext()) {
//      if (iter.GetTag() == SHT_NEEDED) {
//         Elf32::Off value = iter.GetValue();
//         ...
//      }
//    }
template <class ELF>
class DynamicIterator {
public:
  explicit DynamicIterator(ElfParser<ELF>& parser)
    : parser_(parser),
      dyn_size_(0),
      dyn_offset_(0),
      started_(false) {
    dyn_offset_ = parser_.GetSectionOffsetByType(SHT_DYNAMIC, &dyn_size_);
    started_ = (dyn_size_ < kEntrySize);
  }

  bool GetNext() {
    if (!started_)
      started_ = true;
    else {
      if (dyn_size_ < kEntrySize)
        return false;
      dyn_offset_ += kEntrySize;
      dyn_size_ -= kEntrySize;
    }
    return true;
  }

  typename ELF::Off GetTag() {
    return parser_.GetOffAt(dyn_offset_);
  }

  typename ELF::Off GetValue() {
    return parser_.GetOffAt(dyn_offset_ + kTagSize);
  }

private:
  typedef typename ELF::Off Off;
  static const Off kTagSize = static_cast<Off>(sizeof(Off));
  static const Off kValueSize = kTagSize;
  static const Off kEntrySize = kTagSize + kValueSize;

  ElfParser<ELF>& parser_;
  Off dyn_size_;
  Off dyn_offset_;
  bool started_;
};

#define DT_NEEDED 1
#define DT_SONAME 14

template <class ELF>
String GetLibNameT(Reader& reader) {
  ElfParser<ELF> parser(reader);
  int str_table_index = parser.GetDynamicStringTableIndex();
  DynamicIterator<ELF> iter(parser);
  while (iter.GetNext()) {
    if (iter.GetTag() == DT_SONAME) {
      typename ELF::Off str_index = iter.GetValue();
      return parser.GetStringByIndex(str_index, str_table_index);
    }
  }
  return String();
}

template <class ELF>
int GetNeededLibsT(Reader& reader, std::vector<String>* result) {
  ElfParser<ELF> parser(reader);
  int str_table_index = parser.GetDynamicStringTableIndex();
  DynamicIterator<ELF> iter(parser);
  int count = 0;
  while (iter.GetNext()) {
    if (iter.GetTag() == DT_NEEDED) {
      typename ELF::Off str_index = iter.GetValue();
      String lib_name = parser.GetStringByIndex(str_index, str_table_index);
      if (!lib_name.empty()) {
        result->push_back(lib_name);
        count++;
      }
    }
  }
  return count;
}

class ElfFile {
public:
  ElfFile()
    : file_(NULL), big_endian_(false), is_64bits_(false), reader_(NULL) {}

  virtual ~ElfFile() {
    delete reader_;
    Close();
  }

  bool Open(const TCHAR* path, String* error) {
    Close();
    file_ = _tfopen(path, _T("rb"));
    if (file_ == NULL) {
      error->assign(TO_STRING(strerror(errno)));
      return false;
    }
    uint8_t ident[EI_NIDENT];
    if (fread(ident, sizeof(ident), 1, file_) != 1) {
      error->assign(TO_STRING(strerror(errno)));
      Close();
      return false;
    }
    if (!elf_ident_is_elf(ident)) {
      *error = _T("Not an ELF binary file");
      Close();
      return false;
    }
    big_endian_ = elf_ident_is_big_endian(ident);
    is_64bits_ = elf_ident_is_64bits(ident);

    if (big_endian_) {
      reader_ = new FileBigEndianReader(file_);
    } else {
      reader_ = new FileLittleEndianReader(file_);
    }
    return true;
  }

  bool IsOk() { return file_ != NULL; }

  bool IsBigEndian() { return big_endian_; }

  const Reader& GetReader() { return *reader_; };

  // Returns the embedded library name, extracted from the dynamic table.
  String GetLibName() {
    if (is_64bits_)
      return GetLibNameT<Elf64>(*reader_);
    else
      return GetLibNameT<Elf32>(*reader_);
  }

  // Gets the list of needed libraries and appends them to |result|.
  // Returns the number of library names appended.
  int GetNeededLibs(std::vector<String>* result) {
    if (is_64bits_)
      return GetNeededLibsT<Elf64>(*reader_, result);
    else
      return GetNeededLibsT<Elf32>(*reader_, result);
  }

protected:
  void Close() {
    if (file_ != NULL) {
      fclose(file_);
      file_ = NULL;
    }
  }

  FILE* file_;
  bool big_endian_;
  bool is_64bits_;
  Reader* reader_;
};

#ifdef __linux__
static bool IsLdSoConfSeparator(char ch) {
  // The ldconfig manpage indicates that /etc/ld.so.conf contains a list
  // of colon, space, tab newline or comma separated directories.
  return (ch == ' ' || ch == '\t' || ch == '\r' ||
          ch == '\n' || ch == ',' || ch == ':');
}

// Parse the content of /etc/ld.so.conf, it contains according to the
// documentation a 'comma, space, newline, tab separated list of
// directories'. In practice, it can also include comments, and an
// include directive and glob patterns, as in:
//  'include /etc/ld.so.conf.d/*.conf'
void AddHostLdSoConfPaths(const char* ld_so_conf_path,
                          std::vector<String>* lib_search_path) {
  FILE* file = fopen(ld_so_conf_path, "rb");
  if (!file)
    return;

  char line[1024];
  while (fgets(line, sizeof(line), file) != NULL) {
    const char* begin = line;
    const char* end = line + strlen(line);
    while (end > begin && end[-1] == '\n')
      end--;

    bool prev_is_include = false;
    while (begin < end) {
      // Skip over separators
      while (begin < end && IsLdSoConfSeparator(*begin))
        begin++;

      if (begin == end || begin[0] == '#') {
        // Skip empty lines and comments.
        break;
      }
      // Find end of current item
      const char* next_pos = begin;
      while (next_pos < end &&
          !IsLdSoConfSeparator(*next_pos) &&
          *next_pos != '#')
        next_pos++;

      size_t len = static_cast<size_t>(next_pos - begin);
      if (prev_is_include) {
        // If previous token was an 'include', treat this as a glob
        // pattern and try to process all matching files.
        prev_is_include = false;
        if (len == 0) {
          // Ignore stand-alone 'include' in a single line.
          break;
        }
        String pattern(begin, len);
        DLOG("%s: processing include '%s'\n",
             __FUNCTION__,
             pattern.c_str());

        glob_t the_glob;
        memset(&the_glob, 0, sizeof(the_glob));
        int ret = ::glob(pattern.c_str(), 0, NULL, &the_glob);
        if (ret == 0) {
          // Iterate / include all matching files.
          String filepath;
          for (size_t n = 0; n < the_glob.gl_pathc; ++n) {
            filepath.assign(the_glob.gl_pathv[n]);
            DLOG("%s: Including %s\n", __FUNCTION__, filepath.c_str());
            AddHostLdSoConfPaths(filepath.c_str(), lib_search_path);
          }
        }
        globfree(&the_glob);
      } else {
        // The previous token was not an 'include'. But is the current one?
        static const char kInclude[] = "include";
        const size_t kIncludeLen = sizeof(kInclude) - 1;
        if (len == kIncludeLen && !memcmp(begin, kInclude, len)) {
          prev_is_include = true;
        } else if (len > 0) {
          // No, it must be a directory name.
          String dirpath(begin, len);
          struct stat st;
          if (::stat(dirpath.c_str(), &st) != 0) {
            LOG("Could not stat(): %s: %s\n", dirpath.c_str(), strerror(errno));
          } else if (!S_ISDIR(st.st_mode)) {
            LOG("Not a directory: %s\n", dirpath.c_str());
          } else {
            DLOG("%s: Adding %s\n", __FUNCTION__, dirpath.c_str());
            lib_search_path->push_back(dirpath);
          }
        }
      }
      // switch to next item in line.
      begin = next_pos;
    }
  }
  fclose(file);
}
#endif  // __linux__

// Add host shared library search path to |lib_search_path|
void AddHostLibraryPaths(std::vector<String>* lib_search_path) {
  // Only add libraries form LD_LIBRARY_PATH on ELF-based systems.
#if defined(__ELF__)
  // If LD_LIBRARY_PATH is defined, process it
  const TCHAR* env = _tgetenv(_T("LD_LIBRARY_PATH"));
  if (env != NULL) {
    const TCHAR* pos = env;
    while (*pos) {
      size_t path_len;
      const TCHAR* next_pos = _tcschr(pos, ':');
      if (next_pos == NULL) {
        path_len = _tcslen(pos);
        next_pos = pos + path_len;
      } else {
        path_len = next_pos - pos;
        next_pos += 1;
      }

      if (path_len == 0) {
        // Per POSIX convention, an empty path item means "current path".
        // Not that this is generally a very bad idea, security-wise.
        lib_search_path->push_back(_T("."));
      } else {
        lib_search_path->push_back(String(pos, path_len));
      }

      pos = next_pos;
    }
  }
#ifdef __linux__
  AddHostLdSoConfPaths("/etc/ld.so.conf", lib_search_path);
#endif
  // TODO(digit): What about BSD systems?
#endif
}

// Returns true if |libname| is the name of an Android system library.
bool IsAndroidSystemLib(const String& libname) {
  static const TCHAR* const kAndroidSystemLibs[] = {
    _T("libc.so"),
    _T("libdl.so"),
    _T("liblog.so"),
    _T("libm.so"),
    _T("libstdc++.so"),
    _T("libz.so"),
    _T("libandroid.so"),
    _T("libjnigraphics.so"),
    _T("libEGL.so"),
    _T("libGLESv1_CM.so"),
    _T("libGLESv2.so"),
    _T("libOpenSLES.so"),
    _T("libOpenMAXAL.so"),
    NULL
  };
  for (size_t n = 0; kAndroidSystemLibs[n] != NULL; ++n) {
    if (!libname.compare(kAndroidSystemLibs[n]))
      return true;
  }
  return false;
}

// Returns true if |libname| is the name of an NDK-compatible shared
// library. This means its must begin with "lib" and end with "so"
// (without any version numbers).
bool IsAndroidNdkCompatibleLib(const String& libname) {
  return libname.size() > 6 &&
         !libname.compare(0, 3, _T("lib")) &&
         !libname.compare(libname.size() - 3, 3, _T(".so"));
}

// Try to find a library named |libname| in |search_paths|
// Returns true on success, and sets |result| to the full library path,
// false otherwise.
bool FindLibraryPath(const String& libname,
                     const std::vector<String>& search_paths,
                     String* result) {
  // Check in the search paths.
  LOG2(_T("  looking for library: %s\n"), libname.c_str());
  for (size_t n = 0; n < search_paths.size(); ++n) {
    String file_path = search_paths[n];
    if (file_path.empty())
      continue;
    if (file_path[file_path.size() - 1] != _T('/') &&
        file_path[file_path.size() - 1] != _T('\\')) {
      file_path.append(_T("/"));
    }
    file_path.append(libname);

    LOG2(_T("    in %s: "), file_path.c_str());
    struct _stat st;
    if (_tstat(file_path.c_str(), &st) < 0) {
      LOG2(_T("%s\n"), TO_CONST_TCHAR(strerror(errno)));
      continue;
    }
    if (!S_ISREG(st.st_mode)) {
      LOG2(_T("Not a regular file!\n"));
      continue;
    }
    // Found the library file.
    LOG2(_T("OK\n"));
    result->assign(file_path);
    return true;
  }

  return false;
}

// Recursive support

struct LibNode {
  // An enumeration listing possible node types, which are:
  enum Type {
    NODE_NONE,   // No type yet.
    NODE_PATH,   // Valid ELF library, |value| is file path.
    NODE_ERROR,  // Invalid library name, |value| is error string.
    NODE_SYSTEM, // Android system library, |value| is library name.
  };

  Type type;
  String value;
  std::vector<String> needed_libs;

  LibNode() : type(NODE_NONE), value(), needed_libs() {}

  explicit LibNode(const String& path)
    : type(NODE_PATH), value(path), needed_libs() {}

  void Set(Type type_p, const String& value_p) {
    type = type_p;
    value = value_p;
  }
};

typedef std::map<String, LibNode> DependencyGraph;
typedef std::list<String> WorkQueue;

// Used internally by BuildDependencyGraph().
void UpdateDependencies(
    const String& libname,
    const String& libpath,
    DependencyGraph& deps,
    WorkQueue& queue) {
  DLOG(_T("UPDATE libname=%s path=%s\n"), libname.c_str(), libpath.c_str());
  // Sanity check: find if the library is already in the graph.
  if (!libname.empty() && deps.find(libname) != deps.end()) {
    // Should not happen.
    panic(_T("INTERNAL: Library already in graph: %s"), libname.c_str());
  }

  LibNode node;
  ElfFile libfile;
  String error;
  String soname = libname;
  if (!libfile.Open(libpath.c_str(), &error)) {
    node.Set(LibNode::NODE_ERROR, error);
  } else {
    String soname = libfile.GetLibName();
    if (soname.empty())
      soname = libname;
    else if (soname != libname) {
      _ftprintf(stderr,
                _T("WARNING: Library has invalid soname ('%s'): %s\n"),
                soname.c_str(),
                libpath.c_str());
    }
    // Discovered a new library, get its dependent libraries.
    node.Set(LibNode::NODE_PATH, libpath);
    libfile.GetNeededLibs(&node.needed_libs);

    LOG(_T("%s depends on:"), soname.c_str());

    // Add them to the work queue.
    for (size_t n = 0; n < node.needed_libs.size(); ++n) {
      LOG(_T(" %s"), node.needed_libs[n].c_str());
      queue.push_back(node.needed_libs[n]);
    }
    LOG(_T("\n"));
  }
  deps[soname] = node;
}

// Build the full dependency graph.
// |root_libpath| is the path of the root library.
// |lib_search_path| is the list of library search paths.
// Returns a new dependency graph object.
DependencyGraph BuildDependencyGraph(
    const String& root_libpath,
    const std::vector<String>& lib_search_path) {
  DependencyGraph deps;
  std::list<String> queue;

  // As a first step, build the full dependency graph, starting with the
  // root library. This records errors in the graph too.
  UpdateDependencies(path_basename(root_libpath),
                     root_libpath,
                     deps, queue);

  while (!queue.empty()) {
    // Pop first item from queue.
    String libname = queue.front();
    queue.pop_front();

    // Is the library already in the graph?
    DependencyGraph::iterator iter = deps.find(libname);
    if (iter != deps.end()) {
      // Library already found, skip it.
      continue;
    }

    // Find the library in the current search path.
    String libpath;
    if (FindLibraryPath(libname, lib_search_path, &libpath)) {
      UpdateDependencies(libname, libpath, deps, queue);
      continue;
    }

    if (IsAndroidSystemLib(libname)) {
      LOG(_T("Android system library: %s\n"), libname.c_str());
      LibNode node;
      node.Set(LibNode::NODE_SYSTEM, libname);
      deps[libname] = node;
      continue;
    }

    _ftprintf(stderr,
              _T("WARNING: Could not find library: %s\n"),
              libname.c_str());
    LibNode node;
    node.Set(LibNode::NODE_ERROR, _T("Could not find library"));
    deps[libname] = node;
  }

  return deps;
}

// Print the dependency graph in a human-readable format to stdout.
void DumpDependencyGraph(const DependencyGraph& deps) {
  _tprintf(_T("Dependency graph:\n"));
  DependencyGraph::const_iterator iter = deps.begin();
  for ( ; iter != deps.end(); ++iter ) {
    const String& libname = iter->first;
    const LibNode& node = iter->second;
    String node_type;
    switch (node.type) {
      case LibNode::NODE_NONE:  // should not happen.
        node_type = _T("NONE??");
        break;
      case LibNode::NODE_PATH:
        node_type = _T("PATH");
        break;
      case LibNode::NODE_ERROR:
        node_type = _T("ERROR");
        break;
      case LibNode::NODE_SYSTEM:
        node_type = _T("SYSTEM");
    }
    _tprintf(
        _T("[%s] %s %s\n"),
        libname.c_str(),
        node_type.c_str(),
        node.value.c_str());

    if (node.type == LibNode::NODE_PATH) {
      for (size_t n = 0; n < node.needed_libs.size(); ++n) {
        _tprintf(_T("    %s\n"), node.needed_libs[n].c_str());
      }
    }
  }
}

// Return the sorted list of libraries from a dependency graph.
// They are topologically ordered, i.e. a library appears always
// before any other library it depends on.
void GetTopologicalSortedLibraries(
    DependencyGraph& deps,
    std::vector<String>* result) {
  result->clear();
  // First: Compute the number of visitors per library in the graph.
  typedef std::map<String, int> VisitorMap;
  VisitorMap visitors;
  for (DependencyGraph::const_iterator iter = deps.begin();
      iter != deps.end();
      ++iter) {
    if (visitors.find(iter->first) == visitors.end()) {
      visitors[iter->first] = 0;
    }

    const std::vector<String>& needed_libs = iter->second.needed_libs;
    for (size_t n = 0; n < needed_libs.size(); ++n) {
      const String& libname = needed_libs[n];
      VisitorMap::iterator lib_iter = visitors.find(libname);
      if (lib_iter != visitors.end())
        lib_iter->second += 1;
      else
        visitors[libname] = 1;
    }
  }

#if DEBUG
  {
    VisitorMap::const_iterator iter_end = visitors.end();
    VisitorMap::const_iterator iter = visitors.begin();
    for ( ; iter != iter_end; ++iter ) {
      _tprintf(_T("-- %s %d\n"), iter->first.c_str(), iter->second);
    }
  }
#endif

  while (!visitors.empty()) {
    // Find the library with the smallest number of visitors.
    // The value should be 0, unless there are circular dependencies.
    VisitorMap::const_iterator iter_end = visitors.end();
    VisitorMap::const_iterator iter;
    int min_visitors = INT_MAX;
    String min_libname;
    for (iter = visitors.begin(); iter != iter_end; ++iter) {
      // Note: Uses <= instead of < to ensure better diagnostics in
      // case of circular dependencies. This shall return the latest
      // node in the cycle, i.e. the first one where a 'back' edge
      // exists.
      if (iter->second <= min_visitors) {
        min_libname = iter->first;
        min_visitors = iter->second;
      }
    }

    if (min_visitors == INT_MAX) {
      // Should not happen.
      panic(_T("INTERNAL: Could not find minimum visited node!"));
    }

    // min_visitors should be 0, unless there are circular dependencies.
    if (min_visitors != 0) {
      // Warn about circular dependencies
      _ftprintf(stderr,
                _T("WARNING: Circular dependency found from: %s\n"),
                min_libname.c_str());
    }

    // Remove minimum node from the graph, and decrement the visitor
    // count of all its needed libraries. This also breaks dependency
    // cycles.
    result->push_back(min_libname);
    const LibNode& node = deps[min_libname];
    const std::vector<String> needed_libs = node.needed_libs;
    visitors.erase(min_libname);

    for (size_t n = 0; n < needed_libs.size(); ++n)
      visitors[needed_libs[n]]--;
  }
}

// Main function

#define PROGNAME "ndk-depends"

void print_usage(int exit_code) {
  printf(
    "Usage: %s [options] <elf-file>\n\n"

    "This program is used to print the dependencies of a given ELF\n"
    "binary (shared library or executable). It supports any architecture,\n"
    "endianess and bitness.\n\n"

    "By default, all dependencies are printed in topological order,\n"
    "which means that each item always appear before other items\n"
    "it depends on. Except in case of circular dependencies, which will\n"
    "print a warning to stderr.\n\n"

    "The tool will try to find other libraries in the same directory\n"
    "as the input ELF file. It is possible however to provide\n"
    "additional search paths with the -L<path>, which adds an explicit path\n"
    "or --host-libs which adds host-specific library paths, on ELF-based systems\n"
    "only.\n\n"

    "Use --print-paths to print the path of each ELF binary.\n\n"

    "Use --print-direct to only print the direct dependencies\n"
    "of the input ELF binary. All other options except --verbose will be ignored.\n\n"

    "Use --print-java to print a Java source fragment that loads the\n"
    "libraries with System.loadLibrary() in the correct order. This can\n"
    "be useful when copied into your application's Java source code.\n\n"

    "Use --print-dot to print the dependency graph as a .dot file that can be\n"
    "parsed by the GraphViz tool. For example, to generate a PNG image of the\n"
    "graph, use something like:\n\n"

    "  ndk-depends /path/to/libfoo.so --print-dot | dot -Tpng -o /tmp/graph.png\n\n"

    "The --verbose option prints debugging information, which can be useful\n"
    "to diagnose problems with malformed ELF binaries.\n\n"

    "Valid options:\n"
    "  --help|-h|-?    Print this message.\n"
    "  --verbose       Increase verbosity.\n"
    "  --print-direct  Only print direct dependencies.\n"
    "  -L<path>        Append <path> to the library search path.\n"
    "  --host-libs     Append host library search path.\n"
    "  --print-paths   Print full paths of all libraries.\n"
    "  --print-java    Print Java library load sequence.\n"
    "  --print-dot     Print the dependency graph as a Graphviz .dot file.\n"
    "\n", PROGNAME);

  exit(exit_code);
}

}  // namespace


#ifdef _WIN32
int main(void) {
  int argc = 0;
  TCHAR** argv = CommandLineToArgvW(GetCommandLine(), &argc);
#else
int main(int argc, const char** argv) {
#endif

  enum PrintFormat {
    PRINT_DEFAULT = 0,
    PRINT_DIRECT,
    PRINT_PATHS,
    PRINT_JAVA,
    PRINT_DOT_FILE,
  };

  bool do_help = false;
  PrintFormat print_format = PRINT_DEFAULT;
  std::vector<String> lib_search_path;
  std::vector<String> params;

  // Process options.
  while (argc > 1) {
    if (argv[1][0] == _T('-')) {
      const TCHAR* arg = argv[1];
      if (!_tcscmp(arg, _T("--help")) ||
          !_tcscmp(arg, _T("-h")) ||
          !_tcscmp(arg, _T("-?")))
        do_help = true;
      else if (!_tcscmp(arg, _T("--print-direct"))) {
        print_format = PRINT_DIRECT;
      } else if (!_tcscmp(arg, _T("-L"))) {
        if (argc < 3)
          panic(_T("Option -L requires an argument."));
        lib_search_path.push_back(String(argv[2]));
        argc--;
        argv++;
      } else if (!_tcsncmp(arg, _T("-L"), 2)) {
        lib_search_path.push_back(String(arg+2));
      } else if (!_tcscmp(arg, _T("--host-libs"))) {
        AddHostLibraryPaths(&lib_search_path);
      } else if (!_tcscmp(arg, _T("--print-java"))) {
        print_format = PRINT_JAVA;
      } else if (!_tcscmp(arg, _T("--print-paths"))) {
        print_format = PRINT_PATHS;
      } else if (!_tcscmp(arg, _T("--print-dot"))) {
        print_format = PRINT_DOT_FILE;
      } else if (!_tcscmp(arg, _T("--verbose"))) {
        g_verbose++;
      } else {
        panic(_T("Unsupported option '%s', see --help."), arg);
      }
    } else {
      params.push_back(String(argv[1]));
    }
    argc--;
    argv++;
  }

  if (do_help)
    print_usage(0);

  if (params.empty())
    panic(_T("Please provide the path of an ELF shared library or executable."
             "\nSee --help for usage details."));

  // Insert ELF file directory at the head of the search path.
  lib_search_path.insert(lib_search_path.begin(), path_dirname(params[0]));

  if (g_verbose >= 1) {
    _tprintf(_T("Current library search path:\n"));
    for (size_t n = 0; n < lib_search_path.size(); ++n)
      _tprintf(_T("  %s\n"), lib_search_path[n].c_str());
    _tprintf(_T("\n"));
  }

  // Open main input file.
  const TCHAR* libfile_path = params[0].c_str();

  ElfFile libfile;
  String error;
  if (!libfile.Open(libfile_path, &error)) {
    panic(_T("Could not open file '%s': %s"), libfile_path, error.c_str());
  }

  if (print_format == PRINT_DIRECT) {
    // Simple dump, one line per dependency. No frills, no recursion.
    std::vector<String> needed_libs;
    libfile.GetNeededLibs(&needed_libs);

    for (size_t i = 0; i < needed_libs.size(); ++i)
      _tprintf(_T("%s\n"), needed_libs[i].c_str());

    return 0;
  }

  // Topological sort of all dependencies.
  LOG(_T("Building dependency graph...\n"));
  DependencyGraph deps = BuildDependencyGraph(
      libfile_path, lib_search_path);

  if (g_verbose >= 2)
    DumpDependencyGraph(deps);

  LOG(_T("Building sorted list of binaries:\n"));
  std::vector<String> needed_libs;
  GetTopologicalSortedLibraries(deps, &needed_libs);

  if (print_format == PRINT_JAVA) {
    // Print Java libraries in reverse order.
    std::reverse(needed_libs.begin(), needed_libs.end());
    for (size_t i = 0; i < needed_libs.size(); ++i) {
      const String& lib = needed_libs[i];
      if (IsAndroidSystemLib(lib)) {
        // Skip system libraries.
        continue;
      }
      if (!IsAndroidNdkCompatibleLib(lib)) {
        _ftprintf(
            stderr,
            _T("WARNING: Non-compatible library name ignored: %s\n"),
            lib.c_str());
        continue;
      }
      _tprintf(_T("System.loadLibrary(%s);\n"),
                lib.substr(3, lib.size() - 6).c_str());
    }
    return 0;
  }

  if (print_format == PRINT_DOT_FILE) {
    // Using the topological order helps generates a more human-friendly
    // directed graph.
    _tprintf(_T("digraph {\n"));
    for (size_t i = 0; i < needed_libs.size(); ++i) {
      const String& libname = needed_libs[i];
      const std::vector<String>& libdeps = deps[libname].needed_libs;
      for (size_t n = 0; n < libdeps.size(); ++n) {
        // Note: Use quoting to deal with special characters like -
        // which are not normally part of DOT 'id' tokens.
        _tprintf(_T("  \"%s\" -> \"%s\"\n"), libname.c_str(), libdeps[n].c_str());
      }
    }
    _tprintf(_T("}\n"));
    return 0;
  }

  if (print_format == PRINT_PATHS) {
    // Print libraries with path.
    for (size_t i = 0; i < needed_libs.size(); ++i) {
      const String& lib = needed_libs[i];
      LibNode& node = deps[lib];
      const TCHAR* format;
      switch (node.type) {
        case LibNode::NODE_PATH:
          format = _T("%s -> %s\n");
          break;
        case LibNode::NODE_SYSTEM:
          format = _T("%s -> $ /system/lib/%s\n");
          break;
        default:
          format = _T("%s -> !! %s\n");
      }
      _tprintf(format, lib.c_str(), deps[lib].value.c_str());
    }
    return 0;
  }

  // Print simple library names.
  for (size_t i = 0; i < needed_libs.size(); ++i) {
    const String& lib = needed_libs[i];
    _tprintf(_T("%s\n"), lib.c_str());
  }
  return 0;
}
