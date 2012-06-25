#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
#include "yaml-cpp/yaml.h"
#include "Image.hh"
#include "ImageFile.hh"
#include "Destination.hh"

int main(int argc, char* argv[]) {
  std::ifstream fin("destinations.yml");
  YAML::Parser parser(fin);
  YAML::Node doc;
  parser.GetNextDocument(doc);

  map<std::string, Destination*> destinations;
  for(YAML::Iterator it = doc.begin(); it != doc.end(); it++) {
    std::string destname;
    it.first() >> destname;
    fprintf(stderr, "Destination \"%s\".\n", destname.c_str());
    Destination *destination = new Destination;
    it.second() >> *destination;
    destinations[destname] = destination;
  }

  PNGFile infile("test3.png");
  Image *image1 = infile.read();

  _ImageFile *outfile = ImageFile("test3.scaled.jpeg");
  if (outfile != NULL) {
    Image *image2 = image1->resize(1200, -1, 3);
    delete image1;

    outfile->write(image2, *destinations["wall2"]);
    delete image2;
    delete outfile;
  } else
    delete image1;

  for (map<string, Destination*>::iterator di = destinations.begin(); di != destinations.end(); di++)
    delete di->second;

  return 0;
}
