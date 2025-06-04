#include "RaftLog.h"

#include <stdexcept>
#include <iostream>

namespace WW
{

RaftLog::RaftLog()
    : _Logs()
    , _Base_index(1)
    , _SnapShot_index(1)
    , _SnapShot_term(0)
{
}

LogIndex RaftLog::getLastIndex() const
{
    return _Base_index + _Logs.size() - 1;
}

LogIndex RaftLog::getBaseIndex() const
{
    return _Base_index;
}

TermId RaftLog::getLastTerm() const
{
    if (_Logs.empty()) {
        return _SnapShot_term;
    }

    return _Logs.back().getTerm();
}

TermId RaftLog::getTerm(LogIndex _Index) const
{
    if (_Index < _Base_index || _Index > getLastIndex()) {
        if (_Index == _SnapShot_index) {
            return _SnapShot_term;
        }

        return 0;
    }

    return _Logs.at(_Index - _Base_index).getTerm();
}

LogIndex RaftLog::getSnapShotIndex() const
{
    return _SnapShot_index;
}

TermId RaftLog::getSnapShotTerm() const
{
    return _SnapShot_term;
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

    if (_Index == _SnapShot_index) {
        return _SnapShot_term == _Term;
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

void RaftLog::truncateAfter(LogIndex _Truncate_index)
{
    if (_Truncate_index < _Base_index || _Truncate_index > getLastIndex()) {
        // 不在范围内
        return;
    }

    // 通过 resize 丢弃指定范围日志
    _Logs.resize(_Truncate_index - _Base_index);
}

void RaftLog::truncateBefore(LogIndex _Truncate_index)
{
    if (_Truncate_index <= _Base_index || _Truncate_index > getLastIndex() + 1) {
        // 不需要截断，或者超出日志范围
        return;
    }

    // 计算截断的数量
    size_t offset = _Truncate_index - _Base_index;

    // 删除前 offset 条日志
    if (offset > 0) {
        // 更新 snapshot term/index 为截断前最后一条日志
        _SnapShot_index = _Truncate_index - 1;
        _SnapShot_term = getTerm(_SnapShot_index);
        printf("snapshot index:%d, snapshot term:%zu\n", _SnapShot_index, _SnapShot_term);

        _Logs.erase(_Logs.begin(), _Logs.begin() + offset);
        _Base_index = _Truncate_index;
    }
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

void RaftLog::setSnapShotIndex(LogIndex _SnapShot_index)
{
    this->_SnapShot_index = _SnapShot_index;
    _Base_index = _SnapShot_index;
}

void RaftLog::setSnapShotTerm(TermId _SnapShot_term)
{
    this->_SnapShot_term = _SnapShot_term;
}

} // namespace WW
