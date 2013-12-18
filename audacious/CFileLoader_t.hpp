class _FV3_(CFileLoader)
{
 public:
  _FV3_(CFileLoader)(){ rawStream = srcStream = NULL; enableAHDSR = false; }
  virtual _FV3_(~CFileLoader)(){ dealloc(); }
  void unsetAHDSR(){ enableAHDSR = false; }
  void setAHDSR(_fv3_float_t _attack, _fv3_float_t _hold, _fv3_float_t _decay, _fv3_float_t _sustain, _fv3_float_t _release)
  {
    if(!(_attack == 0&&_release == 0&&((_decay == 0&&_hold == 1)||_sustain == 1)))
      {
	enableAHDSR = true;
	ta = _attack; th = _hold, td = _decay; ts = _sustain; tr = _release;
      }
  }
  // stretch x1=1.0f limit 100%=100
  int load(const char * filename, double fs, double stretch, double limit, long src_type)
  {
    if(limit <= 0) limit = 100;
    SndfileHandle sndHandle(filename);
    if(sndHandle.frames() <= 0) return -1;
    //double second = (double)sndHandle.frames()/(double)sndHandle.samplerate();
    double ratio = (double)stretch*(double)fs/(double)sndHandle.samplerate();
    dealloc();
    try
      {
	rawStream = new _fv3_float_t[((int)sndHandle.frames())*sndHandle.channels()];
	srcStream = new _fv3_float_t[sndHandle.frames()*sndHandle.channels()*((int)ratio+1)];
      }
    catch(std::bad_alloc){ dealloc(); return -2; }
    sf_count_t rcount = sndHandle.readf(rawStream, sndHandle.frames());
    if(rcount != sndHandle.frames()){ dealloc(); return -3; }
    int srcOutputFrames;
    _fv3_float_t * irsStream = NULL;
    if(fs != sndHandle.samplerate()||stretch != 1.0f)
      {
	int srcerr;
	_FV3_(SRC_DATA) src_data;
	src_data.src_ratio = ratio;
	src_data.data_in = rawStream;
	src_data.input_frames = sndHandle.frames();
	src_data.data_out = srcStream;
	src_data.output_frames = src_data.input_frames*((int)ratio+1);
	src_data.end_of_input = 1;
	srcerr = _FV3_(src_simple)(&src_data, src_type, sndHandle.channels());
	if(srcerr != 0){ dealloc(); errstring = src_strerror(srcerr); return -4; }
	srcOutputFrames = src_data.output_frames_gen;
	irsStream = srcStream;
      }
    else
      {
	srcOutputFrames = sndHandle.frames();
	irsStream = rawStream;
      }
    long copyFrames = (long)((double)srcOutputFrames*limit/100.0f);
    try
      {
	out.alloc(copyFrames, sndHandle.channels());
      }
    catch(std::bad_alloc){ dealloc(); return -5; }
    splitChannelsS(sndHandle.channels(), copyFrames, irsStream, out.getArray());
    dealloc();
    if(enableAHDSR)
      {
	cahdsr.setRAHDSR(copyFrames,ta,th,td,ts,tr);
	for(long i = 0;i < out.getch();i ++)
	  {
	    cahdsr.init();
	    for(long j = 0;j < out.getsize();j ++)
	      {
		out.c(i)[j] *= cahdsr.process(1);
	      }
	  }
      }
    return 0;
  }
  const char * errstr(){ return errstring.c_str(); };
  _FV3_(slot) out;
 private:
  void dealloc()
    {
      delete[] rawStream;
      delete[] srcStream;
      rawStream = srcStream = NULL;
    }
  _fv3_float_t *rawStream, *srcStream;
  bool enableAHDSR;
  _FV3_(ahdsr) cahdsr;
  _fv3_float_t ta, th, td, ts, tr;
  std::string errstring;
};
