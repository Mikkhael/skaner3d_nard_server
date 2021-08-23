#pragma once

#include "asiolib/all.hpp"
#include <queue>
#include <mutex>

template<typename Request, typename Session>
class async_request_queue
{
    
    std::queue<Request> queue;
    std::mutex mutex;
    bool isPerforming = false;
    
    asio::io_context& ioContext;
    Session& session;
    
    
    void perform(Request&& request){
        asio::post(ioContext, [me = session.shared_from_this(), request]{
            me->performRequest(request);
        });
    }
    
public:

    void performNext(){
        std::lock_guard<std::mutex> lock{mutex};
        if(queue.size() == 0){
            isPerforming = false;
            return;
        }
        perform(queue.frong());
        queue.pop();
    }
    
    void enqueueAndPerform(Request&& request)
    {
        bool shoudStartPerforming = false;
        
        {
            std::lock_guard<std::mutex> lock{mutex};
            queue.push(request);
            if(!isPerforming){
                isPerforming = true;
                shoudStartPerforming = true;
            }
        }
        
        if(shoudStartPerforming){
            performNext();
        }
    }
    
    async_request_queue(asio::io_context& ioContext, Session& session)
        : ioContext(ioContext), session(session)
    {}
    
};