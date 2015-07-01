#ifndef FILE_HH
#define FILE_HH

#include <tuple>

#include "file_descriptor.hh"

/* Unix file descriptors (sockets, files, etc.) */
class File : public FileDescriptor
{
public:
  /* construct from fd number */
  File( int fd );

  /* construct by opening file at path given */
  File( const char * path, int flags );
  File( const char * path, int flags, mode_t mode );

  File( const std::string path, int flags )
    : File( path.c_str(), flags ){};
  File( const std::string path, int flags, mode_t mode )
    : File( path.c_str(), flags, mode ){};

  /* forbid copying FileDescriptor objects or assigning them */
  File( const File & other ) = delete;
  File & operator=( const File & other ) = delete;

  /* move constructor */
  File( File && other ) = default;
  File & operator=( File && other ) = default;

  /* destructor */
  virtual ~File(){};

  /* rewind to begging of file */
  void rewind( void );
};

/* Buffered file. Only reads are buffered for now. */
class BufferedFile : public File
{
private:
  char buf_[MAX_READ_SIZE];
  size_t start_ = 0;
  size_t end_ = 0;

public:
  /* inherit File constructors */
  using File::File;

  std::tuple<const char *, size_t> internal_read( size_t limit = MAX_READ_SIZE,
                                                  bool copy = false );

protected:
  /* base read and write methods */
  virtual std::string rread( size_t limit = MAX_READ_SIZE ) override;

};

#endif /* FILE_HH */
