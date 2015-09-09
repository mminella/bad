#include "sync_print.hh"

#include "rec_loader.hh"

void RecLoader::rewind( void )
{
  loc_ = 0;
  eof_ = false;
  rio_->rewind();
}

RecordPtr RecLoader::next_record( void )
{
  const char * r = rio_->next_record();
  if ( r == nullptr ) {
    eof_ = true;
  }
  return {r, loc_++};
}

uint64_t RecLoader::filter( RR * r1, uint64_t size, const Record & after,
                            const RR * const curMin )
{
  if ( eof_ ) {
    return 0;
  }

  if ( curMin == nullptr ) {
    for ( uint64_t i = 0; i < size; loc_++ ) {
      const uint8_t * r = (const uint8_t *) rio_->next_record();
      if ( r == nullptr ) {
        eof_ = true;
        return i;
      }
      if ( after.compare( r, loc_ ) < 0 ) {
        r1[i++].copy( r, loc_ );
      // } else if ( after.compare( r, loc_ ) == 0 ) {
      //   print( "duplicate-rec", 1, after );
      //   r1[i++].copy( r, loc_ );
      }
    }
  } else {
    for ( uint64_t i = 0; i < size; loc_++ ) {
      const uint8_t * r = (const uint8_t *) rio_->next_record();
      if ( r == nullptr ) {
        eof_ = true;
        return i;
      }
      if ( after.compare( r, loc_ ) < 0 and
           curMin->compare( r, loc_ ) > 0 ) {
        r1[i++].copy( r, loc_ );
      // } else if ( after.compare( r, loc_ ) == 0 and
      //        curMin->compare( r, loc_ ) > 0 ) {
      //   print( "duplicate-rec", 2, after );
      //   r1[i++].copy( r, loc_ );
      // } else if ( after.compare( r, loc_ ) < 0 and
      //        curMin->compare( r, loc_ ) == 0 ) {
      //   print( "duplicate-rec", 3, curMin );
      //   r1[i++].copy( r, loc_ );
      // } else if ( after.compare( r, loc_ ) == 0 and
      //        curMin->compare( r, loc_ ) == 0 ) {
      //   print( "duplicate-rec", 4, after, curMin );
      //   r1[i++].copy( r, loc_ );
      }
    }
  }
  return size;
}
