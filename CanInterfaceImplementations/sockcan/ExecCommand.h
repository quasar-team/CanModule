/*
 * ExecCommand.h
 *
 *  Created on: Mar 9, 2020
 *      Author: mludwig
 * copied from
 * https://www.linuxquestions.org/questions/programming-9/how-to-get-info-from-system-calls-in-linux-using-c-677436/
 */

#ifndef SIMPLETEST6_CANMODULE_CANINTERFACEIMPLEMENTATIONS_SOCKCAN_EXECCOMMAND_H_
#define SIMPLETEST6_CANMODULE_CANINTERFACEIMPLEMENTATIONS_SOCKCAN_EXECCOMMAND_H_

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace execcommand_ns
{

  class ExecCommand
  {
  public:
    typedef std::vector<std::string> CmdResults;
    // hicpp-explicit-conversions
    explicit ExecCommand(const std::string &cmd)
    {
      FILE *pfd = popen(cmd.c_str(), "r");
      if (pfd == NULL)
      {
        std::ostringstream msg;
        msg << cmd << ": Command or process could not be executed.";
        throw std::runtime_error(msg.str());
      }

      while (!feof(pfd))
      {
        char buf[1024] = {0};

        if (fgets(buf, sizeof(buf), pfd) != NULL)
        {
          std::string str(buf);
          m_results.push_back(str.substr(0, str.length() - 1));
        }
      }
      pclose(pfd);
    }

    const CmdResults getResults() const { return m_results; }

    /// uuh oh, elegant !
    friend std::ostream &operator<<(std::ostream &os, const ExecCommand &exec)
    {
      std::for_each(exec.m_results.begin(), exec.m_results.end(), ExecCommand::Displayer(os));
      return os;
    }

  private:
    class Displayer
    {
    public:
      Displayer(std::ostream &os) : m_os(os) {}

      void operator()(const std::string &str)
      {
        m_os << str << std::endl;
      }

    private:
      std::ostream &m_os;
    };
    CmdResults m_results;
  };

} /* namespace execcommand_ns */

#endif /* SIMPLETEST6_CANMODULE_CANINTERFACEIMPLEMENTATIONS_SOCKCAN_EXECCOMMAND_H_ */
