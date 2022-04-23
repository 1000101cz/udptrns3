./server new_video_local_run.mp4  &>/dev/null &
sleep .2
./client 127.0.0.1 template_files/video.mp4
rm new_video_local_run.mp4