#include "Image.hh"
#include <stdio.h>

int main(int argc, char* argv[]) {
  Image image1("test3.png");

  /*
  unsigned int w, h;
  w = image1.width();
  h = image1.height();
  for (unsigned int y = 0; y < h; y++)
    for (unsigned int x = 0; x < w; x++) {
      double *p = image1.at(x, y);
      fprintf(stderr, "at(%d, %d) = (%f, %f, %f)\n", x, y, p[0], p[1], p[2]);
    }
  */

  //  image1.write_png("test3.out.png", 8, NULL, INTENT_PERCEPTUAL);

  Image image2 = image1.resize(800, -1, 3);
  image2.write_png("test3.scaled.png", 8, NULL, INTENT_PERCEPTUAL);

  return 0;
}
