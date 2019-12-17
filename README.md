# BeanCafiine

BeanCafiine is a Cafiine server for Linux written in C.
Cafiine allows modding Wii U games by implementing the filesystem over the network.
You'll need a Cafiine client (I recommend [Geckiine](https://github.com/OatmealDome/Geckiine)) installed on your Wii U.
Then run the server and Cafiine client, and point the client to your computer's IP.

Put your modified content in the `root` directory in the application's working directory (you can change the root directory using the command line parameter `--root=<path>`).
In the root folder should be the title ID of the application you want to modify, and under there you can put the files you wish to replace.

So to replace `/vol/content/Pack/Static.pack` for the EUR version of Splatoon: (note that replacing Splatoon content is likely to result in a ban if you play online - for this you'll want to use nohash, a fork of geckiine that prevents automated bans)
```
<root>/00050000-10176A00/vol/content/Pack/Static.pack
```

The title IDs and full paths of files being accessed are logged to the server's console.