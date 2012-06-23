#include "Image.hh"
#include "ImageFile.hh"
#include <stdio.h>

int main(int argc, char* argv[]) {
  PNGFile infile("test3.png");
  Image *image1 = infile.read();

  Image *image2 = image1->resize(800, -1, 3);
  delete image1;

  PNGFile outfile("test3.scaled.png");
  outfile.set_bit_depth(8);
  outfile.write(image2);
  delete image2;

  return 0;
}
