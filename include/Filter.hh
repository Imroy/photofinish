#ifndef __FILTER_HH__
#define __FILTER_HH__

#include "Destination_items.hh"
#include "Exception.hh"

namespace PhotoFinish {

  class _Filter {
  protected:
    double _radius;

  public:
    _Filter(const D_resize& dr) :
      _radius(dr.has_support() ? dr.support() : 3)
    {}
    virtual ~_Filter()
    {}

    inline double radius(void) const { return _radius; }

    virtual SAMPLE eval(double x) const = 0;
  };

  class Lanczos : public _Filter {
  private:
    double _r_radius;	// Reciprocal of the radius

  public:
    Lanczos(const D_resize& dr) :
      _Filter(dr),
      _r_radius(1.0 / _radius)
    {}
    ~Lanczos()
    {}

    SAMPLE eval(double x) const;
  };

  // Factory function
  _Filter* Filter(const D_resize& resize) throw(DestinationError);

}

#endif /* __FILTER_HH__ */
