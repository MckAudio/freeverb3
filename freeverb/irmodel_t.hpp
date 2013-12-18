class _FV3_(irmodel) : public _FV3_(irbase)
{
 public:
  _FV3_(irmodel)();
  virtual _FV3_(~irmodel)();  
  virtual void loadImpulse(const _fv3_float_t * inputL, const _fv3_float_t * inputR, long size)
    throw(std::bad_alloc);
  virtual void unloadImpulse();  
  virtual void processreplace(_fv3_float_t *inputL, _fv3_float_t *inputR,
			      _fv3_float_t *outputL, _fv3_float_t *outputR,
			      long numsamples, unsigned options);
  virtual void mute();
  virtual long getLatency();
  long getFragmentSize();

 private:
  _FV3_(irmodel)(const _FV3_(irmodel)& x);
  _FV3_(irmodel)& operator=(const _FV3_(irmodel)& x);

  void processSquare(_fv3_float_t *inputL, _fv3_float_t *inputR,
		     _fv3_float_t *outputL, _fv3_float_t *outputR);
  void allocImpulse(long fsize, long isize) throw(std::bad_alloc);
  void freeImpulse();
  void allocFFT(long size, unsigned fftflags) throw(std::bad_alloc);
  void freeFFT();
  long fragmentSize, fftSize, current, fifoSize;
  _FFTW_(plan) planRevL, planRevR, planOrigL, planOrigR;
  _FV3_(delay) delayL, delayR;
  _FV3_(slot) fftRev, fftOrig, fftImpulse, delayline, fifo;
};
