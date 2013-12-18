/**
 * Gardner Large Room Reverberator DelayLine Class
 */
class _FV3_(dl_gd_largeroom) : public _FV3_(delayline)
{
 public:
  _FV3_(dl_gd_largeroom)() throw(std::bad_alloc);
  virtual void setSampleRate(_fv3_float_t fs) throw(std::bad_alloc);
  virtual void mute();
  virtual _fv3_float_t process(_fv3_float_t input);

  void setDCC(_fv3_float_t fc){dccut.setCutOnFreq(fc, currentfs);}
  void setLPF(_fv3_float_t fc){lpf_loop.setLPF_BW(fc, currentfs);}
  void setDecay(_fv3_float_t value){decay = value;}
  
 protected:
  _FV3_(iir_1st) lpf_loop; _FV3_(dccut) dccut;
  _fv3_float_t decay; long pbidx[10][2];
};

/**
 * Gardner Large Room Reverberator Implementation
 */
class _FV3_(gd_largeroom) : public _FV3_(revbase)
{
 public:
  _FV3_(gd_largeroom)() throw(std::bad_alloc);

  virtual void mute(){ DL_Left.mute(); DL_Right.mute(); }
  virtual void processreplace(_fv3_float_t *inputL, _fv3_float_t *inputR, _fv3_float_t *outputL, _fv3_float_t *outputR, long numsamples)
    throw(std::bad_alloc);
  
  void setroomsize(_fv3_float_t value){ DL_Left.setDecay(value); DL_Right.setDecay(value); }
  void setdccutfreq(_fv3_float_t value){ DL_Left.setDCC(value); DL_Right.setDCC(value); }
  void setdamp(_fv3_float_t value){ DL_Left.setLPF(value); DL_Right.setLPF(value); }
  void setLRDiffFactor(_fv3_float_t factor){ lrfactor = factor; }
  
 protected:
  _FV3_(gd_largeroom)(const _FV3_(gd_largeroom)& x);
  _FV3_(gd_largeroom)& operator=(const _FV3_(gd_largeroom)& x);
  virtual void setFsFactors();
  _fv3_float_t lrfactor;

  _FV3_(dl_gd_largeroom) DL_Left, DL_Right;
};
