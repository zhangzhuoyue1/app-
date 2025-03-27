// Singleton模式，保证一个类只有一个实例，并提供一个全局访问点。

#include <mutex>

template <typename T>
class Singleton
{
public:
    static T *getInstance()
    {
        // 使用双重检查锁，保证线程安全
        if (instance == nullptr)
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (instance == nullptr)
            {
                instance = new T();
            }
        }
        return instance;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;

    Singleton(const Singleton<T> &) = delete;
    Singleton &operator=(const Singleton<T> &) = delete;

    static T *instance;
    static std::mutex mutex;
};

template <typename T>
T *Singleton<T>::instance = nullptr;