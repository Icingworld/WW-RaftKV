#pragma once

#include <string>

#include <RaftCommon.h>

namespace WW
{

/**
 * @brief 日志条目
*/
class RaftLogEntry
{
private:
    std::string _UUID;      // 客户端 UUID
    uint64_t _Sequence_id;  // 请求序列号
    TermId _Term;           // 日志任期
    std::string _Command;   // 日志命令

public:
    RaftLogEntry() = default;

    RaftLogEntry(TermId _Term, const std::string & _Command, const std::string & _UUID = "", uint64_t _Sequence_id = 0);

public:
    const std::string & getUUID() const;

    uint64_t getSequenceID() const;

    /**
     * @brief 获取任期
    */
    TermId getTerm() const;

    /**
     * @brief 获取命令
    */
    const std::string & getCommand() const;
};

} // namespace WW
