

//lightweight objects that insulate different ways of playing/stopping.
//the bundle that is passed in is a MixedBundle
//these could be players in future, a superclass of AbstractPlayer would be required

AbstractPlayControl {
	var <source, <>channelOffset;
	var <paused=false;
	
	*new { arg source, channelOffset=0;
		^super.newCopyArgs(source, channelOffset);
	}
	
	
	writeDef {}
	prepareForPlay {}
	build { ^true }
	pause { this.stop } 
	resume { this.start }
	synth { ^nil }
	
	readyForPlay { ^true }
	synthDef { ^nil }

	sendDefToBundle {}
	spawnToBundle {}
	
	playToBundle { arg bundle, args; 
		bundle.addAction(this, \play); //no latency (latency is in stream already)
		^nil //return a nil object instead of a synth
	}
	
	freeToBundle { arg bundle;
		bundle.addAction(this, \free);
	}
	
	resumeToBundle { arg bundle, args;
		//...//
	}
	
	stopToBundle { arg bundle;
		bundle.addAction(this, \stop);
	}
	
	stopClientToBundle { arg bundle;
		//used in shared node proxy
		this.stopToBundle(bundle);
	}
	
	play {
		this.subclassResponsibility(thisMethod);
	}
	
	stop {
		this.subclassResponsibility(thisMethod);
	}
	
	free {}
	
}

StreamControl : AbstractPlayControl {
	var <stream, clock;
		
	play {
		stream.stop;
		stream.play(nil, false, 0); //(clock);
	}
	
	stop {
		stream.stop;
	}
	pause { stream.pause; paused=true }
	resume { stream.resume; paused=false }
	
	free { stream.stop; stream = nil }
	
	playToBundle { arg bundle, args; 
		if(paused.not and: {stream.isPlaying.not}, {
			bundle.addAction(this, \play); //no latency (latency is in stream already)
		})
		^nil //return a nil object instead of a synth
	}
	
	build { arg proxy;
		stream = source.buildForProxy(proxy, channelOffset);
		clock = proxy.clock;
		paused = proxy.paused;
		^stream.notNil;
	}
	readyForPlay { ^stream.notNil }
	clear {
		stream = nil;
	}
	
}


SynthControl : AbstractPlayControl {
	var <synth, >canReleaseSynth=true, >canFreeSynth=true;
	
	
	writeDef { }
	
	sendDefToBundle {} //assumes that SynthDef is loaded in the server 
	name { ^source }
	clear { synth = nil }
	build { arg proxy;
		^proxy.initBus(proxy.rate ? \audio, proxy.numChannels ? 2);
	}
	spawnToBundle { arg bundle, extraArgs, target, addAction=0; //assumes self freeing
		var synthMsg;
		synthMsg = [9, this.name, -1, addAction, target.asTarget.nodeID]++extraArgs; 
		bundle.add(synthMsg);
	}
	
	playToBundle { arg bundle, extraArgs, target, addAction=\addToTail;
		synth = Synth.newToBundle(bundle, this.name, extraArgs, target.asTarget, addAction);
		if(paused) { synth.msgToBundle(bundle, "/n_run", #[0])};
		synth.isPlaying = true;
		^synth
	}
	
	
	stopToBundle { arg bundle;
		if(synth.isPlaying, {
			if(this.canReleaseSynth, {
					bundle.add([15, synth.nodeID, \gate, 0.0]); //to be sure.
			}, {
					if(this.canFreeSynth.not, { bundle.add([11, synth.nodeID]) }); //"/n_free"
			});
			synth.isPlaying = false;
		});
		

	}

	play { arg group, extraArgs;
		var bundle;
		bundle = MixedBundle.new;
		this.playToBundle(bundle, extraArgs, group);
		bundle.send(group.server)
		^synth
	}
	stop { arg latency;
		var bundle;
		bundle = List.new;
		this.stopToBundle(bundle);
		synth.server.listSendBundle(latency, bundle)
	}
			
	stopClientToBundle { }  // used in shared node proxy
	
	pause { if(synth.notNil) {synth.run(false)} }
	resume { if(synth.notNil) { synth.run(true) } }
	canReleaseSynth { ^canReleaseSynth }
}


SynthDefControl : SynthControl {
	classvar <>writeDefs=true;
	var <synthDef;
	
	readyForPlay { ^synthDef.notNil }
	canReleaseSynth { ^synthDef.canReleaseSynth }
	canFreeSynth { ^synthDef.canFreeSynth }
	
	build { arg proxy, index=0; 
		var ok, rate, numChannels;
		synthDef = source.buildForProxy(proxy, channelOffset, index);
		rate = synthDef.rate ?? { if(proxy.rate !== \control) { \audio } { \control } };
		numChannels = synthDef.numChannels ? proxy.numChannels ? 2;
		ok = proxy.initBus(rate, numChannels);
		[rate, numChannels].debug;
		if(ok && synthDef.notNil, { 
			paused = proxy.paused;
			// schedule to avoid hd sleep latency. 
			// def writing is only for server boot.
			if(writeDefs, {
				AppClock.sched(rrand(0.01, 0.3), { this.writeDef; nil });
			}); 
		}, { 
			synthDef = nil; 
		})
	}
	
	free {�synthDef = nil }
	
	sendDefToBundle { arg bundle, server;
		bundle.addPrepare([5, synthDef.asBytes]); //"/d_recv"
	}
	

	writeDef {
		synthDef.writeDefFile;
	}
	
	name { ^synthDef.name }
	
	
}

SoundDefControl : SynthDefControl {
	
	writeDef { }
	build { synthDef = source.synthDef; ^true }
	sendDefToBundle {} //assumes that SoundDef does send to the same server 
}



CXPlayerControl : AbstractPlayControl {
	
	//here the lazy bus init happens and allocation of client side ressources
	//there is no bundle passing in build
	
	build { arg proxy;
		var bus;
		if(proxy.isNeutral.not, {�bus = proxy.asBus });
		source.prepareForPlay(nil, true, bus);
		paused = proxy.paused;
		^proxy.initBus(source.rate ? 'audio', source.numChannels ? 2)
	}
	
	playToBundle { arg bundle, extraArgs, proxy, addAction=\addToHead;
		
		// we'll need channel offset maybe.
		if(source.patchOut.isNil)
		 { source.prepareForPlay(nil, true, proxy.asBus) };
		source.spawnToBundle(bundle); //won't work with saved proxy!
		bundle.addSchedFunction({ this.moveIn(proxy) }, 0.3);
		// if(paused) { bundle.addAction(source, \pause) };
		^nil
		
		
	}
	 
	stopToBundle { arg bundle;
		//source.stopToBundle(bundle);
		source.releaseToBundle(nil, bundle);
		//bundle.addFunction({ source.release;  });  //kompromiss.
	}
	freeToBundle {}
	
	stop {
		source.stop;
	}
	play {
		source.play;
	}
	
	free { 
		source.stop;
	}
	pause { source.stop }
	resume { source.play }
	moveIn {}
	
}

CXSynthPlayerControl : CXPlayerControl {
	
	/*
	stopToBundle { arg bundle, fadeTime=0.5;
		//	source.releaseAndFreeToBundle(fadeTime, bundle); //free it each time for now.
		source.releaseToBundle(nil, bundle);
	}
	freeToBundle {}
	*/
	// ??
	moveIn { arg proxy; source.children[0].synth.moveToTail(proxy.group); }
	
}

