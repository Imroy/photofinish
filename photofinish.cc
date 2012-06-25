#include "Image.hh"
#include "ImageFile.hh"
#include <stdio.h>

int main(int argc, char* argv[]) {
  PNGFile infile("test3.png");
  Image *image1 = infile.read();

  _ImageFile *outfile = ImageFile("test3.scaled.jpeg");
  if (outfile != NULL) {
    Image *image2 = image1->resize(1200, -1, 3);
    delete image1;

    outfile->set_bit_depth(8);
    outfile->write(image2);
    delete image2;
    delete outfile;
  } else
    delete image1;

  return 0;
}
