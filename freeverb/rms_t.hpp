class _FV3_(rms)
{
 public:
  _FV3_(rms)();
  _FV3_(~rms)();
  void free();

  void setsize(long size) throw(std::bad_alloc);
  long getsize();
  void mute();
  inline _fv3_float_t process(_fv3_float_t input)
  {
    if(bufsize == 0) return std::fabs(input);
    if(bufidx == bufsize-1)
      bufidx = 0;
    else
      bufidx ++;
    sum -= buffer[bufidx];
    buffer[bufidx] = input*input;
    sum += buffer[bufidx];
    if(sum < 0) sum = 0;
    UNDENORMAL(input);
    _fv3_float_t ret = std::sqrt(sum/bufs);
    return ret;
  }
  inline _fv3_float_t operator()(_fv3_float_t input){return this->process(input);}

 private:
  _FV3_(rms)(const _FV3_(rms)& x);
  _FV3_(rms)& operator=(const _FV3_(rms)& x); 
  _fv3_float_t *buffer, sum, bufs;
  long bufsize, bufidx;
};
