#ifndef __EXCEPTION_HH__
#define __EXCEPTION_HH__

#include <exception>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace PhotoFinish {
  class Uninitialised : public std::exception {
  protected:
    const std::string _class, _attribute;

  public:
    Uninitialised(const std::string& c, const std::string& a) :
      _class(c), _attribute(a)
    {}

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

  class Unimplemented : public std::exception {
  protected:
    const std::string _class, _method;

  public:
    Unimplemented(const std::string& c, const std::string& m) :
      _class(c), _method(m)
    {}

    virtual const char* what() const throw() {
      return (_class + "::" + _method + " is unimplemented.").c_str();
    }
  };

  class NoResults : public std::exception {
  protected:
    const std::string _class, _method;

  public:
    NoResults(const std::string& c, const std::string& m) :
      _class(c), _method(m)
    {}

    virtual const char* what() const throw() {
      return (_class + "::" + _method + " has no results.").c_str();
    }
  };

  class NoTargets : public std::exception {
  protected:
    const std::string _destination;

  public:
    NoTargets(const std::string& d) :
      _destination(d)
    {}

    virtual const char* what() const throw() {
      return ("Destination \"" + _destination + "\" has no targets.").c_str();
    }
  };

  class ErrorMsg : public std::exception {
  protected:
    const std::string _msg;

  public:
    ErrorMsg(const std::string& m) :
      _msg(m)
    {}

    virtual const char* what() const throw() = 0;
  };

  class MemAllocError : public ErrorMsg {
  public:
    MemAllocError(const std::string& m) :
      ErrorMsg(m)
    {}

    const char* what() const throw() {
      return _msg.c_str();
    }
  };

  class FileError : public ErrorMsg {
  protected:
    const fs::path _filepath;

  public:
    FileError(const fs::path& fp, const std::string& m) :
      ErrorMsg(m), _filepath(fp)
    {}

    FileError(const fs::path& fp) :
      ErrorMsg(""), _filepath(fp)
    {}

    virtual const char* what() const throw() = 0;
  };

  class UnknownFileType : public FileError {
  public:
    UnknownFileType(const fs::path& fp, const std::string& m) :
      FileError(fp, m)
    {}

    UnknownFileType(const fs::path& fp) :
      FileError(fp)
    {}

    virtual const char* what() const throw() {
      std::string w = "Could not determine type for filepath \"" + _filepath.string() + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return w.c_str();
    }
  };

  class FileOpenError : public FileError {
  public:
    FileOpenError(const fs::path& fp, const std::string& m) :
      FileError(fp, m)
    {}

    FileOpenError(const fs::path& fp) :
      FileError(fp)
    {}

    virtual const char* what() const throw() {
      std::string w = "Could not open filepath \"" + _filepath.string() + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return w.c_str();
    }
  };

  class FileContentError : public FileError {
  public:
    FileContentError(const fs::path& fp, const std::string& m) :
      FileError(fp, m)
    {}

    FileContentError(const fs::path& fp) :
      FileError(fp)
    {}

    virtual const char* what() const throw() {
      std::string w = "Something is wrong with the contents of filepath \"" + _filepath.string() + "\"";
      if (_msg.length() > 0)
	w += ": " + _msg;
      w += ".";
      return w.c_str();
    }
  };

  class DestinationError : public ErrorMsg {
  private:
    const std::string _path, _value;

  public:
    DestinationError(const std::string& p, const std::string& v) :
      ErrorMsg(""), _path(p), _value(v)
    {}

    virtual const char* what() const throw() {
      return ("Error with value of destination field \"" + _path + "\" (\"" + _value + "\").").c_str();
    }
  };

  class LibraryError : public ErrorMsg {
  private:
    const std::string _lib;

  public:
    LibraryError(const std::string& l, const std::string& m) :
      ErrorMsg(m), _lib(l)
    {}

    virtual const char* what() const throw() {
      return ("Error in " + _lib + ": " + _msg + ".").c_str();
    }
  };
}

#endif /* __EXCEPTION_HH__ */
