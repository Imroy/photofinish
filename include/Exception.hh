/*
	Copyright 2014-2019 Ian Tester

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
#pragma once

#include <string>
#include <exception>
#include <string.h>

namespace PhotoFinish {

  inline const char* new_c_str(const std::string& s) {
    auto len = s.length() + 1;
    char *ret = new char[len];
    strncpy(ret, s.c_str(), len);
    return ret;
  }

  //! Uninitialised attribute exception
  class Uninitialised : public std::exception {
  protected:
    const std::string _class, _attribute;

  public:
    //! Constructor
    /*!
      \param c Class name
      \param a Attribute name
    */
    Uninitialised(const std::string& c, const std::string& a) :
      _class(c), _attribute(a)
    {}

    //! Constructor
    /*!
      \param c Class name
    */
    Uninitialised(const std::string& c) :
      _class(c)
    {}

    virtual const char* what() const noexcept {
      std::string w = _class;
      if (_attribute.length() > 0)
	w += "::" + _attribute;
      w += " is uninitialised.";
      return new_c_str(w);
    }
  };

  //! Unimplemented method exception
  class Unimplemented : public std::exception {
  protected:
    const std::string _class, _method;

  public:
    //! Constructor
    /*!
      \param c Class name
      \param m Method name
    */
    Unimplemented(const std::string& c, const std::string& m) :
      _class(c), _method(m)
    {}

    virtual const char* what() const noexcept {
      return new_c_str(_class + "::" + _method + " is unimplemented.");
    }
  };

  //! No results exception
  class NoResults : public std::exception {
  protected:
    const std::string _class, _method;

  public:
    //! Constructor
    /*!
      \param c Class name
      \param m Method name
    */
    NoResults(const std::string& c, const std::string& m) :
      _class(c), _method(m)
    {}

    virtual const char* what() const noexcept {
      return new_c_str(_class + "::" + _method + " has no results.");
    }
  };

  //! No targets exception
  class NoTargets : public std::exception {
  protected:
    const std::string _destination;

  public:
    //! Constructor
    /*!
      \param d Name  of destination that has no targets
    */
    NoTargets(const std::string& d) :
      _destination(d)
    {}

    virtual const char* what() const noexcept {
      return new_c_str("Destination \"" + _destination + "\" has no targets.");
    }
  };

  //! Generic error message exception
  class ErrorMsg : public std::exception {
  protected:
    const std::string _msg;

  public:
    //! Constructor
    /*!
      \param m Error message
    */
    ErrorMsg(const std::string& m) :
      _msg(m)
    {}

    virtual const char* what() const noexcept = 0;
  };

  //! Memory allocation exception
  class MemAllocError : public ErrorMsg {
  public:
    //! Constructor
    /*!
      \param m Error message
    */
    MemAllocError(const std::string& m) :
      ErrorMsg(m)
    {}

    const char* what() const noexcept {
      return new_c_str(_msg);
    }
  };

  //! File error abstract base exception
  class FileError : public ErrorMsg {
  protected:
    const std::string _filepath;

  public:
    //! Constructor
    /*!
      \param fp File path
      \param m Error message
    */
    FileError(const std::string& fp, const std::string& m) :
      ErrorMsg(m), _filepath(fp)
    {}

    //! Constructor
    /*!
      \param fp File path
    */
    FileError(const std::string& fp) :
      ErrorMsg(""), _filepath(fp)
    {}

    virtual const char* what() const noexcept = 0;
  };

  //! Unknown file type exception
  class UnknownFileType : public FileError {
  public:
    //! Constructor
    /*!
      \param fp File path
      \param m Error message
    */
    UnknownFileType(const std::string& fp, const std::string& m) :
      FileError(fp, m)
    {}

    //! Constructor
    /*!
      \param fp File path
    */
    UnknownFileType(const std::string& fp) :
      FileError(fp)
    {}

    virtual const char* what() const noexcept {
      std::string w = "Could not determine type for \"" + _filepath + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return new_c_str(w);
    }
  };

  //! File open exception
  class FileOpenError : public FileError {
  public:
    //! Constructor
    /*!
      \param fp File path
      \param m Error message
    */
    FileOpenError(const std::string& fp, const std::string& m) :
      FileError(fp, m)
    {}

    //! Constructor
    /*!
      \param fp File path
    */
    FileOpenError(const std::string& fp) :
      FileError(fp)
    {}

    virtual const char* what() const noexcept {
      std::string w = "Could not open filepath \"" + _filepath + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return new_c_str(w);
    }
  };

  //! File content exception
  class FileContentError : public FileError {
  public:
    //! Constructor
    /*!
      \param fp File path
      \param m Error message
    */
    FileContentError(const std::string& fp, const std::string& m) :
      FileError(fp, m)
    {}

    //! Constructor
    /*!
      \param fp File path
    */
    FileContentError(const std::string& fp) :
      FileError(fp)
    {}

    virtual const char* what() const noexcept {
      std::string w = "Something is wrong with the contents of filepath \"" + _filepath + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return new_c_str(w);
    }
  };

  //! Destination exception
  class DestinationError : public ErrorMsg {
  private:
    const std::string _path, _value;

  public:
    //! Constructor
    /*!
      \param p Destination field "path"
      \param v Value that is wrong
    */
    DestinationError(const std::string& p, const std::string& v) :
      ErrorMsg(""), _path(p), _value(v)
    {}

    virtual const char* what() const noexcept {
      return new_c_str("Error with value of destination field \"" + _path + "\" (\"" + _value + "\").");
    }
  };

  //! Library exception
  class LibraryError : public ErrorMsg {
  private:
    const std::string _lib;

  public:
    //! Constructor
    /*!
      \param l Library name
      \param m Error message
    */
    LibraryError(const std::string& l, const std::string& m) :
      ErrorMsg(m), _lib(l)
    {}

    virtual const char* what() const noexcept {
      return new_c_str("Error in " + _lib + ": " + _msg + ".");
    }
  };

  class cmsTypeError : public ErrorMsg {
  private:
    const unsigned int _type;

  public:
    //! Constructor
    /*!
      \param m Message string.
      \param t LCMS2 type.
    */
    cmsTypeError(const std::string& m, const unsigned int& t) :
      ErrorMsg(m), _type(t)
    {}

    virtual const char* what() const noexcept {
      return new_c_str("Error with value of cmsType: " + _msg + ".");
    }
  };

  //! WebP exception
  class WebPError : public std::exception {
  private:
    int _code;

  public:
    //! Constructor
    /*!
      \param c Error code
    */
    WebPError(int c) :
      _code(c)
    {}

    virtual const char* what() const noexcept {
      switch (_code) {
      case 0:
	return "No error";

      case 1:
	return "Out of memory";

      case 2:
	return "Bitstream out of memory";

      case 3:
	return "NULL parameter";

      case 4:
	return "Invalid configuration";

      case 5:
	return "Bad dimension";

      case 6:
	return "Partition is larger than 512 KiB";

      case 7:
	return "Partition is larger than 16 MiB";

      case 8:
	return "Bad write";

      case 9:
	return "File is larger than 4 GiB";

      case 10:
	return "User abort";

      }
      return "Dunno";
    }
  };

}
