#pragma once

#include <string>

#include <RaftCommon.h>

namespace WW
{

/**
 * @brief 同伴
*/
class RaftPeer
{
private:
    NodeId _Id;             // 节点 ID
    LogIndex _Next_index;   // 应该发送的下一个日志索引
    LogIndex _Match_index;  // 对方已确认的最后日志索引

public:
    explicit RaftPeer(NodeId _Id);

public:
    /**
     * @brief 获取节点 ID
    */
    NodeId getId() const;

    /**
     * @brief 获取应该发送的下一个日志索引
    */
    LogIndex getNextIndex() const;

    /**
     * @brief 获取对方已确认的最后日志索引
    */
    LogIndex getMatchIndex() const;

    /**
     * @brief 设置应该发送的下一个日志索引
    */
    void setNextIndex(LogIndex _Next_index);

    /**
     * @brief 设置对方已确认的最后日志索引
    */
    void setMatchIndex(LogIndex _Match_index);
};

} // namespace WW
