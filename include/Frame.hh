#ifndef __FRAME_HH__
#define __FRAME_HH__

#include "Destination_items.hh"
#include "Filter.hh"

class Frame : public D_target {
private:
  // width and height attributes inherited from D_target are the final size of the image
  double _crop_x, _crop_y, _crop_w, _crop_h;	// coordinates for cropping from the original image
  double _resolution;				// PPI

public:
  Frame(const D_target& target, double x, double y, double w, double h, double r);
  ~Frame();

  // Resize the image using a Lanczos filter
  Image* crop_resize(Image* img, _Filter* filter);

  inline double crop_x(void) const { return _crop_x; }
  inline double crop_y(void) const { return _crop_y; }
  inline double crop_w(void) const { return _crop_w; }
  inline double crop_h(void) const { return _crop_h; }
  inline double resolution(void) const { return _resolution; }
};

#endif /* __FRAME_HH__ */
