#ifndef __InflateStream_h
#define __InflateStream_h

#include <vector>
#include <algorithm>
#include <cassert>

extern "C" {
#include <zlib.h>
}

template<typename OutputIter>
int
inflateStream( std::istream& source, OutputIter dest ) {
  const unsigned int CHUNK = 16384;

  int ret;
  unsigned have;
  z_stream strm;
  std::vector< unsigned char > in(CHUNK);
  std::vector< unsigned char > out(CHUNK);

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit(&strm);
  if (ret != Z_OK)
    return ret;
  
  /* decompress until deflate stream ends or end of file */
  do {
    source.read( reinterpret_cast<char*>(&in[0]), CHUNK );
    strm.avail_in = source.gcount();
    if ( source.bad() ) {
      (void)inflateEnd(&strm);
      return Z_ERRNO;
    }
    if (strm.avail_in == 0)
      break;
    strm.next_in = &in[0];
    
    /* run inflate() on input until output buffer not full */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = &out[0];
      
      ret = inflate(&strm, Z_NO_FLUSH);
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
      switch (ret) {
      case Z_NEED_DICT:
	ret = Z_DATA_ERROR;     /* and fall through */
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
	(void)inflateEnd(&strm);
	return ret;
      }

      have = CHUNK - strm.avail_out;
      std::copy( out.begin(), out.begin() + have, dest );

    } while (strm.avail_out == 0);

    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

  /* clean up and return */
  (void)inflateEnd(&strm);
  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}


#endif
