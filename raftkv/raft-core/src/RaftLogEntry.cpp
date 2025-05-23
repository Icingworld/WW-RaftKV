#include "RaftLogEntry.h"

namespace WW
{

RaftLogEntry::RaftLogEntry(TermId _Term, const std::string & _Command)
    : _Term(_Term)
    , _Command(_Command)
{
}

TermId RaftLogEntry::getTerm() const
{
    return _Term;
}

const std::string & RaftLogEntry::getCommand() const
{
    return _Command;
}

} // namespace WW
