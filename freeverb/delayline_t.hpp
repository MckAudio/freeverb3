/**
 * DelayLine Processor Base Class
 */
class _FV3_(delayline)
{
 public:
  _FV3_(delayline)() throw(std::bad_alloc);
  virtual _FV3_(~delayline)();
  void free();
  virtual void         setSampleRate(_fv3_float_t fs) throw(std::bad_alloc);
  virtual _fv3_float_t getSampleRate();
  void setsize(long size) throw(std::bad_alloc);
  long getsize();
  virtual void mute();
  virtual _fv3_float_t process(_fv3_float_t input);
  /**
   * set the prime mode for delay lines.
   * the size of the delay lines will prime numbers by default.
   * To inhibit prime number size delay lines, set this options to false.
   * @param[in] value set true (default) to enable prime mode.
   */
  virtual void setPrimeMode(bool value){primeMode = value;}
  virtual bool getPrimeMode(){return primeMode;}
  
  inline _fv3_float_t& at(long rindex)
  {
    long readidx = baseidx + rindex;
    if(readidx >= bufsize) readidx -= bufsize;
    return buffer[readidx];
  }
  inline _fv3_float_t& operator[](long rindex){return at(rindex);}

  inline _fv3_float_t at(_fv3_float_t rindex, _fv3_float_t& ap_save)
  {
    _fv3_float_t floor_mod = std::floor(rindex); // >= 0
    _fv3_float_t frac = rindex - floor_mod; // >= 0
    
    long readidx_a = baseidx + (long)floor_mod;
    if(readidx_a >= bufsize) readidx_a -= bufsize;
    long readidx_b = readidx_a + 1;
    if(readidx_b >= bufsize) readidx_b -= bufsize;
    
    _fv3_float_t temp = buffer[readidx_b] + buffer[readidx_a]*(1-frac) - (1-frac)*ap_save;
    UNDENORMAL(temp);
    ap_save = temp;    
    return temp;
  }

 protected:
  inline void allpass(long rindexbase, long rlength, _fv3_float_t feedback)
  {
    long rindex1 = rindexbase, rindex2 = rindex1 + rlength;
    _fv3_float_t r1 = (*this)[rindex1], r2 = (*this)[rindex2];
    r2 -= feedback * r1; r1 += feedback * r2;
    (*this)[rindex1] = r1; (*this)[rindex2] = r2;
  }

  /**
   * get the prime number length for delay lines.
   * @param[in] ms delay length in msec.
   */
  virtual long p_(_fv3_float_t ms);
  
  _FV3_(delayline)(const _FV3_(delayline)& x);
  _FV3_(delayline)& operator=(const _FV3_(delayline)& x);  
  _fv3_float_t *buffer, currentfs;
  long bufsize, baseidx;
  bool primeMode;
};
