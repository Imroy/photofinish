#ifndef __FILTER_HH__
#define __FILTER_HH__

#include <memory>
#include "Destination_items.hh"
#include "Exception.hh"

namespace PhotoFinish {

  class Base_Filter {
  protected:
    double _radius;

  public:
    Base_Filter() :
      _radius(3)
    {}
    Base_Filter(const D_resize& dr) :
      _radius(dr.has_support() ? dr.support() : 3)
    {}

    inline double radius(void) const { return _radius; }

    virtual SAMPLE eval(double x) const = 0;
  };

  class Lanczos : public Base_Filter {
  private:
    double _r_radius;	// Reciprocal of the radius

  public:
    Lanczos(const D_resize& dr) :
      Base_Filter(dr),
      _r_radius(1.0 / _radius)
    {}

    SAMPLE eval(double x) const;
  };

  // Factory/wrapper class
  class Filter : public Base_Filter {
  private:
    typedef std::shared_ptr<Base_Filter> ptr;

    ptr _filter;

  public:
    Filter();
    Filter(const D_resize& resize) throw(DestinationError);

    inline SAMPLE eval(double x) const {
      if (_filter == NULL)
	throw Uninitialised("Filter");
      return _filter->eval(x);
    }
  };

}

#endif /* __FILTER_HH__ */
