class _FV3_(irmodel2zl) : public _FV3_(irmodel2)
{
 public:
  _FV3_(irmodel2zl)();
  virtual _FV3_(~irmodel2zl)();
  virtual void processreplace(_fv3_float_t *inputL, _fv3_float_t *inputR,
			      _fv3_float_t *outputL, _fv3_float_t *outputR,
			      long numsamples, unsigned options);
  virtual void mute();
  virtual void setInitialDelay(long numsamples)
    throw(std::bad_alloc);
  virtual long getLatency();
  
 protected:
  virtual void allocSwap(long numsaples) throw(std::bad_alloc);
  virtual void freeSwap();
  long ZLstart;
  _FV3_(slot) zlFrameSlot, zlOnlySlot;
  _FV3_(delay) ZLdelayL, ZLdelayR, ZLdelayWL, ZLdelayWR;
  
 private:
  _FV3_(irmodel2zl)(const _FV3_(irmodel2zl)& x);
  _FV3_(irmodel2zl)& operator=(const _FV3_(irmodel2zl)& x);
  void processZL(_fv3_float_t *inputL, _fv3_float_t *inputR,
		 _fv3_float_t *outputL, _fv3_float_t *outputR,
		 long numsamples, unsigned options);
};
