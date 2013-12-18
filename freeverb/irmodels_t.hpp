class _FV3_(irmodels) : public _FV3_(irbase)
{
 public:
  _FV3_(irmodels)();
  virtual _FV3_(~irmodels)();
  virtual void loadImpulse(const _fv3_float_t * inputL, const _fv3_float_t * inputR, long size)
    throw(std::bad_alloc);
  virtual void unloadImpulse();
  virtual void processreplace(_fv3_float_t *inputL, _fv3_float_t *inputR,
			      _fv3_float_t *outputL, _fv3_float_t *outputR,
			      long numsamples, unsigned options);
  virtual void mute();

 private:
  _FV3_(irmodels)(const _FV3_(irmodels)& x);
  _FV3_(irmodels)& operator=(const _FV3_(irmodels)& x);
  void allocImpulse(long size)
    throw(std::bad_alloc);
  void freeImpulse();
  void update();
  _FV3_(slot) impulse, delay;
  long current;
};
