#include "RaftLog.h"

#include <stdexcept>

namespace WW
{

RaftLog::RaftLog()
    : _Logs()
    , _Last_index(-1)
    , _Last_term(-1)
{
}

LogIndex RaftLog::getLastIndex() const
{
    return _Last_index;
}

TermId RaftLog::getLastTerm() const
{
    return _Last_term;
}

const RaftLogEntry & RaftLog::at(LogIndex _Index) const
{
    if (_Index > _Last_index) {
        throw std::out_of_range("Index out of range in RaftLog::at");
    }

    return _Logs.at(_Index);
}

bool RaftLog::match(LogIndex _Index, TermId _Term) const
{
    if (_Index == -1) {
        // 空日志
        return true;
    }

    if (_Index > _Last_index) {
        // 不在范围
        return false;
    }

    return at(_Index).getTerm() == _Term;
}

void RaftLog::push(const RaftLogEntry & _Log_entry)
{
    _Logs.emplace_back(_Log_entry);
    _Last_index = _Log_entry.getIndex();
    _Last_term = _Log_entry.getTerm();
}

} // namespace WW
