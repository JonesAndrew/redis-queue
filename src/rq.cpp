#include <rq/rq>

std::vector<std::string> split(std::string s, std::string delimiter) {
    std::vector<std::string> result;
    size_t pos = 0;

    while ((pos = s.find(delimiter)) != std::string::npos) {
        result.emplace_back(s.substr(0, pos));
        s.erase(0, pos + delimiter.length());
    }
    result.emplace_back(s);

    return result;
}

std::string getQueueKey(std::string queue) {
    return "queue:"+queue;
}

std::string getProcessingQueueKey(std::string worker, std::string queue) {
    return "process:"+worker+":"+queue;
}

std::string getAckKey(std::string worker) {
    return "ack:"+worker;
}

std::string getQueuesKeys() {
    return "queues";
}


namespace rq {
    QueueClient::QueueClient(std::string host = "127.0.0.1", unsigned short port = 6379) {
        client.connect(host, port);
    }

    void QueueClient::size(std::string queue) {
        client.llen(getQueueKey(queue), [queue](cpp_redis::reply& reply) {
            std::cout << queue << " size: " << reply << std::endl;
        }).sync_commit();
    }

    void QueueClient::send(std::string queue, std::string task) {
        client.sadd(getQueuesKeys(), {queue}).commit();
        client.lpush(getQueueKey(queue), {task}).sync_commit();
    }

    void QueueClient::cleanOldQueues() {
        client.smembers("workers", [this](cpp_redis::reply& reply) {
            for (auto &a : reply.as_array()) {
                std::string worker = a.as_string();

                client.get(getAckKey(worker), [this, worker](cpp_redis::reply& reply) {
                    if (reply.is_null()) {
                        unAck(worker);
                    }
                }).commit();
            }
        }).sync_commit();
    }

    std::vector<std::string> QueueClient::getQueues() {
        std::vector<std::string> queues;

        client.smembers(getQueuesKeys(), [this, &queues](cpp_redis::reply& reply) {
            for (auto &a : reply.as_array()) {
                queues.emplace_back(a.as_string());
            }
        }).sync_commit();

        return queues;
    }

    void QueueClient::unAck(std::string worker) {
        for (std::string queue : getQueues()) {
            std::string key = getProcessingQueueKey(worker, queue);

            client.lrange(key, 0, -1, [this, queue](cpp_redis::reply& reply) {
                for (auto &a : reply.as_array()) {
                    send(queue, a.as_string());
                }
            }).del({key}).commit();
        }
    }



    QueueWorker::QueueWorker(std::string name, std::string host = "127.0.0.1", unsigned short port = 6379) :
        QueueClient(host, name),
        name(name)
    {
        client.sadd("workers", {name}).commit();
        unAck(name);
    }

    void QueueWorker::ack() {
        client.setex(getAckKey(name), 60, "ack").commit();
    }

    void QueueWorker::process(std::string queue) {
        ack();

        std::string task;
        client.brpoplpush(getQueueKey(queue), getProcessingQueueKey(name, queue), 0, [&task](cpp_redis::reply& reply) {
            task = reply.as_string();
        }).sync_commit();

        std::vector<std::string> args = split(task, ":");
        auto it = functions.find(args[0]);
        if (it != functions.end()) {
            it->second({args.begin() + 1, args.end()});
            finished(queue, task);
        } else {
            finished(queue, task);
            send(queue, task);
        }
    }

    void QueueWorker::finished(std::string queue, std::string task) {
        ack();

        client.lrem(getProcessingQueueKey(name, queue), -1, task).commit();
    }

    void QueueWorker::addTask(std::string task, std::function<void(std::vector<std::string>)> f) {
        functions[task] = f;
    }
}
