# Smallchat

TLDR: This is just a programming example for a few friends of mine. Long story follows.

Yesterday I was talking with a few friends of mine, front-end developers mostly, that are a bit far from system programming. We were remembering the old times of IRC. And inevitably I said: to write a very simple IRC server is an experience everybody should do. There are very interesting parts in a program like that. A single process doing multiplexing, taking the client state, that can be done in different ways, and so forth.

But then the discussion evolved and I thought, I'll show you a very minimal example in C. But what is the smallest chat server you can write? For starters to be truly minimal we should not require any proper client. Even if not very well, it should work with `telnet` or `nc` (netcat). The server main operation is just to receive some chat line and send it to all the other clients, in what is sometimes called a fan-out operation. But yet, this would require a proper readline() function, then buffering, and so forth. We want it simpler: let's cheat using the kernel buffers, and pretending we every time receive a full-formed line from the client (an assumption that is in the practice often true, so things kinda work).

Well, with this tricks we can implement a chat that even has the ability to let the user set their nick in just 200 lines of code (removing spaces and comments, of course). Since I wrote this little program as an example for my friends, I decided to also push it here on Github.


简而言之：这只是我为一些朋友提供的一个编程示例。接下来是长篇故事。

昨天，我与一些朋友聊天，他们大多是前端开发人员，对系统编程了解有限。我们回忆起了IRC的旧时光。不可避免地，我说：编写一个非常简单的IRC服务器是每个人都应该尝试的经历。在这样的程序中有一些非常有趣的部分。一个进程执行多路复用，获取客户端状态，可以用不同的方式实现，等等。

然后，讨论逐渐发展，我想，我会向你展示一个极简的C语言示例。但是，你能写出最简单的聊天服务器吗？要真正做到最简，我们不应该要求任何适当的客户端。即使不是很好，它应该可以使用telnet或nc（netcat）运行。服务器的主要操作只是接收一些聊天信息并将其发送给所有其他客户端，这有时被称为扇出操作。但是，这仍然需要一个适当的readline()函数，然后缓冲等等。我们希望更简单：让我们通过使用内核缓冲区来作弊，并假装每次都从客户端接收到一个完整的行（实际上这种假设通常成立，所以事情有点能够运作）。

嗯，通过这些技巧，我们可以实现一个聊天程序，甚至可以让用户在只有200行的代码中设置他们的昵称（当然，去掉了空格和注释）。由于我编写了这个小程序作为我的朋友的示例，我决定也将其发布在GitHub上。

## usage
### 连接到聊天室
```sh
telnet localhost 7711
```
### 设置昵称
```sh
/nick <nickname>
```