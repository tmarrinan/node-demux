var node_demux = require('./build/Release/node_demux');

function demux() {
	this.video = new node_demux.VideoDemux();
	
	this.loadVideo = function(filename) {
		this.video.LoadVideo(filename);
	};
	
	this.startDemuxing = function() {
		this.video.StartDemuxing();
	};
	
	this.seekVideo = function(frameIdx) {
		this.video.SeekVideo(frameIdx);
	};
	
	this.on = function(type, cb) {
		if (type === "error" || type == "metadata" || type === "start" || type === "end" || type === "frame") {
			this.video.On(type, cb);
		}
	}
}

module.exports = demux;
