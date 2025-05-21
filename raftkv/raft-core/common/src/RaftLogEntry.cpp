#include "RaftLogEntry.h"

namespace WW
{

RaftLogEntry::RaftLogEntry(LogIndex _Index, TermId _Term, const std::string & _Command)
    : _Index(_Index)
    , _Term(_Term)
    , _Command(_Command)
{
}

LogIndex RaftLogEntry::getIndex() const
{
    return _Index;
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
