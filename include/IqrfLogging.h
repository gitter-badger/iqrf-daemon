#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>

//TODO more detailed comments ;-)

//initialize tracing
#define TRC_INIT(stream) \
namespace iqrf { \
  Tracer& Tracer::getTracer() { static Tracer tracer(stream); return tracer; }\
  static Tracer& iqrfTracer(Tracer::getTracer()); }


//convenient trace macros
#ifdef _DEBUG
#define TRC_ERR(trc)  TRC(iqrf::Level::err, trc)
#define TRC_WAR(trc)  TRC(iqrf::Level::war, trc)
#define TRC_INF(trc)  TRC(iqrf::Level::inf, trc)
#define TRC_DBG(trc)  TRC(iqrf::Level::dbg, trc)
#define TRC_ENTER(trc) TRC(iqrf::Level::dbg, "{ENTER} " << trc)
#define TRC_LEAVE(trc) TRC(iqrf::Level::dbg, "{LEAVE} " << trc)
#else
#define TRC_ERR(trc)  TRC(iqrf::Level::err, trc)
#define TRC_WAR(trc)  TRC(iqrf::Level::war, trc)
#define TRC_INF(trc)  TRC(iqrf::Level::inf, trc)
#define TRC_DBG(trc)
#define TRC_ENTER(trc)
#define TRC_LEAVE(trc)
#endif

#define PAR(par)                #par "=\"" << par << "\" "
#define PAR_HEX(par)            #par "=\"0x" << std::hex << par << std::dec << "\" "
#define NAME_PAR(name, par)     #name "=\"" << par << "\" "
#define NAME_PAR_HEX(name,par)  #name "=\"0x" << std::hex << par << std::dec << "\" "

// Inserts binary data to trace in the form:
// 16:  34 35 36 36 35 34 36 35 34 36 35 35 34 36 35 34    4566546546554654
// 32:  36 35 34 36 35 34 36 35 34 36 35 34 36 35 34 36    6546546546546546
// 48:  35 34 36 35 34 36 35 34 36 35 34 36 35 34 32 37    5465465465465427 <I/>
// It is developer's responsibility to passed proper len of "owned" data. Uprintable characters are replaced by '.' 
#define FORM_HEX(ptr, len)      iqrf::TracerHexString((unsigned char*)ptr, len)

//exceptions tracing
#define THROW_EX(extype, exmsg) { \
  std::ostringstream ostr; ostr << exmsg; \
  TRC_WAR("Throwing " << #extype << ": " << ostr.str()); \
  extype ex(ostr.str().c_str()); throw ex; }

#define CATCH_EX(msg, extype, ex) { \
  TRC_WAR("Caught " << msg << ": " << #extype << ": " << ex.what()); }

//auxiliary macro
#ifdef _DEBUG
#define FLF iqrf::TracerNiceFileName(__FILE__) << " ln:" << \
__LINE__ << iqrf::TracerNiceFuncName(__FUNCTION__)
#else
#define FLF iqrf::TracerNiceFuncName(__FUNCTION__)
#endif

#define TRC(level, msg) iqrf::Tracer::getTracer().getStream() << \
levelToChar(level) << FLF << std::endl << msg << std::endl;

namespace iqrf {

  enum Level {
    err,
    war,
    inf,
    dbg
  };

  class TracerNiceFileName {
  public:
    TracerNiceFileName(const std::string& fname)
      :file_name(fname)
    {}

    friend std::ostream& operator << (std::ostream& o, TracerNiceFileName& fn)
    {
      size_t found = fn.file_name.find_last_of("/\\");
      if (std::string::npos != found)
        fn.file_name = fn.file_name.substr(found + 1);

      o << " \"" << fn.file_name << "\"";
      return o;
    }
  private:
    std::string file_name;
  };

  //TODO check conversion of lambdas
  class TracerNiceFuncName {
  public:
    TracerNiceFuncName(const std::string& fname)
      :func_name(fname)
    {}

    friend std::ostream& operator << (std::ostream& o, TracerNiceFuncName& fn)
    {
      size_t end = fn.func_name.find('(');
      if (std::string::npos != end)
        fn.func_name = fn.func_name.substr(0, end);

      size_t beg = fn.func_name.rfind(' ');

      if (std::string::npos != beg) {
        beg++;
        fn.func_name = fn.func_name.substr(beg, fn.func_name.size() - 1);
      }

      o << " " << fn.func_name << "()";
      return o;
    }
  private:
    std::string func_name;
  };

  static const char* levelToChar(Level level)
  {
    switch (level) {
    case err:
      return "{ERR}";
    case war:
      return "{WAR}";
    case inf:
      return "{INF}";
    case dbg:
      return "{DBG}";
    default:
      return "{???}";
    }
  }

  class Tracer {
  public:
    //void trace(const std::string& trc) {
    //  std::lock_guard<std::recursive_mutex> guard(m_mtx);
    //  if (m_ofstream.is_open())
    //    m_ofstream << trc;
    //  else
    //    std::cout << trc;
    //}

    //friend std::ostream& operator << (std::ostream& o, Tracer& t) {
    //  std::lock_guard<std::recursive_mutex> guard(t.m_mtx);
    //  if (t.m_ofstream.is_open())
    //    return t.m_ofstream;
    //  else
    //    return std::cout;
    //}

    static Tracer& getTracer();
    std::ostream& getStream() {
      if (m_ofstream.is_open())
        return m_ofstream;
      else
        return std::cout;
    }

  private:
    Tracer(const std::string& fname)
      :m_fname(fname)
    {
      if (!m_fname.empty()) {
        m_ofstream.open(m_fname);
        if (!m_ofstream.is_open())
          std::cerr << std::endl << "Cannot open: " << PAR(m_fname) << std::endl;
      }
    }

    Tracer & operator = (const Tracer& t);
    Tracer(const Tracer& t);

    std::recursive_mutex m_mtx;
    std::string m_fname;
    std::ofstream m_ofstream;
  };

  class TracerHexString
  {
  public:
    TracerHexString(const void* buf, const long len, bool plain = false) {
      os.setf(std::ios::hex, std::ios::basefield); os.fill('0');

      int i = 1;
      for (; i < len + 1; i++) {
        unsigned char c = 0xff & ((const unsigned char*)buf)[i - 1];
        //bin output
        os << std::setw(2) << (int)c << ' ';
        oschar.put(isgraph(c) ? (char)c : '.');
        if (!(i % 16)) {
          if (!plain) {
            //char output
            os << oschar.str();
            oschar.seekp(0);
          }
          os << std::endl;
        }
      }
      //align last line
      i--;
      if (i % 16) {
        for (; i % 16; i++) {
          os << "   ";
          oschar << ' ';
        }
        if (!plain) {
          os << oschar.str();
        }
        os << std::endl;
      }
    }

    virtual ~TracerHexString() {}

  private:
    TracerHexString();

    std::ostringstream os;
    std::ostringstream oschar;

    friend std::ostream& operator<<(std::ostream& out, const TracerHexString& hex) {
      out << hex.os.str();
      return out;
    }
  };

} //namespace iqrf
