The function WSAEventSelect/WSAAsyncSelect are windows specifc and incases where applications are migrated from windows to linux need these api functionality can use this code from WSAEventSelectLinux(). The main moto of these functions is to provide a continuous monitor to check for any incoming connections or any read, write,close operations on a socket in asynchronous fashion making the main thread to continue without blocking for any activity.

##Function Description
WSAEventAccept(socketfd,void*objPtr),WSAEventAccept(socketfd,eventobject,void* objPtr)
The above function is used to get continuous notiifications for any incoming connections on a port. since epoll_wait() is a blocking call here as timeout is set to -1, this function is launched in a seperate thread.

This function can be modified if you want to use events to be set when ever an network activity is notified.(This helps when porting windows WSAsocket functions on to linux).

when events are used this function sets it and again continues to wait for connections, if you want to directly call functions when an network activity occurs then you can launch the functions in async threads so that this monitoring thread will not go and perform the operations so that multiple events at the same time can be entertained.

##Function Description
WSAEventRWX(socketfd),WSAEventRWX(socketfd,eventObject)
This function is used to get notifications for three network events,Read,Write,Close on a socket descriptor.

Read() can be blocking call based on the way it uses recv(), so Read can be called in a newthread so that the main thread for monitoring network events will not be blocked.

Most of the cases it's the users control on how & when to write to a fd. but read isn't actually at the  users control as the other end can write anytime,so monitoring read and close activities is demonstarted in this function.

##Function Description
WSAEventSelectLinux(socketfd,type_of_flag_for_events),WSAEventSelectLinux(socketfd,event_Obj,type_of_flag_for_events)
This function can take two types of flags, if you want to monitor for incoming connections then FD_ACCEPT flag is passed and for RWX(Read,Write,Close) events FD_READ_WRITE_CLOSE is passed.
