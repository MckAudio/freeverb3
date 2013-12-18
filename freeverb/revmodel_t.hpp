class _FV3_(revmodel) : public _FV3_(revbase)
{
public:
  _FV3_(revmodel)() throw(std::bad_alloc);
  virtual void mute();
  void processreplace(_fv3_float_t *inputL, _fv3_float_t *inputR, _fv3_float_t *outputL, _fv3_float_t *outputR, long numsamples)
    throw(std::bad_alloc);
  void setroomsize(_fv3_float_t value);
  _fv3_float_t	getroomsize();
  void setdamp(_fv3_float_t value);
  _fv3_float_t	getdamp();
  virtual void setwet(_fv3_float_t value);
  virtual _fv3_float_t	getwet();
  virtual void setdry(_fv3_float_t value);
  virtual _fv3_float_t	getdry();
  virtual void printconfig();

 private:
  _FV3_(revmodel)(const _FV3_(revmodel)& x);
  _FV3_(revmodel)& operator=(const _FV3_(revmodel)& x);
  virtual void setFsFactors();
  void setAllpassFeedback(_fv3_float_t fb);
  _fv3_float_t roomsize, damp;
  _FV3_(allpass) allpassL[FV3_FREV_NUM_ALLPASS], allpassR[FV3_FREV_NUM_ALLPASS];
  _FV3_(comb) combL[FV3_FREV_NUM_COMB], combR[FV3_FREV_NUM_COMB];
  const static long combCo[FV3_FREV_NUM_COMB], allpCo[FV3_FREV_NUM_ALLPASS];
};
