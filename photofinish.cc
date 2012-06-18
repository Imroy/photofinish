#include "Image.hh"

int main(int argc, char* argv[]) {
  Image image1("/tmp/test.png");
  //  Image<unsigned char> image1(1024, 768, NULL, TYPE_RGBA_16);
  //  Image inter1(image1);

  //  inter1.transform_from<unsigned short int>(image1, INTENT_PERCEPTUAL, 0);

  return 0;
}
