# File Env

File Env is a file management simulator, written to learn about how operating systems manage user's files and data. The libFS2025 files provide an interface designed to be similar to that provided by POSIX systems, albeit with a much simpler implementation. Additionally, the project contains a menu-driven terminal user interface, where files can be created, deleted, written to, and read from. This is designed to be a very basic version of a user-space file editor. For example, when a file is written to from the systems terminal interface, opening the file before and closing after is abstracted away form the user. However, libFS_2025 simulates the file opening and closing, and of course must open and close the file on the host system as well. 

An example usage of this program can be [viewed here.] (https://Ameb8.github.io/file-env/demo/file-env-demo.mp4)


The below image shows the process of creating a file, attempting to read while empty, and entering the editor in order to write text.

![file-env demo screenshot 1](https://github.com/Ameb8/file-env/demo/create-write-demo.png)

Once the editor is entered, it functions as a very simple version of nano. ctrl+x can be used to save and quit, while ctrl+q quits without saving. 

![file-env-demo screenshot 2](https://github.com/Ameb8/file-env/demo/editor-demo.png)

Next is an example of reading the just created file, both from the program's TUI, and the underlying stored file in the .fsdata directory.

![file-env-demo screenshot 2](https://github.com/Ameb8/file-env/demo/read-demo.png)


Files can also be deleted, both in the simulated filesystem and on the host system.

![file-env-demo screenshot 2](https://github.com/Ameb8/file-env/demo/read-demo.png)
