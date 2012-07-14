/*
	Copyright 2012 Ian Tester

	This file is part of Photo Finish.

	Photo Finish is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Photo Finish is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Photo Finish.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __DEFINABLE_HH__
#define __DEFINABLE_HH__

#include <ostream>
#include <string>
#include "yaml-cpp/yaml.h"

namespace PhotoFinish {

  //! Template class for storing things that can be defined or undefined
  template <typename T>
  class definable {
  private:
    bool _defined;
    T _item;

  public:
    //! Empty constructor
    //! This sets the object to undefined and the item is initialised with its empty constructor
    definable() :
      _defined(false),
      _item()
    {}

    //! Construct from an item
    //! This obvious also sets the object to 'defined'
    definable(const T &i) :
      _defined(true),
      _item(i)
    {}

    //! Is this object defined?
    inline const bool defined(void) const { return _defined; }

    //    inline friend bool defined(const definable<T>& data) const { return data._defined; }

    //! Set this object as 'defined' (or not)
    inline void set_defined(bool v = true) { _defined = v; }

    //! Undefine the object
    inline void undefine(void) { _defined = false; }

    //! Get the item
    inline T get(void) { return _item; }
    //! Get the item, const version
    inline const T& get(void) const { return _item; }

    //! Cast to the contained type.
    inline operator T(void) const { return _item; }

    //! Arrow operator
    inline T* operator ->() { return &_item; }
    //! Arrow operator, const version
    inline const T* operator ->() const { return &_item; }

    //! Assignment operator
    inline definable<T>& operator =(const T &i) {
      _defined = true;
      _item = i;
      return *this;
    }

    //! Allow the contained data to be output to an ostream.
    //! Outputs "[undefined]" if the value is not defined.
    inline friend std::ostream& operator << (std::ostream& out, definable<T>& data) {
      if (data._defined)
	out << data._item;
      else
	out << std::string("[undefined]");
      return out;
    }

    //! Allow the contained data to be read from a YAML document
    inline friend void operator >> (const YAML::Node& node, definable<T>& data) {
      node >> data._item;
      data._defined = true;
    }

  };

}

#endif /* __DEFINABLE_HH__ */
