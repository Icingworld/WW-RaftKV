#include "RaftLog.h"

#include <stdexcept>

namespace WW
{

RaftLog::RaftLog()
    : _Logs()
    , _Base_index(1)
{
}

LogIndex RaftLog::getLastIndex() const
{
    return _Base_index + _Logs.size() - 1;
}

TermId RaftLog::getLastTerm() const
{
    if (_Logs.empty()) {
        return 0;
    }

    return _Logs.back().getTerm();
}

TermId RaftLog::getTerm(LogIndex _Index) const
{
    if (_Index < _Base_index || _Index > getLastIndex()) {
        return 0;
    }

    return _Logs.at(_Index - _Base_index).getTerm();
}

const RaftLogEntry & RaftLog::at(LogIndex _Index) const
{
    if (_Index < _Base_index || _Index > getLastIndex()) {
        throw std::out_of_range("Index out of range in RaftLog::at");
    }

    return _Logs.at(_Index - _Base_index);
}

bool RaftLog::match(LogIndex _Index, TermId _Term) const
{
    if (_Index == 0) {
        // 空日志
        return true;
    }

    if (_Index < _Base_index || _Index > getLastIndex()) {
        // 不在范围
        return false;
    }

    return at(_Index).getTerm() == _Term;
}

void RaftLog::append(const RaftLogEntry & _Log_entry)
{
    _Logs.emplace_back(_Log_entry);
}

void RaftLog::truncate(LogIndex _Truncate_index)
{
    if (_Truncate_index < _Base_index || _Truncate_index > getLastIndex()) {
        // 不在范围内
        return;
    }

    // 通过 resize 丢弃指定范围日志
    _Logs.resize(_Truncate_index - _Base_index);
}

std::vector<RaftLogEntry> RaftLog::getLogFrom(LogIndex _Index) const
{
    std::vector<RaftLogEntry> tmp;

    if (_Index < _Base_index || _Index > getLastIndex()) {
        return tmp;
    }

    tmp.assign(_Logs.begin() + _Index - _Base_index, _Logs.end());
    return tmp;
}

} // namespace WW
