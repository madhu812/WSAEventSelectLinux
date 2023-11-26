#include <vector>
#include <future>
class WSA
{
    public:
    static bool WSAEventAccept(int sockFd, void * objPtr);
    static bool WSAEventRWX(int sockFd);
    bool WSAEventSelectLinux(int &sockFd, int &sockEvntFlag);
    bool Listen();
    bool Connect();
    static void Accept_Incoming();
    static void Read();
    static void Close();
    protected:
    std::vector<std::future<bool>> m_resultFromWSAEventAccept;
    std::vector<std::future<bool>> m_resultFromWSAEventRWX;
};