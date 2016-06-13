node-demux
==========

Node-Demux is a node.js package that decodes video files into raw frames in real-time. This is useful for creating a video player.


### Prerequisites ###
FFMpeg (for developers)
* Windows
    * Download [FFMpeg](http://ffmpeg.zeranoe.com/builds/) (Shared and Dev - 64bit)
    * Use 7-zip to extract downloads
    * Create folder 'C:\Dev' if it does not already exist
    * Move extracted FFMpeg Shared folder to 'C:\Dev\ffmpeg-win64-shared'
    * Move extracted FFMpeg Dev folder to 'C:\Dev\ffmpeg-win64-dev'
    * Add 'C:\Dev\ffmpeg-win64-shared\bin' to your system `Path` variable
* Mac OS X
    * `brew install ffmpeg --with-libvpx --with-libvorbis --with-ffplay`
* Linux
    * Install FFMpeg and FFMpeg-Dev from your package manager (may need at add repository prior to install)


### Install ###

* `npm install node-demux`
    * Note: Windows users may need option to specify compiler `--msvc_version=2013`


### Usage ###

declare new demuxer: `var video = new demux();`

| Method                            | Use |
|-----------------------------------|-----|
| `video.load(filename, options)`   | loads video from file (`filename` is path or url to desired file, `options` is an optional object) |
| `video.play()`                    | starts demuxing video (callback for `'start'` when it begins and `'frame'` on each new decoded frame |
| `video.pause(callback)`           | pauses demuxing (`callback` is called once video successfully pauses decoding |
| `video.stop(callback)`            | stops demuxing (same as pause, but also seeks video back to start) |
| `video.seek(timestamp, callback)` | seeks video to desired time in seconds (`callback` is called once video successfully finished seek) |
| `video.nextFrame()`               | decodes next frame as soon as possible (`'frame'` message will be called) |
| `video.on(message)`               | `message` is one of `'error'`, `'metadata'`, `'start'`, `'end'`, or `'frame'` |


### Load Options ###
| Option                            | Meaning |
|-----------------------------------|---------|
| `decodeFirstFrame`                | boolean flag: decodes one frame after load, seek, or stop |
| `colorspace`                      | string: desired frame colorspace (`'yuv420p'` or `'rgb24'`) |


### Example ###

```
var demux = require('node-demux');


var start;
var nframes;

var video = new demux();
video.on('error', function(err) {
    console.log(err);
});
video.on('metadata', function(metadata) {
	nframes = 0;
    console.log(metadata);
});
video.on('start', function() {
    start = Date.now();
    console.log("start demuxing");
});
video.on('end', function() {
    var time = (Date.now() - start) / 1000;
    console.log("finished demuxing");
    console.log("  total time: " + time.toFixed(3) + "sec");
    console.log("  average frames per second: " + (nframes/time).toFixed(3) + "fps");
});
video.on('frame', function(frameIdx, data) {
    nframes++;
    console.log("received frame " + frameIdx + " (size: " + data.length + ")");
});
video.load("big-buck-bunny_trailer.mp4");
video.play();
```
