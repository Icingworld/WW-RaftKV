#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <zookeeper/zookeeper.h>

namespace WW
{

/**
 * @brief ZooKeeper 客户端
 */
class ZooKeeperClient
{
private:
    zhandle_t * _Zk_handle;                 // zookeeper 句柄
    static std::mutex _Mutex;               // 锁
    static std::condition_variable _Cv;     // 条件变量
    static bool _Connected;                 // 是否连接成功

public:
    ZooKeeperClient();

    ~ZooKeeperClient();

public:
    /**
     * @brief 连接到 ZooKeeper 服务
     * @param host ZooKeeper 服务器地址
     * @param timeout 超时时间
     */
    void connect(const std::string & host, int timeout = 3000);

    /**
     * @brief 创建一个 znode 节点
     * @details 无法递归创建，其父节点必须存在
     * @param path 节点路径
     * @param data 节点数据
     * @param ephemeral 是否是临时节点
     * @return 是否创建成功
     */
    bool create(const std::string & path, const std::string & data, bool ephemeral = false);

    /**
     * @brief 递归创建一个 znode 节点
     * @param path 节点路径
     * @param data 节点数据
     * @param ephemeral 是否是临时节点
     * @return 是否创建成功
     */
    bool createRecursive(const std::string & path, const std::string & data, bool ephemeral = false);

    /**
     * @brief 获取指定 znode 节点的值
     * @param path 节点路径
     * @return 节点值
     */
    std::string getData(const std::string & path);

    /**
     * @brief 获取路径所有子节点
     * @param path 节点路径
     * @return 是否获取成功
     */
    bool getChildren(const std::string & path, std::vector<std::string> & childs);

private:
    /**
     * @brief 全局 ZooKeeper 观察器
     * 
     * @details ZooKeeper C API 所要求的 watcher 回调函数签名
     *   它会在以下几种情况下被触发：
     * - 会话状态改变（如连接建立、断开、重连等）
     * - 被监听节点的创建、删除、修改等事件发生
     * 
     * @param zh ZooKeeper 句柄
     * @param type 事件类型（如 ZOO_CREATED_EVENT, ZOO_DELETED_EVENT 等）
     * @param state 会话状态（如 ZOO_CONNECTED_STATE, ZOO_EXPIRED_SESSION_STATE 等）
     * @param path 触发事件的节点路径
     * @param watcher_ctx 注册 watcher 时传入的上下文
     */
    static void watcher(zhandle_t * zh, int type, int state, const char * path, void * watcher_ctx);

    /**
     * @brief 抛出运行时异常
     * @exception `std::runtime_error`
     */
    [[noreturn]] void _ThrowRuntimeError(const std::string & message) const;
};

} // namespace WW
