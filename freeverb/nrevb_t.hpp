class _FV3_(nrevb) : public _FV3_(nrev)
{
 public:
  _FV3_(nrevb)() throw(std::bad_alloc);
  virtual void mute();
  virtual void setdamp(_fv3_float_t value);
  virtual void setfeedback(_fv3_float_t value);
  void setapfeedback(_fv3_float_t value){apfeedback = value;}
  _fv3_float_t getapfeedback(){return apfeedback;}
  
 protected:
  virtual void processloop2(long count, _fv3_float_t *inputL, _fv3_float_t *inputR, _fv3_float_t *outputL, _fv3_float_t *outputR);
  virtual void processloop4(long count, _fv3_float_t *inputL, _fv3_float_t *inputR, _fv3_float_t *outputL, _fv3_float_t *outputR,
			    _fv3_float_t *outRearL, _fv3_float_t *outRearR);
  virtual void setFsFactors();
  virtual void setcombfeedback(_fv3_float_t back, long zero);
  
  _fv3_float_t feedback2, apfeedback;
  // work values
  _fv3_float_t lastL, lastR;
  _FV3_(allpass) allpass2L[FV3_NREVB_NUM_ALLPASS_2], allpass2R[FV3_NREVB_NUM_ALLPASS_2];
  _FV3_(comb) comb2L[FV3_NREVB_NUM_COMB_2], comb2R[FV3_NREVB_NUM_COMB_2];
  const static long combCo2[FV3_NREVB_NUM_COMB_2], allpassCo2[FV3_NREVB_NUM_ALLPASS_2];
    
 private:
  _FV3_(nrevb)(const _FV3_(nrevb)& x);
  _FV3_(nrevb)& operator=(const _FV3_(nrevb)& x);
};
