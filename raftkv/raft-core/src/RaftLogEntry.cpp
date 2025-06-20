#include "RaftLogEntry.h"

namespace WW
{

RaftLogEntry::RaftLogEntry(TermId _Term, const std::string & _Command, const std::string & _UUID, SequenceType _Sequence_id)
    : _UUID(_UUID)
    , _Sequence_id(_Sequence_id)
    , _Term(_Term)
    , _Command(_Command)
{
}

RaftLogEntry::RaftLogEntry(TermId _Term, std::string && _Command, const std::string & _UUID, SequenceType _Sequence_id)
    : _UUID(_UUID)
    , _Sequence_id(_Sequence_id)
    , _Term(_Term)
    , _Command(std::move(_Command))
{
}

const std::string & RaftLogEntry::getUUID() const
{
    return _UUID;
}

SequenceType RaftLogEntry::getSequenceID() const
{
    return _Sequence_id;
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
