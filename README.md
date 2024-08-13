# WebServerInC
A very simple web server in C built using OS system calls.

I've included both server code as well as a compiled executable that works on Mac M1. If you would also like to run this code but compile your own version you can compile it using GCC. Just run the following command in the project directory.

```gcc -o tcp_server tcp_server.c```

You can then run the executable that was generated using this command

```./tcp_server```

This should result in a server successfully started on port 2000 message. You should be able to visit the site on http://localhost:2000/index.html

If you experience errors when playing with the site first ensure that you are using Google Chrome. For some reason Safari results in the POST requests never returning a response. Also ensure that your index.html file lives in the same folder as the tcp_server executable as it will need to read the index.html in order to display the message board.

If you are not using Mac or Linux you can look into using different software such as Visual Studio for Windows. Have not tested it on Windows though so I am unsure if it will work there.
