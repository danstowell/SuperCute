+Function {

	send { arg key, server, mix=false, onComplete, loadLatency;
		var proxy;
		server = server ? Server.local;
		proxy = Ndef.new(key, nil, server);
		proxy.put(this, 0, true, mix.not, onComplete, loadLatency);
		^proxy
	}
	
	sendAdd { arg key, server;
		^this.send(key, server, true)
	}

}

+Symbol {
	play { arg outBus=0, nChan=2, server;
		var proxy;
		server = server ? Server.local;
		proxy = Ndef.new(this, nil, server).play(outBus, nChan);
	}


}