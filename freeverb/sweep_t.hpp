class _FV3_(sweep)
{
 public:
  _FV3_(sweep)();
  virtual void         setSampleRate(_fv3_float_t fs);
  virtual _fv3_float_t getSampleRate();
  void setSweepMode(long mode);
  long getSweepMode();
  void setStartFs(_fv3_float_t fs);
  void setEndFs(_fv3_float_t fs);
  void setInitialMuteLength(_fv3_float_t ms){initialmutelength = ms;};
  void setLeadInLength(_fv3_float_t ms){leadinlength = ms;};
  void setSweepLength(_fv3_float_t ms){sweeplength = ms;};
  void setLeadOutLength(_fv3_float_t ms){leadoutlength = ms;};
  void setEndMuteLength(_fv3_float_t ms){endmutelength = ms;};
  long getTotalLength();
  void setLeadWindowType(long WINDOW);
  void setInverseMode(bool mode){inverseMode = mode;};
  void init();

  _fv3_float_t forward_sweep(_fv3_float_t input);
  _fv3_float_t inverse_sweep(_fv3_float_t input);
  _fv3_float_t process(_fv3_float_t input);

 protected:
  _FV3_(sweep)(const _FV3_(sweep)& x);
  _FV3_(sweep)& operator=(const _FV3_(sweep)& x);

  _fv3_float_t forward_sweep_explin(_fv3_float_t input);
  _fv3_float_t inverse_sweep_explin(_fv3_float_t input);

  _fv3_float_t currentfs, startfs, endfs,
    initialmutelength, leadinlength, sweeplength, leadoutlength, endmutelength, // ms
    initialMuteLength, leadInLength, sweepLength, leadOutLength, endMuteLength; // samples
  _fv3_float_t S1, S2, WC1In, WC2In, WC1Out, WC2Out, DecayTime, L1, L2;
  long countf, countr;
  bool inverseMode;
  long sweepMode;
};
