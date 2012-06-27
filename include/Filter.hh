#ifndef __FILTER_HH__
#define __FILTER_HH__

#define sqr(x) ((x) * (x))

class _Filter {
protected:
  double _radius;

public:
  _Filter(double r) :
    _radius(r)
  {}

  inline double radius(void) const { return _radius; }

  virtual SAMPLE eval(double x) = 0;
};

class Lanczos : public _Filter {
private:
  double _r_radius;	// Reciprocal of the radius

public:
  Lanczos(double r) :
    _Filter(r),
    _r_radius(1.0 / r)
  {}

  SAMPLE eval(double x);
};

#endif /* __FILTER_HH__ */
