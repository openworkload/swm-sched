
#include "scanner.h"

#include "wm_io.h"

#if defined(WIN32)
#include <Windows.h>
#else
#include <sys/sysinfo.h>
#include <cstring>
#endif

namespace swm {

std::string Scanner::find_cpu_name() {
#if defined(WIN32)
  HKEY r_key;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                    0,
                    KEY_QUERY_VALUE,
                    &r_key) != ERROR_SUCCESS) {
    throw std::runtime_error("Scanner::find_cpu_name(): cannot open reg key");
  }
  else {
    DWORD size = 1024;
    std::string res(size, '\0');
    if (RegQueryValueExA(r_key,
                         "ProcessorNameString",
                         NULL,
                         NULL,
                         (LPBYTE)res.c_str(),
                         &size) != ERROR_SUCCESS) {
      throw std::runtime_error("Scanner::find_cpu_name(): unknown CPU model");
    }

    RegCloseKey(r_key);
    return res.substr(0, res.find('\0'));
  }
#else
  FILE *fp;
  char res[2048] = { 0 };
  fp = popen("/bin/cat /proc/cpuinfo | grep 'model name'", "r");
  char* fr = fgets(res, sizeof(res) - 1, fp);
  pclose(fp);
  
  if (fr == NULL) {
    throw new std::runtime_error("Scanner::find_cpu_name(): cannot read file");
  }
  
  if (!res[0]) {
    throw std::runtime_error("Scanner::find_cpu_name(): unknown CPU model");
  }
  char *mn = strchr(res, ':');
  if (mn && strlen(mn) > 2) {
    mn += 2;
    //while (mn[0] && mn[strlen(mn) - 1] == '\n')
      // FIXME the loop is endless!
      //mn[strlen(mn) - 1] = 0;
  }
  return mn;
#endif
}

size_t Scanner::find_cpu_cores() {
#if defined(WIN32)
  SYSTEM_INFO s_info;
  GetSystemInfo(&s_info);
  return s_info.dwNumberOfProcessors;
#else
  FILE *fp;
  char res[128] = { 0 };
  fp = popen("/bin/cat /proc/cpuinfo | grep -c '^processor'", "r");
  size_t fr = fread(res, 1, sizeof(res) - 1, fp);
  pclose(fp);
  
  if (fr <= 0) {
    throw std::runtime_error("Scanner::find_cpu_cores(): cannot read file");
  }

  if (!res[0]) {
    throw std::runtime_error("Scanner::find_cpu_cores(): unknown cores");
  }
  
  return std::max(1, atoi(res));
#endif
}

double Scanner::find_cpu_freq() {
#if defined(WIN32)
  HKEY r_key;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                    0,
                    KEY_QUERY_VALUE,
                    &r_key) != ERROR_SUCCESS) {
    throw std::runtime_error("Scanner::find_cpu_freq(): cannot open reg key");
  }
  else {
    DWORD res = 0;
    DWORD size = sizeof(DWORD);
    if (RegQueryValueExA(r_key,
                         "~MHz",
                         NULL,
                         NULL,
                         (LPBYTE)&res,
                         &size) != ERROR_SUCCESS) {
      throw std::runtime_error("Scanner::find_cpu_freq(): unknown frequency");
    }

    RegCloseKey(r_key);
    return res;
  }
#else
  FILE * fp;
  char res[2048] = { 0 };
  fp = popen("/bin/cat /proc/cpuinfo | grep 'cpu MHz'", "r");
  double mhz = 0;
  while (fgets(res, sizeof(res) - 1, fp)) {
    char *mh = strchr(res, ':');
    if (mh && strlen(mh) > 2) {
      mh += 2;
      double cand = atof(mh);
      if (cand > mhz) {
        mhz = cand;
      }
      else {
        throw std::runtime_error("Scanner::find_cpu_freq(): unknown frequency");
      }
    }
  }
  
  pclose(fp);
  return mhz;
#endif
}

bool Scanner::scan() {
  if (inited_) {
    return false;
  }

  std::string name;
  try {
    name = find_cpu_name();
  }
  catch (const std::exception &ex) { 
    name = "Unknown CPU";
    std::cerr << "Failed to determine CPU model: " << ex.what() << std::endl;
  }
  
  size_t cores;
  try {
    cores = find_cpu_cores();
  }
  catch (const std::exception &ex) {
    cores = 1;
    std::cerr << "Failed to determine count of CPU cores: " << ex.what() << std::endl;
  }
  
  double freq_mhz;
  try {
    freq_mhz = find_cpu_freq();
  }
  catch (const std::exception &ex) {
    freq_mhz = 1000;
    std::cerr << "Failed to determine CPU frequency: " << ex.what() << std::endl;
  }
  
  cpu_.reset(new swm::ComputeUnit(swm::ComputeUnit::Type::Cpu, 1, name, cores, freq_mhz));
  inited_ = true;
  return true;
}

} // swm
