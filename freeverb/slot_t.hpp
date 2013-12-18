class _FV3_(slot)
{
 public:
  _FV3_(slot)();
  virtual _FV3_(~slot)();
  void alloc(long nsize, long nch)
    throw(std::bad_alloc);
  _fv3_float_t * c(long nch);
  void free();
  void mute();
  void mute(long limit);
  void mute(long offset, long limit);
  long getsize(){ return size; }
  long getch(){ return ch; }
  _fv3_float_t ** getArray();
  _fv3_float_t *L, *R;

 private:
  _FV3_(slot)(const _FV3_(slot)& x);
  _FV3_(slot)& operator=(const _FV3_(slot)& x);
  long size, ch;
  _fv3_float_t ** data;
};
