#pragma once
#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

/**
 * Recommend that the queue template instantiation uses
 * boost::shared_ptr<your_class>, to avoid having to
 * clean up allocated memory after queued items are
 * processed
 *
 * example instance declaration
 * MessageSharedQueue< boost::shared_ptr<MyClass> > m_MessageSharedQueue;
 */
template<typename TQueueItem>
class MessageSharedQueue
{
public:
	MessageSharedQueue(){};
	virtual ~MessageSharedQueue(){};

	const size_t put(TQueueItem queueItem)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_queue.push(queueItem);

		const size_t result =  m_queue.size();
		m_wait.notify_one();

		return result;
	};

	TQueueItem blockingTake(size_t& sizeAfterTake)
	{
		boost::mutex::scoped_lock lock(m_mutex);
		while(m_queue.empty())
		{
			m_wait.wait(lock);
		}

		TQueueItem queueItem = m_queue.front();
		m_queue.pop();

		sizeAfterTake = m_queue.size();
		return queueItem;
	};

	TQueueItem maxWaitTake(bool& isItemValid, size_t& sizeAfterTake, const size_t& maxWaitMs = 1)
	{
	    boost::mutex::scoped_lock lock(m_mutex);

	    if(!m_queue.empty())
	    {
	        TQueueItem queueItem = m_queue.front();
	        m_queue.pop();

	        sizeAfterTake = m_queue.size();
	        isItemValid = true;
	        return queueItem;
	    }
	    else
	    {
            const bool waitResult = m_wait.timed_wait(lock, boost::posix_time::milliseconds(maxWaitMs));
            if(waitResult && !m_queue.empty())
            {
                TQueueItem queueItem = m_queue.front();
                m_queue.pop();

                sizeAfterTake = m_queue.size();
                isItemValid = true;
                return queueItem;
            }
	    }

	    sizeAfterTake = 0;
        isItemValid = false;
        return TQueueItem();
	}

	bool maxWaitForNonEmpty(const size_t& maxWaitMs)
	{
	    if(!m_queue.empty())
	    {
	        return true;
	    }
	    else
	    {
	        boost::mutex::scoped_lock lock(m_mutex);
	        return m_wait.timed_wait(lock, boost::posix_time::milliseconds(maxWaitMs));
	    }
	}

	bool isEmpty() const
	{
	    return m_queue.empty();
	}
	const size_t getSize()
	{
		return m_queue.size();
	}

private:
	std::queue<TQueueItem> m_queue;

	boost::mutex m_mutex;
	boost::condition m_wait;
};
