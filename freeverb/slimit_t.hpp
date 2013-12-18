class _FV3_(slimit)
{
 public:
  _FV3_(slimit)();
  _FV3_(~slimit)();
  void setRMS(long value);
  long getRMS();
  void setLookahead(long value)
    throw(std::bad_alloc);
  long getLookahead();
  void setLookaheadRatio(_fv3_float_t value);
  _fv3_float_t getLookaheadRatio();  
  void setAttack(_fv3_float_t value);
  _fv3_float_t getAttack();
  void setRelease(_fv3_float_t value);
  _fv3_float_t getRelease();
  void setThreshold(_fv3_float_t value);
  _fv3_float_t getThreshold();
  void setCeiling(_fv3_float_t value);
  _fv3_float_t getCeiling();
  inline _fv3_float_t process(_fv3_float_t input)
  {
    _fv3_float_t rmsf = Rms.process(input);
    
    if(lookahead > 0)
      {
	for(long i = 0;i < bufsize;i ++)
	  buffer[i] += lookaheadDelta;
	buffer[bufidx] = rmsf-LookaheadRatio;
	rmsf = 0;
	for(long i = 0;i < bufsize;i ++)
	  if(rmsf < buffer[i]) rmsf = buffer[i];
	bufidx++;
	if(bufidx >= bufsize) bufidx = 0;
      }
    
    _fv3_float_t theta = rmsf > env ? attackDelta : releaseDelta;
    env = (1.0-theta)*rmsf + theta*env;
    UNDENORMAL(env);
    if(env < 0) env = 0;
    
    // gain reduction
    if(env >= Threshold)
      {
	_fv3_float_t log_env = std::log(env);
	return std::exp(R2-R1*C_T2/(log_env/R1+C_2T)-log_env);
      }
    return 1;
  }
  
  _fv3_float_t getEnv();
  void mute();

 private:
  _FV3_(slimit)(const _FV3_(slimit)& x);
  _FV3_(slimit)& operator=(const _FV3_(slimit)& x);

  void update();
  long lookahead, bufidx, bufsize;
  _fv3_float_t Lookahead, LookaheadRatio, Attack, Release;
  _fv3_float_t attackDelta, releaseDelta, lookaheadDelta;
  _fv3_float_t Threshold, Ceiling;
  _fv3_float_t env, R1, C_T2, C_2T, R2;
  _FV3_(rms) Rms;
  _fv3_float_t * buffer;
};
