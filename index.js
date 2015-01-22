var node_demux = require('./build/Release/node_demux');

var messages = [
	"error",
	"metadata",
	"start",
	"end",
	"frame"
];

function demux() {
	this.video = new node_demux.VideoDemux();
	
	this.load = function(filename) {
		this.video.LoadVideo(filename);
	};
	
	this.play = function() {
		this.video.StartDemuxing();
	};
	
	this.pause = function() {
		this.video.PauseDemuxing();
	};
	
	this.stop = function() {
		var _this = this;
		this.video.StopDemuxing(function() {
			_this.video.VideoStopped();
		});
	};
	
	this.seek = function(timestamp, cb) {
		if(!this.video.IsBusy()){
			this.video.SeekVideo(timestamp, cb);
		}
		else {
			var _this = this;
			setImmediate(function() {
				_this.seek(timestamp, cb);
			});
		}
	};
	
	this.on = function(type, cb) {
		if (messages.indexOf(type) >= 0) {
			this.video.On(type, cb);
		}
	};
}

module.exports = demux;
