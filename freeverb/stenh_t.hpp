class _FV3_(stenh)
{
 public:
  _FV3_(stenh)() throw(std::bad_alloc);
  void         setSampleRate(_fv3_float_t fs);
  _fv3_float_t getSampleRate();
  void mute();
  void processreplace(_fv3_float_t *inputL, _fv3_float_t *inputR, _fv3_float_t *outputL, _fv3_float_t *outputR, long numsamples);
  void setChValL(_fv3_float_t value);
  void setChValR(_fv3_float_t value);
  _fv3_float_t getChValL();
  _fv3_float_t getChValR();

  void setBPF_LPF(_fv3_float_t value);
  void setBPF_HPF(_fv3_float_t value);
  void setBRF_LPF(_fv3_float_t value);
  void setBRF_HPF(_fv3_float_t value);
  void setBPFDepth(_fv3_float_t msec);
  void setBRFDepth(_fv3_float_t msec);
  _fv3_float_t getBPF_LPF();
  _fv3_float_t getBPF_HPF();
  _fv3_float_t getBRF_LPF();
  _fv3_float_t getBRF_HPF();
  _fv3_float_t getBPFDepth();
  _fv3_float_t getBRFDepth();
  
  void setOverallDepth(_fv3_float_t msec);
  void setDiffusion(_fv3_float_t value);
  void setWidth(_fv3_float_t value);
  void setDry(_fv3_float_t value);
  _fv3_float_t getOverallDepth();
  _fv3_float_t getDiffusion();
  _fv3_float_t getWidth();
  _fv3_float_t getDry();
  
  void setThreshold(_fv3_float_t dB);
  _fv3_float_t getThreshold();

  void setRMS(_fv3_float_t msec);
  void setAttack(_fv3_float_t msec);
  void setRelease(_fv3_float_t msec);
  void setRatio(_fv3_float_t value);
  void setSoftKnee(_fv3_float_t dB);
  _fv3_float_t getRMS();
  _fv3_float_t getAttack();
  _fv3_float_t getRelease();
  _fv3_float_t getRatio();
  _fv3_float_t getSoftKnee();

  long getLatency();

 private:
  _FV3_(stenh)(const _FV3_(stenh)& x);
  _FV3_(stenh)& operator=(const _FV3_(stenh)& x);

  void updateNRT();
  void update();
  
  _fv3_float_t currentfs;
  _fv3_float_t RMS, Attack, Release, Ratio, SoftKnee;
  _fv3_float_t DepthBPF, DepthBRF, DepthOA, thres_dB;
  _fv3_float_t chvL, chvR, diffusion, width, dry;
  
  _FV3_(efilter) bpf, lpf, hpf;
  _FV3_(delay) delayBPF, delayBRF, delayOA;
  _FV3_(scomp) compS, compD;
};
