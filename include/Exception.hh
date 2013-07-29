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
#ifndef __EXCEPTION_HH__
#define __EXCEPTION_HH__

#include <string>
#include <exception>

namespace PhotoFinish {
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

    virtual const char* what() const throw() {
      std::string w = _class;
      if (_attribute.length() > 0)
	w += "::" + _attribute;
      return (w + " is uninitialised.").c_str();
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

    virtual const char* what() const throw() {
      return (_class + "::" + _method + " is unimplemented.").c_str();
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

    virtual const char* what() const throw() {
      return (_class + "::" + _method + " has no results.").c_str();
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

    virtual const char* what() const throw() {
      return ("Destination \"" + _destination + "\" has no targets.").c_str();
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

    virtual const char* what() const throw() = 0;
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

    const char* what() const throw() {
      return _msg.c_str();
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

    virtual const char* what() const throw() = 0;
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

    virtual const char* what() const throw() {
      std::string w = "Could not determine type for \"" + _filepath + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return w.c_str();
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

    virtual const char* what() const throw() {
      std::string w = "Could not open filepath \"" + _filepath + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return w.c_str();
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

    virtual const char* what() const throw() {
      std::string w = "Something is wrong with the contents of filepath \"" + _filepath + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return w.c_str();
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

    virtual const char* what() const throw() {
      return ("Error with value of destination field \"" + _path + "\" (\"" + _value + "\").").c_str();
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

    virtual const char* what() const throw() {
      return ("Error in " + _lib + ": " + _msg + ".").c_str();
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

    virtual const char* what() const throw() {
      return ("Error with value of cmsType: " + _msg + ".").c_str();
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

    virtual const char* what() const throw() {
      switch (_code) {
      case 0:
	return std::string("No error").c_str();

      case 1:
	return std::string("Out of memory").c_str();

      case 2:
	return std::string("Bitstream out of memory").c_str();

      case 3:
	return std::string("NULL parameter").c_str();

      case 4:
	return std::string("Invalid configuration").c_str();

      case 5:
	return std::string("Bad dimension").c_str();

      case 6:
	return std::string("Partition is larger than 512 KiB").c_str();

      case 7:
	return std::string("Partition is larger than 16 MiB").c_str();

      case 8:
	return std::string("Bad write").c_str();

      case 9:
	return std::string("File is larger than 4 GiB").c_str();

      case 10:
	return std::string("User abort").c_str();

      }
      return std::string("Dunno").c_str();
    }
  };

}

#endif /* __EXCEPTION_HH__ */
