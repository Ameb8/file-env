# File Env

File Env is a file management simulator, written to learn about how operating systems manage user's files and data. The libFS2025 files provide an interface designed to be similar to that provided by POSIX systems, albeit with a much simpler implementation. Additionally, the project contains a menu-driven terminal user interface, where files can be created, deleted, written to, and read from. This is designed to be a very basic version of a user-space file editor. For example, when a file is written to from the systems terminal interface, opening the file before and closing after is abstracted away form the user. However, libFS_2025 simulates the file opening and closing, and of course must open and close the file on the host system as well. An example usage of this program can be seen below, where I create, write to, read, and delete a file.

<video width="640" controls>
  <source src="https://username.github.io/repo-name/demo/video.mp4" title="file-env-demo" type="video/mp4">
  Your browser does not support the video tag.
</video>

If the video does not display above, [click here] (<video controls src="https://ameb8.github.io/file-env/demo/file-env-demo.mp4" title="Title"></video>)

If the video does not display above, [click here] (https://username.github.io/repo-name/demo/video.mp4)
