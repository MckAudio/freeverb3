class _FV3_(irmodel2) : public _FV3_(irbase)
{
 public:
  _FV3_(irmodel2)();
  virtual _FV3_(~irmodel2)();
  virtual void loadImpulse(const _fv3_float_t * inputL, const _fv3_float_t * inputR, long size)
    throw(std::bad_alloc);
  virtual void unloadImpulse();
  virtual void processreplace(_fv3_float_t *inputL, _fv3_float_t *inputR,
			      _fv3_float_t *outputL, _fv3_float_t *outputR,
			      long numsamples, unsigned options);
  virtual void mute();
  virtual long getLatency();
  virtual void setInitialDelay(long numsamples)
    throw(std::bad_alloc);
  void setFragmentSize(long size);
  long getFragmentSize();

 protected:  
  virtual void allocSwap(long numsaples) throw(std::bad_alloc);
  virtual void freeSwap();
  void freeFragments();
  long fifoSize, fragmentSize;
  _FV3_(slot) fifoSlot, reverseSlot, ifftSlot, swapSlot, restSlot;
  std::vector<_FV3_(frag)*> fragments;
  _FV3_(fragfft) fragmentsFFT;
  _FV3_(blockDelay) blkdelayL, blkdelayR;
  _FV3_(delay) delayL, delayR, delayWL, delayWR;
  
 private:
  _FV3_(irmodel2)(const _FV3_(irmodel2)& x);
  _FV3_(irmodel2)& operator=(const _FV3_(irmodel2)& x);
  void processSquare(_fv3_float_t *inputL, _fv3_float_t *inputR,
		     _fv3_float_t *outputL, _fv3_float_t *outputR,
		     unsigned options);
};
