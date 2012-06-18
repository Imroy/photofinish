#include "Image.hh"

int main(int argc, char* argv[]) {
  Image<unsigned char> image1(1024, 768, NULL, TYPE_RGBA_16);
  Image<unsigned char> image2("test.png");

  image1.transform_from<unsigned char>(image2, INTENT_PERCEPTUAL, 0);

  return 0;
}
